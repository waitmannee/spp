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
    sprintf(file,"%s/moniConsumer_revoked.log",path);
    return dcs_log_open(file, "moniConsumer_revoked");
}

/*
模拟	支付系统消费撤销交易报文
*/
int main(int argc, char *argv[])
{
	char buffer[1024];
	int s = 0,fid;
	memset(buffer,0,sizeof( buffer ) );	
	printf("moniConsumer_revoked start!");
	s = OpenLog();
	if(s < 0)
        exit(1);

	dcs_log(0,0,"模拟支付系统消费撤销报文 start");
	PaySysDataRevoked(buffer);
	dcs_log( 0, 0, "ewp data moniConsumer_revoked return[%d]:%s.", strlen(buffer), buffer );
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

	fid = fold_locate_folder("EWP_COM");
    
	if(fid < 0)        
    {
        dcs_log(0,0,"cannot get folder '%s':%s\n", "EWP_COM", ise_strerror(errno) );
        return -1;
    }
     
    dcs_log(0,0, "link folder fid=%d", fid);
	
	fold_write(gs_myFid,fid,buffer, strlen(buffer) );
    	
	dcs_log( 0,0, "Write finished,Write length =%d",s);
	
	TransRcvSvrExit(0);

	exit(0);
}

/* 模拟从支付管理系统过来的数据--消费撤销交易 */
int PaySysDataRevoked(char *buffer)
{
	char tmpbuf[256];
	struct  tm *tm;   time_t  t;
    time(&t);
    tm = localtime(&t);
	
	dcs_log(0,0, "测试环境上的测试");
	sprintf( buffer, "%s", "EWPP");   /*4个字节的报文类型数据*/
	sprintf( buffer, "%s%s",buffer, "0200");   /*4个字节的交易类型数据*/
	sprintf( buffer, "%s%s",buffer, "200000");   //6个字节的交易处理码 200000订单撤销
	
	sprintf(tmpbuf,"%04d%02d%02d",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday);
	sprintf( buffer, "%s%s",buffer, tmpbuf);   //8个字节  接入系统发送交易的日期
	
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%02d%02d%02d",tm->tm_hour,tm->tm_min,tm->tm_sec);
	sprintf( buffer, "%s%s",buffer, tmpbuf);   //6个字节  接入系统发送交易的时间
	
	sprintf( buffer, "%s%s",buffer, "0200");   //4个字节  字符管理系统的编号：0200
	//sprintf( buffer, "%s%s",buffer, "307");   //3个字节  交易码
	sprintf( buffer, "%s%s",buffer, "207");   //3个字节  交易码
	sprintf( buffer, "%s%s" ,buffer, "1");//交易发起方式 1:pos支付
	sprintf(buffer,"%s%-16s",buffer,"CADE000012345678");//16个字节刷卡设备编号
	
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%s%02d%02d%02d","MOBILEIMEI",tm->tm_hour,tm->tm_min,tm->tm_sec);
	sprintf(buffer,"%s%-20s",buffer,tmpbuf);//20个字节 手机编号IMEI左对齐 不足补空格
	
	
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%02d%02d%02d%02d",tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
	sprintf( buffer,"%s%s%s",buffer,"150",tmpbuf);//11个字节 手机号，例如：1502613236
	
	sprintf(buffer,"%s%-20s",buffer,"ORDERNO073531");//20个字节订单号 左对齐 不足补空格
	sprintf(buffer,"%s%20s",buffer,"524");//20个字节商户编号 不足左补空格
	sprintf(buffer,"%s%-19s",buffer,"6013825005000163153");//19个字节转出卡号, 左对齐 不足补空格
	
	sprintf(buffer,"%s%12s",buffer,"996290996290");//12个字节的系统参考号N12
	sprintf(buffer,"%s%s",buffer,"AUTHNO");//6个字节授权码ANS
	sprintf(buffer,"%s%s",buffer,"000424");//6字节的前置系统流水ANS
	sprintf(buffer,"%s%s",buffer,"20120412073531");//14字节交易发起日期和时间ANSyyyyMMddhhmmss
	sprintf(buffer,"%s%s",buffer,"11111111");//8字节mac
	/*
	dcs_log(0,0, "生产环境上的测试，只能全额撤销");
	sprintf( buffer, "%s", "0200");   //4个字节的交易类型数据
	sprintf( buffer, "%s%s",buffer, "200000");   //6个字节的交易处理码 200000订单撤销
	sprintf( buffer, "%s%s",buffer, "20120613");   //8个字节  接入系统发送交易的日期
	
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%02d%02d%02d",tm->tm_hour,tm->tm_min,tm->tm_sec);
	sprintf( buffer, "%s%s",buffer, tmpbuf);   //6个字节  接入系统发送交易的时间
	
	sprintf( buffer, "%s%s",buffer, "0200");   //4个字节  字符管理系统的编号：0200
	sprintf( buffer, "%s%s",buffer, "103");   //3个字节  交易码
	sprintf( buffer, "%s%s" ,buffer, "1");//交易发起方式 1:pos支付
	sprintf(buffer,"%s%-16s",buffer,"CADE000012345678");//16个字节刷卡设备编号
	
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%s%02d%02d%02d","MOBILEIMEI",tm->tm_hour,tm->tm_min,tm->tm_sec);
	sprintf(buffer,"%s%-20s",buffer,tmpbuf);//20个字节 手机编号IMEI左对齐 不足补空格
	
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%02d%02d%02d%02d",tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
	sprintf( buffer,"%s%s%s",buffer,"150",tmpbuf);//11个字节 手机号，例如：1502613236
	
	sprintf(buffer,"%s%-20s",buffer,"JW120608185502794");//20个字节订单号 左对齐 不足补空格
	sprintf(buffer,"%s%20s",buffer,"524");//20个字节商户编号 不足左补空格
	sprintf(buffer,"%s%-19s",buffer,"6222021001098550814");//19个字节转出卡号, 左对齐 不足补空格
	
	sprintf(buffer,"%s%12s",buffer,"185456315183");//12个字节的系统参考号N12
	sprintf(buffer,"%s%s",buffer,"012345");//6个字节授权码ANS
	sprintf(buffer,"%s%s",buffer,"000004");//6字节的前置系统流水ANS
	sprintf(buffer,"%s%s",buffer,"20120613184944");//14字节交易发起日期和时间ANSyyyyMMddhhmmss
	*/
	return 0;
}
