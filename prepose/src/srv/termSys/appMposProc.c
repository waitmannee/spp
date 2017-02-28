#include "ibdcs.h"
#include "trans.h"

extern struct ISO_8583 *iso8583;
extern struct ISO_8583 iso8583_conf[128];
extern struct ISO_8583 iso8583conf[128];
extern struct ISO_8583 iso8583posconf[128];

/* 
 app+MPos终端交易
 */
void appMposProc(char *srcBuf, int iFid, int iMid, int iLen)
{
#ifdef __LOG__
	dcs_debug(srcBuf, iLen, "APP+POS终端交易, iFid=%d, iMid=%d, iLen=%d", iFid, iMid, iLen);
#else
	dcs_log(0, 0, "APP+POS终端交易, iFid=%d, iMid=%d, iLen=%d", iFid, iMid, iLen);
#endif

	int rtn, s, numcase;
	char tmpbuf[512], trace[7], databuf[15], currentDate[20];
	char macblock[1024], posmac[9];
	char cur_version[5];
	//存放解包处理后的实际数据
	char rtndata[1024];
	//存放解包处理后的实际数据长度
	int actual_len;

	ISO_data siso;

	//交易记录
	PEP_JNLS pep_jnls;

	//KSN结构体
	KSN_INFO ksn_info;
	//DUKPT
	char ksnbuf[15 + 1];
	char resultTemp[32 + 1];
	char outPin[20];

	char appMposTpdu[11];

	struct tm *tm;
	time_t t;

	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(trace, 0, sizeof(trace));
	memset(&siso, 0, sizeof(ISO_data));
	memset(&pep_jnls, 0, sizeof(PEP_JNLS));

	memset(databuf, 0, sizeof(databuf));
	memset(currentDate, 0, sizeof(currentDate));
	memset(macblock, 0, sizeof(macblock));
	memset(posmac, 0, sizeof(posmac));
	memset(rtndata, 0, sizeof(rtndata));
	memset(cur_version, 0, sizeof(cur_version));
	memset(appMposTpdu, 0, sizeof(appMposTpdu));

	time(&t);
	tm = localtime(&t);

	sprintf(currentDate, "%04d%02d%02d%02d%02d%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	memcpy(databuf, currentDate + 4, 10);

	//将4个字节的报文类型保存在trnsndp中，替代folder名
	memcpy(pep_jnls.trnsndp, srcBuf, 4);
	srcBuf += 4;
	iLen -= 4;

	dcs_log(0, 0, "APP MPOS交易开始解包");
	actual_len = appMposUnpack(srcBuf, &siso, appMposTpdu, iLen, rtndata);
	if (actual_len <= 0)
	{
		dcs_log(0, 0, "appMposUnpack error!");
		return;
	}
	else
		iLen = actual_len;

	dcs_log(0, 0, "解包结束");

	pep_jnls.trnsndpid = iFid;
	memcpy(pep_jnls.trnmsgd, appMposTpdu + 6, 4);
#ifdef __LOG__
	dcs_log(0, 0, "appMposTpdu=[%s]", appMposTpdu);
	dcs_log(0, 0, "pep_jnls.trnmsgd=[%s]", pep_jnls.trnmsgd);
#endif

	rtn = GetInfo_fiso(siso, &pep_jnls);
	if (rtn == -1)
	{
		if (strlen(pep_jnls.trancde) < 3)
		{
			dcs_log(0, 0, "取20域交易码失败");
			return;
		}
		dcs_log(0, 0, "某个数据域有问题！");
		BackToAppMpos("H9", siso, pep_jnls, databuf, appMposTpdu);
		return;
	}

	s = getbit(&siso, 11, (unsigned char *) pep_jnls.termtrc);
	if (s <= 0)
	{
		dcs_log(0, 0, "取终端流水失败");
		return;
	}

	memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso, 0, (unsigned char *) tmpbuf);
	if (s <= 0)
	{
		dcs_log(0, 0, "报文数据信息错误");
		return;
	}
	sscanf(tmpbuf, "%d", &numcase);
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso, 21, (unsigned char *) tmpbuf);
	if (s <= 0)
	{
		dcs_log(0, 0, "报文数据信息错误");
		return;
	}
