#include "ibdcs.h"
#include "trans.h"
#include "pub_function.h"
#include "memlib.h"

extern struct ISO_8583 *iso8583;
extern struct ISO_8583 iso8583_conf[128];
extern struct ISO_8583 iso8583conf[128];
char g_key[20+1];
static fd55 stFd55_IC[35]={{"9F26"},{"9F27"},{"9F10"},{"9F37"},{"9F36"},{"95"},{"9A"},{"9C"},{"9F02"},{"5F2A"},{"82"},{"9F1A"},{"9F03"},{"9F33"},{"9F74"},
							{"9F34"},{"9F35"},{"9F1E"},{"84"},{"9F09"},{"9F41"},{"91"},{"71"},{"72"},{"DF31"},{"9F63"},{"8A"},{"DF32"},{"DF33"},{"DF34"},
							{"9B"}, {"4F"}, {"5F34"}, {"50"}};
/* 
	模块说明：
		此模块主要处理手机刷卡支付终端发起的消费类交易，不提供冲正接口，由核心系统做超时控制。EWP系统是手机刷卡支付终端的受理系统，之后按照《前置系统接入报文规范》的报文格式，
	将终端报文送到前置系统。EWP系统和前置系统之间暂时不做mac等信息的校验，但是磁道和密码数据都是密文形式传输。前置系统如果解析磁道或者密码错误的话，直接返回EWP平台终端密钥错误。
	否则就受理该笔交易。解析之后的报文数据首先做分控的管理，通过psam卡卡号查询到该台终端的风控规则，由前置系统做分控处理。对于不满足风控要求的交易，返回EWP平台对于的错误信息。
	最后前置系统组成ISO8583报文之后，送往核心系统。其中消费类的交易中，第三域为910000。
	2012.3.7
 */
void ewpProc(char *srcBuf, int iFid, int iMid, int iLen)
{

	#ifdef __LOG__
		dcs_debug(srcBuf, iLen, "EWP Proc, iFid=%d, iMid=%d, iLen=%d", iFid, iMid, iLen);
	#else
		dcs_log(0, 0, "EWP Proc, iFid=%d, iMid=%d, iLen=%d", iFid, iMid, iLen);
	#endif
	
	/*变量定义*/
	int rtn = 0, oRead = 0, s = 0;
	
	int datalen = 0;
	
	//转出卡卡种
	char outCardType;
	
	//当前日期和时间
	char currentDate[8+1];
	char currentTime[6+1];
	
	//前置系统判断报文类型的字段，此处获得该值，保存到数据库中
	//接收到核心系统返回的数据之后，查询原笔交易,获得该字段判断报文类型,进行相应处理。
	char folderName[4+1];
	
	char tmpbuf[1024], bit_22[4];
	
	//前置系统流水
	char trace[7];
	//原交易流水
	char trace_old[6+1];
	
	//转出卡号
	char extkey[20];
	
	char databuf[PUB_MAX_SLEN + 1];
	
	//按照《前置系统接入报文规范》解析出来的数据放入该结构体中
	EWP_INFO ewp_info; 
	
	//交易类型配置信息表，根据交易码查询数据库表msgchg_info，得到的信息放入该结构体中
	MSGCHGINFO	msginfo;
	
	//终端信息表
	TERMMSGSTRUCT termbuf;
	
	//智能终端个人收单商户配置信息表
	POS_CUST_INFO pos_cust_info;
	
	//终端密钥信息表，通过psam卡号查询数据库表samcard_info数据库表之后，放入该结构体中
	SECUINFO secuinfo;
	
	//ISO8583结构体，送往核心系统的数据
	ISO_data siso;
	
	//数据库交易信息保存的结构体，对于数据库表pep_jnls
	PEP_JNLS pep_jnls; 
	
	//终端风控结构体
	SAMCARD_TRADE_RESTRICT samcard_trade_restric;
	
	// psam卡当前的状态信息结构体
	SAMCARD_STATUS_INFO samcard_status_info;
	
	//终端商户风控结构体
	SAMCARD_MERTRADE_RESTRICT samcard_mertrade_restric;
	//保存到缓存数据memcached结构体
	KEY_VALUE_INFO key_value_info;
	
	//系统自带的时间结构体
	struct  tm *tm;   
	time_t  t;
	
	/*交易手续费*/
	long  transfee;
		
	/*风控实现保存交易次数和当天交易总额*/
	char totalamt[12+1];
	int totalcount;
	/*机具编号*/
	char termidm[20+1];
	/*用户名*/
	char username[20+1];
	/*psamID*/
	char samid[16+1];
	/*主密钥索引*/
	int pik_index, mak_index, tdk_index, zmk_index;
	/*密钥数据*/
	char keydata[120+1];
	/*密钥下载应答*/
	char aprespn[2+1];
	int len=0; /*数据长度*/
	char bcd_buf[10];
	
	char key_para_buf[2048], bcd_key_para_buf[2048];
	char temp_bcd_55[512];
	
	/*
	初始化变量
	*/
	memset(totalamt, 0, sizeof(totalamt));
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(trace, 0, sizeof(trace));
	memset(bit_22, 0, sizeof(bit_22));
	memset(folderName, 0, sizeof(folderName));
	memset(databuf, 0, sizeof(databuf));
	memset(currentDate, 0, sizeof(currentDate));
	memset(currentTime, 0, sizeof(currentTime));
	memset(termidm, 0, sizeof(termidm));
	memset(username, 0, sizeof(username));
	memset(samid, 0, sizeof(samid));
	memset(keydata, 0, sizeof(keydata));
	memset(aprespn, 0, sizeof(aprespn));
	
	memset(&ewp_info,0, sizeof(EWP_INFO));
	memset(&msginfo, 0, sizeof(MSGCHGINFO));
	memset(&siso, 0, sizeof(ISO_data));
	memset(&pep_jnls, 0 ,sizeof(PEP_JNLS));
	memset(&termbuf, 0, sizeof(TERMMSGSTRUCT));
	memset(extkey, 0, sizeof(extkey));
	memset(&secuinfo, 0, sizeof(SECUINFO));
	memset(&samcard_trade_restric, 0, sizeof(SAMCARD_TRADE_RESTRICT));
	memset(&samcard_status_info, 0, sizeof(SAMCARD_STATUS_INFO));
	memset(&samcard_mertrade_restric, 0, sizeof(SAMCARD_MERTRADE_RESTRICT));
	memset(&pos_cust_info, 0, sizeof(POS_CUST_INFO));
	memset(&key_value_info, 0, sizeof(KEY_VALUE_INFO));
	memset(key_para_buf, 0x00, sizeof(key_para_buf));
	memset(bcd_key_para_buf, 0x00, sizeof(bcd_key_para_buf));
	
	memcpy(iso8583_conf,iso8583conf,sizeof(iso8583conf));
	iso8583 = &iso8583_conf[0];
  	SetIsoHeardFlag(0);
    SetIsoFieldLengthFlag(1);
	time(&t);
    tm = localtime(&t);
    
    sprintf(currentDate, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
    sprintf(currentTime, "%02d%02d%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
	
	pep_jnls.trnsndpid = iFid ;
	/*得到交易报文类型,整个报文前4个字节*/
	memset(folderName, 0, sizeof(folderName));
	memcpy(folderName, srcBuf, 4);
	memcpy(pep_jnls.trnsndp, folderName, getstrlen(folderName));
	srcBuf += 4;
	iLen -= 4;

	memset(ewp_info.tpduheader, 0, sizeof(ewp_info.tpduheader));
	memcpy(tmpbuf, srcBuf, 5);
	bcd_to_asc((unsigned char *)ewp_info.tpduheader, (unsigned char *)tmpbuf, 10, 0);
	dcs_log(0, 0, "TPDU：%s", ewp_info.tpduheader);

	/*去除TPDU */
	srcBuf += 5;
	iLen -= 5;
	
	datalen = iLen;
	memcpy(databuf, srcBuf, datalen);

	//防重放
	rtn =ewpPreventReplay(databuf, databuf, datalen);
	if(rtn != 0)
		return;
	
	/*多媒体终端密钥下载*/
	if(memcmp(databuf, "0800", 4)==0 && memcmp(databuf+4, "999998", 6)!=0 && memcmp(databuf+4, "999996", 6)!=0)
	{
		dcs_log(0, 0, "解析EWP多媒体终端密钥下载报文");
		//Resolve(databuf, datalen, &ewp_info);/*解析多媒体终端密钥下载报文*/
		memcpy(termidm, databuf+22, 20);
		termidm[getstrlen(termidm)]=0;
		memcpy(username, databuf+42, 20);
		username[getstrlen(username)]=0;
		rtn = GetPsamUpdate(username, termidm, samid);/*获取PSAM卡号以及更新pos_cust_info*/
		if(rtn < 0)
		{
			dcs_log(0, 0, "操作pos_cust_info数据表失败");
			memcpy(aprespn, "99", 2);
		}
		else
			memcpy(aprespn, "00", 2);
		dcs_log(0,0, "GetPsamUpdate samid=%s", samid);
		rtn  = GetKeyindex(samid, &pik_index, &mak_index, &tdk_index, &zmk_index);/*从samcard_info中获取密钥索引*/
		s = mposKeyDownload(pik_index, mak_index, tdk_index, zmk_index, samid, keydata);/*根据密钥索引获取密钥数据*/
		if(s < 0)
		{
			dcs_log(0, 0, "根据密钥索引获取密钥数据失败");
			memcpy(aprespn, "99", 2);
			memcpy(pep_jnls.aprespn, "99", 2);
		}
		else
		{
			memcpy(aprespn, "00", 2);
			memcpy(pep_jnls.aprespn, "00", 2);
		}
		dcs_log(0,0, "mposKeyDownload samid=%s", samid);
		memset(tmpbuf, 0, sizeof(tmpbuf));
		if(getstrlen(ewp_info.tpduheader)!=0)
		{
			memset(bcd_buf, 0, sizeof(bcd_buf));
			asc_to_bcd((unsigned char *) bcd_buf, (unsigned char*)ewp_info.tpduheader, 10, 0);
	#ifdef __LOG__
			dcs_debug(bcd_buf, 5, "bcd_buf:");
	#endif
			memcpy(tmpbuf, bcd_buf, 1);
			len = 1;
			memcpy(tmpbuf+len, bcd_buf+3, 2);
			len += 2;
			memcpy(tmpbuf+len, bcd_buf+1, 2);
			len += 2;
		}
		memcpy(tmpbuf+len, "0810", 4);
		len +=4;
		memcpy(tmpbuf+len, databuf+4, 58);
		len +=58;
		memcpy(tmpbuf+len, aprespn, 2);
		len +=2;
		if(getstrlen(samid) == 0)
			sprintf(tmpbuf+len, "%016d", 0);
		else	
			sprintf(tmpbuf+len, "%16s", samid);
		len +=16;
		if(getstrlen(keydata) == 0)
			sprintf(tmpbuf+len, "%0120d", 0);
		else
			sprintf(tmpbuf+len, "%120s", keydata);
		len +=120;
		memcpy(tmpbuf+len, databuf+62, 20);
		len +=20;
		WriteKeyBack(tmpbuf, len, iFid);/*写回EWP*/
		//管理类信息保存
		memcpy(ewp_info.consumer_transtype, databuf, 4);
		memcpy(ewp_info.consumer_sentinstitu, databuf+4, 4);
		memcpy(ewp_info.consumer_sentdate, databuf+8, 8);
		memcpy(ewp_info.consumer_senttime, databuf+16, 6);
		memcpy(ewp_info.consumer_phonesnno, termidm, strlen(termidm));
		memcpy(ewp_info.consumer_username, username, strlen(username));
		
		rtn = Ewp_SavaManageInfo(siso, pep_jnls, currentDate, currentTime, ewp_info);
		if(rtn == -1)
		{
			dcs_log(0, 0, "保存EWP管理类信息失败");
			return;
		}
		#ifdef  __TEST__
			dcs_log(0, 0, "保存EWP管理类信息成功");
		#endif
		return;
	}
	

	/*解析收到的EWP的数据*/
	dcs_log(0, 0, "解析EWP报文开始");
	parsedata(databuf, datalen, &ewp_info);
	dcs_log(0, 0, "解析EWP报文结束");
	
	/*如果解析帐单数据失效的话，直接抛弃这个包*/
	if (memcmp(ewp_info.retcode, "00", 2) != 0)
	{
		dcs_log(0, 0, "解析帐单数据错误");
		setbit(&siso, 39, (unsigned char *)RET_CODE_F1, 2);
		/*保存解析到的交易数据到数据表pep_db_log中*/
		memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_F1, 2);
		errorTradeLog(ewp_info, folderName, "解析账单数据错误");
		WriteEWP(siso, ewp_info, pep_jnls,0);
		return;
	}
	
	//获得psam卡的密钥信息
	s = GetSecuInfo(ewp_info.consumer_psamno, &secuinfo);
	if(s < 0)
	{
		dcs_log(0, 0, "查询samcard_info失败, psam卡未找到, consumer_psamno=%s", ewp_info.consumer_psamno);
		setbit(&siso, 39, (unsigned char *)RET_CODE_FC, 2);
		memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_FC, 2);
		errorTradeLog(ewp_info, folderName, "psam卡未找到");
		WriteEWP(siso, ewp_info, pep_jnls, 0);
		return;
	}
	/*
		状态信息判断
	*/
	if(memcmp(secuinfo.sam_status, "00", 2) != 0)
	{
		rtn = GetPsamStatusInfo(secuinfo.sam_status, &samcard_status_info);
		if(rtn == -1)
		{
			dcs_log(0,0,"查询psam当前的状态信息失败");
			setbit(&siso, 39, (unsigned char *)RET_CODE_FY, 2);
			memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_FY, 2);
			errorTradeLog(ewp_info, folderName, "查询psam当前的状态信息失败");
			WriteEWP(siso, ewp_info, pep_jnls, 0);
			return;
		}
		else
		{
			dcs_log(0, 0, "psam当前的状态应答码=[%s]", samcard_status_info.xpep_ret_code);
			setbit(&siso, 39, (unsigned char *)samcard_status_info.xpep_ret_code, 2);
			memcpy(ewp_info.consumer_responsecode, samcard_status_info.xpep_ret_code, 2);
			errorTradeLog(ewp_info, folderName, "设备当前不可用");
			WriteEWP(siso, ewp_info, pep_jnls, 0);
			return;
		}
	}

	/* modify by hw 20140826 IC卡公钥参数下载 处理过程 */
	if( memcmp(ewp_info.consumer_transdealcode, "999998", 6) == 0 )
	{
		if(memcmp(ewp_info.netmanagecode, "1", 1)==0)/*IC卡公钥下载*/
		{
			rtn = EwpGeticKey(&ewp_info, key_para_buf);
			if(rtn < 0)
			{
				dcs_log(0, 0, "获取IC卡公钥数据失败");
				memcpy(aprespn, "99", 2);
				memcpy(pep_jnls.aprespn, "99", 2);
			}
			else
			{
				memcpy(aprespn, "00", 2);
				memcpy(pep_jnls.aprespn, "00", 2);
			}
		}
		else 
		{
			rtn = EwpGeticPara(&ewp_info, key_para_buf);
			if(rtn < 0)
			{
				dcs_log(0, 0, "获取IC卡参数数据失败");
				memcpy(aprespn, "99", 2);
				memcpy(pep_jnls.aprespn, "99", 2);
			}
			else
			{
				memcpy(aprespn, "00", 2);
				memcpy(pep_jnls.aprespn, "00", 2);
			}
		}
		memset(tmpbuf, 0, sizeof(tmpbuf));
		if(getstrlen(ewp_info.tpduheader)!=0)
		{
			memset(bcd_buf, 0, sizeof(bcd_buf));
			asc_to_bcd((unsigned char *) bcd_buf, (unsigned char*)ewp_info.tpduheader, 10, 0);
			memcpy(tmpbuf, bcd_buf, 1);
			len = 1;
			memcpy(tmpbuf+len, bcd_buf+3, 2);
			len += 2;
			memcpy(tmpbuf+len, bcd_buf+1, 2);
			len += 2;
		}
		memcpy(tmpbuf+len, "0810", 4);
		len +=4;
		memcpy(tmpbuf+len, databuf+4, 63);
		len +=63;
		memcpy(tmpbuf+len, aprespn, 2);
		len +=2;
		sprintf(tmpbuf+len, "%s", ewp_info.netmanagecode);
		len +=1;
		memcpy(tmpbuf+len, ewp_info.paraseq, 2);
		len +=2;
		sprintf(tmpbuf+len, "%02d", ewp_info.total_keypara);
		len +=2;
		sprintf(tmpbuf+len,"%03d", getstrlen(key_para_buf));
		len +=3;
		memcpy(tmpbuf+len, key_para_buf, getstrlen(key_para_buf));
		len +=getstrlen(key_para_buf);
		memcpy(tmpbuf+len, databuf+70, 20);
		len +=20;
		
		WriteKeyBack(tmpbuf, len, iFid);
		rtn = Ewp_SavaManageInfo(siso, pep_jnls, currentDate, currentTime, ewp_info);
		if(rtn == -1)
		{
			dcs_log(0, 0, "保存EWP管理类信息失败");
		}
		return;
	}

	/*modify by hw 20140924  IC卡公钥参数下载结束 处理过程 */
	if( memcmp(ewp_info.consumer_transdealcode, "999996", 6) == 0 )
	{
		rtn = EwpUpdatedownloadflag(ewp_info, currentDate, currentTime);
		if(rtn < 0)
		{
			dcs_log(0, 0, "IC卡公钥参数更新下载标识失败");
			memcpy(aprespn, "99", 2);
			memcpy(pep_jnls.aprespn, "99", 2);
		}
		else
		{
			memcpy(aprespn, "00", 2);
			memcpy(pep_jnls.aprespn, "00", 2);
		}
		memset(tmpbuf, 0, sizeof(tmpbuf));
		dcs_log(0, 0, "ewp_info.tpduheader[%s]", ewp_info.tpduheader);
		if(getstrlen(ewp_info.tpduheader)!=0)
		{
			memset(bcd_buf, 0, sizeof(bcd_buf));
			asc_to_bcd((unsigned char *) bcd_buf, (unsigned char*)ewp_info.tpduheader, 10, 0);
	#ifdef __LOG__
			dcs_debug(bcd_buf, 5, "bcd_buf:");
	#endif
			memcpy(tmpbuf, bcd_buf, 1);
			len = 1;
			memcpy(tmpbuf+len, bcd_buf+3, 2);
			len += 2;
			memcpy(tmpbuf+len, bcd_buf+1, 2);
			len += 2;
		}
		memcpy(tmpbuf+len, "0810", 4);
		len +=4;
		memcpy(tmpbuf+len, databuf+4, 63);
		len +=63;
		memcpy(tmpbuf+len, aprespn, 2);
		len +=2;
		sprintf(tmpbuf+len, "%s", ewp_info.netmanagecode);
		len +=1;
		memcpy(tmpbuf+len, databuf+70, 20);
		len +=20;
		
		WriteKeyBack(tmpbuf, len, iFid);
		rtn = Ewp_SavaManageInfo(siso, pep_jnls, currentDate, currentTime, ewp_info);
		if(rtn == -1)
		{
			dcs_log(0, 0, "保存EWP管理类信息失败");
		}
		return;
	}

	/*根据解析到的数据交易码来查询数据库msgchg_info来读取一些信息*/
	dcs_log(0, 0, "本次交易的交易码[%s]", ewp_info.consumer_transcode);
	rtn = Readmsgchg(ewp_info.consumer_transcode, &msginfo);
	if(rtn == -1)
	{
		dcs_log(0, 0, "查询msgchg_info失败,交易未开通,trancde=%s", ewp_info.consumer_transcode);
		setbit(&siso, 39, (unsigned char *)RET_CODE_FB, 2);
		memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_FB, 2);
		errorTradeLog(ewp_info, folderName, "查询msgchg_info失败,交易未开通");
		WriteEWP(siso, ewp_info, pep_jnls,0);
		return;
	}else
	{	
		if(memcmp(msginfo.flagpe, "Y", 1)!=0 && memcmp(msginfo.flagpe, "y", 1)!= 0 )
		{
			dcs_log(0, 0, "由于系统或者其他原因，交易暂时关闭");
			setbit(&siso, 39, (unsigned char *)RET_TRADE_CLOSED, 2);
			memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_TRADE_CLOSED, 2);
			errorTradeLog(ewp_info, folderName, "由于系统或者其他原因，交易暂时关闭");
			WriteEWP(siso, ewp_info, pep_jnls, 0);
			return;
		}
	}
	
	/*如果是查询类的交易的话就直接根据订单号查询pep_jnls数据表，返回查询到的信息给EWP*/
	if( memcmp(ewp_info.consumer_transdealcode, "9F0000", 6) == 0 )
	{
		rtn = GetInquiryInfo(ewp_info, &pep_jnls);
		if(rtn == -1)
		{
			dcs_log(0, 0, "查询类：这笔交易不存在");
			setbit(&siso, 39, (unsigned char *)RET_CODE_F4, 2);
			memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_F4, 2);
			errorTradeLog(ewp_info,folderName, "查询类：想要查询的交易不存在" );
			WriteEWP(siso, ewp_info,pep_jnls, 0);
			return;
		}
		WriteEWP(siso, ewp_info, pep_jnls, 1);
		return;
	}
	
	if(memcmp(ewp_info.consumer_transtype, "0620", 4)!=0 && memcmp(ewp_info.consumer_transdealcode,"310000",6) == 0)
	{
		/*余额查询交易上送核心23、55域*/
		/*卡片序列号存在标识为1：标识可以获取卡片序列号（23域有值）*/
		if(memcmp(ewp_info.cardseq_flag, "1", 1) == 0 && strlen(ewp_info.cardseq)>0)
		{
			setbit(&siso, 23, (unsigned char *)ewp_info.cardseq, 3);
		}
		
		/*55域长度不为000.表示55域存在，否则不存在55域固定填000*/
		if(memcmp(ewp_info.filed55_length, "000", 3) != 0)
		{
			len = atoi(ewp_info.filed55_length);
			memset(temp_bcd_55, 0x00, sizeof(temp_bcd_55));
			asc_to_bcd((unsigned char *)temp_bcd_55, (unsigned char*)ewp_info.filed55, len, 0);
			setbit(&siso, 55, (unsigned char *)temp_bcd_55, len/2);
		}
		
	}
	else if(memcmp(ewp_info.consumer_transtype, "0620", 4)==0)
	{	
		len = atoi(ewp_info.filed55_length);
#ifdef __LOG__
		dcs_log(0, 0, "脚本通知上送55域[%d][%s]", len, ewp_info.filed55);
#endif
		/*55域长度不为000.表示55域存在，否则不存在55域固定填000*/
		if(memcmp(ewp_info.filed55_length, "000", 3) != 0)
		{
			asc_to_bcd((unsigned char *)temp_bcd_55, (unsigned char*)ewp_info.filed55, len, 0);
			setbit(&siso, 55, (unsigned char *)temp_bcd_55, len/2);
			
#ifdef __LOG__
			dcs_debug(temp_bcd_55, len/2, "下发终端报文55域:");
#endif
		}
	}
	else
	{
		setbit(&siso, 4, (unsigned char *)ewp_info.consumer_money, 12);		
	}
	
	/*转账或是信用卡还款*/
	if(memcmp(ewp_info.consumer_transtype, "0620", 4)!=0 
	&& (memcmp(ewp_info.consumer_transdealcode,"400000",6) == 0
	||memcmp(ewp_info.consumer_transdealcode, "190000", 6) == 0
	||memcmp(ewp_info.consumer_transdealcode, "480000", 6) == 0))
	{
		setbit(&siso, 20, (unsigned char *)ewp_info.incardno, getstrlen(ewp_info.incardno));
		sscanf(ewp_info.transfee+1, "%ld", &transfee);
		
		if(memcmp(ewp_info.transfee, "C", 1)== 0)
		{
			memset(tmpbuf, '0', sizeof(tmpbuf));
			tmpbuf[12] = 0;
			rtn = SumAmt(ewp_info.consumer_money, ewp_info.transfee+1, tmpbuf);
			if(rtn == -1)
			{
				dcs_log(0, 0, "求和错误");
				return;
			}
			#ifdef __LOG__
				dcs_log(0, 0, "tmpbuf =[%s]", tmpbuf);
			#endif
			setbit(&siso, 4, (unsigned char *)tmpbuf, 12);
		}
		else if(memcmp(ewp_info.transfee, "D", 1)== 0)
		{
			memset(tmpbuf, '0', sizeof(tmpbuf));
			tmpbuf[13] = 0;
			rtn = DecAmt(ewp_info.consumer_money, ewp_info.transfee+1, tmpbuf);
			if(rtn == -1)
			{
				dcs_log(0, 0, "求减错误");
				return;
			}
			#ifdef __LOG__
				dcs_log(0, 0, "tmpbuf =[%s]", tmpbuf);
			#endif
			setbit(&siso, 4, (unsigned char *)tmpbuf+1, 12);
		
		}
		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy(tmpbuf, ewp_info.transfee, 1);
		sprintf(tmpbuf+1, "%08ld", transfee);
		setbit(&siso, 28, (unsigned char *)tmpbuf, 9);
	}
	setbit(&siso, 41, (unsigned char *)msginfo.termcde, 8);
	setbit(&siso, 42, (unsigned char *)msginfo.mercode, 15);
	
	/*交易报文处理,消费冲正消息类型置为0400*/
	if(memcmp(ewp_info.consumer_transtype, "0400", 4)==0)
	{
		setbit(&siso, 0, (unsigned char *)"0400", 4);
		setbit(&siso, 3, (unsigned char *)ewp_info.consumer_transdealcode, 6);
	}
	else if(memcmp(ewp_info.consumer_transtype, "0620", 4)==0)	
	{
		setbit(&siso, 0, (unsigned char *)"0620", 4);
		setbit(&siso, 3, (unsigned char *)msginfo.process, 6);
	}
	else
	{
		setbit(&siso, 0, (unsigned char *)msginfo.msgtype, 4);
		setbit(&siso, 3, (unsigned char *)msginfo.process, 6);
	}
		
	setbit(&siso, 2, (unsigned char *)ewp_info.consumer_cardno, getstrlen(ewp_info.consumer_cardno));
	setbit(&siso, 49, (unsigned char *)"156", 3);
	
	//modify at 20140911 组发送核心的60域内容
	memset(tmpbuf, 0, sizeof(tmpbuf));
