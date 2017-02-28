#include "ibdcs.h"
#include "trans.h"
#include "pub_function.h"

/* 
	模块说明：
		此模块主要用来配合模拟测试验证各个MPOS终端厂家提供的SDK中dukpt的密钥算法的正确性。
	2015.1.13
 */
void mposProc_yindian(char *srcBuf, int iFid, int iMid, int iLen)
{

	#ifdef __LOG__
		dcs_debug(srcBuf, iLen, "MPOS Proc, iFid=%d, iMid=%d, iLen=%d", iFid, iMid, iLen);
	#endif
	
	//变量定义
	int rtn = 0, s = 0;
	int datalen = 0;
	
	char tmpbuf[1024];
	char databuf[PUB_MAX_SLEN + 1];

	char rtnData[300];
	
	//按照《前置系统接入报文规范》解析出来的数据放入该结构体中
	EWP_INFO ewp_info; 
	
	//KSN结构体
	KSN_INFO ksn_info;
	ISO_data siso;
	//TPDU
	char tpdu_header[11];
	
	//数据长度
	int len=0; 
	int flag = 0;
	
	//KSN编号
	char ksn_bcd[10+1];
	char ksn_bcd_temp[10+1];
	char initkey[16+1];
	char dukptkey[16+1];
	char ksnbuf[20+1];
	
	char resultTemp[32+1];
	char outPin[20];
	char rcvbuf[32+1];
	
	//DUKPT
	char lmk_tdkey[32+1];
	char tdes_lmk_tdkey[32+1];
	char lmk_tdes_lmk_tdkey[32+1];
	
	memset(lmk_tdkey,0,sizeof(lmk_tdkey));
	memset(tdes_lmk_tdkey,0,sizeof(tdes_lmk_tdkey));
	memset(lmk_tdes_lmk_tdkey,0,sizeof(lmk_tdes_lmk_tdkey));
	
	memset(resultTemp,0,sizeof(resultTemp));
	memset(outPin,0,sizeof(outPin));
	memset(rcvbuf,0,sizeof(rcvbuf));
	
	
	//初始化变量
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(databuf, 0, sizeof(databuf));
	memset(ksn_bcd, 0, sizeof(ksn_bcd));
	memset(ksn_bcd_temp, 0, sizeof(ksn_bcd_temp));
	memset(initkey, 0, sizeof(initkey));
	memset(dukptkey, 0, sizeof(dukptkey));
	memset(&ewp_info, 0, sizeof(ewp_info));
	memset(&ksn_info, 0x00, sizeof(KSN_INFO));
	memset(&siso, 0x00, sizeof(ISO_data));
	memset(ksnbuf, 0, sizeof(ksnbuf));

	//得到交易链路名称
	
	if( memcmp(srcBuf, "YDCE", 4) == 0)
	{
		dcs_log(0, 0, "解析模拟银点测试报文开始");
	    srcBuf += 4;
	    iLen -= 4;
	}
	
	datalen = iLen;
	memcpy(databuf, srcBuf, datalen);
	

	//解析收到的模拟银点测试的数据
	parsemposdata_yindian(databuf, datalen, &ewp_info);
	dcs_log(0, 0, "解析模拟银点测试报文开始");
	
	//如果解析帐单数据失效的话，直接抛弃这个包
	if(memcmp(ewp_info.retcode, "00", 2) != 0)
	{
		dcs_log(0, 0, "解析帐单数据错误F1");
		WriteMPOS_yindian(iFid, "F1");
		return;
	}
	
	asc_to_bcd((unsigned char *)ksn_bcd, (unsigned char *)ewp_info.ksn, 20, 0);
	dcs_debug(ksn_bcd, 10, "ksn_bcd");
	memcpy(ksn_bcd_temp, ksn_bcd, 10);
	ksn_bcd_temp[7] &=0xE0;
	ksn_bcd_temp[8] =0x00;
	ksn_bcd_temp[9] =0x00;
	dcs_debug(ksn_bcd_temp, 10, "计数器置零后ksn_bcd_temp");
	bcd_to_asc((unsigned char *)ksnbuf, (unsigned char *)ksn_bcd_temp, 20, 0);
	dcs_log(0, 0, "转化后ksn：%s", ksnbuf);
/*
	//联迪最开始测试
	memcpy(ksn_info.ksn, ewp_info.ksn, 20);
	memcpy(ksn_info.initksn, ksnbuf, 20);
	//测试写死
	ksn_info.transfer_key_index = 802;
	ksn_info.bdk_index = 802;
	ksn_info.bdk_xor_index = 901;
	ksn_info.channel_id =106;
	*/
	memcpy(ksn_info.ksn, ewp_info.ksn, 20);
	memcpy(ksn_info.initksn, ksn_bcd, 5);
	memcpy(ksn_info.initksn+5, ksnbuf+10, 10);
	//测试写死
	ksn_info.transfer_key_index = 803;
	ksn_info.bdk_index = 802;
	ksn_info.bdk_xor_index = 901;
	ksn_info.channel_id =1;
	
	memcpy(ksnbuf, ksn_info.initksn, 15);
	ksnbuf[15]=0;
	dcs_log(0, 0, "mpos取数据开始[%s]", ksnbuf);	
	s = Getksninfo(&ksn_info, ksnbuf);
	if(s<0)
	{
		dcs_log(0 , 0, "KSN_INFO中无记录");	
		flag = 1;//需要插入数据库
	}
	else
	{
		dcs_log(0, 0, "ksn_info->ksn=[%s]", ksn_info.ksn);
		dcs_log(0, 0, "ksn_info->bdk_index=[%d]", ksn_info.bdk_index);
		dcs_log(0, 0, "ksn_info->bdk_xor_index=[%d]", ksn_info.bdk_xor_index);
		dcs_log(0, 0, "ksn_info->initkey=[%s]", ksn_info.initkey);
		dcs_log(0, 0, "mpos取数据结束");
	}
	
	//判断该KSN号对应终端是否已存在initkey，存在则跳过initkey计算、保存步骤
	if(getstrlen(ksn_info.initkey) != 32)
	{
		dcs_log(0, 0, "TESTDUKPT" );
		
		//计算DUKPT    initkey
		s = GetIPEK(ksn_bcd_temp, ksn_info.bdk_index, ksn_info.bdk_xor_index, initkey);
		if(s<0)
		{
			dcs_log(0 , 0, "GetIPEK计算失败 F1");
			WriteMPOS_yindian(iFid, "F1");
			return;
		}
		
		bcd_to_asc(ksn_info.initkey, initkey, 32,0);
		dcs_log(0, 0, "TESTDUKPT ksn_info.initkey=[%s]", ksn_info.initkey);
		if(flag ==1)
		{
			//插入数据
			s = Savefinalinfo(ksn_info,ksnbuf);
			if(s<0)
			{
				dcs_log(0 , 0, "Savefinalinfo失败F1");
				WriteMPOS_yindian(iFid, "F1");
			}
		}
		else
		{
			//保存 initkey 值到 ksn_info 数据表
			s = Saveinitkey(ksn_info);
			if(s<0)
			{
				dcs_log(0 , 0, "Saveinitkey失败F1");
				WriteMPOS_yindian(iFid, "F1");
			}	
		}
		
	}
	else
	{
		asc_to_bcd(initkey, ksn_info.initkey, 32, 0);
		dcs_log(0, 0, "initkey 已存在，忽略计算、存储过程");	
	}

	dcs_log(0, 0, "GetDukptKey" );
	
	//计算DUKPT    dukptkey (此处取值BCD码的initkey，不能使用asc码的ksn_info.initkey)
	s = GetDukptKey(ksn_bcd, initkey, dukptkey);
	if(s<0)
	{
		dcs_log(0 , 0, "GetDukptKey失败F1");
		WriteMPOS_yindian(iFid, "F1");
		return;
	}

	//保存 dukptkey 值到 ksn_info 数据表
	bcd_to_asc(ksn_info.dukptkey, dukptkey, 32, 0);
	
	s = Savedukptkey(ksn_info);
	if(s<0)
	{
		dcs_log(0 , 0, "Savedukptkey失败F1");
		WriteMPOS_yindian(iFid, "F1");
		return;	
	}
	
	dcs_log(0, 0, "ksn_info.dukptkey=[%s]", ksn_info.dukptkey);		



	//获取PINK
	getKey(ksn_info.dukptkey,SEC_KEY_TYPE_TPK,ksn_info.tpk);
	dcs_debug(ksn_info.tpk, 16, "PIN计算密钥：");

	//获取MACK
	getKey(ksn_info.dukptkey,SEC_KEY_TYPE_TAK,ksn_info.tak);
	dcs_debug(ksn_info.tak, 16, "MAC计算密钥：");

	//获取TDK
	getKey(ksn_info.dukptkey,SEC_KEY_TYPE_TDK,ksn_info.tdk);
	dcs_debug(ksn_info.tdk, 16, "磁道计算密钥：");


//MAC校验
	memset(resultTemp, 0x00, sizeof(resultTemp));
	memset(rcvbuf, 0x00, sizeof(rcvbuf));
	
	rtn = getmposlmk(ksn_info.tak, resultTemp, 16, 2);
	dcs_debug(resultTemp, 16, "LMK加密mackey:");

	bcd_to_asc(rcvbuf, resultTemp, 32, 0);

	s = mposmac_check_yindian(ewp_info, rcvbuf);
	if(s != 0)
	{
		dcs_log(0, 0, "mac校验失败A0");
	}
	else
	{
		dcs_log(0, 0, "YINDIAN mac校验成功");
	}
	
#if 0
	MposMacCheck(ksn_info.tak, ewp_info);
#endif
	
#if 0
	
	//磁道解密
	dcs_log(0, 0, "进入解密二磁道信息");
	dcs_log(0, 0, "传入二磁道信息:%s", ewp_info.consumer_trackno);

	//获取二磁道信息，并且取出需要解密部分字段	
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	len = getstrlen(ewp_info.consumer_trackno);
	if(len <= 0)
	{
		dcs_log(0, 0, "解密磁道错误F3, s=%d", s);
		WriteMPOS_yindian(iFid, "F3");
		return;
	}
	else
	{
		if(len % 2 == 0)
			memcpy(tmpbuf, ewp_info.consumer_trackno+len-18, 16);
		else
			memcpy(tmpbuf, ewp_info.consumer_trackno+len-17, 16);
		#ifdef __TEST__	
		 	dcs_log(0, 0, "终端上送二磁 ewp_info.consumer_trackno :%s", ewp_info.consumer_trackno);
		 	dcs_log(0, 0, "终端上送二磁密文部分 data:%s", tmpbuf);
		 #endif
	}
	
	//lmk加密异或得到的tdk明文密钥得到lmk_tdkey
	memset(rcvbuf, 0x00, sizeof(rcvbuf));
	rtn = getmposlmk(ksn_info.tdk, rcvbuf, 16, 3);
	
	bcd_to_asc(lmk_tdkey, rcvbuf, 32,0);
	dcs_log(0,0,"lmk_tdkey:%s", lmk_tdkey);

	//lmk_tdkey对明文tdk做加密得到tdes_lmk_tdkey，直接借用POS_PackageEncrypt接口，同样调用D112
	memset(rcvbuf, 0x00, sizeof(rcvbuf));
	s = POS_PackageEncrypt(lmk_tdkey, ksn_info.tdk, 32, tdes_lmk_tdkey);
	if(s < 0)
	{
		dcs_log(0, 0, "解密磁道错误F3, s=%d", s);
		WriteMPOS_yindian(iFid, "F3");
		return;
	}
	dcs_debug(tdes_lmk_tdkey, 16, "tdes_lmk_tdkey:");
	
	//lmk加密tdes_lmk_tdkey得到lmk_tdes_lmk_tdkey
	memset(lmk_tdes_lmk_tdkey, 0x00, sizeof(lmk_tdes_lmk_tdkey));
	rtn = getmposlmk(tdes_lmk_tdkey, lmk_tdes_lmk_tdkey, 16, 3);
	if(s < 0)
	{
		dcs_log(0, 0, "解密磁道错误F3, s=%d", s);
		WriteMPOS_yindian(iFid, "F3");
		return;
	}
	dcs_debug(resultTemp, 16, "lmk_tdes_lmk_tdkey:");

	s = MPOSTrackDecrypt(lmk_tdes_lmk_tdkey, tmpbuf, getstrlen(tmpbuf), rtnData);
	if(s < 0)
	{
		dcs_log(0, 0, "解密磁道错误F3, s=%d", s);
		WriteMPOS_yindian(iFid, "F3");
		return;
	}
	dcs_log(0, 0, "YINDIAN 磁道解密成功[%s]", rtnData);
	
	if(len % 2 == 0)
		memcpy(ewp_info.consumer_trackno+len-18, rtnData, 16);
	else
		memcpy(ewp_info.consumer_trackno+len-17, rtnData, 16);
		
	dcs_log(0,0,"解密后二磁信息 data:%s", ewp_info.consumer_trackno);
#else
	MPOS_TrackDecrypt(&ewp_info,&ksn_info,&siso);
#endif

	//PIN转加密
	if(memcmp(ewp_info.consumer_password, "FFFFFFFFFFFFFFFF", 16) == 0 || memcmp(ewp_info.consumer_password, "ffffffffffffffff", 16) == 0)
	{
		dcs_log(0, 0, "密码为空");
	}
	else
	{
			//LMK加密的pink
			rtn = getmposlmk(ksn_info.tpk, resultTemp, 16, 1);
			
			dcs_debug(resultTemp, 16, "LMK加密pinkey:");
			
			bcd_to_asc(rcvbuf, resultTemp, 32, 0);
		
			memset(outPin, 0x00,sizeof(outPin));
			memset(tmpbuf, 0x00, sizeof(tmpbuf));
			bcd_to_asc(tmpbuf, ewp_info.mw_password, 16, 0);
			//密码加密密钥
			MPOS_PinCon(rcvbuf, tmpbuf, ewp_info.consumer_cardno, outPin);
			
			dcs_debug(outPin, 8, "加密后的密码:");
			
			memset(tmpbuf, 0x00, sizeof(tmpbuf));
			bcd_to_asc(tmpbuf, outPin, 16, 0);
			dcs_log(0, 0, "电银加密后密码：%s", tmpbuf);
			dcs_log(0, 0, "终端上送加密密码：%s", ewp_info.consumer_password);
			
			if(strcasecmp(tmpbuf, ewp_info.consumer_password)==0)
			{
				dcs_log(0, 0, " 密码校验成功");
			}
			#if 0
			//密码转加密
			memset(outPin, 0x00,sizeof(outPin));
			memset(tmpbuf, 0x00, sizeof(tmpbuf));
			asc_to_bcd(tmpbuf, ewp_info.consumer_password, 16, 0);
			if(MposPinConver(ksn_info.tpk, tmpbuf,ewp_info.consumer_cardno,outPin) < 0)
			{
				dcs_debug(outPin, 8, "转加密失败");
			}
			memset(tmpbuf, 0x00, sizeof(tmpbuf));
			bcd_to_asc(tmpbuf, outPin, 16, 0);
			dcs_log(0, 0, "转加密后的密码：%s", tmpbuf);
			#endif
	}

	WriteMPOS_yindian(iFid, "00");
	return;	
}
//解析从ewp那里接收到的数据 存放到结构体EWP_INFO中
int parsemposdata_yindian(char *rcvbuf, int rcvlen, EWP_INFO *ewp_info)
{
	int check_len = 0, offset = 0, temp_len = 0;
	char tmpbuf[1024];
	
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	
	check_len = rcvlen;

	//卡号 19个字节
	memcpy(tmpbuf, rcvbuf + offset, 19);
	ewp_info->macinfo[19] = 0;
	pub_rtrim_lp(tmpbuf, 19, tmpbuf, 1);
	memcpy(ewp_info->consumer_cardno, tmpbuf, sizeof(tmpbuf));
	ewp_info->consumer_cardno[strlen(tmpbuf)]=0;
	offset += 19;
	check_len -= 19;
	dcs_log(0 ,0, "卡号=[%s]", ewp_info->consumer_cardno);	

	//KSN编号  20字节
	memcpy(ewp_info->ksn, rcvbuf + offset, 20);
	offset += 20;
	check_len -= 20;
	dcs_log(0 ,0, "KSN编号=[%s]", ewp_info->ksn);	
	
	//macblk   32
	memcpy(ewp_info->macblk, rcvbuf + offset, 32);
	ewp_info->macblk[32] = 0;
	offset += 32;
	check_len -= 32;
	dcs_log(0, 0, "macblk=[%s]", ewp_info->macblk);	
	
	//mac
	memcpy(ewp_info->macinfo, rcvbuf + offset, 8);
	ewp_info->macinfo[8] = 0;
	offset += 8;
	check_len -= 8;

	/*磁道	二磁道长度  二磁道内容  三磁道长度  三磁道内容*/
	//二磁道长度
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memcpy(tmpbuf, rcvbuf + offset, 2);
	temp_len = atoi(tmpbuf);
	offset += 2;
	check_len -= 2;
	
	//二磁道数据
	memcpy(ewp_info->trackno2, rcvbuf + offset, temp_len);
	ewp_info->trackno2[temp_len]=0;
	offset += temp_len;
	check_len -= temp_len;
	
	#ifdef __TEST__
		dcs_log(0, 0, "trackno2 len =[%d], trackno2 [%s]", temp_len, ewp_info->trackno2);
	#endif
	
	//三磁道长度
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memcpy(tmpbuf, rcvbuf + offset, 3);
	temp_len = atoi(tmpbuf);
	offset += 3;
	check_len -= 3;
	
	//三磁道数据
	memcpy(ewp_info->trackno3, rcvbuf + offset, temp_len);
	ewp_info->trackno3[temp_len]=0;
	offset += temp_len;
	check_len -= temp_len;
	
	#ifdef __TEST__
		dcs_log(0, 0, "trackno3 len =[%d], trackno3 [%s]", temp_len, ewp_info->trackno3);
	#endif

	/*密码*/
	memcpy(ewp_info->mw_password, rcvbuf + offset, 6);
	ewp_info->mw_password[6] = 0;
	offset += 6;
	check_len -= 6;
	dcs_log(0, 0, "解析EWP数据, 剩余字节=[%d]", check_len);


	/*密码密文*/
	memcpy(ewp_info->consumer_password, rcvbuf + offset, 16);
	ewp_info->consumer_password[16] = 0;
	offset += 16;
	check_len -= 16;
	dcs_log(0, 0, "解析EWP数据, 剩余字节=[%d]", check_len);

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
组返回MPOS平台的报文
*/
int WriteMPOS_yindian(int iFid , char *aprespn)
{
	int gs_comfid = -1, s = 0;
	char buffer[1024];
	
    memset(buffer, 0, sizeof(buffer));
  
    gs_comfid = iFid;
	
	#ifdef __TEST__
    if(fold_get_maxmsg(gs_comfid) < 100)
		fold_set_maxmsg(gs_comfid, 500) ;
 	#else
	if(fold_get_maxmsg(gs_comfid) <2)
		fold_set_maxmsg(gs_comfid, 20) ;
    #endif
		
	//组回EWP的报文
	s = DataBackToMPOS_yindian(buffer, aprespn);
	dcs_debug(buffer, s, "data_buf len=%d, foldId=%d", s, gs_comfid);
	fold_write(gs_comfid, -1, buffer, s);
	return 0;
}

/*
组返回模拟测试的报文
*/
int DataBackToMPOS_yindian(char *buffer, char *aprespn)
{
	dcs_log(0, 0, "data back to yindian");
	sprintf(buffer, "%s", aprespn);
	return strlen(buffer);	
}


