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
    //shm_detach((char *)g_pcBcdaShmPtr);
    exit(signo);
}

static int OpenLog()
{  
    char path[500];
    char file[520];
    memset(path,0,sizeof(path));
    memset(file,0,sizeof(file));
    getcwd(path,500);
    sprintf(file,"%s/TransferRepay.log",path);
    return dcs_log_open(file, "TransferRepay");
}

/*
模拟EWP跨行转账或信用卡还款
*/
int main(int argc, char *argv[])
{

	char buffer[1024];
	int s = 0;

	memset(buffer,0,sizeof( buffer ) );

	printf("do a tests=%d\n",s);
	s = OpenLog();
	if(s < 0)
        exit(1);

	dcs_log(0,0,"模拟EWP转账交易 start");

	DataFromEWP(buffer);

	//DataFromEWP11(buffer);
	dcs_log( 0, 0, "ewp data return[%d]:%s.", strlen(buffer), buffer );
	

	dcs_log( 0, 0, "ewp data return:[%s]",  buffer );

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


	s = fold_write(gs_myFid,-1,buffer, strlen(buffer) );
/*
	dcs_log( 0,0, "Write finished,Write length =%d",s);

	TransRcvSvrExit(0);
*/
	exit(0);
}


/* 模拟从EWP过来转账交易数据或是转账汇款 */

int DataFromEWP(char *buffer)
{
	char tmpbuf[256];
	struct  tm *tm;   time_t  t;
    time(&t);
    tm = localtime(&t);

	sprintf( buffer, "%s", "0200");   /*4个字节的交易类型数据*/
	/*sprintf( buffer, "%s%s",buffer, "400000");  6个字节的交易处理码400000ATM转账类的交易*/
	sprintf( buffer, "%s%s",buffer, "480000");/*6个字节的交易处理码480000转账汇款*/
	/*sprintf( buffer, "%s%s",buffer, "190000");6个字节的交易处理码190000信用卡还款*/

	sprintf(tmpbuf,"%04d%02d%02d",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday);
	sprintf( buffer, "%s%s",buffer, tmpbuf);   /*8个字节  接入系统发送交易的日期*/

	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%02d%02d%02d",tm->tm_hour,tm->tm_min,tm->tm_sec);
	sprintf( buffer, "%s%s",buffer, tmpbuf);   /*6个字节  接入系统发送交易的时间*/
	sprintf( buffer, "%s%s",buffer, "0500");   /*4个字节  接入系统的编号EWP：0100*/
	sprintf( buffer, "%s%s",buffer, "205");   /*3个字节  交易码 203ATM转账204信用卡还款205跨行转账汇款*/

	sprintf( buffer, "%s%s" ,buffer, "1");/*交易发起方式 1:pos支付*/

	/*16个字节刷卡设备编号*/
	sprintf( buffer,"%s%s",buffer,"CADE000012345678");
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%s%02d%02d%02d","MOBILEIMEI",tm->tm_hour,tm->tm_min,tm->tm_sec);
	sprintf(buffer,"%s%-20s",buffer,tmpbuf);/*20个字节 手机编号IMEI左对齐 不足补空格*/

	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%02d%02d%02d%02d",tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
	sprintf( buffer,"%s%s%s",buffer,"150",tmpbuf);/*11个字节 手机号用户名，例如：1502613236*/


	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%s%02d%02d%02d","ORDERNO",tm->tm_hour,tm->tm_min,tm->tm_sec);
	sprintf(buffer,"%s%-20s",buffer,tmpbuf);


	/*20个字节订单号 左对齐 不足补空格*/
	sprintf(buffer,"%s%20s",buffer,"314");/*20个字节商户编号 不足左补空格*/

	sprintf(buffer,"%s%-19s",buffer,"6222021001098550814");/*19个字节卡号, 左对齐 不足补空格lp0530*/
	/*sprintf(buffer,"%s%-19s",buffer,"6222021001098550814");*//*19个字节卡号转出卡号*/
	sprintf(buffer,"%s%-19s",buffer,"6223030000181102");/*19个字节卡号转入卡号 6223030000181102 4063651067622510 6210300120424327 356869 6229011067622510*/
	sprintf(buffer,"%s%-50s",buffer,"中文");/*50个字节卡号转入姓名480000*/
	sprintf(buffer,"%s%-25s",buffer,"313100000013");/*25个字节卡号转入行号480000*/ // 313100000013 313222080002

	sprintf(buffer,"%s%012s",buffer,"2000");/*12个字节 金额 右对齐 不足补0*/
	sprintf(buffer,"%s%s%011s",buffer, "C", "0");/*12个字节交易手续费 右对齐 不足补0 ANS12*/

	/*160个字节 磁道,左边对齐 右边不足补空格*/
	/*sprintf( buffer,"%s%-160s",buffer,"D7E2636B605A14278596BCD7877B19DFA2309AF2141C8AE42466118C25D54E548E3B29E20C2C821748BB41114500397F1B41EF750A050DFE9C9D9BA81764B25625DD7C5D3AB2D8B2C4145B4825E8EEF1");*/
	sprintf(buffer,"%s%-160s",buffer,"01C3069434F08DDA01A8B40EACCDD4F1D543D898046A70F159F80FE5C8B14040A615F89433C986E2B20E10749FE4E579A7B2B1816265A492FE7A9BE0F3B9A952EABBE8F0447096D3");/*lp0530*/

	sprintf( buffer,"%s%s",buffer,"4321432143214321");/*16个字节的磁道密钥随机数*/

	sprintf( buffer,"%s%s",buffer,"DA0CDF7BFD46687D");/*16个字节接入系统不对密码做处理lp0530*///DA0CDF7BFD46687D F80E5C9F9C63B3D6
	/*sprintf( buffer,"%s%s",buffer,"DF3C3D60764B2BF6");*/
	//sprintf( buffer,"%s%s",buffer,"DA0CDF7BFD46687D");//密码明文是6个8
	sprintf( buffer,"%s%s",buffer,"1234123412341234");/*16个字节pin密钥随机数*/
	sprintf( buffer,"%s%s",buffer,"1");/*1个字节是否需要发送短信的标记*/
	sprintf( buffer,"%s%s",buffer,"15026513236");/*11个字节的发送短信的手机号*/

	sprintf( buffer,"%s%s",buffer,"selfdefineselfdefine");/*20个字节的自定义域*/


	return 0;
}


int DataFromEWP11(char *buffer)
{
	char tmpbuf[256];
	struct  tm *tm;   time_t  t;
    time(&t);
    tm = localtime(&t);

	sprintf( buffer, "%s", "0200");   /*4个字节的交易类型数据*/
	sprintf( buffer, "%s%s", buffer, "FF0000");/*6个字节的交易处理码FF0000历史交易信息查询*/
	sprintf( buffer, "%s%s", buffer, "0100");/*4个字节的发送机构号*/
	sprintf( buffer, "%s%s", buffer, "205");/*3个字节的交易码*/
	sprintf( buffer, "%s%s", buffer, "20121106");/*8个字节的查询开始日期*/
	sprintf( buffer, "%s%s", buffer, "20121109");/*8个字节的查询结束日期*/
	sprintf( buffer, "%s%s", buffer, "5");/*1个字节的每页显示条数*/
	sprintf( buffer, "%s%s", buffer, "001");/*3个字节的当前页*/
	sprintf( buffer, "%s%s", buffer, "15011090000");/*11个字节用户名*/
	sprintf( buffer,"%s%s", buffer, "selfdefineselfdefine");/*20个字节的自定义域*/

	return 0;
}
