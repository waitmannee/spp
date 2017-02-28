#include "ibdcs.h"
#include "extern_function.h"

//forward declaration
static void dcsComExit(int signo);
static void dcsNetWriterExit(int signo);
static int  WriteTrace(const char *,int,char);
static int  g_iSock       = -1;
static int  g_pidNetWriter= -1;
static int  gs_tracefd    = -1;
static int  gs_tracesemid = -1;
static int  gs_myfid      = -1;
static int  gs_monifid    = -1;
static int  gs_timerid    = -1;

static int CloseTrace();
int NotifyMonitor(int fConnected);
static int OpenLog(char *IDENT);
static int OpenTrace();
static int CreateMyFolder();
static int MakeConnection();
int WriteNet();
int ReadNet();


int main_dcscom(int argc, char *argv[])
{
    //prepare the logging stuff
    if(0 > OpenLog(argv[0]))
        exit(1);
        
    dcs_log(0,0, "dcscom is starting up...\n");        

    catch_all_signals(dcsComExit);
    
    //attach to SHM of IBDCS
    if(dcs_connect_shm() < 0)
    {
        dcs_log(0,0,"dcs_connect_shm() failed:%s\n",strerror(errno));
        exit(1);
    }
    
    if(g_pIbdcsShm->is_MoniPid != getppid())
    {
        dcs_log(0,0,"invalid invoke of '%s'\n",argv[0]);
        exit(1);
    }

    //open the trace file so we can keep all messages
    if(OpenTrace() < 0)
    {
        dcs_log(0,0,"cannot open message trace file:%s.\n",strerror(errno));
        goto lblExit;
    }

    // attach to folder system and create folder of myself
    if( CreateMyFolder() < 0)
        goto lblExit;

    dcs_log(0,0,"*************************************************\n"
            "!!        dcscom startup completed.           !!\n"
            "*************************************************\n");

    //try to establish connection to SWITCH
    dcs_set_sysstat(COMPROC_STATUS,1); 
    g_iSock = MakeConnection();
    if(g_iSock < 0)
        dcsComExit(0);

    //we use 2 processes to do this business:
    //NetWriter(child):   read from my folder and write message to net
    //NetReader(parent):  read from net

    if((g_pidNetWriter = fork()) == 0) //child as NetWriter
    {
        //read from C_Queue and write to net
        catch_all_signals(dcsNetWriterExit);
        WriteNet();
        dcsNetWriterExit( SIGTERM );
    }

    if(g_pidNetWriter < 0)
    {
        dcs_log(0,0,"cannot fork new process:%s.\n",strerror(errno));
        goto lblExit;
    }

    //parent,namely NetReader here, first we notify monitor that connection is 
    //established, then we read from net in a infinite loop
    NotifyMonitor(TRUE); 
    ReadNet();
    
lblExit:
    dcsComExit(0);
    
    return -1;
}//main()

static void dcsComExit(int signo)
{
    if(signo > 0)
        dcs_debug(0,0,"catch a signal %d\n",signo);

    if(signo !=0 && signo != SIGTERM && signo != SIGCHLD)
        return;
    if(signo == SIGCHLD)
    {
        int who = wait(NULL);  //prevent zombie
        if(who == g_pidNetWriter)
            g_pidNetWriter = -1;
    }    

    //notify Monitor that connection lost
    NotifyMonitor(FALSE);

    //close the socket and all files
    if(g_iSock >= 0)  tcp_close_socket(g_iSock);

    //parent should notify its child
    if(g_pidNetWriter > 0) kill(g_pidNetWriter, SIGTERM);
    
    dcs_log(0,0,"dcscom(ReadNet) terminated.\n");
    CloseTrace();
    dcs_log_close();
    exit(signo);
}

static void dcsNetWriterExit(int signo)
{
    if(signo > 0)
        dcs_debug(0,0,"catch a signal %d\n",signo);
    if(signo != SIGTERM)
        return;
        
    tm_unset_timer(gs_timerid);

    //close the socket and all files
    if(g_iSock >= 0) tcp_close_socket(g_iSock);
    CloseTrace();
    dcs_log_close();
    
    dcs_log(0,0,"dcscom(WriteNet) terminated.\n");    
    exit(signo);
}

static int OpenLog(char *IDENT)
{
    char logfile[256];

    //assuming the log file is "$FEP_RUNPATH/log/dcscom.log"
    if(u_fabricatefile("log/dcscom.log",logfile,sizeof(logfile)) < 0) return -1;

    return dcs_log_open(logfile, IDENT);
}