/*	if(memcmp(ewp_info.poitcde_flag, "1", 1)==0)
		memcpy(tmpbuf, "50", 2);
	else
		memcpy(tmpbuf, "20", 2);
*/
	//写死终端读卡能力为6表示能够读取非接
	memcpy(tmpbuf, "60", 2);
	if(memcmp(ewp_info.consumer_sentinstitu, "0100", 4)==0 
	|| memcmp(ewp_info.consumer_sentinstitu, "0600", 4)==0 
	||memcmp(ewp_info.consumer_sentinstitu, "0900", 4)==0)
		memcpy(tmpbuf+2, "22", 2);
	else if(memcmp(ewp_info.consumer_sentinstitu, "0400", 4)==0 
	|| memcmp(ewp_info.consumer_sentinstitu, "0500", 4)==0 
	|| memcmp(ewp_info.consumer_sentinstitu, "0800", 4)==0)
		memcpy(tmpbuf+2, "11", 2);
	else
		memcpy(tmpbuf+2, "03", 2);
	setbit(&siso, 60, (unsigned char *)tmpbuf, 4);
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memcpy(tmpbuf, currentDate+4, 4);
	memcpy(tmpbuf+4, currentTime, 6);
	setbit(&siso, 7, (unsigned char *)tmpbuf, 10);

	//处理电子现金消费脱机上送多次上送的处理
	if(memcmp(ewp_info.consumer_transcode, "211", 3) ==0)
	{
		//测试
		if(memcmp(ewp_info.consumer_cardno,"6222620110006914700",19) == 0)
			setbit(&siso,14,(unsigned char *)"2312",4);
		rtn = ewpEcOrginTrans(siso, ewp_info,pep_jnls, secuinfo,currentDate,folderName);
		if(rtn != 0)
			return ;
	}
	//取前置系统交易流水
	if( pub_get_trace_from_seq(trace) < 0)
	{
		dcs_log(0, 0, "取前置系统交易流水错误" );
		setbit(&siso, 39, (unsigned char *)RET_CODE_F8, 2);
		memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_F8, 2 );
		errorTradeLog(ewp_info, folderName, "取前置系统交易流水错误");
		WriteEWP(siso, ewp_info, pep_jnls, 0);
		return;
	}
	
	//交易码
	memcpy(termbuf.cTransType, ewp_info.consumer_transcode, 3);
	//cSamID
	memcpy(termbuf.cSamID, ewp_info.consumer_psamno, 16);
	//设置folder名称
	memcpy(termbuf.e1code, folderName, getstrlen(folderName));
	
	//天缘收单和湖北裕农项目的所有交易都需要校验mac，由平台的渠道号来确定，不由ewp控制 --heqingqi
	//盛京银行的项目也要校验mac  -- lp 20140423
	if( secuinfo.channel_id == 2 || secuinfo.channel_id == 5 || secuinfo.channel_id == 8)
	{
		if(secuinfo.channel_id == 2)
		{
			if(memcmp(ewp_info.consumer_sentinstitu, "0400", 4) != 0)
			{
				dcs_log(0, 0, "sk 商户收款报文解析错误,参数配置错误");
				setbit(&siso, 39, (unsigned char *)"F1", 2);
				setbit(&siso, 11, (unsigned char *)trace, 6);
				WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
				WriteEWP(siso, ewp_info, pep_jnls,0);
				return;
			}
		}
		if(secuinfo.channel_id == 5)
		{
			if(memcmp(ewp_info.consumer_sentinstitu, "0500", 4) != 0)
			{
				dcs_log(0, 0, "cz 商户收款报文解析错误,参数配置错误");
				setbit(&siso, 39, (unsigned char *)"F1", 2);
				setbit(&siso, 11, (unsigned char *)trace, 6);
				WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
				WriteEWP(siso, ewp_info, pep_jnls,0);
				return;
			}
		}
		if(secuinfo.channel_id == 8)
		{
			if(memcmp(ewp_info.consumer_sentinstitu, "0800", 4) != 0)
			{
				dcs_log(0, 0, "盛京银行商户收款报文解析错误,参数配置错误");
				setbit(&siso, 39, (unsigned char *)"F1", 2);
				setbit(&siso, 11, (unsigned char *)trace, 6);
				WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
				WriteEWP(siso, ewp_info, pep_jnls,0);
				return;
			}
		}
		#ifndef __TEST__ 
		s = ewphbmac_chack(secuinfo, ewp_info);
		if(s != 0)
		{
			dcs_log(0, 0, "mac校验失败");
			setbit(&siso, 39, (unsigned char *)"A0", 2);
			setbit(&siso, 11, (unsigned char *)trace, 6);
			WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
			WriteEWP(siso, ewp_info, pep_jnls,0);
			return;
		}	
		dcs_log(0, 0, "mac校验成功");
		#endif
		
		s = GetCustInfo(ewp_info.consumer_username, ewp_info.consumer_psamno, &pos_cust_info);
		if( memcmp(msginfo.flag20, "1", 1) == 0)
		{
			if(s < 0)
			{
				dcs_log(0, 0, "查询pos_cust_info失败,username=%s,psam=%s", ewp_info.consumer_username, ewp_info.consumer_psamno);
				setbit(&siso, 39, (unsigned char *)RET_CODE_H2, 2);
				setbit(&siso, 11, (unsigned char *)trace, 6);
				WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
				WriteEWP(siso, ewp_info, pep_jnls,0);
				return;
			}

			setbit(&siso, 20, (unsigned char *)pos_cust_info.incard, getstrlen(pos_cust_info.incard));
			setbit(&siso, 41, (unsigned char *)pos_cust_info.cust_termid, 8);
			setbit(&siso, 42, (unsigned char *)pos_cust_info.cust_merid, 15);
		}
		
		if(s > 0)
		{
			memcpy(pep_jnls.posmercode, pos_cust_info.cust_merid, 15);
			memcpy(pep_jnls.postermcde, pos_cust_info.cust_termid, 8);
		}
		else
			dcs_log(0, 0, "该商户未开通商户收款功能，获取posmercode、postermcde失败");
	}

	/* modify by hw 20140901 TC值上送交易 */
	if( memcmp(ewp_info.consumer_transdealcode, "999997", 6) == 0 )
	{
		rtn = EwpsaveTCvalue(ewp_info, &pep_jnls);
		if(rtn < 0)
		{
			dcs_log(0, 0, "保存TC值失败");
			memcpy(aprespn, "99", 2);
		}
		else
		{
			memcpy(aprespn, "00", 2);
		}

		memset(tmpbuf, 0, sizeof(tmpbuf));
		if(getstrlen(ewp_info.tpduheader)!=0)
		{
			memset(bcd_buf, 0, sizeof(bcd_buf));
			asc_to_bcd((unsigned char *) bcd_buf, (unsigned char*)ewp_info.tpduheader, 10, 0);
			memcpy(tmpbuf, bcd_buf, 1);
			len = 1;
			memcpy(tmpbuf+len, bcd_buf+3, 2);
			len += 2;
			memcpy(tmpbuf+len, bcd_buf+1, 2);
			len += 2;
		}
		memcpy(tmpbuf+len, "0330", 4);
		len +=4;
		memcpy(tmpbuf+len, databuf+4, 63);
		len +=63;
		memcpy(tmpbuf+len, aprespn, 2);
		len +=2;
		
		WriteKeyBack(tmpbuf, len, iFid);
		
		rtn = EwpSendTCvalue(ewp_info, pep_jnls, msginfo, siso, secuinfo, pos_cust_info);
		if(rtn == -1)
		{
			dcs_log(0, 0, "IC卡TC值上送核心处理error");
		}
		else
			dcs_log(0, 0, "IC卡TC值上送核心处理success");
		
		return;
	}

	//设置订单号
	if((memcmp(ewp_info.consumer_transtype, "0620", 4)!=0 && memcmp(ewp_info.consumer_transdealcode, "480000", 6) == 0))
	{
		dcs_log(0, 0, "转账汇款交易");
		s = getstrlen(ewp_info.consumer_orderno);
		memset(tmpbuf, 0, sizeof(tmpbuf));
		if(s < 0)
		{
			sprintf(tmpbuf, "%s%s%s%s", "DY", currentDate+2, currentTime, trace);
			memcpy(ewp_info.consumer_orderno, tmpbuf, 20);
			sprintf(tmpbuf+20, "|%s|%s", ewp_info.incdname, ewp_info.incdbkno);
			setbit(&siso, 48, (unsigned char *)tmpbuf, strlen(tmpbuf));
		}
		else
		{
			sprintf(tmpbuf, "%s|%s|%s", ewp_info.consumer_orderno, ewp_info.incdname, ewp_info.incdbkno);
			setbit(&siso, 48, (unsigned char *)tmpbuf, strlen(tmpbuf));
		}
	}
	else if(memcmp(ewp_info.consumer_transtype, "0620", 4)==0)
	{
		dcs_log(0, 0, "脚本通知交易无订单号");
	}
	else
	{
		s = getstrlen(ewp_info.consumer_orderno);
		if( s > 0)
			setbit(&siso, 48, (unsigned char *)ewp_info.consumer_orderno, s);	
		else
		{
			memset(tmpbuf, 0, sizeof(tmpbuf));
			sprintf(tmpbuf, "%s%s%s%s", "DY", currentDate+2, currentTime, trace);
			memcpy(ewp_info.consumer_orderno, tmpbuf, 20);
			setbit(&siso, 48, (unsigned char *)tmpbuf, 20);
		}
	}

	/* modify by hw at 20140903  IC卡消费交易，转账汇款新增 23,55域*/
	if(memcmp(ewp_info.consumer_transtype, "0620", 4)!=0 
	&& (memcmp(ewp_info.consumer_transdealcode, "000000", 6) == 0 
		|| memcmp(ewp_info.consumer_transdealcode, "480000", 6) == 0
		|| memcmp(ewp_info.consumer_transdealcode, "190000", 6) == 0))
	{
		/*卡片序列号存在标识为1,非降级交易：标识可以获取卡片序列号（23域有值）*/
		if(memcmp(ewp_info.cardseq_flag, "1",1) == 0 && strlen(ewp_info.cardseq)>0)
		{
			setbit(&siso, 23, (unsigned char *)ewp_info.cardseq, 3);
		}
		
		/*55域长度不为000.表示55域存在，否则不存在55域固定填000*/
		if(memcmp(ewp_info.filed55_length, "000", 3) != 0)
		{
			memset(temp_bcd_55, 0x00, sizeof(temp_bcd_55));
			len = atoi(ewp_info.filed55_length); 
			asc_to_bcd((unsigned char *)temp_bcd_55, (unsigned char*)ewp_info.filed55, atoi(ewp_info.filed55_length), 0);
			setbit(&siso, 55, (unsigned char *)temp_bcd_55, atoi(ewp_info.filed55_length)/2);
		}
	}

	/* modify by hw at 20140904  脚本通知交易 */
	if(memcmp(ewp_info.consumer_transtype, "0620", 4) == 0)
	{
		/*3域重新赋值*/
		setbit(&siso, 3, (unsigned char *)ewp_info.consumer_transdealcode, 6);
	#ifdef __LOG__
		dcs_log(0,0,"ewp_info.serinputmode [%s] ", ewp_info.serinputmode);
	#endif
		/* 组装61域，根据原交易参考号查询原POS流水号 */
		memset(trace_old, 0, sizeof(trace_old));
		rtn = GetEwpTrace(&ewp_info, trace_old);
		if(rtn == -1)
		{
			dcs_log(0, 0, "脚本通知交易查询原笔失败");
			return;
		}
		dcs_log(0, 0, "原交易前置流水号%s", trace_old);
		
		if(strlen(ewp_info.cardseq)==3)
		{
			setbit(&siso, 23, (unsigned char *)ewp_info.cardseq, 3);
		}
		
		setbit(&siso, 2, (unsigned char *)ewp_info.consumer_cardno, getstrlen(ewp_info.consumer_cardno));
		setbit(&siso, 41, (unsigned char *)ewp_info.termcde, 8);
		setbit(&siso, 42, (unsigned char *)ewp_info.mercode, 15);
		
		//modify at 20141017
		memset(tmpbuf, 0, sizeof(tmpbuf));
		//写死终端读卡能力为6表示能够读取非接
		memcpy(tmpbuf, "60", 2);
		if(memcmp(ewp_info.consumer_sentinstitu, "0100", 4)==0 
		|| memcmp(ewp_info.consumer_sentinstitu, "0600", 4)==0 
		||memcmp(ewp_info.consumer_sentinstitu, "0900", 4)==0)
			memcpy(tmpbuf+2, "22", 2);
		else if(memcmp(ewp_info.consumer_sentinstitu, "0400", 4)==0 
		|| memcmp(ewp_info.consumer_sentinstitu, "0500", 4)==0 
		|| memcmp(ewp_info.consumer_sentinstitu, "0800", 4)==0)
			memcpy(tmpbuf+2, "11", 2);
		else
			memcpy(tmpbuf+2, "03", 2);
		setbit(&siso, 60, (unsigned char *)tmpbuf, 4);

		if(memcmp(ewp_info.authcode, "      ",6)==0 || strlen(ewp_info.authcode)==0)
		{
			dcs_log(0, 0, "授权码为空");
			memset(ewp_info.authcode,0x00,sizeof(ewp_info.authcode));
			memcpy(ewp_info.authcode, "000000", 6);
		}
		memset(tmpbuf, 0, sizeof(tmpbuf));
		sprintf(tmpbuf,"%s%s%s%s", ewp_info.authcode, ewp_info.consumer_sysreferno, ewp_info.revdate, trace_old);
		setbit(&siso, 61, (unsigned char *)tmpbuf, 34);
		
		/* 转账汇款脚本通知交易需要上送4域金额域 */
		if(memcmp(ewp_info.consumer_transdealcode,"480000",6) == 0
		|| memcmp(ewp_info.consumer_transdealcode,"190000",6) == 0
		|| memcmp(ewp_info.consumer_transdealcode,"000000",6) == 0)
		{
			setbit(&siso, 4, (unsigned char *)ewp_info.consumer_money, 12);
		}
	}
	
	/*设置流水号需在冲正交易之前*/
	setbit(&siso, 11, (unsigned char *)trace, 6);
	setbit(&siso, 22, (unsigned char *)ewp_info.serinputmode, 3);
	setbit(&siso, 25, (unsigned char *)msginfo.service, 2);
	
	/*modify by hw at 20140925 消费冲正*/
	if(memcmp(ewp_info.consumer_transtype, "0400", 4)==0)
	{
		rtn = GetOrgInfo(ewp_info, &pep_jnls);
		if(rtn == -1)
		{
			dcs_log(0, 0, "查询pep_jnls失败, 原笔交易未找到");
			setbit(&siso, 39, (unsigned char *)RET_CODE_F0, 2);
			WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
			WriteEWP(siso, ewp_info, pep_jnls, 0);
			return;
		}
		
		/*3域重新赋值*/
		setbit(&siso, 3, (unsigned char *)"000000", 6);
		/*交易金额*/
		setbit(&siso, 4, (unsigned char *)pep_jnls.tranamt, 12);
		/*服务点输入方式码*/
		setbit(&siso, 22, (unsigned char *)pep_jnls.poitcde, 3);
		/*卡片序列号*/
		if(getstrlen(pep_jnls.card_sn)!=0)
			setbit(&siso, 23, (unsigned char *)pep_jnls.card_sn, 3);
		/*原前置流水号*/
		memset(tmpbuf, 0x00, sizeof(tmpbuf));
		sprintf(tmpbuf, "%06ld", pep_jnls.trace);
		setbit(&siso, 11, (unsigned char *)tmpbuf, 6);
		/*重组冲正交易41,42域*/
		setbit(&siso, 41, (unsigned char *)pep_jnls.termcde, 8);
		setbit(&siso, 42, (unsigned char *)pep_jnls.mercode, 15);
		/*重组冲正交易60域，根据服务点输入方式码确定poitcde_flag，05X表示芯片读取，为IC卡不降级（50），其余为（20）*/
		memset(tmpbuf, 0, sizeof(tmpbuf));
/*		if(memcmp(pep_jnls.poitcde, "05", 2)==0)
			memcpy(tmpbuf, "50", 2);
		else
			memcpy(tmpbuf, "20", 2);
*/
		//写死终端读卡能力为6表示能够读取非接
		memcpy(tmpbuf, "60", 2);
		if(memcmp(ewp_info.consumer_sentinstitu, "0100", 4)==0 
		|| memcmp(ewp_info.consumer_sentinstitu, "0600", 4)==0 
		||memcmp(ewp_info.consumer_sentinstitu, "0900", 4)==0)
			memcpy(tmpbuf+2, "22", 2);
		else if(memcmp(ewp_info.consumer_sentinstitu, "0400", 4)==0 
		|| memcmp(ewp_info.consumer_sentinstitu, "0500", 4)==0 
		|| memcmp(ewp_info.consumer_sentinstitu, "0800", 4)==0)
			memcpy(tmpbuf+2, "11", 2);
		else
			memcpy(tmpbuf+2, "03", 2);
		setbit(&siso, 60, (unsigned char *)tmpbuf, 4);

	}

	//判断卡的种类
	outCardType = ReadCardType(ewp_info.consumer_cardno);
	if(outCardType == -1)
	{
		dcs_log(0, 0, "查询card_transfer失败");
		setbit(&siso, 39, (unsigned char *)RET_CODE_FZ, 2);
		setbit(&siso, 11, (unsigned char *)trace, 6);
		WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
		WriteEWP(siso, ewp_info, pep_jnls,0);
		return;
	}
	pep_jnls.outcdtype = outCardType;

	/*内部商户卡种的限制处理*/
	/*根据卡号判断卡种*/
#ifdef __LOG__
	dcs_log(0, 0, "msginfo.cdlimit=%c",msginfo.cdlimit);
#endif

	if( msginfo.cdlimit == 0x43 || msginfo.cdlimit == 0x63 || msginfo.cdlimit == 0x44  || msginfo.cdlimit == 0x64 )
	{	
		dcs_log(0,0,"有卡种限制");
		if((outCardType == msginfo.cdlimit)||(outCardType == '-'))
		{
			dcs_log(0,0,"该银行卡不支持此功能");
			setbit(&siso,39,(unsigned char *)RET_CODE_F2,2);
			WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
			WriteEWP(siso,ewp_info,pep_jnls,0);
			return;
		}	
	}
	
	/*
	psam卡号（16字节）+ 电话号码，
	区号与电话号码用|区分（或者手机号）（15字节，左对齐，右补空格）
	+ 交易发起方式（1字节，ewp平台送过来的信息） ewp上送的内容修改为2个字节，上送给核心仍然是一个字节
	+ 终端类型（2字节，刷卡设备取值为01）
	+ 交易码（3字节）+ 终端流水（6字节，不存在的话填全0）
	+ 渠道ID(4个字节)
	*/ 
	memset(tmpbuf, 0, sizeof(tmpbuf));
	#ifdef __LOG__
		dcs_log(0, 0, "ewp_info.consumer_psamno=[%s]", ewp_info.consumer_psamno);/*psam卡号*/
		dcs_log(0, 0, "ewp_info.consumer_username=[%s]", ewp_info.consumer_username);/*手机号*/
		dcs_log(0, 0, "ewp_info.translaunchway=[%s]", ewp_info.translaunchway);/*交易发起方式*/
		dcs_log(0, 0, "ewp_info.consumer_transcode=[%s]", ewp_info.consumer_transcode);/*交易码*/
		dcs_log(0, 0, "secuinfo.channel_id=[%ld]", secuinfo.channel_id);/*渠道ID*/
	#endif
	
	if(getstrlen(ewp_info.translaunchway) == 0)
		memcpy(ewp_info.translaunchway, "01", 2);
	sprintf(tmpbuf, "%s%-15s%s%s%s%06s%04d", ewp_info.consumer_psamno, ewp_info.consumer_username, ewp_info.translaunchway+1, "01", ewp_info.consumer_transcode, "000000", secuinfo.channel_id);
	
#ifdef __LOG__
	dcs_log(0, 0, "21域[%d][%s]", strlen(tmpbuf), tmpbuf);
