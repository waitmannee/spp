#include "ibdcs.h"
#include "tmcibtms.h"
#include "iso8583.h"
#include <unistd.h>

static int gs_myFid = -1;

static char   *g_pcBcdaShmPtr;
extern struct ISO_8583 *iso8583;
struct ISO_8583 iso8583_conf[128];

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
    sprintf(file,"%s/moniXpep.log",path);
    return dcs_log_open(file, "moniXpep");
}
/*
模拟二维码验证

*/
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
			if(i!=34 && i!=35)
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
int main(int argc, char *argv[])
{
 	ISO_data siso;
    struct  tm *tm;
	time_t  t;
	char    *ltime;
	char buffer1[4096], buffer2[4096];
	char tmpbuf[100];
	int s = 0;
	char bcd_buf[9];

	memset(&siso,0,sizeof(ISO_data));
	memset(tmpbuf,0,sizeof(tmpbuf));
	memset(bcd_buf,0,sizeof(bcd_buf));

	s = OpenLog();
	if(s < 0)
        exit(1);

    dcs_log(0,0," 模拟二维码验证 start");


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
	/*
	iso_to_str: flaghead=0, flaglength=1.
	-------------------------------------------------------------------
	[000] [消息类型        ] [004] [0200]
	[003] [处理代码        ] [006] [0] [0] [910000]
	[007] [交易日期和时间  ] [010] [0] [0] [0730175738]
	[011] [系统流水号      ] [006] [0] [0] [435059]
	[020] [自定义          ] [005] [1] [0] [D0400]
	[021] [终端信息        ] [051] [1] [0] [86001003021000712345678303D040000020000003300000001]
	[022] [服各点进入方式  ] [003] [0] [0] [011]
	[025] [服务点条件码    ] [002] [0] [0] [00]
	026] [自定义          ] [002] [0] [0] [12]
	[041] [受卡方终端标识  ] [008] [0] [0] [12345678]
	[042] [受卡方标识代码  ] [015] [0] [0] [860010030210007]
	[048] [自定义附加数据  ] [130] [2] [0] [亲爱的会员，您在duiduila上购买了买100就送50的兑兑券，验证码为：238182#34，使用时间为2013-07-01到2013-12-31，请及时使用，以免过期。]
	[049] [交易货币代号    ] [003] [0] [0] [156]
	[052] [个人识别号数据  ] [008] [0] [0] [74F793888C149950]
	[053] [安全控制信息    ] [016] [0] [0] [2600000000000000]
	[060] [MACkey使用标志  ] [020] [2] [0] [00000000030000000000]
	[001] [bitmap          ] [008] [22201CC000C19810]
	-------------------------------------------------------------------
	*/
	
	/*原pos商户号（15字节）+ 原pos终端号（8字节）+ 交易发起方式（1字节，固定值‘3’）+终端类型（2字节，pos设备取值为03）+
	交易码（3字节）+ 当前pos终端流水（6字节）+ 当前交易批次号（6字节） + 终端当前交易处理码（6字节）+ 渠道号（4字节）*/
	setbit(&siso,0,(unsigned char *)"0200", 4);
	setbit(&siso,3,(unsigned char *)"910000", 6);
	setbit(&siso,7,(unsigned char *)tmpbuf, 10);
	setbit(&siso,11,(unsigned char *)tmpbuf+4, 6);
	setbit(&siso,21,(unsigned char *)"99929294812006012345809303D040000290000270000000001", 51);
	setbit(&siso,22,(unsigned char *)"022", 3);
	setbit(&siso,25,(unsigned char *)"65", 2);
	setbit(&siso,26,(unsigned char *)"12", 2);
	setbit(&siso,41,(unsigned char *)"12345809",8);
	setbit(&siso,42,(unsigned char *)"999292948120060",15); //403310048168501  06000001
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	sprintf(tmpbuf, "%s", "亲爱的会员，您在duiduila上购买了买100就送50的兑兑券，验证码为：238182#34，使用时间为2013-07-01到2013-12-31，请及时使用，以免过期。");//二维码信息
	setbit(&siso,48,(unsigned char *)tmpbuf, strlen(tmpbuf));
	setbit(&siso,49,(unsigned char *)"156",3);
	//asc_to_bcd (bcd_buf , "F59016391A888998" , 16 , 0);
	//setbit(&siso,52,(unsigned char *)bcd_buf, 8);
	setbit(&siso,53,(unsigned char *)"2600000000000000",16);
	setbit(&siso,60,(unsigned char *)"000000000900000000000010000000",30);

    iso8583=&iso8583_conf[0];
  	SetIsoHeardFlag(1);
    SetIsoFieldLengthFlag(1);

	memset(buffer1, 0, sizeof(buffer1));
	memset(buffer2, 0, sizeof(buffer2));
	memset(tmpbuf, 0, sizeof(tmpbuf));

	s = iso_to_str((unsigned char *)buffer1, &siso, 0);

	bcd_to_asc((unsigned char *)tmpbuf,
		(unsigned char *)buffer1+4,16,0);
	sprintf(buffer2, "%4.4s%16.16s%s", buffer1, tmpbuf, buffer1+12);


    if(fold_initsys() < 0)
    {
        dcs_log(0,0, "cannot attach to folder system!");
        return -1;
    }

    dcs_log(0,0, "attach to folder system ok !\n");

    gs_myFid = fold_locate_folder("PEX_COM");

	if(gs_myFid < 0)
    {
        dcs_log(0,0,"cannot get folder '%s':%s\n", "PEX_COM",  "error");
        return -1;
    }

    dcs_log(0,0, "folder myFid=%d\n", gs_myFid);


	//dcs_debug(buffer2,strlen(buffer2),"send PEX_COM data length ",buffer2);


	fold_write(gs_myFid,-1,buffer2,strlen(buffer2));

	dcs_log( 0, 0, "Write finished!");
	dcs_debug(buffer2, strlen(buffer2), "test");

	TransRcvSvrExit(0);

	exit(0);
}

