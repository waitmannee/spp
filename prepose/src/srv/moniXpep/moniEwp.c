#include "ibdcs.h"
#include "tmcibtms.h"
#include <unistd.h>
#include<pthread.h>
#include  <iconv.h>

static int gs_myFid = -1;


static int RecvData();

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
    sprintf(file,"%s/moniEwp.log",path);
    return dcs_log_open(file, "moniEwp");
}

//代码转换:从一种编码转为另一种编码
int code_convert(char* from_charset, char* to_charset, char* inbuf,
               size_t inlen, char* outbuf, size_t outlen)
{
	iconv_t cd;
	char** pin = &inbuf;  
	char** pout = &outbuf;
	int len = outlen;
	cd = iconv_open(to_charset,from_charset);  
	if(cd == 0)
	{
		return -1;
	}
	memset(outbuf,0,outlen);  
	int ret = 0;
	//ret = iconv(cd,pin,(size_t *)&inlen,pout,(size_t *)&outlen);
	ret = iconv(cd,pin,&inlen,pout,&outlen);
	if( ret<0)
	{
		dcs_log(0, 0, "iconv fail,retcode=%d\n",errno );
		iconv_close(cd);
		return -1;  
	}
	*pout='\0';
	iconv_close(cd);
	return len - outlen ;
}
//utf-8码转为GBK码
int u2g(char *inbuf, int inlen, char *outbuf, int outlen)
{
	return code_convert("utf-8", "GBK", inbuf, inlen, outbuf, outlen);
}
//GBK码转为utf-8码
int g2u(char *inbuf, size_t inlen, char *outbuf, size_t outlen)
{
	return code_convert("GBK", "utf-8", inbuf, inlen, outbuf, outlen);
}


/*
模拟EWP消费报文，即订单支付报文
*/
int main(int argc, char *argv[])
{
 	struct  tm *tm;
	time_t  t;
	char    *ltime;
	char tmpbuf[100],buffer[1024];
	char input[7];
	int s = 0;
	int fid = 0, s_myFid = 0;
	int i = 0;
	int rtn = 0;
	int seconds=0;
	int currentseconds=0,sendnum=0;
	int totaltime=0;
	int starttime=0,currenttime=0;
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(buffer, 0, sizeof( buffer ) );
	memset(input, 0, sizeof( input ) );

	printf("do a tests=%d",s);
	s = OpenLog();
	if(s < 0)
        exit(1);

	dcs_log(0,0,"模拟EWP转账汇款报文 start");

	time(&t);
    tm = localtime(&t);
    ltime = ctime(&t);

    sprintf(tmpbuf,"%02d%02d", tm->tm_mon+1,tm->tm_mday);
    memcpy(tmpbuf+4, ltime+11, 2);
    memcpy(tmpbuf+6, ltime+14, 2);
    memcpy(tmpbuf+8, ltime+17, 2);
	
	
	fid = fold_create_folder("HCCP");
	if( fid <0 )
		fid = fold_locate_folder("HCCP");
	if(fid < 0)
	{
		dcs_log(0,0,"cannot get folder '%s':%s\n", "HCCP", ise_strerror(errno) );
		return -1;
	}
	//启个线程接收返回，并显示接收返回的时间
	pthread_t sthread;
	if(pthread_create(&sthread, NULL, (void *(*)(void *))RecvData, NULL) !=0)
	{
		dcs_log(0, 0, "Create thread fail ");
	}
	pthread_detach(sthread);
	
	memset(buffer, 0, sizeof(buffer));
	rtn = DataFromEWP(buffer);
	if(fold_initsys() < 0)
	{
		dcs_log(0, 0, "cannot attach to folder system!");
		return -1;
	}
	dcs_log(0,0, "attach to folder system ok !\n");
	gs_myFid = fold_locate_folder("TRANSFERRCV");
	if(gs_myFid < 0)
	{
		dcs_log(0,0,"cannot get folder '%s':%s\n", "TRANSFERRCV", ise_strerror(errno) );
		return -1;
	}
	dcs_log(0,0, "folder myFid=%d", gs_myFid);
	fold_write(gs_myFid, fid, buffer, rtn );
	printf("quit if you want yo end!");
	while(1)
	{
		scanf("%s", input);
		if(memcmp(input, "quit", 4)==0)
			break;
	}
	TransRcvSvrExit(0);

	exit(0);
}

int RecvData()
{
	char buffer[1024];
	int len = 0;
	int orgfid = 0;
	int myFid = 0;
	int openid = 0;
	
	memset(buffer, 0, sizeof(buffer));

	myFid = fold_create_folder("HCCP");
	if( myFid <0 )
		myFid = fold_locate_folder("HCCP");
	if(myFid < 0)
	{
		dcs_log(0,0,"cannot get folder '%s':%s\n", "HCCP", ise_strerror(errno) );
		return -1;
	}
	openid = fold_open(myFid);
	if(openid < 0)
	{
		dcs_log(0,0,"open fold fail myFid = %d", myFid);
		exit(0);
	}
	while(1)
	{
		len = fold_read(myFid, openid, &orgfid, buffer, sizeof(buffer), 1);
		dcs_log(0, 0, "recv finished, recv length =%d", len);
		sleep(100);
	}
	return 0;
}

