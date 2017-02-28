#include "ibdcs.h"
#include "trans.h"
#include "pub_function.h"
#include "memlib.h"

extern struct ISO_8583 *iso8583;
extern struct ISO_8583 iso8583_conf[128];
extern struct ISO_8583 iso8583conf[128];

/* 
	模块说明：
		此模块主要处理mpos终端发起的消费类交易，不提供冲正接口，由核心系统做超时控制。EWP系统是手机刷卡支付终端的受理系统，之后按照《前置系统接入报文规范MPOS》的报文格式，
	将终端报文送到前置系统。磁道和密码数据都是密文形式传输。前置系统如果解析磁道或者密码错误的话，直接返回EWP平台终端密钥错误。
	否则就受理该笔交易。解析之后的报文数据首先做风控的管理，通过ksn号查询到该台终端的风控规则，由前置系统做风控处理。对于不满足风控要求的交易，返回EWP平台对应的错误信息。
	最后前置系统组成ISO8583报文之后，送往核心系统。其中消费类的交易中，第三域为910000。
 */
void mposProc(char *srcBuf, int iFid, int iMid, int iLen)
{

	#ifdef __LOG__
		dcs_debug(srcBuf, iLen,"mpos Proc, iFid=%d, iMid=%d, iLen=%d", iFid, iMid, iLen);
	#else
		dcs_log(0, 0, "mpos Proc, iFid=%d, iMid=%d, iLen=%d", iFid, iMid, iLen);
	#endif
	
	/*变量定义*/
	int rtn = 0, s = 0;
	
	int datalen = 0;
		
	//转出卡卡种
	char outCardType;
	
	//当前日期和时间
	char currentDate[8+1];
	char currentTime[6+1];
	
	//前置系统报文类型
	char folderName[4+1];
	
	char tmpbuf[1024];
	
	//前置系统流水
	char trace[7];
		
	char databuf[PUB_MAX_SLEN + 1];
	
	//按照《前置系统接入报文规范》解析出来的数据放入该结构体中
	EWP_INFO ewp_info; 
	
	//交易类型配置信息表，根据交易码查询数据库表msgchg_info，得到的信息放入该结构体中
	MSGCHGINFO	msginfo;
	
	//终端信息表
	TERMMSGSTRUCT termbuf;
	
	//ISO8583结构体，送往核心系统的数据
	ISO_data siso;
	
	//数据库交易信息保存的结构体，对于数据库表pep_jnls
	PEP_JNLS pep_jnls; 
	
	//终端风控结构体
	SAMCARD_TRADE_RESTRICT samcard_trade_restric;
	
	//终端商户风控结构体
	SAMCARD_MERTRADE_RESTRICT samcard_mertrade_restric;

	//KSN结构体
	KSN_INFO ksn_info;
	
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
	int len=0; /*数据长度*/
	
	char temp_bcd_55[512];
	char bcd_buf[10];

	//DUKPT
	char ksnbuf[15+1];
	char resultTemp[32+1];
	char outPin[20];
	
	/*
	初始化变量
	*/
	memset(totalamt, 0, sizeof(totalamt));
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(trace, 0, sizeof(trace));
	memset(folderName, 0, sizeof(folderName));
	memset(databuf, 0, sizeof(databuf));
	memset(currentDate, 0, sizeof(currentDate));
	memset(currentTime, 0, sizeof(currentTime));
	
	memset(&ewp_info,0, sizeof(EWP_INFO));
	memset(&msginfo, 0, sizeof(MSGCHGINFO));
	memset(&siso, 0, sizeof(ISO_data));
	memset(&pep_jnls, 0 ,sizeof(PEP_JNLS));
	memset(&termbuf, 0, sizeof(TERMMSGSTRUCT));
	memset(&samcard_trade_restric, 0, sizeof(SAMCARD_TRADE_RESTRICT));
	memset(&samcard_mertrade_restric, 0, sizeof(SAMCARD_MERTRADE_RESTRICT));
	memset(&ksn_info, 0, sizeof(KSN_INFO));
	memset(&key_value_info, 0, sizeof(KEY_VALUE_INFO));
	
	memcpy(iso8583_conf, iso8583conf, sizeof(iso8583conf));
	iso8583 = &iso8583_conf[0];
	SetIsoHeardFlag(0);
	SetIsoFieldLengthFlag(1);
	time(&t);
	tm = localtime(&t);

	sprintf(currentDate, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
	sprintf(currentTime, "%02d%02d%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);

	pep_jnls.trnsndpid = iFid ;
	/*得到交易链路名称*/
	memset(folderName, 0, sizeof(folderName));
	memcpy(folderName, srcBuf, 4);
	memcpy(pep_jnls.trnsndp, folderName, getstrlen(folderName));

	srcBuf += 4;
	iLen -= 4;

	memcpy(tmpbuf, srcBuf, 5);
	bcd_to_asc((unsigned char *)ewp_info.tpduheader, (unsigned char *)tmpbuf, 10, 0);
	dcs_log(0, 0, "TPDU：%s", ewp_info.tpduheader);
	
	srcBuf += 5;
	iLen -= 5;
	
	datalen = iLen;
	memcpy(databuf, srcBuf, datalen);

	//防重放
	rtn =ewpPreventReplay(databuf,databuf,datalen);
	if(rtn != 0)
		return;
	
	/*解析收到的EWP的数据*/
	dcs_log(0, 0, "解析EWP报文开始");
	mposparsedata(databuf, datalen, &ewp_info);
	dcs_log(0, 0, "解析EWP报文结束");
	
	/*如果解析帐单数据失效的话，直接抛弃这个包*/
	 if (memcmp(ewp_info.retcode, "00", 2) != 0)
	{
		dcs_log(0, 0, "解析帐单数据错误");
		setbit(&siso, 39, (unsigned char *)RET_CODE_F1, 2);
		/*保存解析到的交易数据到数据表pep_db_log中*/
		memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_F1, 2);
		errorTradeLog(ewp_info, folderName, "解析账单数据错误");
		WriteEWP_Mpos(siso, ewp_info, pep_jnls,0);
		return;
	}
	 //根据KSN获取终端的密钥信息	 
	memcpy(ksn_info.ksn, ewp_info.consumer_ksn, 20);
	 
	//将送上来的20字节ksn号转换为15字节的ksn机具号,去除counter值
	memset(ksnbuf, 0x00, sizeof(ksnbuf));
	asc_to_bcd(ksnbuf, ksn_info.ksn, 20, 0);
	ksnbuf[7] &= 0xE0;
	ksnbuf[8] = 0x00;
	ksnbuf[9] = 0x00;
	
	memcpy(ksn_info.initksn,ksnbuf,5);
	bcd_to_asc(ksn_info.initksn+5, ksnbuf+5, 10, 0);
	
	//bcd_to_asc(ksn_info.initksn, ksnbuf, 20, 0);//test
	s = GetMposInfoByKsn(&ksn_info);
	if(s < 0)
	{
		dcs_log(0, 0, "查询ksn_info失败,ksn未找到, ksn=%s", ksn_info.initksn);
		setbit(&siso, 39, (unsigned char *)RET_CODE_FC, 2);
		memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_FC, 2);
		errorTradeLog(ewp_info, folderName, "ksn未找到");
		WriteEWP_Mpos(siso, ewp_info, pep_jnls, 0);
		return;
	}
	s = MakeDukptKey(&ksn_info);
	if(s<0)
	{
		dcs_log(0 , 0, "MakeDukptKey失败");
		setbit(&siso, 39, (unsigned char *)RET_CODE_F1, 2);
		memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_F1, 2);
		errorTradeLog(ewp_info, folderName, "MakeDukptKey失败");
		WriteEWP_Mpos(siso, ewp_info, pep_jnls,0);
		return;
	}

	/*根据解析到的数据交易码来查询数据库msgchg_info来读取一些信息*/
	rtn = Readmsgchg(ewp_info.consumer_transcode, &msginfo);
	if(rtn == -1)
	{
		dcs_log(0, 0, "查询msgchg_info失败,交易未开通,trancde=%s", ewp_info.consumer_transcode);
		setbit(&siso, 39, (unsigned char *)RET_CODE_FB, 2);
		memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_FB, 2);
		errorTradeLog(ewp_info, folderName, "查询msgchg_info失败,交易未开通");
		WriteEWP_Mpos(siso, ewp_info, pep_jnls,0);
		return;
	}else
	{	
		if(memcmp(msginfo.flagpe, "Y", 1)!=0 && memcmp(msginfo.flagpe, "y", 1)!= 0 )
		{
			dcs_log(0, 0, "由于系统或者其他原因，交易暂时关闭");
			setbit(&siso, 39, (unsigned char *)RET_TRADE_CLOSED, 2);
			memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_TRADE_CLOSED, 2);
			errorTradeLog(ewp_info, folderName, "由于系统或者其他原因，交易暂时关闭");
			WriteEWP_Mpos(siso, ewp_info, pep_jnls, 0);
			return;
		}
	}
	/*余额查询交易及其脚本通知都不需要上送金额*/
	if(memcmp(ewp_info.consumer_transdealcode,"310000",6) != 0)
	{	
		setbit(&siso, 4, (unsigned char *)ewp_info.consumer_money, 12);		
	}

	/*转账或是信用卡还款*/
	if(memcmp(ewp_info.consumer_transtype, "0620", 4)!=0 
	&& (memcmp(ewp_info.consumer_transdealcode, "190000", 6) == 0||memcmp(ewp_info.consumer_transdealcode, "480000", 6) == 0))
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
	setbit(&siso, 2, (unsigned char *)ewp_info.consumer_cardno, getstrlen(ewp_info.consumer_cardno));
	setbit(&siso, 49, (unsigned char *)"156", 3);
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	if(memcmp(ewp_info.poitcde_flag, "1", 1)==0)
		memcpy(tmpbuf, "50", 2);
	else
		memcpy(tmpbuf, "20", 2);
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
	
	//取前置系统交易流水
	if( pub_get_trace_from_seq(trace) < 0)
	{
		dcs_log(0, 0, "取前置系统交易流水错误" );
		setbit(&siso, 39, (unsigned char *)RET_CODE_F8, 2);
		memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_F8, 2 );
		errorTradeLog(ewp_info, folderName, "取前置系统交易流水错误");
		WriteEWP_Mpos(siso, ewp_info, pep_jnls, 0);
		return;
	}
	
	//交易码
	memcpy(termbuf.cTransType, ewp_info.consumer_transcode, 3);	

	//cSamID
	memcpy(termbuf.cSamID, ksn_info.initksn, 15);

	//设置folder名称
	memcpy(termbuf.e1code, folderName, getstrlen(folderName));
	
	//校验mac
	/*#ifndef __TEST__ */
	s = MposMacCheck(ksn_info.tak, ewp_info);
	if(s != 0)
	{
		dcs_log(0, 0, "mac校验失败");
		setbit(&siso, 39, (unsigned char *)"A0", 2);
		setbit(&siso, 11, (unsigned char *)trace, 6);
		errorTradeLog(ewp_info, folderName, "mac校验失败");
		WriteEWP_Mpos(siso, ewp_info, pep_jnls,0);
		return;
	}	
	dcs_log(0, 0, "mac校验成功");	
	/*#endif*/

	//TC值上送交易
	if( memcmp(ewp_info.consumer_transdealcode, "999997", 6) == 0 )
	{		
		rtn = EwpsaveTCvalue(ewp_info, &pep_jnls);
		if(rtn < 0)
		{
			dcs_log(0, 0, "保存TC值失败");
			memcpy(ewp_info.consumer_responsecode, "99",2);
		}
		else
			memcpy(ewp_info.consumer_responsecode, "00",2);

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
		memcpy(tmpbuf+len, databuf+4, 67);//使用ewp发送过来的报文
		len +=67;
		memcpy(tmpbuf+len, ewp_info.consumer_responsecode, 2);
		len +=2;
		char ewpinfo[1024];
		int ewpinfo_len =0;
		memset(ewpinfo, 0, sizeof(ewpinfo));
		ewpinfo_len = GetDetailInfo(ewp_info.consumer_responsecode, ewp_info.consumer_transcode, ewpinfo);
		if(ewpinfo_len == -1)
			ewpinfo_len = 0;
		/*1个字节表示应答码内容的长度*/
			/*应答码描述信息，utf-8表示*/
		tmpbuf[len] = ewpinfo_len;
		len +=1;
		memcpy(tmpbuf+len, ewpinfo, ewpinfo_len);
		len +=ewpinfo_len;
		memcpy(tmpbuf+len, ewp_info.selfdefine, 20);
		len +=20;
		WriteKeyBack(tmpbuf, len, iFid);
		
		if(memcmp(ewp_info.consumer_responsecode, "00",2) != 0)
			return;
		
		rtn = MposSendTcValue(ewp_info, pep_jnls, msginfo, siso, ksn_info);
		if(rtn == -1)
		{
			dcs_log(0, 0, "IC卡TC值上送核心处理error");
		}
		else
			dcs_log(0, 0, "IC卡TC值上送核心处理success");
		
		return;
	}

	//设置订单号
	if(memcmp(ewp_info.consumer_transtype, "0620", 4)!=0 && (memcmp(ewp_info.consumer_transdealcode, "480000", 6) == 0))
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

	/* IC卡交易处理23,55域*/
	if(memcmp(ewp_info.consumer_transtype, "0620", 4)!=0 && (memcmp(ewp_info.consumer_transdealcode,"310000",6) == 0
	       || memcmp(ewp_info.consumer_transdealcode, "000000", 6) == 0 
		|| memcmp(ewp_info.consumer_transdealcode, "480000", 6) == 0
		|| memcmp(ewp_info.consumer_transdealcode, "190000", 6) == 0))
	{
		/*卡片序列号存在标识为1,非降级交易：标识可以获取卡片序列号（23域有值）*/
		if(memcmp(ewp_info.cardseq_flag, "1",1) == 0 && memcmp(ewp_info.fallback_flag, "0",1) == 0 && strlen(ewp_info.cardseq)>0)
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
		setbit(&siso, 0, (unsigned char *)msginfo.msgtype, 4);
		setbit(&siso, 3, (unsigned char *)msginfo.process, 6);

	}
	else if(memcmp(ewp_info.consumer_transtype, "0620", 4)==0)
	{	
		//脚本通知交易
		len = atoi(ewp_info.filed55_length);		
		if(memcmp(ewp_info.filed55_length, "000", 3) != 0)
		{
			asc_to_bcd((unsigned char *)temp_bcd_55, (unsigned char*)ewp_info.filed55, len, 0);
			setbit(&siso, 55, (unsigned char *)temp_bcd_55, len/2);
		}
		setbit(&siso, 0, (unsigned char *)"0620", 4);
		setbit(&siso, 3, (unsigned char *)msginfo.process, 6);

	}
	else
	{
		setbit(&siso, 4, (unsigned char *)ewp_info.consumer_money, 12);		
	}

	/*脚本通知交易 */
	if(memcmp(ewp_info.consumer_transtype, "0620", 4) == 0)
	{
		/*3域重新赋值*/
		setbit(&siso, 3, (unsigned char *)ewp_info.consumer_transdealcode, 6);
		
		dcs_log(0,0,"ewp_info.serinputmode [%s] ", ewp_info.serinputmode);
		/* 组装61域，根据原交易参考号查询原POS流水号 */
		char trace_old[6+1];
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
		if(getstrlen(ewp_info.consumer_money)!=0)
			setbit(&siso, 4, (unsigned char *)ewp_info.consumer_money, 12);	
		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy(tmpbuf, "50", 2);
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
	}
	
	/*设置流水号需在冲正交易之前*/
	setbit(&siso, 11, (unsigned char *)trace, 6);
	setbit(&siso, 22, (unsigned char *)ewp_info.serinputmode, 3);
	setbit(&siso, 25, (unsigned char *)msginfo.service, 2);
	
	//判断卡的种类
	outCardType = ReadCardType(ewp_info.consumer_cardno);
	if(outCardType == -1)
	{
		dcs_log(0, 0, "查询card_transfer失败");
		setbit(&siso, 39, (unsigned char *)RET_CODE_FZ, 2);
		setbit(&siso, 11, (unsigned char *)trace, 6);
		WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
		WriteEWP_Mpos(siso, ewp_info, pep_jnls,0);
		return;
	}
	pep_jnls.outcdtype = outCardType;

	/*内部商户卡种的限制处理*/
	/*根据卡号判断卡种*/
	dcs_log(0, 0, "msginfo.cdlimit=%c", msginfo.cdlimit);
	if( msginfo.cdlimit == 0x43 || msginfo.cdlimit == 0x63 || msginfo.cdlimit == 0x44  || msginfo.cdlimit == 0x64 )
	{	
		dcs_log(0, 0, "有卡种限制");
		if((outCardType == msginfo.cdlimit)||(outCardType == '-'))
		{
			dcs_log(0, 0, "该银行卡不支持此功能");
			setbit(&siso, 39, (unsigned char *)RET_CODE_F2, 2);
			WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
			WriteEWP_Mpos(siso, ewp_info,pep_jnls, 0);
			return;
		}	
	}
	
	/*
	psam卡号（16字节）暂改为ksn(15个字节+1个空格)
	+ 电话号码，
	区号与电话号码用|区分（或者手机号）（15字节，左对齐，右补空格）
	+ 交易发起方式（1字节，ewp平台送过来的信息）
	+ 终端类型（2字节，刷卡设备取值为01）
	+ 交易码（3字节）+ 终端流水（6字节，不存在的话填全0）
	+ 渠道ID(4个字节)
	*/ 
	memset(tmpbuf, 0, sizeof(tmpbuf));
	#ifdef __LOG__
		dcs_log(0, 0, "ksn_info.initksn=[%s]", ksn_info.initksn);/*ksn initksn*/
		dcs_log(0, 0, "ewp_info.consumer_username=[%s]", ewp_info.consumer_username);/*手机号*/
		dcs_log(0, 0, "ewp_info.translaunchway=[%s]", ewp_info.translaunchway);/*交易发起方式*/
		dcs_log(0, 0, "ewp_info.consumer_transcode=[%s]", ewp_info.consumer_transcode);/*交易码*/
		dcs_log(0, 0, "ksn.channel_id=[%d]", ksn_info.channel_id);/*渠道ID*/
	#endif
	
	if(getstrlen(ewp_info.translaunchway) == 0)
		memcpy(ewp_info.translaunchway, "1", 1);
	sprintf(tmpbuf, "%-16s%-15s%s%s%s%06s%04d", ksn_info.initksn, ewp_info.consumer_username, ewp_info.translaunchway, "01", ewp_info.consumer_transcode, "000000", ksn_info.channel_id);
	
	//T0_flag,agents_code写死为0    modify begin 150413 
	memcpy(tmpbuf+47, "0000000", 7);
	dcs_log(0, 0, "21域[%d][%s]", strlen(tmpbuf), tmpbuf);
	setbit(&siso, 21, (unsigned char *)tmpbuf, 54);

	if(memcmp(ewp_info.consumer_transtype, "0620", 4) != 0)
	{
		//解密磁道
		s = MPOS_TrackDecrypt(&ewp_info, &ksn_info, &siso);
		if(s < 0)
		{
			dcs_log(0, 0, "解密磁道错误, s=%d", s);
			setbit(&siso, 39, (unsigned char *)RET_CODE_F3, 2);
			WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
			WriteEWP_Mpos(siso, ewp_info, pep_jnls, 0);
			return;
		}
		
		//如果不存在密码，那么就会上送16字节长度的全F或者全f
		if(memcmp(ewp_info.consumer_password, "FFFFFFFFFFFFFFFF", 16) == 0 || memcmp(ewp_info.consumer_password, "ffffffffffffffff", 16) == 0)
		{
			dcs_log(0, 0, "密码为空，通过msgchg_info判断是否需要密码.");
			if(memcmp(msginfo.flagpwd, "1", 1) == 0 )
			{
				dcs_log(0, 0, "该交易不允许银行卡无密支付，返回EWP FA Error!");
				setbit(&siso, 39, (unsigned char *)RET_CODE_FA, 2);
				WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
				WriteEWP_Mpos(siso, ewp_info, pep_jnls, 0);
				return;
			}
			else
			{
				setbit(&siso, 52, (unsigned char *)NULL, 0);
				setbit(&siso, 53, (unsigned char *)NULL, 0);
			}
		}
		else
		{
			//转加密密码
			memset(tmpbuf, 0, sizeof(tmpbuf));
			s =  MposPinConver(ksn_info.tpk, ewp_info.consumer_password, ewp_info.consumer_cardno, tmpbuf);
			if(s < 0)
			{
				dcs_log(0,0,"转加密密码错误,error code=%d",s);
				setbit(&siso,39,(unsigned char *)RET_CODE_F7,2);
				WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
				WriteEWP_Mpos(siso,ewp_info,pep_jnls,0);
				return; 
			}
				
			setbit(&siso, 52, (unsigned char *)tmpbuf, 8);
			setbit(&siso, 53, (unsigned char *)"1000000000000000", 16);
		}
		
		/*
			首先判断该订单是否已经发生过支付。如果发生过支付，并且状态是超时的话，那么判断当前的时间与前一笔交易的时间差，如果小于5分钟，那么就不允许再次发起支付。
		*/
		if(memcmp(ewp_info.consumer_transdealcode, "310000", 6)!=0 )
		{
			s = ewpOrderRepeatPayCharge(ewp_info.consumer_orderno, currentDate, currentTime);
			if(s < 0)
			{
				dcs_log(0, 0, "订单支付状态未知，5分钟之内不允许再次支付");
				setbit(&siso, 39, (unsigned char *)RET_CODE_H0, 2);
				WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
				WriteEWP_Mpos(siso, ewp_info, pep_jnls,0);
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
	s = ChannelTradeCharge(ewp_info.consumer_transcode, ksn_info.channel_id);
	if(s == -1)
	{
		dcs_log(0, 0, "查询ChannelTradeCharge失败,该渠道该交易不存在");
		setbit(&siso, 39, (unsigned char *)RET_CODE_FD, 2);
		WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
		WriteEWP_Mpos(siso, ewp_info, pep_jnls, 0);
		return;
	}
	else if(s == -2)
	{
		dcs_log(0, 0, "查询ChannelTradeCharge失败,该渠道限制交易");
		setbit(&siso, 39, (unsigned char *)RET_CODE_FE, 2);
		WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
		WriteEWP_Mpos(siso, ewp_info, pep_jnls, 0);
		return;
	}
	else if(s == -3)
	{
		dcs_log(0, 0, "查询ChannelTradeCharge失败, 该渠道该交易未开通");
		setbit(&siso, 39, (unsigned char *)RET_CODE_FF, 2);
		WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
		WriteEWP_Mpos(siso, ewp_info, pep_jnls, 0);
		return;
	}
	else
	{
		dcs_log(0, 0, "查询ChannelTradeCharge成功，该渠道交易正常");
	}
	
	/*
		获得终端商户风控信息
	*/
	s = GetSamcardMerRestrictInfo(currentDate, ksn_info.initksn, msginfo.mercode, &samcard_mertrade_restric);
	if(s == -1)
	{
		dcs_log(0,0,"获取终端商户风控数据,数据库操作错误");
		setbit(&siso,39,(unsigned char *)RET_CODE_FZ, 2);
		memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_FZ, 2);
		memcpy(ewp_info.consumer_presyswaterno, trace, 6);
		errorTradeLog(ewp_info, folderName, "获取终端商户风控数据,数据库操作错误");
		WriteEWP_Mpos(siso, ewp_info, pep_jnls,0);
		return;
	}
	else if(s == 0)
	{
		dcs_log(0,0,"该终端商户没有风险控制");
	}
	else
	{
		dcs_log(0,0,"获取终端商户风险控制配置信息成功。");
	}
	
	
	/*
		获得终端风险控制信息
	*/
	s = GetSamcardRestrictInfo(currentDate, ksn_info.initksn, &samcard_trade_restric);
	if(s == 0)
	{
		dcs_log(0,0,"该终端没有风险控制");
		
	}
	else if(s == -1)
	{
		dcs_log(0,0,"数据库操作错误");
		setbit(&siso,39,(unsigned char *)RET_CODE_FZ, 2);
		memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_FZ, 2);
		memcpy(ewp_info.consumer_presyswaterno, trace, 6);
		errorTradeLog(ewp_info, folderName, "数据库操作错误");
		WriteEWP_Mpos(siso, ewp_info, pep_jnls,0);
		return;
	}
	
	/*
		终端分控的判断
		判断该终端是否在黑名单中
	*/
	if( samcard_trade_restric.psam_restrict_flag == 1 && memcmp(samcard_trade_restric.restrict_trade, "-", 1) != 0)
	{
		if( memcmp(samcard_trade_restric.restrict_trade, "*", 1) == 0 && strstr(samcard_trade_restric.restrict_trade, termbuf.cTransType) == NULL )
		{	
			dcs_log(0, 0, "该终端属于黑名单，不允许交易");
			setbit(&siso, 39, (unsigned char *)RET_CODE_FG, 2);
			WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
			WriteEWP_Mpos(siso, ewp_info, pep_jnls, 0);
			return;
		}
	}
	
	if(memcmp(ewp_info.consumer_transtype, "0620", 4)!=0 && memcmp(ewp_info.consumer_transdealcode, "310000", 6) != 0)
	{
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
			setbit(&siso, 39, (unsigned char *)RET_CODE_F6, 2);
			WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
			WriteEWP_Mpos(siso, ewp_info,pep_jnls, 0);
			return;
		} 
		if(getstrlen(msginfo.c_sin_totalamt)!=1 || getstrlen(msginfo.d_sin_totalamt)!=1 || msginfo.c_day_count !=0 || msginfo.d_day_count !=0)
		{
			totalcount = getamt_count(currentDate, ewp_info, totalamt);
			if(totalcount == -1)
			{
				dcs_log(0, 0, "数据库获取总金额以及交易次数失败");
				setbit(&siso, 39, (unsigned char *)RET_CODE_FJ, 2);
				WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
				WriteEWP_Mpos(siso, ewp_info, pep_jnls, 0);
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
					dcs_log(0, 0, "日交易金额限制");
					setbit(&siso, 39, (unsigned char *)RET_CODE_F6, 2);
					WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
					WriteEWP_Mpos(siso, ewp_info, pep_jnls, 0);
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
					dcs_log(0, 0, "成功的交易次数限制");
					setbit(&siso, 39, (unsigned char *)"PF", 2);
					WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
					WriteEWP_Mpos(siso, ewp_info, pep_jnls, 0);
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
		WriteEWP_Mpos(siso,ewp_info,pep_jnls,0);
		return;
	}
	
	#ifdef __LOG__
		dcs_log(0, 0, "samcard_mertrade_restric.c_amttotal = [%s]", samcard_mertrade_restric.c_amttotal);
		dcs_log(0, 0, "outCardType = [%c]", outCardType);
		dcs_log(0, 0, "samcard_mertrade_restric.psammer_restrict_flag = [%d]", samcard_mertrade_restric.psammer_restrict_flag);
	#endif
	
	if( samcard_mertrade_restric.psammer_restrict_flag == 1 &&  outCardType == 'C' && memcmp(samcard_mertrade_restric.c_amttotal, "-", 1)!=0 )
	{	
		dcs_log(0,0,"term mer c_amtdate date:[%s]", samcard_mertrade_restric.c_amtdate);
		memset(tmpbuf, '0', sizeof(tmpbuf));
		tmpbuf[12] = 0;
		SumAmt(ewp_info.consumer_money, samcard_mertrade_restric.c_total_tranamt, tmpbuf);
		if(memcmp(tmpbuf, samcard_mertrade_restric.c_amttotal, 12) > 0)
		{
			dcs_log(0,0,"当前的交易总额，超过总交易额的限制！");
			setbit(&siso,39,(unsigned char *)RET_CODE_FL,2);
			WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
			WriteEWP_Mpos(siso,ewp_info,pep_jnls,0);
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
			WriteEWP_Mpos(siso,ewp_info,pep_jnls,0);
			return;
		}
		
		if( outCardType == samcard_trade_restric.restrict_card )
		{
			dcs_log(0,0,"终端卡种限制");
			setbit(&siso,39,(unsigned char *)RET_CODE_FI,2);
			WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
			WriteEWP_Mpos(siso,ewp_info,pep_jnls,0);
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
		WriteEWP_Mpos(siso,ewp_info,pep_jnls,0);
		return;
	}
	
	if( samcard_trade_restric.psam_restrict_flag == 1 && memcmp(samcard_trade_restric.amtmax, "-", 1)!=0 && memcmp(ewp_info.consumer_money, samcard_trade_restric.amtmax,12)>0)
	{
		dcs_log(0,0,"交易金额,不在最大金额的范围！");
		setbit(&siso,39,(unsigned char *)RET_CODE_FJ,2);
		WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
		WriteEWP_Mpos(siso,ewp_info,pep_jnls,0);
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
		WriteEWP_Mpos(siso,ewp_info,pep_jnls,0);
		return;
	}
	
	if( samcard_trade_restric.psam_restrict_flag == 1 &&  outCardType == 'C' && memcmp(samcard_trade_restric.c_amttotal, "-", 1)!=0)
	{	
		dcs_log(0,0,"c_amtdate date:%s",samcard_trade_restric.c_amtdate);
		memset(tmpbuf, '0', sizeof(tmpbuf));
		tmpbuf[12] = 0;
		SumAmt(ewp_info.consumer_money, samcard_trade_restric.c_total_tranamt, tmpbuf);
		//if( transamt+samcard_trade_restric.c_total_tranamt > samcard_trade_restric.c_amttotal)
		if(memcmp(tmpbuf, samcard_trade_restric.c_amttotal, 12)>0)
		{
			dcs_log(0,0,"当前的交易总额，超过总交易额的限制！");
			setbit(&siso,39,(unsigned char *)RET_CODE_FL,2);
			WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
			WriteEWP_Mpos(siso,ewp_info,pep_jnls,0);
			return;
		}
		pep_jnls.camtlimit=1;
	}
	
	if( samcard_trade_restric.psam_restrict_flag == 1 &&  outCardType=='D'&& memcmp(samcard_trade_restric.d_amttotal,"-",1)!=0)
	{	
		dcs_log(0,0,"d_amtdate date:%s",samcard_trade_restric.d_amtdate);
		memset(tmpbuf, '0', sizeof(tmpbuf));
		tmpbuf[12] = 0;
		SumAmt(ewp_info.consumer_money, samcard_trade_restric.d_total_tranamt, tmpbuf);
		if(memcmp(tmpbuf, samcard_trade_restric.d_amttotal, 12)>0)
		//if( transamt+samcard_trade_restric.d_total_tranamt > samcard_trade_restric.d_amttotal)
		{
			dcs_log(0,0,"当前的交易总额，超过总交易额的限制！");
			setbit(&siso,39,(unsigned char *)RET_CODE_FL,2);
			WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
			WriteEWP_Mpos(siso,ewp_info,pep_jnls,0);
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
		WriteEWP_Mpos(siso,ewp_info,pep_jnls,0);
		return;
	}
	else if(rtn == 1)
	{
		dcs_log(0,0,"该商户不允许该交易，mercode = %s, trancde = %s",msginfo.mercode,termbuf.cTransType);
		setbit(&siso,39,(unsigned char *)RET_CODE_FN,2);
		WriteXepDb(siso, ewp_info, termbuf, pep_jnls);
		WriteEWP_Mpos(siso,ewp_info,pep_jnls,0);
		return;
	}
	else
	{
		dcs_log(0,0,"该商户不在商户黑名单中，mercode = %s, trancde = %s",msginfo.mercode,termbuf.cTransType);
	}	

	/*
		保存数据库
	*/
	memcpy(ewp_info.consumer_psamno,ksn_info.initksn,15);	
	
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
			WriteEWP_Mpos(siso,ewp_info,pep_jnls,0);
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
		WriteEWP_Mpos(siso,ewp_info,pep_jnls,0);
		return;
	}
	*/
	//新增优化功能不使用ewp_info数据库而使用memcached数据库
	dcs_log(0, 0, "test memcached begin");
	s = getstrlen(ewp_info.consumer_orderno);
	if(s <= 0)
	{
		memset(tmpbuf, 0, sizeof(tmpbuf));
		sprintf(tmpbuf, "%s%s%s%s", "DY", currentDate+2, currentTime, trace);
		memcpy(ewp_info.consumer_orderno, tmpbuf, 20);
	}
	OrganizateData(ewp_info, &key_value_info);
	rtn = Mem_SavaKey(key_value_info.keys, key_value_info.value, key_value_info.value_length, 600);
	if(rtn == -1)
	{
		dcs_log(0, 0, "mposProc save mem error");
		dcs_log(0, 0, "mposProc保存EWP_INFO失败");
		setbit(&siso,39,(unsigned char *)RET_CODE_FP,2);
		updateXpepErrorTradeInfo(ewp_info, trace, RET_CODE_FP);
		WriteEWP_Mpos(siso,ewp_info,pep_jnls,0);
		return;
	}
	//新增end
	
	/*发送报文*/  
	rtn=WriteXpe(siso);
	if(rtn == -1)
	{
		dcs_log(0,0,"发送信息给核心失败");
		setbit(&siso,39,(unsigned char *)RET_CODE_F9,2);
		updateXpepErrorTradeInfo(ewp_info, trace, RET_CODE_F9);
		WriteEWP_Mpos(siso,ewp_info,pep_jnls,0);
		return;
	}
	
	return;	
}


/*解析从ewp那里接收到的数据 存放到结构体EWP_INFO中*/
int mposparsedata(char *rcvbuf, int rcvlen, EWP_INFO *ewp_info)
{
	int check_len, offset, temp_len;
	char tmpbuf[1024];

	check_len = 0;
	offset = 0;
	temp_len = 0;
	
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

	/*TC上送,脚本通知交易无需上送此交易发起方式域*/
	if(memcmp(ewp_info->consumer_transdealcode, "999997", 6)!=0 && memcmp(ewp_info->consumer_transtype, "0620", 4)!=0  )
	{
		/*交易发起方式*/
		memcpy(ewp_info->translaunchway, rcvbuf + offset, 1);
		ewp_info->translaunchway[1] = 0;
		offset += 1;
		check_len -= 1;
		
		#ifdef __LOG__
			dcs_log(0, 0, "translaunchway=[%s]", ewp_info->translaunchway);
		#endif
	}	
	
	/*KSN编号 20字节*/
	memcpy(ewp_info->consumer_ksn, rcvbuf + offset, 20);
	ewp_info->consumer_ksn[20] = 0;
	offset += 20;
	check_len -= 20;
	
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
		dcs_log(0,0,"consumer_ksn=[%s], consumer_phonesnno=[%s]", ewp_info->consumer_ksn, ewp_info->consumer_phonesnno);
	#endif
	/*脚本上送通知,TC上送无需上送*/
	if(memcmp(ewp_info->consumer_transtype, "0620", 4)!=0 && memcmp(ewp_info->consumer_transdealcode, "999997", 6)!=0 )
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
	
		/*卡号*/		
		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy(tmpbuf, rcvbuf + offset, 19);
		tmpbuf[19] = 0;
		pub_rtrim_lp(tmpbuf, 19, tmpbuf, 1);
		memcpy(ewp_info->consumer_cardno, tmpbuf, strlen(tmpbuf));
		ewp_info->consumer_cardno[strlen(tmpbuf)]=0;
		offset += 19;
		check_len -= 19;
//		dcs_log(0, 0, "consumer_cardno=[%s]", ewp_info->consumer_cardno);
	}
	//转账类和转账汇款类有 转入卡号,手续费等特殊域
	if(memcmp(ewp_info->consumer_transtype, "0620", 4)!=0 
	&&(memcmp(ewp_info->consumer_transdealcode, "480000", 6)== 0
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
			/*ewp上送的姓名为GBK，需要转换为UTF8格式*/
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
			#ifdef __LOG__
				dcs_log(0, 0, "ewp_info->incdname=[%s], ewp_info->incdbkno=[%s]", ewp_info->incdname, ewp_info->incdbkno);
			#endif
		}
		else if(memcmp(ewp_info->consumer_transdealcode, "190000", 6)==0)
			dcs_log(0, 0, "这是一个信用卡还款类的交易...");
	
		/*金额 12个字节 */
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
	}
	/*本段为消费交易*/
	else if(memcmp(ewp_info->consumer_transtype, "0620", 4)!=0 &&memcmp(ewp_info->consumer_transdealcode, "000000", 6)==0)
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
	}

	/*TC上送,脚本通知无需上送*/
	if(memcmp(ewp_info->consumer_transdealcode, "999997", 6)!=0 && memcmp(ewp_info->consumer_transtype, "0620", 4)!=0 )
	{
		/*二磁道信息*/
		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy(tmpbuf, rcvbuf + offset, 2);//二磁长度
		offset += 2;
		check_len -= 2;
		temp_len = atoi(tmpbuf);
		if(temp_len>37)
		{	
			strcpy(ewp_info->retcode, "F1");
			dcs_log(0, 0, "EWP数据不正确,二磁长度过长为%d",temp_len);	
			return 0;
		}
		else if(temp_len > 0)
		{
			memcpy(ewp_info->trackno2, rcvbuf + offset, temp_len);//二磁内容
			ewp_info->trackno2[temp_len] = 0;
			offset +=temp_len;
			check_len -= temp_len;
		}

		/*三磁道信息*/
		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy(tmpbuf, rcvbuf + offset, 3);//三磁长度
		offset += 3;
		check_len -= 3;
		temp_len = atoi(tmpbuf);
		if(temp_len>104)
		{
			strcpy(ewp_info->retcode, "F1");
			dcs_log(0, 0, "EWP数据不正确,三磁长度过长为%d",temp_len);	
			return 0;
		}
		else if(temp_len > 0)
		{
			memcpy(ewp_info->trackno3, rcvbuf + offset, temp_len);//三磁内容
			ewp_info->trackno3[temp_len] = 0;
			offset +=temp_len;
			check_len -= temp_len;
		}		
		#ifdef __TEST__
			dcs_log(0, 0, "trackno2=[%s]", ewp_info->trackno2);
			dcs_log(0, 0, "trackno3=[%s]", ewp_info->trackno3);
		#endif
				
		/*密码*/
		memcpy(ewp_info->consumer_password, rcvbuf + offset, 16);
		ewp_info->consumer_password[16] = 0;
		offset += 16;
		check_len -= 16;
		
		#ifdef __TEST__
			dcs_log(0, 0, "consumer_password=[%s]",ewp_info->consumer_password);
		#endif
		
		memcpy(ewp_info->macinfo, rcvbuf + offset, 8);
		ewp_info->macinfo[8] = 0;
		offset += 8;
		check_len -= 8;

		#ifdef __LOG__
			dcs_log(0, 0, "macinfo=[%s]", ewp_info->macinfo);
		#endif
		/*余额查询不需要发送短信*/
		if(memcmp(ewp_info->consumer_transdealcode, "310000", 6)!=0 )
		{
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
		}
		
		/* IC卡标识 */
		memcpy(ewp_info->icflag, rcvbuf + offset, 1);
		dcs_log(0, 0, "IC卡标识[%s]", ewp_info->icflag);
		offset += 1;
		check_len -= 1;
		
		if(memcmp(ewp_info->icflag, "1", 1) == 0)
		{
			/*降级标志位fallback*/
			memcpy(ewp_info->fallback_flag, rcvbuf + offset, 1);
			dcs_log(0, 0, "降级标志[%s]", ewp_info->fallback_flag);
			offset += 1;                                    
			check_len -= 1;
		}

		/* 卡片序列号存在标识  */
		memcpy(ewp_info->cardseq_flag, rcvbuf + offset, 1);
		dcs_log(0, 0, "卡片序列号存在标识[%s]", ewp_info->cardseq_flag);
		offset += 1;
		check_len -= 1;
		
		if( memcmp(ewp_info->cardseq_flag, "1", 1)==0 )
		{
			/* 卡片序列号 */
			memcpy(ewp_info->cardseq, rcvbuf + offset, 3);
			dcs_log(0, 0, "卡片序列号[%s]", ewp_info->cardseq);
			offset += 3;                                    
			check_len -= 3;			
		}
		/*IC卡数据域长度*/
		memcpy(ewp_info->filed55_length, rcvbuf + offset, 3);
		dcs_log(0, 0, "IC卡数据域[%s]", ewp_info->filed55_length);
		offset += 3;
		check_len -= 3;
		
		if( memcmp(ewp_info->filed55_length, "000", 3)!=0 )
		{
			/*IC卡数据域*/
			temp_len = atoi(ewp_info->filed55_length);
			memcpy(ewp_info->filed55, rcvbuf + offset, temp_len);
			dcs_log(0, 0, "IC卡数据域[%s]", ewp_info->filed55);
			offset += temp_len;
			check_len -= temp_len;
		}
	}
	//TC值上送,脚本通知要有系统参考号
	else if(memcmp(ewp_info->consumer_transdealcode, "999997", 6) ==0 || memcmp(ewp_info->consumer_transtype, "0620", 4)==0 )
	{
		if(memcmp(ewp_info->consumer_transdealcode, "999997", 6) ==0)
			dcs_log(0, 0, "TC值上送交易");
		else
			dcs_log(0, 0, "IC卡脚本通知");
		/*系统参考号*/
		memcpy(ewp_info->consumer_sysreferno, rcvbuf + offset, 12);
		ewp_info->consumer_sysreferno[12] = 0;
		offset += 12;
		check_len -= 12;		
		#ifdef __LOG__
			dcs_log(0, 0, "consumer_sysreferno=[%s]", ewp_info->consumer_sysreferno);
		#endif

		//原交易日期
		memcpy(ewp_info->originaldate, rcvbuf + offset, 8);
		offset += 8;
		check_len -= 8;
		dcs_log(0, 0, "原交易日期[%s]", ewp_info->originaldate);

		/*IC卡数据域长度*/
		memcpy(ewp_info->filed55_length, rcvbuf + offset, 3);
		dcs_log(0, 0, "IC卡数据域[%s]", ewp_info->filed55_length);
		offset += 3;
		check_len -= 3;
		
		if( memcmp(ewp_info->filed55_length, "000", 3)!=0 )
		{
			/*IC卡数据域*/
			temp_len = atoi(ewp_info->filed55_length);
			memcpy(ewp_info->filed55, rcvbuf + offset, temp_len);
			dcs_log(0, 0, "IC卡数据域[%s]", ewp_info->filed55);
			offset += temp_len;
			check_len -= temp_len;
		}
		else
		{
			dcs_log(0, 0, "IC卡数据域信息为空");
			strcpy(ewp_info->retcode, "F1");
			return -1;
		}
		memcpy(ewp_info->macinfo, rcvbuf + offset, 8);
		ewp_info->macinfo[8] = 0;
		offset += 8;
		check_len -= 8;

		#ifdef __LOG__
			dcs_log(0, 0, "macinfo=[%s]", ewp_info->macinfo);
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
				
	#ifdef __LOG__
		dcs_log(0, 0, "解析EWP数据, check_len=[%d]", check_len);
		dcs_log(0, 0, "解析EWP数据, offset=[%d]", offset);
		
		dcs_log(0, 0, "check_len=[%d]", check_len);
	#endif
	
	/*poitcde_flag 标志位赋值 0:存量磁条, 1:IC 不降级, 2:IC 降级, 3:纯磁条*/	
	memset(ewp_info->serinputmode, 0x00, sizeof(ewp_info->serinputmode));
	if(memcmp(ewp_info->icflag, "1", 1)==0)
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
	
	if(memcmp(ewp_info->consumer_password, "FFFFFFFFFFFFFFFF", 16) == 0 || memcmp(ewp_info->consumer_password, "ffffffffffffffff", 16) == 0)
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


/*
组返回MPOS_EWP平台的报文
code = 0,前置平台返回EWP的错误信息报文，pep_jnls为null，返回数据从ewp_info中取；
code = 1,前置平台返回EWP的交易响应报文，ewp_info为null，返回数据从pep_jnls中取；
*/
int WriteEWP_Mpos(ISO_data siso, EWP_INFO ewp_info, PEP_JNLS pep_jnls, int code)
{
	int gs_comfid = -1, s = 0;
	char buffer1[1024], buffer2[1024];
	
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
	s = DataBackToMPOS(code, buffer2, pep_jnls, ewp_info, siso);

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
	#endif
		fold_write(gs_comfid, -1, buffer2, s);
	}
	else
	{
	#ifdef __LOG__
		dcs_debug(buffer2,s,"[%s] data_buf len=%d,foldId=%d", pep_jnls.trnsndp, s, gs_comfid);
	#endif
		fold_write(gs_comfid, -1, buffer2, s);
	}
	return 0;
}

/*
组返回EWP_MPOS平台的报文
code = 0,前置平台返回EWP的错误信息报文，返回数据从ewp_info中取；
code = 1,前置平台返回EWP的交易响应报文，返回数据从pep_jnls中取；
*/
int DataBackToMPOS(int code, char *buffer, PEP_JNLS pep_jnls, EWP_INFO ewp_info, ISO_data siso)
{
	char tmpbuf[512], ewpinfo[1024], temp_asc_55[512];
	char buf[100];
	int ewpinfo_len=0, len=0, s=0;
		
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(ewpinfo, 0, sizeof(ewpinfo));
	memset(temp_asc_55, 0, sizeof(temp_asc_55));
	memset(buf, 0, sizeof(buf));
	
	/*获取IC卡公钥参数下载标识,默认不更新*/
	memcpy(ewp_info.download_pubkey_flag, "1", 1);
	memcpy(ewp_info.download_para_flag, "1", 1);	
	
	if(code==0)
	{
		dcs_log(0, 0, "FAILED");	
		/*4个字节的交易类型数据*/
		if(memcmp(ewp_info.consumer_transtype, "0200", 4)==0)
			sprintf( buffer, "%s", "0210");
		else if (memcmp(ewp_info.consumer_transtype, "0320", 4)==0)//TC值上送
			sprintf( buffer, "%s", "0330");
		else if(memcmp(ewp_info.consumer_transtype,"0620",4)==0)
			sprintf( buffer, "%s", "0630");
		else 
			sprintf( buffer, "%s", ewp_info.consumer_transtype); 
			
		#ifdef __LOG__
			dcs_log(0, 0, "ewp_info.consumer_transtype=%s", ewp_info.consumer_transtype);
		#endif
		dcs_log(0, 0, "交易类型 [%s]", buffer);
		 /*6个字节的交易处理码*/  
		sprintf( buffer, "%s%s", buffer, ewp_info.consumer_transdealcode);
		dcs_log(0, 0, "交易处理码 [%s]", ewp_info.consumer_transdealcode);
		
		/*8个字节  接入系统发送交易的日期*/ 
		sprintf( buffer, "%s%s",buffer, ewp_info.consumer_sentdate);
		dcs_log(0, 0, "发送交易的日期 [%s]", ewp_info.consumer_sentdate);
		 /*6个字节  接入系统发送交易的时间*/
		 
		sprintf( buffer, "%s%s", buffer, ewp_info.consumer_senttime);
		dcs_log(0, 0, "发送交易的时间 [%s]", ewp_info.consumer_senttime);
		/*4个字节  接入系统的编号EWP：0100*/
		sprintf( buffer, "%s%s", buffer, ewp_info.consumer_sentinstitu); 
		dcs_log(0, 0, "发送机构编号 [%s]", ewp_info.consumer_sentinstitu);
		/*3个字节 */
		sprintf( buffer, "%s%s",buffer, ewp_info.consumer_transcode); 
		dcs_log(0, 0, "交易码 [%s]", ewp_info.consumer_transcode);
		/*1个字节的交易发起方式*/
		if( memcmp(ewp_info.consumer_transdealcode, "999997", 6)!=0 && memcmp(ewp_info.consumer_transtype, "0620", 4)!=0 )
		{
			sprintf( buffer, "%s%s", buffer, ewp_info.translaunchway); 
			dcs_log(0, 0, "交易发起方式 [%s]", ewp_info.translaunchway);
		}
		/*20个字节KSN编号*/  
		sprintf( buffer, "%s%s", buffer, ewp_info.consumer_ksn);
		dcs_log(0, 0, "KSN编号 [%s]", ewp_info.consumer_ksn);

		/*20个字节 机具编号左对齐 不足补空格*/	
		sprintf( buffer, "%s%-20s", buffer, ewp_info.consumer_phonesnno);
		dcs_log(0, 0, "机具编号 [%s]", ewp_info.consumer_phonesnno);

		if( memcmp(ewp_info.consumer_transdealcode, "999997", 6)!=0  && memcmp(ewp_info.consumer_transtype, "0620", 4)!=0)
		{
			/*11个字节 手机号，例如：1502613236*/	
			sprintf( buffer, "%s%s", buffer, ewp_info.consumer_username);
			dcs_log(0, 0, "用户名 [%s]", ewp_info.consumer_username);
			/*20个字节的订单号， 左对齐 不足补空格*/	
			sprintf( buffer, "%s%-20s", buffer, ewp_info.consumer_orderno);
			dcs_log(0, 0, "订单号 [%s]", buffer);
				
			/*20个字节的商户编号*/
			sprintf( buffer, "%s%20s", buffer, ewp_info.consumer_merid);
			dcs_log(0, 0, "商户编号 [%s]", ewp_info.consumer_merid);
				
			/*19个字节卡号 左对齐 不足补空格*/
			sprintf( buffer,"%s%-19s", buffer, ewp_info.consumer_cardno);
//			dcs_log(0, 0, "卡号 [%s]", ewp_info.consumer_cardno);
		
		}
		getbit(	&siso, 39, (unsigned char *)ewp_info.consumer_responsecode);
		ewpinfo_len = GetDetailInfo(ewp_info.consumer_responsecode, ewp_info.consumer_transcode, ewpinfo);
		if(ewpinfo_len == -1)
			ewpinfo_len = 0;

		dcs_log(0, 0, "取交易应答描述成功[%s]", ewpinfo);
		dcs_log(0, 0, "ewp_info.consumer_transtype[%s]", ewp_info.consumer_transtype);
		dcs_log(0, 0, "ewp_info.consumer_transdealcode[%s]", ewp_info.consumer_transdealcode);
		
		if( memcmp(ewp_info.consumer_transtype, "0620", 4)!=0 
		&& (memcmp(ewp_info.consumer_transdealcode, "480000", 6)==0
		  ||memcmp(ewp_info.consumer_transdealcode, "190000", 6)==0))
		{
			/*19个字节转入卡号 左对齐 不足补空格*/
			sprintf( buffer, "%s%-19s", buffer, ewp_info.incardno);
			if(memcmp(ewp_info.consumer_transdealcode, "480000", 6)==0)
			{
				dcs_log(0, 0, "转账汇款交易.");
				/*50个字节转入卡姓名 左对齐 不足补空格*/
				memset(tmpbuf, 0x00, sizeof(tmpbuf));			
				/*返回ewp的姓名格式需要为GBK*/	
				u2g(ewp_info.incdname, strlen(ewp_info.incdname), tmpbuf, sizeof(tmpbuf));
				sprintf( buffer, "%s%-50s", buffer, tmpbuf);
				/*25个字节转入卡行号  左对齐 不足补空格*/
				sprintf( buffer, "%s%-25s", buffer, ewp_info.incdbkno);
			}
			else if(memcmp(ewp_info.consumer_transdealcode, "190000", 6)==0)
				dcs_log(0, 0, "信用卡还款类的交易.");
			
			/*12个字节 金额右对齐 不足补0*/
			sprintf( buffer, "%s%012s", buffer,ewp_info.consumer_money);	
			sprintf(buffer, "%s%s", buffer, ewp_info.transfee);
			
			/*2个字节 应答码*/
			sprintf( buffer, "%s%2s", buffer, ewp_info.consumer_responsecode);
		
			/*6个字节 前置系统流水*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			if( getbit(	&siso,11,(unsigned char *)tmpbuf) <= 0 )
				sprintf( buffer, "%s%s", buffer, "000000");
			else
				sprintf( buffer, "%s%s", buffer, tmpbuf);
		
			/*12个字节 系统参考号*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			if( getbit( &siso, 37, (unsigned char *)tmpbuf) <= 0 )
				sprintf( buffer, "%s%12s", buffer, "000000000000");
			else
				sprintf( buffer, "%s%s", buffer, tmpbuf);
			
			/*6个字节的授权码*/
			memset(tmpbuf, 0, sizeof(tmpbuf));
			if( getbit( &siso, 38, (unsigned char *)tmpbuf) <= 0 )
				sprintf( buffer, "%s%s", buffer, "000000");
			else
				sprintf( buffer, "%s%s", buffer, tmpbuf);
				
			dcs_log(0, 0, "authcode = [%s]", tmpbuf);	
			/*1个字节表示应答码内容的长度*/
			/*应答码描述信息，utf-8表示*/
			len = strlen(buffer);
			buffer[len] = ewpinfo_len;
			len +=1;
			memcpy(buffer+len, ewpinfo, ewpinfo_len);
			len +=ewpinfo_len;
		
			/*IC卡数据域,转账汇款下发，信用卡还款下发*/
			if( memcmp(ewp_info.icflag, "0", 1)!=0)
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
	
			sprintf(buffer+len, "%s%s", ewp_info.download_pubkey_flag, ewp_info.download_para_flag);
			len +=2;
			
			
			/*20个字节的自定义域原样返回给EWP*/
			memcpy(buffer+len,ewp_info.selfdefine, getstrlen(ewp_info.selfdefine));
			len +=20;
			return len;
		}
		
		/*消费类的交易如：订单支付 000000*/
		else if(memcmp(ewp_info.consumer_transtype, "0620", 4)!=0 && memcmp(ewp_info.consumer_transdealcode, "000000", 6)==0)
		{
			dcs_log(0, 0, "消费类的交易.");
			/*12个字节 金额右对齐 不足补0*/
			sprintf( buffer, "%s%012s", buffer, ewp_info.consumer_money);	
			
			/*2个字节 应答码*/
			sprintf( buffer, "%s%2s", buffer, ewp_info.consumer_responsecode);
		
			/*6个字节 前置系统流水*/
			memset(tmpbuf, 0, sizeof(tmpbuf));
			if( getbit(&siso, 11, (unsigned char *)tmpbuf) <= 0 )
				sprintf( buffer, "%s%s", buffer, "000000");
			else
				sprintf( buffer, "%s%s", buffer, tmpbuf);
		
			/*12个字节 系统参考号*/
			memset(tmpbuf, 0, sizeof(tmpbuf));
			if( getbit( &siso, 37, (unsigned char *)tmpbuf) <= 0 )
				sprintf( buffer, "%s%s", buffer, "000000000000");
			else
				sprintf( buffer, "%s%12s", buffer, tmpbuf);
			
			/*6个字节的授权码*/
			memset(tmpbuf, 0, sizeof(tmpbuf));
			if( getbit( &siso, 38, (unsigned char *)tmpbuf) <= 0 )
				sprintf( buffer, "%s%s", buffer, "000000");
			else
				sprintf( buffer, "%s%s", buffer, tmpbuf);
				
			dcs_log(0,0,"authcode = [%s]",tmpbuf);	
			/*1个字节表示应答码内容的长度*/
			/*应答码描述信息，utf-8表示*/
			
			len = strlen(buffer);
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
			sprintf(buffer+len, "%s%s", ewp_info.download_pubkey_flag, ewp_info.download_para_flag);
			len +=2;
			/*20个字节的自定义域原样返回给EWP*/
			memcpy(buffer+len,ewp_info.selfdefine, getstrlen(ewp_info.selfdefine));
			len +=20;
			return len;
				
		}
		
		/*余额查询的交易*/
		else if(memcmp(ewp_info.consumer_transtype, "0620", 4)!=0 && memcmp(ewp_info.consumer_transdealcode, "310000", 6)==0)
		{
			dcs_log(0, 0, "余额查询的交易.");	
			/*2个字节 应答码*/
			sprintf( buffer, "%s%2s", buffer, ewp_info.consumer_responsecode);
		
			/*6个字节 前置系统流水*/
			memset(tmpbuf, 0, sizeof(tmpbuf));
			if( getbit( &siso, 11, (unsigned char *)tmpbuf) <=0 )
				sprintf( buffer, "%s%s", buffer, "000000");
			else	
				sprintf( buffer, "%s%s", buffer, tmpbuf);
		
			/*12个字节 系统参考号*/
			memset(tmpbuf, 0, sizeof(tmpbuf));
			if(getbit( &siso, 37, (unsigned char *)tmpbuf) <=0 )
				sprintf( buffer,"%s%s", buffer, "000000000000");
			else	
				sprintf( buffer, "%s%s", buffer, tmpbuf);
			
			/*应答码为00成功时，才返回金额。否则全为0*/
			/*12个字节 金额右对齐 不足补空格*/
			sprintf( buffer, "%s%024d", buffer, 0);	
			
			
			/*6个字节的授权码*/
			memset(tmpbuf, 0, sizeof(tmpbuf));
			if( getbit( &siso, 38, (unsigned char *)tmpbuf) <= 0 )
				sprintf( buffer, "%s%s", buffer, "000000");
			else	
				sprintf( buffer, "%s%s", buffer, tmpbuf);
			dcs_log(0, 0, "authcode = [%s]", tmpbuf);	
			len = strlen(buffer);
			
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
			
				/*存量机器除外，都需检查并下发IC卡参数公钥更新标识，1表示需要处理，0表示无需处理 */			
				sprintf(buffer+len, "%s%s", ewp_info.download_pubkey_flag, ewp_info.download_para_flag);
				len +=2;
			}
			
			/*20个字节的自定义域原样返回给EWP*/
			memcpy(buffer+len,ewp_info.selfdefine, getstrlen(ewp_info.selfdefine));
			len +=20;
			return len;
		}
		/*TC值上送交易错误返回*/
		else if(memcmp(ewp_info.consumer_transdealcode, "999997", 6)==0)
		{
			//dcs_log(0, 0, "TC值上送.");	
			/*2个字节 应答码*/
			sprintf( buffer, "%s%2s", buffer, ewp_info.consumer_responsecode);
			len = strlen(buffer);
			buffer[len] = ewpinfo_len;
			len +=1;
			memcpy(buffer+len, ewpinfo, ewpinfo_len);
			len +=ewpinfo_len;
				
			/*20个字节的自定义域原样返回给EWP*/
			memcpy(buffer+len,ewp_info.selfdefine, getstrlen(ewp_info.selfdefine));
			len +=20;
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
			memcpy(buffer+len, ewp_info.selfdefine, getstrlen(ewp_info.selfdefine));
			len +=20;
			return len;	
		}
	}
	else if(code==1)
	{	
		dcs_log(0, 0, "SUCESS");		
		/*4个字节的交易类型数据*/
		if(memcmp(pep_jnls.msgtype, "0200", 4)==0)
			sprintf( buffer, "%s", "0210");
		else if(memcmp(pep_jnls.msgtype, "0620",4)==0)//脚本
			sprintf( buffer, "%s", "0630");	
		 /*6个字节的交易处理码*/  
		sprintf( buffer, "%s%s",buffer, ewp_info.consumer_transdealcode);
		/*8个字节  接入系统发送交易的日期*/
		sprintf( buffer, "%s%s",buffer, pep_jnls.trndate);  
		 /*6个字节  接入系统发送交易的时间*/ 
		sprintf( buffer, "%s%s",buffer, pep_jnls.trntime);  
		
		/*4个字节  接入系统的编号EWP：0100 */
		memset(tmpbuf,0,sizeof(tmpbuf));
		memcpy(tmpbuf,pep_jnls.sendcde,4);
		sprintf( buffer, "%s%s",buffer,tmpbuf);   
		
		/*3个字节  区分业务类型订单支付：210??*/
		sprintf( buffer, "%s%s",buffer, pep_jnls.trancde); 
		/*1个字节的交易发起方式*/
		if(memcmp(pep_jnls.msgtype,"0620",4)!=0)
			sprintf( buffer, "%s%s",buffer, ewp_info.translaunchway); 
		
		/*20个字节KSN编号*/
		memset(tmpbuf, 0, sizeof(tmpbuf));
		memset(buf, 0, sizeof(buf));
		
		memcpy(buf, pep_jnls.samid, 5);
		
		bcd_to_asc((unsigned char *)tmpbuf, (unsigned char *)buf, 10, 0);
		sprintf( buffer, "%s%s%s", buffer, tmpbuf, pep_jnls.samid+5); 

		dcs_log(0,0,"KSN编号[%s%s]", tmpbuf, pep_jnls.samid+5);
		
		/*20个字节 手机编号(机具编号)左对齐 不足补空格??*/
		pep_jnls.termidm[20] ='\0';
		sprintf( buffer,"%s%-20s",buffer,pep_jnls.termidm);
		
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
//			dcs_log(0,0,"pep_jnls.outcdno = [%s]",pep_jnls.outcdno);
				
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
		&&(memcmp(ewp_info.consumer_transdealcode, "480000",6)==0
		||memcmp(ewp_info.consumer_transdealcode, "190000",6)==0))
		{		
			/*19个字节转入卡号， 左对齐 不足补空格??*/
			pub_rtrim_lp(pep_jnls.intcdno, strlen(pep_jnls.intcdno), pep_jnls.intcdno, 1);
//			dcs_log(0,0,"pep_jnls.intcdno = [%s]", pep_jnls.intcdno);
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
				sprintf( buffer,"%s%s",buffer,pep_jnls.syseqno);		
			/*6个字节 授权码*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			if( getbit( &siso,38,(unsigned char *)tmpbuf) <= 0 )
				sprintf( buffer,"%s%s",buffer,"000000");
			else
				sprintf( buffer,"%s%s",buffer,tmpbuf);
			dcs_log(0,0,"pep_jnls.authcode = [%s]",tmpbuf);	
			len = strlen(buffer);
			buffer[len] = ewpinfo_len;
			len +=1;
			
			memcpy(buffer+len, ewpinfo, ewpinfo_len);
			len +=ewpinfo_len;
		
			/*IC卡数据域,转账汇款下发，信用卡还款下发 上线修改*/
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
				
			sprintf(buffer+len, "%s%s", ewp_info.download_pubkey_flag, ewp_info.download_para_flag);
			len +=2;
			
			/*20个字节的自定义域原样返回给EWP*/
			memcpy(buffer+len, ewp_info.selfdefine, getstrlen(ewp_info.selfdefine));
			len +=20;
			return len;		 
		
		}
		
		/*消费类的交易*/
		else if(memcmp(ewp_info.consumer_transtype, "0620", 4)!=0 && memcmp(ewp_info.consumer_transdealcode,"000000",6)==0)
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
			sprintf(buffer+len, "%s%s", ewp_info.download_pubkey_flag, ewp_info.download_para_flag);
			len +=2;
			/*20个字节的自定义域原样返回给EWP*/
			memcpy(buffer+len,ewp_info.selfdefine, getstrlen(ewp_info.selfdefine));
			len +=20;
			return len;							
		}
		/*余额查询类的交易*/
		else if(memcmp(ewp_info.consumer_transtype, "0620", 4)!=0 && memcmp(ewp_info.consumer_transdealcode,"310000",6)==0)
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
			
			//dcs_log(0,0,"buffer[%s]",buffer);
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
				
			/*IC卡数据域*/
			if(memcmp(ewp_info.icflag, "0", 1)!=0)
			{	
				s = getbit(&siso, 55,(unsigned char *)tmpbuf);
				if(s <= 0)
		 		{
		 			dcs_log(0, 0, "bit55 is null!");
					memcpy(buffer+len, "000", 3);
					len +=3;
					//dcs_debug(buffer,len,"buffer:");
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
			sprintf(buffer+len, "%s%s", ewp_info.download_pubkey_flag, ewp_info.download_para_flag);
			len +=2;
				
			/*20个字节的自定义域原样返回给EWP*/
			memcpy(buffer+len, ewp_info.selfdefine, getstrlen(ewp_info.selfdefine));
			len +=20;
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
			
			memcpy(buffer+len, ewp_info.selfdefine, getstrlen(ewp_info.selfdefine));
			len +=20;
			return len;	
		}
	}
	return len;
}

	
int MposSendTcValue(EWP_INFO ewp_info, PEP_JNLS pep_jnls, MSGCHGINFO msginfo, ISO_data siso, KSN_INFO ksn_info)
{
	int len = 0;
	int rtn = -1;
	char oldtrace[7]; //原交易流水号
	char temp_bcd_55[512];
	char tmpbuf[1024];
	
	memset(oldtrace, 0x00, sizeof(oldtrace));
	memset(temp_bcd_55, 0x00, sizeof(temp_bcd_55));
	memset(tmpbuf, 0x00, sizeof(tmpbuf));

	dcs_log(0, 0, "组织TC值报文上送到核心");
	
	setbit(&siso, 0, (unsigned char *)"0320", 2);
	setbit(&siso, 2, (unsigned char *)pep_jnls.outcdno, strlen(pep_jnls.outcdno));
	setbit(&siso, 3, (unsigned char *)msginfo.process, 6);
	
	setbit(&siso, 4, (unsigned char *)pep_jnls.tranamt, 12);
	
	sprintf(oldtrace, "%06d", pep_jnls.trace);
	setbit(&siso, 11, (unsigned char *)oldtrace, 6);
	
	if(getstrlen(ewp_info.translaunchway) == 0)
	{
		memcpy(ewp_info.translaunchway, "1", 1);
	}
	
	sprintf(tmpbuf, "%s%-15s%s%s%s%06s%04ld", ewp_info.consumer_ksn, ewp_info.consumer_username, ewp_info.translaunchway, "01", ewp_info.consumer_transcode, "000000", ksn_info.channel_id);
	
	dcs_log(0, 0, "21域  length[%d]  content[%s]", strlen(tmpbuf), tmpbuf);

	//T0_flag,agents_code写死为0    modify begin 150421 
	memcpy(tmpbuf+47, "0000000", 7);
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
	
	//授权码固定填写000000，核心不需要此字段
	memset(tmpbuf, 0, sizeof(tmpbuf));
	sprintf(tmpbuf,"%s%s%s%06d", "000000", pep_jnls.syseqno, pep_jnls.revdate, pep_jnls.trace);
	setbit(&siso, 61, (unsigned char *)tmpbuf, 34);
	
	//发送报文  
	rtn=WriteXpe(siso);
	if(rtn == -1)
	{
		dcs_log(0,0,"发送信息给核心失败");
		return -1;
	}
	return 0;
}