#endif
	/*--- modify begin 140325 ---*/
  	if(getstrlen(pos_cust_info.T0_flag)==0)
    	memcpy(pos_cust_info.T0_flag, "0", 1);
 	if(getstrlen(pos_cust_info.agents_code)==0)
  		memcpy(pos_cust_info.agents_code,"000000",6);
 	sprintf(tmpbuf+47, "%s", pos_cust_info.T0_flag);
  	sprintf(tmpbuf+48, "%s", pos_cust_info.agents_code);
	setbit(&siso, 21, (unsigned char *)tmpbuf, 54);
	
	/*新增对商户收款，撤销退货交易的受理*/
	if(memcmp(ewp_info.consumer_transdealcode, "200000", 6) == 0 || memcmp(ewp_info.consumer_transdealcode, "270000", 6) == 0)
	{
		dcs_log(0, 0, "交易类型[%s]", ewp_info.consumer_transdealcode);
		rtn = GetOrginMerTransInfo(ewp_info, &pep_jnls, currentDate);
		if(rtn == -1)
		{
			dcs_log(0, 0, "查找原笔交易信息失败");
			setbit(&siso, 39, (unsigned char *)RET_CODE_F0, 2);
			WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
			WriteEWP(siso, ewp_info, pep_jnls, 0);
			return;
		}
		if(getstrlen(ewp_info.cardseq)!=0)
			setbit(&siso, 23, (unsigned char *)ewp_info.cardseq, 3);
		setbit(&siso, 41, (unsigned char *)pep_jnls.termcde, 8);
		setbit(&siso, 42, (unsigned char *)pep_jnls.mercode, 15);
		if(memcmp(ewp_info.consumer_transdealcode, "200000", 6) == 0)
		{
			dcs_log(0, 0, "交易类型:商户收款撤销");
			setbit(&siso, 4, (unsigned char *)pep_jnls.tranamt, 12);
		}
		else
		{
			if(memcmp(ewp_info.consumer_transcode, "212", 3) == 0)
				dcs_log(0, 0, "交易类型:商户收款脱机退货");
			else
				dcs_log(0, 0, "交易类型:商户收款退货");
			setbit(&siso, 4, (unsigned char *)ewp_info.consumer_money, 12);		
		}
		
		memset(tmpbuf, 0, sizeof(tmpbuf));
		sprintf(tmpbuf, "%s%s%s", pep_jnls.authcode, pep_jnls.syseqno, pep_jnls.revdate);
		setbit(&siso, 61, (unsigned char *)tmpbuf, 28);
	}

	if(memcmp(ewp_info.consumer_transtype, "0620", 4)!=0 && memcmp(ewp_info.consumer_transtype, "0400", 4)!=0
	&& getstrlen(ewp_info.consumer_trackno) != 0)
	{
		//解密磁道
		s = DecryptTrack(ewp_info.consumer_track_randnum, ewp_info.consumer_trackno, &termbuf, &siso, secuinfo, extkey);
		if(s < 0)
		{
			dcs_log(0, 0, "解密磁道错误, s=%d", s);
			setbit(&siso, 39, (unsigned char *)RET_CODE_F3, 2);
			WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
			WriteEWP(siso, ewp_info, pep_jnls, 0);
			return;
		}
		if(memcmp(ewp_info.consumer_sentinstitu, "0100", 4) ==0 
			|| memcmp(ewp_info.consumer_sentinstitu, "0600", 4) ==0 
			|| memcmp(ewp_info.consumer_sentinstitu, "0900", 4) ==0)
		{
			len = getstrlen(ewp_info.consumer_cardno);
			memset(tmpbuf, 0, sizeof(tmpbuf));
			s = getbit(&siso, 35, (unsigned char *)tmpbuf);
			if(s<0)
			{
				dcs_log(0, 0, "取35域信息失败");
				setbit(&siso, 39, (unsigned char *)RET_CODE_F3, 2);
				WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
				WriteEWP(siso, ewp_info, pep_jnls, 0);
				return;	
			}
			if(memcmp(tmpbuf+len+1+4, "2", 1) == 0 || memcmp(tmpbuf+len+1+4, "6", 1) == 0)
			{
				dcs_log(0, 0, "小刷卡器交易是IC卡的返回45");
				setbit(&siso, 39, (unsigned char *)"45", 2);
				WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
				WriteEWP(siso, ewp_info, pep_jnls, 0);
				return;
			}
		}

		//如果不存在密码，那么就会上送16字节长度的全F或者全f
		if(memcmp(ewp_info.consumer_password, "FFFFFFFFFFFFFFFF", 16) == 0 
			|| memcmp(ewp_info.consumer_password, "ffffffffffffffff", 16) == 0
			|| getstrlen(ewp_info.consumer_password) == 0)
		{
			dcs_log(0, 0, "密码为空，通过msgchg_info判断是否需要密码.");
			if(memcmp(msginfo.flagpwd, "1", 1) == 0 )
			{
				dcs_log(0, 0, "该交易不允许银行卡无密支付，返回EWP FA Error!");
				setbit(&siso, 39, (unsigned char *)RET_CODE_FA, 2);
				WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
				WriteEWP(siso,ewp_info,pep_jnls,0);
				return;
			}
			else
			{
				//setbit(&siso,22,(unsigned char *)"022", 3);//刷卡，无PIN
				setbit(&siso,52,(unsigned char *)NULL, 0);
				setbit(&siso,53,(unsigned char *)NULL, 0);
			}
		}else
		{
			//转加密密码
			memset(tmpbuf, 0, sizeof(tmpbuf));
			s = DecryptPin(ewp_info.consumer_pin_randnum, ewp_info.consumer_password, extkey, &termbuf, tmpbuf, secuinfo);
			if(s < 0)
			{
				dcs_log(0,0,"转加密密码错误,error code=%d",s);
				setbit(&siso,39,(unsigned char *)RET_CODE_F7,2);
				WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
				WriteEWP(siso,ewp_info,pep_jnls,0);
				return; 
			}
			
			setbit(&siso,52, (unsigned char *)tmpbuf, 8);
			//setbit(&siso,22,(unsigned char *)"021", 3); //刷卡，且PIN可输入
			setbit(&siso,53, (unsigned char *)"1000000000000000", 16);
		}
		
		/*
			首先判断该订单是否已经发生过支付。如果发生过支付，并且状态是超时的话，那么判断当前的时间与前一笔交易的时间差，如果小于5分钟，那么就不允许再次发起支付。
		*/
		if(memcmp(ewp_info.consumer_transdealcode, "200000",6) != 0 && memcmp(ewp_info.consumer_transdealcode, "270000",6) != 0)
		{
			s = ewpOrderRepeatPayCharge(ewp_info.consumer_orderno, currentDate, currentTime);
			if(s < 0)
			{
				dcs_log(0, 0, "订单支付状态未知，5分钟之内不允许再次支付");
				setbit(&siso, 39, (unsigned char *)RET_CODE_H0, 2);
				WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
				WriteEWP(siso, ewp_info, pep_jnls,0);
				return;
			}
		}
	}
	/*
		通过psam卡号的渠道号查询本交易是否可以做
		 0：成功
		-1：该渠道该交易不存在
		-2：该渠道限制交易
		-3：该渠道该交易未开通
		-4：数据库查询错误
	*/
	s = ChannelTradeCharge(ewp_info.consumer_transcode, secuinfo.channel_id);
	if(s == -1)
	{
		dcs_log(0,0,"查询ChannelTradeCharge失败,该渠道该交易不存在");
		setbit(&siso,39,(unsigned char *)RET_CODE_FD,2);
		WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
		WriteEWP(siso,ewp_info,pep_jnls,0);
		return;
	}else if(s == -2)
	{
		dcs_log(0,0,"查询ChannelTradeCharge失败,该渠道限制交易");
		setbit(&siso,39,(unsigned char *)RET_CODE_FE,2);
		WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
		WriteEWP(siso,ewp_info,pep_jnls,0);
		return;
	}else if(s == -3)
	{
		dcs_log(0,0,"查询ChannelTradeCharge失败,该渠道该交易未开通");
		setbit(&siso,39,(unsigned char *)RET_CODE_FF,2);
		WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
		WriteEWP(siso,ewp_info,pep_jnls,0);
		return;
	}else
	{
		dcs_log(0,0,"查询ChannelTradeCharge成功，该渠道交易正常");
	}
	
	/*
	TODO... 商户收款交易不需要做以下的风控，但是融易付客户端的账户充值、AA收款、收取功能费，跑一下以下的风控流程。
	*/
	/*商户收款交易，不做后面的风控*/
	//if(memcmp(msginfo.flag20, "1", 1) == 0)
	//{   
	    /*保存数据库*/
	//	rtn = WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
	//	if(rtn == -1)
	//	{
	//		dcs_log(0,0,"保存pep_jnls数据失败");
	//		setbit(&siso,39,(unsigned char *)RET_CODE_FO,2);
	//		memcpy(ewp_info.consumer_responsecode,(unsigned char *)RET_CODE_FO,2);
	//		memcpy(ewp_info.consumer_presyswaterno,trace,6);
	//		errorTradeLog(ewp_info,folderName,"保存pep_jnls数据失败");
	//		WriteEWP(siso,ewp_info,pep_jnls,0);
	//		return;
	//	}
	//
		/*把解析到的数据保存以后需要的信息到数据表ewp_info中*/
	//	rtn = WriteDbEwpInfo(ewp_info);
	//	if(rtn == -1)
	//	{
	//		dcs_log(0,0,"保存EWP_INFO失败");
	//		setbit(&siso,39,(unsigned char *)RET_CODE_FP,2);
	//		updateXpepErrorTradeInfo(ewp_info, trace, RET_CODE_FP);
	//		WriteEWP(siso,ewp_info,pep_jnls,0);
	//		return;
	//	}
	//
		/*发送报文*/
	//	rtn=WriteXpe(siso);
	//	if(rtn == -1)
	//	{
	//		dcs_log(0,0,"发送信息给核心失败");
	//		setbit(&siso,39,(unsigned char *)RET_CODE_F9,2);
	//		updateXpepErrorTradeInfo(ewp_info, trace, RET_CODE_F9);
	//		WriteEWP(siso,ewp_info,pep_jnls,0);
	//		return;
	//	}
	//
	//	return;	
	//}
	
	/*
		获得终端商户风控信息
	*/
	s = GetSamcardMerRestrictInfo(currentDate, ewp_info.consumer_psamno, msginfo.mercode, &samcard_mertrade_restric);
	if(s == -1)
	{
		dcs_log(0,0,"获取终端商户风控数据,数据库操作错误");
		setbit(&siso,39,(unsigned char *)RET_CODE_FZ, 2);
		memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_FZ, 2);
		memcpy(ewp_info.consumer_presyswaterno, trace, 6);
		errorTradeLog(ewp_info, folderName, "获取终端商户风控数据,数据库操作错误");
		WriteEWP(siso, ewp_info, pep_jnls,0);
		return;
	}else if(s == 0)
	{
			dcs_log(0,0,"该终端商户没有风险控制");
	}else
	{
			dcs_log(0,0,"获取终端商户风险控制配置信息成功。");
	}
	
	
	/*
		获得终端风险控制信息
	*/
	s = GetSamcardRestrictInfo(currentDate, ewp_info.consumer_psamno, &samcard_trade_restric);
	if(s == 0)
	{
		dcs_log(0,0,"该终端没有风险控制");
		
	}else if(s == -1){
		dcs_log(0,0,"数据库操作错误");
		setbit(&siso,39,(unsigned char *)RET_CODE_FZ, 2);
		memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_FZ, 2);
		memcpy(ewp_info.consumer_presyswaterno, trace, 6);
		errorTradeLog(ewp_info, folderName, "数据库操作错误");
		WriteEWP(siso, ewp_info, pep_jnls,0);
		return;
	}
	
	/*
		终端分控的判断
		判断该终端是否在黑名单中
	*/
	if( samcard_trade_restric.psam_restrict_flag == 1 && memcmp(samcard_trade_restric.restrict_trade, "-", 1) != 0)
	{
		if( memcmp(samcard_trade_restric.restrict_trade, "*", 1) == 0 && strstr(samcard_trade_restric.restrict_trade,termbuf.cTransType) == NULL )
		{	
			dcs_log(0,0,"该终端属于黑名单，不允许交易");
			setbit(&siso,39,(unsigned char *)RET_CODE_FG,2);
			WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
			WriteEWP(siso,ewp_info,pep_jnls,0);
			return;
		}
	
	}
	
	if(memcmp(ewp_info.consumer_transtype, "0620", 4)!=0 && memcmp(ewp_info.consumer_transdealcode,"310000",6) != 0)
	{
		/*rtn = AmoutLimitDeal(msginfo, ewp_info.consumer_money, outCardType);	*/
		/*
			单笔交易风控
		*/
		#ifdef __LOG__	
  			dcs_log(0, 0, "msginfo.c_amtlimit =[%s]", msginfo.c_amtlimit);
  			dcs_log(0, 0, "msginfo.d_amtlimit =[%s]", msginfo.d_amtlimit);
  			dcs_log(0, 0, "ewp_info.consumer_money =[%s]", ewp_info.consumer_money);
  			dcs_log(0, 0, "outCardType =[%c]", outCardType);
  		#endif
		rtn = AmoutLimitDeal(msginfo.c_amtlimit, msginfo.d_amtlimit, ewp_info.consumer_money, outCardType);
		if(rtn==-1)
		{
			dcs_log(0, 0, "单笔交易风控失败");
			setbit(&siso,39,(unsigned char *)RET_CODE_F6,2);
			WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
			WriteEWP(siso,ewp_info,pep_jnls,0);
			return;
		} 
		if(getstrlen(msginfo.c_sin_totalamt)!=1 || getstrlen(msginfo.d_sin_totalamt)!=1 || msginfo.c_day_count !=0 || msginfo.d_day_count !=0)
		{
			totalcount = getamt_count(currentDate, ewp_info, totalamt);
			if(totalcount == -1)
			{
				dcs_log(0, 0, "数据库获取总金额以及交易次数失败");
				setbit(&siso,39,(unsigned char *)RET_CODE_FJ,2);
				WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
				WriteEWP(siso, ewp_info, pep_jnls, 0);
				return;
			}
			if((outCardType =='D' && getstrlen(msginfo.d_sin_totalamt)!=1) 
			|| (outCardType =='C' && getstrlen(msginfo.c_sin_totalamt)!=1)
			|| (outCardType =='-' && (getstrlen(msginfo.c_sin_totalamt)!=1||getstrlen(msginfo.d_sin_totalamt)!=1)))
			{
				memset(tmpbuf, '0', sizeof(tmpbuf));
				tmpbuf[12] = 0;
				#ifdef __LOG__	
  					dcs_log(0, 0, "ewp_info.consumer_money =[%s], totalamt =[%s]", ewp_info.consumer_money, totalamt);
  				#endif
				rtn = SumAmt(ewp_info.consumer_money, totalamt, tmpbuf);
				if(rtn == -1)
				{
					dcs_log(0, 0, "求和错误");
					return;
				}
				#ifdef __LOG__	
  					dcs_log(0, 0, "tmpbuf =[%s]", tmpbuf);
  				#endif
				rtn = AmoutLimitDeal(msginfo.c_sin_totalamt, msginfo.d_sin_totalamt, tmpbuf, outCardType);
				if(rtn==-1)
				{
					dcs_log(0,0,"日交易金额限制");
					setbit(&siso,39,(unsigned char *)RET_CODE_F6,2);
					WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
					WriteEWP(siso,ewp_info,pep_jnls,0);
					return;
				} 
			}
			if(msginfo.c_day_count !=0 || msginfo.d_day_count !=0)
			{
				#ifdef __LOG__	
  					dcs_log(0, 0, "outCardType =[%c]", outCardType);
  					dcs_log(0, 0, "msginfo.c_day_count =[%d]", msginfo.c_day_count);
  					dcs_log(0, 0, "msginfo.d_day_count =[%d]", msginfo.d_day_count);
  					dcs_log(0, 0, "totalcount =[%d]", totalcount);
  				#endif
				if((outCardType=='C' && msginfo.c_day_count !=0 && totalcount >= msginfo.c_day_count)
				||(outCardType=='D' && msginfo.d_day_count !=0 && totalcount >= msginfo.d_day_count)
				||(outCardType=='-' && msginfo.c_day_count !=0 && msginfo.d_day_count ==0 && totalcount >= msginfo.c_day_count)
				||(outCardType=='-' && msginfo.c_day_count ==0 && msginfo.d_day_count !=0 && totalcount >= msginfo.d_day_count)
				||(outCardType=='-' && msginfo.c_day_count !=0 && msginfo.d_day_count !=0 && totalcount >= (msginfo.c_day_count < msginfo.d_day_count ? msginfo.c_day_count : msginfo.d_day_count)))
				{
					dcs_log(0,0,"成功的交易次数限制");
					setbit(&siso,39,(unsigned char *)"PF",2);
					WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
					WriteEWP(siso,ewp_info,pep_jnls,0);
					return;
				}
			}
		}
	}
	
	/*
		终端商户风控的判断
		判断终端商户是否有信用卡交易日限额的限制
	*/
	if( samcard_mertrade_restric.psammer_restrict_flag == 1 && outCardType=='-' && memcmp(samcard_mertrade_restric.c_amttotal, "-", 1)!=0 )
	{
		dcs_log(0,0,"卡种无法判断，又有交易日限额的控制！交易不给做");
		setbit(&siso,39,(unsigned char *)RET_CODE_FK,2);
		WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
		WriteEWP(siso,ewp_info,pep_jnls,0);
		return;
	}
	
	#ifdef __LOG__
		dcs_log(0,0,"samcard_mertrade_restric.c_amttotal = [%s]", samcard_mertrade_restric.c_amttotal);
		dcs_log(0,0,"outCardType = [%c]", outCardType);
		dcs_log(0,0,"samcard_mertrade_restric.psammer_restrict_flag = [%d]", samcard_mertrade_restric.psammer_restrict_flag);
	#endif
	
	if( samcard_mertrade_restric.psammer_restrict_flag == 1 &&  outCardType == 'C' && memcmp(samcard_mertrade_restric.c_amttotal, "-", 1)!=0 )
	{	
		dcs_log(0,0,"term mer c_amtdate date:[%s]", samcard_mertrade_restric.c_amtdate);
		memset(tmpbuf, '0', sizeof(tmpbuf));
		tmpbuf[12] = 0;
		SumAmt(ewp_info.consumer_money, samcard_mertrade_restric.c_total_tranamt, tmpbuf);
		if(memcmp(tmpbuf, samcard_mertrade_restric.c_amttotal, 12) > 0)
		//if( transamt+samcard_mertrade_restric.c_total_tranamt > samcard_mertrade_restric.c_amttotal)
		{
			dcs_log(0,0,"当前的交易总额，超过总交易额的限制！");
			setbit(&siso,39,(unsigned char *)RET_CODE_FL,2);
			WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
			WriteEWP(siso,ewp_info,pep_jnls,0);
			return;
		}
		pep_jnls.termmeramtlimit = 1;
	}
	
	/*
		终端分控的判断
		判断终端是否有卡种限制
	*/
	
	if( samcard_trade_restric.psam_restrict_flag == 1 && samcard_trade_restric.restrict_card != '-')
	{
		if( outCardType == '-' )
		{
			dcs_log(0,0,"终端卡种限制，未知当前交易卡种");
			setbit(&siso,39,(unsigned char *)RET_CODE_FH,2);
			WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
			WriteEWP(siso,ewp_info,pep_jnls,0);
			return;
		}
		
		if( outCardType == samcard_trade_restric.restrict_card )
		{
			dcs_log(0,0,"终端卡种限制");
			setbit(&siso,39,(unsigned char *)RET_CODE_FI,2);
			WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
			WriteEWP(siso,ewp_info,pep_jnls,0);
			return;
		}
	}
	
	/*
	终端分控的判断
	判断终端限制的最大最小金额
	*/
	if( samcard_trade_restric.psam_restrict_flag == 1 && memcmp(samcard_trade_restric.amtmin, "-", 1)!=0 && memcmp(ewp_info.consumer_money, samcard_trade_restric.amtmin,12)<0)
	{
		dcs_log(0,0,"交易金额,不在最大最小金额的范围！");
		setbit(&siso,39,(unsigned char *)RET_CODE_FJ,2);
		WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
		WriteEWP(siso,ewp_info,pep_jnls,0);
		return;
	}
	
	if( samcard_trade_restric.psam_restrict_flag == 1 && memcmp(samcard_trade_restric.amtmax, "-", 1)!=0 && memcmp(ewp_info.consumer_money, samcard_trade_restric.amtmax,12)>0)
	{
		dcs_log(0,0,"交易金额,不在最大金额的范围！");
		setbit(&siso,39,(unsigned char *)RET_CODE_FJ,2);
		WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
		WriteEWP(siso,ewp_info,pep_jnls,0);
		return;
	}
	
	/*
	终端分控的判断
	判断终端是否有交易日限额的限制
	*/
	if( samcard_trade_restric.psam_restrict_flag == 1 && outCardType=='-' && (memcmp(samcard_trade_restric.c_amttotal, "-", 1)!=0 ||memcmp(samcard_trade_restric.d_amttotal, "-", 1)!=0) )
	{
		dcs_log(0,0,"卡种无法判断，又有交易日限额的控制！交易不给做");
		setbit(&siso,39,(unsigned char *)RET_CODE_FK,2);
		WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
		WriteEWP(siso,ewp_info,pep_jnls,0);
		return;
	}
	
	if( samcard_trade_restric.psam_restrict_flag == 1 &&  outCardType == 'C' && memcmp(samcard_trade_restric.c_amttotal, "-", 1)!=0)
	{	

#ifdef __LOG__
		dcs_log(0,0,"c_amtdate date:%s",samcard_trade_restric.c_amtdate);
#endif
		memset(tmpbuf, '0', sizeof(tmpbuf));
		tmpbuf[12] = 0;
		SumAmt(ewp_info.consumer_money, samcard_trade_restric.c_total_tranamt, tmpbuf);
		//if( transamt+samcard_trade_restric.c_total_tranamt > samcard_trade_restric.c_amttotal)
		if(memcmp(tmpbuf, samcard_trade_restric.c_amttotal, 12)>0)
		{
			dcs_log(0,0,"当前的交易总额，超过总交易额的限制！");
			setbit(&siso,39,(unsigned char *)RET_CODE_FL,2);
			WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
			WriteEWP(siso,ewp_info,pep_jnls,0);
			return;
		}
		pep_jnls.camtlimit=1;
	}
	
	if( samcard_trade_restric.psam_restrict_flag == 1 &&  outCardType=='D'&& memcmp(samcard_trade_restric.d_amttotal,"-",1)!=0)
	{	

#ifdef __LOG__
		dcs_log(0,0,"d_amtdate date:%s",samcard_trade_restric.d_amtdate);
#endif
		memset(tmpbuf, '0', sizeof(tmpbuf));
		tmpbuf[12] = 0;
		SumAmt(ewp_info.consumer_money, samcard_trade_restric.d_total_tranamt, tmpbuf);
		if(memcmp(tmpbuf, samcard_trade_restric.d_amttotal, 12)>0)
		//if( transamt+samcard_trade_restric.d_total_tranamt > samcard_trade_restric.d_amttotal)
		{
			dcs_log(0,0,"当前的交易总额，超过总交易额的限制！");
			setbit(&siso,39,(unsigned char *)RET_CODE_FL,2);
			WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
			WriteEWP(siso,ewp_info,pep_jnls,0);
			return;
		}
		pep_jnls.damtlimit=1;
	}
	
	/*
	风控：1，商户号不在商户黑名单表中
	2，否则的话交易日期不能在添加日期和结束日期之间
	3，否则的话要满足当前的交易码在 交易码列表中
	*/
	rtn = checkMerBlank(msginfo.mercode, termbuf.cTransType);
	if(rtn == -1)
	{
		dcs_log(0,0,"数据库表操作失败");
		setbit(&siso,39,(unsigned char *)RET_CODE_FM,2);
		memcpy(ewp_info.consumer_responsecode,(unsigned char *)RET_CODE_FM,2);
		memcpy(ewp_info.consumer_presyswaterno,trace,6);
		errorTradeLog(ewp_info,folderName,"操作数据库表商户黑名单失败");
		WriteEWP(siso,ewp_info,pep_jnls,0);
		return;
	}else if(rtn == 1)
	{
		dcs_log(0,0,"该商户不允许该交易，mercode = %s, trancde = %s",msginfo.mercode,termbuf.cTransType);
		setbit(&siso,39,(unsigned char *)RET_CODE_FN,2);
		WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
		WriteEWP(siso,ewp_info,pep_jnls,0);
		return;
	}else
	{
		dcs_log(0,0,"该商户不在商户黑名单中，mercode = %s, trancde = %s",msginfo.mercode,termbuf.cTransType);
	}	

	//脱机消费
	if(memcmp(ewp_info.consumer_transcode, "211", 3) ==0)
	{
		//超时控制
		char bcdbuf[512];
		memset(g_key, 0, sizeof(g_key));
		sprintf(g_key,"%8s%6s%6s",currentDate,currentTime,trace);

		memset(bcdbuf, 0x00, sizeof(bcdbuf));
		asc_to_bcd((unsigned char*)bcdbuf, (unsigned char*)ewp_info.filed55,atoi(ewp_info.filed55_length),0);
		//解析55域
		rtn = analyse55(bcdbuf, atoi(ewp_info.filed55_length)/2, stFd55_IC, 34);
		if(rtn < 0 )
		{
			dcs_log(0, 0, "解析55域失败,解析结果[%d]", rtn);
			return ;
		}
		else
		{
			memcpy(pep_jnls.tc_value, stFd55_IC[0].szValue, 16);
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy(tmpbuf, currentDate, 2);
			memcpy(tmpbuf+2, stFd55_IC[6].szValue, 6);
			memcpy(pep_jnls.trndate, tmpbuf, 8);
			memcpy(pep_jnls.trntime, "000000", 6);
	#ifdef __LOG__
			dcs_log(0, 0, "TC值:[%s]", pep_jnls.tc_value, pep_jnls.trndate);
			dcs_log(0, 0, "pep_jnls.trndate=[%s], pep_jnls.trntime=[%s]", pep_jnls.trndate, pep_jnls.trntime);
	#endif
		}
	}

	/*
		保存数据库
	*/
	if(memcmp(ewp_info.consumer_transtype, "0620", 4) == 0)
	{
		rtn = Ewp_SavaManageInfo(siso, pep_jnls, currentDate, currentTime, ewp_info);
		if(rtn == -1)
		{
			dcs_log(0, 0, "保存EWP管理类信息失败,脚本上送通知交易");
			return;
		}	
	}
	else	
	{
		rtn = WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
		if(rtn == -1)
		{
			dcs_log(0,0,"保存pep_jnls数据失败");
			setbit(&siso,39,(unsigned char *)RET_CODE_FO,2);
			memcpy(ewp_info.consumer_responsecode,(unsigned char *)RET_CODE_FO,2);
			memcpy(ewp_info.consumer_presyswaterno,trace,6);
			errorTradeLog(ewp_info,folderName,"保存pep_jnls数据失败");
			WriteEWP(siso,ewp_info,pep_jnls,0);
			return;
		}
	}
	
	/*把解析到的数据保存以后需要的信息到数据表ewp_info中*/
	/*
	rtn = WriteDbEwpInfo(ewp_info);
	if(rtn == -1){
		dcs_log(0,0,"保存EWP_INFO失败");
		setbit(&siso,39,(unsigned char *)RET_CODE_FP,2);
		updateXpepErrorTradeInfo(ewp_info, trace, RET_CODE_FP);
		WriteEWP(siso,ewp_info,pep_jnls,0);
		return;
	}
	*/
	//新增优化功能不使用ewp_info数据库而使用memcached数据库
	dcs_log(0, 0, "save data to memcached begin");
	s = getstrlen(ewp_info.consumer_orderno);
	if(s <= 0)
	{
		memset(tmpbuf, 0, sizeof(tmpbuf));
		sprintf(tmpbuf, "%s%s%s%s", "DY", currentDate+2, currentTime, trace);
		memcpy(ewp_info.consumer_orderno, tmpbuf, 20);
	}
	OrganizateData(ewp_info, &key_value_info);
	//脱机消费 重新组 key,因为发起时间和交易时间有可能不同
	if(memcmp(ewp_info.consumer_transcode, "211", 3) ==0)
	{
		sprintf(key_value_info.keys, "%s%s%s",pep_jnls.trndate, pep_jnls.trntime, ewp_info.consumer_orderno);
		key_value_info.keys_length = strlen(key_value_info.keys);
	}
    
    rtn = Mem_SavaKey(key_value_info.keys, key_value_info.value, key_value_info.value_length, 600);
	if(rtn == -1)
	{
		dcs_log(0, 0, "save mem error");
		setbit(&siso,39,(unsigned char *)RET_CODE_F9,2);
		updateXpepErrorTradeInfo(ewp_info, trace, RET_CODE_F9);
		WriteEWP(siso,ewp_info,pep_jnls,0);
		return;
	}
	/*发送报文*/
	rtn=WriteXpe(siso);
	if(rtn == -1)
	{
		dcs_log(0,0,"发送信息给核心失败");
		setbit(&siso,39,(unsigned char *)RET_CODE_F9,2);
		updateXpepErrorTradeInfo(ewp_info, trace, RET_CODE_F9);
		WriteEWP(siso,ewp_info,pep_jnls,0);
		return;
	}
	return;	
}