/*
	//根据KSN获取终端的密钥信息
	memcpy(ksn_info.ksn, tmpbuf, 20);

	//将送上来的20字节ksn号转换为15字节的ksn机具号,去除counter值
	memset(ksnbuf, 0x00, sizeof(ksnbuf));
	asc_to_bcd(ksnbuf, ksn_info.ksn, 20, 0);
	ksnbuf[7] &= 0xE0;
	ksnbuf[8] = 0x00;
	ksnbuf[9] = 0x00;

	memcpy(ksn_info.initksn, ksnbuf, 5);
	bcd_to_asc(ksn_info.initksn + 5, ksnbuf + 5, 10, 0);

	//bcd_to_asc(ksn_info.initksn, ksnbuf, 20, 0);//test
	s = GetMposInfoByKsn(&ksn_info);
	if (s < 0)
	{
		dcs_log(0, 0, "查询ksn_info失败,ksn未找到, ksn=%s", ksn_info.initksn);
		setbit(&siso, 39, (unsigned char *) RET_CODE_FC, 2);
		BackToAppMpos((unsigned char *) RET_CODE_FC, siso, pep_jnls, databuf, appMposTpdu);
		return;
	}
	s = MakeDukptKey(&ksn_info);
	if (s < 0)
	{
		dcs_log(0, 0, "MakeDukptKey失败");
		setbit(&siso, 39, (unsigned char *) RET_CODE_F1, 2);
		BackToAppMpos((unsigned char *) RET_CODE_F1, siso, pep_jnls, databuf, appMposTpdu);
		return;
	}

	//从报文域里获取到当前的KSN编号，计算出MAK，PIK，TDK参与mac校验，PIN转加密，磁道解密等
	//校验mac
	if (numcase == 200 && memcmp(pep_jnls.trancde, "000", 3) == 0)
	{
		dcs_debug( rtndata, iLen-8, "macblock:");
		memset(tmpbuf, 0, sizeof(tmpbuf));
		s = getbit(&siso, 64, (unsigned char *) tmpbuf);
		if (s <= 0)
		{
			dcs_log(0, 0, "bit_64 error", posmac);
			BackToAppMpos("H9", siso, pep_jnls, databuf, appMposTpdu);
			return;
		}
		dcs_log(0, 0, "终端上送的Mac = [%s]", tmpbuf);
		//校验mac
		s = appMposMacCheck(ksn_info.tak, rtndata, iLen - 8, tmpbuf);
		if (s != 0)
		{
			dcs_log(0, 0, "mac校验失败");
			setbit(&siso, 39, (unsigned char *) "A0", 2);
			setbit(&siso, 11, (unsigned char *) trace, 6);
			BackToAppMpos("A0", siso, pep_jnls, databuf, appMposTpdu);
			return;
		}
		dcs_log(0, 0, "mac校验成功");
	}
	//磁道解密
	s = appMposTrackDecrypt(&ksn_info,&siso);
	if (s != 0)
	{
		dcs_log(0, 0, "磁道解密失败");
		setbit(&siso, 39, (unsigned char *) "F3", 2);
		BackToAppMpos("F3", siso, pep_jnls, databuf, appMposTpdu);
		return;
	}
	//转加密密码
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(outPin, 0, sizeof(outPin));
	s = getbit(&siso, 52, (unsigned char *)tmpbuf);
	if(s <= 0)
	{
		dcs_log(0, 0, "get bit_52 error!");
		BackToAppMpos("H9", siso, pep_jnls, databuf, appMposTpdu);
		return ;
	}
	//转加密
	s =  MposPinConver(ksn_info.tpk, tmpbuf, pep_jnls.outcdno, outPin);
	if(s < 0)
	{
		dcs_log(0,0,"转加密密码错误,error code=%d",s);
		setbit(&siso, 39, (unsigned char *) "F7", 2);
		BackToAppMpos("F7", siso, pep_jnls, databuf, appMposTpdu);
		return;
	}
	*/
	memcpy(iso8583_conf, iso8583conf, sizeof(iso8583conf));
	switch (numcase)
	{
	case 200:
		memset(tmpbuf, 0, sizeof(tmpbuf));
		getbit(&siso, 3, (unsigned char *) tmpbuf);

		//消费交易 or 预授权完成请求 离线消费交易上送
		if (memcmp(tmpbuf, "000000", 6) == 0)
		{
			dcs_log(0, 0, "消费交易的处理");
			rtn = appMposappMposProcessTrade(siso, pep_jnls, appMposTpdu, currentDate);
			dcs_log(0, 0, "消费交易的处理结果[%d]", rtn);
			if (rtn == -1)
			{
				dcs_log(0, 0, "交易的处理error");
				break;
			}
			else
				dcs_log(0, 0, "交易的处理success");
			break;
		}
		//余额查询类的交易
		else if (memcmp(tmpbuf, "310000", 6) == 0)
		{
			dcs_log(0, 0, "余额查询交易处理");
			break;
		}
		break;

	}
	return;
}

