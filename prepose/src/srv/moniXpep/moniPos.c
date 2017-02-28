#include "ibdcs.h"
#include "tmcibtms.h"
#include "iso8583.h"
#include <unistd.h>
#include "extern_function.h"
#include "folder.h"

static int gs_myFid = -1;

static char   *g_pcBcdaShmPtr;
extern struct ISO_8583 *iso8583;
struct ISO_8583 iso8583_conf[128];

char MAIN_KEY[] = "31313131313131313131313131313131";
char PIN_KEY[] = "2F786830A25A5E4E128BF54D3C15415B";
char MAC_KEY[] = "DA4BBCB91ABC0AFB";

char iso8583name[129][30] = {
	"消息类型",			/*bit 0 */
	"bitmap",
	"主帐号",
	"处理代码",
	"交易金额",
	"自定义",
	"自定义",
	"交易日期和时间",
	"自定义",
	"自定义",
	"自定义",
	"系统流水号",
	"本地交易时间",
	"本地交易日期",
	"自定义",
	"结算日期",
	"自定义",
	"受理日期",
	"商户类型",
	"自定义",
	"自定义",
	"终端信息",
	"服各点进入方式",
	"自定义",
	"自定义",
	"服务点条件码",
	"自定义",
	"自定义",
	"交易费",
	"自定义",
	"交易处理费",
	"自定义",
	"代理方机构标识码",
	"发送机构标识码",
	"自定义",
	"第二磁道数据",
	"第三磁道数据",
	"检索参考号",
	"授权标识响应",
	"响应代码",					/* bit 39 */
	"服务限制代码",
	"受卡方终端标识",
	"受卡方标识代码",
	"受卡方名称/地点",
	"附加响应数据",
	"自定义",
	"自定义",
	"自定义",
	"自定义附加数据",
	"交易货币代号",
	"结算货币代号",
	"自定义",
	"个人识别号数据",
	"安全控制信息",
	"附加金额",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"MACkey使用标志",
	"自定义",
	"原因码",
	"结算批次号",
	"MAC值",						/* bit 64 */
	"自定义",
	"结算代号",
	"自定义",
	"自定义",
	"自定义",
	"网络管理功能码",
	"报文编号",
	"后报文编号",
	"动作日期",
	"贷记笔数",
	"撤消贷记笔数",
	"借记笔数",
	"撤消借记笔数",
	"转帐笔数",
	"撤消转帐笔数",
	"自定义",
	"授权笔数",
	"贷记手续费金额",
	"自定义",
	"借记手续费金额",
	"自定义",
	"贷记交易金额",
	"撤消贷记金额",
	"借记交易金额",
	"撤消借记金额",
	"原始数据",
	"文件更新代码",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"报文保密代码",
	"净清算额",
	"自定义",
	"自定义",
	"接收机构标识代码",
	"文件名称",
	"转出账户",						/* bit 102 */
	"转入账户",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"MAC值"						/* bit 128 */
};
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
    sprintf(file,"%s/moniPos.log",path);
    return dcs_log_open(file, "moniPos");
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
	char tmpbuf[100], bcdbuf[1024];
	int s = 0;
    
	memset(&siso,0,sizeof(ISO_data));
	memset(tmpbuf,0,sizeof(tmpbuf));
	memset(bcdbuf,0,sizeof(bcdbuf));
	
	s = OpenLog();
	if(s < 0)
        exit(1);
        	
	if(IsoLoad8583config(&iso8583_conf[0],"iso8583_pos.conf") < 0)
    {
        dcs_log(0,0,"IsoLoad8583config() failed:%s\n",strerror(errno));
        
        printf("IsoLoad8583config failed \n");
        
        exit(0);
    }
    
    iso8583=&iso8583_conf[0];
  	SetIsoHeardFlag(0);
    SetIsoFieldLengthFlag(0);
    
    
	time(&t);
    tm = localtime(&t);
    ltime = ctime(&t);
    
    sprintf(tmpbuf,"%02d%02d", tm->tm_mon+1,tm->tm_mday);
    memcpy(tmpbuf+4, ltime+11, 2);
    memcpy(tmpbuf+6, ltime+14, 2);
    memcpy(tmpbuf+8, ltime+17, 2);

	dcs_log(0,0,"datatime=%s", tmpbuf);

/*
	dcs_log(0,0," 模拟POS消费交易 start");//0,1,2,3,4,7,11,21,22,25,26,35,36,41,42,49,52,53,60
	setbit(&siso,0,(unsigned char *)"0200", 4);
	setbit(&siso,2,(unsigned char *)"6222620110001030171", 19);
	setbit(&siso,3,(unsigned char *)"910000", 6);
	setbit(&siso,4,(unsigned char *)"000000700000", 12);//消费金额
	setbit(&siso,7,(unsigned char *)tmpbuf, 10);
	
	//前置系统报文21域规则：
	setbit(&siso,11,(unsigned char *)"424690", 6);//前置系统流水
	setbit(&siso,21,(unsigned char *)"00022900007000000003000", 23);
	setbit(&siso,22,(unsigned char *)"021", 3);
	setbit(&siso,25,(unsigned char *)"00", 2);
	setbit(&siso,26,(unsigned char *)"06", 2);
	setbit(&siso,35,(unsigned char *)"6222620110001030171=1912220165332468", 36);
	setbit(&siso,36,(unsigned char *)"996222620110001030171=1561560500050001328013000000010000000000===0332468165", 75);
	
	setbit(&siso,41,(unsigned char *)"11223344",8);
	setbit(&siso,42,(unsigned char *)"860010030210046",15);
	setbit(&siso,48,(unsigned char *)"000200000020000023711223344860010030210046",40);//交易码3,交易处理码6,消息类型4,终端流水6,pos终端终端号6,pos终端商户号15
	setbit(&siso,52,(unsigned char *)"F33B25652C1BD648",16);
	setbit(&siso,53,(unsigned char *)"1000000000000000",16);
	
	setbit(&siso,49,(unsigned char *)"156",3);
    setbit(&siso,60,(unsigned char *)"22000070",8);
*/

