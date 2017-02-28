#include "ibdcs.h"
#include "trans.h"
#include "memlib.h"

extern struct ISO_8583 *iso8583;
extern struct ISO_8583 iso8583_conf[128];
extern struct ISO_8583 iso8583conf[128];
/* 
	支付管理系统数据处理 

1、支付管理系统与前置系统通信报文格式参考《前置系统接入报文规范》
2、支付管理系统会发起消费撤销和消费退货交易	
	
*/
void custProc(char *srcBuf, int iFid, int iMid, int iLen) 
{
#ifdef __LOG__
	dcs_debug(srcBuf, iLen, "into CustProc,iFid=%d, iMid=%d, iLen=%d", iFid, iMid, iLen);
#else
	dcs_log(0, 0, "into CustProc,iFid=%d, iMid=%d, iLen=%d", iFid, iMid, iLen);
#endif
	int datalen ,rtn, s;
	char databuf[PUB_MAX_SLEN + 1], tmpbuf[512], tmpbuf1[512];
	EWP_INFO ewp_info;
	PEP_JNLS pep_jnls;
	MSGCHGINFO msginfo;
	ISO_data siso;
	
	//保存到缓存数据memcached结构体
	KEY_VALUE_INFO key_value_info;
	
	//当前日期和时间
	char currentDate[8+1];
	char currentTime[6+1];
	/*4个字节的报文类型*/
	char folderName[4+1];
	
	//系统自带的时间结构体
	struct  tm *tm;   
	time_t  t;
	
	char _tpdu_src[6];
	//密钥密文，被过程密钥加密
	char randKey[32+1];
	//发送机构号
	char sendCde[4+1];
	//发送日期和时间
	char sendDate[8+1];
	char sendTime[6+1];
	//用户随机数
	char sendRandom[16+1];
	//平台随机数
	char xpepRandom[16+1];
	//用户数据
	char sendUserData[20+1];
	char sendResponse[2+1];
	//mac数据
	char sendMac[8+1], _sendMac[9];
	
	
	memset(&ewp_info, 0, sizeof(EWP_INFO));
	memset(databuf, 0, sizeof(databuf));
	memset(&pep_jnls, 0, sizeof(PEP_JNLS));
	memset(&msginfo, 0, sizeof(MSGCHGINFO));
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(&siso, 0, sizeof(ISO_data));
	
	memset(folderName, 0, sizeof(folderName));
	
	memset(currentDate, 0, sizeof(currentDate));
	memset(currentTime, 0, sizeof(currentTime));
	
	memset(sendMac, 0, sizeof(sendMac));
	memset(_sendMac, 0, sizeof(_sendMac));
	memset(&key_value_info, 0, sizeof(KEY_VALUE_INFO));

	time(&t);
	tm = localtime(&t);

	sprintf(currentDate, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
	sprintf(currentTime, "%02d%02d%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);

	/*得到交易报文类型*/
	memset(folderName, 0, sizeof(folderName));
	memcpy(folderName,srcBuf,4);
	srcBuf += 4;
	iLen -= 4;

	/*测试环境TPDU为6008200000,生产环境为6005330000*/
	memset(ewp_info.tpduheader, 0, sizeof(ewp_info.tpduheader));
	memset(_tpdu_src, 0, sizeof(_tpdu_src));
	memcpy(tmpbuf, srcBuf, 5);
	memcpy(_tpdu_src, srcBuf, 5);
	bcd_to_asc((unsigned char *)ewp_info.tpduheader, (unsigned char *)tmpbuf, 10, 0);
	dcs_log(0, 0, "CUST TPDU：%s", ewp_info.tpduheader);

	srcBuf += 5;
	iLen -= 5;
	
	datalen = iLen;
	memcpy(databuf, srcBuf, datalen);
	//导入核心8583配置文件
	memcpy(iso8583_conf,iso8583conf,sizeof(iso8583conf));
	iso8583 = &iso8583_conf[0];
  	SetIsoHeardFlag(0);
    SetIsoFieldLengthFlag(1);
	//管理平台和融易付发起的密钥重置报文
	if(memcmp(databuf, "0800", 4) == 0)
	{
		memset(sendCde, 0, sizeof(sendCde));
		memset(sendDate, 0, sizeof(sendDate));
		memset(sendTime, 0, sizeof(sendTime));
		memset(xpepRandom, 0, sizeof(xpepRandom));
		memset(randKey, 0, sizeof(randKey));
		memset(sendUserData, 0, sizeof(sendUserData));
		memset(sendRandom, 0, sizeof(sendRandom));
		memset(sendResponse, 0, sizeof(sendResponse));
	
		memcpy(sendCde, databuf+4, 4);
		memcpy(sendDate, databuf+4+4, 8);
		memcpy(sendTime, databuf+4+4+8, 6);
		//获取用户随机数
		memcpy(sendRandom, databuf+4+4+8+6, 16);
		memcpy(sendUserData, databuf+4+4+8+6+16, 20);
		
		#ifdef __TEST__
			dcs_log(0, 0, "sendCde=%s",sendCde);
			dcs_log(0, 0, "sendDate=%s",sendDate);
			dcs_log(0, 0, "sendTime=%s",sendTime);
			dcs_log(0, 0, "sendRandom=%s",sendRandom);
			dcs_log(0, 0, "sendUserData=%s",sendUserData);
		#endif
		
		s = CustKeyGen(sendRandom, xpepRandom, randKey);
		if(s != 32)
		{
			memcpy(sendResponse, "99", 2);
			updateCustKeyInfo(sendCde, sendDate, sendTime, sendRandom, sendResponse, xpepRandom, randKey, sendUserData);
		}else
		{
			memcpy(sendResponse, "00", 2);
			updateCustKeyInfo(sendCde, sendDate, sendTime, sendRandom, sendResponse, xpepRandom, randKey, sendUserData);
		}
		
		memset(tmpbuf, 0, sizeof(tmpbuf));
		s = 0;
		memcpy(tmpbuf, _tpdu_src, 1);
		s += 1;
		memcpy(tmpbuf+s, _tpdu_src+3, 2);
		s += 2;
		memcpy(tmpbuf+s, _tpdu_src+1, 2);
		s += 2;
		memcpy(tmpbuf+s, "0810", 4);
		s += 4;
		memcpy(tmpbuf+s, sendCde, 4);
		s += 4;
		memcpy(tmpbuf+s, sendDate, 8);
		s += 8;
		memcpy(tmpbuf+s, sendTime, 6);
		s += 6;
		memcpy(tmpbuf+s, sendRandom, 16);
		s += 16;
		memcpy(tmpbuf+s, sendResponse, 2);
		s += 2;
		memcpy(tmpbuf+s, xpepRandom, 16);
		s += 16;
		if(strlen(randKey) == 32)
			memcpy(tmpbuf+s, randKey, 32);
		else
			memcpy(tmpbuf+s, "00000000000000000000000000000000", 32);
		s += 32;
		memcpy(tmpbuf+s, sendUserData, 20);
		s += 20;
		
		WriteKeyBack(tmpbuf, s, iFid);
		return;
	}
	
	/*
		1、调用ewpProc中的拆包方法进行拆包，得到EWP_INFO结构体
	*/
	parsedata(databuf, datalen, &ewp_info);
	
	dcs_log(0, 0, "解析管理系统过来的报文结束.");
	
	/*解析融易付、支付管理系统发送的数据失效*/
	if(memcmp(ewp_info.retcode, "00", 2) != 0)
	{
		dcs_log(0, 0, "解析融易付、支付管理系统发送的数据失败.");
		setbit(&siso, 39, (unsigned char *)RET_CODE_F1, 2);
		memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_F1, 2);
		errorTradeLog(ewp_info, folderName, "解析融易付、支付管理系统发送的数据失败.");
		pep_jnls.trnsndpid = iFid ;
		memcpy(pep_jnls.trnsndp, folderName, getstrlen(folderName));
		WriteEWP(siso, ewp_info, pep_jnls, 0);
		return;
	}
	
	//计算mac
	memset(tmpbuf1, 0, sizeof(tmpbuf1));
	memcpy(tmpbuf1, databuf, datalen-8);
	memcpy(_sendMac, databuf+datalen-8, 8);
	s = pub_cust_mac(ewp_info.consumer_sentinstitu, tmpbuf1, datalen-8, sendMac);
	if(s == 0)
	{
		if( memcmp(sendMac, _sendMac, 8) == 0 )
		{
			dcs_log(0, 0, "Mac校验通过");
		}else
		{
			dcs_log(0, 0, "前置计算得到的mac：%s", sendMac);
			dcs_log(0, 0, "管理平台上送的mac：%s", _sendMac);
			//返回A0
			dcs_log(0, 0, "mac校验失败");
			setbit(&siso, 39, (unsigned char *)"A0", 2);
			memcpy(ewp_info.consumer_responsecode, (unsigned char *)"A0", 2);
			errorTradeLog(ewp_info, folderName, "mac校验失败");
			pep_jnls.trnsndpid = iFid ;
			memcpy(pep_jnls.trnsndp, folderName, getstrlen(folderName));
			WriteEWP(siso, ewp_info, pep_jnls, 0);
			return;
		}
		
	}else
	{
			dcs_log(0, 0, "其它错误");
			setbit(&siso, 39, (unsigned char *)"A0", 2);
			memcpy(ewp_info.consumer_responsecode, (unsigned char *)"A0", 2);
			errorTradeLog(ewp_info, folderName, "其它错误");
			pep_jnls.trnsndpid = iFid ;
			memcpy(pep_jnls.trnsndp, folderName, getstrlen(folderName));
			WriteEWP(siso, ewp_info, pep_jnls, 0);
			return;
	}
	
	if( memcmp(ewp_info.consumer_transdealcode, "200000", 6) == 0 && memcmp(currentDate, ewp_info.pretranstime, 8) != 0 )
	{
		dcs_log(0, 0, "查询原笔交易失败(因为消费撤销必须是当天撤销), 返回F0");
		setbit(&siso, 39, (unsigned char *)RET_CODE_F0, 2);
		memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_F0, 2);
		errorTradeLog(ewp_info, folderName, "查询原笔交易失败，非当天交易");
		pep_jnls.trnsndpid = iFid ;
		memcpy(pep_jnls.trnsndp, folderName, getstrlen(folderName));
		WriteEWP(siso, ewp_info, pep_jnls, 0);
		return;
	}
	
	/*
		2、通过EWP_INFO中的consumer_transcode，查询数据库表msgchg_info，得到MSGCHGINFO结构体
	*/
	rtn = Readmsgchg(ewp_info.consumer_transcode, &msginfo);
	if(rtn == -1)
	{
		dcs_log(0, 0, "查询msgchg_info失败,交易未开通,trancde=%s", ewp_info.consumer_transcode);
		setbit(&siso, 39, (unsigned char *)RET_CODE_FB, 2);
		memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_FB, 2);
		errorTradeLog(ewp_info, folderName, "查询msgchg_info失败,交易未开通");
		pep_jnls.trnsndpid = iFid ;
		memcpy(pep_jnls.trnsndp, folderName, getstrlen(folderName));
		WriteEWP(siso, ewp_info, pep_jnls, 0);
		return;
	}
	
	//判断msgchg_info表中交易是否开启的字段，如果交易关闭则不允许发起退款
	//订单撤销：0A1
	//订单退货：0A2
	if(memcmp(msginfo.flagpe, "N", 1) == 0 || 
	 		memcmp(msginfo.flagpe, "n", 1) == 0 || 
	 			( memcmp(msginfo.flagpe, "Y", 1)!=0 && memcmp(msginfo.flagpe, "y", 1)!= 0 ))
	{
			dcs_log(0, 0, "由于系统或者其他原因，交易暂时关闭");
			setbit(&siso, 39, (unsigned char *)RET_TRADE_CLOSED, 2);
			memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_TRADE_CLOSED, 2);
			errorTradeLog(ewp_info, folderName, "由于系统或者其他原因，交易暂时关闭");
			WriteEWP(siso, ewp_info, pep_jnls, 0);
			return;
	}
	
		
	/*
		3、通过原始交易信息，包括原始交易日期和时间、原始交易卡号、原始交易流水、原始交易系统参考号、
			原始交易订单号等字段查询pep_jnls数据库表，得到原始交易，组iso8583报文
		注意：原始交易必须是成功交易才可以发起消费撤销和消费退货
	*/
	//TODO  必须没发起过成功的消费撤销或者成功退款金额不大于原笔金额
	//TODO  转账汇款通过前置的数据库，发现交易是A6或者超时的，扣款成功的信息需要到核心系统中去找。但是A6或者超时的，扣款应该是成功的。如果扣款不成功，核心应该不能发起撤销。
	rtn = GetOriginalTrans(ewp_info, &pep_jnls);
	if(rtn == -1)
	{
		dcs_log(0, 0, "查询原笔交易失败,返回F0");
		setbit(&siso, 39, (unsigned char *)RET_CODE_F0, 2);
		memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_F0, 2);
		errorTradeLog(ewp_info, folderName, "查询原笔交易失败");
		pep_jnls.trnsndpid = iFid ;
		memcpy(pep_jnls.trnsndp, folderName, getstrlen(folderName));
		WriteEWP(siso, ewp_info, pep_jnls, 0);
		return;
	}
	
	pep_jnls.trnsndpid = iFid;
	
	memcpy(pep_jnls.trnsndp, folderName, getstrlen(folderName));
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memcpy(tmpbuf, currentDate+4, 4);
	memcpy(tmpbuf+4, currentTime, 6);
	
	setbit(&siso, 7, (unsigned char *)tmpbuf, 10);
	
	//消费撤销采用无磁无密的方式
	/*如果是消费撤销的交易*/
	if( memcmp(ewp_info.consumer_transdealcode, "200000", 6) == 0 )
	{
		dcs_log(0, 0, "消费撤销的交易");
		rtn = purchaseCancelPackage(&siso, pep_jnls, msginfo, ewp_info);
		if( rtn == -1 )
		{
			dcs_log(0, 0, "重组消费撤销交易的报文失败");
			setbit(&siso, 39, (unsigned char *)RET_CODE_F5, 2);
			memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_F5, 2);
			errorTradeLog(ewp_info, folderName, "重组消费撤销交易的报文失败");
			WriteEWP(siso, ewp_info, pep_jnls, 0);
			return;
		}
	}
	/*消费退货的交易*/
	else if( memcmp(ewp_info.consumer_transdealcode, "270000", 6) == 0 )
	{	
		dcs_log(0, 0, "消费退货的交易");
		rtn = purchaseReturnPackage(&siso, pep_jnls, msginfo, ewp_info);
		if(rtn == -1 )
		{
			dcs_log(0, 0, "重组消费退货交易的报文失败");
			setbit(&siso, 39, (unsigned char *)RET_CODE_F5, 2);
			memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_F5, 2);
			errorTradeLog(ewp_info, folderName, "重组消费退货交易的报文失败");
			WriteEWP(siso, ewp_info, pep_jnls, 0);
			return;
		}
	}
	
	/*
		4、保存数据库pep_jnls
	*/
	rtn = CustWriteXepDb(siso, ewp_info, pep_jnls);
	if(rtn == -1)
	{
		dcs_log(0, 0, "保存pep_jnls数据失败");
		setbit(&siso, 39, (unsigned char *)RET_CODE_FO, 2);
		memcpy(ewp_info.consumer_responsecode, (unsigned char *)RET_CODE_FO, 2);
		
		memset(tmpbuf, 0, sizeof(tmpbuf));
		getbit(&siso, 11, (unsigned char *)tmpbuf);
		
		memcpy(ewp_info.consumer_presyswaterno, tmpbuf, 6);
		
		errorTradeLog(ewp_info, folderName, "保存pep_jnls数据失败");
		WriteEWP(siso, ewp_info, pep_jnls, 0);
		return;
	}
	
	/*把解析到的数据保存以后需要的信息到数据表ewp_info中*/
	/*
	rtn = WriteDbEwpInfo(ewp_info);
	if(rtn == -1){
		dcs_log(0, 0, "保存EWP_INFO失败");
		setbit(&siso, 39, (unsigned char *)RET_CODE_FP, 2);
		
		memset(tmpbuf, 0, sizeof(tmpbuf));
		getbit(&siso, 11, (unsigned char *)tmpbuf);
		
		updateXpepErrorTradeInfo(ewp_info, tmpbuf,  RET_CODE_FP);
		WriteEWP(siso, ewp_info, pep_jnls, 0);
		return;
	}
	*/
	//新增优化功能不使用ewp_info数据库而使用memcached数据库
	dcs_log(0, 0, "test memcached begin");
	OrganizateData(ewp_info, &key_value_info);
	rtn = Mem_SavaKey(key_value_info.keys, key_value_info.value, key_value_info.value_length, 600);
	if(rtn == -1)
	{
		dcs_log(0, 0, "custProc save mem error");
		dcs_log(0, 0, "custProc保存EWP_INFO失败");
		setbit(&siso, 39, (unsigned char *)RET_CODE_FP, 2);
		
		memset(tmpbuf, 0, sizeof(tmpbuf));
		getbit(&siso, 11, (unsigned char *)tmpbuf);
		updateXpepErrorTradeInfo(ewp_info, tmpbuf, RET_CODE_FP);
		WriteEWP(siso, ewp_info, pep_jnls, 0);
		return;
	}
	//新增end
	
	/*
		5、发送交易报文到核心系统
	*/
	rtn = WriteXpe(siso);
	
	if(rtn == -1)
	{
		dcs_log(0, 0, "发送交易报文到核心系统操作失败。");
		setbit(&siso, 39, (unsigned char *)RET_CODE_F9, 2);
		memset(tmpbuf, 0, sizeof(tmpbuf));
		getbit(&siso, 11, (unsigned char *)tmpbuf);
		updateXpepErrorTradeInfo(pep_jnls, ewp_info, tmpbuf, RET_CODE_F9);
		WriteEWP(siso, ewp_info, pep_jnls, 0);
		return;
	}
	
	return;
}

