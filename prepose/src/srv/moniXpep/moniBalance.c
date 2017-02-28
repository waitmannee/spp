#include "ibdcs.h"
#include "tmcibtms.h"
#include <unistd.h>
static int gs_myFid = -1;

static void TransRcvSvrExit(int signo)
{
    if(signo > 0)
        dcs_log(0,0,"catch a signal %d\n",signo);
    if(signo !=0 && signo != SIGTERM && signo != SIGSEGV)
        return;

    dcs_log(0,0,"AppServer terminated.\n");
    exit(signo);
}

static int OpenLog()
{  
    char path[500];
    char file[520];
    memset(path,0,sizeof(path));
    memset(file,0,sizeof(file));
    getcwd(path,500);
    sprintf(file,"%s/moniBalance.log",path);
    return dcs_log_open(file, "moniBalance");
}

/*
模拟EWP余额查询报文
*/
int main(int argc, char *argv[])
{
	char buffer[1024];
	int s = 0;
	int fid;
    
	memset(buffer,0,sizeof( buffer ) );	
	printf("moniBalance start!\n");
	s = OpenLog();
	if(s < 0)
        exit(1);
	dcs_log(0,0,"模拟EWP余额查询报文 start");
	EwpDataBalance(buffer);
	dcs_log( 0, 0, "ewp data Balance return[%d]:%s.", strlen(buffer), buffer );
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
    
    if(fold_get_maxmsg(gs_myFid) < 10)
	    fold_set_maxmsg(gs_myFid, 20) ;

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

/* 模拟从EWP过来的数据--余额查询交易 */
int EwpDataBalance(char *buffer)
{
	char tmpbuf[256];
	struct  tm *tm;   time_t  t;
    time(&t);
    tm = localtime(&t);
	
	sprintf( buffer, "%s", "EWPP");   /*4个字节的报文类型数据*/
	sprintf( buffer, "%s%s",buffer, "0200");   /*4个字节的交易类型数据*/
	sprintf( buffer, "%s%s",buffer, "310000");   /*6个字节的交易处理码*/
	
	sprintf(tmpbuf,"%04d%02d%02d",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday);
	sprintf( buffer, "%s%s",buffer, tmpbuf);   /*8个字节  接入系统发送交易的日期*/
	
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%02d%02d%02d",tm->tm_hour,tm->tm_min,tm->tm_sec);
	sprintf( buffer, "%s%s",buffer, tmpbuf);   /*6个字节  接入系统发送交易的时间*/
	
	sprintf( buffer, "%s%s",buffer, "0100");   /*4个字节  接入系统的编号EWP：0100*/
	sprintf( buffer, "%s%s",buffer, "10F");   /*3个字节  区分业务类型余额查询：201*/
	
	sprintf( buffer, "%s%s" ,buffer, "1");/*交易发起方式 1:pos支付*/
	/*
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%04d%02d%02d%02d%02d%02d",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
	sprintf( buffer,"%s%s%s",buffer,"DY",tmpbuf);
*/
/*16个字节刷卡设备编号*/
	sprintf( buffer,"%s%s",buffer,"CADE000012345678");
	memset(tmpbuf,0,sizeof(tmpbuf));
	
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%s%02d%02d%02d","MOBILEIMEI",tm->tm_hour,tm->tm_min,tm->tm_sec);
	sprintf(buffer,"%s%-20s",buffer,tmpbuf);/*20个字节 手机编号IMEI左对齐 不足补空格*/
	
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%02d%02d%02d%02d",tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
	sprintf( buffer,"%s%s%s",buffer,"150",tmpbuf);/*11个字节 手机号，例如：1502613236*/
	
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%s%02d%02d%02d%02d%02d","ORDERNO",tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
	sprintf(buffer,"%s%-20s",buffer,tmpbuf);/*20个字节订单号 左对齐 不足补空格*/
	
	sprintf(buffer,"%s%20s",buffer,"524");/*20个字节商户编号 不足左补空格*/
	sprintf(buffer,"%s%-19s",buffer,"6222021001098550814");/*19个字节卡号, 左对齐 不足补空格*/
	/*sprintf(buffer,"%s%-160s",buffer,"6013825005000163153=49125201000070700");*//*160个字节 磁道,左边对齐 右边不足补空格*/
	
	/*sprintf( buffer,"%s%s",buffer,"MACKEYRANDNUMBER");*//*16个字节的磁道密钥随机数*/
	
	/*sprintf( buffer,"%s%s",buffer,"0909629006010110");*//*16个字节接入系统不对密码做处理*/
	
	/*sprintf( buffer,"%s%s",buffer,"PINKEYRANDNUMBER");*//*16个字节pin密钥随机数*/
	/*sprintf(buffer,"%s%-160s",buffer,"AA42C66D6102B4860C6146C0B3FD655852F685B886A90BCD8789EA1F95CFCAE6720D861C7F7A20BB1A718B57DA473D7CBA828FF4C664C707146C2FCB54F6061310D4E5ED30416E85");*/
	/*160个字节 磁道,左边对齐 右边不足补空格*/
	sprintf(buffer,"%s%-160s",buffer,"D7E2636B605A14278596BCD7877B19DFA2309AF2141C8AE42466118C25D54E548E3B29E20C2C821748BB41114500397F1B41EF750A050DFE9C9D9BA81764B25625DD7C5D3AB2D8B2C4145B4825E8EEF1");
	
	sprintf( buffer,"%s%s",buffer,"4321432143214321");/*16个字节的磁道密钥随机数*/
	
	sprintf( buffer,"%s%s",buffer,"DF3C3D60764B2BF6");/*16个字节接入系统不对密码做处理*/
	
	sprintf( buffer,"%s%s",buffer,"1234123412341234");/*16个字节pin密钥随机数*/
	sprintf( buffer,"%s%s",buffer,"selfdefineselfdefine");/*20个字节自定义域原样返回给EWP*/

	return 0;
}
