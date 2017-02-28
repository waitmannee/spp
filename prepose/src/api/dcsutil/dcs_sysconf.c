#include "ibdcs.h"

int dcs_load_config(IBDCS_CFG *pIbdcsCfg)
{
    // this function load configuration from file
    // "$FEP_RUNPATH/config/ibcds.conf",and placed
    // all information into the buffer given by caller

    char cfgfile[256];
    int  fd, ret = 0;
    
    if(u_fabricatefile("config/ibdcs.conf",cfgfile,sizeof(cfgfile)) < 0)
    {
        dcs_log(0,0,"cannot get full path name of 'ibdcs.conf'\n");
        return -1;
    }
	
	dcs_log(0, 0, "cfgfile.path = %s", cfgfile);
	
    if(0 > (fd = conf_open(cfgfile)) )
    {
        dcs_log(0,0,"open file '%s' failed.\n",cfgfile);
        return -1;
    }
	
	//dcs_log(0, 0, "memery set  .......");

    memset(pIbdcsCfg,0, sizeof(IBDCS_CFG));
	
	//dcs_log(0, 0, "memery set  .......after");

    //read items with respect to TCP connection
    ret = conf_get_first_string(fd,"Switch_Ip",  pIbdcsCfg->cSwIP);
    if(ret < 0)
    {
        dcs_log(0,0,"loading config item 'Switch_Ip' failed.\n");
        goto lblFailure;
    }
	//dcs_log(0, 0, "pIbdcsCfg->cSwIP = [%s]", pIbdcsCfg->cSwIP);
	//dcs_log(0, 0, "memery set  .......after.....0");
    ret=conf_get_first_number(fd,"Switch_Port",&pIbdcsCfg->iSwPort);
    if(ret < 0)
    {
        dcs_log(0,0,"loading config item 'Switch_Port' failed.\n");
        goto lblFailure;
    }
	//dcs_log(0, 0, "memery set  .......after.....2");
    ret = conf_get_first_number(fd,"Local_Port", &pIbdcsCfg->iLocPort);
    if(ret < 0)
    {
        dcs_log(0,0,"loading config item 'Local_Port' failed.\n");
        goto lblFailure;
    }
	//dcs_log(0, 0, "memery set  .......after.....3");

    //stuffes of SHM
    ret = conf_get_first_number(fd,"Shm_Timeout_Rows",&pIbdcsCfg->iTbTRows);
    if(ret < 0)
    {
        dcs_log(0,0,"loading config item 'Shm_Timeout_Rows' failed.\n");
        goto lblFailure;
    }
    //dcs_log(0, 0, "memery set  .......after.....4");
    //stuffes of timeout control
    ret=conf_get_first_number(fd,"T1",&pIbdcsCfg->iT1);
    if(ret < 0)
    {
        dcs_log(0,0,"loading config item 'T1' failed.\n");
        goto lblFailure;
    }
	//dcs_log(0, 0, "memery set  .......after.....5");
	
    ret=conf_get_first_number(fd,"T2",&pIbdcsCfg->iT2);
    if(ret < 0)
    {
        dcs_log(0,0,"loading config item 'T2' failed.\n");
        goto lblFailure;
    }
	//dcs_log(0, 0, "memery set  .......after.....6");
	
    ret=conf_get_first_number(fd,"T3",&pIbdcsCfg->iT3);
    if(ret < 0)
    {
        dcs_log(0,0,"loading config item 'T3' failed.\n");
        goto lblFailure;
    }
    //dcs_log(0, 0, "memery set  .......after.....7");

    //others
    ret=conf_get_first_number(fd,"MacKey_Use",&pIbdcsCfg->iMacKUse);
    if(ret < 0)
    {
        dcs_log(0,0,"loading config item 'MacKey_Use' failed.\n");
        goto lblFailure;
    }

	//dcs_log(0, 0, "memery set  .......after.....8");
    ret=conf_get_first_number(fd,"RunMode",&pIbdcsCfg->iRunMode);
    if(ret < 0)
    {
        dcs_log(0,0,"loading config item 'RunMode' failed.\n");
        goto lblFailure;
    }
	
	//dcs_log(0, 0, "memery set  .......after.....9");

    //read stuffes about iso8583
    ret=conf_get_first_string(fd, "Send_Institute_Code", pIbdcsCfg->szSendInsCode);
    if(ret < 0)
    {
        dcs_log(0,0,"loading config item 'Send_Institute_Code' failed.\n");
        goto lblFailure;
    }
	//dcs_log(0, 0, "memery set  .......after.....10...[%s]", pIbdcsCfg->szSendInsCode);
	//dcs_log(0, 0, "memery set  .......after.....10");

	
    
	//ret=conf_get_first_string(fd, "Recv_Institute_Code",  pIbdcsCfg->szRecvInsCode);
	char tmp[256];
	memset(tmp, 0x00, sizeof(tmp));
	ret=conf_get_first_string(fd, "Recv_Institute_Code", tmp);
	//printf("pIbdcsCfg->szRecvInsCode, tmp = [%s]\n", tmp);
	memcpy(pIbdcsCfg->szRecvInsCode, tmp, strlen(tmp));
	
	
    if(ret < 0)
    {
        dcs_log(0,0,"loading config item 'Recv_Institute_Code' failed.\n");
        goto lblFailure;
    }
	//dcs_log(0, 0, "memery set  .......after.....11");
    
    ret=conf_get_first_number(fd,"HeartBeating",&pIbdcsCfg->iHeartBeating);
	//dcs_log(0, 0, "memery set  .......after.....12");
    if(ret < 0)  pIbdcsCfg->iHeartBeating = 0;

    //dcs_log(0, 0, "memery set  .......load config sucess !");
	
    conf_close(fd);
    return 0;

lblFailure:
    conf_close(fd);
    return -1;
}