static int CreateMyFolder()
{
    if(fold_initsys() < 0)
    {
        dcs_log(0,0, "cannot attach to folder system!");
        return -1;
    }

    gs_myfid = fold_create_folder(DCSCOM_FOLD_NAME);
    if(gs_myfid < 0)
        gs_myfid = fold_locate_folder(DCSCOM_FOLD_NAME);
    if(gs_myfid < 0)
    {
        dcs_log(0,0,"cannot create folder '%s':%s\n", DCSCOM_FOLD_NAME, strerror(errno) );
        return -1;
    }
    
    gs_monifid = fold_locate_folder(MONITOR_FOLD_NAME);
    
    if(gs_monifid < 0)
    {
        dcs_log(0,0,"cannot locate folder '%s': %s\n", MONITOR_FOLD_NAME,strerror(errno) );
        return -1;
    }

    return 0;
}

static int MakeConnection()
{
    int   iConnSock, i;
    
    dcs_log(0,0,"try to connect to SWITCH(%s:%d) with local port=%d...\n",
                                g_pIbdcsCfg->cSwIP,
                                g_pIbdcsCfg->iSwPort,
                                g_pIbdcsCfg->iLocPort);
    for(i=0;;i++)
    {
        iConnSock = tcp_connet_server(g_pIbdcsCfg->cSwIP,
                                      g_pIbdcsCfg->iSwPort,
                                      g_pIbdcsCfg->iLocPort);
        if(iConnSock < 0)
        {
            if(i % 120 == 0)
                dcs_log(0,0,"connect to SWITCH failed:%s,try again...\n", strerror(errno));
            sleep(5);
            continue;
        }

        //connection established successfully
        dcs_log(0,0,"connection to SWITCH established successfully.\n");
        return(iConnSock);
    }//for(;;)
}

int NotifyMonitor(int fConnected)
{
    struct IBDCSPacket packet;
    
    packet.pkt_cmd   =  PKT_CMD_TCPCONN;
    packet.pkt_val   =  fConnected ? 1 : 0;
    packet.pkt_bytes =  0;
    
    if( fold_write(gs_monifid, -1, &packet, sizeof(packet)) < 0 )
    {
        dcs_log(0,0, "<NotifyMonitor> fold_write() failed: %s\n", strerror(errno));
        return -1;
    }    
    
    return 0;    
}

int ReadNet()
{
    //
    //read from net and write to folder of AppServer2
    //
    char   caBuffer[MAX_IBDCS_MSG_SIZE];
    int    nread, nwritten = 0;
    int    timeout = 0, n;
    static int gs_appfid = -1;
    struct IBDCSPacket *pktptr;
        
    #define _SZ  (sizeof(caBuffer) - sizeof(struct IBDCSPacket) + 1)
    
    OpenLog("dcscom(ReadNet)");
    dcs_log(0,0,"dcscom(ReadNet) startup finished.\n");
    
    if( g_pIbdcsCfg->iHeartBeating )
        timeout =  g_pIbdcsCfg->iT3 * 1000;
    else
        timeout =  -1;
    
    // locate folder id of AppServer2
    for(n=0 ;; n++ )
    {
        //Freax TO DO:
        gs_appfid = fold_locate_folder(APPSRV2_FOLD_NAME);
        if(gs_appfid >= 0)
            break;
        
        if( n == 0 )
            dcs_log(0,0,"locate folder '%s' failed, try again...\n",
                            APPSRV2_FOLD_NAME);
        sleep(2);
    }
    
    
    for(;;)
    {
        //read from network in blocking mode
        memset(caBuffer, 0, sizeof(caBuffer));
        pktptr = (struct IBDCSPacket *)caBuffer;
        dcs_debug(0,0,"going to read timeout = %d!\n",timeout);
        nread = read_msg_from_SWnet(g_iSock,pktptr->pkt_buf, _SZ, timeout);
        dcs_debug(0,0,"nread1:%d!\n",nread);
        if(nread < 0)
        {
            //read  socket error
	    close(g_iSock);
            break;
        }

        dcs_debug(pktptr->pkt_buf,nread,"Read Net %.4d bytes-->\n",nread);

        if(nread == 0)  //response of echo test
            continue;

        WriteTrace(pktptr->pkt_buf,nread,'R');

        pktptr->pkt_cmd   = PKT_CMD_DATAFROMSWITCH;
        pktptr->pkt_bytes = nread;
                
        //forward this message to AppServer2
        n = nread + sizeof(struct IBDCSPacket) - 1;
        nwritten = fold_write( gs_appfid, -1, pktptr, n);
        if(nwritten < 0)
        {
            close(g_iSock);
            dcs_log(0,0,"fold_write() failed:%s!\n",strerror(errno));
            break;
        }

        dcs_debug(pktptr,nwritten,"write AppServer2 %.4d bytes data!\n",nwritten);
    }//for(;;)

    return 0;
}