/*
	dcs_log(0,0," 模拟POS冲正交易 start");
	setbit(&siso,0,(unsigned char *)"0400", 4);
	setbit(&siso,2,(unsigned char *)"6222620110001030171", 19);
	setbit(&siso,3,(unsigned char *)"000000", 6);
	setbit(&siso,4,(unsigned char *)"000000700000", 12);
	setbit(&siso,7,(unsigned char *)tmpbuf, 10);
	
	//前置系统报文21域规则：
	
	setbit(&siso,11,(unsigned char *)"424688", 6);//424669true
	setbit(&siso,21,(unsigned char *)"00022900007000000003000", 23);
	setbit(&siso,22,(unsigned char *)"021", 3);
	setbit(&siso,25,(unsigned char *)"00", 2);
	
	setbit(&siso,39,(unsigned char *)"98", 2);
	setbit(&siso,41,(unsigned char *)"11223344",8);
	setbit(&siso,42,(unsigned char *)"860010030210046",15);
	setbit(&siso,48,(unsigned char *)"000000000020000023711223344860010030210046",40);//交易码3,交易处理码6,消息类型4,终端流水6,pos终端终端号6,pos终端商户号15
	setbit(&siso,49,(unsigned char *)"156",3); 
	
    setbit(&siso,60,(unsigned char *)"22000070",8);
*/
/*
	dcs_log(0,0," 模拟POS余额查询的交易 start");//0,1,2,3,4,7,11,21,22,25,26,35,36,41,42,49,52,53,60
	setbit(&siso,0,(unsigned char *)"0200", 4);
	setbit(&siso,2,(unsigned char *)"6222620110001030171", 19);
	setbit(&siso,3,(unsigned char *)"310000", 6);
	setbit(&siso,4,(unsigned char *)"000000700000", 12);//原消费金额
	setbit(&siso,7,(unsigned char *)tmpbuf, 10);
	
	//前置系统报文21域规则：
	setbit(&siso,11,(unsigned char *)"424688", 6);//前置系统流水
	setbit(&siso,21,(unsigned char *)"0002370000713100000310F", 23);
	setbit(&siso,22,(unsigned char *)"021", 3);
	setbit(&siso,25,(unsigned char *)"00", 2);
	setbit(&siso,26,(unsigned char *)"06", 2);
	setbit(&siso,35,(unsigned char *)"6222620110001030171=1912220165332468", 36);
	setbit(&siso,36,(unsigned char *)"996222620110001030171=1561560500050001328013000000010000000000===0332468165", 75);
	
	setbit(&siso,41,(unsigned char *)"06000001",8);
	setbit(&siso,42,(unsigned char *)"403310048168501",15);
	setbit(&siso,48,(unsigned char *)"000200000020000023711223344860010030210046",40);//交易码3,交易处理码6,消息类型4,终端流水6,pos终端终端号6,pos终端商户号15
	setbit(&siso,52,(unsigned char *)"F33B25652C1BD648",16);
	setbit(&siso,53,(unsigned char *)"1000000000000000",16);
	
	setbit(&siso,49,(unsigned char *)"156",3);
    setbit(&siso,60,(unsigned char *)"000000000900000000000010000000",30);
*/
/*
	dcs_log(0,0," 模拟POS消费撤销的交易 start");//0,1,2,3,4,7,11,21,22,25,35,36,41,42,49,60,61
	setbit(&siso,0,(unsigned char *)"0200", 4);
	setbit(&siso,2,(unsigned char *)"6222620110001030171", 19);
	setbit(&siso,3,(unsigned char *)"200000", 6);
	setbit(&siso,7,(unsigned char *)tmpbuf, 10);
	
	//前置系统报文21域规则：
	setbit(&siso,11,(unsigned char *)"424691", 6);//前置系统流水
	setbit(&siso,21,(unsigned char *)"00022900007000000003000", 23);
	setbit(&siso,22,(unsigned char *)"022", 3);
	setbit(&siso,25,(unsigned char *)"00", 2);
	setbit(&siso,35,(unsigned char *)"6222620110001030171=1912220165332468", 36);
	setbit(&siso,36,(unsigned char *)"996222620110001030171=1561560500050001328013000000010000000000===0332468165", 75);
	
	setbit(&siso,41,(unsigned char *)"11223344",8);
	setbit(&siso,42,(unsigned char *)"860010030210046",15);
	
	setbit(&siso,48,(unsigned char *)"000200000020000023711223344860010030210046",40);//交易码3,交易处理码6,消息类型4,终端流水6,pos终端终端号6,pos终端商户号15
	
	setbit(&siso,49,(unsigned char *)"156",3);
    setbit(&siso,60,(unsigned char *)"000000000900000000000010000000",30);
    setbit(&siso,61,(unsigned char *)"0000004246901816340706181634",28);//原交易授权号	N6参考号	N12时间N10
*/
/*
    dcs_log(0,0," 模拟POS消费退货的交易 start");//0,1,2,3,4,7,11,21,22,25,35,36,41,42,49,60,61
	setbit(&siso,0,(unsigned char *)"0200", 4);
	setbit(&siso,2,(unsigned char *)"6222620110001030171", 19);
	setbit(&siso,3,(unsigned char *)"270000", 6);
	setbit(&siso,7,(unsigned char *)tmpbuf, 10);
	setbit(&siso,4,(unsigned char *)"000000070000", 12);
	//前置系统报文21域规则：
	setbit(&siso,11,(unsigned char *)"424682", 6);//前置系统流水
	setbit(&siso,21,(unsigned char *)"00022900007027000003000", 23);
	setbit(&siso,22,(unsigned char *)"022", 3);
	setbit(&siso,25,(unsigned char *)"00", 2);
	setbit(&siso,35,(unsigned char *)"6222620110001030171=1912220165332468", 36);
	setbit(&siso,36,(unsigned char *)"996222620110001030171=1561560500050001328013000000010000000000===0332468165", 75);
	
	setbit(&siso,41,(unsigned char *)"11223344",8);
	setbit(&siso,42,(unsigned char *)"860010030210046",15);
	
	setbit(&siso,49,(unsigned char *)"156",3);
    setbit(&siso,60,(unsigned char *)"000000000900000000000010000000",30);
    setbit(&siso,61,(unsigned char *)"0000004246811729450706172945",28);//原交易授权号	N6参考号	N12时间N10
 */
 /*  
    printf("start to test 200000!");
    dcs_log(0,0," 模拟POS消费撤销冲正交易 start");//0,1,2,3,4,7,11,21,22,25,41,42,48,60,61
	setbit(&siso,0,(unsigned char *)"0400", 4);
	setbit(&siso,2,(unsigned char *)"6225220108926189", 19);
	setbit(&siso,3,(unsigned char *)"200000", 6);
	setbit(&siso,4,(unsigned char *)"000000700000", 12);//消费金额
	setbit(&siso,7,(unsigned char *)tmpbuf, 10);
	
	//前置系统报文21域规则：
	setbit(&siso,11,(unsigned char *)"000346", 6);//前置系统流水
	setbit(&siso,21,(unsigned char *)"00022900007020000003000", 23);
	setbit(&siso,22,(unsigned char *)"022", 3);
	setbit(&siso,25,(unsigned char *)"00", 2);
	
	setbit(&siso,41,(unsigned char *)"11223344",8);
	setbit(&siso,42,(unsigned char *)"860010030210046",15);
	setbit(&siso,48,(unsigned char *)"000200000040000022911223344860010030210046",40);//交易码3,交易处理码6,消息类型4,终端流水6,pos终端终端号6,pos终端商户号15
	setbit(&siso,49,(unsigned char *)"156",3);
    setbit(&siso,60,(unsigned char *)"000000000900000000000010000000",30);
    setbit(&siso,61,(unsigned char *)"0000004246911736350706173635",28);//原交易授权号	N6参考号	N12时间N10
*/

	printf("just a test\n");
	dcs_log(0,0," 模拟POS消费交易发送给前置 start");//0,1,3,4,11,22,25,26,35,36,41,42,49,52,53,60,64
	setbit(&siso,0,(unsigned char *)"0200", 4);
	setbit(&siso,3,(unsigned char *)"000000", 6);
	setbit(&siso,4,(unsigned char *)"000000700000", 12);//消费金额
	
	//前置系统报文21域规则：
	setbit(&siso,11,(unsigned char *)"000900", 6);//pos终端上送流水
	setbit(&siso,20,(unsigned char *)"00000", 5);
	setbit(&siso,21,(unsigned char *)"14NP8110710333950000", 20);
	setbit(&siso,22,(unsigned char *)"022", 3);
	setbit(&siso,25,(unsigned char *)"00", 2);
	//setbit(&siso,26,(unsigned char *)"06", 2);
	setbit(&siso,35,(unsigned char *)"6228210660015825514D49121206363770000", 37);
	setbit(&siso,36,(unsigned char *)"996228210660015825514D156156000000000000000000000011414144912DD000000000000D000000000000D095302700000000", 104);
	setbit(&siso,41,(unsigned char *)"12345924",8);
	setbit(&siso,42,(unsigned char *)"999682907440095",15);
	//setbit(&siso,52,(unsigned char *)"F33B25652C1BD648",16);
	/*
	memset( tmpbuf, 0x0, sizeof(tmpbuf) );
	sprintf( bcdbuf, "111111" );
	GetPin( bcdbuf, strlen(bcdbuf), "6228210660015825514", tmpbuf );
	setbit(&siso,52,(unsigned char *)tmpbuf, 8 );
	if ( strlen( PIN_KEY ) > 16 )
		sprintf( tmpbuf, "2600000000000000" );
	else
		sprintf( tmpbuf, "2000000000000000" );
	*/
	sprintf( tmpbuf, "0600000000000000" );
	setbit(&siso,53,(unsigned char *)tmpbuf, strlen(tmpbuf) );
	//setbit(&siso,53,(unsigned char *)"2600000000000000",16);
	
	setbit(&siso,49,(unsigned char *)"156",3);
    setbit(&siso,60,(unsigned char *)"22000111000601",14);
    SendDataToFold( &siso );	
    TransRcvSvrExit(0);

	exit(0);
    
/*
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
    
    if(fold_get_maxmsg(gs_myFid) < 10)
	    fold_set_maxmsg(gs_myFid, 20) ;

	fold_write(gs_myFid,-1,buffer2,strlen(buffer2));
	
	dcs_log( 0,0, "Write finished!");
	
	TransRcvSvrExit(0);

	exit(0);
*/
}