/*解析从ewp那里接收到的数据 存放到结构体EWP_INFO中*/
int parsedata(char *rcvbuf, int rcvlen, EWP_INFO *ewp_info)
{
	int check_len, offset, temp_len, rtn;
	char tmpbuf[1024];

	check_len = 0;
	offset = 0;
	temp_len = 0;
	rtn = 0;
	
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	
	check_len = rcvlen;

	/*交易类型*/
	memcpy(ewp_info->consumer_transtype, rcvbuf + offset, 4);
	ewp_info->consumer_transtype[4] = 0;
	offset += 4;
	check_len -= 4;
	
	/*交易处理码*/
	memcpy(ewp_info->consumer_transdealcode, rcvbuf + offset, 6);
	ewp_info->consumer_transdealcode[6] = 0;
	offset += 6;
	check_len -= 6;
	
	#ifdef __LOG__
		dcs_log(0, 0, "consumer_transtype=[%s], consumer_transdealcode=[%s]", ewp_info->consumer_transtype, ewp_info->consumer_transdealcode);
	#endif
	
	/*发送日期*/
	memcpy(ewp_info->consumer_sentdate, rcvbuf + offset, 8);
	ewp_info->consumer_sentdate[8] = 0;
	offset += 8;
	check_len -= 8;
	
	/*发送时间*/
	memcpy(ewp_info->consumer_senttime, rcvbuf + offset, 6);
	ewp_info->consumer_senttime[6] = 0;
	offset += 6;
	check_len -= 6;
	
	#ifdef __LOG__
		dcs_log(0,0,"consumer_sentdate=[%s], consumer_senttime=[%s]", ewp_info->consumer_sentdate, ewp_info->consumer_senttime);
	#endif
	
	/*发送机构号*/
	memcpy(ewp_info->consumer_sentinstitu, rcvbuf + offset, 4);
	ewp_info->consumer_sentinstitu[4] = 0;
	offset += 4;
	check_len -= 4;
	
	/*交易码*/
	memcpy(ewp_info->consumer_transcode, rcvbuf + offset, 3);
	ewp_info->consumer_transcode[3] = 0;
	offset += 3;
	check_len -= 3;
	
	#ifdef __LOG__
		dcs_log(0,0,"consumer_sentinstitu=[%s], consumer_transcode=[%s]", ewp_info->consumer_sentinstitu, ewp_info->consumer_transcode);
	#endif
	
	//解析一体机上送消费撤销和退货210,电子现金的退货212
	if(memcmp(ewp_info->consumer_transcode, "209", 3) == 0 || memcmp(ewp_info->consumer_transcode, "210", 3) == 0 || memcmp(ewp_info->consumer_transcode, "212", 3) == 0 )
	{
		rtn = UnpackResj(rcvbuf+ offset, check_len, ewp_info);
		return rtn;
	}
	
	/*查询交易，IC卡公钥参数下载，下载结束，TC上送，脚本通知无需上送此交易发起方式域*/
	if(memcmp(ewp_info->consumer_transdealcode, "9F0000", 6)!=0 && memcmp(ewp_info->consumer_transtype, "0620", 4)!=0 
	&& memcmp(ewp_info->consumer_transdealcode, "999998", 6)!=0 && memcmp(ewp_info->consumer_transdealcode, "999997", 6)!=0 
	&& memcmp(ewp_info->consumer_transdealcode, "999996", 6)!=0)
	{
		/*交易发起方式 ,改为2字节*/
		memcpy(ewp_info->translaunchway, rcvbuf + offset, 2);
		ewp_info->translaunchway[2] = 0;
		offset += 2;
		check_len -= 2;
		
		#ifdef __LOG__
			dcs_log(0, 0, "translaunchway=[%s]", ewp_info->translaunchway);
		#endif
	}	
	/*psam编号*/
	memcpy(ewp_info->consumer_psamno, rcvbuf + offset, 16);
	ewp_info->consumer_psamno[16] = 0;
	offset += 16;
	check_len -= 16;
	
	/*机具编号手机编号 20字节*/
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memcpy(tmpbuf, rcvbuf + offset, 20);
	tmpbuf[20] = 0;
	pub_rtrim_lp(tmpbuf, 20, tmpbuf, 1);

	memcpy(ewp_info->consumer_phonesnno, tmpbuf, strlen(tmpbuf));
	ewp_info->consumer_phonesnno[strlen(tmpbuf)]=0;
	offset += 20;
	check_len -= 20;
	
	#ifdef __LOG__
		dcs_log(0,0,"consumer_psamno=[%s], consumer_phonesnno=[%s]", ewp_info->consumer_psamno, ewp_info->consumer_phonesnno);
	#endif
	
	/*IC卡公钥参数下载，下载结束，TC上送，脚本通知无需上送*/
	if(memcmp(ewp_info->consumer_transtype, "0620", 4)!=0 && memcmp(ewp_info->consumer_transdealcode, "999998", 6)!=0 
	&& memcmp(ewp_info->consumer_transdealcode, "999997", 6)!=0 && memcmp(ewp_info->consumer_transdealcode, "999996", 6)!=0)
	{
		/*用户名*/
		memcpy(ewp_info->consumer_username, rcvbuf + offset, 11);
		ewp_info->consumer_username[11] = 0;
		offset += 11;
		check_len -= 11;
		
		/*订单号*/
		memset(tmpbuf,0,sizeof(tmpbuf));
		memcpy(tmpbuf, rcvbuf + offset, 20);
		tmpbuf[20] = 0;
		//pub_rtrim(tmpbuf, 20, tmpbuf);
		pub_rtrim_lp(tmpbuf, 20, tmpbuf,1);
		memcpy(ewp_info->consumer_orderno, tmpbuf, strlen(tmpbuf));
		ewp_info->consumer_orderno[strlen(tmpbuf)]=0;
		offset += 20;
		check_len -= 20;
		
		#ifdef __LOG__
			dcs_log(0, 0, "consumer_username=[%s], consumer_orderno=[%s]", ewp_info->consumer_username, ewp_info->consumer_orderno);
		#endif
		
		/*商户编号*/
		memset(tmpbuf,0,sizeof(tmpbuf));
		memcpy(tmpbuf, rcvbuf + offset, 20);
		tmpbuf[20] = 0;
		pub_rtrim_lp(tmpbuf, 20, tmpbuf, 1);
		memcpy(ewp_info->consumer_merid, tmpbuf, strlen(tmpbuf));
		ewp_info->consumer_merid[strlen(tmpbuf)]=0;
		offset += 20;
		check_len -= 20;
		
		#ifdef __LOG__
			dcs_log(0, 0, "consumer_merid=[%s]", ewp_info->consumer_merid);
		#endif
		/*卡号 modify by hw IC参数公钥参数下载，脚本通知 无卡号上送*/
		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy(tmpbuf, rcvbuf + offset, 19);
		tmpbuf[19] = 0;
		pub_rtrim_lp(tmpbuf, 19, tmpbuf, 1);
		memcpy(ewp_info->consumer_cardno, tmpbuf, strlen(tmpbuf));
		ewp_info->consumer_cardno[strlen(tmpbuf)]=0;
		offset += 19;
		check_len -= 19;
#ifdef __TEST__
		dcs_log(0, 0, "consumer_cardno=[%s]", ewp_info->consumer_cardno);
#endif
	}

	#ifdef __LOG__
		dcs_log(0, 0, "解析EWP数据, check_len=[%d]", check_len);
		dcs_log(0, 0, "解析EWP数据, offset=[%d]", offset);
	#endif
	if(memcmp(ewp_info->consumer_transtype, "0620", 4)!=0 
	&&(memcmp(ewp_info->consumer_transdealcode, "400000", 6)==0
	||memcmp(ewp_info->consumer_transdealcode, "480000", 6)== 0
	||memcmp(ewp_info->consumer_transdealcode, "190000", 6)==0))
	{
		/*转入卡号*/
		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy(tmpbuf, rcvbuf + offset, 19);
		tmpbuf[19] = 0;
		pub_rtrim_lp(tmpbuf, 19, tmpbuf, 1);
		memcpy(ewp_info->incardno, tmpbuf, strlen(tmpbuf));
		ewp_info->incardno[strlen(tmpbuf)]=0;
		offset += 19;
		check_len -= 19;
		#ifdef __TEST__
			dcs_log(0, 0, "ewp_info->incardno=[%s]", ewp_info->incardno);
		#endif
		if(memcmp(ewp_info->consumer_transdealcode, "480000", 6)==0)
		{
			dcs_log(0, 0, "这是一个转账汇款交易...");
			/*转入卡姓名*/
			char name[50+1];
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memset(name, 0, sizeof(name));
			memcpy(tmpbuf, rcvbuf + offset, 50);
			tmpbuf[50] = 0;
			pub_rtrim_lp(tmpbuf, 50, tmpbuf, 1);
			//ewp上送的姓名为GBK，需要转换为UTF8格式
			g2u(tmpbuf, strlen(tmpbuf), name, sizeof(name));
			memcpy(ewp_info->incdname, name, strlen(name));
			ewp_info->incdname[strlen(name)]=0;
			offset += 50;
			check_len -= 50;
			/*转入卡行号*/
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy(tmpbuf, rcvbuf + offset, 25);
			tmpbuf[25] = 0;
			pub_rtrim_lp(tmpbuf, 25, tmpbuf, 1);
			memcpy(ewp_info->incdbkno, tmpbuf, strlen(tmpbuf));
			ewp_info->incdbkno[strlen(tmpbuf)]=0;
			offset += 25;
			check_len -= 25;
			#ifdef __TEST__
				dcs_log(0, 0, "ewp_info->incdname=[%s], ewp_info->incdbkno=[%s]", ewp_info->incdname, ewp_info->incdbkno);
			#endif
		}
		else if(memcmp(ewp_info->consumer_transdealcode, "190000", 6)==0)
			dcs_log(0, 0, "这是一个信用卡还款类的交易...");
		else if(memcmp(ewp_info->consumer_transdealcode, "400000", 6)==0)
			dcs_log(0, 0, "这是一个ATM转账类的交易...");
		/*金额 12个字节 */
		/*
		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy(tmpbuf, rcvbuf + offset, 12);
		tmpbuf[12] = 0;	
		pub_rtrim_lp(tmpbuf,12, tmpbuf, 0);
		memcpy(ewp_info->consumer_money, tmpbuf, strlen(tmpbuf));
		ewp_info->consumer_money[strlen(tmpbuf)]=0;
		*/
		memcpy(ewp_info->consumer_money, rcvbuf + offset, 12);
		ewp_info->consumer_money[12]=0;
		offset += 12;
		check_len -= 12;
		
		/*交易手续费 12个字节ANS12 */
		memcpy(ewp_info->transfee, rcvbuf + offset, 12);
		ewp_info->transfee[12]=0;
		offset += 12;
		check_len -= 12;
		
		#ifdef __LOG__
			dcs_log(0, 0, "consumer_money=[%s], transfee =[%s]", ewp_info->consumer_money, ewp_info->transfee);
		#endif
		
		/*磁道160个字节 */
		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy(tmpbuf, rcvbuf + offset, 160);
		tmpbuf[160] = 0;
		pub_rtrim_lp(tmpbuf, 160, tmpbuf, 1);
		memcpy(ewp_info->consumer_trackno, tmpbuf, strlen(tmpbuf));
		ewp_info->consumer_trackno[strlen(tmpbuf)]=0;
		offset += 160;
		check_len -= 160;
		
		#ifdef __TEST__
			dcs_log(0, 0, "consumer_trackno=[%s]", ewp_info->consumer_trackno);
		#endif
		
		/*16个字节的磁道密钥随机数*/
		memcpy(ewp_info->consumer_track_randnum, rcvbuf + offset, 16);
		ewp_info->consumer_track_randnum[16] = 0;
		offset += 16;
		check_len -= 16;
		
		/*密码*/
		memcpy(ewp_info->consumer_password, rcvbuf + offset, 16);
		ewp_info->consumer_password[16] = 0;
		offset += 16;
		check_len -= 16;
		
		#ifdef __TEST__
			dcs_log(0, 0, "consumer_track_randnum=[%s], consumer_password=[%s]", ewp_info->consumer_track_randnum, ewp_info->consumer_password);
		#endif
		
		/*16个字节的pin密钥随机数*/
		memcpy(ewp_info->consumer_pin_randnum, rcvbuf + offset, 16);
		ewp_info->consumer_pin_randnum[16] = 0;
		offset += 16;
		check_len -= 16;
		
		#ifdef __LOG__
			dcs_log(0, 0, "consumer_pin_randnum=[%s]", ewp_info->consumer_pin_randnum);
		#endif
		/*解析sk项目的mac信息 ，解析盛京银行项目的mac信息*/
		if(memcmp(ewp_info->consumer_sentinstitu, "0400", 4) == 0 
		|| memcmp(ewp_info->consumer_sentinstitu, "0800", 4) == 0)
		{
			memcpy(ewp_info->macinfo, rcvbuf + offset, 8);
			ewp_info->macinfo[8] = 0;
			offset += 8;
			check_len -= 8;
		
			#ifdef __LOG__
				dcs_log(0, 0, "macinfo=[%s]", ewp_info->macinfo);
			#endif
		}
		/*1个字节的是否需要发送短信的标记*/
		memcpy(ewp_info->modeflg, rcvbuf + offset, 1);
		ewp_info->modeflg[1] = 0;
		offset += 1;
		check_len -= 1;
		
		if(memcmp(ewp_info->modeflg, "1", 1)==0)
		{
			/*11个字节的手机号*/
			memcpy(ewp_info->modecde, rcvbuf + offset, 11);
			ewp_info->modecde[11] = 0;
			offset += 11;
			check_len -= 11;
			#ifdef __LOG__
				dcs_log(0, 0, "modeflg=[%s], modecde=[%s]", ewp_info->modeflg, ewp_info->modecde);
			#endif
		}
		else
		{
			#ifdef __LOG__
				dcs_log(0, 0, "modeflg=[%s]", ewp_info->modeflg);
			#endif
		}
		
		/*判断报文在此处是否结束，结束表示是旧交易，没结束表示新接口交易*/
		if( (check_len-26)>=0 )
		{
			/* 终端类型  */
			memcpy(ewp_info->termtyp, rcvbuf + offset, 1);
			dcs_log(0, 0, "终端类型[%s]", ewp_info->termtyp);
			offset += 1;
			check_len -= 1;
			
			/* IC卡标识 */
			memcpy(ewp_info->icflag, rcvbuf + offset, 1);
#ifdef __LOG__
			dcs_log(0, 0, "IC卡标识[%s]", ewp_info->icflag);
#endif
			offset += 1;
			check_len -= 1;
			
			if(memcmp(ewp_info->icflag, "1", 1) == 0)
			{
				/*降级标志位fallback*/
				memcpy(ewp_info->fallback_flag, rcvbuf + offset, 1);
#ifdef __LOG__
				dcs_log(0, 0, "降级标志[%s]", ewp_info->fallback_flag);
#endif
				offset += 1;                                    
				check_len -= 1;
			}

			/* 卡片序列号存在标识  */
			memcpy(ewp_info->cardseq_flag, rcvbuf + offset, 1);
#ifdef __LOG__
			dcs_log(0, 0, "卡片序列号存在标识[%s]", ewp_info->cardseq_flag);
#endif
			offset += 1;
			check_len -= 1;
			
			if( memcmp(ewp_info->cardseq_flag, "1", 1)==0 )
			{
				/* 卡片序列号 */
				memcpy(ewp_info->cardseq, rcvbuf + offset, 3);
#ifdef __LOG__
		dcs_log(0, 0, "卡片序列号[%s]", ewp_info->cardseq);
#endif

				offset += 3;                                    
				check_len -= 3;			
			}
			
			/*IC卡数据域长度*/
			memcpy(ewp_info->filed55_length, rcvbuf + offset, 3);
#ifdef __LOG__
			dcs_log(0, 0, "IC卡数据域[%s]", ewp_info->filed55_length);
#endif
			offset += 3;
			check_len -= 3;
			
			if( memcmp(ewp_info->filed55_length, "000", 3)!=0 )
			{
				/*IC卡数据域*/
				temp_len = atoi(ewp_info->filed55_length);
				memcpy(ewp_info->filed55, rcvbuf + offset, temp_len);
#ifdef __LOG__
				dcs_log(0, 0, "IC卡数据域[%s]", ewp_info->filed55);
#endif
				offset += temp_len;
				check_len -= temp_len;
			}
		}
		
		/*20个字节的自定义域*/
		memcpy(ewp_info->selfdefine, rcvbuf + offset, 20);
		ewp_info->selfdefine[20] = 0;
		offset += 20;
		check_len -= 20;
		
		#ifdef __LOG__
			dcs_log(0, 0, "selfdefine=[%s]", ewp_info->selfdefine);
		#endif
	}
	/*区别消费和消费冲正，本段为消费交易*/
	else if(memcmp(ewp_info->consumer_transtype, "0620", 4)!=0 
	&& memcmp(ewp_info->consumer_transdealcode, "000000", 6)==0 
	&& memcmp(ewp_info->consumer_transtype, "0400", 4)!=0)
	{
		dcs_log(0, 0, "这是一个消费类的交易...");
		/*金额 12个字节 */
		memcpy(ewp_info->consumer_money, rcvbuf + offset, 12);
		ewp_info->consumer_money[12]=0;
		offset += 12;
		check_len -= 12;
		
		/*优惠金额 12个字节*/
		memcpy(ewp_info->discountamt, rcvbuf + offset, 12);
		ewp_info->discountamt[12]=0;
		offset += 12;
		check_len -= 12;
		
		#ifdef __LOG__
			dcs_log(0, 0, "consumer_money=[%s], discountamt=[%s]", ewp_info->consumer_money, ewp_info->discountamt);
		#endif
		//一体机上送电子现金脱机消费(交易码211)没有磁道和密码域
		if(memcmp(ewp_info->consumer_transcode, "211", 3) != 0)
		{
			/*磁道160个字节 */
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy(tmpbuf, rcvbuf + offset, 160);
			tmpbuf[160] = 0;
			pub_rtrim_lp(tmpbuf, 160, tmpbuf, 1);
			memcpy(ewp_info->consumer_trackno, tmpbuf, strlen(tmpbuf));
			ewp_info->consumer_trackno[strlen(tmpbuf)]=0;
			offset += 160;
			check_len -= 160;

			#ifdef __TEST__
				dcs_log(0, 0, "consumer_trackno=[%s]", ewp_info->consumer_trackno);
			#endif

			/*16个字节的磁道密钥随机数*/
			memcpy(ewp_info->consumer_track_randnum, rcvbuf + offset, 16);
			ewp_info->consumer_track_randnum[16] = 0;
			offset += 16;
			check_len -= 16;

			/*密码*/
			memcpy(ewp_info->consumer_password, rcvbuf + offset, 16);
			ewp_info->consumer_password[16] = 0;
			offset += 16;
			check_len -= 16;

			#ifdef __TEST__
				dcs_log(0, 0, "consumer_track_randnum=[%s], consumer_password=[%s]", ewp_info->consumer_track_randnum, ewp_info->consumer_password);
			#endif

			/*16个字节的pin密钥随机数*/
			memcpy(ewp_info->consumer_pin_randnum, rcvbuf + offset, 16);
			ewp_info->consumer_pin_randnum[16] = 0;
			offset += 16;
			check_len -= 16;

			#ifdef __LOG__
				dcs_log(0, 0, "consumer_pin_randnum=[%s]", ewp_info->consumer_pin_randnum);
			#endif

		}
		else
			dcs_log(0, 0, "电子现金脱机消费通知上送");
		
		/*解析sk或车载项目的mac信息或盛京银行上送的mac信息*/
		if( memcmp(ewp_info->consumer_sentinstitu, "0400", 4) == 0 
		|| memcmp(ewp_info->consumer_sentinstitu, "0500", 4) == 0
		|| memcmp(ewp_info->consumer_sentinstitu, "0800", 4) == 0 )
		{
			memcpy(ewp_info->macinfo, rcvbuf + offset, 8);
			ewp_info->macinfo[8] = 0;
			offset += 8;
			check_len -= 8;
		
			#ifdef __LOG__
				dcs_log(0, 0, "macinfo=[%s]", ewp_info->macinfo);
			#endif
		}
		
		/*1个字节的是否需要发送短信的标记*/
		memcpy(ewp_info->modeflg, rcvbuf + offset, 1);
		ewp_info->modeflg[1] = 0;
		offset += 1;
		check_len -= 1;
		
		if(memcmp(ewp_info->modeflg, "1", 1)==0)
		{
			/*11个字节的手机号*/
			memcpy(ewp_info->modecde, rcvbuf + offset, 11);
			ewp_info->modecde[11] = 0;
			offset += 11;
			check_len -= 11;
			#ifdef __LOG__
				dcs_log(0, 0, "modeflg=[%s], modecde=[%s]", ewp_info->modeflg, ewp_info->modecde);
			#endif
		}
		else
		{
			#ifdef __LOG__
				dcs_log(0, 0, "modeflg=[%s]", ewp_info->modeflg);
			#endif
		}
		
		/*判断报文在此处是否结束，结束表示是旧交易，没结束表示新接口交易*/
		if( (check_len-26)>=0 )
		{
			/* 终端类型  */
			memcpy(ewp_info->termtyp, rcvbuf + offset, 1);
			dcs_log(0, 0, "终端类型[%s]", ewp_info->termtyp);
			offset += 1;
			check_len -= 1;
			
			/* IC卡标识 */
			memcpy(ewp_info->icflag, rcvbuf + offset, 1);
#ifdef __LOG__
			dcs_log(0, 0, "IC卡标识[%s]", ewp_info->icflag);
#endif
			offset += 1;
			check_len -= 1;
			
			if(memcmp(ewp_info->icflag, "1", 1) == 0)
			{
				/*降级标志位fallback*/
				memcpy(ewp_info->fallback_flag, rcvbuf + offset, 1);
#ifdef __LOG__
				dcs_log(0, 0, "降级标志[%s]", ewp_info->fallback_flag);
#endif
				offset += 1;                                    
				check_len -= 1;
			}

			/* 卡片序列号存在标识  */
			memcpy(ewp_info->cardseq_flag, rcvbuf + offset, 1);
#ifdef __LOG__
			dcs_log(0, 0, "卡片序列号存在标识[%s]", ewp_info->cardseq_flag);
#endif
			offset += 1;
			check_len -= 1;
			
			if( memcmp(ewp_info->cardseq_flag, "1", 1)==0 )
			{
				/* 卡片序列号 */
				memcpy(ewp_info->cardseq, rcvbuf + offset, 3);
#ifdef __LOG__
				dcs_log(0, 0, "卡片序列号[%s]", ewp_info->cardseq);
#endif
				offset += 3;                                    
				check_len -= 3;			
			}

			/*IC卡数据域长度*/
			memcpy(ewp_info->filed55_length, rcvbuf + offset, 3);
#ifdef __LOG__
			dcs_log(0, 0, "IC卡数据域[%s]", ewp_info->filed55_length);
#endif
			offset += 3;
			check_len -= 3;
			
			if( memcmp(ewp_info->filed55_length, "000", 3)!=0 )
			{
				/*IC卡数据域*/
				temp_len = atoi(ewp_info->filed55_length);
				memset(ewp_info->filed55, 0x00, sizeof(ewp_info->filed55));
				memcpy(ewp_info->filed55, rcvbuf + offset, temp_len);
#ifdef __LOG__
				dcs_log(0, 0, "IC卡数据域[%s]", ewp_info->filed55);
#endif
				offset += temp_len;
				check_len -= temp_len;
			}
		}
		
		/*20个字节的自定义域*/
		memcpy(ewp_info->selfdefine, rcvbuf + offset, 20);
		ewp_info->selfdefine[20] = 0;
		offset += 20;
		check_len -= 20;
		
		#ifdef __LOG__
			dcs_log(0, 0, "selfdefine=[%s]", ewp_info->selfdefine);
		#endif
	}
	else if(memcmp(ewp_info->consumer_transtype, "0620", 4)!=0 && memcmp(ewp_info->consumer_transdealcode, "310000", 6)==0)
	{
		#ifdef __LOG__
			dcs_log(0, 0, "这是一个余额查询类的交易...");
		#endif
		
		/*磁道160个字节 */
		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy(tmpbuf, rcvbuf + offset, 160);
		tmpbuf[160] = 0;
		pub_rtrim_lp(tmpbuf, 160, tmpbuf, 1);
		memcpy(ewp_info->consumer_trackno, tmpbuf, strlen(tmpbuf));
		ewp_info->consumer_trackno[strlen(tmpbuf)]=0;
		offset += 160;
		check_len -= 160;
		
		#ifdef __TEST__
			dcs_log(0, 0, "consumer_trackno=[%s]", ewp_info->consumer_trackno);
		#endif
		
		/*16个字节的磁道密钥随机数*/
		memcpy(ewp_info->consumer_track_randnum, rcvbuf + offset, 16);
		ewp_info->consumer_track_randnum[16] = 0;
		offset += 16;
		check_len -= 16;
		
		/*密码*/
		memcpy(ewp_info->consumer_password, rcvbuf + offset, 16);
		ewp_info->consumer_password[16] = 0;
		offset += 16;
		check_len -= 16;
		
		#ifdef __TEST__
			dcs_log(0, 0, "consumer_track_randnum=[%s], consumer_password=[%s]", ewp_info->consumer_track_randnum, ewp_info->consumer_password);
		#endif
		
		/*16个字节的pin密钥随机数*/
		memcpy(ewp_info->consumer_pin_randnum, rcvbuf + offset, 16);
		ewp_info->consumer_pin_randnum[16] = 0;
		offset += 16;
		check_len -= 16;
		
		#ifdef __LOG__
			dcs_log(0, 0, "consumer_pin_randnum=[%s]", ewp_info->consumer_pin_randnum);
		#endif
		
		/*解析sk项目的mac信息,解析盛京银行项目的mac信息*/
		if(memcmp(ewp_info->consumer_sentinstitu, "0400", 4) == 0 
		||memcmp(ewp_info->consumer_sentinstitu, "0800", 4) == 0)
		{
			memcpy(ewp_info->macinfo, rcvbuf + offset, 8);
			ewp_info->macinfo[8] = 0;
			offset += 8;
			check_len -= 8;
		
			#ifdef __LOG__
				dcs_log(0, 0, "macinfo=[%s]", ewp_info->macinfo);
				dcs_log(0, 0, "SK 项目");
			#endif
		}
		
		/*modify by hw 20140826*/
		if( (check_len-26)>=0 )
		{
			/* 终端类型  */
			memcpy(ewp_info->termtyp, rcvbuf + offset, 1);
			dcs_log(0, 0, "终端类型[%s]", ewp_info->termtyp);
			offset += 1;
			check_len -= 1;
			
			/* IC卡标识 */
			memcpy(ewp_info->icflag, rcvbuf + offset, 1);
#ifdef __LOG__
			dcs_log(0, 0, "IC卡标识[%s]", ewp_info->icflag);
#endif
			offset += 1;
			check_len -= 1;
			
			if(memcmp(ewp_info->icflag, "1", 1) == 0)
			{
				/*降级标志位fallback*/
				memcpy(ewp_info->fallback_flag, rcvbuf + offset, 1);
#ifdef __LOG__
				dcs_log(0, 0, "降级标志[%s]", ewp_info->fallback_flag);
#endif
				offset += 1;                                    
				check_len -= 1;
			}

			/* 卡片序列号存在标识  */
			memcpy(ewp_info->cardseq_flag, rcvbuf + offset, 1);
#ifdef __LOG__
			dcs_log(0, 0, "卡片序列号存在标识[%s]", ewp_info->cardseq_flag);
#endif
			offset += 1;
			check_len -= 1;
			
			if( memcmp(ewp_info->cardseq_flag, "1", 1)==0 )
			{
				/* 卡片序列号 */
				memcpy(ewp_info->cardseq, rcvbuf + offset, 3);
#ifdef __LOG__
				dcs_log(0, 0, "卡片序列号[%s]", ewp_info->cardseq);
#endif
				offset += 3;                                    
				check_len -= 3;			
			}
			
			/*IC卡数据域长度*/
			memcpy(ewp_info->filed55_length, rcvbuf + offset, 3);
			temp_len =  atoi(ewp_info->filed55_length);
#ifdef __LOG__
			dcs_log(0, 0, "IC卡数据域[%s][%d]", ewp_info->filed55_length, temp_len);
#endif
			offset += 3;
			check_len -= 3;
			
			if( memcmp(ewp_info->filed55_length, "000", 3)!=0 )
			{
				/*IC卡数据域*/
				memset(ewp_info->filed55, 0x00, sizeof(ewp_info->filed55));
				memcpy(ewp_info->filed55, rcvbuf + offset, temp_len);
#ifdef __LOG__
				dcs_log(0, 0, "IC卡数据域[%s]", ewp_info->filed55);
#endif
				offset += temp_len;
				check_len -= temp_len;
			}
		}
		
		/*20个字节的自定义域*/
		memcpy(ewp_info->selfdefine, rcvbuf + offset, 20);
		ewp_info->selfdefine[20] = 0;
		offset += 20;
		check_len -= 20;
		
		#ifdef __LOG__
			dcs_log(0, 0, "selfdefine=[%s]", ewp_info->selfdefine);
		#endif
	}
	else if(memcmp(ewp_info->consumer_transdealcode, "9F0000", 6)==0)
	{
		dcs_log(0, 0, "这是一个查询交易...");
		/*20个字节的自定义域*/
		memcpy(ewp_info->selfdefine, rcvbuf + offset, 20);
		ewp_info->selfdefine[20] = 0;
		offset += 20;
		check_len -= 20;
		
		#ifdef __LOG__
			dcs_log(0, 0, "selfdefine=[%s]", ewp_info->selfdefine);
		#endif
	}
	else if(memcmp(ewp_info->consumer_transtype, "0620", 4)!=0 && memcmp(ewp_info->consumer_transdealcode, "200000", 6)==0)
	{
		#ifdef __LOG__
			dcs_log(0, 0, "这是一个消费撤销类的交易...");
		#endif
		
		/*系统参考号*/
		memcpy(ewp_info->consumer_sysreferno, rcvbuf + offset, 12);
		ewp_info->consumer_sysreferno[12] = 0;
		offset += 12;
		check_len -= 12;
	
		/*授权码*/
		memcpy(ewp_info->authcode, rcvbuf + offset, 6);
		ewp_info->authcode[6] = 0;
		offset += 6;
		check_len -= 6;
		
		#ifdef __LOG__
			dcs_log(0, 0, "consumer_sysreferno=[%s], authcode=[%s]", ewp_info->consumer_sysreferno, ewp_info->authcode);
		#endif
		
		/*前置系统流水*/
		memcpy(ewp_info->consumer_presyswaterno, rcvbuf + offset, 6);
		ewp_info->consumer_presyswaterno[6] = 0;
		offset += 6;
		check_len -= 6;
	
		/*原笔交易发起日期和时间*/
		memcpy(ewp_info->pretranstime, rcvbuf + offset, 14);
		ewp_info->pretranstime[14] = 0;
		offset += 14;
		check_len -= 14;
		
		/*modify by hw 20140826*/
		if( (check_len-8)>0 )
		{
			/*服务点输入方式码*/
			memcpy(ewp_info->consumer_sysreferno, rcvbuf + offset, 3);
			dcs_log(0, 0, "服务点输入方式码[%s]", ewp_info->consumer_sysreferno);
			offset += 3;                                    
			check_len -= 3;
				
			/* 终端类型  */
			memcpy(ewp_info->termtyp, rcvbuf + offset, 1);
			dcs_log(0, 0, "终端类型[%s]", ewp_info->termtyp);
			offset += 1;
			check_len -= 1;
				
			/* IC卡标识 */
			memcpy(ewp_info->icflag, rcvbuf + offset, 1);
#ifdef __LOG__
			dcs_log(0, 0, "IC卡标识[%s]", ewp_info->icflag);
#endif
			offset += 1;
			check_len -= 1;
			
			/* 卡片序列号存在标识  */
			memcpy(ewp_info->cardseq_flag, rcvbuf + offset, 1);
#ifdef __LOG__
			dcs_log(0, 0, "卡片序列号存在标识[%s]", ewp_info->cardseq_flag);
#endif
			offset += 1;
			check_len -= 1;
			
			if( memcmp(ewp_info->cardseq_flag, "1", 1)==0 )
			{
				/* 卡片序列号 */
				memcpy(ewp_info->cardseq, rcvbuf + offset, 3);
#ifdef __LOG__
				dcs_log(0, 0, "卡片序列号[%s]", ewp_info->cardseq);
#endif
				offset += 3;                                    
				check_len -= 3;
			}
		}
		/*mac信息*/
		memcpy(ewp_info->macinfo, rcvbuf + offset, 8);
		ewp_info->macinfo[8] = 0;
		offset += 8;
		check_len -= 8;
		
		#ifdef __LOG__
			dcs_log(0, 0, "consumer_presyswaterno=[%s], pretranstime=[%s]", ewp_info->consumer_presyswaterno, ewp_info->pretranstime);
			dcs_log(0, 0, "macinfo=[%s]", ewp_info->macinfo);	
		#endif
	}
	
	else if(memcmp(ewp_info->consumer_transtype, "0620", 4)!=0 && memcmp(ewp_info->consumer_transdealcode, "270000", 6)==0)
	{
		dcs_log(0, 0, "这是一个消费退货的交易...");
		/*退货金额*/
		memcpy(ewp_info->consumer_money, rcvbuf + offset, 12);
		ewp_info->consumer_money[12] = 0;
		offset += 12;
		check_len -= 12;
	
		/*系统参考号*/
		memcpy(ewp_info->consumer_sysreferno, rcvbuf + offset, 12);
		ewp_info->consumer_sysreferno[12] = 0;
		offset += 12;
		check_len -= 12;
		
		#ifdef __LOG__
			dcs_log(0, 0, "退货金额consumer_money=[%s], consumer_sysreferno=[%s]", ewp_info->consumer_money, ewp_info->consumer_sysreferno);
		#endif
		
		/*授权码*/
		memcpy(ewp_info->authcode, rcvbuf + offset, 6);
		ewp_info->authcode[6] = 0;
		offset += 6;
		check_len -= 6;
		
		#ifdef __LOG__
			dcs_log(0, 0, "authcode=[%s]", ewp_info->authcode);
		#endif
		
		/*前置系统流水*/
		memcpy(ewp_info->consumer_presyswaterno, rcvbuf + offset, 6);
		ewp_info->consumer_presyswaterno[6] = 0;
		offset += 6;
		check_len -= 6;
	
		/*原笔交易发起日期和时间*/
		memcpy(ewp_info->pretranstime, rcvbuf + offset, 14);
		ewp_info->pretranstime[14] = 0;
		offset += 14;
		check_len -= 14;
		
		#ifdef __LOG__
			dcs_log(0, 0, "consumer_presyswaterno=[%s], pretranstime=[%s]", ewp_info->consumer_presyswaterno, ewp_info->pretranstime);	
		#endif
	}
	
	/*modify by hw 20140825 IC卡公钥参数下载*/
	else if(memcmp(ewp_info->consumer_transdealcode, "999998", 6)==0)
	{
		dcs_log(0, 0, "IC卡公钥参数下载");
		
		/*网络管理信息码 1：公钥下载， 2：参数下载*/
		memset(ewp_info->netmanagecode, 0, sizeof(ewp_info->netmanagecode));
		memcpy(ewp_info->netmanagecode, rcvbuf + offset, 1);
		dcs_log(0, 0, "下载标识	1:公钥下载；0：参数下载[%s]", ewp_info->netmanagecode);
		offset += 1;
		check_len -= 1;
		
		/*当前公钥/参数序号*/
		memset(ewp_info->paraseq, 0, sizeof(ewp_info->paraseq));
		memcpy(ewp_info->paraseq, rcvbuf + offset, 2);
		dcs_log(0, 0, "当前公钥/参数序号[%s]", ewp_info->paraseq);
		offset += 2;                                    
		check_len -= 2;
		
		/*20个字节的自定义域*/
		memcpy(ewp_info->selfdefine, rcvbuf + offset, 20);
		ewp_info->selfdefine[20] = 0;
		offset += 20;
		check_len -= 20;
		
		#ifdef __LOG__
			dcs_log(0, 0, "selfdefine=[%s]", ewp_info->selfdefine);
		#endif
	}
	
	/*modify by hw 20140825 TC值上送*/
	else if(memcmp(ewp_info->consumer_transdealcode, "999997", 6)==0)
	{
		dcs_log(0, 0, "TC值上送");
		
		/*用户名*/
		memcpy(ewp_info->consumer_username, rcvbuf + offset, 11);
		dcs_log(0, 0, "用户名[%s]", ewp_info->consumer_username);
		offset += 11;
		check_len -= 11;
		
		/*系统参考号*/
		memcpy(ewp_info->consumer_sysreferno, rcvbuf + offset, 12);
		dcs_log(0, 0, "系统参考号[%s]", ewp_info->consumer_sysreferno);
		offset += 12;
		check_len -= 12;

		/*IC卡数据域长度*/
		memcpy(ewp_info->filed55_length, rcvbuf + offset, 3);
#ifdef __LOG__
		dcs_log(0, 0, "IC卡数据域[%s]", ewp_info->filed55_length);
#endif
		offset += 3;
		check_len -= 3;
		
		if( memcmp(ewp_info->filed55_length, "000", 3)!=0 )
		{
			/*IC卡数据域*/
			temp_len = atoi(ewp_info->filed55_length);
			memcpy(ewp_info->filed55, rcvbuf + offset, temp_len);
#ifdef __LOG__
			dcs_log(0, 0, "IC卡数据域[%s]", ewp_info->filed55);
#endif
			offset += temp_len;
			check_len -= temp_len;
		}
		
		/*原交易日期*/
		memset(tmpbuf,0x00,sizeof(tmpbuf));
		memcpy(tmpbuf, rcvbuf + offset, 14);
		memcpy(ewp_info->originaldate, tmpbuf, 8);
		dcs_log(0, 0, "原交易日期[%s]", ewp_info->originaldate);
		offset += 14;
		check_len -= 14;
		
		/*随机数*/
		memcpy(ewp_info->consumer_track_randnum, rcvbuf + offset, 16);
		ewp_info->consumer_track_randnum[16] = 0;
#ifdef __LOG__
		dcs_log(0, 0, "随机数[%s]", ewp_info->consumer_track_randnum);
#endif

		offset += 16;
		check_len -= 16;
		
		/*解析sk或车载项目的mac信息或盛京银行上送的mac信息*/
		if( memcmp(ewp_info->consumer_sentinstitu, "0400", 4) == 0 
		|| memcmp(ewp_info->consumer_sentinstitu, "0500", 4) == 0
		|| memcmp(ewp_info->consumer_sentinstitu, "0800", 4) == 0 )
		{
			memcpy(ewp_info->macinfo, rcvbuf + offset, 8);
			ewp_info->macinfo[8] = 0;
			offset += 8;
			check_len -= 8;
		
			#ifdef __LOG__
				dcs_log(0, 0, "macinfo=[%s]", ewp_info->macinfo);
			#endif
		}
		
		/*20个字节的自定义域
		memcpy(ewp_info->selfdefine, rcvbuf + offset, 20);
		ewp_info->selfdefine[20] = 0;
		offset += 20;
		check_len -= 20;
		*/
		#ifdef __LOG__
			dcs_log(0, 0, "selfdefine=[%s]", ewp_info->selfdefine);
		#endif
	}
	
	/*modify by hw 20140825 IC卡脚本通知*/
	else if(memcmp(ewp_info->consumer_transtype, "0620", 4)==0)
	{
		dcs_log(0, 0, "IC卡脚本通知");
		
		/*用户名*/
		memcpy(ewp_info->consumer_username, rcvbuf + offset, 11);
		dcs_log(0, 0, "用户名[%s]", ewp_info->consumer_username);
		offset += 11;
		check_len -= 11;
		
		/*系统参考号*/
		memcpy(ewp_info->consumer_sysreferno, rcvbuf + offset, 12);
		dcs_log(0, 0, "系统参考号[%s]", ewp_info->consumer_sysreferno);
		offset += 12;
		check_len -= 12;
		
		/*IC卡数据域长度*/
		memcpy(ewp_info->filed55_length, rcvbuf + offset, 3);
#ifdef __LOG__
		dcs_log(0, 0, "IC卡数据域长度[%s]", ewp_info->filed55_length);
#endif

		offset += 3;
		check_len -= 3;
		
		if( memcmp(ewp_info->filed55_length, "000", 3)!=0 )
		{
			/*IC卡数据域*/
			temp_len = atoi(ewp_info->filed55_length);
			memcpy(ewp_info->filed55, rcvbuf + offset, temp_len);
#ifdef __LOG__
			dcs_log(0, 0, "IC卡数据域[%s]", ewp_info->filed55);
#endif
			offset += temp_len;
			check_len -= temp_len;
		}
		
		/*原交易日期*/
		memset(tmpbuf,0x00,sizeof(tmpbuf));
		memcpy(tmpbuf, rcvbuf + offset, 14);
		memcpy(ewp_info->originaldate, tmpbuf, 8);
		dcs_log(0, 0, "原交易日期[%s]", ewp_info->originaldate);
		offset += 14;
		check_len -= 14;
		
		/*随机数*/
		memcpy(ewp_info->consumer_track_randnum, rcvbuf + offset, 16);
		ewp_info->consumer_track_randnum[16] = 0;
#ifdef __LOG__
		dcs_log(0, 0, "随机数[%s]", ewp_info->consumer_track_randnum);
#endif
		offset += 16;
		check_len -= 16;
		
		/*解析sk或车载项目的mac信息或盛京银行上送的mac信息*/
		if( memcmp(ewp_info->consumer_sentinstitu, "0400", 4) == 0 
		|| memcmp(ewp_info->consumer_sentinstitu, "0500", 4) == 0
		|| memcmp(ewp_info->consumer_sentinstitu, "0800", 4) == 0 )
		{
			memcpy(ewp_info->macinfo, rcvbuf + offset, 8);
			ewp_info->macinfo[8] = 0;
			offset += 8;
			check_len -= 8;
		
			#ifdef __LOG__
				dcs_log(0, 0, "macinfo=[%s]", ewp_info->macinfo);
			#endif
		}
		
		/*20个字节的自定义域
		memcpy(ewp_info->selfdefine, rcvbuf + offset, 20);
		ewp_info->selfdefine[20] = 0;
		offset += 20;
		check_len -= 20;
		*/
		
		#ifdef __LOG__
			dcs_log(0, 0, "selfdefine=[%s]", ewp_info->selfdefine);
		#endif
	}	
	/*modify by hw 20140924 IC卡公钥参数下载结束*/
	else if(memcmp(ewp_info->consumer_transdealcode, "999996", 6)==0)
	{
		dcs_log(0, 0, "IC卡公钥参数下载结束");
		
		/*下载标识 1：公钥下载， 2：参数下载*/
		memset(ewp_info->netmanagecode, 0, sizeof(ewp_info->netmanagecode));
		memcpy(ewp_info->netmanagecode, rcvbuf + offset, 1);
		dcs_log(0, 0, "下载标识[%s], 1:公钥下载结束， 0：参数下载结束", ewp_info->netmanagecode);
		offset += 1;
		check_len -= 1;
		
		/*20个字节的自定义域*/
		memcpy(ewp_info->selfdefine, rcvbuf + offset, 20);
		ewp_info->selfdefine[20] = 0;
		offset += 20;
		check_len -= 20;
		
		#ifdef __LOG__
			dcs_log(0, 0, "selfdefine=[%s]", ewp_info->selfdefine);
		#endif
	}	
	/*modify by hw 20140925 区别消费和消费冲正，本段为消费冲正交易*/
	else if(memcmp(ewp_info->consumer_transtype, "0400", 4)==0)
	{	
		dcs_log(0, 0, "消费冲正");
		
		/*原交易金额*/
		memcpy(ewp_info->consumer_money, rcvbuf + offset, 12);
		ewp_info->consumer_money[12]=0;
		offset += 12;
		check_len -= 12;
		dcs_log(0, 0, "consumer_money=[%s]", ewp_info->consumer_money);
		
		/*随机数*/
		memcpy(ewp_info->consumer_track_randnum, rcvbuf + offset, 16);
		ewp_info->consumer_track_randnum[16] = 0;
		offset += 16;
		check_len -= 16;

#ifdef __LOG__
		dcs_log(0, 0, "consumer_track_randnum=[%s]", ewp_info->consumer_track_randnum);
#endif
		/*解析sk或车载项目的mac信息或盛京银行上送的mac信息*/
		if( memcmp(ewp_info->consumer_sentinstitu, "0400", 4) == 0 
		|| memcmp(ewp_info->consumer_sentinstitu, "0500", 4) == 0
		|| memcmp(ewp_info->consumer_sentinstitu, "0800", 4) == 0 )
		{
			memcpy(ewp_info->macinfo, rcvbuf + offset, 8);
			ewp_info->macinfo[8] = 0;
			offset += 8;
			check_len -= 8;
		
			#ifdef __LOG__
				dcs_log(0, 0, "macinfo=[%s]", ewp_info->macinfo);
			#endif
		}
		
		/*IC卡数据域长度*/
		memcpy(ewp_info->filed55_length, rcvbuf + offset, 3);
#ifdef __LOG__
		dcs_log(0, 0, "IC卡数据域[%s]", ewp_info->filed55_length);
#endif
		offset += 3;
		check_len -= 3;
		
		if( memcmp(ewp_info->filed55_length, "000", 3)!=0 )
		{
			/*IC卡数据域*/
			temp_len = atoi(ewp_info->filed55_length);
			memcpy(ewp_info->filed55, rcvbuf + offset, temp_len);
#ifdef __LOG__
			dcs_log(0, 0, "IC卡数据域[%s]", ewp_info->filed55);
#endif
			offset += temp_len;
			check_len -= temp_len;
		}
		
		/*20个字节的自定义域
		memcpy(ewp_info->selfdefine, rcvbuf + offset, 20);
		ewp_info->selfdefine[20] = 0;
		offset += 20;
		check_len -= 20;
		*/
		
		#ifdef __LOG__
			dcs_log(0, 0, "selfdefine=[%s]", ewp_info->selfdefine);
		#endif
	}
	
	#ifdef __LOG__
		dcs_log(0, 0, "解析EWP数据, check_len=[%d]", check_len);
		dcs_log(0, 0, "解析EWP数据, offset=[%d]", offset);
		
		dcs_log(0, 0, "check_len=[%d]", check_len);
	#endif
	
	/*poitcde_flag 标志位赋值 0:存量磁条, 1:IC 不降级, 2:IC 降级, 3:纯磁条 , 4:挥卡*/
	memset(ewp_info->serinputmode, 0x00, sizeof(ewp_info->serinputmode));
	if(memcmp(ewp_info->icflag, "2", 1)==0)// 添加挥卡方式
	{
		memcpy(ewp_info->poitcde_flag, "4",1);
		memcpy(ewp_info->serinputmode, "07", 2);
	}
	else if(memcmp(ewp_info->icflag, "1", 1)==0)
	{
		if(memcmp(ewp_info->fallback_flag, "0", 1)==0)
		{
			memcpy(ewp_info->poitcde_flag, "1",1);
			memcpy(ewp_info->serinputmode, "05", 2);
		}
		if(memcmp(ewp_info->fallback_flag, "1", 1)==0)
		{
			memcpy(ewp_info->poitcde_flag, "2", 1);
			memcpy(ewp_info->serinputmode, "02", 2);
		}
	}
	else if(memcmp(ewp_info->icflag, "0", 1)==0)
	{
		memcpy(ewp_info->poitcde_flag, "3", 1);
		memcpy(ewp_info->serinputmode, "02", 2);
	}
	else
	{
		memcpy(ewp_info->poitcde_flag, "0", 1);
		memcpy(ewp_info->serinputmode, "02", 2);
	}
	
	if(memcmp(ewp_info->consumer_password, "FFFFFFFFFFFFFFFF", 16) == 0 || memcmp(ewp_info->consumer_password, "ffffffffffffffff", 16) == 0 || strlen(ewp_info->consumer_password) == 0)
	{
		sprintf(ewp_info->serinputmode, "%s2", ewp_info->serinputmode);
	}
	else 
	{
		sprintf(ewp_info->serinputmode, "%s1", ewp_info->serinputmode);
	}	

	if(check_len != 0 )
		strcpy(ewp_info->retcode, "F1");
	else
	{
		strcpy(ewp_info->retcode, "00");
		ewp_info->retcode[2] = 0;
	}
	
	if(	memcmp(ewp_info->retcode, "00", 2) == 0)
		dcs_log(0, 0, "EWP数据解析成功, retcode=[%s]", ewp_info->retcode);
	else
		dcs_log(0, 0, "EWP数据不正确, 直接丢弃该包, retcode=[%s]", ewp_info->retcode);
		

	return 0;
}