/*
	重组消费撤销交易的报文
	返回0表示组包成功，返回-1表示组包失败
*/
int purchaseCancelPackage(ISO_data *siso, PEP_JNLS pep_jnls, MSGCHGINFO msginfo, EWP_INFO ewp_info)
{
	int oRead, s=0;
	char tmpbuf[128], trace[7];
	
	memset(trace, 0, sizeof(trace));
	memset(tmpbuf, 0, sizeof(tmpbuf));
	
	setbit(siso, 0, (unsigned char *)"0200", 4);
	setbit(siso, 2, (unsigned char *)pep_jnls.outcdno, strlen(pep_jnls.outcdno));
	setbit(siso, 3, (unsigned char *)"200000", 6);
	
	/*	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	sprintf(tmpbuf, "%012s", pep_jnls.tranamt);*/
	s = getstrlen(pep_jnls.tranamt);
	if(s > 0)
		setbit(siso, 4, (unsigned char *)pep_jnls.tranamt, 12);
	
	
	//取前置系统交易流水
	if((oRead=pub_get_trace_from_seq(trace))<0)
	{
		dcs_log(0, 0, "取前置系统交易流水错误");
		return -1;
	}
	
	setbit(siso, 11, (unsigned char *)trace, 6);
	/*
	21域不知道该如何添加
	前置系统报文21域规则：
	psam卡号（16字节）+ 电话号码，区号与电话号码用|区分（或者手机号）（15字节，左对齐，右补空格）
	+交易发起方式（1字节，ewp平台送过来的信息）+终端类型（2字节，刷卡设备取值为01）+交易码（3字节）
	+终端流水（6字节，不存在的话填全0）	
	+4个字节的渠道ID
	*/
	
	pep_jnls.samid[getstrlen(pep_jnls.samid)] = '\0';
	dcs_log(0, 0, "pep_jnls.samid=[%s]", pep_jnls.samid);
	pep_jnls.modecde[getstrlen(pep_jnls.modecde)] = '\0';
	
	dcs_log(0, 0, "pep_jnls.samid=[%s]", pep_jnls.samid);
	dcs_log(0, 0, "pep_jnls.sendcde=[%s]", pep_jnls.sendcde);
	
	if(memcmp(pep_jnls.sendcde, "03000000", 8)!=0)
	{
		memset(tmpbuf, 0, sizeof(tmpbuf));
		sprintf(tmpbuf, "%-16s%-15s%c", pep_jnls.samid, pep_jnls.modecde, pep_jnls.translaunchway[0]);
		sprintf(tmpbuf, "%s%s%s%06s%s", tmpbuf, "01", ewp_info.consumer_transcode, pep_jnls.termtrc, "0006");
	
		dcs_log(0, 0, "增值交易撤销21域=[%s], length=%d", tmpbuf, strlen(tmpbuf));
		setbit(siso, 21,(unsigned char *)tmpbuf, 47);
	}
	else
	{
		memset(tmpbuf, 0, sizeof(tmpbuf));
		sprintf(tmpbuf, "%s%s%s%s%s", pep_jnls.posmercode, pep_jnls.postermcde, "3", "03", ewp_info.consumer_transcode);
		sprintf(tmpbuf+29, "%s%s%s%04d", pep_jnls.termtrc, "000000", ewp_info.consumer_transdealcode, 1);
	
		dcs_log(0, 0, "传统POS交易撤销21域[%s], length=%d", tmpbuf, strlen(tmpbuf));
		setbit(siso, 21, (unsigned char *)tmpbuf, 51);
	}
	
	/*
	setbit(siso, 22, (unsigned char *)"012", 3);*//*无磁无密*/
	setbit(siso, 22, (unsigned char *)"022", 3);/*有磁无密*/
	//81
	setbit(siso, 25, (unsigned char *)msginfo.service, 2);
	
	s = getstrlen(pep_jnls.track2);
	if(s > 0 )
		setbit(siso, 35, (unsigned char *)pep_jnls.track2, s);
	
	s = getstrlen(pep_jnls.track3);
	if(s > 0 )
		setbit(siso, 36, (unsigned char *)pep_jnls.track3, s);
	
	setbit(siso, 41, (unsigned char *)pep_jnls.termcde, 8);
	setbit(siso, 42, (unsigned char *)pep_jnls.mercode, 15);
	
	setbit(siso, 48, (unsigned char *)ewp_info.consumer_orderno, getstrlen(ewp_info.consumer_orderno));
	setbit(siso, 49, (unsigned char *)"156", 3);
	setbit(siso,60,(unsigned char *)"00000000030000000000", 20);
	
	//TODO ... 需要考虑前置失败无法获取授权码的情况
	/*
	61自定义数据字段
	原交易授权号		N6
	参考号			N12
	时间				N10
	*/
	memset(tmpbuf, 0, sizeof(tmpbuf));
	if( getstrlen( pep_jnls.authcode ) == 0 )
		sprintf(tmpbuf, "%s%s%s", "000000", ewp_info.consumer_sysreferno, pep_jnls.revdate);
	else
		sprintf(tmpbuf, "%s%s%s", pep_jnls.authcode, ewp_info.consumer_sysreferno, pep_jnls.revdate);
	setbit(siso, 61, (unsigned char *)tmpbuf, 28);	
	
	return 0;
}