int SendDataToFold( ISO_data * iso )
{
	char	buffer[1024], newbuf[1024];
	char    tmpbuf[200], bcdbuf[1024];
	int	len, s,Fid;

	setbit( iso, 64,(unsigned char *)"00000000000", 8 );
	memset( newbuf, 0x0, sizeof( newbuf ) );

	len = GetPospMacBlk(iso,newbuf);
	if ( len <= 0 )
		return -1;
		
	memset( bcdbuf, 0x0, sizeof( bcdbuf ) );
	s = GetPospMAC( newbuf, len, bcdbuf, 0 );

	dcs_debug( bcdbuf, 8, "GetPospMAC return[%d]", s );
	if ( s < 0 )
		return -1;

	memset( tmpbuf, 0x0, sizeof(tmpbuf) );
	bcd_to_asc((unsigned char *)tmpbuf,(unsigned char *)bcdbuf,16,0);
	setbit(iso, 64, (unsigned char *)tmpbuf, 8 );

	iso8583 =&iso8583_conf[0];
  	SetIsoHeardFlag(0);
    SetIsoFieldLengthFlag(0);

	memset(buffer, 0, sizeof(buffer));
	s = iso_to_str( (unsigned char *)buffer, iso, 1);
	if ( s < 0 )
	{
		dcs_log(0, 0, "iso_to_str error.");
		exit( 0 );
	}

	memset(newbuf, 0, sizeof(newbuf));
	memset(tmpbuf, 0, sizeof(tmpbuf));
	sprintf( newbuf, "6000030021602200000000" );
	
	asc_to_bcd( (unsigned char *) tmpbuf, (unsigned char*) newbuf, 22, 0 );
	memset(newbuf, 0, sizeof(newbuf));
	memcpy(newbuf,"TPOS",4);//pos发起
	memcpy( newbuf+4, tmpbuf, 11);
	sprintf(newbuf+15,"086001003021004611223344%04d",s);//添加报文头2
	memcpy( newbuf+43, buffer, s);
	s = s + 43;

	dcs_debug( newbuf, s, "buffer len:%d,", s);
	
    
    dcs_log(0,0, "attach to folder system ok !\n");
    
    gs_myFid = fold_locate_folder("TRANSRCV");
    
	if(gs_myFid < 0)        
    {
        dcs_log(0,0,"cannot get folder '%s':%s\n", "TRANSRCV", ise_strerror(errno) );
        return -1;
    }
     
    dcs_log(0,0, "folder myFid=%d", gs_myFid);
    

	  Fid = fold_locate_folder("TM04");
    
	if(Fid < 0)        
    {
        dcs_log(0,0,"cannot get folder '%s':%s\n", "TM04", ise_strerror(errno) );
        return -1;
    }
     
    dcs_log(0,0, "folder TM04 Fid=%d", Fid);

	fold_write(gs_myFid,Fid,newbuf, s );
	
	dcs_log( 0,0, "Write finished,Write length =%d",s);
	
	return 0;
}

int GetPospMacBlk( ISO_data *iso, char * macblk )
{
	char buffer[2048];
	int len;

	memset(buffer, 0, sizeof(buffer));
	iso8583 =&iso8583_conf[0];
  	SetIsoHeardFlag(0);
    SetIsoFieldLengthFlag(0);

	len = iso_to_str((unsigned char *)buffer,iso, 0);
	dcs_debug( buffer, len, "buffer len=%d.", len );
	memcpy( macblk, buffer, len - 8);
	return len - 8;
}

int GetPospMAC(  char *source, int len, char * dest, int flag )
{
	int i, j, k, s;
	unsigned char tmp[9], tmp1[9],tmp2[9],tmp3[9];
	unsigned char buff[18], bcd[9];
	
	char bcdbuf[200];
	unsigned char inkey[9], mainkey[33];
	
	if ( source == NULL || dest == NULL)
		return -1;
	


	memset( inkey, 0x0, sizeof(inkey) );
	memset( bcdbuf, 0x0, sizeof(bcdbuf) );
	memset( mainkey, 0x0, sizeof(mainkey) );
	
	asc_to_bcd( (unsigned char *) bcdbuf, (unsigned char*) MAC_KEY, 16, 0);
	asc_to_bcd( (unsigned char *) mainkey, (unsigned char*) MAIN_KEY, 32, 0);
	//des( (unsigned char *) bcdbuf, (unsigned char*) inkey, (unsigned char *) mainkey, 0 );
	D3des( (unsigned char *) bcdbuf, (unsigned char*) inkey, (unsigned char *) mainkey, 0 );

	dcs_debug( inkey, 8, "旧 MAC_KEY=%s.", MAC_KEY );
	
	if ( len % 8 )
	{
		i = len / 8 + 1;
		memset( (void*) (source + len), 0x0, 8 );		// 后补0x0
	}
	else
		i = len / 8;
	memset( (void*) bcd, 0x0, sizeof(bcd) );
	//dcs_debug( bcd, 8, "des turn start." );
	for ( j = 0; j < i; j++ )
	{
		memcpy( (void*) tmp, (void*)(source + 8*j), 8 );
		//dcs_debug( tmp, 8, "src turn=%d.", j );
		for ( k = 0; k < 8; k++ )
		{
			bcd[k] = tmp[k] ^ bcd[ k];
		}
		//dcs_debug( bcd, 8, "MAC turn=%d.", j );
	}
	dcs_debug( bcd, 8, "XOR" );
	memset( buff, 0x0, sizeof( buff ) );
	bcd_to_asc( (unsigned char*) buff, (unsigned char *)bcd, 16, 0 );

	dcs_log( 0, 0, "buff=%s", buff );
	
	memset( tmp1, 0x0, sizeof( tmp1 ) );
	memset( tmp2, 0x0, sizeof( tmp2 ) );
	memset( tmp3, 0x0, sizeof( tmp3 ) );
	
	memcpy( tmp1, buff, 8 );
	memcpy( tmp2, buff + 8, 8 );
	dcs_debug( tmp1, 8, "XOR 1" );
	dcs_debug( tmp2, 8, "XOR 2" );

	s = des( tmp1, tmp3, inkey, 1 );

	dcs_debug( tmp3, 8, "des 1 s=%d.", s );
	for ( k = 0; k < 8; k++ )
	{
		tmp2[k] = tmp2[k] ^ tmp3[k];
	}
	memset( tmp3, 0x0, sizeof( tmp3 ) );
	s = des( tmp2, tmp3, inkey, 1 );

	dcs_debug( tmp3, 8, "des 2 s=%d", s );
	memcpy( (void*) dest, tmp3, 8 );
	dest[8] = 0;

	return 0;
}