int WriteKeyBack(char *keyData, int size, int folderId)
{
	#ifdef __TEST__
    if(fold_get_maxmsg(folderId) < 100)
		fold_set_maxmsg(folderId, 500) ;
 	#else
	if(fold_get_maxmsg(folderId) <2)
		fold_set_maxmsg(folderId, 20) ;
    #endif

#ifdef __LOG__
	dcs_debug(keyData, size, "WriteKeyBack:");
#else
	dcs_log(0, 0, "WriteKeyBack");
#endif
	fold_write(folderId, -1, keyData, size);
	return 0;
}

/*
组返回EWP平台的报文
code = 0,前置平台返回EWP的错误信息报文，pep_jnls为null，返回数据从ewp_info中取；
code = 1,前置平台返回EWP的交易响应报文，ewp_info为null，返回数据从pep_jnls中取；
*/
int WriteEWP(ISO_data siso,EWP_INFO ewp_info,PEP_JNLS pep_jnls,int code)
{
	int gs_comfid = -1, s, rtn;
	char buffer1[1024], buffer2[1024], sendMac[9];
	
    memset(buffer2, 0, sizeof(buffer2));
  
    gs_comfid = pep_jnls.trnsndpid;
	
	#ifdef __TEST__
    if(fold_get_maxmsg(gs_comfid) < 100)
		fold_set_maxmsg(gs_comfid, 500) ;
 	#else
	if(fold_get_maxmsg(gs_comfid) <2)
		fold_set_maxmsg(gs_comfid, 20) ;
    #endif
		
	//组回EWP的报文
	s = DataBackToEWP(code, buffer2, pep_jnls, ewp_info, siso);
	//0200 pos管理平台
	//0700 融易付平台
	if( memcmp(ewp_info.consumer_sentinstitu, "0200", 4)==0 || memcmp(ewp_info.consumer_sentinstitu, "0700", 4)==0 )
	{
		memset(sendMac, 0, sizeof(sendMac));
		rtn = pub_cust_mac(ewp_info.consumer_sentinstitu, buffer2, s, sendMac);
		if(rtn !=0)
		{
			dcs_log(0, 0, "返回管理平台、融易付平台[%s]的mac计算错误,返回-1", ewp_info.consumer_sentinstitu);
			return -1;
		}
		else
		{
			memcpy(buffer2+s, sendMac, 8);
			s +=8;
		}
	}

	if(getstrlen(ewp_info.tpduheader) != 0)
	{
		memset(buffer1, 0, sizeof(buffer1));
		asc_to_bcd((unsigned char *)buffer1, (unsigned char*)ewp_info.tpduheader, getstrlen(ewp_info.tpduheader), 0);
		
		memcpy(buffer1+5, buffer2, s);
		s += 5;
		
		memcpy(buffer2, buffer1, 1);
		memcpy(buffer2+1, buffer1+3, 2);
		memcpy(buffer2+3, buffer1+1, 2);
		memcpy(buffer2+5, buffer1+5, s-5);

	#ifdef __LOG__
		dcs_debug(buffer2, s, "[%s] data_buf len=%d,foldId=%d", pep_jnls.trnsndp, s, gs_comfid);
	#else
		dcs_log(0, 0, "[%s] data_buf len=%d,foldId=%d", pep_jnls.trnsndp, s, gs_comfid);
	#endif
		fold_write(gs_comfid, -1, buffer2, s);
	}
	else
	{
	#ifdef __LOG__
		dcs_debug(buffer2,s,"[%s] data_buf len=%d,foldId=%d", pep_jnls.trnsndp, s, gs_comfid);
	#else
		dcs_log(0, 0, "[%s] data_buf len=%d,foldId=%d", pep_jnls.trnsndp, s, gs_comfid);
	#endif
		fold_write(gs_comfid, -1, buffer2, s);
	}
	return 0;
}