int appMposUnpack(char *srcBuf, ISO_data *iso, char *appMpos_tpdu, int iLen, char *resdata)
{
	char tempbuf[20];

	if (iLen < 50)
	{
		dcs_log(0, 0, "appMposUnpack error");
		return -1;
	}

	memset(tempbuf, 0, sizeof(tempbuf));

	//去掉TPDU(bcd 5)
	memcpy(tempbuf, srcBuf, 5);
	bcd_to_asc((unsigned char *) appMpos_tpdu, (unsigned char *) tempbuf, 10, 0);
	dcs_log(0, 0, "tempbuf TPDU：[%s]", appMpos_tpdu);

	iLen -= 5;
	srcBuf += 5;
	//去掉报文头2
	iLen -= 6;
	srcBuf += 6;
	//去掉其他头信息
	iLen -= 28;
	srcBuf += 28;
	//报文以明文的方式传输
	memcpy(iso8583_conf, iso8583posconf, sizeof(iso8583posconf));
	iso8583 = &iso8583_conf[0];
	SetIsoHeardFlag(0);
	SetIsoFieldLengthFlag(0);

	if (str_to_iso((unsigned char *) srcBuf, iso, iLen) < 0)
	{
		dcs_log(0, 0, "str_to_iso error ");
		return -1;
	}
	memcpy(resdata, srcBuf, iLen);
	return iLen;
}

int Sndbacktoappmpos(ISO_data iso, int fMid, char *tpdu)
{
	char buffer[1024], sendBuff[2048], tmp1[11], tmpbuf[37];
	int s, rtn = 0, i = 0, actual_len = 0;
	char flag;

	memset(buffer, 0, sizeof(buffer));
	memset(sendBuff, 0, sizeof(sendBuff));
	memset(tmp1, 0, sizeof(tmp1));
	memset(tmpbuf, 0, sizeof(tmpbuf));

	memcpy(iso8583_conf, iso8583posconf, sizeof(iso8583posconf));
	iso8583 = &iso8583_conf[0];
	SetIsoHeardFlag(0);
	SetIsoFieldLengthFlag(0);

	s = iso_to_str((unsigned char *) buffer, &iso, 1);
	if (s < 0)
	{
		dcs_log(0, 0, "iso_to_str error ");
		return -2;
	}
	asc_to_bcd((unsigned char *) tmp1, (unsigned char*) tpdu, 10, 0);
	memcpy(tmpbuf, tmp1, 1);
	memcpy(tmpbuf + 1, tmp1 + 3, 2);
	memcpy(tmpbuf + 3, tmp1 + 1, 2);
	memcpy(sendBuff, tmpbuf, 5);

	memcpy(sendBuff + 5, "1111111111111111111111111111111111", 34);

	memcpy(sendBuff + 39, buffer, s);

#ifdef __LOG__
	dcs_debug(sendBuff, s+39, "POS数据返回终端, foldId =%d, len =%d", fMid, s+39);
#else
	dcs_log(0, 0, "POS数据返回终端, foldId =%d, len =%d", fMid, s+39);
#endif

	s = fold_write(fMid, -1, sendBuff, s + 39);
	if (s < 0)
	{
		dcs_log(0, 0, "fold_write() failed:%s", ise_strerror(errno));
		return -3;
	}
	return 0;
}

int writeAppMpos(char *isodata, int len, int fMid, char *tpdu)
{
	char sendBuff[2048], rtnData[1024], tmp1[37];
	int s, rtn = 0, i = 0, actual_len = 0;

	memset(sendBuff, 0, sizeof(sendBuff));
	memset(tmp1, 0, sizeof(tmp1));
	memset(rtnData, 0, sizeof(rtnData));

	actual_len = len;
	asc_to_bcd((unsigned char *) tmp1, (unsigned char*) tpdu, 22, 0);
	memcpy(sendBuff, tmp1, 11);

	memcpy(sendBuff + 11, isodata, rtn);
	s = rtn + 11;
#ifdef __LOG__
	dcs_debug(sendBuff, s, "POS数据返回终端，tpdu=%s, foldId =%d, len =%d", tpdu, fMid, s);
#else
	dcs_log(0, 0, "POS数据返回终端，tpdu=%s, foldId =%d, len =%d", tpdu, fMid, s);
#endif

	s = fold_write(fMid, -1, sendBuff, s);

	if (s < 0)
	{
		dcs_log(0, 0, "fold_write() failed:%s", ise_strerror(errno));
		return -3;
	}

	return 0;
}