/* ================================================================ 
des() Description: DES algorithm,do 加密 (1) or 解密 (0). 
================================================================ */ 
int des(unsigned char *source,unsigned char * dest,unsigned char * inkey, int flg) 
{ 
	unsigned char bufout[64], 
	kwork[56], worka[48], kn[48], buffer[64], key[64], 
	nbrofshift, temp1, temp2; 
	int valindex; 
	int i, j, k, iter; 

	if ( source == NULL || dest == NULL || inkey == NULL )
		return -1;
	/* INITIALIZE THE TABLES */ 
	/* Table - s1 */ 
	static unsigned char s1[4][16] = { 
					14, 4, 13, 1, 2, 15, 11, 8, 3, 10, 6, 12, 5, 9, 0, 7, 
					0, 15, 7, 4, 14, 2, 13, 1, 10, 6, 12, 11, 9, 5, 3, 8, 
					4, 1, 14, 8, 13, 6, 2, 11, 15, 12, 9, 7, 3, 10, 5, 0, 
					15, 12, 8, 2, 4, 9, 1, 7, 5, 11, 3, 14, 10, 0, 6, 13 }; 
	/* Table - s2 */ 
	static unsigned char s2[4][16] = { 
					15, 1, 8, 14, 6, 11, 3, 4, 9, 7, 2, 13, 12, 0, 5, 10, 
					3, 13, 4, 7, 15, 2, 8, 14, 12, 0, 1, 10, 6, 9, 11, 5, 
					0, 14, 7, 11, 10, 4, 13, 1, 5, 8, 12, 6, 9, 3, 2, 15, 
					13, 8, 10, 1, 3, 15, 4, 2, 11, 6, 7, 12, 0, 5, 14, 9 }; 
	/* Table - s3 */ 
	static unsigned char s3[4][16] = { 
					10, 0, 9, 14, 6, 3, 15, 5, 1, 13, 12, 7, 11, 4, 2, 8, 
					13, 7, 0, 9, 3, 4, 6, 10, 2, 8, 5, 14, 12, 11, 15, 1, 
					13, 6, 4, 9, 8, 15, 3, 0, 11, 1, 2, 12, 5, 10, 14, 7, 
					1, 10, 13, 0, 6, 9, 8, 7, 4, 15, 14, 3, 11, 5, 2, 12 }; 
	/* Table - s4 */ 
	static unsigned char s4[4][16] = { 
					7, 13, 14, 3, 0, 6, 9, 10, 1, 2, 8, 5, 11, 12, 4, 15, 
					13, 8, 11, 5, 6, 15, 0, 3, 4, 7, 2, 12, 1, 10, 14, 9, 
					10, 6, 9, 0, 12, 11, 7, 13, 15, 1, 3, 14, 5, 2, 8, 4, 
					3, 15, 0, 6, 10, 1, 13, 8, 9, 4, 5, 11, 12, 7, 2, 14 }; 
	/* Table - s5 */ 
	static unsigned char s5[4][16] = { 
					2, 12, 4, 1, 7, 10, 11, 6, 8, 5, 3, 15, 13, 0, 14, 9, 
					14, 11, 2, 12, 4, 7, 13, 1, 5, 0, 15, 10, 3, 9, 8, 6, 
					4, 2, 1, 11, 10, 13, 7, 8, 15, 9, 12, 5, 6, 3, 0, 14, 
					11, 8, 12, 7, 1, 14, 2, 13, 6, 15, 0, 9, 10, 4, 5, 3 }; 
	/* Table - s6 */ 
	static unsigned char s6[4][16] = { 
					12, 1, 10, 15, 9, 2, 6, 8, 0, 13, 3, 4, 14, 7, 5, 11, 
					10, 15, 4, 2, 7, 12, 9, 5, 6, 1, 13, 14, 0, 11, 3, 8, 
					9, 14, 15, 5, 2, 8, 12, 3, 7, 0, 4, 10, 1, 13, 11, 6, 
					4, 3, 2, 12, 9, 5, 15, 10, 11, 14, 1, 7, 6, 0, 8, 13 }; 
	/* Table - s7 */ 
	static unsigned char s7[4][16] = { 
					4, 11, 2, 14, 15, 0, 8, 13, 3, 12, 9, 7, 5, 10, 6, 1, 
					13, 0, 11, 7, 4, 9, 1, 10, 14, 3, 5, 12, 2, 15, 8, 6, 
					1, 4, 11, 13, 12, 3, 7, 14, 10, 15, 6, 8, 0, 5, 9, 2, 
					6, 11, 13, 8, 1, 4, 10, 7, 9, 5, 0, 15, 14, 2, 3, 12 }; 
	/* Table - s8 */ 
	static unsigned char s8[4][16] = { 
					13, 2, 8, 4, 6, 15, 11, 1, 10, 9, 3, 14, 5, 0, 12, 7, 
					1, 15, 13, 8, 10, 3, 7, 4, 12, 5, 6, 11, 0, 14, 9, 2, 
					7, 11, 4, 1, 9, 12, 14, 2, 0, 6, 10, 13, 15, 3, 5, 8, 
					2, 1, 14, 7, 4, 10, 8, 13, 15, 12, 9, 0, 3, 5, 6, 11 }; 
	
	/* Table - Shift */ 
	static unsigned char shift[16] = { 
	1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1 }; 
	
	/* Table - Binary */ 
	static unsigned char binary[64] = { 
					0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 1, 
					0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 
					1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 
					1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1 }; 
	/* MAIN PROCESS */ 
	/* Convert from 64-bit key into 64-byte key */ 
	for (i = 0; i < 8; i++) { 
		key[8*i] = ((j = *(inkey + i)) / 128) % 2; 
		key[8*i+1] = (j / 64) % 2; 
		key[8*i+2] = (j / 32) % 2; 
		key[8*i+3] = (j / 16) % 2; 
		key[8*i+4] = (j / 8) % 2; 
		key[8*i+5] = (j / 4) % 2; 
		key[8*i+6] = (j / 2) % 2; 
		key[8*i+7] = j % 2; 
	} 
	/* Convert from 64-bit data into 64-byte data */ 
	for (i = 0; i < 8; i++) { 
		buffer[8*i] = ((j = *(source + i)) / 128) % 2; 
		buffer[8*i+1] = (j / 64) % 2; 
		buffer[8*i+2] = (j / 32) % 2; 
		buffer[8*i+3] = (j / 16) % 2; 
		buffer[8*i+4] = (j / 8) % 2; 
		buffer[8*i+5] = (j / 4) % 2; 
		buffer[8*i+6] = (j / 2) % 2; 
		buffer[8*i+7] = j % 2; 
	} 
	/* Initial Permutation of Data */ 
	bufout[ 0] = buffer[57]; 
	bufout[ 1] = buffer[49]; 
	bufout[ 2] = buffer[41]; 
	bufout[ 3] = buffer[33]; 
	bufout[ 4] = buffer[25]; 
	bufout[ 5] = buffer[17]; 
	bufout[ 6] = buffer[ 9]; 
	bufout[ 7] = buffer[ 1]; 
	bufout[ 8] = buffer[59]; 
	bufout[ 9] = buffer[51]; 
	bufout[10] = buffer[43]; 
	bufout[11] = buffer[35]; 
	bufout[12] = buffer[27]; 
	bufout[13] = buffer[19]; 
	bufout[14] = buffer[11]; 
	bufout[15] = buffer[ 3]; 
	bufout[16] = buffer[61]; 
	bufout[17] = buffer[53]; 
	bufout[18] = buffer[45]; 
	bufout[19] = buffer[37]; 
	bufout[20] = buffer[29]; 
	bufout[21] = buffer[21]; 
	bufout[22] = buffer[13]; 
	bufout[23] = buffer[ 5]; 
	bufout[24] = buffer[63]; 
	bufout[25] = buffer[55]; 
	bufout[26] = buffer[47]; 
	bufout[27] = buffer[39]; 
	bufout[28] = buffer[31]; 
	bufout[29] = buffer[23]; 
	bufout[30] = buffer[15]; 
	bufout[31] = buffer[ 7]; 
	bufout[32] = buffer[56]; 
	bufout[33] = buffer[48]; 
	bufout[34] = buffer[40]; 
	bufout[35] = buffer[32]; 
	bufout[36] = buffer[24]; 
	bufout[37] = buffer[16]; 
	bufout[38] = buffer[ 8]; 
	bufout[39] = buffer[ 0]; 
	bufout[40] = buffer[58]; 
	bufout[41] = buffer[50]; 
	bufout[42] = buffer[42]; 
	bufout[43] = buffer[34]; 
	bufout[44] = buffer[26]; 
	bufout[45] = buffer[18]; 
	bufout[46] = buffer[10]; 
	bufout[47] = buffer[ 2]; 
	bufout[48] = buffer[60]; 
	bufout[49] = buffer[52]; 
	bufout[50] = buffer[44]; 
	bufout[51] = buffer[36]; 
	bufout[52] = buffer[28]; 
	bufout[53] = buffer[20]; 
	bufout[54] = buffer[12]; 
	bufout[55] = buffer[ 4]; 
	bufout[56] = buffer[62]; 
	bufout[57] = buffer[54]; 
	bufout[58] = buffer[46]; 
	bufout[59] = buffer[38]; 
	bufout[60] = buffer[30]; 
	bufout[61] = buffer[22]; 
	bufout[62] = buffer[14]; 
	bufout[63] = buffer[ 6]; 
	/* Initial Permutation of Key */ 
	kwork[ 0] = key[56]; 
	kwork[ 1] = key[48]; 
	kwork[ 2] = key[40]; 
	kwork[ 3] = key[32]; 
	kwork[ 4] = key[24]; 
	kwork[ 5] = key[16]; 
	kwork[ 6] = key[ 8]; 
	kwork[ 7] = key[ 0]; 
	kwork[ 8] = key[57]; 
	kwork[ 9] = key[49]; 
	kwork[10] = key[41]; 
	kwork[11] = key[33]; 
	kwork[12] = key[25]; 
	kwork[13] = key[17]; 
	kwork[14] = key[ 9]; 
	kwork[15] = key[ 1]; 
	kwork[16] = key[58]; 
	kwork[17] = key[50]; 
	kwork[18] = key[42]; 
	kwork[19] = key[34]; 
	kwork[20] = key[26]; 
	kwork[21] = key[18]; 
	kwork[22] = key[10]; 
	kwork[23] = key[ 2]; 
	kwork[24] = key[59]; 
	kwork[25] = key[51]; 
	kwork[26] = key[43]; 
	kwork[27] = key[35]; 
	kwork[28] = key[62]; 
	kwork[29] = key[54]; 
	kwork[30] = key[46]; 
	kwork[31] = key[38]; 
	kwork[32] = key[30]; 
	kwork[33] = key[22]; 
	kwork[34] = key[14]; 
	kwork[35] = key[ 6]; 
	kwork[36] = key[61]; 
	kwork[37] = key[53]; 
	kwork[38] = key[45]; 
	kwork[39] = key[37]; 
	kwork[40] = key[29]; 
	kwork[41] = key[21]; 
	kwork[42] = key[13]; 
	kwork[43] = key[ 5]; 
	kwork[44] = key[60]; 
	kwork[45] = key[52]; 
	kwork[46] = key[44]; 
	kwork[47] = key[36]; 
	kwork[48] = key[28]; 
	kwork[49] = key[20]; 
	kwork[50] = key[12]; 
	kwork[51] = key[ 4]; 
	kwork[52] = key[27]; 
	kwork[53] = key[19]; 
	kwork[54] = key[11]; 
	kwork[55] = key[ 3]; 
	/* 16 Iterations */ 
	for (iter = 1; iter < 17; iter++) { 
		for (i = 0; i < 32; i++) 
		{
			buffer[i] = bufout[32+i]; 
		}
		/* Calculation of F(R, K) */ 
		/* Permute - E */ 
		worka[ 0] = buffer[31]; 
		worka[ 1] = buffer[ 0]; 
		worka[ 2] = buffer[ 1]; 
		worka[ 3] = buffer[ 2]; 
		worka[ 4] = buffer[ 3]; 
		worka[ 5] = buffer[ 4]; 
		worka[ 6] = buffer[ 3]; 
		worka[ 7] = buffer[ 4]; 
		worka[ 8] = buffer[ 5]; 
		worka[ 9] = buffer[ 6]; 
		worka[10] = buffer[ 7]; 
		worka[11] = buffer[ 8]; 
		worka[12] = buffer[ 7]; 
		worka[13] = buffer[ 8]; 
		worka[14] = buffer[ 9]; 
		worka[15] = buffer[10]; 
		worka[16] = buffer[11]; 
		worka[17] = buffer[12]; 
		worka[18] = buffer[11]; 
		worka[19] = buffer[12]; 
		worka[20] = buffer[13]; 
		worka[21] = buffer[14]; 
		worka[22] = buffer[15]; 
		worka[23] = buffer[16]; 
		worka[24] = buffer[15]; 
		worka[25] = buffer[16]; 
		worka[26] = buffer[17]; 
		worka[27] = buffer[18]; 
		worka[28] = buffer[19]; 
		worka[29] = buffer[20]; 
		worka[30] = buffer[19]; 
		worka[31] = buffer[20]; 
		worka[32] = buffer[21]; 
		worka[33] = buffer[22]; 
		worka[34] = buffer[23]; 
		worka[35] = buffer[24]; 
		worka[36] = buffer[23]; 
		worka[37] = buffer[24]; 
		worka[38] = buffer[25]; 
		worka[39] = buffer[26]; 
		worka[40] = buffer[27]; 
		worka[41] = buffer[28]; 
		worka[42] = buffer[27]; 
		worka[43] = buffer[28]; 
		worka[44] = buffer[29]; 
		worka[45] = buffer[30]; 
		worka[46] = buffer[31]; 
		worka[47] = buffer[ 0]; 
		/* KS Function Begin */ 
		if (flg) { 
			 nbrofshift = shift[iter-1]; 
			 for (i = 0; i < (int) nbrofshift; i++) { 
				  temp1 = kwork[0]; 
				  temp2 = kwork[28]; 
				  for (j = 0; j < 27; j++) { 
					   kwork[j] = kwork[j+1]; 
					   kwork[j+28] = kwork[j+29]; 
				  } 
				  kwork[27] = temp1; 
				  kwork[55] = temp2; 
			 } 
		} else if (iter > 1) { 
			 nbrofshift = shift[17-iter]; 
			 for (i = 0; i < (int) nbrofshift; i++) { 
				  temp1 = kwork[27]; 
				  temp2 = kwork[55]; 
				  for (j = 27; j > 0; j--) { 
					   kwork[j] = kwork[j-1]; 
					   kwork[j+28] = kwork[j+27]; 
				  } 
				  kwork[0] = temp1; 
				  kwork[28] = temp2; 
				 } 
		} 
		/* Permute kwork - PC2 */ 
		kn[ 0] = kwork[13]; 
		kn[ 1] = kwork[16]; 
		kn[ 2] = kwork[10]; 
		kn[ 3] = kwork[23]; 
		kn[ 4] = kwork[ 0]; 
		kn[ 5] = kwork[ 4]; 
		kn[ 6] = kwork[ 2]; 
		kn[ 7] = kwork[27]; 
		kn[ 8] = kwork[14]; 
		kn[ 9] = kwork[ 5]; 
		kn[10] = kwork[20]; 
		kn[11] = kwork[ 9]; 
		kn[12] = kwork[22]; 
		kn[13] = kwork[18]; 
		kn[14] = kwork[11]; 
		kn[15] = kwork[ 3]; 
		kn[16] = kwork[25]; 
		kn[17] = kwork[ 7]; 
		kn[18] = kwork[15]; 
		kn[19] = kwork[ 6]; 
		kn[20] = kwork[26]; 
		kn[21] = kwork[19]; 
		kn[22] = kwork[12]; 
		kn[23] = kwork[ 1]; 
		kn[24] = kwork[40]; 
		kn[25] = kwork[51]; 
		kn[26] = kwork[30]; 
		kn[27] = kwork[36]; 
		kn[28] = kwork[46]; 
		kn[29] = kwork[54]; 
		kn[30] = kwork[29]; 
		kn[31] = kwork[39]; 
		kn[32] = kwork[50]; 
		kn[33] = kwork[44]; 
		kn[34] = kwork[32]; 
		kn[35] = kwork[47]; 
		kn[36] = kwork[43]; 
		kn[37] = kwork[48]; 
		kn[38] = kwork[38]; 
		kn[39] = kwork[55]; 
		kn[40] = kwork[33]; 
		kn[41] = kwork[52]; 
		kn[42] = kwork[45]; 
		kn[43] = kwork[41]; 
		kn[44] = kwork[49]; 
		kn[45] = kwork[35]; 
		kn[46] = kwork[28]; 
		kn[47] = kwork[31]; 
		/* KS Function End */ 
		/* worka XOR kn */ 
		for (i = 0; i < 48; i++) 
			worka[i] = worka[i] ^ kn[i]; 
		/* 8 s-functions */ 
		valindex = s1[2*worka[ 0]+worka[ 5]] 
		[2*(2*(2*worka[ 1]+worka[ 2])+ 
		worka[ 3])+worka[ 4]]; 
		valindex = valindex * 4; 
		kn[ 0] = binary[0+valindex]; 
		kn[ 1] = binary[1+valindex]; 
		kn[ 2] = binary[2+valindex]; 
		kn[ 3] = binary[3+valindex]; 
		valindex = s2[2*worka[ 6]+worka[11]] 
		[2*(2*(2*worka[ 7]+worka[ 8])+ 
		worka[ 9])+worka[10]]; 
		valindex = valindex * 4; 
		kn[ 4] = binary[0+valindex]; 
		kn[ 5] = binary[1+valindex]; 
		kn[ 6] = binary[2+valindex]; 
		kn[ 7] = binary[3+valindex]; 
		valindex = s3[2*worka[12]+worka[17]] 
		[2*(2*(2*worka[13]+worka[14])+ 
		worka[15])+worka[16]]; 
		valindex = valindex * 4; 
		kn[ 8] = binary[0+valindex]; 
		kn[ 9] = binary[1+valindex]; 
		kn[10] = binary[2+valindex]; 
		kn[11] = binary[3+valindex]; 
		valindex = s4[2*worka[18]+worka[23]] 
		[2*(2*(2*worka[19]+worka[20])+ 
		worka[21])+worka[22]]; 
		valindex = valindex * 4; 
		kn[12] = binary[0+valindex]; 
		kn[13] = binary[1+valindex]; 
		kn[14] = binary[2+valindex]; 
		kn[15] = binary[3+valindex]; 
		valindex = s5[2*worka[24]+worka[29]] 
		[2*(2*(2*worka[25]+worka[26])+ 
		worka[27])+worka[28]]; 
		valindex = valindex * 4; 
		kn[16] = binary[0+valindex]; 
		kn[17] = binary[1+valindex]; 
		kn[18] = binary[2+valindex]; 
		kn[19] = binary[3+valindex]; 
		valindex = s6[2*worka[30]+worka[35]] 
		[2*(2*(2*worka[31]+worka[32])+ 
		worka[33])+worka[34]]; 
		valindex = valindex * 4; 
		kn[20] = binary[0+valindex]; 
		kn[21] = binary[1+valindex]; 
		kn[22] = binary[2+valindex]; 
		kn[23] = binary[3+valindex]; 
		valindex = s7[2*worka[36]+worka[41]] 
		[2*(2*(2*worka[37]+worka[38])+ 
		worka[39])+worka[40]]; 
		valindex = valindex * 4; 
		kn[24] = binary[0+valindex]; 
		kn[25] = binary[1+valindex]; 
		kn[26] = binary[2+valindex]; 
		kn[27] = binary[3+valindex]; 
		valindex = s8[2*worka[42]+worka[47]] 
		[2*(2*(2*worka[43]+worka[44])+ 
		worka[45])+worka[46]]; 
		valindex = valindex * 4; 
		kn[28] = binary[0+valindex]; 
		kn[29] = binary[1+valindex]; 
		kn[30] = binary[2+valindex]; 
		kn[31] = binary[3+valindex]; 
		/* Permute - P */ 
		worka[ 0] = kn[15]; 
		worka[ 1] = kn[ 6]; 
		worka[ 2] = kn[19]; 
		worka[ 3] = kn[20]; 
		worka[ 4] = kn[28]; 
		worka[ 5] = kn[11]; 
		worka[ 6] = kn[27]; 
		worka[ 7] = kn[16]; 
		worka[ 8] = kn[ 0]; 
		worka[ 9] = kn[14]; 
		worka[10] = kn[22]; 
		worka[11] = kn[25]; 
		worka[12] = kn[ 4]; 
		worka[13] = kn[17]; 
		worka[14] = kn[30]; 
		worka[15] = kn[ 9]; 
		worka[16] = kn[ 1]; 
		worka[17] = kn[ 7]; 
		worka[18] = kn[23]; 
		worka[19] = kn[13]; 
		worka[20] = kn[31]; 
		worka[21] = kn[26]; 
		worka[22] = kn[ 2]; 
		worka[23] = kn[ 8]; 
		worka[24] = kn[18]; 
		worka[25] = kn[12]; 
		worka[26] = kn[29]; 
		worka[27] = kn[ 5]; 
		worka[28] = kn[21]; 
		worka[29] = kn[10]; 
		worka[30] = kn[ 3]; 
		worka[31] = kn[24]; 
		/* bufout XOR worka */ 
		for (i = 0; i < 32; i++) { 
		bufout[i+32] = bufout[i] ^ worka[i]; 
		bufout[i] = buffer[i]; 
		} 
	} /* End of for Iter */ 
	/* Prepare Output */ 
	for (i = 0; i < 32; i++) { 
		j = bufout[i]; 
		bufout[i] = bufout[32+i]; 
		bufout[32+i] = j; 
	} 
	/* Inverse Initial Permutation */ 
	buffer[ 0] = bufout[39]; 
	buffer[ 1] = bufout[ 7]; 
	buffer[ 2] = bufout[47]; 
	buffer[ 3] = bufout[15]; 
	buffer[ 4] = bufout[55]; 
	buffer[ 5] = bufout[23]; 
	buffer[ 6] = bufout[63]; 
	buffer[ 7] = bufout[31]; 
	buffer[ 8] = bufout[38]; 
	buffer[ 9] = bufout[ 6]; 
	buffer[10] = bufout[46]; 
	buffer[11] = bufout[14]; 
	buffer[12] = bufout[54]; 
	buffer[13] = bufout[22]; 
	buffer[14] = bufout[62]; 
	buffer[15] = bufout[30]; 
	buffer[16] = bufout[37]; 
	buffer[17] = bufout[ 5]; 
	buffer[18] = bufout[45]; 
	buffer[19] = bufout[13]; 
	buffer[20] = bufout[53]; 
	buffer[21] = bufout[21]; 
	buffer[22] = bufout[61]; 
	buffer[23] = bufout[29]; 
	buffer[24] = bufout[36]; 
	buffer[25] = bufout[ 4]; 
	buffer[26] = bufout[44]; 
	buffer[27] = bufout[12]; 
	buffer[28] = bufout[52]; 
	buffer[29] = bufout[20]; 
	buffer[30] = bufout[60]; 
	buffer[31] = bufout[28]; 
	buffer[32] = bufout[35]; 
	buffer[33] = bufout[ 3]; 
	buffer[34] = bufout[43]; 
	buffer[35] = bufout[11]; 
	buffer[36] = bufout[51]; 
	buffer[37] = bufout[19]; 
	buffer[38] = bufout[59]; 
	buffer[39] = bufout[27]; 
	buffer[40] = bufout[34]; 
	buffer[41] = bufout[ 2]; 
	buffer[42] = bufout[42]; 
	buffer[43] = bufout[10]; 
	buffer[44] = bufout[50]; 
	buffer[45] = bufout[18]; 
	buffer[46] = bufout[58]; 
	buffer[47] = bufout[26]; 
	buffer[48] = bufout[33]; 
	buffer[49] = bufout[ 1]; 
	buffer[50] = bufout[41]; 
	buffer[51] = bufout[ 9]; 
	buffer[52] = bufout[49]; 
	buffer[53] = bufout[17]; 
	buffer[54] = bufout[57]; 
	buffer[55] = bufout[25]; 
	buffer[56] = bufout[32]; 
	buffer[57] = bufout[ 0]; 
	buffer[58] = bufout[40]; 
	buffer[59] = bufout[ 8]; 
	buffer[60] = bufout[48]; 
	buffer[61] = bufout[16]; 
	buffer[62] = bufout[56]; 
	buffer[63] = bufout[24]; 
	j = 0; 
	for (i = 0; i < 8; i++) { 
		*(dest + i) = 0x00; 
		for (k = 0; k < 7; k++) 
			*(dest + i) = ((*(dest + i)) + buffer[j+k]) * 2; 
		*(dest + i) = *(dest + i) + buffer[j+7]; 
		j += 8; 
	} 
	return 0;
} 