/*
组返回EWP平台的报文
code = 0,前置平台返回EWP的错误信息报文，返回数据从ewp_info中取；
code = 1,前置平台返回EWP的交易响应报文，返回数据从pep_jnls中取；
*/
int DataBackToEWP(int code, char *buffer, PEP_JNLS pep_jnls, EWP_INFO ewp_info, ISO_data siso)
{
	char tmpbuf[512], ewpinfo[1024], temp_asc_55[512];
	char buf[100];
	int ewpinfo_len=0, len=0, merlen=0, s=0;
	POS_CUST_INFO pos_cust_info;
	
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(ewpinfo, 0, sizeof(ewpinfo));
	memset(temp_asc_55, 0, sizeof(temp_asc_55));
	memset(buf, 0, sizeof(buf));
	memset(&pos_cust_info, 0, sizeof(POS_CUST_INFO));
	
	/*获取IC卡公钥参数下载标识,默认不更新*/
	memcpy(ewp_info.download_pubkey_flag, "1", 1);
	memcpy(ewp_info.download_para_flag, "1", 1);	
	key_para_update(&ewp_info);
	
	if(code==0)
	{
		dcs_log(0, 0, "FAILED");	
		/*4个字节的交易类型数据*/
		if(memcmp(ewp_info.consumer_transtype,"0200",4)==0)
			sprintf( buffer, "%s", "0210");
		else if(memcmp(ewp_info.consumer_transtype,"0400",4)==0)
			sprintf( buffer, "%s", "0410");
		else if(memcmp(ewp_info.consumer_transtype,"0320",4)==0)
			sprintf( buffer, "%s", "0330");
		else if(memcmp(ewp_info.consumer_transtype,"0620",4)==0)
			sprintf( buffer, "%s", "0630");
		else
			sprintf( buffer, "%s", ewp_info.consumer_transtype); 
			
		#ifdef __LOG__
			dcs_log(0,0,"ewp_info.consumer_transtype=%s",ewp_info.consumer_transtype);
		#endif
//		dcs_log(0,0,"buffer 交易类型 [%s]",buffer);
		 /*6个字节的交易处理码*/
		sprintf( buffer, "%s%s",buffer, ewp_info.consumer_transdealcode);
//		dcs_log(0,0,"buffer 交易处理码 [%s]",buffer);
		/*8个字节  接入系统发送交易的日期*/
		sprintf( buffer, "%s%s",buffer, ewp_info.consumer_sentdate);
//		dcs_log(0,0,"buffer 发送交易的日期 [%s]",buffer);
		 /*6个字节  接入系统发送交易的时间*/
		sprintf( buffer, "%s%s",buffer, ewp_info.consumer_senttime);
//		dcs_log(0,0,"buffer 发送交易的时间 [%s]",buffer);
		/*4个字节  接入系统的编号EWP：0100*/
		sprintf( buffer, "%s%s",buffer, ewp_info.consumer_sentinstitu); 
//		dcs_log(0,0,"buffer 系统的编号 [%s]",buffer);
		/*3个字节  区分业务类型订单支付：210*/
		sprintf( buffer, "%s%s",buffer, ewp_info.consumer_transcode); 
//		dcs_log(0,0,"buffer 交易码 [%s]",buffer);
		/*消费类的交易如：订单支付 000000*/
		if(memcmp(ewp_info.consumer_transdealcode, "9F0000", 6)!=0 
			&& memcmp(ewp_info.consumer_transtype, "0620", 4)!=0 
			&& memcmp(ewp_info.consumer_transtype, "0320", 4)!=0 )
		{
			/*2个字节的交易发起方式*/
			sprintf( buffer, "%s%s",buffer, ewp_info.translaunchway);
			dcs_log(0,0,"buffer 订单支付 [%s]",buffer);
		}
		
		/*16个字节刷卡设备编号*/
		sprintf( buffer,"%s%s",buffer,ewp_info.consumer_psamno);
//		dcs_log(0,0,"buffer 设备编号 [%s]",buffer);

		/*20个字节 手机编号左对齐 不足补空格*/
		sprintf( buffer,"%s%-20s",buffer,ewp_info.consumer_phonesnno);
//		dcs_log(0,0,"buffer 手机编号 [%s]",buffer);
		
		if(memcmp(ewp_info.consumer_transtype,"0320",4)==0)
		{
			sprintf( buffer,"%s%s",buffer,"F1");
			dcs_log(0,0,"buffer [ 应答码F1]");
			len=strlen(buffer);
			return len;
		}
		if(memcmp(ewp_info.consumer_transtype, "0620", 4)!=0)
		{
			/*11个字节 手机号，例如：1502613236*/
			sprintf( buffer,"%s%s",buffer,ewp_info.consumer_username);
//			dcs_log(0,0,"buffer 用户名 [%s]",buffer);
			/*20个字节的订单号， 左对齐 不足补空格*/
			sprintf( buffer,"%s%-20s",buffer,ewp_info.consumer_orderno);
//			dcs_log(0,0,"buffer 订单号 [%s]",buffer);
			
			/*20个字节的商户编号*/
			sprintf( buffer,"%s%20s",buffer,ewp_info.consumer_merid);
#ifdef __LOG__
			dcs_log(0,0,"buffer 商户编号 [%s]",buffer);
#endif
			/*19个字节卡号 左对齐 不足补空格*/
			sprintf( buffer,"%s%-19s",buffer,ewp_info.consumer_cardno);
//			dcs_log(0,0,"buffer 卡号 [%s]",buffer);
		}
		getbit(	&siso,39,(unsigned char *)ewp_info.consumer_responsecode);
		ewpinfo_len = GetDetailInfo(ewp_info.consumer_responsecode, ewp_info.consumer_transcode, ewpinfo);
		if(ewpinfo_len == -1)
			ewpinfo_len = 0;

		dcs_log(0, 0, "取交易应答描述成功[%s]", ewpinfo);
		dcs_log(0, 0, "ewp_info.consumer_transtype[%s]", ewp_info.consumer_transtype);
		dcs_log(0, 0, "ewp_info.consumer_transdealcode[%s]", ewp_info.consumer_transdealcode);
		
		if(memcmp(ewp_info.consumer_transtype, "0620", 4)!=0 
			&& (memcmp(ewp_info.consumer_transdealcode,"400000",6)==0
				||memcmp(ewp_info.consumer_transdealcode,"480000",6)==0
				||memcmp(ewp_info.consumer_transdealcode, "190000", 6)==0))
		{
			/*19个字节转入卡号 左对齐 不足补空格*/
			sprintf( buffer,"%s%-19s",buffer,ewp_info.incardno);
			if(memcmp(ewp_info.consumer_transdealcode,"480000",6)==0)
			{
				dcs_log(0,0,"转账汇款交易.");
				/*50个字节转入卡姓名 左对齐 不足补空格*/
				memset(tmpbuf, 0x00, sizeof(tmpbuf));
				/*返回ewp的姓名格式需要为GBK*/
				u2g(ewp_info.incdname, strlen(ewp_info.incdname), tmpbuf, sizeof(tmpbuf));
				sprintf( buffer, "%s%-50s", buffer, tmpbuf);
				/*25个字节转入卡行号  左对齐 不足补空格*/
				sprintf( buffer, "%s%-25s", buffer, ewp_info.incdbkno);
			}
			else if(memcmp(ewp_info.consumer_transdealcode, "190000", 6)==0)
				dcs_log(0,0,"信用卡还款类的交易.");
			else if(memcmp(ewp_info.consumer_transdealcode, "400000", 6)==0)
				dcs_log(0,0,"ATM转账类的交易.");
			/*12个字节 金额右对齐 不足补0*/
			sprintf( buffer,"%s%012s",buffer,ewp_info.consumer_money);	
			sprintf(buffer,"%s%s",buffer,ewp_info.transfee);
			/*2个字节 应答码*/
			sprintf( buffer,"%s%2s",buffer,ewp_info.consumer_responsecode);
		
			/*6个字节 前置系统流水*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			if( getbit(	&siso,11,(unsigned char *)tmpbuf) <= 0 )
				sprintf( buffer,"%s%6s",buffer,"000000");
			else
				sprintf( buffer,"%s%6s",buffer,tmpbuf);
		
			/*12个字节 系统参考号*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			if( getbit( &siso,37,(unsigned char *)tmpbuf) <= 0 )
				sprintf( buffer,"%s%12s",buffer,"000000000000");
			else
				sprintf( buffer,"%s%12s",buffer,tmpbuf);
			
			/*6个字节的授权码*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			if( getbit( &siso,38,(unsigned char *)tmpbuf) <= 0 )
				sprintf( buffer,"%s%s",buffer,"000000");
			else
				sprintf( buffer,"%s%s",buffer,tmpbuf);
				
			dcs_log(0,0,"authcode = [%s]",tmpbuf);	
			/*1个字节表示应答码内容的长度*/
			/*应答码描述信息，utf-8表示*/
			len = strlen(buffer);
			
			buffer[len] = ewpinfo_len;
			len +=1;
			memcpy(buffer+len, ewpinfo, ewpinfo_len);
			len +=ewpinfo_len;
			//如果是sk项目的话
			if(memcmp(ewp_info.consumer_sentinstitu, "0400", 4) == 0 
			|| memcmp(ewp_info.consumer_sentinstitu, "0800", 4) == 0)
			{
				dcs_log(0, 0, "into SK program get GetCustInfo");	
				s = GetCustInfo(ewp_info.consumer_username, ewp_info.consumer_psamno, &pos_cust_info);
				if(s < 0)
				{
					dcs_log(0, 0, "get pos_cust_info error");	
					memcpy(buffer+len, "               ", 15);
					len += 15; 
					memcpy(buffer+len, "        ", 8);
					len += 8; 
					buffer[len] = 0;
					len +=1;
				}
				else
				{
					memcpy(buffer+len, pos_cust_info.cust_merid, 15);
					len += 15; 
					memcpy(buffer+len, pos_cust_info.cust_termid, 8);
					len += 8; 
					merlen = getstrlen(pos_cust_info.merdescr);
					buffer[len] = merlen;
					len +=1;
					memcpy(buffer+len, pos_cust_info.merdescr, merlen);
					len += merlen; 
				}
			}
			
			/*IC卡数据域,转账汇款下发，信用卡还款下发,上线时修改*/
			if((memcmp(ewp_info.consumer_transdealcode, "480000", 6)==0 || memcmp(ewp_info.consumer_transdealcode, "190000", 6)==0)
			&& memcmp(ewp_info.poitcde_flag, "0", 1)!=0)
			{	
				s = getbit(&siso, 55,(unsigned char *)tmpbuf);
				if(s <= 0)
		 		{
		 			dcs_log(0, 0, "bit55 is null!");
					memcpy(buffer+len, "000", 3);
					len +=3;
		 		}	
				else
				{
					bcd_to_asc((unsigned char *)temp_asc_55, (unsigned char *)tmpbuf, 2*s, 0);
					memset(tmpbuf, 0x00, sizeof(tmpbuf));
					sprintf(tmpbuf, "%03d%s", 2*s, temp_asc_55);
					memcpy(buffer+len, tmpbuf, 2*s+3);
					len +=2*s+3;
				}
			}
			/*
			else if(memcmp(ewp_info.consumer_transdealcode,"190000",6)==0 && memcmp(ewp_info.poitcde_flag, "0", 1)!=0)
			{
				len = strlen(buffer);
				memcpy(buffer+len, "000", 3);
				len +=3;
			}
			*/
			
			/*存量机器除外，都需检查并下发IC卡参数公钥更新标识，1表示需要处理，0表示无需处理 */
			if(memcmp(ewp_info.poitcde_flag, "0", 1)!=0 )
			{
				sprintf(buffer+len,"%s%s",ewp_info.download_pubkey_flag, ewp_info.download_para_flag);
				len +=2;
			}
			
			/*20个字节的自定义域原样返回给EWP*/
			memcpy(buffer+len,ewp_info.selfdefine, getstrlen(ewp_info.selfdefine));
			len +=20;
			return len;
		}
		
		/*消费类的交易如：订单支付 000000*/
		else if(memcmp(ewp_info.consumer_transtype, "0620", 4)!=0 
			 && memcmp(ewp_info.consumer_transdealcode,"000000",6)==0
			 && memcmp(ewp_info.consumer_transtype, "0400", 4)!=0)
		{
			dcs_log(0,0,"消费类的交易.");
			/*12个字节 金额右对齐 不足补0*/
			sprintf( buffer,"%s%012s",buffer,ewp_info.consumer_money);	
			
			/*2个字节 应答码*/
			sprintf( buffer,"%s%2s",buffer,ewp_info.consumer_responsecode);
		
			/*6个字节 前置系统流水*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			if( getbit(&siso,11,(unsigned char *)tmpbuf) <= 0 )
				sprintf( buffer,"%s%6s",buffer,"000000");
			else
				sprintf( buffer,"%s%6s",buffer,tmpbuf);
		
			/*12个字节 系统参考号*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			if( getbit( &siso,37,(unsigned char *)tmpbuf) <= 0 )
				sprintf( buffer,"%s%12s",buffer,"000000000000");
			else
				sprintf( buffer,"%s%12s",buffer,tmpbuf);
			
			/*6个字节的授权码*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			if( getbit( &siso,38,(unsigned char *)tmpbuf) <= 0 )
				sprintf( buffer,"%s%s",buffer,"000000");
			else
				sprintf( buffer,"%s%s",buffer,tmpbuf);
				
			dcs_log(0,0,"authcode = [%s]",tmpbuf);	
			/*1个字节表示应答码内容的长度*/
			/*应答码描述信息，utf-8表示*/
			/*
			ewpinfo_len = 65;
			memset(ewpinfo,0,sizeof(ewpinfo));
			sprintf(ewpinfo, "\\u67e5\\u8be2samcard_info\\u5931\\u8d25,psam\\u5361\\u672a\\u627e\\u5230");
			*/
			len = strlen(buffer);
			buffer[len] = ewpinfo_len;
			len +=1;
			memcpy(buffer+len, ewpinfo, ewpinfo_len);
			len +=ewpinfo_len;
			
			/*如果是收款交易就返回EWP收单商户号，收单终端号，商户名称*/
			if( memcmp(ewp_info.consumer_sentinstitu, "0400", 4) == 0 
			|| memcmp(ewp_info.consumer_sentinstitu, "0500", 4) == 0
			|| memcmp(ewp_info.consumer_sentinstitu, "0800", 4) == 0)
			{
				//if(memcmp(ewp_info.consumer_transcode, "200", 3) == 0)
				//	dcs_log(0, 0, "收款类交易 ");
				//else if(memcmp(ewp_info.consumer_sentinstitu, "0500", 4) == 0)
				//	dcs_log(0, 0, "车载类交易 ");
				//else
				//	dcs_log(0, 0, "SK program ");	
				
				dcs_log(0, 0, "consumer_sentinstitu = %s", ewp_info.consumer_sentinstitu);	
					
				s = GetCustInfo(ewp_info.consumer_username, ewp_info.consumer_psamno, &pos_cust_info);
				if(s < 0)
				{
					dcs_log(0, 0, "get pos_cust_info error");	
					memcpy(buffer+len, "               ", 15);
					len += 15; 
					memcpy(buffer+len, "        ", 8);
					len += 8; 
					buffer[len] = 0;
					len +=1;
				}
				else
				{
					memcpy(buffer+len, pos_cust_info.cust_merid, 15);
					len += 15; 
					memcpy(buffer+len, pos_cust_info.cust_termid, 8);
					len += 8; 
					merlen = getstrlen(pos_cust_info.merdescr);
					buffer[len] = merlen;
					len +=1;
					memcpy(buffer+len, pos_cust_info.merdescr, merlen);
					len += merlen; 
				}
				
			}
			
			/*IC卡数据域*/
			if(memcmp(ewp_info.poitcde_flag, "0", 1)!=0)
			{	
				s = getbit(&siso, 55,(unsigned char *)tmpbuf);
				if(s <= 0)
		 		{
		 			dcs_log(0, 0, "bit55 is null!");
					memcpy(buffer+len, "000", 3);
					len +=3;
		 		}	
				else
				{
					bcd_to_asc((unsigned char *)temp_asc_55, (unsigned char *)tmpbuf, 2*s, 0);
					memset(tmpbuf, 0x00, sizeof(tmpbuf));
					sprintf(tmpbuf, "%03d%s", 2*s, temp_asc_55);
					memcpy(buffer+len, tmpbuf, 2*s+3);
					len +=2*s+3;
				}
			
				/*存量机器除外，都需检查并下发IC卡参数公钥更新标识，1表示需要处理，0表示无需处理 */
				sprintf(buffer+len,"%s%s",ewp_info.download_pubkey_flag, ewp_info.download_para_flag);
				len +=2;
			}
			
			/*20个字节的自定义域原样返回给EWP*/
			memcpy(buffer+len,ewp_info.selfdefine, getstrlen(ewp_info.selfdefine));
			len +=20;
			return len;
				
		}
		/*查询类的交易*/
		else if(memcmp(ewp_info.consumer_transdealcode,"9F0000",6)==0)
		{
			dcs_log(0,0,"查询类的交易.");
			/*2个字节 应答码*/
			sprintf( buffer,"%s%2s",buffer,ewp_info.consumer_responsecode);
			/*6个字节 前置系统流水*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			getbit(	&siso,11,(unsigned char *)tmpbuf);
			sprintf( buffer,"%s%s",buffer,tmpbuf);
		
			/*12个字节 系统参考号*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			getbit(	&siso,37,(unsigned char *)tmpbuf);
			sprintf( buffer,"%s%12s",buffer,tmpbuf);
					
			/*6个字节的授权码*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			if( getbit( &siso,38,(unsigned char *)tmpbuf) <= 0 )
				sprintf( buffer,"%s%s",buffer,"000000");
			else	
				sprintf( buffer,"%s%s",buffer,tmpbuf);
			dcs_log(0,0,"authcode = [%s]",tmpbuf);
			len = strlen(buffer);	
			
			buffer[len] = ewpinfo_len;
			len +=1;
			memcpy(buffer+len, ewpinfo, ewpinfo_len);
			len +=ewpinfo_len;
			/*SK EWP项目*/
			if(memcmp(ewp_info.consumer_sentinstitu, "0400", 4) == 0 
			|| memcmp(ewp_info.consumer_sentinstitu, "0800", 4) == 0)
			{
				dcs_log(0, 0, "SK program ");	
				s = GetCustInfo(ewp_info.consumer_username, ewp_info.consumer_psamno, &pos_cust_info);
				if(s < 0)
				{
					dcs_log(0, 0, "get pos_cust_info error");	
					memcpy(buffer+len, "               ", 15);
					len += 15; 
					memcpy(buffer+len, "        ", 8);
					len += 8; 
					buffer[len] = 0;
					len +=1;
				}
				else
				{
					memcpy(buffer+len, pos_cust_info.cust_merid, 15);
					len += 15; 
					memcpy(buffer+len, pos_cust_info.cust_termid, 8);
					len += 8; 
					merlen = getstrlen(pos_cust_info.merdescr);
					buffer[len] = merlen;
					len +=1;
					memcpy(buffer+len, pos_cust_info.merdescr, merlen);
					len += merlen; 
				}
			}
			
			/*20个字节的自定义域原样返回给EWP*/
			memcpy(buffer+len,ewp_info.selfdefine, getstrlen(ewp_info.selfdefine));
			len +=20;
			return len;
					
		}
		
		/*余额查询的交易*/
		else if(memcmp(ewp_info.consumer_transtype, "0620", 4)!=0
			 && memcmp(ewp_info.consumer_transdealcode,"310000",6)==0)
		{
			dcs_log(0,0,"余额查询的交易.");
			/*2个字节 应答码*/
			sprintf( buffer,"%s%2s",buffer,ewp_info.consumer_responsecode);
		
			/*6个字节 前置系统流水*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			if( getbit( &siso,11,(unsigned char *)tmpbuf) <=0 )
				sprintf( buffer,"%s%s",buffer,"000000");
			else	
				sprintf( buffer,"%s%s",buffer,tmpbuf);
		
			/*12个字节 系统参考号*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			if(getbit( &siso,37,(unsigned char *)tmpbuf) <=0 )
				sprintf( buffer,"%s%s",buffer,"000000000000");
			else	
				sprintf( buffer,"%s%12s",buffer,tmpbuf);
			
			/*应答码为00成功时，才返回金额。否则全为0*/
			/*12个字节 金额右对齐 不足补空格*/
			sprintf( buffer,"%s%024s",buffer,"000");	
			
			
			/*6个字节的授权码*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			if( getbit( &siso,38,(unsigned char *)tmpbuf) <= 0 )
				sprintf( buffer,"%s%s",buffer,"000000");
			else	
				sprintf( buffer,"%s%s",buffer,tmpbuf);
			dcs_log(0,0,"authcode = [%s]",tmpbuf);	
			len = strlen(buffer);
			
			buffer[len] = ewpinfo_len;
			len +=1;
			memcpy(buffer+len, ewpinfo, ewpinfo_len);
			len +=ewpinfo_len;
			/*SK EWP项目*/
			if(memcmp(ewp_info.consumer_sentinstitu, "0400", 4) == 0 
			|| memcmp(ewp_info.consumer_sentinstitu, "0800", 4) == 0)
			{
				dcs_log(0, 0, "SK program 余额查询");
				s = GetCustInfo(ewp_info.consumer_username, ewp_info.consumer_psamno, &pos_cust_info);
				if(s < 0)
				{
					dcs_log(0, 0, "get pos_cust_info error");	
					memcpy(buffer+len, "               ", 15);
					len += 15; 
					memcpy(buffer+len, "        ", 8);
					len += 8; 
					buffer[len] = 0;
					len +=1;
				}
				else
				{
					memcpy(buffer+len, pos_cust_info.cust_merid, 15);
					len += 15; 
					memcpy(buffer+len, pos_cust_info.cust_termid, 8);
					len += 8; 
					merlen = getstrlen(pos_cust_info.merdescr);
					buffer[len] = merlen;
					len +=1;
					memcpy(buffer+len, pos_cust_info.merdescr, merlen);
					len += merlen; 
				}
			}
			
			/*IC卡数据域*/
			if(memcmp(ewp_info.icflag, "0", 1)!=0)
			{	
				s = getbit(&siso, 55,(unsigned char *)tmpbuf);
				if(s <= 0)
		 		{
		 			dcs_log(0, 0, "bit55 is null!");
					memcpy(buffer+len, "000", 3);
					len +=3;
		 		}	
				else
				{
					bcd_to_asc((unsigned char *)temp_asc_55, (unsigned char *)tmpbuf, 2*s, 0);
					memset(tmpbuf, 0x00, sizeof(tmpbuf));
					sprintf(tmpbuf, "%03d%s", 2*s, temp_asc_55);
					memcpy(buffer+len, tmpbuf, 2*s+3);
					len +=2*s+3;
				}
			
				/*存量机器除外，都需检查并下发IC卡参数公钥更新标识，1表示需要处理，0表示无需处理 */
				sprintf(buffer+len,"%s%s",ewp_info.download_pubkey_flag, ewp_info.download_para_flag);
				len +=2;
			}
			
			/*20个字节的自定义域原样返回给EWP*/
			memcpy(buffer+len,ewp_info.selfdefine, getstrlen(ewp_info.selfdefine));
			len +=20;
			return len;
		}	
		/*消费撤销类的交易*/
		else if(memcmp(ewp_info.consumer_transdealcode, "200000",6)==0)
		{
			dcs_log(0,0,"消费撤销类的交易.");
			/*2个字节 应答码*/
			sprintf( buffer,"%s%2s",buffer,ewp_info.consumer_responsecode);
			
			/*6个字节 前置系统流水*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			if( getbit( &siso,11,(unsigned char *)tmpbuf) <=0 )
				sprintf( buffer,"%s%s",buffer,"000000");
			else	
				sprintf( buffer,"%s%s",buffer,tmpbuf);
		
			/*12个字节 系统参考号*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			if(getbit( &siso,37,(unsigned char *)tmpbuf) <=0 )
				sprintf( buffer,"%s%s",buffer,"000000000000");
			else	
				sprintf( buffer,"%s%12s",buffer,tmpbuf);
			
			/*6个字节的授权码*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			if( getbit(	&siso,38,(unsigned char *)tmpbuf) <= 0)
				sprintf( buffer, "%s%s", buffer, "000000");
			else	
				sprintf( buffer, "%s%s", buffer, tmpbuf);
				
			dcs_log(0,0,"authcode = [%s]", tmpbuf);
			len = strlen(buffer);
			//add for sj begin
			if(memcmp(pep_jnls.sendcde, "0800", 4) == 0)
			{
				s = GetCustInfo(pep_jnls.termid, pep_jnls.samid, &pos_cust_info);
				if(s < 0)
				{
					dcs_log(0, 0, "get pos_cust_info error");	
					memcpy(buffer+len, "               ", 15);
					len += 15; 
					memcpy(buffer+len, "        ", 8);
					len += 8; 
					buffer[len] = 0;
					len +=1;
				}
				else
				{
					memcpy(buffer+len, pos_cust_info.cust_merid, 15);
					len += 15; 
					memcpy(buffer+len, pos_cust_info.cust_termid, 8);
					len += 8; 
					merlen = getstrlen(pos_cust_info.merdescr);
					buffer[len] = merlen;
					len +=1;
					memcpy(buffer+len, pos_cust_info.merdescr, merlen);
					len += merlen; 	
				}	
				sprintf(buffer+len, "%s%s", ewp_info.download_pubkey_flag, ewp_info.download_para_flag);
				len +=2;
				/*20个字节的自定义域原样返回给EWP*/
				memcpy(buffer+len, ewp_info.selfdefine, getstrlen(ewp_info.selfdefine));
				len +=20;
				//add for sj end
			}
			return len;
		}
		/*消费退货的交易*/
		else if( memcmp(ewp_info.consumer_transdealcode, "270000",6)==0)
		{
			dcs_log(0,0,"消费退货的交易.");
			/*12个字节 退货的金额右对齐 不足0*/
			sprintf( buffer,"%s%012s",buffer,ewp_info.consumer_money);	
			
			/*2个字节 应答码*/
			sprintf( buffer,"%s%2s",buffer,ewp_info.consumer_responsecode);
			/*6个字节 前置系统流水*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			if( getbit( &siso,11,(unsigned char *)tmpbuf) <=0 )
				sprintf( buffer,"%s%s",buffer,"000000");
			else	
				sprintf( buffer,"%s%s",buffer,tmpbuf);
		
			/*12个字节 系统参考号*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			if(getbit( &siso,37,(unsigned char *)tmpbuf) <=0 )
				sprintf( buffer,"%s%s",buffer,"000000000000");
			else	
				sprintf( buffer,"%s%12s",buffer,tmpbuf);
			
			/*6个字节的授权码*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			if( getbit( &siso,38,(unsigned char *)tmpbuf) <= 0 )
				sprintf( buffer,"%s%s",buffer,"000000");
			else		
				sprintf( buffer,"%s%s",buffer,tmpbuf);
			dcs_log(0,0,"authcode = [%s]",tmpbuf);	
			len = strlen(buffer);
			//add for sj begin
			if(memcmp(pep_jnls.sendcde, "0800", 4) == 0)
			{
				s = GetCustInfo(pep_jnls.termid, pep_jnls.samid, &pos_cust_info);
				if(s < 0)
				{
					dcs_log(0, 0, "get pos_cust_info error");	
					memcpy(buffer+len, "               ", 15);
					len += 15; 
					memcpy(buffer+len, "        ", 8);
					len += 8; 
					buffer[len] = 0;
					len +=1;
				}
				else
				{
					memcpy(buffer+len, pos_cust_info.cust_merid, 15);
					len += 15; 
					memcpy(buffer+len, pos_cust_info.cust_termid, 8);
					len += 8; 
					merlen = getstrlen(pos_cust_info.merdescr);
					buffer[len] = merlen;
					len +=1;
					memcpy(buffer+len, pos_cust_info.merdescr, merlen);
					len += merlen; 	
				}	
				sprintf(buffer+len, "%s%s", ewp_info.download_pubkey_flag, ewp_info.download_para_flag);
				len +=2;
				/*20个字节的自定义域原样返回给EWP*/
				memcpy(buffer+len, ewp_info.selfdefine, getstrlen(ewp_info.selfdefine));
				len +=20;
				//add for sj end
			}
			return len;
		}
		/*脚本通知  0620*/
		else if(memcmp(ewp_info.consumer_transtype, "0620", 4)==0)
		{
			sprintf( buffer, "%s%2s", buffer, ewp_info.consumer_responsecode);
			len =strlen(buffer);
			buffer[len] = ewpinfo_len;
			len +=1;
			memcpy(buffer+len, ewpinfo, ewpinfo_len);
			len +=ewpinfo_len;
			
			dcs_log(0, 0, "ewpinfo_len[%d]ewpinfo[%s]!",ewpinfo_len ,ewpinfo);
			
			/*IC卡数据域*/
			if(memcmp(ewp_info.icflag, "0", 1)!=0)
			{	
				s = getbit(&siso, 55,(unsigned char *)tmpbuf);
				if(s <= 0)
		 		{
		 			dcs_log(0, 0, "bit55 is null!");
					memcpy(buffer+len, "000", 3);
					len +=3;
		 		}	
				else
				{
					bcd_to_asc((unsigned char *)temp_asc_55, (unsigned char *)tmpbuf, 2*s, 0);
					memset(tmpbuf, 0x00, sizeof(tmpbuf));
					sprintf(tmpbuf, "%03d%s", 2*s, temp_asc_55);
					memcpy(buffer+len, tmpbuf, 2*s+3);
					len +=2*s+3;
				}
			}
			
			/*20个字节的自定义域原样返回给EWP
			memcpy(buffer+len, ewp_info.selfdefine, getstrlen(ewp_info.selfdefine));
			len +=20;
			*/
			
			dcs_log(0, 0, "len= [%d]", len);
			return len;	
		}
		/*消费冲正*/
		else if(memcmp(ewp_info.consumer_transtype, "0400", 4)==0)
		{
			/*2个字节 应答码*/
			dcs_log(0,0,"消费冲正失败交易应答");
			sprintf( buffer,"%s%2s",buffer,ewp_info.consumer_responsecode);	
			dcs_log(0,0,"%s", buffer);
			len = len +2;
		}
	}
	else if(code==1)
	{	
		dcs_log(0, 0, "SUCESS");		
		/*4个字节的交易类型数据*/
		if(memcmp(pep_jnls.msgtype,"0200",4)==0)
			sprintf( buffer, "%s", "0210");
		if(memcmp(pep_jnls.msgtype,"0400",4)==0)
			sprintf( buffer, "%s", "0410");
		if(memcmp(pep_jnls.msgtype,"0620",4)==0)
			sprintf( buffer, "%s", "0630");	
			
		 /*6个字节的交易处理码*/
		sprintf( buffer, "%s%s",buffer, ewp_info.consumer_transdealcode);
		if(memcmp(pep_jnls.trancde, "211",3) == 0)//脱机交易取系统时间
		{
			/*8个字节  接入系统发送交易的日期*/
			sprintf( buffer, "%s%s",buffer, pep_jnls.pepdate);
			 /*6个字节  接入系统发送交易的时间*/
			sprintf( buffer, "%s%s",buffer, pep_jnls.peptime);
		}
		else
		{
			/*8个字节  接入系统发送交易的日期*/
			sprintf( buffer, "%s%s",buffer, pep_jnls.trndate);
			 /*6个字节  接入系统发送交易的时间*/
			sprintf( buffer, "%s%s",buffer, pep_jnls.trntime);
		}

		
		/*4个字节  接入系统的编号EWP：0100 */
		memset(tmpbuf,0,sizeof(tmpbuf));
		memcpy(tmpbuf,pep_jnls.sendcde,4);
		sprintf( buffer, "%s%s",buffer,tmpbuf);   
		
		/*3个字节  区分业务类型订单支付：210??*/
		sprintf( buffer, "%s%s",buffer, pep_jnls.trancde); 
		if(memcmp(ewp_info.consumer_transdealcode,"9F0000",6)!=0
			 && memcmp(pep_jnls.msgtype,"0620",4)!=0) 
		{
			/*2个字节的交易发起方式*/
			sprintf( buffer, "%s%s",buffer, ewp_info.translaunchway);
		} 
		/*16个字节刷卡设备编号也即psam编号*/
		pep_jnls.samid[16]='\0';
		sprintf( buffer, "%s%-16s",buffer, pep_jnls.samid); 
		dcs_log(0,0,"pep_jnls.samid = [%s]",tmpbuf);
		
		/*20个字节 手机编号(机具编号)左对齐 不足补空格??*/
		pep_jnls.termidm[20] ='\0';
		sprintf( buffer,"%s%-20s",buffer,pep_jnls.termidm);
		
		//if(memcmp(ewp_info.consumer_transtype, "0630", 4)!=0)
		if(memcmp(pep_jnls.msgtype,"0620",4)!=0)
		{
			sprintf( buffer, "%s%-11s",buffer, pep_jnls.termid);
			/*20个字节 订单号，左对齐 不足补空格*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			memcpy(tmpbuf,pep_jnls.billmsg,20);
			sprintf( buffer, "%s%-20s",buffer,tmpbuf); 
			dcs_log(0,0,"pep_jnls.billmsg = [%s]",tmpbuf);
			
			/*20个字节的商户编号*/
			sprintf( buffer,"%s%20d",buffer,pep_jnls.merid);
			
			/*19个字节卡号， 左对齐 不足补空格*/
			pub_rtrim_lp(pep_jnls.outcdno,strlen(pep_jnls.outcdno),pep_jnls.outcdno,1);
#ifdef __TEST__
			dcs_log(0,0,"pep_jnls.outcdno = [%s]",pep_jnls.outcdno);
#endif
			sprintf(buffer,"%s%-19s",buffer,pep_jnls.outcdno);
		}
		
		/*根据应答码和交易码得到关于交易应答码的详细描述信息*/
		ewpinfo_len = GetDetailInfo(pep_jnls.aprespn, pep_jnls.trancde, ewpinfo);
		/*1个字节表示应答码内容的长度*/
		/*应答码描述信息，utf-8表示*/
		if(ewpinfo_len == -1)
			ewpinfo_len = 0;
		/*转账类的交易*/
		if(memcmp(ewp_info.consumer_transtype, "0620", 4)!=0
		&& (memcmp(ewp_info.consumer_transdealcode, "400000",6)==0
		||memcmp(ewp_info.consumer_transdealcode, "480000",6)==0
		||memcmp(ewp_info.consumer_transdealcode, "190000",6)==0))
		{		
			/*19个字节转入卡号， 左对齐 不足补空格??*/
			pub_rtrim_lp(pep_jnls.intcdno, strlen(pep_jnls.intcdno), pep_jnls.intcdno, 1);
#ifdef __TEST__
			dcs_log(0,0,"pep_jnls.intcdno = [%s]", pep_jnls.intcdno);
#endif
			if(memcmp(ewp_info.consumer_transdealcode, "480000", 6)==0)
			{
				dcs_log(0,0,"转账汇款交易.");
				/*50个字节转入卡姓名 左对齐 不足补空格*/
				memset(tmpbuf, 0x00, sizeof(tmpbuf));
				/*返回ewp的姓名格式需要为GBK*/
				u2g(ewp_info.incdname, strlen(ewp_info.incdname), tmpbuf, sizeof(tmpbuf));
				sprintf( buffer, "%s%-50s", buffer, tmpbuf);
				/*25个字节转入卡行号  左对齐 不足补空格*/
				sprintf( buffer, "%s%-25s", buffer, ewp_info.incdbkno);
			}
			else if(memcmp(ewp_info.consumer_transdealcode,"400000",6)==0)
				dcs_log(0, 0, "ATM转账类的交易.");
			else if(memcmp(ewp_info.consumer_transdealcode,"190000",6)==0)
				dcs_log(0, 0, "信用卡还款类的交易.");
			sprintf(buffer,"%s%-19s",buffer,pep_jnls.intcdno);
			/*12个字节 金额，右对齐 不足补0*/
			sprintf(buffer,"%s%012s",buffer,pep_jnls.tranamt);
			sprintf(buffer,"%s%s",buffer,ewp_info.transfee);
			/*2个字节前置系统的交易应答码??*/
			sprintf( buffer,"%s%s",buffer,pep_jnls.aprespn);
			/*6个字节  前置系统交易流水trance */
			sprintf( buffer,"%s%06d",buffer,pep_jnls.trace);
			/*12个字节前置系统返回的交易参考号??*/
			if(getstrlen(pep_jnls.syseqno)==0)
				sprintf( buffer,"%s%012d",buffer, 0);
			else
				sprintf( buffer,"%s%12s",buffer,pep_jnls.syseqno);		
			/*6个字节 授权码*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			if( getbit( &siso,38,(unsigned char *)tmpbuf) <= 0 )
				sprintf( buffer,"%s%s",buffer,"000000");
			else
				sprintf( buffer,"%s%6s",buffer,tmpbuf);
			dcs_log(0,0,"pep_jnls.authcode = [%s]",tmpbuf);	
			len = strlen(buffer);
			buffer[len] = ewpinfo_len;
			len +=1;
			memcpy(buffer+len, ewpinfo, ewpinfo_len);
			len +=ewpinfo_len;
			/*sk项目返回EWP*/
			if(memcmp(pep_jnls.sendcde, "0400", 4) == 0 
			|| memcmp(pep_jnls.sendcde, "0800", 4) == 0)
			{
				dcs_log(0, 0, "SK program ");	
				s = GetCustInfo(pep_jnls.termid, pep_jnls.samid, &pos_cust_info);
				if(s < 0)
				{
					dcs_log(0, 0, "get pos_cust_info error");	
					memcpy(buffer+len, "               ", 15);
					len += 15; 
					memcpy(buffer+len, "        ", 8);
					len += 8; 
					buffer[len] = 0;
					len +=1;
				}
				else
				{
					memcpy(buffer+len, pos_cust_info.cust_merid, 15);
					len += 15; 
					memcpy(buffer+len, pos_cust_info.cust_termid, 8);
					len += 8; 
					merlen = getstrlen(pos_cust_info.merdescr);
					buffer[len] = merlen;
					len +=1;
					memcpy(buffer+len, pos_cust_info.merdescr, merlen);
					len += merlen; 
				}
			}
			
			/*IC卡数据域,转账汇款下发，信用卡还款下发 上线修改*/
			if((memcmp(ewp_info.consumer_transdealcode,"480000",6)==0 || memcmp(ewp_info.consumer_transdealcode,"190000",6)==0 )
			&&  memcmp(ewp_info.poitcde_flag, "0", 1)!=0)
			{	
				s = getbit(&siso, 55,(unsigned char *)tmpbuf);
				if(s <= 0)
		 		{
		 			dcs_log(0, 0, "bit55 is null!");
					memcpy(buffer+len, "000", 3);
					len +=3;
		 		}	
				else
				{
					bcd_to_asc((unsigned char *)temp_asc_55, (unsigned char *)tmpbuf, 2*s, 0);
					memset(tmpbuf, 0x00, sizeof(tmpbuf));
					sprintf(tmpbuf, "%03d%s", 2*s, temp_asc_55);
					memcpy(buffer+len, tmpbuf, 2*s+3);
					len +=2*s+3;
				}
			}
			/*
			else if(memcmp(ewp_info.consumer_transdealcode,"190000",6)==0 && memcmp(ewp_info.poitcde_flag, "0", 1)!=0)
			{
				len = strlen(buffer);
				memcpy(buffer+len, "000", 3);
				len +=3;
			}
			*/
			/*存量机器除外，都需检查并下发IC卡参数公钥更新标识，1表示需要处理，0表示无需处理 */
			if(memcmp(ewp_info.poitcde_flag, "0", 1)!=0 )
			{
				sprintf(buffer+len,"%s%s",ewp_info.download_pubkey_flag, ewp_info.download_para_flag);
				len +=2;
			}
			/*20个字节的自定义域原样返回给EWP*/
			memcpy(buffer+len,ewp_info.selfdefine, getstrlen(ewp_info.selfdefine));
			len +=20;
			return len;		 
		
		}
		
		/*查询交易*/
		else if(memcmp(ewp_info.consumer_transdealcode,"9F0000",6)==0)
		{
			dcs_log(0,0,"查询交易.");
			/*2个字节前置系统的交易应答码??*/
			sprintf( buffer,"%s%s",buffer,pep_jnls.aprespn);
			/*6个字节  前置系统交易流水trance */
			sprintf( buffer,"%s%06d",buffer,pep_jnls.trace);
			/*12个字节前置系统返回的交易参考号??*/
			if(getstrlen(pep_jnls.syseqno)==0)
				sprintf( buffer,"%s%012d",buffer, 0);
			else
				sprintf( buffer,"%s%12s",buffer,pep_jnls.syseqno);		
			/*6个字节 授权码*/
			if(getstrlen(pep_jnls.authcode) == 0)
				sprintf( buffer,"%s%s",buffer,"000000");
			else
				sprintf( buffer,"%s%s",buffer,pep_jnls.authcode);
			dcs_log(0,0,"pep_jnls.authcode = [%s]",pep_jnls.authcode);
			len = strlen(buffer);	
			
			buffer[len] = ewpinfo_len;
			len +=1;
			memcpy(buffer+len, ewpinfo, ewpinfo_len);
			len +=ewpinfo_len;
			
			if(memcmp(pep_jnls.sendcde, "0400", 4) == 0 || memcmp(pep_jnls.sendcde, "0800", 4) == 0 )
			{
				dcs_log(0, 0, "SK program ");	
				s = GetCustInfo(pep_jnls.termid, pep_jnls.samid, &pos_cust_info);
				if(s < 0)
				{
					dcs_log(0, 0, "get pos_cust_info error");	
					memcpy(buffer+len, "               ", 15);
					len += 15; 
					memcpy(buffer+len, "        ", 8);
					len += 8; 
					buffer[len] = 0;
					len +=1;
				}
				else
				{
					memcpy(buffer+len, pos_cust_info.cust_merid, 15);
					len += 15; 
					memcpy(buffer+len, pos_cust_info.cust_termid, 8);
					len += 8; 
					merlen = getstrlen(pos_cust_info.merdescr);
					buffer[len] = merlen;
					len +=1;
					memcpy(buffer+len, pos_cust_info.merdescr, merlen);
					len += merlen; 
				}
			}
			
			/*20个字节的自定义域原样返回给EWP*/
			memcpy(buffer+len,ewp_info.selfdefine, getstrlen(ewp_info.selfdefine));
			len +=20;
			dcs_log(0, 0, "len= [%d]", len);
			return len;			
				
		}
		/*消费类的交易*/
		else if(memcmp(ewp_info.consumer_transtype, "0620", 4)!=0
			 && memcmp(ewp_info.consumer_transdealcode,"000000",6)==0
			 && memcmp(ewp_info.consumer_transtype, "0400", 4)!=0)
		{
			dcs_log(0,0,"消费类的交易.ewp_info.poitcde_flag：%s", ewp_info.poitcde_flag);
			/*12个字节 金额，右对齐 不足补0*/
			sprintf(buffer,"%s%012s",buffer,pep_jnls.tranamt);
			/*2个字节前置系统的交易应答码*/
			sprintf( buffer,"%s%s",buffer,pep_jnls.aprespn);
			/*6个字节  前置系统交易流水trance */
			sprintf( buffer,"%s%06d",buffer,pep_jnls.trace);
			/*12个字节前置系统返回的交易参考号??*/
			if(getstrlen(pep_jnls.syseqno)==0)
				sprintf( buffer,"%s%012d",buffer, 0);
			else
				sprintf( buffer,"%s%12s",buffer,pep_jnls.syseqno);		
			/*6个字节 授权码*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			if( getbit( &siso,38,(unsigned char *)tmpbuf) <= 0 )
				sprintf( buffer,"%s%s",buffer,"000000");
			else
				sprintf( buffer,"%s%6s",buffer,tmpbuf);
			dcs_log(0,0,"pep_jnls.authcode = [%s]",tmpbuf);	
		
			len = strlen(buffer);
			buffer[len] = ewpinfo_len;
			len +=1;
			
			memcpy(buffer+len, ewpinfo, ewpinfo_len);
			len +=ewpinfo_len;
			
			/*如果是收款交易就返回EWP收单商户号，收单终端号，商户名称*/
			if( memcmp(pep_jnls.sendcde, "0400", 4) == 0 || memcmp(pep_jnls.sendcde, "0500", 4) == 0 
			 || memcmp(pep_jnls.sendcde, "0800", 4) == 0 )
			{						
				s = GetCustInfo(pep_jnls.termid, pep_jnls.samid, &pos_cust_info);
				if(s < 0)
				{
					dcs_log(0, 0, "get pos_cust_info error");	
					memcpy(buffer+len, "               ", 15);
					len += 15; 
					memcpy(buffer+len, "        ", 8);
					len += 8; 
					buffer[len] = 0;
					len +=1;
				}
				else
				{
					memcpy(buffer+len, pos_cust_info.cust_merid, 15);
					len += 15; 
					memcpy(buffer+len, pos_cust_info.cust_termid, 8);
					len += 8; 
					merlen = getstrlen(pos_cust_info.merdescr);
					buffer[len] = merlen;
					len +=1;
					memcpy(buffer+len, pos_cust_info.merdescr, merlen);
					len += merlen; 	
				}	
			}
			
			/*IC卡数据域*/
			if(memcmp(ewp_info.poitcde_flag, "0", 1)!=0)
			{	
				s = getbit(&siso, 55,(unsigned char *)tmpbuf);
				if(s <= 0)
		 		{
		 			dcs_log(0, 0, "bit55 is null!");
					memcpy(buffer+len, "000", 3);
					len +=3;
		 		}	
				else
				{
					bcd_to_asc((unsigned char *)temp_asc_55, (unsigned char *)tmpbuf, 2*s, 0);
					memset(tmpbuf, 0x00, sizeof(tmpbuf));
					sprintf(tmpbuf, "%03d%s", 2*s, temp_asc_55);
					memcpy(buffer+len, tmpbuf, 2*s+3);
					len +=2*s+3;
				}
			
				/*存量机器除外，都需检查并下发IC卡参数公钥更新标识，1表示需要处理，0表示无需处理 */
				sprintf(buffer+len,"%s%s",ewp_info.download_pubkey_flag, ewp_info.download_para_flag);
				len +=2;
			}
			
			/*20个字节的自定义域原样返回给EWP*/
			memcpy(buffer+len,ewp_info.selfdefine, getstrlen(ewp_info.selfdefine));
			len +=20;
			return len;							
		}
		/*余额查询类的交易*/
		else if(memcmp(ewp_info.consumer_transtype, "0620", 4)!=0
			 && memcmp(ewp_info.consumer_transdealcode,"310000",6)==0)
		{
			dcs_log(0,0,"余额查询类的交易.ewp_info.poitcde_flag:%s",ewp_info.poitcde_flag);
			
			/*2个字节  前置系统的交易应答码*/
			sprintf( buffer,"%s%s",buffer,pep_jnls.aprespn);
			
			/*6个字节  前置系统交易流水*/
			sprintf( buffer,"%s%06d",buffer,pep_jnls.trace);
			
			/*12个字节 前置系统返回的交易参考号*/
			if(getstrlen(pep_jnls.syseqno)==0)
				sprintf( buffer,"%s%012d",buffer, 0);
			else
				sprintf( buffer,"%s%12s",buffer,pep_jnls.syseqno);	
			
			memset(buf,0x00,sizeof(buf));
			/*应答码为00成功时，才返回金额。否则全为0*/
			/*24个字节 金额(账面金额和可用金额)，右对齐 不足补空格*/
			if(memcmp(pep_jnls.aprespn,"00",2)==0)
			{
				memset(tmpbuf,0,sizeof(tmpbuf));
				s = getbit(	&siso, 54, (unsigned char *)tmpbuf);
				if( s <= 0 )
				{
					memcpy(buf,"000",3);
					PubAddSymbolToStr(buf,24,'0',0);
					sprintf(buffer, "%s%s", buffer, buf);
				}	
				else if(s == 20)
				{
					memcpy(buf,"000",3);
					PubAddSymbolToStr(buf,12,'0',0);
					sprintf(buffer,"%s%12s%12s", buffer, tmpbuf+8,buf);
				}	
				else if(s == 40)
					sprintf(buffer,"%s%12s%12s", buffer, tmpbuf+8, tmpbuf+28);
			}
			else
			{
				memcpy(buf,"000",3);
				PubAddSymbolToStr(buf,24,'0',0);
				sprintf(buffer,"%s%24s",buffer,buf);
			}
			
			/*6个字节 授权码*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			if( getbit(	&siso,38,(unsigned char *)tmpbuf) <= 0 )
				sprintf( buffer,"%s%s",buffer,"000000");
			else
				sprintf( buffer,"%s%6s",buffer,tmpbuf);
			dcs_log(0,0,"pep_jnls.authcode = [%s]",tmpbuf);	
			len = strlen(buffer);	
			
			buffer[len] = ewpinfo_len;
			len +=1;
			memcpy(buffer+len, ewpinfo, ewpinfo_len);
			len +=ewpinfo_len;
			
			dcs_log(0, 0, "ewpinfo[%s]", ewpinfo);
			/*SK EWP 项目*/
			if(memcmp(pep_jnls.sendcde, "0400", 4) == 0 || memcmp(pep_jnls.sendcde, "0800", 4) == 0)
			{
				if(memcmp(pep_jnls.sendcde, "0400", 4) == 0)
					dcs_log(0, 0, "SK program ");
				if(memcmp(pep_jnls.sendcde, "0800", 4) == 0)
					dcs_log(0, 0, "盛京银行 program ");
				s = GetCustInfo(pep_jnls.termid, pep_jnls.samid, &pos_cust_info);
				if(s < 0)
				{
					dcs_log(0, 0, "get pos_cust_info error");	
					memcpy(buffer+len, "               ", 15);
					len += 15; 
					memcpy(buffer+len, "        ", 8);
					len += 8; 
					buffer[len] = 0;
					len +=1;
				}
				else
				{
					dcs_log(0, 0, "[%s][%s][%s]",pos_cust_info.cust_merid, pos_cust_info.cust_termid, pos_cust_info.merdescr);
				  	memcpy(buffer+len, pos_cust_info.cust_merid, 15);
				 	len += 15; 
				 	memcpy(buffer+len, pos_cust_info.cust_termid, 8);
					len += 8; 
					merlen = getstrlen(pos_cust_info.merdescr);
					buffer[len] = merlen;
					len +=1;
					memcpy(buffer+len, pos_cust_info.merdescr, merlen);
					len += merlen; 
				}
			}

			/*IC卡数据域*/
			if(memcmp(ewp_info.poitcde_flag, "0", 1)!=0)
			{	
				s = getbit(&siso, 55,(unsigned char *)tmpbuf);
				if(s <= 0)
		 		{
		 			dcs_log(0, 0, "bit55 is null!");
					memcpy(buffer+len, "000", 3);
					len +=3;
		 		}	
				else
				{
					bcd_to_asc((unsigned char *)temp_asc_55, (unsigned char *)tmpbuf, 2*s, 0);
					memset(tmpbuf, 0x00, sizeof(tmpbuf));
					sprintf(tmpbuf, "%03d%s", 2*s, temp_asc_55);
					memcpy(buffer+len, tmpbuf, 2*s+3);
					len +=2*s+3;
				}

				/*存量机器除外，都需检查并下发IC卡参数公钥更新标识，1表示需要处理，0表示无需处理 */
				sprintf(buffer+len,"%s%s",ewp_info.download_pubkey_flag, ewp_info.download_para_flag);
				len +=2;
			}
			
			/*20个字节的自定义域原样返回给EWP*/
			memcpy(buffer+len, ewp_info.selfdefine, getstrlen(ewp_info.selfdefine));
			len +=20;
			return len;				
		}
		/*消费撤销 订单撤销：200000*/
		else if(memcmp(ewp_info.consumer_transtype, "0620", 4)!=0
			 && memcmp(ewp_info.consumer_transdealcode,"200000",6)==0)
		{
			dcs_log(0,0,"消费撤销类的交易.");
			/*2个字节前置系统的交易应答码??*/
			sprintf( buffer,"%s%s",buffer,pep_jnls.aprespn);
			/*6个字节  前置系统交易流水trance */
			sprintf( buffer,"%s%06d",buffer,pep_jnls.trace);
			/*12个字节前置系统返回的交易参考号??*/
			if(getstrlen(pep_jnls.syseqno)==0)
				sprintf( buffer,"%s%012d",buffer, 0);
			else
				sprintf( buffer,"%s%12s",buffer,pep_jnls.syseqno);		
			/*6个字节 授权码*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			if( getbit(	&siso,38,(unsigned char *)tmpbuf) <= 0 )
				sprintf( buffer,"%s%s",buffer,"000000");
			else
				sprintf( buffer,"%s%6s",buffer,tmpbuf);
			dcs_log(0,0,"pep_jnls.authcode = [%s]",tmpbuf);	
			len = strlen(buffer);
			//add for sj begin
			if(memcmp(pep_jnls.sendcde, "0800", 4) == 0)
			{
				s = GetCustInfo(pep_jnls.termid, pep_jnls.samid, &pos_cust_info);
				if(s < 0)
				{
					dcs_log(0, 0, "get pos_cust_info error");	
					memcpy(buffer+len, "               ", 15);
					len += 15; 
					memcpy(buffer+len, "        ", 8);
					len += 8; 
					buffer[len] = 0;
					len +=1;
				}
				else
				{
					memcpy(buffer+len, pos_cust_info.cust_merid, 15);
					len += 15; 
					memcpy(buffer+len, pos_cust_info.cust_termid, 8);
					len += 8; 
					merlen = getstrlen(pos_cust_info.merdescr);
					buffer[len] = merlen;
					len +=1;
					memcpy(buffer+len, pos_cust_info.merdescr, merlen);
					len += merlen; 	
				}	
				
				sprintf(buffer+len, "%s%s", ewp_info.download_pubkey_flag, ewp_info.download_para_flag);
				len +=2;
				/*20个字节的自定义域原样返回给EWP*/
				memcpy(buffer+len, ewp_info.selfdefine, getstrlen(ewp_info.selfdefine));
				len +=20;
				//add for sj end
			}
			
			return len;
		}		
		/*消费退货 订单退货：270000*/
		else if(memcmp(ewp_info.consumer_transtype, "0620", 4)!=0
			 && memcmp(ewp_info.consumer_transdealcode,"270000",6)==0)
		{
			dcs_log(0,0,"消费退货的交易.");
		
			/*12个字节 退货金额，右对齐 不足补0*/
			sprintf(buffer,"%s%012s",buffer,pep_jnls.tranamt);
			/*2个字节前置系统的交易应答码??*/
			sprintf( buffer,"%s%s",buffer,pep_jnls.aprespn);
			/*6个字节  前置系统交易流水trance */
			sprintf( buffer,"%s%06d",buffer,pep_jnls.trace);
			/*12个字节前置系统返回的交易参考号??*/
			if(getstrlen(pep_jnls.syseqno)==0)
				sprintf( buffer,"%s%012d",buffer, 0);
			else
				sprintf( buffer,"%s%12s",buffer,pep_jnls.syseqno);			
			/*6个字节 授权码*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			if( getbit(	&siso,38,(unsigned char *)tmpbuf) <= 0 )
				sprintf( buffer,"%s%s",buffer,"000000");
			else
				sprintf( buffer,"%s%6s",buffer,tmpbuf);
			dcs_log(0,0,"pep_jnls.authcode = [%s]",tmpbuf);	
			len = strlen(buffer);
			//add for sj begin
			if(memcmp(pep_jnls.sendcde, "0800", 4) == 0)
			{
				s = GetCustInfo(pep_jnls.termid, pep_jnls.samid, &pos_cust_info);
				if(s < 0)
				{
					dcs_log(0, 0, "get pos_cust_info error");	
					memcpy(buffer+len, "               ", 15);
					len += 15; 
					memcpy(buffer+len, "        ", 8);
					len += 8; 
					buffer[len] = 0;
					len +=1;
				}
				else
				{
					memcpy(buffer+len, pos_cust_info.cust_merid, 15);
					len += 15; 
					memcpy(buffer+len, pos_cust_info.cust_termid, 8);
					len += 8; 
					
					merlen = getstrlen(pos_cust_info.merdescr);
					buffer[len] = merlen;
					len +=1;
					memcpy(buffer+len, pos_cust_info.merdescr, merlen);
					len += merlen; 	
				}	
				
				sprintf(buffer+len, "%s%s", ewp_info.download_pubkey_flag, ewp_info.download_para_flag);
				len +=2;
				
				/*20个字节的自定义域原样返回给EWP*/
				memcpy(buffer+len, ewp_info.selfdefine, getstrlen(ewp_info.selfdefine));
				len +=20;
				//add for sj end
			}	
			return len;	
		}
		/*脚本通知  0620*/
		else if(memcmp(ewp_info.consumer_transtype, "0620", 4)==0)
		{
			sprintf( buffer,"%s%s", buffer, pep_jnls.aprespn);
			len =strlen(buffer);
			buffer[len] = ewpinfo_len;
			len +=1;
			memcpy(buffer+len, ewpinfo, ewpinfo_len);
			len +=ewpinfo_len;
			
			/*IC卡数据域*/
			if(memcmp(ewp_info.poitcde_flag, "0", 1)!=0)
			{	
				s = getbit(&siso, 55,(unsigned char *)tmpbuf);
				if(s <= 0)
		 		{
		 			dcs_log(0, 0, "bit55 is null!");
					memcpy(buffer+len, "000", 3);
					len +=3;
		 		}	
				else
				{
					bcd_to_asc((unsigned char *)temp_asc_55, (unsigned char *)tmpbuf, 2*s, 0);
					memset(tmpbuf, 0x00, sizeof(tmpbuf));
					sprintf(tmpbuf, "%03d%s", 2*s, temp_asc_55);
					memcpy(buffer+len, tmpbuf, 2*s+3);
					len +=2*s+3;
				}
			}
			
			/*20个字节的自定义域原样返回给EWP
			memcpy(buffer+len, ewp_info.selfdefine, getstrlen(ewp_info.selfdefine));
			len +=20;
			*/
			return len;	
		}
		/*消费冲正 0410 000000*/
		else if(memcmp(pep_jnls.msgtype, "0400", 4)==0)
		{
			dcs_log(0,0,"消费冲正类交易");
			/*2个字节前置系统的交易应答码*/
			sprintf( buffer,"%s%s",buffer,pep_jnls.aprespn);
			len = len+2;
		}
	}
	
	return len;
}

