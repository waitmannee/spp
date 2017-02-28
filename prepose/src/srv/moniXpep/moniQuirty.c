#include "ibdcs.h"
#include "tmcibtms.h"
#include <unistd.h>

static int gs_myFid = -1;

static char   *g_pcBcdaShmPtr;

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
    sprintf(file,"%s/moniQuirty.log",path);
    return dcs_log_open(file, "moniQuirty");
}

/*
模拟EWP查询报文
*/
int main(int argc, char *argv[])
{
	char buffer[1024];
	int s = 0;
	memset(buffer,0,sizeof( buffer ) );	
	printf("moniQuirty start!");
	s = OpenLog();
	if(s < 0)
        exit(1);

	dcs_log(0,0,"模拟EWP查询报文 start");
	EwpDataInquiry(buffer);
	dcs_log( 0, 0, "ewp data Inquiry return[%d]:%s.", strlen(buffer), buffer );
    if(fold_initsys() < 0)
    {
        dcs_log(0,0, "cannot attach to folder system!");
        return -1;
    }
    
    dcs_log(0,0, "attach to folder system ok !\n");
    
    gs_myFid = fold_locate_folder("TRANSRCV");
    
	if(gs_myFid < 0)        
    {
        dcs_log(0,0,"cannot get folder '%s':%s\n", "TRANSRCV", ise_strerror(errno) );
        return -1;
    }
     
    dcs_log(0,0, "folder myFid=%d", gs_myFid);
    

	fold_write(gs_myFid,-1,buffer, strlen(buffer) );
	
	dcs_log( 0,0, "Write finished,Write length =%d",s);
	
	TransRcvSvrExit(0);

	exit(0);
}

/* 模拟从EWP过来的数据--查询交易 */
int EwpDataInquiry(char *buffer)
{
	char tmpbuf[256];
	struct  tm *tm;   time_t  t;
    time(&t);
    tm = localtime(&t);
	
	sprintf( buffer, "%s", "0200");   /*4个字节的交易类型数据*/
	sprintf( buffer, "%s%s",buffer, "9F0000");   /*6个字节的交易处理码*/
	
	sprintf(tmpbuf,"%04d%02d%02d",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday);
	sprintf( buffer, "%s%s",buffer, tmpbuf);   /*8个字节  接入系统发送交易的日期*/
	
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%02d%02d%02d",tm->tm_hour,tm->tm_min,tm->tm_sec);
	sprintf( buffer, "%s%s",buffer, tmpbuf);   /*6个字节  接入系统发送交易的时间*/
	
	sprintf( buffer, "%s%s",buffer, "0100");   /*4个字节  接入系统的编号EWP：0100*/
	sprintf( buffer, "%s%s",buffer, "9F0");   /*3个字节  交易查询：9F0*/
	
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%04d%02d%02d%02d%02d%02d",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
	sprintf( buffer,"%s%s%s",buffer,"DY",tmpbuf);/*16个字节刷卡设备编号*/
	
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%s%02d%02d%02d","MOBILEIMEI",tm->tm_hour,tm->tm_min,tm->tm_sec);
	sprintf(buffer,"%s%-20s",buffer,tmpbuf);/*20个字节 手机编号IMEI左对齐 不足补空格*/
	
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%02d%02d%02d%02d",tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
	sprintf( buffer,"%s%s%s",buffer,"150",tmpbuf);/*11个字节 手机号，例如：1502613236*/
	

	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%s%02d%02d%02d%02d%02d","ORDERNO", tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	sprintf(buffer,"%s%-20s",buffer,tmpbuf);
	
	//sprintf(buffer,"%s%-20s",buffer,"ORDERNO160813");
	/*20个字节订单号 左对齐 不足补空格*/
	
	sprintf(buffer,"%s%20s",buffer,"524");/*20个字节商户编号 不足左补空格*/
	sprintf(buffer,"%s%-19s",buffer,"6013825005000163153");/*19个字节卡号, 左对齐 不足补空格*/
	sprintf( buffer,"%s%s",buffer,"selfdefineselfdefine");/*20个字节的自定义域*/
	return 0;
}
