//#include "ibdcs.h"
#include "ibdcs.h"

static int dcs_lock_sysstat();
static int dcs_unlock_sysstat();

int dcs_set_sysstat(int which,  int val)
{
    dcs_lock_sysstat();
    
    switch(which)
    {
        case COMPROC_STATUS:
            g_pIbdcsStat->ComProc_Status = val; break;
        case TCP_CONNECTIONS:
            g_pIbdcsStat->Tcp_Connections= val; break;
        case APPL_READY:
            g_pIbdcsStat->Appl_Ready = val; break;
        case CYH_B_REQ:
            g_pIbdcsStat->CYH_B_Req = val; break;
        default:
            dcs_log(0,0,"dcs_set_sysstat() encounter illegal status %d.\n",which);
            dcs_unlock_sysstat();
            return -1;
    }//switch
    
    dcs_unlock_sysstat();
    return 0;
}

int dcs_get_sysstat(int which,  int* val)
{
    if(val == NULL)
        return -1;
    
    dcs_lock_sysstat();
    
    switch(which)
    {
        case COMPROC_STATUS:
            *val = g_pIbdcsStat->ComProc_Status; break;
        case TCP_CONNECTIONS:
            *val = g_pIbdcsStat->Tcp_Connections; break;
        case APPL_READY:
            *val = g_pIbdcsStat->Appl_Ready; break;
        case CYH_B_REQ:
            *val = g_pIbdcsStat->CYH_B_Req; break;
        default:
            dcs_log(0,0,"dcs_get_sysstat() encounter illegal status %d.\n",which);
            dcs_unlock_sysstat();
            return -1;
    }//switch
    
    dcs_unlock_sysstat();
    return 0;
}

int dcs_dump_sysstat(FILE *fpOut)
{
    IBDCS_STAT *p = NULL;
    p = g_pIbdcsStat;
    if(p == NULL)
    {
        fprintf(fpOut,"not attach to SHM of IBDCS!\n");
        return -1;
    }
    
    if(fpOut == NULL)
        fpOut = stdout;
    fprintf(fpOut,"");
    fprintf(fpOut,"ComProc_Status = %2d  [通信进程状态]\n",        p->ComProc_Status);
    fprintf(fpOut,"                     0:nonRunning; 1:making Call; 2:Connected\n");
    fprintf(fpOut,"Tcp_Connections= %2d  [到中心的链接数]\n",      p->Tcp_Connections);
    fprintf(fpOut,"Appl_Ready     = %2d  [业务上和中心是联机否]\n", p->Appl_Ready);
    fprintf(fpOut,"CYH_B_Req      = %2d  [开始请求报文发送状态]\n", p->CYH_B_Req);
    fprintf(fpOut,"                     0:Req not sent yet;       1: waiting for resp;\n");
    fprintf(fpOut,"                     2:positive Resp received; 3: Negitive Resp received\n");
    fflush(fpOut);
    return 0;
}

int dcs_lock_sysstat()
{
    if(sem_lock(g_pIbdcsShm->is_semid, 0, TRUE) < 0)
    {
        dcs_log(0,0,"dcs_lock_sysstat() failed:%s.\n",strerror(errno));
        return -1;
    }
    
    return 0;
}

int dcs_unlock_sysstat()
{
    if(sem_unlock(g_pIbdcsShm->is_semid, 0, TRUE) < 0)
    {
        dcs_log(0,0,"dcs_unlock_sysstat() failed:%s.\n",strerror(errno));
        return -1;
    }
    
    return 0;
}