/*
	重组消费退货交易的报文
	返回0表示组包成功，返回-1表示组包失败
*/
int purchaseReturnPackage(ISO_data *siso, PEP_JNLS pep_jnls, MSGCHGINFO msginfo,EWP_INFO ewp_info)
{
	int oRead, s=0;
	char tmpbuf[128], trace[7];
	
	memset(trace, 0, sizeof(trace));
	memset(tmpbuf, 0, sizeof(tmpbuf));
	
	setbit(siso, 0, (unsigned char *)"0200",4);
	setbit(siso, 2, (unsigned char *)pep_jnls.outcdno, strlen(pep_jnls.outcdno));
	setbit(siso, 3, (unsigned char *)"270000",6);
	
	/*退货金额*/
	memset(tmpbuf, 0, sizeof(tmpbuf));
	sprintf(tmpbuf, "%012s", ewp_info.consumer_money);
	setbit(siso, 4, (unsigned char *)tmpbuf, 12);
	
	//取前置系统交易流水
	if((oRead=pub_get_trace_from_seq(trace))<0)
	{
		dcs_log(0, 0, "取前置系统交易流水错误");
		return -1;
	}
	
	setbit(siso, 11, (unsigned char *)trace, 6);
	/*
	21域不知道该如何添加
	前置系统报文21域规则：
	psam卡号（16字节）+ 电话号码，区号与电话号码用|区分（或者手机号）（15字节，左对齐，右补空格）
	+交易发起方式（1字节，ewp平台送过来的信息）+终端类型（2字节，刷卡设备取值为01）+交易码（3字节）
	+终端流水（6字节，不存在的话填全0）
	+4个字节的渠道ID "0200"	
	*/
	
	pep_jnls.samid[getstrlen(pep_jnls.samid)]='\0';
	dcs_log(0, 0, "pep_jnls.samid=[%s]", pep_jnls.samid);
	pep_jnls.modecde[getstrlen(pep_jnls.modecde)]='\0';
	dcs_log(0, 0, "pep_jnls.sendcde=[%s]", pep_jnls.sendcde);
	if(memcmp(pep_jnls.sendcde, "03000000", 8)!=0)
	{
		memset(tmpbuf, 0, sizeof(tmpbuf));
		sprintf(tmpbuf, "%-16s%-15s%c", pep_jnls.samid, pep_jnls.modecde, pep_jnls.translaunchway[0]);
		sprintf(tmpbuf, "%s%s%s%06s%s", tmpbuf, "01", ewp_info.consumer_transcode, pep_jnls.termtrc, "0006");
		dcs_log(0, 0, "增值交易退货21域[%s], length=%d", tmpbuf, strlen(tmpbuf));
		setbit(siso, 21, (unsigned char *)tmpbuf, 47);
	}
	else
	{
		memset(tmpbuf, 0, sizeof(tmpbuf));
		sprintf(tmpbuf, "%s%s%s%s%s", pep_jnls.posmercode, pep_jnls.postermcde, "3", "03", ewp_info.consumer_transcode);
		sprintf(tmpbuf+29, "%s%s%s%04d", pep_jnls.termtrc, "000000", ewp_info.consumer_transdealcode, 1);
	
		dcs_log(0, 0, "传统POS交易退货21域[%s], length=%d", tmpbuf, strlen(tmpbuf));
		setbit(siso, 21, (unsigned char *)tmpbuf, 51);
	}
	
	
	setbit(siso, 22, (unsigned char *)"022", 3);
	setbit(siso, 25, (unsigned char *)msginfo.service,2);
	
	s = getstrlen(pep_jnls.track2);
	if(s > 0 )
		setbit(siso, 35, (unsigned char *)pep_jnls.track2, s);
	
	s = getstrlen(pep_jnls.track3);
	if(s > 0 )
		setbit(siso, 36, (unsigned char *)pep_jnls.track3, s);
	
	setbit(siso, 41, (unsigned char *)pep_jnls.termcde, 8);
	setbit(siso, 42, (unsigned char *)pep_jnls.mercode, 15);
	
	setbit(siso, 48, (unsigned char *)ewp_info.consumer_orderno, getstrlen(ewp_info.consumer_orderno));
	setbit(siso, 49, (unsigned char *)"156", 3);
	setbit(siso,60,(unsigned char *)"00000000030000000000", 20);
	
	//TODO ... 需要考虑前置失败无法获取授权码的情况
	/*
	61自定义数据字段
	原交易授权号		N6
	参考号			N12
	时间				N10
	*/
	memset(tmpbuf, 0, sizeof(tmpbuf));
	if( getstrlen( pep_jnls.authcode ) == 0 )
		sprintf(tmpbuf, "%s%s%s", "000000", ewp_info.consumer_sysreferno, pep_jnls.revdate);
	else
		sprintf(tmpbuf, "%s%s%s", pep_jnls.authcode, ewp_info.consumer_sysreferno, pep_jnls.revdate);
	setbit(siso, 61, (unsigned char *)tmpbuf, 28);
	return 0;
}