/*金额限制的处理*/
int AmoutLimitDeal(char *camtlimit, char *damtlimit, char *otransamt, char outcdtype)
{
	int limitc = 0, limitd = 0;
	
	limitc = getstrlen(camtlimit);
	limitd = getstrlen(damtlimit);
	
	#ifdef __TEST__
		dcs_log(0, 0, "limitc = [%d]", limitc);
		dcs_log(0, 0, "limitd = [%d]", limitd);
		dcs_log(0, 0, "camtlimit = [%s]", camtlimit);
		dcs_log(0, 0, "damtlimit = [%s]", damtlimit);
		dcs_log(0, 0, "otransamt = [%s]", otransamt);
		dcs_log(0, 0, "outcdtype = [%c]", outcdtype);
	#endif
	
	if(limitc == 1 && limitd == 1)
		return 0;
		
	if(outcdtype == 'C')/*信用卡的单笔交易限额*/
	{
		if(limitc == 1) return 0;
		else if(limitc != 1 && memcmp(camtlimit, otransamt, 12) >= 0)
			return 0;
		
	}
	else if(outcdtype == 'D') /*借记卡的单笔交易限额*/
	{
		if(limitd == 1) return 0;
		else if(limitd != 1 && memcmp(damtlimit, otransamt, 12) >= 0)
			return 0;
	}
	else if(outcdtype == '-') /*未知卡的单笔交易限额*/
	{	
		if(limitc == 1 && limitd != 1)
		{
			if(memcmp(damtlimit, otransamt, 12) >= 0) return 0;
			else return -1;
		}
		else if(limitc != 1 && limitd == 1)
		{
			if(memcmp(camtlimit, otransamt, 12) >= 0) return 0;
			else return -1;
		}
		else if(limitc != 1 && limitd != 1)
		{
			if(memcmp(camtlimit, damtlimit, 12) >= 0)
			{
				if(memcmp(damtlimit, otransamt, 12) >= 0) return 0;
			}
			else 
			{
				if(memcmp(camtlimit, otransamt, 12) >= 0) return 0;
			}
		}
	}
	return -1;
}
/*十进制大数求和*/
int SumAmt(char *curamt, char *tranamt, char *totalamt)
{
	int a[13]={0}, b[13]={0}, len1, len2, c, k, i;
	char tmpbuf[13];
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	if(totalamt==NULL)
	{
		#ifdef __LOG__	
  			dcs_log(0, 0, "error totalamt");
  		#endif
		return -1;
	}
	if(curamt==NULL&&tranamt==NULL)
	{
		#ifdef __LOG__	
  			dcs_log(0, 0, "error param");
  		#endif
		return -1;
	}
	else if(curamt==NULL&&tranamt!=NULL)
	{
		len1 = strlen(tranamt);
		if(strlen(totalamt)>=len1)
		{
			sprintf(tmpbuf, "%012ld", atol(tranamt));
			memset(totalamt, 0, strlen(totalamt));
			memcpy(totalamt, tmpbuf, 12);
			#ifdef __LOG__	
  				dcs_log(0, 0, "totalamt=[%s]", totalamt);
  			#endif
			return 0;
		}
		return -1;
	}
	else if(curamt!=NULL&&tranamt==NULL)
	{
		len1 = strlen(curamt);
		if(strlen(totalamt)>=len1)
		{
			sprintf(tmpbuf, "%012ld", atol(curamt));
			memset(totalamt, 0, strlen(totalamt));
			memcpy(totalamt, tmpbuf, 12);
			#ifdef __LOG__	
  				dcs_log(0, 0, "totalamt=[%s]", totalamt);
  			#endif
			return 0;
		}
		return -1;
	}
	
	len1 = strlen(curamt);
	len2 = strlen(tranamt);
	if(len1<len2)
		k = len2;
	else 
		k = len1;
	c = k;
	for(i = 0; i < len1; k--, i++)
		a[k-1] = curamt[len1 - 1 -i] - '0';
	for(k = c, i = 0; i<len2; k--, i++)
		b[k-1] = tranamt[len2 - 1- i] - '0';
	for(i =c-1; i>=0; i--)
	{
		a[i] +=b[i];
		if(a[i] >= 10)
		{
			a[i] -= 10;
			a[i-1]++;
		}
		tmpbuf[i]= a[i]+'0';
	}
	if(strlen(totalamt)<12)
	{
		#ifdef __LOG__	
  			dcs_log(0, 0, "len short");
  		#endif
		return -1;
	}
	memset(totalamt, 0, strlen(totalamt));
	sprintf(totalamt, "%012ld", atol(tmpbuf));
	totalamt[12]= 0;
	#ifdef __LOG__	
  		dcs_log(0, 0, "totalamt=[%s]", totalamt);
  	#endif
	return 0;
}

/*
十进制大数相减
curamt :减数
tranamt:被减数
*/
int DecAmt(char *curamt, char *tranamt, char *totalamt)
{
	int a[14]={0}, b[14]={0}, len1, len2, c, k, i;
	char tmpbuf1[14];
	char tmpbuf2[14];
	char tmpbuf[14];

	memset(tmpbuf1, 0, sizeof(tmpbuf1));
	memset(tmpbuf2, 0, sizeof(tmpbuf2));
	memset(tmpbuf, 0, sizeof(tmpbuf));
	if(totalamt==NULL||curamt==NULL||tranamt==NULL)
	{
		#ifdef __LOG__	
  			dcs_log(0, 0, "error param");
  		#endif
		return -1;
	}
	sprintf(tmpbuf1, "%012ld", atol(curamt));
	tmpbuf1[13] =0;
	sprintf(tmpbuf2, "%012ld", atol(tranamt));
	tmpbuf2[13] =0;

	len1 = strlen(curamt);
	len2 = strlen(tranamt);
	if(len1<len2)
		k = len2;
	else 
		k = len1;
	c = k;
	if(strcmp(tmpbuf1, tmpbuf2) < 0)
	{
		for(i = 0; i < len1; k--, i++)
			b[k] = curamt[len1 - 1 -i] - '0';
		for(k = c, i = 0; i<len2; k--, i++)
			a[k] = tranamt[len2 - 1- i] - '0';
	}
	else
	{
		for(i = 0; i < len1; k--, i++)
			a[k] = curamt[len1 - 1 -i] - '0';
		for(k = c, i = 0; i<len2; k--, i++)
			b[k] = tranamt[len2 - 1- i] - '0';
	}
	for(i =c; i>=0; i--)
	{
		a[i] -=b[i];
		if(a[i]<0)
		{
			a[i] +=10;
			a[i-1]--;
		}
		tmpbuf[i]= a[i]+'0';
	}
	
	if(strlen(totalamt)<13)
	{
		#ifdef __LOG__	
  			dcs_log(0, 0, "len short");
  		#endif
		return -1;
	}
	if(strcmp(tmpbuf1, tmpbuf2) >= 0)
		memcpy(totalamt, "+", 1);
	else
		memcpy(totalamt, "-", 1);
	sprintf(totalamt+1, "%012ld", atol(tmpbuf+1));
	totalamt[13] =0;
	#ifdef __LOG__	
  		dcs_log(0, 0, "totalamt=[%s]", totalamt);
  	#endif
	return 0;
}

/*解析多媒体终端密钥下载报文*/
int Resolve(char *rcvbuf, int rcvlen, EWP_INFO *ewp_info)
{
	int check_len, offset;
	char tmpbuf[1024];

	check_len = 0;
	offset = 0;
	check_len = rcvlen;

	/*交易类型*/
	memcpy(ewp_info->consumer_transtype, rcvbuf + offset, 4);
	ewp_info->consumer_transtype[4] = 0;
	offset += 4;
	check_len -= 4;
	
	/*发送机构号*/
	memcpy(ewp_info->consumer_sentinstitu, rcvbuf + offset, 4);
	ewp_info->consumer_sentinstitu[4] = 0;
	offset += 4;
	check_len -= 4;
	
	#ifdef __LOG__
		dcs_log(0, 0, "consumer_transtype=[%s], consumer_sentinstitu=[%s]", ewp_info->consumer_transtype, ewp_info->consumer_sentinstitu);
	#endif
	
	/*发送日期*/
	memcpy(ewp_info->consumer_sentdate, rcvbuf + offset, 8);
	ewp_info->consumer_sentdate[8] = 0;
	offset += 8;
	check_len -= 8;
	
	/*发送时间*/
	memcpy(ewp_info->consumer_senttime, rcvbuf + offset, 6);
	ewp_info->consumer_senttime[6] = 0;
	offset += 6;
	check_len -= 6;
	
	#ifdef __LOG__
		dcs_log(0,0,"consumer_sentdate=[%s], consumer_senttime=[%s]", ewp_info->consumer_sentdate, ewp_info->consumer_senttime);
	#endif
	
	
	/*机具编号 20字节*/
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memcpy(tmpbuf, rcvbuf + offset, 20);
	memcpy(ewp_info->consumer_phonesnno, tmpbuf, getstrlen(tmpbuf));
	ewp_info->consumer_phonesnno[getstrlen(tmpbuf)]=0;
	offset += 20;
	check_len -= 20;
	
	/*用户名*/
	memcpy(ewp_info->consumer_username, rcvbuf + offset, 11);
	ewp_info->consumer_username[11] = 0;
	offset += 11;
	check_len -= 11;
	
	#ifdef __LOG__
		dcs_log(0, 0, "consumer_phonesnno=[%s], consumer_username=[%s]", ewp_info->consumer_phonesnno, ewp_info->consumer_username);
	#endif
	
	/*20个字节的自定义域*/
	memcpy(ewp_info->selfdefine, rcvbuf + offset, 20);
	ewp_info->selfdefine[20] = 0;
	offset += 20;
	check_len -= 20;
		
	#ifdef __LOG__
		dcs_log(0, 0, "selfdefine=[%s]", ewp_info->selfdefine);
	#endif
		
	if(check_len != 0 )
		strcpy(ewp_info->retcode, "F1");
	else
	{
		strcpy(ewp_info->retcode, "00");
		ewp_info->retcode[2] = 0;
	}
	if(	memcmp(ewp_info->retcode, "00", 2) == 0)
		dcs_log(0, 0, "EWP数据解析成功, retcode=[%s]", ewp_info->retcode);
	else
		dcs_log(0, 0, "EWP数据不正确, 直接丢弃该包, retcode=[%s]", ewp_info->retcode);
	return 0;
}

	
	