/* ================================================================ 
D3des() 
Description: 3DES algorithm,do 加密 (1) or 解密 (0). 
Parameters: source, dest, inkey are 16 bytes.
================================================================ */ 
int D3des(unsigned char *source,unsigned char * dest,unsigned char * inkey, int flg)
{
	unsigned char buffer[64], key1[9], key2[9], tmp1[9], tmp2[9]; 
	int ret; 

	if ( source == NULL || dest == NULL || inkey == NULL )
		return -1;

	memset( key1, 0x0, sizeof( key1 ) );
	memset( key2, 0x0, sizeof( key2 ) );
	memcpy( key1, inkey, 8 );
	memcpy( key2, inkey + 8, 8 );
	if ( flg == 1 )	/*加密*/
	{
		if( getstrlen(source) == 8 )
		{
			memset( tmp1, 0x0, sizeof( tmp1 ) );
			ret = des( source, tmp1, key1, 1 );
			if ( ret  < 0 )
				return -1;
			memset( tmp2, 0x0, sizeof( tmp2 ) );
			ret = des( tmp1, tmp2, key2, 0 );
			if ( ret  < 0 )
				return -1;
			ret = des( tmp2, dest, key1, 1 );
			if ( ret  < 0 )
				return -1;
		}else
		{
			memset( tmp1, 0x0, sizeof( tmp1 ) );
			ret = des( source, tmp1, key1, 1 );
			if ( ret  < 0 )
				return -1;
			memset( tmp2, 0x0, sizeof( tmp2 ) );
			ret = des( tmp1, tmp2, key2, 0 );
			if ( ret  < 0 )
				return -1;
			ret = des( tmp2, dest, key1, 1 );
			if ( ret  < 0 )
				return -1;
			memset( tmp1, 0x0, sizeof( tmp1 ) );
			ret = des( source + 8, tmp1, key1, 1 );
			if ( ret  < 0 )
				return -1;
			memset( tmp2, 0x0, sizeof( tmp2 ) );
			ret = des( tmp1, tmp2, key2, 0 );
			if ( ret  < 0 )
				return -1;
			ret = des( tmp2, dest + 8, key1, 1 );
			if ( ret  < 0 )
				return -1;
		}
	} else 
	{
		if( getstrlen(source) == 8 )
		{
			memset( tmp1, 0x0, sizeof( tmp1 ) );
			ret = des( source, tmp1, key1, 0 );
			if ( ret  < 0 )
				return -1;
			memset( tmp2, 0x0, sizeof( tmp2 ) );
			ret = des( tmp1, tmp2, key2, 1 );
			if ( ret  < 0 )
				return -1;
			ret = des( tmp2, dest, key1, 0 );
			if ( ret  < 0 )
				return -1;
		}else
		{
			memset( tmp1, 0x0, sizeof( tmp1 ) );
			ret = des( source, tmp1, key1, 0 );
			if ( ret  < 0 )
				return -1;
			memset( tmp2, 0x0, sizeof( tmp2 ) );
			ret = des( tmp1, tmp2, key2, 1 );
			if ( ret  < 0 )
				return -1;
			ret = des( tmp2, dest, key1, 0 );
			if ( ret  < 0 )
				return -1;
			memset( tmp1, 0x0, sizeof( tmp1 ) );
			ret = des( source + 8, tmp1, key1, 0 );
			if ( ret  < 0 )
				return -1;
			memset( tmp2, 0x0, sizeof( tmp2 ) );
			ret = des( tmp1, tmp2, key2, 1 );
			if ( ret  < 0 )
				return -1;
			ret = des( tmp2, dest + 8, key1, 0 );
			if ( ret  < 0 )
				return -1;
		}
	}
	return 0;
}

