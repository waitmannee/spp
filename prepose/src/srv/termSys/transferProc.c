#include "ibdcs.h"
#include "trans.h"
#include "memlib.h"

extern struct ISO_8583 *iso8583;
extern struct ISO_8583 iso8583_conf[128];
extern struct ISO_8583 iso8583conf[128];
/*
	转账汇款交易处理
*/
void transferProc(char *srcBuf, int iFid, int iMid, int iLen)
{
	//系统自带的时间结构体
	struct  tm *tm;   
	time_t  t;
	//当前日期
	char currentDate[14+1];
	//中间变量
	char tmpbuf[1024], databuf[PUB_MAX_SLEN + 1];
	char temp_bcd_55[512];
	char bank_name[64];

	//转出卡号
	char extkey[20];

	//终端信息表
	TERMMSGSTRUCT termbuf;

	//数据库交易信息保存的结构体，对于数据库表pep_jnls
	PEP_JNLS pep_jnls; 

	//前置系统流水
	char trace[7];

	//智能终端个人收单商户配置信息表
	POS_CUST_INFO pos_cust_info;
	
	//终端密钥信息表，通过psam卡号查询数据库表samcard_info数据库表之后，放入该结构体中
	SECUINFO secuinfo;
	//ISO8583结构体，送往核心系统的数据
	ISO_data siso;

	//保存到缓存数据memcached结构体
	KEY_VALUE_INFO key_value_info;
	EWP_INFO ewp_info;
	MSGCHGINFO	msginfo;
	TRANSFER_INFO transfer_info;
	char curToalAmt[13];

	char outCardType = ' ';
	int datalen = 0, rtn = 0, s = 0, len = 0;
	
	memset(&key_value_info, 0, sizeof(KEY_VALUE_INFO));
	memset(&ewp_info, 0, sizeof(EWP_INFO));
	memset(&msginfo, 0, sizeof(MSGCHGINFO));
	memset(&transfer_info, 0, sizeof(TRANSFER_INFO));
	memset(currentDate, 0, sizeof(currentDate));
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(databuf, 0, sizeof(databuf));
	memset(temp_bcd_55, 0, sizeof(temp_bcd_55));
	memset(bank_name, 0, sizeof(bank_name));
	memset(extkey, 0, sizeof(extkey));
	memset(trace, 0, sizeof(trace));
	memset(&termbuf, 0, sizeof(TERMMSGSTRUCT));
	memset(&pep_jnls, 0, sizeof(PEP_JNLS));
	memset(&secuinfo, 0, sizeof(SECUINFO));
	memset(&siso, 0, sizeof(ISO_data));
	memset(&pos_cust_info, 0, sizeof(POS_CUST_INFO));
	memset(curToalAmt, 0, sizeof(curToalAmt));
	
	
#ifdef __LOG__
	dcs_debug(srcBuf, iLen, "转账汇款交易, iFid=%d, iMid=%d, iLen=%d", iFid, iMid, iLen);
#else
	dcs_log(0, 0, "转账汇款交易, iFid=%d, iMid=%d, iLen=%d", iFid, iMid, iLen);
#endif
	
	memcpy(iso8583_conf, iso8583conf, sizeof(iso8583conf));
	iso8583 = &iso8583_conf[0];
  	SetIsoHeardFlag(0);
    SetIsoFieldLengthFlag(1);
	time(&t);
    tm = localtime(&t);
    
    sprintf(currentDate, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
    sprintf(currentDate+8, "%02d%02d%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
	
	pep_jnls.trnsndpid = iFid ;
	/*得到交易报文类型,整个报文前4个字节*/
	memcpy(pep_jnls.trnsndp, srcBuf, 4);
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
	
	//TODO  解析EWP交易报文
	analysisTransferRQ(databuf, datalen, &ewp_info);
	if(memcmp(ewp_info.retcode, "00", 2)!= 0 )
	{
		dcs_log(0, 0, "解析账单信息失败");
		return;
	}
	
	//获得psam卡的密钥信息
	rtn = GetSecuInfo(ewp_info.consumer_psamno, &secuinfo);
	if(rtn < 0)
	{
		dcs_log(0, 0, "查询samcard_info失败, psam卡未找到, consumer_psamno=%s", ewp_info.consumer_psamno);
		setbit(&siso, 39, (unsigned char *)RET_CODE_FC, 2);
		memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_FC, 2);
		errorTradeLog(ewp_info, pep_jnls.trnsndp, "psam卡未找到");
		RetToEWP(siso, ewp_info, pep_jnls, 0);
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
		errorTradeLog(ewp_info, pep_jnls.trnsndp, "查询msgchg_info失败,交易未开通");
		RetToEWP(siso, ewp_info, pep_jnls,0);
		return;
	}
	else
	{	
		if(memcmp(msginfo.flagpe, "Y", 1)!=0 && memcmp(msginfo.flagpe, "y", 1)!= 0 )
		{
			dcs_log(0, 0, "由于系统或者其他原因，交易暂时关闭");
			setbit(&siso, 39, (unsigned char *)RET_TRADE_CLOSED, 2);
			memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_TRADE_CLOSED, 2);
			errorTradeLog(ewp_info, pep_jnls.trnsndp, "由于系统或者其他原因，交易暂时关闭");
			RetToEWP(siso, ewp_info, pep_jnls, 0);
			return;
		}
	}
	memcpy(transfer_info.trans_name, msginfo.remark, getstrlen(msginfo.remark));
	//判断该笔订单是否发起过代付
	rtn = JudgeOrderInfo(ewp_info.consumer_orderno);
	if(rtn >0)
	{
		dcs_log(0, 0, "该笔订单已经发起过扣款");
		setbit(&siso, 39, (unsigned char *)"HP", 2);
		memcpy(ewp_info.consumer_responsecode, (unsigned char *)"HP", 2);
		errorTradeLog(ewp_info, pep_jnls.trnsndp, "该笔订单已经发起过扣款");
		RetToEWP(siso, ewp_info, pep_jnls, 0);
		return;
	}
	/*单笔金额风控不超过2万,单日5万*/
	//if(memcmp(ewp_info.consumer_money, "000002000000", 12)> 0)
	if(memcmp(ewp_info.consumer_money, msginfo.d_amtlimit, 12)> 0)
	{
		dcs_log(0, 0, "单笔金额超限,最大金额为[%s]", msginfo.d_amtlimit);
		setbit(&siso, 39, (unsigned char *)RET_CODE_F6, 2);
		memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_F6, 2);
		errorTradeLog(ewp_info, pep_jnls.trnsndp, "单笔金额超限");
		RetToEWP(siso, ewp_info, pep_jnls, 0);
		return;
	}
	//判断日累计金额
	rtn = getCurTotalAmt(curToalAmt, currentDate,ewp_info.consumer_cardno);
	if(rtn == -1)
	{
		dcs_log(0, 0, "获取当日当前总金额失败");
		return;
	}
	//累加和 000005000000 
	memset(tmpbuf, '0', sizeof(tmpbuf));
	tmpbuf[12] = '\0';

	rtn = SumAmt(ewp_info.consumer_money, curToalAmt, tmpbuf);
	if(rtn == -1)
	{
		dcs_log(0, 0, "求和错误");
		return;
	}
	#ifdef __TEST__
		dcs_log(0, 0, "add amt =[%s]", tmpbuf);
	#endif
	/*单日5万*/
	//if(memcmp(tmpbuf, "000005000000", 12)> 0)
	if(memcmp(tmpbuf, msginfo.d_sin_totalamt, 12)> 0)
	{
		dcs_log(0, 0, "单日交易总金额超限,总金额最大为[%s]",msginfo.d_sin_totalamt);
		setbit(&siso, 39, (unsigned char *)RET_CODE_FL, 2);
		memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_FL, 2);
		errorTradeLog(ewp_info, pep_jnls.trnsndp, "单日金额超限");
		RetToEWP(siso, ewp_info, pep_jnls, 0);
		return;
	}
	//扣款金额是代付金额+手续费
	//代付金额就是实际的上送金额	
	memcpy(transfer_info.pay_amt, ewp_info.consumer_money, getstrlen(ewp_info.consumer_money));
	memcpy(transfer_info.transfer_type, ewp_info.transfer_type, getstrlen(ewp_info.transfer_type));
	
	//计算扣款金额
	
	memset(tmpbuf, '0', sizeof(tmpbuf));
	tmpbuf[12] = '\0';
	rtn = SumAmt(ewp_info.consumer_money, ewp_info.transfee+1, tmpbuf);
	if(rtn == -1)
	{
		dcs_log(0, 0, "求和错误");
		return;
	}

	#ifdef __TEST__
		dcs_log(0, 0, "扣款金额tmpbuf =[%s]", tmpbuf);
	#endif
	setbit(&siso, 4, (unsigned char *)tmpbuf, 12);
	
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	memcpy(tmpbuf, ewp_info.transfee, 1);
	memcpy(tmpbuf+1, ewp_info.transfee+4, 8);
	setbit(&siso, 28, (unsigned char *)tmpbuf, 9);
	
	setbit(&siso, 41, (unsigned char *)msginfo.termcde, 8);
	setbit(&siso, 42, (unsigned char *)msginfo.mercode, 15);
	setbit(&siso, 0, (unsigned char *)msginfo.msgtype, 4);
	setbit(&siso, 3, (unsigned char *)"910000", 6);
	setbit(&siso, 2, (unsigned char *)ewp_info.consumer_cardno, getstrlen(ewp_info.consumer_cardno));
	setbit(&siso, 49, (unsigned char *)"156", 3);

	//发送核心的60域内容
	setbit(&siso, 60, (unsigned char *)"6011", 4);
	setbit(&siso, 7, (unsigned char *)currentDate+4, 10);
	
	//取前置系统交易流水
	if(pub_get_trace_from_seq(trace) < 0)
	{
		dcs_log(0, 0, "取前置系统交易流水错误" );
		setbit(&siso, 39, (unsigned char *)RET_CODE_F8, 2);
		memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_F8, 2 );
		errorTradeLog(ewp_info, pep_jnls.trnsndp, "取前置系统交易流水错误");
		RetToEWP(siso, ewp_info, pep_jnls, 0);
		return;
	}
	dcs_log(0, 0, "获取前置流水trace =[%s]", trace);
	//mac校验
	#ifndef __TEST__ 
	rtn = ewphbmac_chack(secuinfo, ewp_info);
	if(rtn != 0)
	{
		dcs_log(0, 0, "mac校验失败");
		setbit(&siso, 39, (unsigned char *)"A0", 2);
		setbit(&siso, 11, (unsigned char *)trace, 6);
		SavePepjnls(siso, ewp_info, pep_jnls);
		SaveTransferInfo(siso, ewp_info, pep_jnls, transfer_info);
		RetToEWP(siso, ewp_info, pep_jnls,0);
		return;
	}	
	dcs_log(0, 0, "mac校验成功");
	#endif
	//设置订单号 订单号不合法，直接返回报文格式错误
	setbit(&siso, 48, (unsigned char *)ewp_info.consumer_orderno, getstrlen(ewp_info.consumer_orderno));	
	
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
	/*设置流水号*/
	setbit(&siso, 11, (unsigned char *)trace, 6);
	setbit(&siso, 22, (unsigned char *)ewp_info.serinputmode, 3);
	setbit(&siso, 25, (unsigned char *)msginfo.service, 2);
	
	//判断卡的种类
	outCardType = GetCardInfo(ewp_info.consumer_cardno, bank_name);
	memcpy(transfer_info.in_acc_bank_name, bank_name, getstrlen(bank_name));
	if(outCardType == -1)
	{
		dcs_log(0, 0, "查询card_transfer失败");
		setbit(&siso, 39, (unsigned char *)RET_CODE_FZ, 2);
		setbit(&siso, 11, (unsigned char *)trace, 6);
		SavePepjnls(siso, ewp_info, pep_jnls);
		SaveTransferInfo(siso, ewp_info, pep_jnls, transfer_info);
		RetToEWP(siso, ewp_info, pep_jnls,0);
		return;
	}
	pep_jnls.outcdtype = outCardType;
	//简单风控借记卡
	if(outCardType!='D')
	{
		dcs_log(0,0,"卡种限制");
		setbit(&siso,39,(unsigned char *)RET_CODE_F2,2);
		SavePepjnls(siso, ewp_info, pep_jnls);
		SaveTransferInfo(siso, ewp_info, pep_jnls, transfer_info);
		RetToEWP(siso,ewp_info,pep_jnls,0);
		return;
	}
	rtn = GetCustInfo(ewp_info.consumer_username, ewp_info.consumer_psamno, &pos_cust_info);
	if(rtn < 0)
	{
		dcs_log(0, 0, "查询pos_cust_info失败,username=%s,psam=%s", ewp_info.consumer_username, ewp_info.consumer_psamno);
		memcpy(pos_cust_info.T0_flag, "0", 1);
  		memcpy(pos_cust_info.agents_code,"000000",6);
	}	
	else
	{
		memcpy(pep_jnls.posmercode, pos_cust_info.cust_merid, 15);
		memcpy(pep_jnls.postermcde, pos_cust_info.cust_termid, 8);
	}
	
	//21域信息
	/*
	psam卡号（16字节）+ 电话号码，
	区号与电话号码用|区分（或者手机号）（15字节，左对齐，右补空格）
	+ 交易发起方式（1字节，ewp平台送过来的信息） ewp上送的内容修改为2个字节，上送给核心仍然是一个字节
	+ 终端类型（2字节，刷卡设备取值为01）
	+ 交易码（3字节）+ 终端流水（6字节，不存在的话填全0）
	+ 渠道ID(4个字节)
	*/ 
	memset(tmpbuf, 0, sizeof(tmpbuf));
	sprintf(tmpbuf, "%s%-15s%s%s%s%06s%04d", ewp_info.consumer_psamno, ewp_info.consumer_username, ewp_info.translaunchway+1, "01", ewp_info.consumer_transcode, "000000", secuinfo.channel_id);
	
  	if(getstrlen(pos_cust_info.T0_flag)==0)
    	memcpy(pos_cust_info.T0_flag, "0", 1);
 	if(getstrlen(pos_cust_info.agents_code)==0)
  		memcpy(pos_cust_info.agents_code,"000000",6);
 	sprintf(tmpbuf+47, "%s", pos_cust_info.T0_flag);
  	sprintf(tmpbuf+48, "%s", pos_cust_info.agents_code);
	setbit(&siso, 21, (unsigned char *)tmpbuf, 54);
	
	#ifdef __LOG__
		dcs_log(0, 0, "21域[%d][%s]", strlen(tmpbuf), tmpbuf);
	#endif
	if(getstrlen(ewp_info.consumer_trackno) != 0)
	{
		//解密磁道
		rtn = DecryptTrack(ewp_info.consumer_track_randnum, ewp_info.consumer_trackno, &termbuf, &siso, secuinfo, extkey);
		if(rtn < 0)
		{
			dcs_log(0, 0, "解密磁道错误, s=%d", s);
			setbit(&siso, 39, (unsigned char *)RET_CODE_F3, 2);
			SavePepjnls(siso, ewp_info, pep_jnls);
			SaveTransferInfo(siso, ewp_info, pep_jnls, transfer_info);
			RetToEWP(siso, ewp_info, pep_jnls, 0);
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
			SavePepjnls(siso, ewp_info, pep_jnls);
			SaveTransferInfo(siso, ewp_info, pep_jnls, transfer_info);
			RetToEWP(siso,ewp_info,pep_jnls,0);
			return;
		}
		else
		{
			setbit(&siso,52,(unsigned char *)NULL, 0);
			setbit(&siso,53,(unsigned char *)NULL, 0);
		}
	}
	else
	{
		//转加密密码
		memset(tmpbuf, 0, sizeof(tmpbuf));
		rtn = DecryptPin(ewp_info.consumer_pin_randnum, ewp_info.consumer_password, extkey, &termbuf, tmpbuf, secuinfo);
		if(rtn < 0)
		{
			dcs_log(0,0,"转加密密码错误,error code=%d",s);
			setbit(&siso,39,(unsigned char *)RET_CODE_F7,2);
			SavePepjnls(siso, ewp_info, pep_jnls);
			SaveTransferInfo(siso, ewp_info, pep_jnls, transfer_info);
			RetToEWP(siso,ewp_info,pep_jnls,0);
			return; 
		}
			
		setbit(&siso,52, (unsigned char *)tmpbuf, 8);
		setbit(&siso,53, (unsigned char *)"1000000000000000", 16);
	}
	rtn = SavePepjnls(siso, ewp_info, pep_jnls);
	if(rtn == -1)
	{
		dcs_log(0,0,"保存pep_jnls数据失败");
		setbit(&siso,39,(unsigned char *)RET_CODE_FO,2);
		memcpy(ewp_info.consumer_responsecode,(unsigned char *)RET_CODE_FO,2);
		memcpy(ewp_info.consumer_presyswaterno,trace,6);
		errorTradeLog(ewp_info, pep_jnls.trnsndp, "保存pep_jnls数据失败");
		RetToEWP(siso,ewp_info,pep_jnls,0);
		return;
	}
	rtn = SaveTransferInfo(siso, ewp_info, pep_jnls, transfer_info);
	if(rtn == -1)
	{
		dcs_log(0,0,"保存transfer_info数据失败");
		setbit(&siso,39,(unsigned char *)RET_CODE_FO,2);
		memcpy(ewp_info.consumer_responsecode,(unsigned char *)RET_CODE_FO,2);
		memcpy(ewp_info.consumer_presyswaterno,trace,6);
		errorTradeLog(ewp_info, pep_jnls.trnsndp, "保存transfer_info数据失败");
		RetToEWP(siso,ewp_info,pep_jnls,0);
		return;
	}
	//新增优化功能不使用ewp_info数据库而使用memcached数据库
	//dcs_log(0, 0, "save data to memcached begin");
	OrganizateData(ewp_info, &key_value_info);
	rtn = Mem_SavaKey(key_value_info.keys, key_value_info.value, key_value_info.value_length, 600);
	if(rtn == -1)
	{
		dcs_log(0, 0, "save mem error");
		setbit(&siso,39,(unsigned char *)RET_CODE_F9,2);
		updateXpepErrorTradeInfo(ewp_info, trace, RET_CODE_F9);
		RetToEWP(siso,ewp_info,pep_jnls,0);
		return;
	}
	/*组消费包发送给核心*/
	rtn=WriteXpe(siso);
	if(rtn == -1)
	{
		dcs_log(0,0,"发送信息给核心失败");
		setbit(&siso,39,(unsigned char *)RET_CODE_F9,2);
		updateXpepErrorTradeInfo(ewp_info, trace, RET_CODE_F9);
		RetToEWP(siso,ewp_info,pep_jnls,0);
		return;
	}
	//向超时控制进程丢一个冲正报文，若超时未收到核心返回则发起冲正
	//int AddReversalControl(char *billmsg, char *trace)
	rtn = AddReversalControl(ewp_info.consumer_orderno, trace, currentDate);
	if(rtn < 0)
	{
		dcs_log(0, 0, "AddReversalControl error");
	}
	return;	
}