int dcs_dump_sysconf(FILE *fpOut)
{
    if(fpOut==NULL)
        fpOut = stdout;

    if(g_pIbdcsCfg == NULL)
    {
        fprintf(fpOut,"not attach to shared memory of IBDCS!\n");
        return -1;
    }
    
    fprintf(fpOut,"          SwitchIP = %s\n", g_pIbdcsCfg->cSwIP);
    fprintf(fpOut,"        SwitchPort = %d\n",g_pIbdcsCfg->iSwPort);
    fprintf(fpOut,"         LocalPort = %d\n",g_pIbdcsCfg->iLocPort);
    fprintf(fpOut,"       SendInsCode = %s\n",g_pIbdcsCfg->szSendInsCode);
    fprintf(fpOut,"       RecvInsCode = %s\n",g_pIbdcsCfg->szRecvInsCode);

    /*共享内存中超时表的记录数*/
    fprintf(fpOut,"RowsInTimeoutTable = %d\n",g_pIbdcsCfg->iTbTRows);
    
    /*超时时间*/
    fprintf(fpOut,"                T1 = %d\n" ,g_pIbdcsCfg->iT1);
    fprintf(fpOut,"                T2 = %d\n" ,g_pIbdcsCfg->iT2);
    fprintf(fpOut,"                T3 = %d\n",g_pIbdcsCfg->iT3);
    fprintf(fpOut,"        MacKeyUsed = %d\n",g_pIbdcsCfg->iMacKUse);
    fprintf(fpOut,"       RunningMode = %d\n",g_pIbdcsCfg->iRunMode);
    fprintf(fpOut,"      HeartBeating = %d\n",g_pIbdcsCfg->iHeartBeating);
    fflush(fpOut);
    return 0;
}

int dcs_set_sysconf(int which,  int val)
{
    switch(which)
    {
        case TIMEOUT1:
            g_pIbdcsCfg->iT1 = val;break;
        case TIMEOUT2:
            g_pIbdcsCfg->iT2 = val;break;
        case TIMEOUT3:
            g_pIbdcsCfg->iT3 = val;break;
        case MACKEY_USED:
            g_pIbdcsCfg->iMacKUse = val;break;
        case RUN_MODE:
            g_pIbdcsCfg->iRunMode = val;break;
        default:
            dcs_log(0,0,"dcs_set_sysconf() encounter illegal"
                    " config item %d.\n",which);
            return -1;
    }//switch

    return 0;
}