int iso_to_str( unsigned char * dstr , ISO_data * iso, int flag )
{
	int flaghead, flaglength, flagmap;
	int len, slen, i, s;
	char buffer[2048], tmpbuf[999], bcdbuf[999], newbuf[999];
	char map[25];
	char dispinfo[8192];

	iso8583=&iso8583_conf[0];  
	flaghead = GetIsoHeardFlg();
	flaglength = GetFieldLeagthFlag();
	//dcs_log( 0, 0, "iso_to_str:flaghead=%d, flaglength=%d.", flaghead, flaglength );
	sprintf( dispinfo, "iso_to_str: flaghead=%d, flaglength=%d.\n", flaghead, flaglength );
	sprintf( dispinfo, "%s-------------------------------------------------------------------\n", dispinfo );
	flagmap = 0;
	memset( buffer, 0x0, sizeof(buffer) );
	memset( map, 0x0, sizeof( map ) );
	memset( tmpbuf, 0x0, sizeof( tmpbuf ) );
	s = getbit( iso, 0, (unsigned char*) tmpbuf );
	//dcs_debug( tmpbuf, s, "Get bit[0  ] len=%d.", s );
	sprintf( dispinfo, "%s[%3.3d] [%-16.16s] [%3.3d] [%s]\n", dispinfo, 0, iso8583name[0], s, tmpbuf );
	if ( s < 4 )
	{
		len = -1;
		goto isoend;
	}
	if ( flaghead == 1 )  ///不压缩
	{
		memcpy( dstr, tmpbuf, 4 );
		len = 4;
	} else	///压缩
	{
		memset( bcdbuf, 0x0, sizeof( bcdbuf ) );
		asc_to_bcd( (unsigned char*) bcdbuf, (unsigned char*) tmpbuf, 4, 0 );  //暂时不考虑左边加0的情况
		memcpy( dstr, bcdbuf, 2 );
		len = 2;
	}
	slen = 0;
	for ( i = 1; i < 192; i++ )
	{
		s = getbit( iso, i + 1, (unsigned char *)tmpbuf );
		if ( s > 0 )
		{
			tmpbuf[ s] = 0x0;
			if ( i == 63 || i == 127 || i == 191 )
			{
				memset( bcdbuf, 0x0, sizeof( bcdbuf ) );
				bcd_to_asc( (unsigned char*) bcdbuf, (unsigned char*) tmpbuf, s  * 2, 0 );
				sprintf( newbuf, bcdbuf );
			} else
			{
				memset( newbuf, 0x0, sizeof( newbuf ) );
				memcpy( newbuf, tmpbuf, s );
			}
			//if ( i == 34 || i == 35 )
			//{
			//	newbuf[19] = 0;
			//}
			if ( i == 51 )
			{
				memset( bcdbuf, 0x0, sizeof( bcdbuf ) );
				bcd_to_asc( (unsigned char*) bcdbuf, (unsigned char*) tmpbuf, s  * 2, 0 );
				sprintf( newbuf, bcdbuf );
				//newbuf[0] = 0;
			}
			sprintf( dispinfo, "%s[%3.3d] [%-16.16s] [%3.3d] [%1.1d] [%1.1d] [%s]\n", dispinfo, i + 1, iso8583name[i + 1], s, iso8583_conf[i].flag, iso8583_conf[i].type, newbuf );
			//dcs_debug( tmpbuf, s, "Get bit[%3.3d] len=%d. flag=%d, type=%d.", i + 1, s, iso8583_conf[i].flag, iso8583_conf[i].type );
			map[ i / 8 ] = map[ i / 8 ] | (0x80 >> ( i % 8 ) );
			if ( i > 64 )
			{
				map[0] = map[0] | 0x80;
				flagmap = 1;
			}
			if ( i > 128 )
			{
				map[8] = map[8] | 0x80;
				flagmap = 2;
			}
			if ( iso8583_conf[i].flag == 0 )	////固定长
			{
				if ( iso8583_conf[i].type == 0 )  ///不压缩
				{
					memcpy( buffer + slen, tmpbuf, s );
					slen += s;
					if ( iso8583_conf[i].len != s )	//长度不够(多或少）
					{
						slen += iso8583_conf[i].len - s;
					}
				} else	///压缩
				{
					memset( bcdbuf, 0x0, sizeof( bcdbuf ) );
					asc_to_bcd( (unsigned char*) bcdbuf, (unsigned char*) tmpbuf, s, 0 );  //暂时不考虑左边加0的情况
					s = (s + 1) / 2;
					memcpy( buffer + slen, bcdbuf, s );
					slen += s;
				}
			} else		////可变长
			{
				if ( iso8583_conf[i].flag == 1 ) //2位可变长
				{
					if ( flaglength == 1 )
					{
						sprintf( buffer + slen, "%02d", s );
						slen += 2;
					} else
					{
						buffer[slen] = (unsigned char)( (s / 10) *16 + s % 10);
						slen += 1;
					}
				} else
				{
					if ( flaglength == 1 )
					{
						sprintf( buffer + slen, "%03d", s );
						slen += 3;
					} else
					{
						buffer[slen] = (unsigned char)(s / 100) ;
						slen += 1;
						buffer[slen] = (unsigned char)( ((s % 100) / 10) *16 + s % 10);
						slen += 1;
					}
				}
				if ( iso8583_conf[i].type == 0 )  ///不压缩
				{
					memcpy( buffer + slen, tmpbuf, s );
					slen += s;
				} else	///压缩
				{
					memset( bcdbuf, 0x0, sizeof( bcdbuf ) );
					asc_to_bcd( (unsigned char*) bcdbuf, (unsigned char*) tmpbuf, s, 0 );  //暂时不考虑左边加0的情况
					s = (s + 1) / 2;
					memcpy( buffer + slen, bcdbuf, s );
					slen += s;
				}
			}
		} else if ( s < 0 )  ////getbit()
		{
			len = -1;
			goto isoend;
		}
	} // end of for
	memset( tmpbuf, 0x0, sizeof( tmpbuf ) );
	bcd_to_asc( (unsigned char *) tmpbuf, (unsigned char *)map, (flagmap + 1 ) * 8 * 2, 0 );
	sprintf( dispinfo, "%s[%3.3d] [%-16.16s] [%3.3d] [%s]\n", dispinfo, 1, iso8583name[1], (flagmap + 1 ) * 8, tmpbuf );
	//dcs_debug( map, (flagmap + 1) * 8, "Get bitmap len=%d.", (flagmap + 1) * 8 );
	memcpy( dstr + len, map, (flagmap + 1) * 8 );
	len += (flagmap + 1) * 8;
	memcpy( dstr + len, buffer, slen );
	len += slen;
isoend:
	sprintf( dispinfo, "%s-------------------------------------------------------------------", dispinfo );
	if ( flag )
		dcs_log( 0, 0, "%s", dispinfo );

	return len;
}