int GetAppMposMacBlk(ISO_data *iso, char *macblk)
{
	char buffer[1024];
	int len;

	memset(buffer, 0, sizeof(buffer));

	memcpy(iso8583_conf, iso8583posconf, sizeof(iso8583posconf));
	iso8583 = &iso8583_conf[0];
	SetIsoHeardFlag(0);
	SetIsoFieldLengthFlag(0);
	len = iso_to_str((unsigned char *) buffer, iso, 0);

#ifdef __LOG__
	dcs_debug( buffer, len-8, "POS计算mac数据 len=%d.", len-8 );
#endif

	memcpy(macblk, buffer, len - 8);

	return len - 8;
}

/*
 函数功能：返回pos机错误的信息应答
 */
int BackToAppMpos(char *retcode, ISO_data siso, PEP_JNLS pep_jnls, char *curdate, char *tpdu)
{
	char tmpbuf[127];
	int len = 0;

	dcs_log(0, 0, "BackToAppMpos");

	memset(tmpbuf, 0, sizeof(tmpbuf));

	if (memcmp(pep_jnls.trancde, "000", 3) == 0 || memcmp(pep_jnls.trancde, "010", 3) == 0)
	{
		//消费和余额查询和撤销
		setbit(&siso, 0, (unsigned char *) "0210", 4);
		setbit(&siso, 2, (unsigned char *) pep_jnls.outcdno, getstrlen(pep_jnls.outcdno));
		setbit(&siso, 12, (unsigned char *) curdate + 4, 6);
		setbit(&siso, 13, (unsigned char *) curdate, 4);
		setbit(&siso, 32, (unsigned char *) "40002900", 8);

		memset(tmpbuf, 0, sizeof(tmpbuf));
		if (getstrlen(pep_jnls.termtrc) == 0)
			memcpy(tmpbuf, curdate, 6);
		else
		{
			memcpy(tmpbuf, pep_jnls.termtrc, 6);
			setbit(&siso, 11, (unsigned char *) pep_jnls.termtrc, 6);
		}
		setbit(&siso, 37, (unsigned char *) tmpbuf, 12);
		setbit(&siso, 39, (unsigned char *) retcode, 2);
		setbit(&siso, 44, (unsigned char *) "00000000   00000000   ", 22);

	}

	memset(tmpbuf, 0, sizeof(tmpbuf));
	memcpy(tmpbuf, pep_jnls.trancde, 3);
	if (memcmp(pep_jnls.billmsg, "DY", 2) == 0)
	{
		len = 0;
		sprintf(tmpbuf + 3, "%02d", len);
		setbit(&siso, 20, (unsigned char *) tmpbuf, 5);
	}
	else
	{
		len = getstrlen(pep_jnls.billmsg);
		sprintf(tmpbuf + 3, "%02d", len);
		memcpy(tmpbuf + 5, pep_jnls.billmsg, len);
		setbit(&siso, 20, (unsigned char *) tmpbuf, len + 5);
	}
	setbit(&siso, 7, (unsigned char *) NULL, 0);
	setbit(&siso, 21, (unsigned char *) NULL, 0);
	setbit(&siso, 22, (unsigned char *) NULL, 0);
	setbit(&siso, 26, (unsigned char *) NULL, 0);
	setbit(&siso, 35, (unsigned char *) NULL, 0);
	setbit(&siso, 36, (unsigned char *) NULL, 0);
	setbit(&siso, 48, (unsigned char *) NULL, 0);
	setbit(&siso, 52, (unsigned char *) NULL, 0);
	setbit(&siso, 53, (unsigned char *) NULL, 0);
	setbit(&siso, 64, (unsigned char *) NULL, 0);
	setbit(&siso, 62, (unsigned char *) NULL, 0);

	Sndbacktoappmpos(siso, pep_jnls.trnsndpid, tpdu);
	return 0;
}

