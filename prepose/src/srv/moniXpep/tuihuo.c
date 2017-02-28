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
    sprintf(file,"%s/chexiao.log",path);
    return dcs_log_open(file, "chexiao");
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
        
    dcs_log(0,0," 模拟消退货交易 start");
	
	
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
	setbit(&siso,2,(unsigned char *)"6013825005000163153", 19);
	setbit(&siso,3,(unsigned char *)"270000", 6);
	setbit(&siso,4,(unsigned char *)"000000000199", 12);
	setbit(&siso,7,(unsigned char *)tmpbuf, 10);
	setbit(&siso,11,(unsigned char *)tmpbuf+4, 6);
	setbit(&siso,12,(unsigned char *)tmpbuf+4, 6);
	setbit(&siso,13,(unsigned char *)tmpbuf, 4);
	//前置系统报文21域规则：
	//psam卡号（16字节）+ 电话号码，区号与电话号码用|区分（或者手机号）（15字节，左对齐，右补空格）+交易发起方式（1字节，ewp平台送过来的信息）+终端类型（2字节，刷卡设备取值为01）+交易码（3字节）+终端流水（6字节，不存在的话填全0）
	setbit(&siso,21,(unsigned char *)"567890AB64010000021|61621619   101307000000", 43);
	setbit(&siso,22,(unsigned char *)"021", 3);
	setbit(&siso,25,(unsigned char *)"00", 2);
	setbit(&siso,35,(unsigned char *)"6013825005000163153=49125201000070700",37);
	setbit(&siso,36,(unsigned char *)"996222001001108560831=156000000000000000000350299921600000701000000000000000000000=0000",87);
	//setbit(&siso,38,(unsigned char *)"821239",6);
	setbit(&siso,41,(unsigned char *)"12345678",8);
	setbit(&siso,42,(unsigned char *)"860010030210225",15);
	setbit(&siso,48,(unsigned char *)"123456",6);
	setbit(&siso,49,(unsigned char *)"156",3);
	setbit(&siso,52,(unsigned char *)"A186C45D096BDF29",16);
	//setbit(&siso,53,(unsigned char *)"1000000000000000",16);
	setbit(&siso,60,(unsigned char *)"000000000900000000000010000000",30);
	
	/*
	数据元长度         N3		
	原交易授权号		N6
	参考号			N12
	时间				N10
	*/
	setbit(&siso,61,(unsigned char *)"8212390000000000000426161900",28);
    
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