int str_to_iso( unsigned char * dstr, ISO_data * iso, int Len )
{
	int flaghead, flaglength, flagmap;
	int len, slen, i, s, j;
	char buffer[2048], tmpbuf[999], bcdbuf[999];
	char dispinfo[8192];
	unsigned char bit[193];
	unsigned char tmp;

	iso8583=&iso8583_conf[0];  
	flaghead = GetIsoHeardFlg();
	flaglength = GetFieldLeagthFlag();
	sprintf( dispinfo, "iso_to_str:flaghead=%d, flaglength=%d.\n", flaghead, flaglength );
	sprintf( dispinfo, "%s-------------------------------------------------------------------\n", dispinfo );
	//dcs_log( 0, 0, "iso_to_str:flaghead=%d, flaglength=%d.", flaghead, flaglength );
	if ( Len < 10 )
		return -1;
	clearbit( iso );
	memset( bit, 0x0, sizeof(bit) );
	flagmap = 0;
	slen = 0;
	memset( tmpbuf, 0x0, sizeof( tmpbuf ) );
	if ( flaghead == 0 )	///压缩
	{
		bcd_to_asc( (unsigned char*) tmpbuf, (unsigned char*)dstr, 4, 0 );
		slen += 2;
	} else		///不压缩
	{
		memcpy( tmpbuf, dstr, 4 );
		slen += 4;
	}
	setbit( iso, 0, (unsigned char*) tmpbuf, 4 );
	//dcs_log( tmpbuf, 4, "bit_%-3.3d [%-3.3d]: %s", 0, 4, tmpbuf );
	sprintf( dispinfo, "%s[%3.3d] [%-16.16s] [%3.3d] [%s]\n", dispinfo, 0, iso8583name[0], 4, tmpbuf );
	if ( (dstr[slen] & 0x80) != 0 )
	{
		flagmap = 1;
		if ( (dstr[slen + 8] & 0x80) != 0 )
			flagmap = 2;
	}
	for ( i = 0; i <= flagmap; i++ )
	{
		for ( j = 1; j < 64; j++ )
		{
			bit[ i * 8 * 8 + j ] = (0x80 >> (j % 8) ) & dstr[ slen + i * 8 + j / 8 ];
		}
	}
	//dcs_log( dstr + slen, (flagmap + 1 ) * 8, "bit_%-3.3d [%-3.3d]", 1, (flagmap + 1 ) * 8 );
	memset( tmpbuf, 0x0, sizeof( tmpbuf ) );
	bcd_to_asc( (unsigned char *) tmpbuf, (unsigned char *)dstr + slen, (flagmap + 1 ) * 8 * 2, 0 );
	sprintf( dispinfo, "%s[%3.3d] [%-16.16s] [%3.3d] [%s]\n", dispinfo, 1, iso8583name[1], (flagmap + 1 ) * 8, tmpbuf );
	slen += (flagmap + 1 ) * 8;
	if ( slen > Len )
		return -1;

	for ( i = 1; i < (flagmap + 1 ) * 8 * 8; i++ )
	{
		if ( bit[i] == 0 )
			continue;
		//dcs_log( dstr + slen, 3, "Set bit[%d] slen=%d. flag=%d, type=%d.", i + 1, slen, iso8583_conf[i].flag, iso8583_conf[i].type );
		if ( iso8583_conf[i].flag == 0 )	////固定长
		{
			len = iso8583_conf[i].len;
		} else if ( iso8583_conf[i].flag == 1 )	////LLVAR可变长
		{
			if ( flaglength == 0 )		///长度压缩
			{
				tmp = dstr[slen];
				len = (tmp & 0xF0) / 16 * 10 + ( tmp & 0x0F );
				slen += 1;
			} else
			{
				sscanf( (const char *)dstr + slen, "%2d", &len );
				slen += 2;
			}
		} else		///LLLVAR可变长
		{
			if ( flaglength == 0 )		///长度压缩
			{
				tmp = dstr[slen];
				len = ( tmp & 0x0F ) * 100;
				tmp = dstr[slen + 1];
				len += (tmp & 0xF0) / 16 * 10 + ( tmp & 0x0F );
				slen += 2;
			} else
			{
				sscanf( (const char *) dstr + slen, "%3d", &len );
				slen += 3;
			}
		}
		//dcs_log( 0, 0, "slen=%d, len=%d.", slen, len );
		memset( tmpbuf, 0x0, sizeof(tmpbuf) );
		if ( iso8583_conf[i].type == 0 )  ///不压缩
		{
			memcpy( tmpbuf, dstr + slen, len );
			slen += len;
		} else	///压缩	暂不考虑左添加
		{
			memset( bcdbuf, 0x0, sizeof( bcdbuf ) );
			memcpy( bcdbuf, dstr + slen, (len + 1 ) / 2 );
			bcd_to_asc( (unsigned char*) tmpbuf, (unsigned char*) bcdbuf, ((len + 1 ) / 2) * 2, 0 );  //暂时不考虑左边加0的情况
			slen += (len + 1 ) / 2;
			tmpbuf[len] = 0x0;
			if ( i + 1 == 35 || i + 1 == 36 )
			{
				for ( s = 0; s < len; s ++ )
				{
					if ( tmpbuf[s] == 'D' )
						tmpbuf[s] = '=';
				}
			}
		}
		//dcs_log( tmpbuf, len, "bit_%-3.3d [%-3.3d]: %s", i + 1, len, tmpbuf );
		s = setbit( iso, i+1, (unsigned char *)tmpbuf, len );
		if ( slen > Len || s < 0 )
		{
			dcs_log( 0, 0, "Analyse Packet length error: bit[%d], len=%d. slen[%d] Len[%d] s[%d]", i + 1, len, slen, Len, s );
			dcs_log( 0, 0, "%s", dispinfo );
			return -1;
		}
		if ( i == 63 || i == 127 || i == 191 )
		{
			memset( bcdbuf, 0x0, sizeof( bcdbuf ) );
			bcd_to_asc( (unsigned char*) bcdbuf, (unsigned char*) tmpbuf, len  * 2, 0 );
			sprintf( tmpbuf, bcdbuf );
		}
		if ( i == 34 || i == 35 )
		{
			tmpbuf[19] = 0;
		}
		if ( i == 51 )
		{
			tmpbuf[0] = 0;
		}
		sprintf( dispinfo, "%s[%3.3d] [%-16.16s] [%3.3d] [%s]\n", dispinfo, i + 1, iso8583name[i + 1], len, tmpbuf );
	}
	//dcs_log( 0, 0, "bit_end" );
	sprintf( dispinfo, "%s-------------------------------------------------------------------", dispinfo );
	dcs_log( 0, 0, "%s", dispinfo );
	return 0;
}