/*函数功能，判断输入参数是否是数字或是字母的组合*/
int checkRegex(char *buf)
{
	int status = -1, i = 0;
	int cflags = REG_EXTENDED;
	regmatch_t pmatch[1];
	const char *pattern ="^[a-zA-Z0-9]{1,20}$";
	const size_t nmatch = 1;
	regex_t reg;
  
	regcomp(&reg, pattern, cflags);
	status = regexec(&reg, buf, nmatch, pmatch, 0);
	if(status ==REG_NOMATCH)
	{
		dcs_log(0, 0, "No Match");
		/* 释放正则表达式 */
		regfree(&reg);
		return -1;
	}
	else if(status==0)
	{
		//dcs_log(0, 0, "Match");
		/* 释放正则表达式 */
		regfree(&reg);
		return 0;
	}
    return 0;
}

/*解析从ewp那里接收到的转账汇款的请求数据 存放到结构体EWP_INFO中*/
int analysisTransferRQ(char *rcvbuf, int rcvlen, EWP_INFO *ewp_info)
{
	int check_len, offset, temp_len, rtn;
	char tmpbuf[1024];
	int flag = 0;

	check_len = 0;
	offset = 0;
	temp_len = 0;
	rtn = 0;
	
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	
	check_len = rcvlen;

	/*转账汇款类型*/
	memcpy(ewp_info->transfer_type, rcvbuf + offset, 1);
	ewp_info->transfer_type[1] = 0;
	offset += 1;
	check_len -= 1;
	
	//#ifdef __LOG__
		dcs_log(0, 0, "transfer_type =[%s]", ewp_info->transfer_type);
	//#endif
	
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
		dcs_log(0, 0, "consumer_sentdate=[%s], consumer_senttime=[%s]", ewp_info->consumer_sentdate, ewp_info->consumer_senttime);
	#endif
	
	/*交易码*/
	memcpy(ewp_info->consumer_transcode, rcvbuf + offset, 3);
	ewp_info->consumer_transcode[3] = 0;
	offset += 3;
	check_len -= 3;
	
	//#ifdef __LOG__
		dcs_log(0, 0, "consumer_transcode=[%s]", ewp_info->consumer_transcode);
	//#endif
	

	/*交易发起方式 ,改为2字节*/
	memcpy(ewp_info->translaunchway, rcvbuf + offset, 2);
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
	
	if(checkRegex(ewp_info->consumer_orderno)<0)
	{
		flag = 1;
		dcs_log(0, 0, "订单号不合规则consumer_orderno=[%s]", ewp_info->consumer_orderno);
	}
		
	#ifdef __LOG__
		dcs_log(0, 0, "consumer_username=[%s], consumer_orderno=[%s]", ewp_info->consumer_username, ewp_info->consumer_orderno);
	#endif

	/*转出卡号*/
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
		
	//#ifdef __LOG__
		dcs_log(0, 0, "consumer_money=[%s], transfee =[%s]", ewp_info->consumer_money, ewp_info->transfee);
	//#endif
		
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
	
	memcpy(ewp_info->macinfo, rcvbuf + offset, 8);
	ewp_info->macinfo[8] = 0;
	offset += 8;
	check_len -= 8;
		
	#ifdef __LOG__
		dcs_log(0, 0, "macinfo=[%s]", ewp_info->macinfo);
	#endif
	
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
	/*20个字节的自定义域*/
	memcpy(ewp_info->selfdefine, rcvbuf + offset, 20);
	ewp_info->selfdefine[20] = 0;
	offset += 20;
	check_len -= 20;
		
	#ifdef __LOG__
		dcs_log(0, 0, "selfdefine=[%s]", ewp_info->selfdefine);
	#endif
	
	#ifdef __LOG__
		dcs_log(0, 0, "解析EWP数据, check_len=[%d], offset=[%d]", check_len, offset);	
		dcs_log(0, 0, "check_len=[%d]", check_len);
	#endif
	
	/*poitcde_flag 标志位赋值 0:存量磁条, 1:IC 不降级, 2:IC 降级, 3:纯磁条 , 4:挥卡*/
	memset(ewp_info->serinputmode, 0x00, sizeof(ewp_info->serinputmode));
	if(memcmp(ewp_info->icflag, "2", 1)==0)// 添加挥卡方式
	{
		memcpy(ewp_info->poitcde_flag, "4", 1);
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

	if(check_len != 0 || flag ==1)
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
组返回EWP平台的报文
code = 0,前置平台返回EWP的错误信息报文，pep_jnls为null，返回数据从ewp_info中取；
code = 1,前置平台返回EWP的交易响应报文，ewp_info为null，返回数据从pep_jnls中取；
*/
int RetToEWP(ISO_data siso, EWP_INFO ewp_info, PEP_JNLS pep_jnls, int code)
{
	int gs_comfid = -1, s = 0, rtn = 0;
	char buffer1[1024], buffer2[1024];
	
    memset(buffer1, 0, sizeof(buffer1));
	memset(buffer2, 0, sizeof(buffer2));
  
    gs_comfid = pep_jnls.trnsndpid;
	
	#ifdef __TEST__
    if(fold_get_maxmsg(gs_comfid) < 100)
		fold_set_maxmsg(gs_comfid, 500) ;
 	#else
	if(fold_get_maxmsg(gs_comfid) < 2)
		fold_set_maxmsg(gs_comfid, 20) ;
    #endif
	
	if(getstrlen(ewp_info.tpduheader)==0)
	{
		rtn = GetInfoFromMem(pep_jnls, &ewp_info);
		if(rtn < 0)
		{
			dcs_log(0, 0, "Read ewp_info from memcached error!");
			return;
		}
	}
	//组回EWP的报文
	s = DataReturnToEWP(code, buffer2, pep_jnls, ewp_info, siso);
	if(getstrlen(ewp_info.tpduheader)!=0)
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
		dcs_debug(buffer2, s,"[%s] data_buf len=%d, foldId=%d", pep_jnls.trnsndp, s, gs_comfid);
	#else
		dcs_log(0, 0, "[%s] data_buf len=%d, foldId=%d", pep_jnls.trnsndp, s, gs_comfid);
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
int DataReturnToEWP(int code, char *buffer, PEP_JNLS pep_jnls, EWP_INFO ewp_info, ISO_data siso)
{
	char tmpbuf[512], ewpinfo[1024], temp_asc_55[512];
	int ewpinfo_len=0, len=0, merlen=0, s=0;
	POS_CUST_INFO pos_cust_info;
	
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(ewpinfo, 0, sizeof(ewpinfo));
	memset(temp_asc_55, 0, sizeof(temp_asc_55));
	memset(&pos_cust_info, 0, sizeof(POS_CUST_INFO));
	
	/*获取IC卡公钥参数下载标识,默认不更新*/
	memcpy(ewp_info.download_pubkey_flag, "1", 1);
	memcpy(ewp_info.download_para_flag, "1", 1);	
	key_para_update(&ewp_info);
	
	if(code==0)
	{
		dcs_log(0, 0, "FAILED");
		//转账汇款类型
		sprintf( buffer, "%s", ewp_info.transfer_type);	
		/*8个字节  接入系统发送交易的日期*/
		sprintf( buffer, "%s%s",buffer, ewp_info.consumer_sentdate);
		//dcs_log(0,0,"发送交易的日期 [%s]",ewp_info.consumer_sentdate);
		/*6个字节  接入系统发送交易的时间*/
		sprintf( buffer, "%s%s",buffer, ewp_info.consumer_senttime);
		//dcs_log(0,0,"发送交易的时间 [%s]",ewp_info.consumer_senttime);
		/*3个字节  区分业务类型订单支付：210*/
		sprintf( buffer, "%s%s",buffer, ewp_info.consumer_transcode); 
		//dcs_log(0,0,"交易码 [%s]",ewp_info.consumer_transcode);
		/*2个字节的交易发起方式*/
		sprintf( buffer, "%s%s",buffer, ewp_info.translaunchway);
		//dcs_log(0,0,"交易发起方式[%s]",ewp_info.translaunchway);
		/*16个字节刷卡设备编号*/
		sprintf( buffer,"%s%s",buffer,ewp_info.consumer_psamno);
		//dcs_log(0,0,"PSAM设备编号 [%s]",ewp_info.consumer_psamno);

		/*20个字节 手机编号左对齐 不足补空格*/
		sprintf( buffer,"%s%-20s",buffer,ewp_info.consumer_phonesnno);
		//dcs_log(0,0,"机具编号 [%s]",ewp_info.consumer_phonesnno);
		/*11个字节 手机号，例如：1502613236*/
		sprintf( buffer,"%s%s",buffer,ewp_info.consumer_username);
		//dcs_log(0,0,"用户名 [%s]",ewp_info.consumer_username);
		/*20个字节的订单号， 左对齐 不足补空格*/
		sprintf( buffer,"%s%-20s",buffer,ewp_info.consumer_orderno);
		//dcs_log(0,0," 订单号 [%s]",ewp_info.consumer_orderno);
		/*19个字节卡号 左对齐 不足补空格*/
		sprintf( buffer,"%s%-19s",buffer,ewp_info.consumer_cardno);
		//dcs_log(0,0,"卡号 [%s]",ewp_info.consumer_cardno);
		getbit(	&siso,39,(unsigned char *)ewp_info.consumer_responsecode);
		ewpinfo_len = GetDetailInfo(ewp_info.consumer_responsecode, ewp_info.consumer_transcode, ewpinfo);
		if(ewpinfo_len == -1)
			ewpinfo_len = 0;

		//dcs_log(0, 0, "取交易应答描述成功[%s]", ewpinfo);
		//dcs_log(0, 0, "ewp_info.consumer_transtype[%s]", ewp_info.consumer_transtype);
		//dcs_log(0, 0, "ewp_info.consumer_transdealcode[%s]", ewp_info.consumer_transdealcode);
		/*19个字节转入卡号 左对齐 不足补空格*/
		sprintf( buffer,"%s%-19s",buffer,ewp_info.incardno);
		/*50个字节转入卡姓名 左对齐 不足补空格*/
		memset(tmpbuf, 0x00, sizeof(tmpbuf));
		/*返回ewp的姓名格式需要为GBK*/
		u2g(ewp_info.incdname, strlen(ewp_info.incdname), tmpbuf, sizeof(tmpbuf));
		sprintf( buffer, "%s%-50s", buffer, tmpbuf);
		/*25个字节转入卡行号  左对齐 不足补空格*/
		sprintf( buffer, "%s%-25s", buffer, ewp_info.incdbkno);
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
				
		//dcs_log(0,0,"authcode = [%s]",tmpbuf);	
		/*1个字节表示应答码内容的长度*/
		/*应答码描述信息，utf-8表示*/
		len = strlen(buffer);
			
		buffer[len] = ewpinfo_len;
		len +=1;
		memcpy(buffer+len, ewpinfo, ewpinfo_len);
		len +=ewpinfo_len;
		//dcs_log(0, 0, "into SK program get GetCustInfo");	
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
	else if(code==1)
	{
		//dcs_log(0, 0, "SUCCESS");
		//转账汇款类型
		sprintf( buffer, "%s", pep_jnls.transfer_type);	
		/*8个字节  接入系统发送交易的日期*/
		sprintf( buffer, "%s%s",buffer, pep_jnls.trndate);
		/*6个字节  接入系统发送交易的时间*/
		sprintf( buffer, "%s%s",buffer, pep_jnls.trntime);
		/*3个字节  区分业务类型订单支付：210??*/
		sprintf( buffer, "%s%s",buffer, pep_jnls.trancde); 
		/*2个字节的交易发起方式*/
		sprintf( buffer, "%s%s",buffer, ewp_info.translaunchway);
		/*16个字节刷卡设备编号也即psam编号*/
		pep_jnls.samid[16]='\0';
		sprintf( buffer, "%s%-16s",buffer, pep_jnls.samid); 
		//dcs_log(0,0,"pep_jnls.samid = [%s]",tmpbuf);
		
		/*20个字节 手机编号(机具编号)左对齐 不足补空格??*/
		pep_jnls.termidm[20] ='\0';
		sprintf( buffer,"%s%-20s",buffer,pep_jnls.termidm);
		sprintf( buffer, "%s%-11s",buffer, pep_jnls.termid);
		/*20个字节 订单号，左对齐 不足补空格*/
		memset(tmpbuf,0,sizeof(tmpbuf));
		memcpy(tmpbuf,pep_jnls.billmsg,20);
		sprintf( buffer, "%s%-20s",buffer,tmpbuf); 
		//dcs_log(0,0,"pep_jnls.billmsg = [%s]",tmpbuf);
		/*19个字节卡号， 左对齐 不足补空格*/
		pub_rtrim_lp(pep_jnls.outcdno,strlen(pep_jnls.outcdno),pep_jnls.outcdno,1);
#ifdef __TEST__
		dcs_log(0,0,"pep_jnls.outcdno = [%s]",pep_jnls.outcdno);
#endif
		sprintf(buffer,"%s%-19s",buffer,pep_jnls.outcdno);
		/*根据应答码和交易码得到关于交易应答码的详细描述信息*/
		ewpinfo_len = GetDetailInfo(pep_jnls.aprespn, pep_jnls.trancde, ewpinfo);
		/*1个字节表示应答码内容的长度*/
		/*应答码描述信息，utf-8表示*/
		if(ewpinfo_len == -1)
			ewpinfo_len = 0;
		/*19个字节转入卡号， 左对齐 不足补空格??*/
		pub_rtrim_lp(pep_jnls.intcdno, strlen(pep_jnls.intcdno), pep_jnls.intcdno, 1);

		//dcs_log(0,0,"pep_jnls.intcdno = [%s]", pep_jnls.intcdno);
		sprintf(buffer,"%s%-19s",buffer,pep_jnls.intcdno);
		
		/*50个字节转入卡姓名 左对齐 不足补空格*/
		memset(tmpbuf, 0x00, sizeof(tmpbuf));
		/*返回ewp的姓名格式需要为GBK*/
		u2g(ewp_info.incdname, strlen(ewp_info.incdname), tmpbuf, sizeof(tmpbuf));
		sprintf( buffer, "%s%-50s", buffer, tmpbuf);
		/*25个字节转入卡行号  左对齐 不足补空格*/
		sprintf( buffer, "%s%-25s", buffer, ewp_info.incdbkno);
		
		/*12个字节 金额，右对齐 不足补0*/
		sprintf(buffer,"%s%012s",buffer,pep_jnls.tranamt);
		sprintf(buffer,"%s%s",buffer,ewp_info.transfee);
		/*2个字节前置系统的交易应答码??*/
		sprintf( buffer,"%s%s",buffer,pep_jnls.aprespn);
		/*6个字节  前置系统交易流水trance */
		sprintf( buffer,"%s%06d",buffer,pep_jnls.trace);
		/*12个字节前置系统返回的交易参考号*/
		if(getstrlen(pep_jnls.syseqno)==0)
			sprintf( buffer,"%s%012d",buffer, 0);
		else
			sprintf( buffer,"%s%12s",buffer,pep_jnls.syseqno);		
		/*6个字节 授权码*/
		memset(tmpbuf,0,sizeof(tmpbuf));
		if(getbit( &siso,38,(unsigned char *)tmpbuf) <= 0 )
			sprintf( buffer,"%s%s",buffer,"000000");
		else
			sprintf( buffer,"%s%6s",buffer,tmpbuf);
		//dcs_log(0,0,"pep_jnls.authcode = [%s]",tmpbuf);	
		len = strlen(buffer);
		buffer[len] = ewpinfo_len;
		len +=1;
		memcpy(buffer+len, ewpinfo, ewpinfo_len);
		len +=ewpinfo_len;
		//dcs_log(0, 0, "SK program ");	
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
		/*IC卡数据域,转账汇款下发，信用卡还款下发 上线修改*/
		if( memcmp(ewp_info.poitcde_flag, "0", 1)!=0)
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
			sprintf(buffer+len,"%s%s", ewp_info.download_pubkey_flag, ewp_info.download_para_flag);
			len +=2;
		}
		/*20个字节的自定义域原样返回给EWP*/
		memcpy(buffer+len,ewp_info.selfdefine, getstrlen(ewp_info.selfdefine));
		len +=20;
		return len;	
	}
	return len;
}

/*向超时控制里添加一条记录  超时向核心发起冲正请求*/
int AddReversalControl(char *billmsg, char *trace, char *currentDate)
{
	int rtn = 0, s = 0, fid = -1;
	struct TIMEOUT_INFO timeout_info;
	ISO_data siso;
	PEP_JNLS pep_jnls;
	char  buffer[1024], buffer2[1024], rcvbuf[128], tmpbuf[128];
	int channel_id = 0;
	char T0_flag[2], agent_code[6+1];

	memset(&timeout_info, 0x00, sizeof(struct TIMEOUT_INFO));
	memset(&pep_jnls, 0x00, sizeof(PEP_JNLS));
	memset(&siso, 0x00, sizeof(ISO_data));
	memset(buffer, 0x00, sizeof(buffer));
	memset(rcvbuf, 0x00, sizeof(rcvbuf));
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	memset(T0_flag, 0x00, sizeof(T0_flag));
	memset(agent_code, 0x00, sizeof(agent_code));
	memset(buffer2, 0x00, sizeof(buffer2));

	//加载前置与核心的8583配置文件
	if(IsoLoad8583config(&iso8583conf[0], "iso8583.conf") < 0)
	{
		dcs_log(0, 0, "Load iso8583.conf failed:%s",strerror(errno));
		return -1;
	}
	
	memcpy(iso8583_conf, iso8583conf, sizeof(iso8583conf));
	iso8583 = &iso8583_conf[0];
  	SetIsoHeardFlag(0);
    SetIsoFieldLengthFlag(1);
	
    fid = fold_locate_folder("PEX_COM");
    if(fid < 0)
    {
        dcs_log(0, 0, "locate folder PEX_COM failed.");
        return -1;
    }
	
	//查询需要发送的信息
	rtn = getSendInfo(billmsg, trace, &pep_jnls);
	if(rtn<0)
	{
		dcs_log(0, 0, "获取消费信息失败");
        return -1;
	}
	//发起冲正交易
	setbit(&siso, 0, (unsigned char *)"0400", 4);
	//卡号
	setbit(&siso, 2, (unsigned char *)pep_jnls.outcdno, strlen(pep_jnls.outcdno));

	/*3域重新赋值*/
	setbit(&siso, 3, (unsigned char *)"000000", 6);
	/*交易金额*/
	setbit(&siso, 4, (unsigned char *)pep_jnls.tranamt, 12);
	
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	memcpy(tmpbuf, currentDate+4, 4);
	memcpy(tmpbuf+4, currentDate+8, 6);
	setbit(&siso, 7, (unsigned char *)tmpbuf, 10);
	
	/*原前置流水号*/
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	sprintf(tmpbuf, "%06ld", pep_jnls.trace);
	setbit(&siso, 11, (unsigned char *)tmpbuf, 6);

	channel_id = GetChannelId(pep_jnls.samid);
	if(channel_id < 0)
	{
		dcs_log(0, 0, "获取渠道信息失败");
		return -1;
	}
	
	memset(T0_flag, 0x00, sizeof(T0_flag));
	memset(agent_code, 0x00, sizeof(agent_code));
	rtn = GetAgentInfo(pep_jnls.termid, pep_jnls.samid, T0_flag, agent_code);
	if(rtn ==-1)
	{
		dcs_log(0, 0, "获取代理商信息失败");
		return -1;
	}
	if(strlen(T0_flag)==0)
	{
		memcpy(agent_code, "000000", 6);
		memcpy(T0_flag, "0", 1);
	}
	else if(memcmp(T0_flag, "0", 1)==0)
	{
		memcpy(agent_code, "000000", 6);
	}
	
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	sprintf(tmpbuf, "%s%-15s%s01%s%s%04d%s%s",pep_jnls.samid, pep_jnls.termid, pep_jnls.translaunchway+1, pep_jnls.trancde, "000000", channel_id, T0_flag, agent_code);
	setbit(&siso, 21,(unsigned char *)tmpbuf, strlen(tmpbuf));
	
	/*服务点输入方式码*/
	setbit(&siso, 22, (unsigned char *)pep_jnls.poitcde, 3);
	/*卡片序列号*/
	if(getstrlen(pep_jnls.card_sn)!=0)
		setbit(&siso, 23, (unsigned char *)pep_jnls.card_sn, 3);

	setbit(&siso, 25,(unsigned char *)pep_jnls.poscode, 2);
	setbit(&siso, 28,(unsigned char *)pep_jnls.filed28, strlen(pep_jnls.filed28));
	setbit(&siso, 39,(unsigned char *)"98", 2);
	/*重组冲正交易41,42域*/
	setbit(&siso, 41, (unsigned char *)pep_jnls.termcde, 8);
	setbit(&siso, 42, (unsigned char *)pep_jnls.mercode, 15);

	setbit(&siso, 48, (unsigned char *)pep_jnls.billmsg, getstrlen(pep_jnls.billmsg));	
	setbit(&siso, 49, (unsigned char *)"156", 3);
	setbit(&siso, 60, (unsigned char *)"6011", 4);
	
	s = iso_to_str((unsigned char *)buffer, &siso, 0); //0 表示不打印报文内容
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	bcd_to_asc((unsigned char *)tmpbuf, (unsigned char *)buffer, 20 ,0);
	
	memcpy(buffer2, tmpbuf, 20);
	memcpy(buffer2+20, buffer+10, s-10);
	s = s+10;

	bcd_to_asc((unsigned char *)timeout_info.data, (unsigned char*)buffer2, 2*s, 0);
	
	sprintf(timeout_info.key, "%s%s%06ld", "PEX_COM", pep_jnls.billmsg, pep_jnls.trace);
	timeout_info.timeout = 180;
	timeout_info.sendnum = 3;
	timeout_info.foldid = fid;
	timeout_info.length = 2*s;

	add_timeout_control(timeout_info, rcvbuf, sizeof(rcvbuf));

	if(memcmp(rcvbuf+2, "response=00", 11)==0)
	{
		dcs_log(0, 0, "添加扣款超时冲正控制 success");
	}
	else
	{
		dcs_log(0, 0, "添加扣款超时冲正控制 false rcvbuf =[%s]", rcvbuf);
		return -1;
	}	
	return 0;
}