int WriteNet()
{
    //
    //read from my folder and write to net
    //
    char   caBuffer[MAX_IBDCS_MSG_SIZE];
    int    nread, nwritten, fromfid;
    struct IBDCSPacket *pktptr;
    int fid;
    OpenLog("dcscom(WriteNet)");
    dcs_log(0,0,"dcscom(WriteNet) startup finished.\n");

    if( g_pIbdcsCfg->iHeartBeating )
    {
        //register a timer to trigger me to send echo test to switch   
        int interval =  g_pIbdcsCfg->iT3 / 4 + 1;
        gs_timerid = tm_set_timer(gs_myfid, interval );
        
        if( gs_timerid < 0 )
        {
            dcs_log(0,0, "register timer failed: %s\n", strerror(errno));
            return 0;
        } 
    }
    fid = fold_open(gs_myfid);
    for(;;)
    {
        //read from my folder in blocking mode
        memset(caBuffer, 0, sizeof(caBuffer));
        nread = fold_read( gs_myfid,fid, &fromfid, caBuffer, sizeof(caBuffer), 1);
        if(nread < 0)
        {
//            dcs_log(0,0,"fold_read() failed:%s\n",strerror(errno));
	    close(g_iSock);
            break;
        }
        
        pktptr = (struct IBDCSPacket *)caBuffer;
        
        //write to net
        switch(pktptr->pkt_cmd)
        {
            case PKT_CMD_DATATOSWITCH:
                nread = (int)pktptr->pkt_bytes;
                break;

            case PKT_CMD_TIMER:
                //nread    = 0;
//                nread = iso_genmsg_test_net(pktptr->pkt_buf);                
                break;

            default:
                break;
        }//switch
        
        nwritten = write_msg_to_SWnet(g_iSock, pktptr->pkt_buf, nread,
                                    g_pIbdcsCfg->iT3 * 1000);
        if(nwritten < nread)
        {
//            dcs_log(0,0,"write_msg_to_net() failed:%s\n",strerror(errno));
	    close(g_iSock);
            break;
        }

        if(pktptr->pkt_cmd == PKT_CMD_TIMER)
            continue;

        dcs_debug(pktptr->pkt_buf,nwritten,"write Net %.4d bytes-->\n",nwritten);

        WriteTrace(pktptr->pkt_buf,nwritten,'S');
    }//for(;;)

    return 0;
}

////////////////////////////////////////////////////////////////////

static int OpenTrace()
{
    char tracefile[256];

    //assuming the log file is "$FEP_RUNPATH/data/msgtrace.log"
    if(u_fabricatefile("data/msgtrace.log",tracefile,sizeof(tracefile)) < 0)
        return -1;

    if(gs_tracefd < 0)
        gs_tracefd = open(tracefile,O_WRONLY|O_APPEND|O_CREAT,0666);
    if(gs_tracefd < 0)
        return -1;
    
    if(gs_tracesemid < 0)  gs_tracesemid = sem_create("0", 1);
    
    if(gs_tracesemid < 0)
    {
        close(gs_tracefd);
        return -1;
    }

    return 0;
}

static int CloseTrace()
{
	close(gs_tracefd);
	gs_tracefd = -1;
	sem_delete(gs_tracesemid);
	gs_tracesemid = -1;
    return 0;
}

static int WriteTrace(const char * msgTrace,int nbytes,char cDirect)
{
    struct  tm *tm;
    time_t  t;
    char    caBuf[64];

    if(gs_tracefd < 0) return -1;

    //format the message
    time(&t);
    tm = localtime(&t);

    memset(caBuf,0,sizeof(caBuf));
    sprintf(caBuf,"%4d/%02d/%02d %02d:%02d:%02d %c(%.4dbytes) ",
            tm->tm_year+1900,
            tm->tm_mon + 1,
            tm->tm_mday,
            tm->tm_hour,
            tm->tm_min,
            tm->tm_sec,
            cDirect,
            nbytes);

    sem_lock(gs_tracesemid,0,TRUE);
    write(gs_tracefd, caBuf, strlen(caBuf));
    write(gs_tracefd, msgTrace, nbytes);
    write(gs_tracefd, "||\n", 3);
    sem_unlock(gs_tracesemid,0,TRUE);
    return nbytes;
}