int GetPin( char * PinBlk, int len, char * cardno, char * pin )
{
	char	buffer[1024], newbuf[1024];
	char    tmpbuf[200], bcdbuf[200], card[20];
	int	    i, s;
	unsigned char inkey[33], mainkey[33];

	memset( inkey, 0x0, sizeof(inkey) );
	memset( bcdbuf, 0x0, sizeof(bcdbuf) );
	memset( mainkey, 0x0, sizeof(mainkey) );
	if ( strlen( PIN_KEY ) > 16 )
	{
		asc_to_bcd( (unsigned char *) bcdbuf, (unsigned char*) PIN_KEY, 32, 0);
		asc_to_bcd( (unsigned char *) mainkey, (unsigned char*) MAIN_KEY, 32, 0);
		D3des( (unsigned char *) bcdbuf, (unsigned char*) inkey, (unsigned char *) mainkey, 0 );
	} else
	{
		asc_to_bcd( (unsigned char *) bcdbuf, (unsigned char*) PIN_KEY, 16, 0);
		asc_to_bcd( (unsigned char *) mainkey, (unsigned char*) MAIN_KEY, 16, 0);
		des( (unsigned char *) bcdbuf, (unsigned char*) inkey, (unsigned char *) mainkey, 0 );
	}
	dcs_debug( inkey, 16, "PIN PIN_KEY=%s.", PIN_KEY );

	memset( buffer, 0x0, sizeof(buffer) );
	sprintf( buffer, "%02d%sFFFFFFFFFFFFFFFFFFFFFFFF", len, PinBlk );
	dcs_log( 0, 0, "PinBlk=%s, len=%d", buffer, strlen(buffer) );
	memset( tmpbuf, 0x0, sizeof(tmpbuf) );
	asc_to_bcd( (unsigned char*) tmpbuf, (unsigned char*) buffer, 16, 0 );
	dcs_debug( tmpbuf, 9, "PinBlk" );

	memset( bcdbuf, 0x0, sizeof(bcdbuf) );
	sprintf( bcdbuf, "0000000000000000" );
	i = getstrlen( cardno );
	if ( i > 1 )
	{
		s = i;
		for ( i = 1; i <= 12; i++ )
		{
			if ( (s - i) <= 0 )
				break;
			bcdbuf[ 16 - i] = cardno[s - i - 1];
		}
	}
	dcs_debug( bcdbuf, 16, "bcdbuf" );
	memset( card, 0x0, sizeof(card) );
	asc_to_bcd( (unsigned char*) card, (unsigned char*) bcdbuf, 16, 0 );
	dcs_debug( card, 9, "card" );
	for ( i = 0; i < 8; i++ )
	{
		tmpbuf[i] = tmpbuf[i] ^ card[i];
	}
	dcs_debug( tmpbuf, 9, "PinBlk1" );
	memset( bcdbuf, 0x0, sizeof(bcdbuf) );
	if ( strlen( PIN_KEY ) > 16 )
	{
		D3des( (unsigned char*)tmpbuf, (unsigned char*)bcdbuf, (unsigned char*)inkey, 1 );
	} else
	{
		des( (unsigned char*)tmpbuf, (unsigned char*)bcdbuf, (unsigned char*)inkey, 1 );
	}
	dcs_debug( bcdbuf, 8, "Get Pin" );
	memcpy( pin, bcdbuf, 8 );
	return 8;
}