/*模拟一体机测试数据*/
int DataFromEWP(char *buffer)
{
	char tmpbuf[256], buf[16];
	struct  tm *tm;   
	time_t  t;
    time(&t);
    tm = localtime(&t);
	int num=0;
	int len =0;
	
	sprintf( buffer, "%s", "TRNF");   /*4个字节的报文类型数据*/
	len +=4;
	memset(buf, 0x00, sizeof(buf));
	asc_to_bcd((unsigned char*) buf, (unsigned char*) "6005360000", 10, 0);
	//sprintf( buffer, "%s%s", buffer, buf);   /*5个字节的TPDU头数据*/
	memcpy(buffer+len, buf, 5);
	len +=5;
	sprintf( buffer+len, "%s", "0");   /*1个字节的转账汇款类型 次日到账*/
	len +=1;
	sprintf(tmpbuf,"%04d%02d%02d",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday);
	sprintf( buffer+len, "%s", tmpbuf);   /*8个字节  接入系统发送交易的日期*/
	len +=8;
	
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%02d%02d%02d",tm->tm_hour,tm->tm_min,tm->tm_sec);
	sprintf( buffer+len, "%s", tmpbuf);   /*6个字节  接入系统发送交易的时间*/
	len +=6;

	sprintf( buffer+len, "%s", "804");   /*3个字节  交易码 区分业务类型订单支付：210  200老板收款*/
	len +=3;
	sprintf( buffer+len, "%s", "05");/*交易发起方式*/
	len +=2;
	sprintf( buffer+len, "%s","0DF0110030000002");
	len +=16;
	
	sprintf(buffer+len,"%-20s","D1G0060000552");/*20个字节 手机编号IMEI左对齐 不足补空格*/
	len +=20;
	sprintf( buffer+len,"%s","SJ000095900");/*11个字节 手机号，例如：15026513236*/
	len +=11;
	
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%s%02d%02d%02d%02d%02d%06d","PH17",tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec, 0);
	sprintf(buffer+len,"%-20s",tmpbuf);
	//sprintf( xpebuffer, "%s%s", xpebuffer, tmpbuf);   /*20个字节订单号*/
	len +=20;
	
	sprintf(buffer+len, "%-19s", "6214441000010103");/*转出卡*/
	len +=19;
	sprintf(buffer+len, "%-19s", "6225882129194467");/*转入卡*/
	len +=19;
	
	dcs_log(0,0,"姓名：\n", "李平" );
	//u2g(char *inbuf, int inlen, char *outbuf, int outlen);
	char outbuf[10];
	int outlen = 10;
	u2g("李平", strlen("李平"), outbuf, outlen);
	sprintf(buffer+len, "%-50s", outbuf);/*姓名*/
	len +=50;
	sprintf(buffer+len, "%-25s", "313290000017");/*行号*/
	len +=25;
	
	num =20000;
	sprintf(buffer+len,"%012d",num);/*12个字节 金额 右对齐 不足补0*/
	len +=12;
	num =5;
	sprintf(buffer+len,"C%011d",num);/*12个字节 手续费 右对齐 不足补0*/
	len +=12;
	
	sprintf( buffer+len,"%-160s","B0393627FFC661F7E7E9A5A5AD61DE1B");
	len +=160;
	sprintf( buffer+len,"%s","483ADF3268D13998");/*16个字节的磁道密钥随机数*/
	len +=16;
	
	sprintf( buffer+len,"%s","5D38EB55A6FA16F7");/*密码*/
	len +=16;
	sprintf( buffer+len,"%s","C9F387A55CD35C77");/*16个字节pin密钥随机数*/ 
	len +=16;
	sprintf( buffer+len,"%s","3AA31C77");/*8个字节的mac值*/ 
	len +=8;
	
	sprintf( buffer+len,"%s","1");/*是否发短信的标记*/
	len +=1;
	sprintf( buffer+len,"%s","15026513236");/*发短信的手机号码*/
	len +=11;
	sprintf( buffer+len,"%s","1");/*终端类型*/
	len +=1;
	sprintf( buffer+len,"%s","0");/*IC卡标识*/
	len +=1;
	sprintf( buffer+len,"%s","0");/*卡片序列号存在标识*/
	len +=1;
	sprintf( buffer+len,"%03d", 0);/*IC卡数据域*/
	len +=3;
	
	sprintf( buffer+len,"%s","0000000000<0.1588.0>");/*20个字节的自定义域*/
	len +=20;
	
	return len;
	
}