/*get  bit_20 ,Bit_21,bit_23*/
int GetInfo_fiso(ISO_data siso, PEP_JNLS *pep_jnls)
{
	char tmpbuf[127], temp_len1[3];
	int s, temp_len2, len;

	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(temp_len1, 0, sizeof(temp_len1));

	s = getbit(&siso, 20, (unsigned char *) tmpbuf);
	dcs_log(0, 0, "pos报文20域：[%s], s= [%d]", tmpbuf, s);

	if (s >= 5)
	{
		memcpy(pep_jnls->trancde, tmpbuf, 3);
		memcpy(temp_len1, tmpbuf + 3, 2);
		sscanf(temp_len1, "%d", &temp_len2);
		if (temp_len2 != 0)
		{
			memcpy(pep_jnls->billmsg, tmpbuf + 5, temp_len2);
			dcs_log(0, 0, "订单号：[%s]", pep_jnls->billmsg);
		}
	}
	else
	{
		dcs_log(0, 0, "Error,20域的数据有问题,长度[%d],值[%s].", s, tmpbuf);
		return -1;
	}

	memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso, 21, (unsigned char *) tmpbuf);
	if (s <= 0)
	{
		dcs_log(0, 0, "Error,取21域数据错误,长度[%d]", s);
		return -1;
	}
	dcs_log(0, 0, "21域实际长度 = %d", s);
	len = 0;
	/*取机具编号信息*/
	s -= 2;
	if (s < 0)
	{
		dcs_log(0, 0, "21域数据错误");
		return -1;
	}

	dcs_debug(tmpbuf, s+2, "21域数据:");
	memset(temp_len1, 0, sizeof(temp_len1));
	memcpy(temp_len1, tmpbuf, 2);
	len += 2;
	dcs_log(0, 0, "temp_len1 = %s", temp_len1);
	sscanf(temp_len1, "%d", &temp_len2);
	dcs_log(0, 0, "temp_len2 = %d", temp_len2);
	if (temp_len2 > 25)
	{
		dcs_log(0, 0, "机具编号长度不应该超过25");
		return -1;
	}
	else if (temp_len2 != 0)
	{
		memcpy(pep_jnls->termidm, tmpbuf + 2, temp_len2);
		dcs_log(0, 0, "pep_jnls->termidm =[%s]", pep_jnls->termidm);
		len += temp_len2;
	}
	s -= temp_len2;
	if (s < 0)
	{
		dcs_log(0, 0, "21域数据错误");
		return -1;
	}
	/*取GPRS定位信息*/
	memset(temp_len1, 0, sizeof(temp_len1));
	memcpy(temp_len1, tmpbuf + len, 2);
	len += 2;
	sscanf(temp_len1, "%d", &temp_len2);
	s -= 2;
	if (s < 0)
	{
		dcs_log(0, 0, "21域数据错误");
		return -1;
	}
	if (temp_len2 != 0)
	{
		memcpy(pep_jnls->gpsid, tmpbuf + len, temp_len2);
		len += temp_len2;
	}
	s -= temp_len2;
	if (s < 0)
	{
		dcs_log(0, 0, "21域数据错误");
		return -1;
	}
	/*取APN信息*/
	memset(temp_len1, 0, sizeof(temp_len1));
	memcpy(temp_len1, tmpbuf + len, 2);
	len += 2;
	sscanf(temp_len1, "%d", &temp_len2);
	s -= 2;
	if (s < 0)
	{
		dcs_log(0, 0, "21域数据错误");
		return -1;
	}
	if (temp_len2 != 0 && temp_len2 < 26)
	{
		memcpy(pep_jnls->termid, tmpbuf + len, temp_len2);
		len += temp_len2;
		dcs_log(0, 0, "len = %d", len);
	}
	else if (temp_len2 > 25)
	{
		dcs_log(0, 0, "21.3域数据错误");
	}
	s -= temp_len2;
	if (s == 0)
		dcs_log(0, 0, "21域处理完毕长度= %d", s);

	return 0;
}