int EwpSendTCvalue(EWP_INFO ewp_info, PEP_JNLS pep_jnls, MSGCHGINFO msginfo, ISO_data siso, SECUINFO secuinfo, POS_CUST_INFO pos_cust_info)
{
	int len = 0;
	int rtn = -1;
	char oldtrace[7]; //原交易流水号
	char temp_bcd_55[512];
	char tmpbuf[1024];
	
	memset(oldtrace, 0x00, sizeof(oldtrace));
	memset(temp_bcd_55, 0x00, sizeof(temp_bcd_55));
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	
	setbit(&siso, 0, (unsigned char *)"0320", 2);
	setbit(&siso, 2, (unsigned char *)pep_jnls.outcdno, strlen(pep_jnls.outcdno));
	setbit(&siso, 3, (unsigned char *)msginfo.process, 6);
	
	/*余额查询不需要上送4域*/
	if(memcmp(pep_jnls.process, "310000", 6)!=0)
	{
		setbit(&siso, 4, (unsigned char *)pep_jnls.tranamt, 12);
	}
	else 
	{
		setbit(&siso, 4, (unsigned char *)NULL, 0);
	}
	
	sprintf(oldtrace, "%06d", pep_jnls.trace);
	setbit(&siso, 11, (unsigned char *)oldtrace, 6);
	
	if(getstrlen(ewp_info.translaunchway) == 0)
	{
		memcpy(ewp_info.translaunchway, "01", 2);
	}
	
	sprintf(tmpbuf, "%s%-15s%s%s%s%06s%04d", ewp_info.consumer_psamno, ewp_info.consumer_username, ewp_info.translaunchway+1, "01", ewp_info.consumer_transcode, "000000", secuinfo.channel_id);
	
#ifdef __LOG__
	dcs_log(0, 0, "21域  length[%d]  content[%s]", strlen(tmpbuf), tmpbuf);
#endif
  	if(getstrlen(pos_cust_info.T0_flag)==0)
    {
    	memcpy(pos_cust_info.T0_flag, "0", 1);
	}
	
	if(getstrlen(pos_cust_info.agents_code)==0)
  	{
  		memcpy(pos_cust_info.agents_code,"000000",6);
 	}
 	
 	sprintf(tmpbuf+47, "%s", pos_cust_info.T0_flag);
  	sprintf(tmpbuf+48, "%s", pos_cust_info.agents_code);
	setbit(&siso, 21, (unsigned char *)tmpbuf, 54);
	
	setbit(&siso, 22, (unsigned char *)pep_jnls.poitcde, 3);
	setbit(&siso, 23, (unsigned char *)pep_jnls.card_sn, 3);
	setbit(&siso, 25, (unsigned char *)msginfo.service, 2);	
	setbit(&siso, 41, (unsigned char *)pep_jnls.termcde, 8);
	setbit(&siso, 42, (unsigned char *)pep_jnls.mercode, 15);

	if(memcmp(ewp_info.filed55_length, "000", 3) != 0)
	{
		len = atoi(ewp_info.filed55_length);
		memset(temp_bcd_55, 0x00, sizeof(temp_bcd_55));
		asc_to_bcd((unsigned char *)temp_bcd_55, (unsigned char*)ewp_info.filed55, len, 0);
		setbit(&siso, 55, (unsigned char *)temp_bcd_55, len/2);
	}
	
	//授权码固定填写000000，核心不需要此字段modify at 20141203
	memset(tmpbuf, 0, sizeof(tmpbuf));
	sprintf(tmpbuf,"%s%s%s%06d", "000000", pep_jnls.syseqno, pep_jnls.revdate, pep_jnls.trace);
	setbit(&siso, 61, (unsigned char *)tmpbuf, 34);
	
	/*发送报文*/
	rtn=WriteXpe(siso);
	if(rtn == -1)
	{
		dcs_log(0,0,"发送信息给核心失败");
		return -1;
	}
	
	return 0;

}

/*拆包 撤销和退货*/
int UnpackResj(char *rcvbuf, int check_len, EWP_INFO *ewp_info)
{
	int offset = 0;
	char exist_track_flag[2], exist_pwd_flag[2];
	char tmpbuf[1024];
	
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	memset(exist_track_flag, 0x00, sizeof(exist_track_flag));
	memset(exist_pwd_flag, 0x00, sizeof(exist_pwd_flag));
	
	/*交易发起方式 改为2个字节*/
	memcpy(ewp_info->translaunchway, rcvbuf, 2);
	ewp_info->translaunchway[2] = 0;
	offset += 2;
	check_len -= 2;
		
	#ifdef __LOG__
		dcs_log(0, 0, "translaunchway=[%s]", ewp_info->translaunchway);
	#endif
	
	/*psam编号*/
	memcpy(ewp_info->consumer_psamno, rcvbuf + offset, 16);
	ewp_info->consumer_psamno[16] = 0;
	offset += 16;
	check_len -= 16;
	
	/*机具编号手机编号 20字节*/
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memcpy(tmpbuf, rcvbuf + offset, 20);
	tmpbuf[20] = 0;
	pub_rtrim_lp(tmpbuf, 20, tmpbuf, 1);

	memcpy(ewp_info->consumer_phonesnno, tmpbuf, strlen(tmpbuf));
	ewp_info->consumer_phonesnno[strlen(tmpbuf)]=0;
	offset += 20;
	check_len -= 20;
	
	#ifdef __LOG__
		dcs_log(0, 0, "consumer_psamno=[%s], consumer_phonesnno=[%s]", ewp_info->consumer_psamno, ewp_info->consumer_phonesnno);
	#endif
	
	/*用户名 11字节*/
	memcpy(ewp_info->consumer_username, rcvbuf + offset, 11);
	ewp_info->consumer_username[11] = 0;
	offset += 11;
	check_len -= 11;
		
	/*订单号 20字节*/
	memset(tmpbuf,0,sizeof(tmpbuf));
	memcpy(tmpbuf, rcvbuf + offset, 20);
	tmpbuf[20] = 0;
	pub_rtrim_lp(tmpbuf, 20, tmpbuf,1);
	memcpy(ewp_info->consumer_orderno, tmpbuf, strlen(tmpbuf));
	ewp_info->consumer_orderno[strlen(tmpbuf)]=0;
	offset += 20;
	check_len -= 20;
		
	#ifdef __LOG__
		dcs_log(0, 0, "consumer_username=[%s], consumer_orderno=[%s]", ewp_info->consumer_username, ewp_info->consumer_orderno);
	#endif
		
	/*商户编号*/
	memset(tmpbuf,0,sizeof(tmpbuf));
	memcpy(tmpbuf, rcvbuf + offset, 20);
	tmpbuf[20] = 0;
	pub_rtrim_lp(tmpbuf, 20, tmpbuf, 1);
	memcpy(ewp_info->consumer_merid, tmpbuf, strlen(tmpbuf));
	ewp_info->consumer_merid[strlen(tmpbuf)]=0;
	offset += 20;
	check_len -= 20;
		
	#ifdef __LOG__
		dcs_log(0, 0, "consumer_merid=[%s]", ewp_info->consumer_merid);
	#endif
	
	/*卡号 19个字节*/
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memcpy(tmpbuf, rcvbuf + offset, 19);
	tmpbuf[19] = 0;
	pub_rtrim_lp(tmpbuf, 19, tmpbuf, 1);
	memcpy(ewp_info->consumer_cardno, tmpbuf, strlen(tmpbuf));
	ewp_info->consumer_cardno[strlen(tmpbuf)]=0;
	offset += 19;
	check_len -= 19;
#ifdef __TEST__
	dcs_log(0, 0, "consumer_cardno=[%s]", ewp_info->consumer_cardno);
#endif
	/*磁道是否存在标识 1：存在 0：不存在*/
	memcpy(exist_track_flag, rcvbuf + offset, 1);
	offset += 1;
	check_len -= 1;
	dcs_log(0, 0, "exist_track_flag=[%s]", exist_track_flag);
	
	/*解析磁道 160个字节 磁道随机数16个字节*/
	if(memcmp(exist_track_flag, "1", 1)==0)
	{
		/*磁道160个字节 */
		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy(tmpbuf, rcvbuf + offset, 160);
		tmpbuf[160] = 0;
		pub_rtrim_lp(tmpbuf, 160, tmpbuf, 1);
		memcpy(ewp_info->consumer_trackno, tmpbuf, strlen(tmpbuf));
		ewp_info->consumer_trackno[strlen(tmpbuf)]=0;
		offset += 160;
		check_len -= 160;
		
		#ifdef __TEST__
			dcs_log(0, 0, "consumer_trackno=[%s]", ewp_info->consumer_trackno);
		#endif
		
		/*16个字节的磁道密钥随机数*/
		memcpy(ewp_info->consumer_track_randnum, rcvbuf + offset, 16);
		ewp_info->consumer_track_randnum[16] = 0;
		offset += 16;
		check_len -= 16;
		
		#ifdef __TEST__
			dcs_log(0, 0, "consumer_track_randnum=[%s]", ewp_info->consumer_track_randnum);
		#endif
	}
	
	/*密码存在标识 1：存在密码 0：不存在密码*/
	memcpy(exist_pwd_flag, rcvbuf + offset, 1);
	offset += 1;
	check_len -= 1;
	dcs_log(0, 0, "exist_pwd_flag=[%s]", exist_pwd_flag);
	
	/*密码标识存在时*/
	if(memcmp(exist_pwd_flag, "1", 1)==0)
	{
		/*密码*/
		memcpy(ewp_info->consumer_password, rcvbuf + offset, 16);
		ewp_info->consumer_password[16] = 0;
		offset += 16;
		check_len -= 16;
		
		/*16个字节的pin密钥随机数*/
		memcpy(ewp_info->consumer_pin_randnum, rcvbuf + offset, 16);
		ewp_info->consumer_pin_randnum[16] = 0;
		offset += 16;
		check_len -= 16;
		
		#ifdef __TEST__
			dcs_log(0, 0, "consumer_password=[%s], consumer_pin_randnum=[%s]", ewp_info->consumer_password, ewp_info->consumer_pin_randnum);
		#endif
	}
	
	if(memcmp(ewp_info->consumer_transcode, "210", 3)== 0||memcmp(ewp_info->consumer_transcode, "212", 3)== 0)
	{
		#ifdef __LOG__
			dcs_log(0, 0, "一体机退货交易");
		#endif
		memcpy(ewp_info->consumer_money, rcvbuf + offset, 12);
		ewp_info->consumer_money[12]=0;
		offset += 12;
		check_len -= 12;
		
		#ifdef __LOG__
			dcs_log(0, 0, "退货金额consumer_money=[%s]", ewp_info->consumer_money);
		#endif
		
	}
	else
		#ifdef __LOG__
			dcs_log(0, 0, "一体机撤销交易");
		#endif
	
	/* 终端类型  */
	memcpy(ewp_info->termtyp, rcvbuf + offset, 1);
	dcs_log(0, 0, "终端类型[%s]", ewp_info->termtyp);
	offset += 1;
	check_len -= 1;
			
	/* IC卡标识 */
	memcpy(ewp_info->icflag, rcvbuf + offset, 1);
#ifdef __LOG__
	dcs_log(0, 0, "IC卡标识[%s]", ewp_info->icflag);
#endif

	offset += 1;
	check_len -= 1;
			
	if(memcmp(ewp_info->icflag, "1", 1) == 0)
	{
		/*降级标志位fallback*/
		memcpy(ewp_info->fallback_flag, rcvbuf + offset, 1);
#ifdef __LOG__
		dcs_log(0, 0, "降级标志[%s]", ewp_info->fallback_flag);
#endif
		offset += 1;                                    
		check_len -= 1;
	}

	/* 卡片序列号存在标识  */
	memcpy(ewp_info->cardseq_flag, rcvbuf + offset, 1);
#ifdef __LOG__
	dcs_log(0, 0, "卡片序列号存在标识[%s]", ewp_info->cardseq_flag);
#endif

	offset += 1;
	check_len -= 1;
			
	if(memcmp(ewp_info->cardseq_flag, "1", 1)==0)
	{
		/* 卡片序列号 */
		memcpy(ewp_info->cardseq, rcvbuf + offset, 3);
#ifdef __LOG__
		dcs_log(0, 0, "卡片序列号[%s]", ewp_info->cardseq);
#endif
		offset += 3;                                    
		check_len -= 3;			
	}
			
	/*系统参考号*/
	memcpy(ewp_info->consumer_sysreferno, rcvbuf + offset, 12);
	ewp_info->consumer_sysreferno[12] = 0;
	offset += 12;
	check_len -= 12;
	
	/*授权码*/
	memcpy(ewp_info->authcode, rcvbuf + offset, 6);
	ewp_info->authcode[6] = 0;
	offset += 6;
	check_len -= 6;
		
	#ifdef __LOG__
		dcs_log(0, 0, "consumer_sysreferno=[%s], authcode=[%s]", ewp_info->consumer_sysreferno, ewp_info->authcode);
	#endif
		
	/*前置系统流水*/
	memcpy(ewp_info->consumer_presyswaterno, rcvbuf + offset, 6);
	ewp_info->consumer_presyswaterno[6] = 0;
	offset += 6;
	check_len -= 6;
	
	/*原笔交易发起日期和时间*/
	memcpy(ewp_info->pretranstime, rcvbuf + offset, 14);
	ewp_info->pretranstime[14] = 0;
	offset += 14;
	check_len -= 14;	
	#ifdef __LOG__
		dcs_log(0, 0, "consumer_presyswaterno=[%s], pretranstime=[%s]", ewp_info->consumer_presyswaterno, ewp_info->pretranstime);
	#endif	
	
	/*1个字节的是否需要发送短信的标记*/
	memcpy(ewp_info->modeflg, rcvbuf + offset, 1);
	ewp_info->modeflg[1] = 0;
	offset += 1;
	check_len -= 1;
	
	/*mac校验 8个字节*/
	memcpy(ewp_info->macinfo, rcvbuf + offset, 8);
	ewp_info->macinfo[8] = 0;
	offset += 8;
	check_len -= 8;
		
	#ifdef __TEST__
		dcs_log(0, 0, "modeflg=[%s], macinfo=[%s]", ewp_info->modeflg, ewp_info->macinfo);
	#endif
		
	if(memcmp(ewp_info->modeflg, "1", 1)==0)
	{
		/*11个字节的手机号*/
		memcpy(ewp_info->modecde, rcvbuf + offset, 11);
		ewp_info->modecde[11] = 0;
		offset += 11;
		check_len -= 11;
		#ifdef __LOG__
			dcs_log(0, 0, "modecde=[%s]", ewp_info->modecde);
		#endif
	}
	
	/*20个字节的自定义域*/
	memcpy(ewp_info->selfdefine, rcvbuf + offset, 20);
	ewp_info->selfdefine[20] = 0;
	offset += 20;
	check_len -= 20;
		
	#ifdef __LOG__
		dcs_log(0, 0, "selfdefine=[%s]", ewp_info->selfdefine);
	#endif
	
	/*生成22域服务点输入方式码*/
	memset(ewp_info->serinputmode, 0x00, sizeof(ewp_info->serinputmode));
	if(memcmp(ewp_info->icflag, "2", 1)==0)// 添加挥卡方式
	{
		memcpy(ewp_info->serinputmode, "07", 2);
	}
	else if(memcmp(ewp_info->icflag, "1", 1)==0)
	{
		if(memcmp(ewp_info->fallback_flag, "0", 1)==0)
		{
			memcpy(ewp_info->serinputmode, "05", 2);
		}
		if(memcmp(ewp_info->fallback_flag, "1", 1)==0)
		{
			memcpy(ewp_info->serinputmode, "02", 2);
		}
	}
	else if(memcmp(ewp_info->icflag, "0", 1)==0)
	{
		memcpy(ewp_info->serinputmode, "02", 2);
	}
	else
	{
		memcpy(ewp_info->serinputmode, "02", 2);
	}
	if(memcmp(exist_track_flag, "0", 1)==0)
	{
		memcpy(ewp_info->serinputmode, "01", 2);
	}
	if(memcmp(exist_pwd_flag, "1", 1)!=0)
	{
		sprintf(ewp_info->serinputmode, "%s2", ewp_info->serinputmode);
	}
	else 
	{
		sprintf(ewp_info->serinputmode, "%s1", ewp_info->serinputmode);
	}	

	if(check_len != 0 )
		strcpy(ewp_info->retcode, "F1");
	else
	{
		strcpy(ewp_info->retcode, "00");
		ewp_info->retcode[2] = 0;
	}
	
	if(	memcmp(ewp_info->retcode, "00", 2) == 0)
		dcs_log(0, 0, "数据解析成功, retcode=[%s]", ewp_info->retcode);
	else
		dcs_log(0, 0, "数据不正确, 直接丢弃该包, retcode=[%s]", ewp_info->retcode);
	return 0;
	
}

/**
*
函数名称：OrganizateData
函数功能：组key和value的长度和值，初始化结构体变量KEY_VALUE_INFO
输入参数：ewp_info
key:ewp_info中sentdate，senttime，orderno
values：<t>数据字典</t> xml格式数据
*
**/
int OrganizateData(EWP_INFO ewp_info, KEY_VALUE_INFO *key_value_info)
{
	char tempbuf[1024];
	int temp_len = 0;
	
	memset(tempbuf, 0x00, sizeof(tempbuf));
	
	sprintf(key_value_info->keys, "%s%s%s", ewp_info.consumer_sentdate, ewp_info.consumer_senttime, ewp_info.consumer_orderno);
	key_value_info->keys_length = strlen(key_value_info->keys);
	
	sprintf(tempbuf, "%s\r\n\t", "<t>");
	sprintf(tempbuf, "%s%s%s%s\r\n\t", tempbuf, "<transtype>", ewp_info.consumer_transtype, "</transtype>");
	sprintf(tempbuf, "%s%s%s%s\r\n\t", tempbuf, "<transdealcode>", ewp_info.consumer_transdealcode, "</transdealcode>");
	sprintf(tempbuf, "%s%s%s%s\r\n\t", tempbuf, "<sentdate>", ewp_info.consumer_sentdate, "</sentdate>");
	sprintf(tempbuf, "%s%s%s%s\r\n\t", tempbuf, "<senttime>", ewp_info.consumer_senttime, "</senttime>");
	sprintf(tempbuf, "%s%s%s%s\r\n\t", tempbuf, "<sentinstitu>", ewp_info.consumer_sentinstitu, "</sentinstitu>");
	sprintf(tempbuf, "%s%s%s%s\r\n\t", tempbuf, "<transcode>", ewp_info.consumer_transcode, "</transcode>");
	sprintf(tempbuf, "%s%s%s%s\r\n\t", tempbuf, "<orderno>", ewp_info.consumer_orderno, "</orderno>");
	sprintf(tempbuf, "%s%s%s%s\r\n\t", tempbuf, "<psamid>", ewp_info.consumer_psamno, "</psamid>");
	sprintf(tempbuf, "%s%s%s%s\r\n\t", tempbuf, "<translaunchway>", ewp_info.translaunchway, "</translaunchway>");
	sprintf(tempbuf, "%s%s%s%s\r\n\t", tempbuf, "<selfdefine>", ewp_info.selfdefine, "</selfdefine>");
	sprintf(tempbuf, "%s%s%s%s\r\n\t", tempbuf, "<incdname>", ewp_info.incdname, "</incdname>");
	sprintf(tempbuf, "%s%s%s%s\r\n\t", tempbuf, "<incdbkno>", ewp_info.incdbkno, "</incdbkno>");
	sprintf(tempbuf, "%s%s%s%s\r\n\t", tempbuf, "<transfee>", ewp_info.transfee, "</transfee>");
	sprintf(tempbuf, "%s%s%s%s\r\n\t", tempbuf, "<tpduheader>", ewp_info.tpduheader, "</tpduheader>");
	sprintf(tempbuf, "%s%s%s%s\r\n", tempbuf, "<poitcde_flag>", ewp_info.poitcde_flag, "</poitcde_flag>");
	sprintf(tempbuf, "%s%s\r\n", tempbuf, "</t>");
	
	temp_len = strlen(tempbuf);
	memcpy(key_value_info->value, tempbuf, temp_len);
	key_value_info->value_length = temp_len;
	
	#ifdef __TEST__
		dcs_log(0, 0, "key_value_info->keys=[%s]", key_value_info->keys);
		dcs_log(0, 0, "key_value_info->keys_length=[%d]", key_value_info->keys_length);
		dcs_log(0, 0, "key_value_info->value=[%s]", key_value_info->value);
		dcs_log(0, 0, "key_value_info->value_length=[%d]", key_value_info->value_length);
	#endif
	return 0;
}

/*
* 处理EWP电子现金脱机消费交易多次上送的情况
* 如果之前上送过，则取上次上送的信息重组报文发送到核心系统
*/
int ewpEcOrginTrans(ISO_data siso,EWP_INFO ewp_info, PEP_JNLS pep_jnls, SECUINFO secuinfo, char * currentDate,char * folderName)
{
	char tmpbuf[127];
	char bcdbuf[512];
	int s = 0, rtn = 0;
	POS_CUST_INFO pos_cust_info;
	//保存到缓存数据memcached结构体
	KEY_VALUE_INFO key_value_info;

	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	memset(bcdbuf, 0x00, sizeof(bcdbuf));
	memset(&pos_cust_info, 0, sizeof(POS_CUST_INFO));
	memset(&key_value_info, 0, sizeof(KEY_VALUE_INFO));
	s = atoi(ewp_info.filed55_length);
	asc_to_bcd((unsigned char*)bcdbuf, (unsigned char*)ewp_info.filed55,s,0);
	rtn = analyse55(bcdbuf, s/2, stFd55_IC, 34);
	if(rtn < 0 )
	{
		dcs_log(0, 0, "解析55域失败,解析结果[%d]", rtn);
		return -1;
	}
	else
	{
		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy(tmpbuf, currentDate, 2);
		memcpy(tmpbuf+2, stFd55_IC[6].szValue, 6);
		memcpy(pep_jnls.trndate, tmpbuf, 8);
		memcpy(pep_jnls.trntime, "000000", 6);
		dcs_log(0, 0, "pep_jnls.trndate=[%s], pep_jnls.trntime=[%s]", pep_jnls.trndate, pep_jnls.trntime);
	}
	/*查询原笔 */
	rtn = getEwpOriTuoInfo(&pep_jnls,ewp_info);
	if(rtn == -1)
	{
		dcs_log(0, 0, "脱机上送处理失败[%d]", rtn);
		setbit(&siso, 39, (unsigned char *)RET_CODE_FZ, 2);
		/*保存解析到的交易数据到数据表xpep_errtrade_jnls中*/
		memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_FZ, 2);
		errorTradeLog(ewp_info, folderName, "数据库操作失败");
		WriteEWP(siso, ewp_info, pep_jnls,0);
		return -1;
	}
	//如果已经发送过，取之前发送的信息重组报文再次发送核心
	else if(rtn ==3)
	{
		dcs_log(0, 0, "找到已上送请求,组包发往核心");

		setbit(&siso, 3,(unsigned char *)pep_jnls.process, 6);

		memset(tmpbuf, 0, sizeof(tmpbuf));
		sprintf(tmpbuf, "%s%s", pep_jnls.pepdate+4, pep_jnls.peptime);
#ifdef __LOG__
		dcs_log(0, 0, "7域信息[%s],len =[%d]", tmpbuf, strlen(tmpbuf));
#endif
		//需要使用原来的值重新赋值
		setbit(&siso, 7,(unsigned char *)tmpbuf, 10);
		memset(tmpbuf, 0, sizeof(tmpbuf));
		sprintf(tmpbuf, "%06d", pep_jnls.trace);
		setbit(&siso, 11,(unsigned char *)tmpbuf, 6);

		memset(tmpbuf, 0, sizeof(tmpbuf));
		s = GetCustInfo(ewp_info.consumer_username, ewp_info.consumer_psamno, &pos_cust_info);
		if(s == -1)
		{
			dcs_log(0, 0, "脱机上送处理失败[%d]", rtn);
			setbit(&siso, 39, (unsigned char *)RET_CODE_FZ, 2);
			/*保存解析到的交易数据到数据表xpep_errtrade_jnls中*/
			memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_FZ, 2);
			errorTradeLog(ewp_info, folderName, "数据库操作失败");
			WriteEWP(siso, ewp_info, pep_jnls,0);
			return -1;
		}
		if(getstrlen(ewp_info.translaunchway) == 0)
				memcpy(ewp_info.translaunchway, "01", 2);
		sprintf(tmpbuf, "%s%-15s%s%s%s%06s%04d", ewp_info.consumer_psamno, ewp_info.consumer_username, ewp_info.translaunchway+1, "01", ewp_info.consumer_transcode, "000000", secuinfo.channel_id);

	#ifdef __LOG__
		dcs_log(0, 0, "21域[%d][%s]", strlen(tmpbuf), tmpbuf);
	#endif
		if(getstrlen(pos_cust_info.T0_flag)==0)
			memcpy(pos_cust_info.T0_flag, "0", 1);
		if(getstrlen(pos_cust_info.agents_code)==0)
			memcpy(pos_cust_info.agents_code,"000000",6);
		sprintf(tmpbuf+47, "%s", pos_cust_info.T0_flag);
		sprintf(tmpbuf+48, "%s", pos_cust_info.agents_code);
		setbit(&siso, 21, (unsigned char *)tmpbuf, 54);
		setbit(&siso, 22, (unsigned char *)pep_jnls.poitcde, 3);

		/*卡片序列号存在标识为1,非降级交易：标识可以获取卡片序列号（23域有值）*/
		if(memcmp(ewp_info.cardseq_flag, "1",1) == 0 && strlen(ewp_info.cardseq)>0)
		{
			setbit(&siso, 23, (unsigned char *)ewp_info.cardseq, 3);
		}

		/*55域长度不为000.表示55域存在，否则不存在55域固定填000*/
		if(memcmp(ewp_info.filed55_length, "000", 3) != 0)
		{
			s = atoi(ewp_info.filed55_length);
			setbit(&siso, 55, (unsigned char *)bcdbuf, s/2);
		}

		setbit(&siso, 25,(unsigned char *)pep_jnls.poscode, 2);
		setbit(&siso, 20, (unsigned char *)NULL, 0);

		setbit(&siso, 41,(unsigned char *)pep_jnls.termcde, 8);
		setbit(&siso, 42, (unsigned char *)pep_jnls.mercode, 15);

		setbit(&siso, 63, (unsigned char *)NULL, 0);
		setbit(&siso, 64, (unsigned char *)NULL, 0);

		//保存memcache
		memset(tmpbuf, 0, sizeof(tmpbuf));
		sprintf(tmpbuf,"%d",pep_jnls.trace);
		OrganizateData(ewp_info, &key_value_info);
		//脱机消费 重新组 key,以为发起时间和交易时间有可能不同
		if(memcmp(ewp_info.consumer_transcode, "211", 3) ==0)
		{
			sprintf(key_value_info.keys, "%s%s%s",pep_jnls.trndate, pep_jnls.trntime, ewp_info.consumer_orderno);
			key_value_info.keys_length = strlen(key_value_info.keys);
		}
		s = Mem_SavaKey(key_value_info.keys, key_value_info.value, key_value_info.value_length, 600);
		if(s == -1)
		{
			dcs_log(0, 0, "save mem error");
			setbit(&siso,39,(unsigned char *)RET_CODE_F9,2);
			updateXpepErrorTradeInfo(ewp_info, tmpbuf, RET_CODE_F9);
			WriteEWP(siso,ewp_info,pep_jnls,0);
			return -1;
		}

		memset(g_key, 0, sizeof(g_key));
		s=WriteXpe(siso);
		if(s == -1)
		{
			dcs_log(0,0,"发送信息给核心失败");
			setbit(&siso,39,(unsigned char *)RET_CODE_F9,2);
			updateXpepErrorTradeInfo(ewp_info, tmpbuf, RET_CODE_F9);
			WriteEWP(siso,ewp_info,pep_jnls,0);
			return -1;
		}
	}
	return rtn;
}


