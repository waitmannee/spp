#include "ibdcs.h"
#include "tmcibtms.h"
#include "iso8583.h"
#include <unistd.h>

static int gs_myFid = -1;

static char   *g_pcBcdaShmPtr;
extern struct ISO_8583 *iso8583;
struct ISO_8583 iso8583_conf[128];

static void TransRcvSvrExit(int signo)
{
    if(signo > 0)
        dcs_log(0,0,"catch a signal %d\n",signo);
    if(signo !=0 && signo != SIGTERM && signo != SIGSEGV)
        return;

    dcs_log(0,0,"AppServer terminated.\n");
    shm_detach((char *)g_pcBcdaShmPtr);
    exit(signo);
}

static int OpenLog()
{  
    char path[500];
    char file[520];
    memset(path,0,sizeof(path));
    memset(file,0,sizeof(file));
    getcwd(path,500);
    sprintf(file,"%s/moniXpepConsume.log",path);
    return dcs_log_open(file, "moniXpepConsume");
}
/*
模拟消费交易
*/
int main(int argc, char *argv[])
{
 	ISO_data siso;
    struct  tm *tm;   
	time_t  t;
	char    *ltime;
	char buffer1[4096], buffer2[4096];
	char tmpbuf[100];
	int s = 0;
    
	memset(&siso,0,sizeof(ISO_data));
	memset(&tmpbuf,0,sizeof(tmpbuf));
	
	s = OpenLog();
	if(s < 0)
        exit(1);
        
    dcs_log(0,0," 模拟消费交易 start");
	
	
	if(IsoLoad8583config(&iso8583_conf[0],"iso8583.conf") < 0)
    {
        dcs_log(0,0,"IsoLoad8583config() failed:%s\n",strerror(errno));
        
        printf("IsoLoad8583config failed \n");
        
        exit(0);
    }
    
    iso8583=&iso8583_conf[0];
  	SetIsoHeardFlag(1);
    SetIsoFieldLengthFlag(1);
    
    
	time(&t);
    tm = localtime(&t);
    ltime = ctime(&t);
    
    sprintf(tmpbuf,"%02d%02d", tm->tm_mon+1,tm->tm_mday);
    memcpy(tmpbuf+4, ltime+11, 2);
    memcpy(tmpbuf+6, ltime+14, 2);
    memcpy(tmpbuf+8, ltime+17, 2);

	dcs_log(0,0,"datatime=%s", tmpbuf);
	
	setbit(&siso,0,(unsigned char *)"0200", 4);
	setbit(&siso,2,(unsigned char *)"6222021001098550814", 19);
	setbit(&siso,3,(unsigned char *)"910000", 6);
	setbit(&siso,4,(unsigned char *)"000000000009", 12);
	setbit(&siso,7,(unsigned char *)tmpbuf, 10);
	setbit(&siso,11,(unsigned char *)tmpbuf+4, 6);
	setbit(&siso,12,(unsigned char *)tmpbuf+4, 6);
	setbit(&siso,13,(unsigned char *)tmpbuf, 4);
	//setbit(&siso,21,(unsigned char *)"567890AB64010000021   61621619    202200001500", 46);
	setbit(&siso,21,(unsigned char *)"860010030210046112233443030000001150000110000000001", 51);
	setbit(&siso,22,(unsigned char *)"021", 3);
	setbit(&siso,25,(unsigned char *)"00", 2);
	setbit(&siso,35,(unsigned char *)"6225220108926189=30101200000084800000",37);
	setbit(&siso,36,(unsigned char *)"996225220108926189=15615600000000000008800000002404040000000000000000000=000000000000=000000000000000000",104);
	setbit(&siso,41,(unsigned char *)"11223344",8);
	setbit(&siso,42,(unsigned char *)"860010030210046",15);  //860010030210225 860010030210218
	setbit(&siso,48,(unsigned char *)"123456",6);
	setbit(&siso,49,(unsigned char *)"156",3);
	setbit(&siso,52,(unsigned char *)"F59016391A888998",16);
	setbit(&siso,53,(unsigned char *)"1000000000000000",16);
	setbit(&siso,60,(unsigned char *)"00000000030000000000",20);
    
    iso8583=&iso8583_conf[0];
  	SetIsoHeardFlag(1);
    SetIsoFieldLengthFlag(1);
	
	memset(buffer1, 0, sizeof(buffer1));
	memset(buffer2, 0, sizeof(buffer2));
	memset(tmpbuf, 0, sizeof(tmpbuf));

	s = isotostr((unsigned char *)buffer1,&siso);
	
	bcd_to_asc((unsigned char *)tmpbuf, 
		(unsigned char *)buffer1+4,16,0);
	sprintf(buffer2, "%4.4s%16.16s%s", buffer1,tmpbuf,buffer1+12);


    if(fold_initsys() < 0)
    {
        dcs_log(0,0, "cannot attach to folder system!");
        return -1;
    }
    
    dcs_log(0,0, "attach to folder system ok !\n");
    
    gs_myFid = fold_locate_folder("PEX_COM");
    
	if(gs_myFid < 0)        
    {
        dcs_log(0,0,"cannot get folder '%s':%s\n", "PEX_COM", ise_strerror(errno) );
        return -1;
    }
     
    dcs_log(0,0, "folder myFid=%d\n", gs_myFid);
    
	fold_write(gs_myFid,-1,buffer2,strlen(buffer2));
	
	dcs_log( 0,0, "Write finished!");
	
	TransRcvSvrExit(0);

	exit(0);
}