/*处理消费撤销 、退货、消费、余额查询、脚本通知*/
int appMposappMposProcessTrade(ISO_data siso, PEP_JNLS pep_jnls, char *tpdu, char *currentDate)
{
	char tmpbuf[227], trace[6 + 1], outPin[8 + 1], bit_22[3 + 1], databuf[10 + 1];

	int s, rtn, n;
	char outCardType;

	//交易类型配置信息表，余额查询根据交易码查询数据库表msgchg_info，得到的信息放入该结构体中
	MSGCHGINFO msginfo;

	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(trace, 0, sizeof(trace));
	memset(outPin, 0, sizeof(outPin));
	memset(bit_22, 0, sizeof(bit_22));
	memset(databuf, 0, sizeof(databuf));
	memset(&msginfo, 0, sizeof(MSGCHGINFO));

	memcpy(databuf, currentDate + 4, 10);

	if (pub_get_trace_from_seq(trace) < 0)
	{
		dcs_log(0, 0, "取前置系统交易流水错误");
		BackToAppMpos("96", siso, pep_jnls, databuf, tpdu);
		return -1;
	}
	/*交易码来查询数据库msgchg_info来读取一些信息*/
	rtn = Readmsgchg(pep_jnls.trancde, &msginfo);
	if (rtn == -1)
	{
		dcs_log(0, 0, "查询msgchg_info失败,交易未开通, trancde=%s", pep_jnls.trancde);
		BackToAppMpos("96", siso, pep_jnls, databuf, tpdu);
		return -1;
	}
	else
	{
		if (memcmp(msginfo.flagpe, "Y", 1) != 0 && memcmp(msginfo.flagpe, "y", 1) != 0)
		{
			dcs_log(0, 0, "由于系统或者其他原因，交易暂时关闭");
			BackToAppMpos(RET_TRADE_CLOSED, siso, pep_jnls, databuf, tpdu);
			return -1;
		}

	}
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso, 53, (unsigned char *) tmpbuf);
	if (s > 0 && memcmp(tmpbuf + 2, "1", 1) == 0)
	{
		//磁道解密
		//s = PosTrackDecrypt(mer_info_t, &siso);
		if (s < 0)
		{
			dcs_log(0, 0, "磁道解密错误");
			BackToAppMpos("F3", siso, pep_jnls, databuf, tpdu);
			return -1;
		}
	}

	else if (memcmp(pep_jnls.trancde, "000", 3) == 0)
	{
		if (memcmp(pep_jnls.trancde, "000", 3) == 0)
		{
			dcs_log(0, 0, "消费类的交易处理");
			memcpy(pep_jnls.process, "000000", 6);
			setbit(&siso, 0, (unsigned char *) msginfo.msgtype, 4);
			setbit(&siso, 25, (unsigned char *) msginfo.service, 2);
			setbit(&siso, 3, (unsigned char *) msginfo.process, 6);
		}
		memset(tmpbuf, 0, sizeof(tmpbuf));
		s = getbit(&siso, 35, (unsigned char *) tmpbuf);
		if (s > 0)
		{
			for (n = 0; n < 19; n++)
			{
				if (tmpbuf[n] == '=')
					break;
			}
			memcpy(pep_jnls.outcdno, tmpbuf, n);
			rtn = 0;
		}
		else
			rtn = -2;
	}
	s = getstrlen(pep_jnls.outcdno);
	if (s > 0)
	{
		dcs_log(0, 0, "有二磁道");
		setbit(&siso, 2, (unsigned char *) pep_jnls.outcdno, s);
	}
	else
	{
		dcs_log(0, 0, "无二磁道");
		memset(pep_jnls.outcdno, 0, sizeof(pep_jnls.outcdno));
		s = getbit(&siso, 2, (unsigned char *) pep_jnls.outcdno);
	}
	if (s > 0)
	{
		pep_jnls.outcdno[s] = '\0';
		outCardType = ReadCardType(pep_jnls.outcdno);
		pep_jnls.outcdtype = outCardType;
	}
	else
	{
		dcs_log(0, 0, "无主账号信息");
	}

	s = GetInfo_fiso(siso, &pep_jnls);
	if (s < 0)
	{
		dcs_log(0, 0, "get bit_20 21 error!");
		return -1;
	}
	s = getbit(&siso, 11, (unsigned char *) pep_jnls.termtrc);
	if (s <= 0)
	{
		dcs_log(0, 0, "get bit_11 error!");
		return -1;
	}
	setbit(&siso, 7, (unsigned char *) databuf, 10);
	setbit(&siso, 11, (unsigned char *) trace, 6);

	s = getbit(&siso, 22, (unsigned char *) bit_22);
	if (s <= 0)
	{
		dcs_log(0, 0, "get bit_22 error!");
		BackToAppMpos("H9", siso, pep_jnls, databuf, tpdu);
		return -1;
	}

	dcs_log(0, 0, "bit_22-11=[%s]", bit_22);
	dcs_log(0, 0, "pep_jnls.trancde-11=[%s]", pep_jnls.trancde);

	if (memcmp(bit_22, "02", 2) != 0 && memcmp(bit_22, "05", 2) != 0 && memcmp(bit_22, "95", 2) != 0
			&& (memcmp(pep_jnls.trancde, "000", 3) == 0 || memcmp(pep_jnls.trancde, "010", 3) == 0))
	{
		dcs_log(0, 0, "错误:   不是刷卡交易，暂时不受理");
		setbit(&siso, 39, (unsigned char *) "HC", 2);
		BackToAppMpos("HC", siso, pep_jnls, databuf, tpdu);
		return -1;
	}
	/*消费撤销和脚本通知,TC值上送没有52域*/
	if (memcmp(bit_22 + 2, "1", 1) == 0 && memcmp(pep_jnls.trancde, "001", 3) != 0 && memcmp(pep_jnls.trancde, "04", 2) != 0)
	{
		memset(tmpbuf, 0, sizeof(tmpbuf));
		s = getbit(&siso, 52, (unsigned char *) tmpbuf);
		if (s <= 0)
		{
			dcs_log(0, 0, "get bit_52 error!");
			BackToAppMpos("H9", siso, pep_jnls, databuf, tpdu);
			return -1;
		}
#ifdef __TEST__
		dcs_debug(tmpbuf, s, "终端报文52域:");
#endif
		//PIN转加密的PIK
		/*
		 s = PosPinConvert(mer_info_t.mgr_pik, "a596ee13f9a93800", pep_jnls.outcdno, tmpbuf, outPin);
		 if(s == 1)
		 {
		 #ifdef __TEST__
		 dcs_debug(outPin, 8, "设置发送核心系统的报文52域:");
		 #endif
		 setbit(&siso, 52, (unsigned char *)outPin, 8);

		 memset(tmpbuf, 0x00, sizeof(tmpbuf));
		 s = getbit(&siso, 53, (unsigned char *)tmpbuf);
		 if(s>0 && memcmp(pep_jnls.trancde, "03", 2) == 0)
		 {
		 dcs_log(0, 0, "tmpbuf1=[%s]", tmpbuf);
		 memcpy(tmpbuf+2, "0", 1);
		 dcs_log(0, 0, "tmpbuf2=[%s]", tmpbuf);
		 setbit(&siso, 53, (unsigned char *)tmpbuf, 16);
		 }
		 else
		 setbit(&siso, 53, (unsigned char *)"1000000000000000", 16);
		 }
		 else
		 {
		 dcs_log(0, 0, "pin转加密失败。");
		 setbit(&siso, 39, (unsigned char *)RET_CODE_F7, 2);
		 BackToAppMpos(RET_CODE_F7, siso, pep_jnls, databuf, tpdu);
		 return -1;
		 }
		 */
	}

	//二维码demo测试用
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso, 63, (unsigned char *) tmpbuf);
	if (s > 0 && memcmp(tmpbuf, "EWM", 3) == 0)
	{
		//配合demo演示，直接返回成功，并通知商城
		dcs_log(0, 0, "演示直接返回");
		BackToAppMpos("00", siso, pep_jnls, databuf, tpdu);
		Analy2code(tmpbuf + 3, s - 3, trace, currentDate);
		return 0;
		setbit(&siso, 63, (unsigned char *) NULL, 0);
	}
	return 0;

	/*保存pep_jnls数据库*/
	rtn = WritePosXpep(siso, pep_jnls, databuf);
	if (rtn == -1)
	{
		dcs_log(0, 0, "保存pep_jnls数据失败");
		BackToAppMpos("96", siso, pep_jnls, databuf, tpdu);
		return -1;
	}
	setbit(&siso, 20, (unsigned char *) NULL, 0);
	setbit(&siso, 64, (unsigned char *) NULL, 0);

	/*发送报文*/
	s = WriteXpe(siso);
	if (s == -1)
	{
		dcs_log(0, 0, "发送信息给核心失败");
		BackToAppMpos("96", siso, pep_jnls, databuf, tpdu);
		return -1;
	}
	dcs_log(0, 0, "数据发送成功[%d]", s);
	return 0;
}

/**
 * 校验手机app Mpos mac
 * Parma:tak  dukpt算法的mac计算密钥,source 参与计算的数据内存起始地址,len数据长度,mac 需要校验的mac值
 * return: -1 表示失败 ，0表示成功
 */
int appMposMacCheck(char * tak, char *source, int len, char *mac)
{
	int result = -1;
	unsigned char resultTemp[16 + 1], mak[32 + 1];

	if (source == NULL || tak == NULL || mac == NULL)
		return -1;

	memset(resultTemp, 0, sizeof(resultTemp));
	memset(mak, 0x00, sizeof(mak));

	result = getmposlmk(tak, resultTemp, 16, 2);
	if (result < 0)
	{
		dcs_log(0, 0, "加密mac密钥失败");
		return -1;
	}
#ifdef  __TEST__
	dcs_debug(resultTemp, 16, "LMK加密mackey:");
#endif

	bcd_to_asc(mak, resultTemp, 32, 0);
	memset(resultTemp, 0, sizeof(resultTemp));
	result = mpos_mac(mak, mac, source, len, resultTemp);
	if (result == 0)
	{
		return 0;
	}
	else
		return -1;
}
/**
 * app Mpos 磁道解密
 */
int appMposTrackDecrypt(KSN_INFO*ksn_info, ISO_data *iso)
{
	char track2[38], track3[105], tmp[17], rtndata[9], bcd_buf[8 + 1];
	char tdkey[16 + 1];
	int s, len;

	memset(track2, 0, sizeof(track2));
	memset(track3, 0, sizeof(track3));
	memset(tmp, 0, sizeof(tmp));
	memset(rtndata, 0, sizeof(rtndata));
	memset(tdkey, 0, sizeof(tdkey));

	dcs_log(0, 0, "磁道解密函数");
	//tdk 对自己进行3des加密得到真正的tdkey
	s = D3desbylen(ksn_info->tdk, 16, tdkey, ksn_info->tdk, 1);
	if (s < 0)
	{
		dcs_log(0, 0, "磁道密钥计算失败");
		return -1;
	}

	len = getbit(iso, 35, (unsigned char *) track2);
	if (len <= 0)
	{
		dcs_log(0, 0, "二磁数据不存在，返回-2.");
		return -2;
	}
	else
	{
		if (len % 2 == 0)
			memcpy(tmp, track2 + len - 18, 16);
		else
			memcpy(tmp, track2 + len - 17, 16);
#ifdef __TEST__
		dcs_log(0,0,"终端上送二磁 track2 :%s", track2);
		dcs_log(0,0,"终端上送二磁密文部分 data:%s", tmp);
#endif
		memset(bcd_buf, 0, sizeof(bcd_buf));
		asc_to_bcd(bcd_buf, tmp, 16, 0);
		//tdkey对二磁道密文进行3des解密得到明文磁道内容rtndata
		s = D3desbylen(bcd_buf, 8, rtndata, tdkey, 0);
		if (s < 0)
		{
			dcs_log(0, 0, "二磁道解密失败");
			return -1;
		}
		memset(tmp, 0, sizeof(tmp));
		bcd_to_asc((unsigned char *) tmp, (unsigned char *) rtndata, 16, 0);
#ifdef __TEST__
		dcs_log(0,0,"二磁密文部分解密之后的明文数据 data:%s", tmp);
#endif

		if (len % 2 == 0)
			memcpy(track2 + len - 18, tmp, 16);
		else
			memcpy(track2 + len - 17, tmp, 16);
		for (s = 0; s < len; s++)
		{
			if (track2[s] == 'D')
				track2[s] = '=';
		}
#ifdef __TEST__
		dcs_log(0,0,"最终明文二磁数据 track2 :%s, len:%d", track2, len);
#endif
		setbit(iso, 35, (unsigned char *) track2, len);
	}

	len = getbit(iso, 36, (unsigned char *) track3);
	if (len <= 0)
	{
		dcs_log(0, 0, "不存在三磁");
		return 0;
	}
	else
	{
		memset(tmp, 0, sizeof(tmp));
		memset(rtndata, 0, sizeof(rtndata));
		if (len % 2 == 0)
			memcpy(tmp, track3 + len - 18, 16);
		else
			memcpy(tmp, track3 + len - 17, 16);
#ifdef __TEST__
		dcs_log(0,0,"终端上送三磁 track3 :%s", track3);
		dcs_log(0,0,"终端上送三磁密文部分 data:%s", tmp);
#endif
		memset(bcd_buf, 0, sizeof(bcd_buf));
		asc_to_bcd(bcd_buf, tmp, 16, 0);
		//tdkey对三磁道密文进行3des解密得到明文磁道内容rtndata
		s = D3desbylen(bcd_buf, 8, rtndata, tdkey, 0);
		if (s < 0)
		{
			dcs_log(0, 0, "三磁道解密失败");
			return -1;
		}

		memset(tmp, 0, sizeof(tmp));
		bcd_to_asc((unsigned char *) tmp, (unsigned char *) rtndata, 16, 0);
#ifdef __TEST__
		dcs_log(0,0,"三磁密文部分解密之后的明文数据 data:%s", tmp);
#endif

		if (len % 2 == 0)
			memcpy(track3 + len - 18, tmp, 16);
		else
			memcpy(track3 + len - 17, tmp, 16);
		for (s = 0; s < len; s++)
		{
			if (track2[s] == 'D')
				track2[s] = '=';
		}
#ifdef __TEST__
		dcs_log(0,0,"最终明文三磁数据 track2 :%s, len:%d", track3, len);
#endif

		setbit(iso, 36, (unsigned char *) track3, len);
	}
}






