#include "ibdcs.h"
#include "trans.h"

extern struct ISO_8583 *iso8583;
extern struct ISO_8583 iso8583_conf[128];
extern struct ISO_8583 iso8583conf[128];
extern struct ISO_8583 iso8583posconf[128];
char card_sn[3+1];
char g_key[20+1];

static fd55 stFd55_IC[35]={{"9F26"},{"9F27"},{"9F10"},{"9F37"},{"9F36"},{"95"},{"9A"},{"9C"},{"9F02"},{"5F2A"},{"82"},{"9F1A"},{"9F03"},{"9F33"},{"9F74"},
							{"9F34"},{"9F35"},{"9F1E"},{"84"},{"9F09"},{"9F41"},{"91"},{"71"},{"72"},{"DF31"},{"9F63"},{"8A"},{"DF32"},{"DF33"},{"DF34"},
							{"9B"}, {"4F"}, {"5F34"}, {"50"}};
/* 
Pos终端交易
*/
void posProc(char * srcBuf, int iFid , int iMid, int iLen)
{
	#ifdef __LOG__
		dcs_debug(srcBuf, iLen, "POS终端交易, iFid=%d, iMid=%d, iLen=%d", iFid, iMid, iLen);
	#else
		dcs_log(0, 0, "POS终端交易, iFid=%d, iMid=%d, iLen=%d", iFid, iMid, iLen);
	#endif
	
	int rtn, s, numcase;
	char tmpbuf[512], trace[7], databuf[15], currentDate[20];
	char signtrancode[3+1], signflg[4], tran_batch_no[7], tmstatinfo[3+1], paradownloadflag[256+1];
	char tmp_workey[150];
	char macblock[1024], posmac[9];
	long batchno, pos_batchno;
	char rtncode[2+1];
	char callno[14];
	char cur_version[5], elecsign[2048], bit_55[2048], pubkeybuf[2048], bcd_pubkeybuf[1024];
	int bit_55_len ;
	char rid[10+1], capki[2+1], aid[32+1];
	//存放解包处理后的实际数据
	char rtndata[1024];
	//存放解包处理后的实际数据长度
	int actual_len;
	int messlen=0;
	char pos_jieji_amount[13], pos_jieji_count[4], pos_daiji_amount[13], pos_daiji_count[4];
	int i_pos_jieji_count, i_pos_daiji_count;
	
	XPEP_CD_INFO xpep_cd_info;

	ISO_data siso;
	
	//交易记录
	PEP_JNLS pep_jnls;
	
	//终端商户记录表
	MER_INFO_T mer_info_t;
	
	//签到记录表
	MER_SIGNIN_INFO mer_signin_info;
	
	ICPUBKINFO icpubkinfo;
	struct  tm *tm;   
	time_t  t;

	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(tmp_workey, 0, sizeof(tmp_workey));
	memset(trace, 0, sizeof(trace));
	memset(&siso, 0, sizeof(ISO_data));
	memset(&pep_jnls, 0, sizeof(PEP_JNLS));
	memset(&mer_info_t, 0, sizeof(MER_INFO_T));
	memset(&mer_signin_info, 0, sizeof(MER_SIGNIN_INFO));
	
	memset(databuf, 0, sizeof(databuf));
	memset(currentDate, 0, sizeof(currentDate));
	memset(macblock, 0, sizeof(macblock));
	memset(posmac, 0, sizeof(posmac));
	memset(rtncode, 0, sizeof(rtncode));
	memset(rtndata, 0, sizeof(rtndata));
	memset(cur_version, 0, sizeof(cur_version));
	
	memset(&xpep_cd_info, 0, sizeof(XPEP_CD_INFO));
	memset(callno, 0, sizeof(callno));
	memset(elecsign, 0, sizeof(elecsign));
	memset(bit_55, 0, sizeof(bit_55));
	memset(tmstatinfo, 0, sizeof(tmstatinfo));
	memset(paradownloadflag, 0x00, sizeof(paradownloadflag));
	
	time(&t);
	tm = localtime(&t);

	sprintf(currentDate, "%04d%02d%02d%02d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	memcpy(databuf, currentDate+4, 10);

	//将4个字节的报文类型保存在trnsndp中，替代folder名
	memcpy(pep_jnls.trnsndp, srcBuf, 4);
	srcBuf += 4;
	iLen -= 4;
	messlen = iLen;
	
	dcs_log(0, 0, "POS交易开始解包");
	actual_len = posUnpack(srcBuf, &siso, &mer_info_t, iLen, rtndata);
	if(actual_len <= 0)
	{
		dcs_log(0, 0, "posUnpack error!");
		return;
	}
	else
		iLen = actual_len;

	dcs_log(0, 0, "解包结束");

	pep_jnls.trnsndpid = iFid ;
	memcpy(pep_jnls.trnmsgd, mer_info_t.mgr_tpdu+6, 4);
	memcpy(pep_jnls.termid, mer_info_t.pos_telno, getstrlen(mer_info_t.pos_telno));
#ifdef __LOG__
		 dcs_log(0, 0, "mer_info_t.mgr_tpdu=[%s]", mer_info_t.mgr_tpdu);
		 dcs_log(0, 0, "pep_jnls.trnmsgd=[%s]", pep_jnls.trnmsgd);
		 dcs_log(0, 0, "mer_info_t.pos_telno=[%s]", mer_info_t.pos_telno);
		 dcs_log(0, 0, "pep_jnls.termid=[%s]", pep_jnls.termid);
		 dcs_log(0, 0, "mer_info_t.cur_version_info=[%s]", mer_info_t.cur_version_info);
		 dcs_log(0, 0, "mer_info_t.encrpy_flag=[%d]", mer_info_t.encrpy_flag);
#endif
	s = getstrlen(mer_info_t.pos_telno);
	if(s!=0)
	{ 	
		if(memcmp(mer_info_t.pos_telno, "021", 3)==0)
		{
			s -=3;
			memcpy(callno, mer_info_t.pos_telno+3, s);
		}
		else
			memcpy(callno, mer_info_t.pos_telno, s);
		callno[s] = 0;
	}
    
	rtn = GetBit_fiso(siso, &pep_jnls);
	if(rtn == -1)
	{
		if(strlen(pep_jnls.trancde)< 3)
		{
			dcs_log(0, 0, "取20域交易码失败");
			return;
		}
		dcs_log(0, 0, "某个数据域有问题！");
		BackToPos("H9", siso, pep_jnls, databuf, mer_info_t);
		return;
	}
	
	//管理类信息保存消息类型
	if(getbit(&siso, 0, (unsigned char *)pep_jnls.msgtype)<0)
	{
		dcs_log(0, 0, "can not get bit_0!");	
		return;
	}
	
	//防重放,此调用需要放在获取交易码之后使用
	s = posPreventReplay(pep_jnls.trancde,pep_jnls.msgtype,srcBuf+5,messlen-5);
	if(s != 0)
		return;
	
	memset(card_sn, 0, sizeof(card_sn));
	s = getbit(&siso, 23, (unsigned char *)card_sn);
	if( s <= 0)
	{
#ifdef __LOG__
		dcs_log(0, 0, "23域无数据");
#endif
	}
	else
	{
#ifdef __LOG__
		dcs_log(0, 0, "23域为 = %s", card_sn);
#endif
	}
	
	#ifdef __LOG__
  		dcs_log(0, 0, "mer_info_t.mgr_term_id=[%s]", mer_info_t.mgr_term_id);
  		dcs_log(0, 0, "mer_info_t.mgr_mer_id=[%s]", mer_info_t.mgr_mer_id);
  		dcs_log(0, 0, "pep_jnls.termidm=[%s]", pep_jnls.termidm);
  	#endif

	rtn = getCurVersion(pep_jnls.termidm, cur_version);
	if(rtn < 0)
	{
		dcs_log(0, 0, "取当前软件版本信息失败");
		SaveErTradeInfo("HA", currentDate, "取当前的软件版本信息失败", mer_info_t, pep_jnls, pep_jnls.termtrc);
		BackToPos("HA", siso, pep_jnls, databuf, mer_info_t);
		return;
	}
	#ifdef __LOG__
  		dcs_log(0, 0, "cur_version=[%s]", cur_version);
  		dcs_log(0, 0, "mer_info_t.cur_version_info=[%s]", mer_info_t.cur_version_info);
  	#endif
	/*保存软件版本信息*/
	if(getstrlen(mer_info_t.cur_version_info)!= 0&&memcmp(cur_version, mer_info_t.cur_version_info, 4)!=0)
	{
		dcs_log(0, 0, "保存软件版本信息");
		rtn = UpdatePos_info(mer_info_t, pep_jnls.termidm, 1);	
		if(rtn < 0)
		{
			dcs_log(0, 0, "保存软件版本信息失败");
			SaveErTradeInfo("HA", currentDate, "保存软件版本信息失败", mer_info_t, pep_jnls, trace);
			BackToPos("HA", siso, pep_jnls, databuf, mer_info_t);
			return;
		}
	}
	/*只有参数下载没有流水*/
    if(memcmp(pep_jnls.trancde, "007", 3) == 0)
    {
        dcs_log(0, 0, "pos终端参数下载，终端物理编号： [%s]", pep_jnls.termidm);
        rtn = DownLoadinfo(siso, currentDate, pep_jnls.termidm, mer_info_t, iFid);
        if(rtn == -1)
        {
            dcs_log(0, 0, "POS机未入库！");
            SaveErTradeInfo(RET_CODE_HA, currentDate, "POS机未入库", mer_info_t, pep_jnls, trace);
            BackToPos("HA", siso, pep_jnls, databuf, mer_info_t);
			memcpy(pep_jnls.aprespn, "HA", 2);
        }
        else
		{
            dcs_log(0, 0, "参数下载处理成功");
			memcpy(pep_jnls.aprespn, "00", 2);
		}
		//管理类信息的保存manage_info
		rtn = Pos_SavaManageInfo(siso, pep_jnls, currentDate);
		if(rtn == -1)
		{
			dcs_log(0, 0, "保存管理类信息,参数下载失败");
		}
        return ;
    }
    
    /*IC卡参数、密钥下载没有流水*/
    if(memcmp(pep_jnls.trancde, "901", 3) == 0 || memcmp(pep_jnls.trancde, "902", 3) == 0 || memcmp(pep_jnls.trancde, "903", 3) == 0 )
	{
		rtn = DownICkeypara( siso, pep_jnls, mer_info_t, iFid, currentDate);
		if(rtn <0)
		{
			dcs_log(0, 0, "IC卡参数、密钥下载失败");
			memcpy(pep_jnls.aprespn, "ER", 2);
		}
		else
		{
			dcs_log(0, 0, "IC卡参数、密钥下载成功");
			memcpy(pep_jnls.aprespn, "00", 2);
		}
		//POS管理类信息保存
		rtn = Pos_SavaManageInfo(siso, pep_jnls, currentDate);
		if(rtn == -1)
		{
			dcs_log(0, 0, "保存管理类信息，IC卡参数、公钥下载失败");
		}
		return;
	}
    /*POS主密钥远程下载也没有流水*/
    if(memcmp(pep_jnls.trancde, "008", 3) == 0)
    {
        dcs_log(0, 0, "pos主密钥远程下载，终端物理编号： [%s]", pep_jnls.termidm);
        rtn = GetRemoteKey(siso, currentDate, pep_jnls.termidm, mer_info_t, iFid, pep_jnls.billmsg);
        if(rtn == -1)
        {
            dcs_log(0, 0, "POS机未入库！");
            SaveErTradeInfo(RET_CODE_HA, currentDate, "POS机未入库", mer_info_t, pep_jnls, trace);
            BackToPos("HA", siso, pep_jnls, databuf, mer_info_t);
            memcpy(pep_jnls.aprespn, "ER", 2);
        }
        else
		{
            dcs_log(0, 0, "pos主密钥远程下载处理成功");
			memcpy(pep_jnls.aprespn, "00", 2);
		}
		rtn = Pos_SavaManageInfo(siso, pep_jnls, currentDate);
		if(rtn == -1)
		{
			dcs_log(0, 0, "保存管理类信息，POS主密钥远程下载失败");
		}
        return ;
    }
    
    s = getbit(&siso, 11, (unsigned char *)pep_jnls.termtrc);
    if(s <= 0)
    {
        dcs_log(0, 0, "取终端流水失败");
        return;
    }
    
    /*通过商户号、终端号查询数据库表POS_CONF_INFO，保存数据到MER_INFO_T对象中*/
    s = getbit(&siso, 41, (unsigned char *)mer_info_t.mgr_term_id);
	if(s<=0)
	{
		dcs_log(0, 0, "获取终端配置信息错误");
		SaveErTradeInfo(RET_CODE_H4, currentDate, "取41域数据失败", mer_info_t, pep_jnls, pep_jnls.termtrc);
		return;
	}
		
	s = getbit(&siso, 42, (unsigned char *)mer_info_t.mgr_mer_id);
	if(s<=0)
	{
		dcs_log(0, 0, "取42域数据失败");
		SaveErTradeInfo(RET_CODE_H5, currentDate, "取42域数据失败", mer_info_t, pep_jnls, pep_jnls.termtrc);
		return;		
	}	
	
	memcpy(pep_jnls.postermcde, mer_info_t.mgr_term_id, 8);
	memcpy(pep_jnls.posmercode, mer_info_t.mgr_mer_id, 15);
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso, 0, (unsigned char *)tmpbuf);
	if(s <= 0)
	{
		dcs_log(0, 0, "报文数据信息错误");
		return;	
	}
	sscanf(tmpbuf, "%d", &numcase);
	
	rtn  = GetPosInfo(&mer_info_t, pep_jnls.termidm);
    if(rtn < 0)
    {
		dcs_log(0, 0, "获取终端配置信息错误或商户号不存在");
		SaveErTradeInfo("03", currentDate, "商户号不存在", mer_info_t, pep_jnls, trace);
		BackToPos("03", siso, pep_jnls, databuf, mer_info_t);
		return;
	}
	
	/*当是电话线接入的时候添加风控*/
	if(mer_info_t.pos_machine_type == 3)
	{
		#ifdef __TEST__
			dcs_log(0, 0, "test 主叫号码设置不验证。");
		#else
			if(memcmp(callno, "61005720", 8)==0||memcmp(callno, "61005721", 8)==0||memcmp(callno, "61630563", 8)==0
			 ||memcmp(callno, "68794511", 8)==0||memcmp(callno, "68794510", 8)==0 ||memcmp(callno, "51355988", 8)==0)
			{
				dcs_log(0, 0, "公司内部生产测试OK！");
			}
			else
			{
				s = getstrlen(mer_info_t.pos_telno);
				if(s != getstrlen(callno)|| memcmp(callno, mer_info_t.pos_telno, s)!= 0)
				{
					dcs_log(0, 0, "主叫号码设置不正确");
					SaveErTradeInfo("H9", currentDate, "风控限制", mer_info_t, pep_jnls, pep_jnls.termtrc);
					BackToPos("H9", siso, pep_jnls, databuf, mer_info_t);
					return;
				}
			}
		#endif
	}
	/*当是APN方式接入时添加风控*/
	else if(mer_info_t.pos_machine_type == 5)
	{
		#ifdef __TEST__
			dcs_log(0, 0, "test APN设置不验证。");
		#else
			s = getstrlen(pep_jnls.termid);
			dcs_log(0, 0, "pep_jnls.termid=[%s]",pep_jnls.termid);
			dcs_log(0, 0, "mer_info_t.pos_apn=[%s]",mer_info_t.pos_apn);
			if(s != getstrlen(mer_info_t.pos_apn)|| memcmp(pep_jnls.termid, mer_info_t.pos_apn, s)!= 0)
			{
				dcs_log(0, 0, "APN设置不正确");
				SaveErTradeInfo("H9", currentDate, "风控限制", mer_info_t, pep_jnls, pep_jnls.termtrc);
				BackToPos("H9", siso, pep_jnls, databuf, mer_info_t);
				return;
			}
		#endif
	}
	/*GPRS的方式接入时存上送的基站信息*/
	else if(mer_info_t.pos_machine_type == 4)
	{
		if(mer_info_t.encrpy_flag == 0 && memcmp(pep_jnls.trancde, "003", 3)!=0 && memcmp(pep_jnls.trancde, "007", 3)!=0
		&& memcmp(pep_jnls.trancde, "008", 3)!=0)
		{
			dcs_log(0, 0, "必须是密文");
			SaveErTradeInfo("H9", currentDate, "风控限制", mer_info_t, pep_jnls, pep_jnls.termtrc);
			BackToPos("H9", siso, pep_jnls, databuf, mer_info_t);
			return;
		}
		if(getstrlen(mer_info_t.pos_gpsno)== 0)
		{
			dcs_log(0, 0, "保存GPS基站信息[%s]", pep_jnls.gpsid);
			memset(mer_info_t.pos_gpsno, 0x00, sizeof(mer_info_t.pos_gpsno));
			memcpy(mer_info_t.pos_gpsno, pep_jnls.gpsid, getstrlen(pep_jnls.gpsid));
			rtn = UpdatePos_info(mer_info_t, pep_jnls.termidm, 2);	
			if(rtn < 0)
			{
				dcs_log(0, 0, "保存基站信息失败");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				SaveErTradeInfo("H9", currentDate, "风控限制", mer_info_t, pep_jnls, pep_jnls.termtrc);
				BackToPos("H9", siso, pep_jnls, databuf, mer_info_t);
				return;
			}
		}
	}
	/*
		pos机转账汇款、信用卡还款
		兑兑啦积分消费、积分查询、二维码验证、短信验证、条形码验证
		以上交易不参与批结算
	*/
	if(memcmp(pep_jnls.trancde, "013", 3)!=0 && memcmp(pep_jnls.trancde, "017", 3)!=0 
	&& memcmp(pep_jnls.trancde, "D01", 3)!=0 && memcmp(pep_jnls.trancde, "D02", 3)!=0
	&& memcmp(pep_jnls.trancde, "D03", 3)!=0 && memcmp(pep_jnls.trancde, "D04", 3)!=0)
	{
		memset(tmpbuf, 0, sizeof(tmpbuf));
		s = getbit(&siso, 60, (unsigned char *)tmpbuf);
		if( s <= 0)
		{
			dcs_log(0, 0, "报文60域数据信息错误");
			return ;
		}
		memset(signtrancode, 0, sizeof(signtrancode));
		memcpy(signtrancode, tmpbuf, 2);
		memset(tran_batch_no, 0, sizeof(tran_batch_no));
		memcpy(tran_batch_no, tmpbuf+2, 6);
		memset(signflg, 0, sizeof(signflg));
		memcpy(signflg, tmpbuf+8, 3);
		memcpy(pep_jnls.netmanage_code, signflg, 3);
#ifdef __LOG__
		dcs_log(0, 0, "POS上送的批次号:[%s]", tran_batch_no);
		dcs_log(0, 0, "当前数据表中的批次号:[%s]", mer_info_t.mgr_batch_no);
#endif

		if(memcmp(tran_batch_no, mer_info_t.mgr_batch_no, 6) != 0&& numcase != 800)
		{
			dcs_log(0, 0, "POS上送的批次号和当前数据表中的批次号不匹配");
			if(numcase == 500)
			{
				memcpy(rtncode, "T4", 2);
				SaveErTradeInfo(rtncode, currentDate, "批次错误,请联系客服", mer_info_t, pep_jnls, pep_jnls.termtrc);
			}
			else
			{
				memcpy(rtncode, "77", 2);
				SaveErTradeInfo(rtncode, currentDate, "批次号不匹配请重新签到", mer_info_t, pep_jnls, pep_jnls.termtrc);
			}
			BackToPos(rtncode, siso, pep_jnls, databuf, mer_info_t);
			return;
		}
	}
	//校验mac
	if((numcase != 500 && numcase != 320 && numcase!=820 && numcase!=800) ||(numcase==820&&memcmp(pep_jnls.trancde, "009", 3)==0)
	||(numcase==800&&(memcmp(pep_jnls.trancde, "800", 3)==0||memcmp(pep_jnls.trancde, "801", 3)==0)))
	{
		memset(macblock, 0, sizeof(macblock));
		memset(posmac, 0, sizeof(posmac));
		
//		dcs_debug( rtndata, iLen-8, "macblock:");
		s = GetPospMAC(rtndata, iLen-8, mer_info_t.mgr_mak, posmac);
		
		memset(tmpbuf, 0, sizeof(tmpbuf));
		s = getbit(&siso, 64, (unsigned char *)tmpbuf);
		if(s <= 0)
		{
			dcs_log(0, 0, "bit_64 error", posmac);
			BackToPos("H9", siso, pep_jnls, databuf, mer_info_t);
			return;
		}
#ifdef __LOG__
		dcs_log(0, 0, "平台计算得到的Mac = [%s]", posmac);
		dcs_log(0, 0, "终端上送的Mac = [%s]", tmpbuf);
#endif
		if(memcmp(posmac, tmpbuf, 8) != 0)
		{
			dcs_log(0, 0, "mac校验失败");
			SaveErTradeInfo("A0", currentDate, "mac校验失败", mer_info_t, pep_jnls, pep_jnls.termtrc);
			BackToPos("A0", siso, pep_jnls, databuf, mer_info_t);
			return;
		}
	}
	memcpy(iso8583_conf,iso8583conf,sizeof(iso8583conf));
	switch(numcase)
	{
		case 320: //批上送
			setbit(&siso, 0, (unsigned char *)"0330", 4);
			setbit(&siso, 12, (unsigned char *)databuf+4, 6);
			setbit(&siso, 13, (unsigned char *)databuf, 4);
			setbit(&siso, 32, (unsigned char *)"40002900", 8);
			
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy(tmpbuf, pep_jnls.termtrc, 6);
			memcpy(tmpbuf+6, currentDate+8, 6);
			
			setbit(&siso, 37, (unsigned char *)tmpbuf, 12);
			setbit(&siso, 39, (unsigned char *)"00", 2);	//?
			if(memcmp(signflg, "204", 3) == 0 || memcmp(signflg, "206", 3) == 0)//AAC和ARPC暂不保存,直接返回
			{
				dcs_log(0, 0, "AAC和ARPC暂不保存,直接返回");
				s = sndBackToPos(siso, iFid, mer_info_t);
				if(s < 0 )
				{
					dcs_log(0, 0, "pos back error!");
				}
				break;
			}
			if(memcmp(signflg, "203", 3) == 0 || memcmp(signflg, "205", 3) == 0)
			{
				dcs_log(0, 0, "解析55域TC值信息9F26");
				
				s = getbit(&siso, 55, (unsigned char *)pep_jnls.filed55);
				if(s <= 0)
				{
				 	dcs_log(0, 0, "bit55 is null!");	
				}
				else
				{
//					memset(tmpbuf, 0, sizeof(tmpbuf));
//					bcd_to_asc((unsigned char *)tmpbuf, (unsigned char *)pep_jnls.filed55, 2*s, 0);
					
				#ifdef __LOG__
					dcs_debug(pep_jnls.filed55, s, "解析TC值得55域内容:");
				#endif
					rtn = analyse55(pep_jnls.filed55, s, stFd55_IC, 34);
					if(rtn < 0 )
					{
						dcs_log(0, 0, "解析55域失败,解析结果[%d]", rtn);
						memcpy(pep_jnls.aprespn, "ER", 2);	
					}
					else 
					{
						memcpy(pep_jnls.tc_value, stFd55_IC[0].szValue, 16);
						dcs_log(0, 0, "TC值:[%s]", pep_jnls.tc_value);
						memcpy(pep_jnls.aprespn, "00", 2);
					}
				}
			#ifdef __LOG__
				dcs_log(0, 0, "保存TC值上送交易信息到manage_info中");
			#endif
				s = saveTCvalue(siso, currentDate, &pep_jnls);
				if(s < 0)
				{
					dcs_log(0, 0, "保存TC值上送交易信息失败");
					SaveErTradeInfo("FZ", currentDate, "保存TC值上送交易信息失败", mer_info_t, pep_jnls, trace);
					BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
					break;
				}
				
				setbit(&siso, 21, (unsigned char *)NULL, 0);
				s = sndBackToPos(siso, iFid, mer_info_t);
				if(s < 0 )
				{
					dcs_log(0, 0, "pos back error!");
				}
				else
				{
					dcs_log(0, 0, "pos back success!");
				}
				rtn = ProcessTcvalue(siso, pep_jnls, mer_info_t, currentDate);
				if(rtn == -1)
				{
					dcs_log(0, 0, "IC卡TC值上送核心处理error");
					break;
				}
				else
					dcs_log(0, 0, "IC卡TC值上送核心处理success");
				
				break;
			}
			/*保存批上送信息*/
			s  = savebatchsend_jnls(siso, currentDate, pep_jnls.termidm);
			if(s < 0)
			{
				dcs_log(0, 0, "保存批上送信息失败");
				SaveErTradeInfo("FZ", currentDate, "保存批上送信息失败", mer_info_t, pep_jnls, trace);
				BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
				memcpy(pep_jnls.aprespn, "ER", 2);
				rtn = Pos_SavaManageInfo(siso, pep_jnls, currentDate);
				if(rtn == -1)
				{
					dcs_log(0, 0, "保存管理类信息，POS批上送处理失败");
				}
				break;
			}
			
			sscanf(tran_batch_no, "%06ld", &batchno);
			sscanf(mer_info_t.mgr_batch_no, "%06ld", &pos_batchno);
		
			//对账不平时批上送结束时更新一下数据表pos_conf_info中的批次号加1
			if(memcmp(signflg, "202", 3) == 0)
			{	
				if((pos_batchno - batchno) == 0 )
				{	
					s = updatepos_conf_info(siso, mer_info_t, pep_jnls.termidm);
					if(s < 0)
					{
						dcs_log(0, 0, "更新数据表pos_conf_info失败");
						SaveErTradeInfo("FZ", currentDate, "更新数据表pos_conf_info失败", mer_info_t, pep_jnls, trace);
						BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
						break;
					}
				}
				else if((pos_batchno - batchno) >1 )
				{
					dcs_log(0, 0, "批上送结束时60域error!");
					BackToPos("H9", siso, pep_jnls, databuf, mer_info_t);
					break;
				}
			}
			
			//批结算对账平的时候更新下数据表pos_conf_info批次号+1
			if(memcmp(signflg, "207", 3) == 0)
			{
				if((pos_batchno - batchno) == 0 )
				{
					s = updatepos_conf_info(siso, mer_info_t, pep_jnls.termidm);
					if(s < 0)
					{
						dcs_log(0, 0, "更新数据表pos_conf_info失败");
						SaveErTradeInfo("FZ", currentDate, "更新数据表pos_conf_info失败", mer_info_t, pep_jnls, trace);
						BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
						break;
					}
				}
				else if((pos_batchno - batchno) >1 )
				{
					dcs_log(0, 0, "批上送结束时60域error!");
					BackToPos("H9", siso, pep_jnls, databuf, mer_info_t);
					break;
				}
			}
			setbit(&siso, 21, (unsigned char *)NULL, 0);
			s = sndBackToPos(siso, iFid, mer_info_t);
			if(s < 0 )
			{
				dcs_log(0, 0, "pos back error!");
				memcpy(pep_jnls.aprespn, "ER", 2);
			}
			else
			{
				dcs_log(0, 0, "pos back success");
				memcpy(pep_jnls.aprespn, "00", 2);
			}
			rtn = Pos_SavaManageInfo(siso, pep_jnls, currentDate);
			if(rtn == -1)
			{
				dcs_log(0, 0, "保存管理类信息，POS批上送结束失败");
			}
			break;
		case 500:  //批结算
			setbit(&siso, 0, (unsigned char *)"0510", 4);
			setbit(&siso, 12, (unsigned char *)currentDate+8, 6);
			setbit(&siso, 13, (unsigned char *)currentDate+4, 4);
			setbit(&siso, 15, (unsigned char *)currentDate+4, 4);
			setbit(&siso, 32, (unsigned char *)"40002900", 8);
			
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy(tmpbuf, pep_jnls.termtrc, 6);
			memcpy(tmpbuf+6, currentDate+8, 6);
			
			setbit(&siso, 37, (unsigned char *)tmpbuf, 12);
			setbit(&siso, 39, (unsigned char *)"00", 2);	
			
			memset(tmpbuf, 0, sizeof(tmpbuf));
			s = getbit(&siso, 48, (unsigned char *)tmpbuf);
			if(s <= 0)
			{
				dcs_log(0, 0, "get-bit-48 error");
				memcpy(pep_jnls.aprespn, "ER", 2);
				rtn = Pos_SavaManageInfo(siso, pep_jnls, currentDate);
				if(rtn == -1)
				{
					dcs_log(0, 0, "保存管理类信息，批结算信息失败");
				}
				break;
			}
		#ifdef __LOG__
			dcs_debug(tmpbuf, s, "pos bit 48 tmpbuf :");
		#endif
			memset(pos_jieji_amount, 0, sizeof(pos_jieji_amount));
			memset(pos_jieji_count, 0, sizeof(pos_jieji_count));
			memset(pos_daiji_amount, 0, sizeof(pos_daiji_amount));
			memset(pos_daiji_count, 0, sizeof(pos_daiji_count));
			
			memcpy(pos_jieji_amount, tmpbuf, 12);
			memcpy(pos_jieji_count, tmpbuf+12, 3);
			memcpy(pos_daiji_amount, tmpbuf+15, 12);
			memcpy(pos_daiji_count, tmpbuf+27, 3);
			
			dcs_log(0, 0, "借记总金额:%s", pos_jieji_amount);
			dcs_log(0, 0, "借记总笔数:%s", pos_jieji_count);
			dcs_log(0, 0, "贷记总金额:%s", pos_daiji_amount);
			dcs_log(0, 0, "贷记总笔数:%s", pos_daiji_count);
			
			sscanf(pos_jieji_count, "%d", &i_pos_jieji_count);
			sscanf(pos_daiji_count, "%d", &i_pos_daiji_count);
			
			//与pep_jnls中的借记贷记交易总金额，总笔数做对比
			rtn  = getjnlsdata(siso, &xpep_cd_info, pep_jnls.termidm);
			if(rtn < 0)
			{
				dcs_log(0, 0, "查询数据表pep_jnls信息失败!");
				memcpy(tmpbuf+30, "3", 1);
				memcpy(tmpbuf+s-1, "1", 1);
			}
			
			if( memcmp(pos_jieji_amount, xpep_cd_info.xpep_jieji_amount, 12)==0 && 
				memcmp(pos_daiji_amount, xpep_cd_info.xpep_daiji_amount, 12) ==0 &&
				xpep_cd_info.xpep_jieji_count == i_pos_jieji_count && 
				xpep_cd_info.xpep_daiji_count==i_pos_daiji_count)
			{
				memcpy(tmpbuf+30, "1", 1);
				memcpy(tmpbuf+s-1, "1", 1);
			}
			else
			{
				memcpy(tmpbuf+30, "2", 1);
				memcpy(tmpbuf+s-1, "1", 1);
			}
			setbit(&siso, 48,(unsigned char *)tmpbuf, s);	
			rtn = saveSettlemtntJnls(siso, currentDate, pos_jieji_amount, i_pos_jieji_count, pos_daiji_amount, i_pos_daiji_count, xpep_cd_info, pep_jnls.termidm);
			if(rtn < 0)
			{
				dcs_log(0, 0, "保存批结算数据失败!");
				memcpy(tmpbuf+30, "3", 1);
				memcpy(tmpbuf+s-1, "1", 1);
				setbit(&siso, 48,(unsigned char *)tmpbuf, s);	
			}
			setbit(&siso, 21,(unsigned char *)NULL, 0);							
			s = sndBackToPos(siso, iFid, mer_info_t);
			if(s < 0 )
			{
				dcs_log(0, 0, "sndBackToPos error!");
				memcpy(pep_jnls.aprespn, "ER", 2);
			}
			else
			{
				memcpy(pep_jnls.aprespn, "00", 2);
				dcs_log(0, 0, "sndBackToPos success!");
			}
			rtn = Pos_SavaManageInfo(siso, pep_jnls, currentDate);
			if(rtn == -1)
			{
				dcs_log(0, 0, "保存管理类信息，批结算信息失败");
			}
			break;
		case 820:
			if(memcmp(pep_jnls.trancde, "004", 3) == 0)
			{
				dcs_log(0, 0, "POS 签退.");
				setbit(&siso, 0, (unsigned char *)"0830", 4);
				setbit(&siso, 12, (unsigned char *)databuf+4, 6);
				setbit(&siso, 13, (unsigned char *)databuf, 4);
					
				memset(tmpbuf, 0, sizeof(tmpbuf));
				memcpy(tmpbuf, pep_jnls.termtrc, 6);
				memcpy(tmpbuf+6, databuf+4,6);
				setbit(&siso, 37, (unsigned char *)tmpbuf, 12);
				
				setbit(&siso, 32, (unsigned char *)"40002900", 8);
				setbit(&siso, 39, (unsigned char *)"00", 2);
				setbit(&siso, 21, (unsigned char *)NULL, 0);	
				s = sndBackToPos(siso, iFid, mer_info_t);
				if(s < 0 )
				{
					memcpy(pep_jnls.aprespn, "ER", 2);
					dcs_log(0, 0, "sndBackToPos error!");
				}
				else
				{
					memcpy(pep_jnls.aprespn, "00", 2);
					dcs_log(0, 0, "sndBackToPos sucess!");
				}
				rtn = Pos_SavaManageInfo(siso, pep_jnls, currentDate);
				if(rtn == -1)
				{
					dcs_log(0, 0, "保存管理类信息，POS签退失败");
				}
			}
			else if(memcmp(pep_jnls.trancde, "009", 3)==0)
			{
				dcs_log(0, 0, "POS 电子签名");
				setbit(&siso, 0, (unsigned char *)"0830", 4);
				s = getConfirmated(siso, mer_info_t, &pep_jnls, elecsign, bit_55, &bit_55_len);
				if(s < 0)
				{
					dcs_log(0, 0, "getConfirmated查询原交易失败");
					BackToPos("F0", siso, pep_jnls, databuf, mer_info_t);
					break;
				}
				#ifdef __TEST__
					dcs_debug(bit_55, bit_55_len, "bit_55 : s=[%d]", bit_55_len);
				#endif
				rtn = saveElecSign(pep_jnls, elecsign, s, bit_55, bit_55_len);
				if(rtn < 0)
				{
					dcs_log(0, 0, "saveElecSign保存签名信息失败");
					BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
					break;
				}
				setbit(&siso, 39, (unsigned char *)"00", 2);
				setbit(&siso, 55, (unsigned char *)NULL, 0);
				setbit(&siso, 62, (unsigned char *)NULL, 0);
				
				GetPosMacValue(siso,  mer_info_t);
				s = sndBackToPos(siso, iFid, mer_info_t);
				if(s < 0 )
				{
					dcs_log(0, 0, "sndBackToPos error!");
				}
			}
			break;
		case 800:
			if(memcmp(pep_jnls.trancde, "800", 3)==0)
			{
				dcs_log(0, 0, "交易回执");
				rtn = ConfirmAndsave(siso, &pep_jnls, currentDate, mer_info_t.T0_flag, mer_info_t.agents_code);
				if(rtn < 0)
				{
					dcs_log(0, 0, "交易回执处理失败");
					BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
					break;
				}
				else
				{
					dcs_log(0, 0, "交易回执处理成功");
					setbit(&siso, 0, (unsigned char *)"0810", 4);
					setbit(&siso, 39, (unsigned char *)"00", 2);
					setbit(&siso, 44, (unsigned char *)pep_jnls.filed44, 22);
					setbit(&siso, 21,(unsigned char *)NULL, 0);
					GetPosMacValue(siso, mer_info_t);
					s = sndBackToPos(siso, iFid, mer_info_t);
					if(s < 0 )
					{
						dcs_log(0, 0, "sndBackToPos error!");
					}
					break;	
				}
			}
			else if(memcmp(pep_jnls.trancde, "801", 3)==0)
			{
				dcs_log(0, 0, "末笔消费交易状态查询");
				rtn = GetJnlsStatus(siso, &pep_jnls, currentDate);
				if(rtn < 0)
				{
					dcs_log(0, 0, "末笔消费交易状态查询处理失败");
					BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
					break;
				}
				else
				{
					dcs_log(0, 0, "末笔消费交易状态查询处理成功");
					setbit(&siso, 0, (unsigned char *)"0810", 4);
					setbit(&siso, 12, (unsigned char *)pep_jnls.peptime, 6);
					setbit(&siso, 13, (unsigned char *)pep_jnls.pepdate+4, 4);
					setbit(&siso, 37, (unsigned char *)pep_jnls.syseqno, 12);
					setbit(&siso, 39, (unsigned char *)pep_jnls.aprespn, 2);
					setbit(&siso, 44, (unsigned char *)pep_jnls.filed44, 22);
					setbit(&siso, 21,(unsigned char *)NULL, 0);
					setbit(&siso, 22,(unsigned char *)NULL, 0);
					setbit(&siso, 61,(unsigned char *)NULL, 0);
					if(memcmp(pep_jnls.aprespn, "00", 2)== 0 )
						GetPosMacValue(siso, mer_info_t);
					s = sndBackToPos(siso, iFid, mer_info_t);
					if(s < 0 )
					{
						dcs_log(0, 0, "sndBackToPos error!");
					}
					break;	
				}
				break;
			}
			else if(memcmp(pep_jnls.trancde, "003", 3)==0)
				dcs_log(0, 0, "签到");
#ifdef __LOG__
			dcs_log(0, 0, "交易类型码:%s",signtrancode);
			dcs_log(0, 0, "批次号:%s",tran_batch_no);
			dcs_log(0, 0, "密钥长度:%s",signflg);
#endif
			if(memcmp(signflg, mer_info_t.mgr_des, 3)!=0)
			{
				dcs_log(0, 0, "密钥算法和后台的配置不一致");
				BackToPos("HB", siso, pep_jnls, databuf, mer_info_t);
				memcpy(pep_jnls.aprespn, "HB", 2);
				rtn = Pos_SavaManageInfo(siso, pep_jnls, currentDate);
				if(rtn == -1)
				{
					dcs_log(0, 0, "保存管理类信息，签到失败");
				}
				break;
			}
			//终端签到
			if(memcmp(signtrancode, "00", 2) == 0 && ( memcmp(signflg, "001", 3) == 0 || memcmp(signflg, "003", 3) == 0 || memcmp(signflg, "004", 3) == 0) )
			{
				setbit(&siso, 0, (unsigned char *)"0810", 4);
				setbit(&siso, 12, (unsigned char *)databuf+4, 6);
				setbit(&siso, 13, (unsigned char *)databuf, 4);

				memset(tmpbuf, 0, sizeof(tmpbuf));
				memcpy(tmpbuf, pep_jnls.termtrc, 6);
				memcpy(tmpbuf+6, databuf+4,6);
				setbit(&siso, 37, (unsigned char *)tmpbuf, 12);
				
				setbit(&siso, 32, (unsigned char *)"40002900", 8);
				setbit(&siso,39,(unsigned char *)"00", 2);
				memset(mer_info_t.mgr_signtime, 0, sizeof(mer_info_t.mgr_signtime));
				memcpy(mer_info_t.mgr_signtime, currentDate, 14);
				    
				//单倍长
				if(memcmp(signflg, "001", 3) == 0)
				{
					memset(tmpbuf, 0, sizeof(tmpbuf));
					memset(tmp_workey, 0, sizeof(tmp_workey));
					s = PosGenKey(&mer_info_t, &mer_signin_info);
					if(s == 1){
						memcpy(tmpbuf, mer_signin_info.mgr_pik, 16);
						memcpy(tmpbuf+16, mer_signin_info.mgr_pik_checkvalue, 8);
						memcpy(tmpbuf+16+8, mer_signin_info.mgr_mak, 16);
						memcpy(tmpbuf+16+8+16, mer_signin_info.mgr_mak_checkvalue, 8);
						asc_to_bcd( (unsigned char *)tmp_workey, (unsigned char*)tmpbuf, 48, 0 );
						setbit(&siso, 62,(unsigned char *)tmp_workey, 24);
						memset(tmpbuf, 0, sizeof(tmpbuf));
						sprintf(tmpbuf, "%s%s%s", signtrancode, mer_info_t.mgr_batch_no, signflg);
						setbit(&siso, 60,(unsigned char *)tmpbuf, 11);
						
					}
					else
					{
						dcs_log(0, 0, "签到失败!");
						setbit(&siso, 39, (unsigned char *)"HB", 2);
						memcpy(pep_jnls.aprespn, "HB", 2);
					}
				}
				else if(memcmp(signflg, "003", 3) == 0) //双倍长
				{
					dcs_log(0, 0, "双倍长");
					memset(tmpbuf, 0, sizeof(tmpbuf));
					memset(tmp_workey, 0, sizeof(tmp_workey));
					s = PosGenKey(&mer_info_t, &mer_signin_info);
					if(s == 1)
					{
						memcpy(tmpbuf, mer_signin_info.mgr_pik, 32);
						memcpy(tmpbuf+32, mer_signin_info.mgr_pik_checkvalue, 8);
						memcpy(tmpbuf+32+8, mer_signin_info.mgr_mak, 32);
						memcpy(tmpbuf+32+8+32, mer_signin_info.mgr_mak_checkvalue, 8);
						asc_to_bcd( (unsigned char *)tmp_workey, (unsigned char*)tmpbuf, 80, 0 );
						#ifdef __LOG__
							dcs_debug(tmp_workey, 40, "tmp_workey len=%d",40);
						#endif
						setbit(&siso, 62,(unsigned char *)tmp_workey, 40);
						
						memset(tmpbuf, 0, sizeof(tmpbuf));
						sprintf(tmpbuf, "%s%s%s", signtrancode, mer_info_t.mgr_batch_no, signflg);
						setbit(&siso, 60,(unsigned char *)tmpbuf, 11);
					}
					else
					{
						dcs_log(0, 0, "s=%d",s);
						dcs_log(0, 0, "签到失败!");
						setbit(&siso, 39,(unsigned char *)"HB", 2);
						memcpy(pep_jnls.aprespn, "HB", 2);
					}
				}else //磁道双倍长
				{
					memset(tmpbuf, 0, sizeof(tmpbuf));
					memset(tmp_workey, 0, sizeof(tmp_workey));
					s = PosGenKey(&mer_info_t, &mer_signin_info);
					if(s == 1){
						memcpy(tmpbuf, mer_signin_info.mgr_pik, 32);
						memcpy(tmpbuf+32, mer_signin_info.mgr_pik_checkvalue, 8);
						memcpy(tmpbuf+32+8, mer_signin_info.mgr_mak, 32);
						memcpy(tmpbuf+32+8+32, mer_signin_info.mgr_mak_checkvalue, 8);
						memcpy(tmpbuf+32+8+32+8, mer_signin_info.mgr_tdk, 32);
						memcpy(tmpbuf+32+8+32+8+32, mer_signin_info.mgr_tdk_checkvalue, 8);
						asc_to_bcd((unsigned char *)tmp_workey, (unsigned char*)tmpbuf, 120, 0 );
						setbit(&siso, 62,(unsigned char *)tmp_workey, 60);
						
						memset(tmpbuf, 0, sizeof(tmpbuf));
						sprintf(tmpbuf, "%s%s%s", signtrancode, mer_info_t.mgr_batch_no, signflg);
						setbit(&siso, 60,(unsigned char *)tmpbuf, 11);
					}
					else
					{
						dcs_log(0, 0,"签到失败!");
						setbit(&siso, 39,(unsigned char *)"HB", 2);
						memcpy(pep_jnls.aprespn, "HB", 2);
					}
				}
				setbit(&siso, 21,(unsigned char *)NULL, 0);
				setbit(&siso, 63,(unsigned char *)NULL, 0);
				s = sndBackToPos(siso, iFid, mer_info_t);
				if(s < 0 )
				{
					dcs_log(0,0,"sndBackToPos error!");
					memcpy(pep_jnls.aprespn, "ER", 2);
				}
				else
				{
					memcpy(pep_jnls.aprespn, "00", 2);
					dcs_log(0,0,"sndBackToPos sucess!");
				}
				rtn = Pos_SavaManageInfo(siso, pep_jnls, currentDate);
				if(rtn == -1)
				{
					dcs_log(0, 0, "保存管理类信息，POS签到失败");
				}
				break;
			}
		break;
		case 200:		
			memset(tmpbuf, 0, sizeof(tmpbuf));
			getbit(&siso, 3, (unsigned char *)tmpbuf);
			
			//消费交易 or 预授权完成请求 离线消费交易上送
			if( memcmp(tmpbuf, "000000", 6) == 0 )
			{	
				dcs_log(0, 0, "消费交易的处理");
				rtn = ProcessTrade(siso, pep_jnls, mer_info_t, currentDate);	
				dcs_log(0, 0, "消费交易的处理结果[%d]", rtn);
				if(rtn == -1)
				{
					dcs_log(0, 0, "交易的处理error");
					break;
				}
				else
					dcs_log(0, 0, "交易的处理success");
				break;
			}
			//余额查询类的交易
			else if( memcmp(tmpbuf, "310000", 6) == 0 )
			{
				dcs_log(0,0,"余额查询交易处理");
				rtn = ProcessTrade(siso, pep_jnls, mer_info_t, currentDate);
				if(rtn == -1)
				{
					dcs_log(0, 0, "余额查询交易处理error");
					break;
				}
				else
					dcs_log(0, 0, "余额查询交易处理success");
				break;
			}
			//消费撤销 or 预授权完成撤销
			else if(memcmp(tmpbuf, "200000", 6) == 0)
			{
				dcs_log(0,0,"消费撤销交易处理");
				rtn = ProcessTrade(siso, pep_jnls, mer_info_t, currentDate);
				if(rtn == -1)
				{
					dcs_log(0, 0, "消费撤销交易处理error");
					break;
				}
				else
					dcs_log(0, 0, "消费撤销交易处理success");
				break;
			}
			//用户认证
			else if( memcmp(tmpbuf, "330000", 6) == 0 )
			{
				dcs_log(0, 0, "用户认证的处理");
				rtn = UserAuthentication(siso, pep_jnls, mer_info_t, currentDate);
				if(rtn == -1)
				{
					dcs_log(0, 0, "用户认证的处理error");
					break;
				}
				else
					dcs_log(0, 0, "用户认证的处理success");
				break;
			}
			//信用卡还款
			else if( memcmp(tmpbuf, "190000", 6) == 0 )
			{
				rtn = ProTransfer(siso, pep_jnls, mer_info_t, currentDate);
				if(rtn == -1)
				{
					dcs_log(0, 0, "信用卡还款交易处理error");
					break;
				}
				else
					dcs_log(0,0,"信用卡还款交易处理success");
				break;
			}
			
			//ATM跨行转账交易
			/*
			else if( memcmp(tmpbuf, "400000", 6) == 0 )
			{
				rtn = ProTransfer(siso, pep_jnls, mer_info_t, currentDate);
				if(rtn == -1)
				{
					dcs_log(0, 0, "ATM跨行转账交易处理error");
					break;
				}
				else
					dcs_log(0, 0, "ATM跨行转账交易处理success");
				break;
			}
			*/
			//转账汇款交易
			else if( memcmp(tmpbuf, "480000", 6) == 0 )
			{
				rtn = ProTransfer(siso, pep_jnls, mer_info_t, currentDate);
				if(rtn == -1)
				{
					dcs_log(0, 0, "转账汇款交易处理error");
					break;
				}
				else
					dcs_log(0, 0, "转账汇款交易处理success");
				break;
			}
		break;
		//退货
		case 220:
			memset(tmpbuf, 0, sizeof(tmpbuf));
			s = getbit(&siso, 3, (unsigned char *)tmpbuf);
			if(s <= 0)
			{
				dcs_log(0, 0, "get bit_3 error!");	
				BackToPos("H9", siso, pep_jnls, databuf, mer_info_t);
				break;
			}
			//退货类的交易交易处理码是200000
			if( memcmp(tmpbuf, "200000", 6) == 0 )
			{		
				dcs_log(0, 0, "消费退货的交易处理");
				rtn = ProcessTrade(siso, pep_jnls, mer_info_t, currentDate);
				if(rtn == -1)
				{
					dcs_log(0, 0, "消费退货交易处理error");
					break;
				}
				else
					dcs_log(0, 0, "消费退货交易处理success");
				break;
			}
			break;
		case 100:
			memset(tmpbuf, 0, sizeof(tmpbuf));
			s = getbit(&siso, 3, (unsigned char *)tmpbuf);
			if(s <= 0)
			{
				dcs_log(0, 0, "get bit_3 error!");
				BackToPos("H9", siso, pep_jnls, databuf, mer_info_t);	
				break;
			}
			//预授权交易
			if(memcmp(tmpbuf, "030000", 6) == 0)
			{
				dcs_log(0, 0, "预授权交易处理");
				rtn = ProcessTrade(siso, pep_jnls, mer_info_t, currentDate);
				if(rtn == -1)
				{
					dcs_log(0, 0, "预授权交易处理error");
					break;
				}
				else
					dcs_log(0, 0, "预授权交易处理success");
				break;
			}
			//预授权撤销交易
			else if(memcmp(tmpbuf, "200000", 6) == 0)
			{
				dcs_log(0, 0, "预授权撤销交易处理");
				rtn = ProcessTrade(siso, pep_jnls, mer_info_t, currentDate);
				if(rtn == -1)
				{
					dcs_log(0, 0, "预授权撤销交易处理error");
					break;
				}
				else
					dcs_log(0, 0, "预授权撤销交易处理success");
				break;
			}
			break;
		case 400:		
			memset(tmpbuf, 0, sizeof(tmpbuf));
			s = getbit(&siso, 3, (unsigned char *)tmpbuf);
			if(s <= 0)
			{
				dcs_log(0, 0, "get bit_3 error!");
				BackToPos("H9", siso, pep_jnls, databuf, mer_info_t);	
				break;
			}
			//消费冲正交易 or 预授权完成冲正
			if( memcmp(tmpbuf, "000000", 6) == 0 )
			{
				dcs_log(0, 0, "消费冲正");
				rtn = ProcessRevoid(siso, pep_jnls, currentDate, mer_info_t);
				if(rtn == -1)
				{
					dcs_log(0, 0, "处理消费冲正失败");
					break;
				}
				else
				{
					dcs_log(0, 0, "处理消费冲正成功");
					break;
				}
				break;
			}
			//消费撤销冲正 or 预授权撤销冲正 or 预授权完成撤销冲正
			else if(memcmp(tmpbuf, "200000", 6) == 0)
			{		
				dcs_log(0, 0, "撤销冲正");
				rtn = ProcessRevoid(siso, pep_jnls, currentDate, mer_info_t);
				if(rtn == -1)
				{
					dcs_log(0, 0, "处理撤销冲正失败");
					break;
				}
				else
				{
					dcs_log(0, 0, "处理撤销冲正成功");
					break;
				}
				break;
			}
			else if(memcmp(tmpbuf, "030000", 6) == 0)
			{
				dcs_log(0, 0, "预授权冲正交易处理");
				rtn = ProcessRevoid(siso, pep_jnls, currentDate, mer_info_t);
				if(rtn == -1)
				{
					dcs_log(0, 0, "预授权冲正交易处理失败");
					break;
				}
				else
				{
					dcs_log(0, 0, "预授权冲正交易处理成功");
					break;
				}
				break;
			}
		break;
		case 620:
            dcs_log(0, 0, "脚本通知的交易处理");
            rtn = ProcessTrade(siso, pep_jnls, mer_info_t, currentDate);
            if(rtn == -1)
            {
                dcs_log(0, 0, "脚本通知交易处理error");
                break;
            }
            else
                dcs_log(0, 0, "脚本通知交易处理success");
        break;
	}  
	return;
}

int trimTelNo(char *telNo)
{
	int t=0;
	if(telNo == NULL)
		return 0;
		
	for(t=0;t<16;t++)
	{
		if(*(telNo+t) == 0x30 && *(telNo+t+1) != 0x30 && t<8)
		{
			if(t == 7)
				t = t+1;
			break;
		}
	}
	return 16-t;
}

int posUnpack(char *srcBuf, ISO_data *iso, MER_INFO_T *mer_info_t, int iLen, char *resdata)
{
	char tmp1[20], tmp2[20], rtnData[1024];
	int tt = 0, s;
	//实际应用数据长度
	int actual_len = 0;
	
	MER_INFO_T nmer_info_t;

	if(iLen < 50)
	{
		dcs_log(0, 0, "posUnpack error");
		return -1;
	}

	memset(tmp1, 0, sizeof(tmp1));
	memset(tmp2, 0, sizeof(tmp2));
	memset(rtnData, 0, sizeof(rtnData));
	memset(&nmer_info_t, 0, sizeof(MER_INFO_T));
	
	//去掉TPDU(bcd 5)
	memcpy(tmp1, srcBuf, 5);
	bcd_to_asc((unsigned char *)tmp2, (unsigned char *)tmp1, 10, 0);
	dcs_log(0, 0, "TPDU：%s", tmp2);
	memcpy(mer_info_t->mgr_tpdu, tmp2, 10);
	
	iLen -= 5;	
	srcBuf += 5;
	if((0x4C == *srcBuf)&& (0x52 == *(srcBuf+1))&& (0x49 == *(srcBuf+2)))
	{
		iLen -= 3;
		srcBuf += 3;
		if ( (0x00 == *srcBuf)&& (0x1C == *(srcBuf+1)))
		{
			iLen -= 2;
			srcBuf += 2;
			memset(tmp1, 0, sizeof(tmp1));
			memcpy(tmp1, srcBuf, 8);
			memset(tmp2, 0, sizeof(tmp2));
			bcd_to_asc((unsigned char *)tmp2, (unsigned char *)tmp1, 16, 0);
			dcs_log(0, 0, "主叫号码：%s", tmp2);
			tt = trimTelNo(tmp2);
			if(tt == 0)
			{
				dcs_log(0, 0, "主叫号码错误");
				memcpy(mer_info_t->pos_telno, tmp2, 16);
			}
			else
			{
				memcpy(mer_info_t->pos_telno, tmp2+16-tt, tt);
				dcs_log(0, 0, "pos_telno=%s", mer_info_t->pos_telno);
			}
			
			iLen -= 28;
			srcBuf += 28;
		}
	}
	
	memset(tmp1, 0, sizeof(tmp1));
	memset(tmp2, 0, sizeof(tmp2));
	memcpy(tmp1, srcBuf, 6);
	bcd_to_asc((unsigned char *)tmp2, (unsigned char *)tmp1, 12, 0);
	memcpy(mer_info_t->cur_version_info, tmp2+8, 4);
	iLen -= 6;
	srcBuf += 6;
	
	//begin--------------
	//加密方式
	memset(tmp1, 0, sizeof(tmp1));
	memcpy(tmp1, srcBuf, 1);
	sscanf(tmp1, "%d", &mer_info_t->encrpy_flag);

	//商户号
	memcpy(mer_info_t->mgr_mer_id, srcBuf+1, 15);
	
	//终端号
	memcpy(mer_info_t->mgr_term_id, srcBuf+16, 8);
	
	//明文长度
	memset(tmp1, 0, sizeof(tmp1));
	memcpy(tmp1, srcBuf+24, 4);
	sscanf(tmp1, "%d", &actual_len);
#ifdef __LOG__
	dcs_log(0, 0, "当前的版本信息mer_info_t->cur_version_info=[%s]", mer_info_t->cur_version_info);
	dcs_log(0, 0, "加密方式mer_info_t->encrpy_flag = [%d]", mer_info_t->encrpy_flag);
	dcs_log(0, 0, "商户号mer_info_t->mgr_mer_id = [%s]", mer_info_t->mgr_mer_id);
	dcs_log(0, 0, "终端号mer_info_t->mgr_term_id =[%s]", mer_info_t->mgr_term_id);
	dcs_log(0, 0, "明文长度actual_len =[%d]", actual_len);
#endif
	iLen -= 28;
	srcBuf += 28;
	//报文以密文的方式传输
	if(mer_info_t->encrpy_flag == 1)
	{
		//int GetPosTdk(MER_INFO_T *mer_info_t)
		s =  GetPosTdk(mer_info_t);
		if(s < 0 )
		{
			dcs_log(0, 0, "取磁道主密钥失败");
			return -1;
		}
		
		memcpy(&nmer_info_t, mer_info_t, sizeof(MER_INFO_T));
		s = POSPackageDecrypt(nmer_info_t, srcBuf, iLen, rtnData);
		if(s < 0 )
		{
			dcs_log(0, 0, "解密报文失败");
			return -1;
		}
		iLen = actual_len; 
		memcpy(srcBuf, rtnData, actual_len);
	#ifdef __LOG__
		dcs_log(0, 0, "s= [%d], ilen = [%d]", s, actual_len);
		dcs_debug(srcBuf, actual_len, "解密报文后数据:");
	#endif

	}//lp20121212end-------

	memcpy(iso8583_conf,iso8583posconf,sizeof(iso8583posconf));
	iso8583=&iso8583_conf[0];
	SetIsoHeardFlag(0);
	SetIsoFieldLengthFlag(0);

	if(str_to_iso((unsigned char *)srcBuf, iso, iLen)<0) 
	{
		dcs_log(0, 0, "str_to_iso error ");
		return -1;            
	}
	memcpy(resdata, srcBuf, actual_len);
	return actual_len;
 }
 
int sndBackToPos(ISO_data iso, int fMid, MER_INFO_T mer_info_t)
{
	char buffer[1024], sendBuff[2048], tmp1[11], tmpbuf[37];
	int s, rtn = 0, i = 0, actual_len = 0;
	char encpyData[1024];
	char flag;
	
	memset(buffer, 0, sizeof(buffer));
	memset(sendBuff, 0, sizeof(sendBuff));
	memset(tmp1, 0, sizeof(tmp1));
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(encpyData, 0, sizeof(encpyData));
	memcpy(iso8583_conf,iso8583posconf,sizeof(iso8583posconf));
	iso8583=&iso8583_conf[0];
	SetIsoHeardFlag(0);
	SetIsoFieldLengthFlag(0);
	
	s = iso_to_str((unsigned char *)buffer, &iso, 1);
	if( s < 0 ) 
	{
		dcs_log(0, 0, "iso_to_str error ");
		return -2;
	}
	
	memcpy(tmpbuf, mer_info_t.mgr_tpdu, 2);
	memcpy(tmpbuf+2, mer_info_t.mgr_tpdu+6, 4);
	memcpy(tmpbuf+6, mer_info_t.mgr_tpdu+2, 4);
	
	/*detail*/
	memcpy(tmpbuf+10, "60310", 5);
	if(mer_info_t.pos_update_flag==1 && memcmp(mer_info_t.cur_version_info, mer_info_t.new_version_info, 4)==0)
		UpdatePos_info(mer_info_t, mer_info_t.pos_machine_id, 3);
	
	#ifdef __LOG__
		dcs_log(0, 0, "mer_info_t.para_update_flag  = [%c]", mer_info_t.para_update_flag);
		dcs_log(0, 0, "mer_info_t.para_update_mode  = [%c]", mer_info_t.para_update_mode);
		dcs_log(0, 0, "mer_info_t.pos_update_flag  = [%d]", mer_info_t.pos_update_flag);
		dcs_log(0, 0, "mer_info_t.pos_update_mode  = [%c]", mer_info_t.pos_update_mode);
	#endif
	
	flag = GetDetailFlag(&mer_info_t);
	tmpbuf[15]= flag ;
	
	memcpy(tmpbuf+16, "31", 2);
	if(mer_info_t.pos_update_flag==1 && flag == mer_info_t.pos_update_mode )
		memcpy(tmpbuf+18, mer_info_t.new_version_info, 4);
	else
		memcpy(tmpbuf+18, mer_info_t.cur_version_info, 4);
	asc_to_bcd((unsigned char *)tmp1, (unsigned char*)tmpbuf, 22, 0);
   	memcpy(sendBuff, tmp1, 11);
	
	actual_len = s ;
	//begin-------------
	//加密数据
	if(mer_info_t.encrpy_flag == 1&& memcmp(mer_info_t.pos_machine_id, "LD", 2)!=0 && memcmp(mer_info_t.pos_machine_id, "D1", 2)!=0)
	{
		i = s %8;
		if(i != 0 )
		{
			memset( (void*) (buffer + s), 0x0, 8 );
			s +=(8 - i);
		}
		rtn = POSPackageEncrypt(mer_info_t, buffer, s, encpyData);
		if( rtn < 0 ) 
		{
			dcs_log(0, 0, "加密数据失败 ");
			return -2;
		}
		memcpy(buffer, encpyData, rtn);
	}
		
	//联迪添加解密的功能lp20121218
	else if(mer_info_t.encrpy_flag == 1 && (memcmp(mer_info_t.pos_machine_id, "LD", 2)==0 || memcmp(mer_info_t.pos_machine_id, "D1", 2)==0))
	{
		i = s %8;
		if(i != 0 )
		{
			memset( (void*) (buffer + s), 0x0, 8 );
			s +=(8 - i);
		}
		rtn = POSPackageDecrypt(mer_info_t, buffer, s, encpyData);
		if(rtn < 0)
		{
      		dcs_log(0, 0, "报文解密error");
      		return -2;
   	 	}
   	 	rtn = s ;
   	 	memcpy(buffer, encpyData, rtn);
   	 	dcs_log(0, 0, "解密数据0925_sndBackToPos");
	}

   	//加上对报文头2的处理
    memset(tmpbuf, 0, sizeof(tmpbuf));
    dcs_log(0, 0, "mer_info_t.encrpy_flag = [%d] ", mer_info_t.encrpy_flag);
    sprintf(tmpbuf, "%d%s", mer_info_t.encrpy_flag, mer_info_t.mgr_mer_id);
   	sprintf(tmpbuf+16, "%s%04d", mer_info_t.mgr_term_id, actual_len);
    memcpy(sendBuff+11, tmpbuf, 28);
    if(rtn ==0 )
    	rtn =  s;
    memcpy(sendBuff+39, buffer, rtn);
   	s = rtn+39;//lp20121212end---------------

#ifdef __LOG__
   	dcs_debug(sendBuff, s, "POS数据返回终端, foldId =%d, len =%d", fMid, s);
#else
	dcs_log(0, 0, "POS数据返回终端, foldId =%d, len =%d", fMid, s);
#endif
	s = fold_write(fMid, -1, sendBuff, s);
	if(s < 0)
	{
	      	dcs_log(0, 0, "fold_write() failed:%s", ise_strerror(errno));
	      	return -3;
    	}
	return 0;
 }
 
//int writePos(char *isodata, int len, int fMid, char *tpdu) 
int writePos(char *isodata, int len, int fMid, char *tpdu, MER_INFO_T mer_info_t)//lp20121212
{
	char sendBuff[2048], rtnData[1024], tmp1[37];
	int s, rtn = 0, i = 0, actual_len = 0;
	
	memset(sendBuff, 0, sizeof(sendBuff));
	memset(tmp1, 0, sizeof(tmp1));
	memset(rtnData, 0, sizeof(rtnData));
	
	actual_len = len ; 
	asc_to_bcd((unsigned char *)tmp1, (unsigned char*)tpdu, 22, 0);
    memcpy(sendBuff, tmp1, 11);

	//begin------
	if(mer_info_t.encrpy_flag == 1 && memcmp(mer_info_t.pos_machine_id, "LD", 2)!=0 && memcmp(mer_info_t.pos_machine_id, "D1", 2)!=0)
	{
		i = len %8;
		if(i != 0 )
		{
			memset( (void*) (isodata + len), 0x0, 8 );
			len +=(8 - i);
		}
		rtn = POSPackageEncrypt(mer_info_t, isodata, len, rtnData);
		if(rtn < 0)
		{
      		dcs_log(0, 0, "报文加密error");
      		return -2;
   	 	}
   	 	memcpy(isodata, rtnData, rtn);
	}
		
	//联迪添加解密的功能lp20121218
	else if(mer_info_t.encrpy_flag == 1&& (memcmp(mer_info_t.pos_machine_id, "LD", 2)==0 || memcmp(mer_info_t.pos_machine_id, "D1", 2)==0))
	{
		#ifdef __LOG__
			dcs_log(0, 0, "actual_len = [%d],len = [%d]", actual_len, len);
		#endif
		i = len %8;
		if(i != 0 )
		{
			memset( (void*) (isodata + len), 0x0, 8 );
			len +=(8 - i);
		}
		rtn = POSPackageDecrypt(mer_info_t, isodata, len, rtnData);
		if(rtn < 0)
		{
      		dcs_log(0, 0, "报文解密error");
      		return -2;
   	 	}
   	 	rtn  = len ;
   	 	memcpy(isodata, rtnData, rtn);
   	 	dcs_log(0, 0, "解密数据0925_writePos ");
	}
		
   	memset(tmp1, 0, sizeof(tmp1));
    sprintf(tmp1, "%d%s", mer_info_t.encrpy_flag, mer_info_t.mgr_mer_id);
    sprintf(tmp1+16, "%s%04d", mer_info_t.mgr_term_id, actual_len);
   	memcpy(sendBuff+11, tmp1, 28);
    if(rtn == 0)
    	rtn = len;
   	memcpy(sendBuff+39, isodata, rtn);
    s = rtn+39;//lp20121212end--------

#ifdef __LOG__
    dcs_debug(sendBuff, s, "POS数据返回终端，tpdu=%s, foldId =%d, len =%d", tpdu, fMid, s);
#else
	dcs_log(0, 0, "POS数据返回终端，tpdu=%s, foldId =%d, len =%d", tpdu, fMid, s);
#endif
	s = fold_write(fMid, -1, sendBuff, s);
	
	if(s < 0)
	{
	      	dcs_log(0, 0, "fold_write() failed:%s", ise_strerror(errno));
	      	return -3;
    	}
	
	return 0;
}
 
int GetPospMacBlk(ISO_data *iso, char *macblk)
{
	char buffer[1024];
	int len;
	
	memset(buffer, 0, sizeof(buffer));
	
	//if(IsoLoad8583config(&iso8583_conf[0], "iso8583_pos.conf") < 0)
	{
	//	dcs_log(0, 0, "load iso8583_pos failed:%s", strerror(errno));
		//return -1;
	}
	if(GetIsoMultiThreadFlag() != 0)//多线程
	{
		extern pthread_key_t iso8583_conf_key;//线程私有变量
		extern pthread_key_t iso8583_key;//线程私有变量
		pthread_setspecific(iso8583_conf_key, iso8583posconf);
		pthread_setspecific(iso8583_key, iso8583posconf);
	}
	else
	{
		memcpy(iso8583_conf,iso8583posconf,sizeof(iso8583posconf));
		iso8583 =&iso8583_conf[0];
	}

	SetIsoHeardFlag(0);
	SetIsoFieldLengthFlag(0);
	len = iso_to_str((unsigned char *)buffer, iso, 0);
	
	#ifdef __TEST__
		dcs_debug( buffer, len-8, "POS计算mac数据 len=%d.", len-8 );
	#endif
	
	memcpy( macblk, buffer, len - 8);
	
	return len - 8;
}

/*
函数功能：返回pos机错误的信息应答
*/
int BackToPos(char *retcode, ISO_data siso, PEP_JNLS pep_jnls, char *curdate, MER_INFO_T mer_info_t)
{
	char tmpbuf[127], msgtype[4+1];
	int s, len;
	
	dcs_log(0, 0, "BackToPos");
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(msgtype, 0, sizeof(msgtype));
	s = getbit(&siso, 0, (unsigned char *)msgtype);
	if(s <= 0)
	{
		dcs_log(0, 0, "报文数据信息错误");
		return -1;	
	}
	if(memcmp(msgtype, "0400", 4)!=0)
	{
		if(memcmp(pep_jnls.trancde, "800", 3)==0)
		{
			dcs_log(0, 0, "交易回执错误应答返回");
			setbit(&siso, 0, (unsigned char *)"0810", 4);		
			setbit(&siso, 39, (unsigned char *)retcode, 2);
			setbit(&siso, 44,(unsigned char *)"00000000   00000000   ", 22);
		}
		else if(memcmp(pep_jnls.trancde, "801", 3)==0)
		{
			dcs_log(0, 0, "联机末笔交易信息查询错误应答返回");
			setbit(&siso, 0, (unsigned char *)"0810", 4);		
			setbit(&siso, 39, (unsigned char *)retcode, 2);
			setbit(&siso, 12, (unsigned char *)curdate+4, 6);
			setbit(&siso, 13, (unsigned char *)curdate, 4);
				
			memset(tmpbuf, 0, sizeof(tmpbuf));
			if(getstrlen(pep_jnls.termtrc)==0)
				memcpy(tmpbuf, curdate, 6);
			else
				memcpy(tmpbuf, pep_jnls.termtrc, 6);
			memcpy(tmpbuf+6, curdate+4,6);
			setbit(&siso, 37, (unsigned char *)tmpbuf, 12);
			setbit(&siso, 32, (unsigned char *)"40002900", 8);
		}
		else if(memcmp(pep_jnls.trancde, "009", 3)==0)
		{
			dcs_log(0, 0, "电子签名错误应答返回");
			setbit(&siso, 0, (unsigned char *)"0830", 4);		
			setbit(&siso, 39, (unsigned char *)retcode, 2);
		}
		//批上送错误应答报文返回
		else if(memcmp(pep_jnls.trancde, "006", 3)==0 || memcmp(pep_jnls.trancde, "046", 3)==0)
		{
			dcs_log(0, 0, "批上送错误应答返回");
			setbit(&siso, 0, (unsigned char *)"0330", 4);
			setbit(&siso, 12, (unsigned char *)curdate+4, 6);
			setbit(&siso, 13, (unsigned char *)curdate, 4);
				
			memset(tmpbuf, 0, sizeof(tmpbuf));
			if(getstrlen(pep_jnls.termtrc)==0)
				memcpy(tmpbuf, curdate, 6);
			else
				memcpy(tmpbuf, pep_jnls.termtrc, 6);
			memcpy(tmpbuf+6, curdate+4,6);
			setbit(&siso, 37, (unsigned char *)tmpbuf, 12);
				
			setbit(&siso, 32, (unsigned char *)"40002900", 8);
			setbit(&siso, 39, (unsigned char *)retcode, 2);
		}
	
		//批结算返回
		else if(memcmp(pep_jnls.trancde, "005", 3)==0)
		{
			dcs_log(0, 0, "批结算错误应答返回");
			setbit(&siso, 0, (unsigned char *)"0510", 4);
			setbit(&siso, 12,(unsigned char *)curdate+4, 6);
			setbit(&siso, 13,(unsigned char *)curdate, 4);
			setbit(&siso, 15,(unsigned char *)curdate, 4);
	
			memset(tmpbuf, 0, sizeof(tmpbuf));
			if(getstrlen(pep_jnls.termtrc)==0)
				memcpy(tmpbuf, curdate, 6);
			else
				memcpy(tmpbuf, pep_jnls.termtrc, 6);
			memcpy(tmpbuf+6, curdate+4,6);
			setbit(&siso, 37,(unsigned char *)tmpbuf, 12);	
			setbit(&siso, 32,(unsigned char *)"40002900", 8);
			setbit(&siso, 39,(unsigned char *)retcode, 2);
			memset(tmpbuf, 0, sizeof(tmpbuf));
			s = getbit(&siso, 48,(unsigned char *)tmpbuf);
			if(s > 0)
			{
				memcpy(tmpbuf+30, "3", 1);
				memcpy(tmpbuf+s-1, "1", 1);
				setbit(&siso, 48,(unsigned char *)tmpbuf, s);
			}
			setbit(&siso, 2, (unsigned char *)NULL, 0);	
		}
		else if(memcmp(pep_jnls.trancde, "008", 3)==0)
		{
			setbit(&siso, 0,(unsigned char *)"0810", 4);
			setbit(&siso, 12,(unsigned char *)curdate+4, 6);
			setbit(&siso, 13,(unsigned char *)curdate, 4);
			setbit(&siso, 39, (unsigned char *)retcode, 2);
		}	
		//签到或参数下载返回
		else if(memcmp(pep_jnls.trancde, "003", 3)==0||memcmp(pep_jnls.trancde, "007", 3)==0)
		{	
			setbit(&siso, 0,(unsigned char *)"0810", 4);
			setbit(&siso, 12,(unsigned char *)curdate+4, 6);
			setbit(&siso, 13,(unsigned char *)curdate, 4);
				
			memset(tmpbuf, 0, sizeof(tmpbuf));
			if(memcmp(pep_jnls.trancde, "007", 3) == 0)
			{
				dcs_log(0, 0, "参数下载错误应答返回");
				memcpy(tmpbuf, curdate, 6);
			}
			else
			{
				dcs_log(0, 0, "签到错误应答返回");
				setbit(&siso, 32, (unsigned char *)"40002900", 8);
				if(getstrlen(pep_jnls.termtrc)==0)
					memcpy(tmpbuf, curdate, 6);
				else
					memcpy(tmpbuf, pep_jnls.termtrc, 6);
			}	
			memcpy(tmpbuf+6, curdate+4,6);
			setbit(&siso, 37, (unsigned char *)tmpbuf, 12);	
			setbit(&siso, 39, (unsigned char *)retcode, 2);
		}
	
		//签退返回
		else if(memcmp(pep_jnls.trancde, "004", 3)==0)
		{
			setbit(&siso, 0, (unsigned char *)"0830", 4);
			dcs_log(0, 0, "签退错误应答返回");
			setbit(&siso, 12, (unsigned char *)curdate+4, 6);
			setbit(&siso, 13, (unsigned char *)curdate, 4);	
			memset(tmpbuf, 0, sizeof(tmpbuf));
			if(getstrlen(pep_jnls.termtrc)==0)
				memcpy(tmpbuf, curdate, 6);
			else
				memcpy(tmpbuf, pep_jnls.termtrc, 6);
			memcpy(tmpbuf+6, curdate+4,6);
			setbit(&siso, 37, (unsigned char *)tmpbuf, 12);		
			setbit(&siso, 32, (unsigned char *)"40002900", 8);
			setbit(&siso, 39, (unsigned char *)retcode, 2);	
		}
		else if(memcmp(pep_jnls.trancde, "002", 3)==0 || memcmp(pep_jnls.trancde, "051", 3)==0 )
		{	
			if(memcmp(pep_jnls.trancde, "002", 3)==0)
			{
				dcs_log(0,0,"退货错误应答返回");
			}
			else if(memcmp(pep_jnls.trancde, "051", 3)==0)
			{
				dcs_log(0,0,"脱机退货错误应答返回");
			}
			setbit(&siso, 0, (unsigned char *)"0230", 4);
			setbit(&siso, 2, (unsigned char *)pep_jnls.outcdno, getstrlen(pep_jnls.outcdno));
			setbit(&siso, 3, (unsigned char *)"200000", 6);
			setbit(&siso, 12,(unsigned char *)curdate+4, 6);
			setbit(&siso, 13,(unsigned char *)curdate, 4);
			setbit(&siso, 15,(unsigned char *)curdate, 4);
			setbit(&siso, 25,(unsigned char *)"00", 2);// 返回原25域内容
			setbit(&siso, 32,(unsigned char *)"40002900", 8);
		
			memset(tmpbuf, 0, sizeof(tmpbuf));
			if(getstrlen(pep_jnls.termtrc)==0)
				memcpy(tmpbuf, curdate, 6);
			else
			{
				memcpy(tmpbuf, pep_jnls.termtrc, 6);
				setbit(&siso, 11, (unsigned char *)pep_jnls.termtrc, 6);
			}	
			memcpy(tmpbuf+6, curdate+4, 6);
			setbit(&siso, 37,(unsigned char *)tmpbuf, 12);
			setbit(&siso, 39,(unsigned char *)retcode, 2);
			setbit(&siso, 44,(unsigned char *)"00000000   00000000   ", 22);
			setbit(&siso, 63,(unsigned char *)"CUP", 3);	
		}
		else if(memcmp(pep_jnls.trancde, "000", 3)==0 || memcmp(pep_jnls.trancde, "M01", 3)==0
		|| memcmp(pep_jnls.trancde, "050", 3)==0 || memcmp(pep_jnls.trancde, "010", 3)==0 
		|| memcmp(pep_jnls.trancde, "03", 2)==0 || memcmp(pep_jnls.trancde, "001", 3)==0 
		|| memcmp(pep_jnls.trancde, "D0", 2)==0 )
		{
			//消费和余额查询和撤销
			setbit(&siso, 0, (unsigned char *)"0210", 4);
			setbit(&siso, 2, (unsigned char *)pep_jnls.outcdno, getstrlen(pep_jnls.outcdno));
			if(memcmp(pep_jnls.trancde, "000", 3)==0 || memcmp(pep_jnls.trancde, "M01", 3)==0
			|| memcmp(pep_jnls.trancde, "050", 3)==0 || memcmp(pep_jnls.trancde, "001", 3)==0 
			|| memcmp(pep_jnls.trancde, "032", 3)==0 || memcmp(pep_jnls.trancde, "033", 3)==0)
			{
				if(memcmp(pep_jnls.trancde, "000", 3)==0 || memcmp(pep_jnls.trancde, "M01", 3)==0
				|| memcmp(pep_jnls.trancde, "050", 3)==0 || memcmp(pep_jnls.trancde, "032", 3)==0 )
				{
					if(memcmp(pep_jnls.trancde, "000", 3)==0 || memcmp(pep_jnls.trancde, "M01", 3)==0)
						dcs_log(0, 0, "消费错误应答返回");
					else if(memcmp(pep_jnls.trancde, "032", 3)==0)
						dcs_log(0, 0, "预授权完成请求错误应答返回");
					else 
						dcs_log(0, 0, "脱机类消费交易错误应答返回");
					setbit(&siso, 3, (unsigned char *)"000000", 6);
				}
				else if(memcmp(pep_jnls.trancde, "001", 3)==0 ||memcmp(pep_jnls.trancde, "033", 3)==0 )
				{
					if(memcmp(pep_jnls.trancde, "001", 3)==0)
						dcs_log(0, 0, "消费撤销错误应答返回");
					else
						dcs_log(0, 0, "预授权完成撤销撤销错误应答返回");
					setbit(&siso, 3, (unsigned char *)"200000", 6);
				}
				setbit(&siso, 15,(unsigned char *)curdate, 4);
				setbit(&siso, 63,(unsigned char *)"CUP", 3);
			}
			else if(memcmp(pep_jnls.trancde, "030", 3)==0)
			{
				dcs_log(0, 0, "预授权错误应答返回");
				setbit(&siso, 0, (unsigned char *)"0110", 4);
				setbit(&siso, 3, (unsigned char *)"030000", 6);
			}
			else if(memcmp(pep_jnls.trancde, "031", 3)==0)
			{
				dcs_log(0, 0, "预授权撤销错误应答返回");
				setbit(&siso, 0, (unsigned char *)"0110", 4);
				setbit(&siso, 3, (unsigned char *)"200000", 6);
			}
			else if(memcmp(pep_jnls.trancde, "010", 3)==0 || memcmp(pep_jnls.trancde, "D02", 3)==0)
			{
				dcs_log(0, 0, "余额查询错误应答返回");
				setbit(&siso, 3, (unsigned char *)"310000", 6);
				setbit(&siso, 41,(unsigned char *)mer_info_t.mgr_term_id, 8);
				setbit(&siso, 42,(unsigned char *)mer_info_t.mgr_mer_id, 15);
			}
			else if(memcmp(pep_jnls.trancde, "D01", 3)==0)
			{
				dcs_log(0, 0, "积分消费错误应答返回");
				setbit(&siso, 3, (unsigned char *)"000000", 6);
				setbit(&siso, 41,(unsigned char *)mer_info_t.mgr_term_id, 8);
				setbit(&siso, 42,(unsigned char *)mer_info_t.mgr_mer_id, 15);
			}
			else if(memcmp(pep_jnls.trancde, "D03", 3)==0 || memcmp(pep_jnls.trancde, "D04", 3)==0)
			{
				dcs_log(0, 0, "用户认证的错误应答返回");
				setbit(&siso, 3, (unsigned char *)"330000", 6);
				setbit(&siso, 41,(unsigned char *)mer_info_t.mgr_term_id, 8);
				setbit(&siso, 42,(unsigned char *)mer_info_t.mgr_mer_id, 15);
				setbit(&siso, 48,(unsigned char *)NULL, 0);
			}
			setbit(&siso, 12,(unsigned char *)curdate+4, 6);
			setbit(&siso, 13,(unsigned char *)curdate, 4);			
			setbit(&siso, 32,(unsigned char *)"40002900", 8);
		
			memset(tmpbuf, 0, sizeof(tmpbuf));
			if(getstrlen(pep_jnls.termtrc)==0)
				memcpy(tmpbuf, curdate, 6);
			else
			{
				memcpy(tmpbuf, pep_jnls.termtrc, 6);
				setbit(&siso, 11, (unsigned char *)pep_jnls.termtrc, 6);
			}	
			memcpy(tmpbuf+6, curdate+4, 6);
			setbit(&siso, 37,(unsigned char *)tmpbuf, 12);
			setbit(&siso, 39,(unsigned char *)retcode, 2);
			setbit(&siso, 44,(unsigned char *)"00000000   00000000   ", 22);
		
		}
		//处理信用卡还款和电银汇款的错误应答
		else if(memcmp(pep_jnls.trancde, "013", 3)==0 || memcmp(pep_jnls.trancde, "017", 3)==0)
		{	
			dcs_log(0, 0, "错误处理");
			setbit(&siso, 0, (unsigned char *)"0210", 4);
			setbit(&siso, 2, (unsigned char *)pep_jnls.outcdno, getstrlen(pep_jnls.outcdno));
			setbit(&siso, 12, (unsigned char *)curdate+4, 6);
			setbit(&siso, 13, (unsigned char *)curdate, 4);			
			setbit(&siso, 32, (unsigned char *)"40002900", 8);
		
			memset(tmpbuf, 0, sizeof(tmpbuf));
			if(getstrlen(pep_jnls.termtrc)==0)
				memcpy(tmpbuf, curdate, 6);
			else
			{
				memcpy(tmpbuf, pep_jnls.termtrc, 6);
				setbit(&siso, 11, (unsigned char *)pep_jnls.termtrc, 6);
			}	
			memcpy(tmpbuf+6, curdate+4, 6);
			setbit(&siso, 37, (unsigned char *)tmpbuf, 12);
			setbit(&siso, 39, (unsigned char *)retcode, 2);
			setbit(&siso, 41, (unsigned char *)mer_info_t.mgr_term_id, 8);
			setbit(&siso, 42, (unsigned char *)mer_info_t.mgr_mer_id, 15);
		
			setbit(&siso, 44, (unsigned char *)"00000000   00000000   ", 22);
			if(memcmp(pep_jnls.trancde, "013", 3)==0)
			{
				dcs_log(0, 0, "电银转账错误处理");
				setbit(&siso, 60, (unsigned char *)NULL, 0);
			}
			else
			{
				dcs_log(0, 0, "信用卡错误处理");
			}
		}
		//04X交易码统一属于脚本通知类（类别码：0012），该类交易不参与对账和清算。
		 else if(memcmp(pep_jnls.trancde, "04", 2)==0 && memcmp(pep_jnls.trancde, "046", 3)!=0)
        {
            dcs_log(0, 0, "脚本通知错误应答返回");
            setbit(&siso, 0, (unsigned char *)"0630", 4);       
            setbit(&siso, 39, (unsigned char *)retcode, 2);
			if(getstrlen(pep_jnls.termtrc)!=0)
				setbit(&siso, 11, (unsigned char *)pep_jnls.termtrc, 6);
            setbit(&siso, 12,(unsigned char *)curdate+4, 6);
            setbit(&siso, 13,(unsigned char *)curdate, 4);
            setbit(&siso, 41,(unsigned char *)mer_info_t.mgr_term_id, 8);
			setbit(&siso, 42,(unsigned char *)mer_info_t.mgr_mer_id, 15);
        }
        else if(memcmp(pep_jnls.trancde, "901", 3)==0)
        {
            dcs_log(0, 0, "POS状态上送错误应答返回");
            setbit(&siso, 0,(unsigned char *)"0830", 4);
            setbit(&siso, 12,(unsigned char *)curdate+4, 6);
            setbit(&siso, 13,(unsigned char *)curdate, 4);
            setbit(&siso, 39, (unsigned char *)retcode, 2);
        }
        else if(memcmp(pep_jnls.trancde, "902", 3)==0)
        {
            dcs_log(0, 0, "POS公钥、参数传递错误应答返回");
            setbit(&siso, 0,(unsigned char *)"0810", 4);
            setbit(&siso, 12,(unsigned char *)curdate+4, 6);
            setbit(&siso, 13,(unsigned char *)curdate, 4);
            setbit(&siso, 39, (unsigned char *)retcode, 2);
        }
        else if(memcmp(pep_jnls.trancde, "903", 3)==0)
        {
            dcs_log(0, 0, "POS公钥、参数下载结束错误应答返回");
            setbit(&siso, 0,(unsigned char *)"0810", 4);
            setbit(&siso, 12,(unsigned char *)curdate+4, 6);
            setbit(&siso, 13,(unsigned char *)curdate, 4);
            setbit(&siso, 39, (unsigned char *)retcode, 2);
        }
	}
	//冲正
	else if(memcmp(msgtype, "0400", 4)== 0)
	{
		//消费冲正 or 消费撤销冲正 or 预授权冲正 or 预授权撤销冲正 or 预授权完成请求冲正 or 预授权完成撤销冲正
		if(memcmp(pep_jnls.trancde, "000", 3)==0 || memcmp(pep_jnls.trancde, "M01", 3)==0 ||  memcmp(pep_jnls.trancde, "C00", 3)==0 )
			dcs_log(0, 0, "消费冲正错误应答返回");
		else if(memcmp(pep_jnls.trancde, "001", 3)==0 || memcmp(pep_jnls.trancde, "C01", 3)==0)//消费撤销冲正
			dcs_log(0, 0, "消费撤销冲正错误应答返回");
		else if(memcmp(pep_jnls.trancde, "C30", 3)==0)
		{
			dcs_log(0, 0, "预授权冲正错误应答返回");
			setbit(&siso, 3, (unsigned char *)"030000", 6);
		}
		else if(memcmp(pep_jnls.trancde, "C31", 3)==0)
		{
			dcs_log(0, 0, "预授权撤销冲正错误应答返回");
			setbit(&siso, 3, (unsigned char *)"200000", 6);
		}
		else if(memcmp(pep_jnls.trancde, "C32", 3)==0)
		{
			dcs_log(0, 0, "预授权完成请求冲正错误应答返回");
			setbit(&siso, 3, (unsigned char *)"000000", 6);
		}
		else if(memcmp(pep_jnls.trancde, "C33", 3)==0)
		{
			dcs_log(0, 0, "预授权完成撤销冲正错误应答返回");
			setbit(&siso, 3, (unsigned char *)"200000", 6);
		}
		setbit(&siso, 0, (unsigned char *)"0410", 4);
		setbit(&siso, 12, (unsigned char *)curdate+4, 6);//时间
		setbit(&siso, 13, (unsigned char *)curdate, 4);//日期
		setbit(&siso, 15, (unsigned char *)curdate, 4);
				
		memset(tmpbuf, 0, sizeof(tmpbuf));
		if(getstrlen(pep_jnls.termtrc)!=0)
		{
			setbit(&siso, 11, (unsigned char *)pep_jnls.termtrc, 6);
			memcpy(tmpbuf, pep_jnls.termtrc,6);
		}
		else
			memcpy(tmpbuf, curdate, 6);
		memcpy(tmpbuf+6, curdate+4, 6);
		setbit(&siso, 37, (unsigned char *)tmpbuf, 12);	
				
		setbit(&siso, 32, (unsigned char *)"40002900", 8);
		setbit(&siso, 39, (unsigned char *)retcode, 2);
				
		setbit(&siso, 44, (unsigned char *)"00000000   00000000   ", 22);	
		setbit(&siso, 62, (unsigned char *)NULL, 0);
	}
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memcpy(tmpbuf, pep_jnls.trancde, 3);
	if(memcmp(pep_jnls.billmsg, "DY",2)==0)
	{
		len =0;
		sprintf(tmpbuf+3, "%02d", len);
		setbit(&siso, 20, (unsigned char *)tmpbuf, 5);	
	}
	else
	{
		len = getstrlen(pep_jnls.billmsg);
		sprintf(tmpbuf+3, "%02d", len);
		memcpy(tmpbuf+5, pep_jnls.billmsg, len);
		setbit(&siso, 20, (unsigned char *)tmpbuf, len+5);	
	}
	
	setbit(&siso, 7, (unsigned char *)NULL, 0);	
	setbit(&siso, 21, (unsigned char *)NULL, 0);
	setbit(&siso, 22, (unsigned char *)NULL, 0);
	setbit(&siso, 26, (unsigned char *)NULL, 0);
	setbit(&siso, 35, (unsigned char *)NULL, 0);
	setbit(&siso, 36, (unsigned char *)NULL, 0);
	setbit(&siso, 48, (unsigned char *)NULL, 0);
	setbit(&siso, 52, (unsigned char *)NULL, 0);
	setbit(&siso, 53, (unsigned char *)NULL, 0);
	setbit(&siso, 64, (unsigned char *)NULL, 0);
	//setbit(&siso, 55, (unsigned char *)NULL, 0);
	setbit(&siso, 62, (unsigned char *)NULL, 0);
	
	sndBackToPos(siso, pep_jnls.trnsndpid, mer_info_t);	
	return 0;
}

/*get  bit_20 ,Bit_21,bit_23*/
int GetBit_fiso(ISO_data siso, PEP_JNLS *pep_jnls)
{
	char tmpbuf[127], temp_len1[3];
	int s, temp_len2, len;
	
	memset(tmpbuf,0,sizeof(tmpbuf));
	memset(temp_len1,0,sizeof(temp_len1));
	
	s = getbit(&siso, 20, (unsigned char *)tmpbuf);
	
#ifdef __LOG__
	dcs_log(0, 0, "pos报文20域：[%s], s= [%d]", tmpbuf, s);
#endif
	if(s >= 5)
	{
		memcpy(pep_jnls->trancde, tmpbuf, 3);
		memcpy(temp_len1, tmpbuf+3, 2);
		sscanf(temp_len1, "%d", &temp_len2);
		if(temp_len2 !=0)
		{
			memcpy(pep_jnls->billmsg, tmpbuf+5, temp_len2);
			dcs_log(0, 0, "订单号：[%s]", pep_jnls->billmsg);
		}
	}
	else
	{	
		dcs_log(0, 0, "Error,20域的数据有问题,长度[%d],值[%s].", s, tmpbuf);
		return -1;
	}	
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso, 21, (unsigned char *)tmpbuf);
	if( s <= 0)
	{
		dcs_log(0, 0, "Error,取21域数据错误,长度[%d]", s);
		return -1;
	}
#ifdef __LOG__
	dcs_log(0, 0, "21域实际长度 = %d", s);
#endif
	len = 0;
	/*取机具编号信息*/
	s -=2;
	if( s< 0)
	{
		dcs_log(0, 0, "21域数据错误");
		return -1;
	}
#ifdef __LOG__
	dcs_debug(tmpbuf, s+2, "21域数据:");
#endif
	memset(temp_len1, 0, sizeof(temp_len1));
	memcpy(temp_len1, tmpbuf, 2);
	len +=2;
#ifdef __LOG__
	dcs_log(0, 0, "temp_len1 = %s", temp_len1);
#endif
	sscanf(temp_len1, "%d", &temp_len2);
#ifdef __LOG__
	dcs_log(0, 0, "temp_len2 = %d", temp_len2);
#endif

	if(temp_len2 > 25)
	{
		dcs_log(0, 0, "机具编号长度不应该超过25");
		return -1;
	}
	else if(temp_len2 != 0)
	{
		memcpy(pep_jnls->termidm, tmpbuf+2, temp_len2);
#ifdef __LOG__
		dcs_log(0, 0, "pep_jnls->termidm =[%s]", pep_jnls->termidm);
#endif
		len +=temp_len2;
	}
	s -=temp_len2;
	if( s< 0)
	{
		dcs_log(0, 0, "21域数据错误");
		return -1;
	}
	/*取GPRS定位信息*/
	memset(temp_len1, 0, sizeof(temp_len1));
	memcpy(temp_len1, tmpbuf+len, 2);
	len +=2;
	sscanf(temp_len1, "%d", &temp_len2);
	s -=2;
	if( s < 0)
	{
		dcs_log(0, 0, "21域数据错误");
		return -1;
	}
	if(temp_len2!=0)
	{
		memcpy(pep_jnls->gpsid, tmpbuf+len, temp_len2);
		len +=temp_len2;
	}
	s -=temp_len2;
	if( s< 0)
	{
		dcs_log(0, 0, "21域数据错误");
		return -1;
	}
	/*取APN信息*/
	memset(temp_len1, 0, sizeof(temp_len1));
	memcpy(temp_len1, tmpbuf+len, 2);
	len +=2;
	sscanf(temp_len1, "%d", &temp_len2);
	s -=2;
	if( s< 0)
	{
		dcs_log(0, 0, "21域数据错误");
		return -1;
	}
	if(temp_len2!=0 && temp_len2<26)
	{
		memcpy(pep_jnls->termid, tmpbuf+len, temp_len2);
		len +=temp_len2;
	}
	else if(temp_len2>25)
	{
		dcs_log(0, 0, "21.3域数据错误");
	}
	s -=temp_len2;
	if(s == 0)
	{
#ifdef __LOG__
		dcs_log(0, 0, "21域处理完毕长度= %d", s);
#endif
	}

	return 0;
}

/*参数下载的处理*/
int DownLoadinfo(ISO_data siso, char *currentDate, char *termidm, MER_INFO_T mer_info_t, int iFid)
{
	unsigned char tran_batch_no[6+1], tmpbuf[512], tmptpdu[23], version_info[5];
	unsigned char gbktmp[40*3];
	int rtn, s, len = 0;
	
	memset(tran_batch_no, 0, sizeof(tran_batch_no));
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(tmptpdu, 0, sizeof(tmptpdu));
	memset(version_info, 0, sizeof(version_info));
	
	memcpy(tmptpdu, mer_info_t.mgr_tpdu, 10);
	memcpy(version_info, mer_info_t.cur_version_info, 4);
	
	setbit(&siso, 0, (unsigned char *)"0810", 4);
	setbit(&siso, 12, (unsigned char *)currentDate+8, 6);
	setbit(&siso, 13, (unsigned char *)currentDate+4, 4);
	setbit(&siso, 21, (unsigned char *)NULL, 0);
	setbit(&siso, 37, (unsigned char *)currentDate+2, 12);
	setbit(&siso, 39, (unsigned char *)"00", 2);
	
	/*根据机具编号查询数据表pos_conf_info得到一些参数信息*/
	rtn = GetParameter(termidm, &mer_info_t);
	if(rtn < 0)
	{
		dcs_log(0,0,"获取参数信息错误");
		return -1;
	}
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso, 60, (unsigned char *)tmpbuf);
	if(s <= 0)
	{
		dcs_log(0, 0, "获取60域信息错误");
		return -1;
	}
	memcpy(tmpbuf+2, mer_info_t.mgr_batch_no, 6);
	memcpy(tmpbuf+8, "365", 3);
	setbit(&siso, 60, (unsigned char *)tmpbuf, 11);
#ifdef __LOG__
	dcs_debug(tmpbuf, 11, "60bit info");	
#endif
	setbit(&siso, 41, (unsigned char *)mer_info_t.mgr_term_id, 8);
	setbit(&siso, 42, (unsigned char *)mer_info_t.mgr_mer_id, 15);
	memcpy(mer_info_t.batch_settle_flag, "00", 2);
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(gbktmp, 0, sizeof(gbktmp));
	
	if(u2g(mer_info_t.mgr_mer_des, strlen(mer_info_t.mgr_mer_des), gbktmp, sizeof(gbktmp)) < 0)
	{
		dcs_log(0,0,"u2g() fail!");
		return -1;
	}	
	gbktmp[40] = 0;
	sprintf((char *)tmpbuf, "%s%-40s%s%s%s%s", "22", gbktmp, "27", mer_info_t.mgr_des, "28", mer_info_t.batch_settle_flag);
	/*sprintf(tmpbuf+51, "%s%s%s%02d%s%s", "25", mer_info_t.pos_main_key_index, "12" ,mer_info_t.mgr_timeout, "21", mer_info_t.mgr_signout_flag);*/
	sprintf((char *)tmpbuf+51, "%s%s%s%02d%s%s", "33", mer_info_t.input_order_flg, "12", mer_info_t.mgr_timeout, "21", mer_info_t.mgr_signout_flag);
		
	sprintf((char *)tmpbuf+62, "%s%s%s%d%s%s", "11", mer_info_t.pos_type, "13", mer_info_t.cz_retry_count, "31", "10");
	len = 73;
	if(getstrlen(mer_info_t.trans_telno1)!= 0)
	{
		sprintf((char *)tmpbuf+len, "%s%-14s", "14", mer_info_t.trans_telno1);
		len +=16;
	}
	if(getstrlen(mer_info_t.trans_telno2)!= 0)
	{
		sprintf((char *)tmpbuf+len, "%s%-14s", "15", mer_info_t.trans_telno2);
		len +=16;
	}
	if(getstrlen(mer_info_t.trans_telno3)!= 0)
	{
		sprintf((char *)tmpbuf+len, "%s%-14s", "16", mer_info_t.trans_telno3);
		len +=16;
	}
	if(getstrlen(mer_info_t.manager_telno)!= 0)
	{
		sprintf((char *)tmpbuf+len, "%s%-14s", "17", mer_info_t.manager_telno);
		len +=16;
	}
	sprintf((char *)tmpbuf+len, "%s%d%s%d", "20", mer_info_t.hand_flag, "23", mer_info_t.trans_retry_count);
	len += 6;
	if(getstrlen(mer_info_t.pos_ip_port1)!= 0)
	{
		sprintf((char *)tmpbuf+len, "%s%-30s", "29", mer_info_t.pos_ip_port1);
		len +=32;
	}
	if(getstrlen(mer_info_t.pos_ip_port2)!= 0)
	{
		sprintf((char *)tmpbuf+len, "%s%-30s", "30", mer_info_t.pos_ip_port2);
		len +=32;
	}
	
	//sprintf(tmpbuf+73, "%s%-14s%s%-14s%s%-14s%", "14", mer_info_t.trans_telno1, "15", mer_info_t.trans_telno2, "16", mer_info_t.trans_telno3);
	//sprintf(tmpbuf+73+48, "%s%-14s%s%d%s%d", "17", mer_info_t.manager_telno, "20", mer_info_t.hand_flag, "23", mer_info_t.trans_retry_count);
	/*sprintf(tmpbuf+143, "%s%-30s%s%-30s%s%s", "29", mer_info_t.pos_ip_port1, "30" ,mer_info_t.pos_ip_port2, "32", mer_info_t.rtn_track_pwd_flag);*/
	//sprintf(tmpbuf+143, "%s%-30s%s%-30s", "29", mer_info_t.pos_ip_port1, "30" ,mer_info_t.pos_ip_port2);
	
	if(mer_info_t.pos_machine_type == 4)
		memcpy(tmpbuf+len, "361", 3);
	else
		memcpy(tmpbuf+len, "360", 3);
	len +=3;
	sprintf((char *)tmpbuf+len, "%s%d", "34", mer_info_t.pos_update_flag);
	len +=3;
	if(getstrlen(mer_info_t.pos_update_add)!= 0)
	{
		sprintf((char *)tmpbuf+len, "%s%-30s", "35", mer_info_t.pos_update_add);
		len +=32;
	}
	
	if(getstrlen(mer_info_t.remitfeeformula)!= 0)
	{
		sprintf((char *)tmpbuf+len, "%s%-25s", "37", mer_info_t.remitfeeformula);
		len +=27;
	}
	if(getstrlen(mer_info_t.creditfeeformula)!= 0)
	{
		sprintf((char *)tmpbuf+len, "%s%-25s", "38", mer_info_t.creditfeeformula);
		len +=27;
	}
	
	memcpy(tmpbuf+len, "26", 2);
	len +=2;
	tmpbuf[len]=((mer_info_t.tag26>>8)&0xFF);
	tmpbuf[len+1]=(mer_info_t.tag26&0xFF);
	
	len +=2;

	memcpy(tmpbuf+len, "39", 2);
	len +=2;
	tmpbuf[len]=((mer_info_t.tag39>>8)&0xFF);
	tmpbuf[len+1]=(mer_info_t.tag39&0xFF);
	len +=2;
	
	setbit(&siso, 62, (unsigned char *)tmpbuf, len);
#ifdef __LOG__
	dcs_debug(tmpbuf, len, "62bit info");
#endif
	if(mer_info_t.para_update_flag=='1')
	{
		UpdatePos_info(mer_info_t, mer_info_t.pos_machine_id, 4);
		mer_info_t.para_update_flag ='0';
	}
	memcpy(mer_info_t.mgr_tpdu, tmptpdu, 10);
	memcpy(mer_info_t.cur_version_info, version_info, 4);
	s = sndBackToPos(siso, iFid, mer_info_t);
	if(s < 0 )
	{
		dcs_log(0,0,"posUnpack error!");
		return -1;
	}
	
	return 0;
}

/*处理冲正消费冲正和撤销冲正*/
int ProcessRevoid(ISO_data siso, PEP_JNLS pep_jnls, char *currentDate, MER_INFO_T mer_info_t)
{
	char tmpbuf[127], databuf[10+1], trace[6+1] /*, bit_22[3]*/;
	int s, rtn;
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(databuf, 0, sizeof(databuf));
	memset(trace, 0, sizeof(trace));
	
	memcpy(databuf, currentDate+4, 10);
	memcpy(pep_jnls.termcde, mer_info_t.mgr_term_id, 8);
	memcpy(pep_jnls.mercode, mer_info_t.mgr_mer_id, 15);
	
	s = getbit(&siso, 11, (unsigned char *)tmpbuf);
	if(s <= 0)
	{
		dcs_log(0, 0, "get bit_11 error!");	
		BackToPos("H9", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
	memcpy(pep_jnls.termtrc, tmpbuf, 6);			
	
	if(memcmp(pep_jnls.trancde, "001", 3)==0||memcmp(pep_jnls.trancde, "C31", 3)==0 ||memcmp(pep_jnls.trancde, "C33", 3)==0)
	{	
		memcpy(pep_jnls.process, "200000", 6);
		dcs_log(0, 0, "查询原笔交易");
		if(memcmp(pep_jnls.trancde, "001", 3)==0||memcmp(pep_jnls.trancde, "C31", 3)==0)
		{
			rtn = GetOrignTrans(siso, currentDate, &pep_jnls, trace);
			if(rtn == -1)
			{
				dcs_log(0,0,"查找原笔交易流水失败");
				SaveErTradeInfo("25", currentDate, "查找原笔交易流水失败", mer_info_t, pep_jnls, trace);
				BackToPos("25", siso, pep_jnls, databuf, mer_info_t);
				return -1;
			}
		}
		else if(memcmp(pep_jnls.trancde, "C33", 3)==0)
		{
			dcs_log(0, 0, "预授权完成撤销冲正");
			rtn = GetPreTrans(siso, currentDate, &pep_jnls, trace);
			if(rtn == -1)
			{
				dcs_log(0,0,"查找原笔交易流水失败");
				SaveErTradeInfo("25", currentDate, "查找原笔交易流水失败", mer_info_t, pep_jnls, trace);
				BackToPos("25", siso, pep_jnls, databuf, mer_info_t);
				return -1;
			}
		}
	}
	else if(memcmp(pep_jnls.trancde, "000", 3)==0 || memcmp(pep_jnls.trancde, "M01", 3)==0
	||memcmp(pep_jnls.trancde, "C32", 3)==0 || memcmp(pep_jnls.trancde, "C30", 3)==0)
	{
		if(memcmp(pep_jnls.trancde, "C30", 3)==0)
		{
			memcpy(pep_jnls.process, "030000", 6);
			dcs_log(0,0,"预授权冲正查询原笔交易流水放到11域中");
		}
		else
			memcpy(pep_jnls.process, "000000", 6);
		dcs_log(0,0,"查询原笔交易流水放到11域中");
		rtn = GetTrace(siso, trace, currentDate, &pep_jnls);
		if(rtn == -1)
		{
			dcs_log(0, 0, "查找原笔交易流水失败");
			SaveErTradeInfo("25", currentDate, "查找原笔交易流水失败", mer_info_t, pep_jnls, trace);
			BackToPos("25", siso, pep_jnls, databuf, mer_info_t);
			return -1;
		}
		dcs_log(0,0,"posproc pep_jnls.outcdtype=[%c]", pep_jnls.outcdtype);
	}

	setbit(&siso, 11, (unsigned char *)trace, 6);
	setbit(&siso, 7, (unsigned char *)databuf, 10); 
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso, 60, (unsigned char *)tmpbuf);
	if( s<= 0)
	{
	 	dcs_log(0, 0, "can not get bit_60!");	
	 	SaveErTradeInfo(RET_CODE_H9,currentDate, "取60域错误", mer_info_t, pep_jnls, trace);
		BackToPos(RET_CODE_H9, siso, pep_jnls, databuf, mer_info_t);		
	 	return -1;
	}
				
	rtn = GetChannelRestrict(mer_info_t, pep_jnls.trancde);
	if(rtn < 0)
	{
		dcs_log(0, 0, "渠道限制交易，交易不受理");
		setbit(&siso, 39, (unsigned char *)"12", 2);
		SaveErTradeInfo("FE", currentDate, "渠道限制交易，交易不受理", mer_info_t, pep_jnls, trace);
		BackToPos("FE", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
				
	memcpy(pep_jnls.batch_no, tmpbuf+2, 6);
	memset(tmpbuf, 0, sizeof(tmpbuf));
	sprintf(tmpbuf, "%s%s%s%s%s", mer_info_t.mgr_mer_id, mer_info_t.mgr_term_id, "3", "03", pep_jnls.trancde);
	sprintf(tmpbuf+29, "%s%s%s%04d", pep_jnls.termtrc, pep_jnls.batch_no, pep_jnls.process, rtn);

	/*--- modify begin 140325 ---*/	
	if(getstrlen(mer_info_t.T0_flag)==0)
		memcpy(mer_info_t.T0_flag, "0", 1);
	if(getstrlen(mer_info_t.agents_code)==0)
		memcpy(mer_info_t.agents_code, "000000", 6);
	sprintf(tmpbuf+51, "%s", mer_info_t.T0_flag);
	sprintf(tmpbuf+52, "%s", mer_info_t.agents_code);
	setbit(&siso, 21, (unsigned char *)tmpbuf, 58);
	/*--- end by hw ---*/

	
	rtn = Update_reverflag(siso, trace, pep_jnls.termidm);
	if(rtn < 0)
	{
		dcs_log(0,0,"设置冲正字段失败");
		SaveErTradeInfo(RET_CODE_H7, currentDate, "设置冲正标记失败", mer_info_t, pep_jnls, trace);
		BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
	if(memcmp(pep_jnls.trancde, "001", 3)==0)
	{
		memcpy(pep_jnls.trancde, "C01", 3);
	}
	else if(memcmp(pep_jnls.trancde, "000", 3)==0 || memcmp(pep_jnls.trancde, "M01", 3)==0)
	{
		memcpy(pep_jnls.trancde, "C00", 3);
	}
	else if(memcmp(pep_jnls.trancde, "C30", 3)==0)
	{
		dcs_log(0, 0, "预授权冲正");
		setbit(&siso, 3, (unsigned char *)"590000", 6);	
	}
	else if(memcmp(pep_jnls.trancde, "C31", 3)==0)
	{
		dcs_log(0, 0, "预授权撤销冲正");
		setbit(&siso, 3, (unsigned char *)"500000", 6);	
	}
	else if(memcmp(pep_jnls.trancde, "C32", 3)==0)
	{
		dcs_log(0, 0, "预授权完成冲正");
		setbit(&siso, 3, (unsigned char *)"510000", 6);		
	}
	else if(memcmp(pep_jnls.trancde, "C33", 3)==0)
	{
		dcs_log(0, 0, "预授权完成撤销冲正");
		setbit(&siso, 3, (unsigned char *)"570000", 6);	
	}
	
	setbit(&siso, 2, (unsigned char *)pep_jnls.outcdno, getstrlen(pep_jnls.outcdno));	
	
	/*
	s = getbit(&siso, 22, (unsigned char *)bit_22);
    if(s <= 0)
    {
        dcs_log(0, 0, "get bit_22 error!");
        BackToPos("H9", siso, pep_jnls, databuf, mer_info_t);   
        return -1;
    } 
	*/
    /*保存pep_icinfo数据库*/
 /*   if(memcmp(bit_22, "05", 2) == 0 || memcmp(bit_22, "95", 2) == 0 )
    {

        rtn = WriteIcinfo(siso, pep_jnls, databuf);
        if(rtn == -1)
        {
            dcs_log(0, 0, "保存pep_icinfo数据失败");
            SaveErTradeInfo("FO", currentDate, "保存pep_icinfo数据失败", mer_info_t, pep_jnls, trace);
            BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
            return -1;
        }
        setbit(&siso, 55, (unsigned char *)NULL, 0);
        setbit(&siso, 23, (unsigned char *)NULL, 0);
    }   
*/	
	

	/*保存pep_jnls数据库*/
	rtn = WritePosXpep(siso, pep_jnls, databuf);
	if(rtn == -1)
	{
		dcs_log(0, 0, "保存pep_jnls数据失败");
		SaveErTradeInfo("FO", currentDate, "保存pep_jnls数据失败", mer_info_t, pep_jnls, trace);
		BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}	
	setbit(&siso, 20, (unsigned char *)NULL, 0);
	setbit(&siso, 64, (unsigned char *)NULL, 0);	

	memset(tmpbuf, 0, sizeof(tmpbuf));
	s=getbit(&siso, 60, (unsigned char *)tmpbuf);
	if( s<13 )
		memcpy(tmpbuf, "20", 2);
	else 
		memcpy(tmpbuf,tmpbuf+11,2);//取终端上送的60.4,60.5送给核心
	memcpy(tmpbuf+2,"03", 2);//终端类型
	tmpbuf[4]='\0';
	setbit(&siso, 60, (unsigned char *)tmpbuf, 4);
	if(strlen(card_sn) ==3)
		setbit(&siso, 23, (unsigned char *)card_sn, 3);
	
	//modify 20141114 放开本段注释
	/*
	dcs_log(0,0,"pep_jnls.authcode[%s]", pep_jnls.authcode);
	dcs_log(0,0,"pep_jnls.syseqno[%s]",  pep_jnls.syseqno);
	dcs_log(0,0,"pep_jnls.revdate[%s]",  pep_jnls.revdate);
	memset(tmpbuf, 0, sizeof(tmpbuf));
	sprintf(tmpbuf, "%s%s%s", pep_jnls.authcode, pep_jnls.syseqno, pep_jnls.revdate);
	setbit(&siso, 61, (unsigned char *)tmpbuf, 28);
	*/
	setbit(&siso, 61, (unsigned char *)NULL, 0);
	setbit(&siso, 63, (unsigned char *)NULL, 0); //二维码支付有63域信息，有中文发送到核心其解析不了


	
	//发送报文
	s = WriteXpe(siso);
	if(s == -1)
	{
		dcs_log(0, 0, "发送冲正信息给核心失败");
		SaveErTradeInfo(RET_CODE_F9, currentDate, "发送冲正信息给核心失败", mer_info_t, pep_jnls, trace);
		BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
	return 0;
}

/*处理消费撤销 、退货、消费、余额查询、脚本通知*/
int ProcessTrade(ISO_data siso, PEP_JNLS pep_jnls, MER_INFO_T mer_info_t, char *currentDate)
{
	char tmpbuf[127], trace[6+1], outPin[8+1], bit_22[3+1], databuf[10+1], IC_55[300];

	int s, rtn, n;
	char outCardType;
	
	//交易类型配置信息表，余额查询根据交易码查询数据库表msgchg_info，得到的信息放入该结构体中
	MSGCHGINFO	msginfo;
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(trace, 0, sizeof(trace));
	memset(outPin, 0, sizeof(outPin));
	memset(bit_22, 0, sizeof(bit_22));
	memset(databuf, 0, sizeof(databuf));
	memset(&msginfo, 0, sizeof(MSGCHGINFO));
	memset(IC_55, 0, sizeof(IC_55));
	
	memcpy(pep_jnls.termcde, mer_info_t.mgr_term_id, 8);
	memcpy(pep_jnls.mercode, mer_info_t.mgr_mer_id, 15);
	memcpy(databuf, currentDate+4, 10);
		
	//新增对脱机消费交易多次上送的处理
	if(memcmp(pep_jnls.trancde, "050", 3) ==0)
	{
		rtn = JudgeOrginTrans(siso, pep_jnls, mer_info_t, currentDate);
		if(rtn != 0)
			return 0;
	}
	if( pub_get_trace_from_seq(trace) < 0)
	{
		dcs_log(0, 0, "取前置系统交易流水错误");
		SaveErTradeInfo(RET_CODE_F8, currentDate, "取前置系统交易流水错误", mer_info_t, pep_jnls, trace);
		BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
	/*交易码来查询数据库msgchg_info来读取一些信息*/
	rtn = Readmsgchg(pep_jnls.trancde, &msginfo);
	if(rtn == -1)
	{
		dcs_log(0, 0, "查询msgchg_info失败,交易未开通, trancde=%s", pep_jnls.trancde);
		SaveErTradeInfo(RET_CODE_FB, currentDate, "查询msgchg_info失败,交易未开通", mer_info_t, pep_jnls, trace);
		BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
	else
	{	
		if(memcmp(mer_info_t.mgr_tpdu+2, "0726", 4)== 0)/*0726只给我们自己生产测试用的TPDU头中的目的地址*/
		{
			dcs_log(0, 0, "内部生产测试不走交易关闭的风控");
		}
		else
		{
			if(memcmp(msginfo.flagpe, "Y", 1)!=0 && memcmp(msginfo.flagpe, "y", 1)!= 0)
			{
				dcs_log(0, 0, "由于系统或者其他原因，交易暂时关闭");
				SaveErTradeInfo(RET_TRADE_CLOSED, currentDate, "交易暂时关闭", mer_info_t, pep_jnls, trace);
				BackToPos(RET_TRADE_CLOSED, siso, pep_jnls, databuf, mer_info_t);
				return -1;
			}
		}
	}		
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso, 53, (unsigned char *)tmpbuf);
	if(s > 0 && memcmp(tmpbuf+2, "1", 1) == 0)
	{
		s = PosTrackDecrypt(mer_info_t, &siso);
		if(s < 0)
		{
			dcs_log(0, 0, "磁道解密错误");
		 	SaveErTradeInfo("F3", currentDate, "磁道解密错误", mer_info_t, pep_jnls, trace);
			BackToPos("F3", siso, pep_jnls, databuf, mer_info_t);
			return -1;
		}
	}
	if(memcmp(pep_jnls.trancde, "032", 3) == 0 ||memcmp(pep_jnls.trancde, "031", 3)==0)
	{
		if(memcmp(pep_jnls.trancde, "031", 3)==0)
		{
			dcs_log(0, 0, "预授权撤销交易");
			if((mer_info_t.tag26&8192)== 0)
			{
				dcs_log(0, 0, "暂不支持该交易");
				SaveErTradeInfo("58", currentDate, "暂不支持该交易", mer_info_t, pep_jnls, trace);
				BackToPos("58", siso, pep_jnls, databuf, mer_info_t);
				return -1;
			}
			dcs_log(0, 0, "预授权撤销取原笔交易");
			rtn = GetPreAuthOriTrans(siso, &pep_jnls, mer_info_t);
			memcpy(pep_jnls.termcde, mer_info_t.mgr_term_id, 8);
			memcpy(pep_jnls.postermcde, mer_info_t.mgr_term_id, 8);
			memcpy(pep_jnls.process, "200000", 6);
		}
		else
		{
			dcs_log(0, 0, "预授权完成交易处理");
#ifdef __LOG__
			dcs_log(0, 0, "mer_info_t.tag26 =[%d]", mer_info_t.tag26);
#endif
			if((mer_info_t.tag26&4096)== 0)
			{
				dcs_log(0, 0, "暂不支持预授权完成交易");
				SaveErTradeInfo("58", currentDate, "暂不支持该交易", mer_info_t, pep_jnls, trace);
				BackToPos("58", siso, pep_jnls, databuf, mer_info_t);
				return -1;
			}
			dcs_log(0, 0, "预授权完成交易取原笔");
			rtn = GetPreAuthOriTrans(siso, &pep_jnls, mer_info_t);
			memcpy(pep_jnls.termcde, mer_info_t.mgr_term_id, 8);
			memcpy(pep_jnls.postermcde, mer_info_t.mgr_term_id, 8);
			memcpy(pep_jnls.process, "000000", 6);
		}
		setbit(&siso, 0, (unsigned char *)msginfo.msgtype, 4);
		setbit(&siso, 25, (unsigned char *)msginfo.service, 2);
		setbit(&siso, 3, (unsigned char *)msginfo.process, 6);
#ifdef __LOG__
		dcs_log(0, 0, "GetPreAuthOriTrans rtn=%d", rtn);
#endif
	}
	else if(memcmp(pep_jnls.trancde, "001", 3) == 0 ||memcmp(pep_jnls.trancde, "033", 3)==0)
	{
		if(memcmp(pep_jnls.trancde, "001", 3) == 0)
		{
			dcs_log(0, 0, "消费撤销交易");
			if((mer_info_t.tag26&512)== 0)
			{
				dcs_log(0, 0, "暂不支持该交易");
				SaveErTradeInfo("58", currentDate, "暂不支持该交易", mer_info_t, pep_jnls, trace);
				BackToPos("58", siso, pep_jnls, databuf, mer_info_t);
				return -1;
			}
			dcs_log(0, 0, "消费撤销取原笔交易");
		}
		else if(memcmp(pep_jnls.trancde, "033", 3) == 0)
		{
			dcs_log(0, 0, "预授权完成撤销交易");
			if((mer_info_t.tag26&2048)== 0)
			{
				dcs_log(0, 0, "暂不支持预授权完成撤销交易");
				SaveErTradeInfo("58", currentDate, "暂不支持该交易", mer_info_t, pep_jnls, trace);
				BackToPos("58", siso, pep_jnls, databuf, mer_info_t);
				return -1;
			}
			setbit(&siso, 0, (unsigned char *)msginfo.msgtype, 4);
			setbit(&siso, 25, (unsigned char *)msginfo.service, 2);
			setbit(&siso, 3, (unsigned char *)msginfo.process, 6);
			dcs_log(0, 0, "预授权完成撤销取原笔交易");
		}
		rtn = GetOriTrans(siso, &pep_jnls, mer_info_t, currentDate);
		memcpy(pep_jnls.process, "200000", 6);
#ifdef __LOG__
		dcs_log(0, 0, "GetOriTrans rtn=%d", rtn);
#endif
	}
	else if(memcmp(pep_jnls.trancde, "002", 3) == 0 || memcmp(pep_jnls.trancde, "051", 3) == 0)
	{
		if(memcmp(pep_jnls.trancde, "002", 3) == 0)
		{
			dcs_log(0, 0, "消费退货交易");
			if((mer_info_t.tag26&256)== 0)
			{
				dcs_log(0, 0, "暂不支持该交易");
				SaveErTradeInfo("58", currentDate, "暂不支持该交易", mer_info_t, pep_jnls, trace);
				BackToPos("58", siso, pep_jnls, databuf, mer_info_t);
				return -1;
			}
			setbit(&siso, 0, (unsigned char *)"0200", 2);
			setbit(&siso, 3, (unsigned char *)"270000", 6);
			dcs_log(0, 0, "消费退货取原笔交易");
		}
		else if(memcmp(pep_jnls.trancde, "051", 3) == 0)
		{
			dcs_log(0, 0, "脱机退货交易");
			if((mer_info_t.tag26&4)== 0)
			{
				dcs_log(0, 0, "暂不支持脱机退货交易");
				SaveErTradeInfo("58", currentDate, "暂不支持脱机退货交易", mer_info_t, pep_jnls, trace);
				BackToPos("58", siso, pep_jnls, databuf, mer_info_t);
				return -1;
			}
			setbit(&siso, 0, (unsigned char *)msginfo.msgtype, 4);
			setbit(&siso, 3, (unsigned char *)msginfo.process, 6);
			setbit(&siso, 25, (unsigned char *)msginfo.service, 2);
			dcs_log(0, 0, "脱机退货取原笔交易");
		}
		setbit(&siso, 63, (unsigned char *)NULL, 0);
		rtn = GetOriTransRtn(siso, &pep_jnls);
		memcpy(pep_jnls.termcde, mer_info_t.mgr_term_id, 8);
		memcpy(pep_jnls.postermcde, mer_info_t.mgr_term_id, 8);
		memcpy(pep_jnls.process, "200000", 6);
#ifdef __LOG__
		dcs_log(0, 0, "GetOriTransRtn rtn=%d", rtn);
#endif
		memset(tmpbuf, 0, sizeof(tmpbuf));
		s = getbit(&siso, 35, (unsigned char *)tmpbuf);
		if(s > 0)
		{
			for(n=0; n<20; n++)
			{
				if(tmpbuf[n] == '=' )
					break;
			}
			memcpy(pep_jnls.outcdno, tmpbuf, n);
		}
	}
	else if(memcmp(pep_jnls.trancde, "04", 2)==0 && memcmp(pep_jnls.trancde, "046", 3)!=0)
	{
		dcs_log(0, 0, "脚本通知取原笔交易");
		s = getbit(&siso, 3, (unsigned char *)pep_jnls.process);
		if(s <= 0)
		{
			dcs_log(0, 0, "get bit_3 error!");  
			BackToPos("H9", siso, pep_jnls, databuf, mer_info_t);
			return -1;
		}
		rtn = GetOriTransRtnIC(siso, &pep_jnls);
		if(rtn != 0)
		{
			dcs_log(0,0,"查找原笔交易流水失败");
			SaveErTradeInfo("25", currentDate, "查找原笔交易流水失败", mer_info_t, pep_jnls, trace);
			BackToPos("25", siso, pep_jnls, databuf, mer_info_t);
			return -1;
        }
     
        setbit(&siso, 3, (unsigned char *)msginfo.process, 6);
		setbit(&siso, 25, (unsigned char *)msginfo.service, 2);
        setbit(&siso, 41, (unsigned char *)pep_jnls.termcde, 8);
		setbit(&siso, 42, (unsigned char *)pep_jnls.mercode, 15);
		if(getstrlen(pep_jnls.tranamt)!=0)
			setbit(&siso, 4, (unsigned char *)pep_jnls.tranamt, 12);
	}
	else if(memcmp(pep_jnls.trancde, "000", 3) == 0 || memcmp(pep_jnls.trancde, "M01", 3) == 0
	|| memcmp(pep_jnls.trancde, "050", 3) == 0 
	||memcmp(pep_jnls.trancde, "010", 3) == 0 || memcmp(pep_jnls.trancde, "D01", 3) == 0 
	|| memcmp(pep_jnls.trancde, "D02", 3) == 0 || memcmp(pep_jnls.trancde, "030", 3) == 0)
	{
		if(memcmp(pep_jnls.trancde, "000", 3) == 0 || memcmp(pep_jnls.trancde, "M01", 3) == 0 || memcmp(pep_jnls.trancde, "050", 3) == 0)
		{
//			dcs_log(0, 0, "消费类的交易处理");
#ifdef __LOG__
			dcs_log(0, 0, "mer_info_t.tag26 =[%d]", mer_info_t.tag26);
#endif
			if( memcmp(pep_jnls.trancde, "050", 3) == 0 )
			{
				if((mer_info_t.tag26&8)== 0)
				{
					dcs_log(0, 0, "暂不支持脱机消费交易");
					SaveErTradeInfo("58", currentDate, "暂不支持脱机消费交易", mer_info_t, pep_jnls, trace);
					BackToPos("58", siso, pep_jnls, databuf, mer_info_t);
					return -1;
				}
				setbit(&siso, 63, (unsigned char *)NULL, 0);
		#ifdef __LOG__
				dcs_log(0, 0, "解析55域TC值信息9F26");
		#endif
				s = getbit(&siso, 55, (unsigned char *)pep_jnls.filed55);
				if(s <= 0)
				{
				 	dcs_log(0, 0, "bit55 is null!");	
				}
				else
				{	
					#ifdef __LOG__
						dcs_debug(pep_jnls.filed55, s, "解析TC值得55域内容:");
					#endif
					rtn = analyse55(pep_jnls.filed55, s, stFd55_IC, 34);
					if(rtn < 0 )
					{
						dcs_log(0, 0, "解析55域失败,解析结果[%d]", rtn);
						return -1;
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
			}	
			else
			{
				if((mer_info_t.tag26&1024)== 0)
				{
					dcs_log(0, 0, "暂不支持联机消费交易");
					SaveErTradeInfo("58", currentDate, "暂不支持联机消费交易", mer_info_t, pep_jnls, trace);
					BackToPos("58", siso, pep_jnls, databuf, mer_info_t);
					return -1;
				}		
				
				memset(tmpbuf, 0, sizeof(tmpbuf));
				s = getbit(&siso, 63, (unsigned char *)tmpbuf);
				if(s>0 && memcmp(tmpbuf, "EWM", 3)==0)
				{
					//配合demo演示，直接返回成功，并通知商城
					dcs_log(0, 0, "演示直接返回");
					BackToPos("00", siso, pep_jnls, databuf, mer_info_t);
					Analy2code(tmpbuf+3, s-3, trace, currentDate);
					return 0;
					//二维码支付，解析二维码信息,解析出二维码信息并保存
					/*
					rtn = Analy2code(tmpbuf+3, s-3, trace, currentDate);
					if(rtn < 0)
					{
						dcs_log(0, 0, "解析并保存二维码信息失败");
						SaveErTradeInfo("96", currentDate, "解析并保存二维码信息失败", mer_info_t, pep_jnls, trace);
						BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
						return -1;
					}
					*/
					setbit(&siso, 63, (unsigned char *)NULL, 0);	
				}
			}
//			setbit(&siso, 3, (unsigned char *)"910000", 6);
			memcpy(pep_jnls.process, "000000", 6);
			setbit(&siso, 0, (unsigned char *)msginfo.msgtype, 4);
			setbit(&siso, 25, (unsigned char *)msginfo.service, 2);
			setbit(&siso, 3, (unsigned char *)msginfo.process, 6);
		}
		else if(memcmp(pep_jnls.trancde, "030", 3) == 0)
		{
//			dcs_log(0, 0, "预授权类的交易处理");
			#ifdef __LOG__
				dcs_log(0, 0, "mer_info_t.tag26 =[%d]", mer_info_t.tag26);
			#endif

			if((mer_info_t.tag26&16384)== 0)
			{
				dcs_log(0, 0, "暂不支持该交易");
				SaveErTradeInfo("58", currentDate, "暂不支持该交易", mer_info_t, pep_jnls, trace);
				BackToPos("58", siso, pep_jnls, databuf, mer_info_t);
				return -1;
			}
			setbit(&siso, 0, (unsigned char *)msginfo.msgtype, 4);
			setbit(&siso, 25, (unsigned char *)msginfo.service, 2);
			setbit(&siso, 3, (unsigned char *)msginfo.process, 6);
			memcpy(pep_jnls.process, "030000", 6);
		}
		else if(memcmp(pep_jnls.trancde, "010", 3) == 0 || memcmp(pep_jnls.trancde, "D01", 3) == 0 
		|| memcmp(pep_jnls.trancde, "D02", 3) == 0)
		{
			setbit(&siso, 41, (unsigned char *)msginfo.termcde, 8);
			setbit(&siso, 42, (unsigned char *)msginfo.mercode, 15);
			setbit(&siso, 25, (unsigned char *)msginfo.service, 2);
			setbit(&siso, 3, (unsigned char *)msginfo.process, 6);
			if(memcmp(pep_jnls.trancde, "D01", 3) == 0)
			{
				dcs_log(0, 0, "积分消费的交易处理");
				memcpy(pep_jnls.process, "000000", 6);
			}
			else if(memcmp(pep_jnls.trancde, "010", 3) == 0)
			{
//				dcs_log(0, 0, "余额查询交易处理");
				if((mer_info_t.tag26&32768)== 0)
				{
					dcs_log(0, 0, "暂不支持该交易");
					SaveErTradeInfo("58", currentDate, "暂不支持该交易", mer_info_t, pep_jnls, trace);
					BackToPos("58", siso, pep_jnls, databuf, mer_info_t);
					return -1;
				}
				memcpy(pep_jnls.process, "310000", 6);
			}
			else
			{
				dcs_log(0, 0, "积分查询的处理");
				memcpy(pep_jnls.process, "310000", 6);
			}
		}
		memset(tmpbuf, 0, sizeof(tmpbuf));
		s = getbit(&siso, 35, (unsigned char *)tmpbuf);
		if(s > 0)
		{
			for(n=0; n<19; n++)
			{
				if(tmpbuf[n] == '=' )
					break;
			}
			memcpy(pep_jnls.outcdno, tmpbuf, n);
			rtn = 0 ;
		}
		else
			rtn = -2;
	}
	s = getstrlen(pep_jnls.outcdno);
	if(s > 0)
	{
#ifdef __LOG__
		dcs_log(0, 0, "有二磁道");
#endif
		setbit(&siso, 2, (unsigned char *)pep_jnls.outcdno, s);
	}
	else 
	{
#ifdef __LOG__
		dcs_log(0, 0, "无二磁道");
#endif
		memset(pep_jnls.outcdno, 0, sizeof(pep_jnls.outcdno));
		s = getbit(&siso, 2, (unsigned char *)pep_jnls.outcdno);
	}
	if(s > 0)
	{
		pep_jnls.outcdno[s] = '\0';
		outCardType = ReadCardType(pep_jnls.outcdno);
		pep_jnls.outcdtype = outCardType;	
	}
	else
	{
		dcs_log(0, 0, "无主账号信息");
	}
	
	s = GetBit_fiso(siso, &pep_jnls);
	if(s < 0)
	{
		dcs_log(0, 0, "get bit_20 21 error!");
		return -1;
	}
	s = getbit(&siso, 11, (unsigned char *)pep_jnls.termtrc);
	if(s <= 0)
	{
		dcs_log(0, 0, "get bit_11 error!");
		return -1;
	}
	setbit(&siso, 7, (unsigned char *)databuf, 10);
	setbit(&siso, 11, (unsigned char *)trace, 6);			
	if(rtn == -1)
	{
		dcs_log(0, 0, "错误:   查找原笔交易失败");
		setbit(&siso, 39, (unsigned char *)RET_CODE_F0, 2);
	 	WritePosXpep(siso, pep_jnls, databuf);
		BackToPos(RET_CODE_F0, siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
	else if(rtn == -3 && (memcmp(pep_jnls.trancde, "002", 3) == 0 || memcmp(pep_jnls.trancde, "051", 3) == 0))
	{
		dcs_log(0, 0, "错误:   退货金额超限");
		setbit(&siso, 39, (unsigned char *)"61", 2);
	 	WritePosXpep(siso, pep_jnls, databuf);
		BackToPos("61", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
	else if(rtn == -2 && memcmp(pep_jnls.trancde, "D01", 3) != 0 
					&& memcmp(pep_jnls.trancde, "D02", 3) != 0 
					&& memcmp(pep_jnls.trancde, "050", 3) != 0
					&& memcmp(pep_jnls.trancde, "051", 3) != 0)
	{
		dcs_log(0, 0, "错误:   无二磁道");
		setbit(&siso, 39, (unsigned char *)"56", 2);
	 	WritePosXpep(siso, pep_jnls, databuf);
		BackToPos("56", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
	else if(rtn == -4 && memcmp(pep_jnls.trancde, "032", 3) == 0)
	{
		dcs_log(0, 0, "错误:   预授权完成请求金额超限");
		setbit(&siso, 39, (unsigned char *)"61", 2);
	 	WritePosXpep(siso, pep_jnls, databuf);
		BackToPos("61", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}			
	s = getbit(&siso, 22, (unsigned char *)bit_22);
	if(s <= 0)
	{
		dcs_log(0, 0, "get bit_22 error!");
		BackToPos("H9", siso, pep_jnls, databuf, mer_info_t);	
		return -1;
	}
	
	#ifdef __LOG__
		dcs_log(0, 0, "bit_22-11=[%s]", bit_22);
		dcs_log(0, 0, "mer_info_t.track_pwd_flag-11=[%s]", mer_info_t.track_pwd_flag);
		dcs_log(0, 0, "pep_jnls.trancde-11=[%s]", pep_jnls.trancde);
	#endif
		
	if(memcmp(bit_22, "02", 2) != 0 && memcmp(bit_22, "05", 2) != 0 && memcmp(bit_22, "95", 2) != 0 && memcmp(bit_22, "07", 2) != 0 && memcmp(bit_22, "96", 2) != 0
	&&(memcmp(pep_jnls.trancde, "000", 3)==0 || memcmp(pep_jnls.trancde, "M01", 3)==0|| memcmp(pep_jnls.trancde, "010", 3)==0))
	{
		dcs_log(0, 0, "错误:   不是刷卡交易，暂时不受理");
		setbit(&siso, 39, (unsigned char *)"HC", 2);
	 	WritePosXpep(siso, pep_jnls, databuf);
		BackToPos("HC", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
	/*消费撤销和脚本通知,TC值上送没有52域*/
	if(memcmp(bit_22+2, "1", 1) == 0&&memcmp(pep_jnls.trancde, "001", 3) != 0
	&&memcmp(pep_jnls.trancde, "04", 2)!=0 )
	{	
		memset(tmpbuf, 0, sizeof(tmpbuf));
		s = getbit(&siso, 52, (unsigned char *)tmpbuf);
		if(s <= 0)
		{
			dcs_log(0, 0, "get bit_52 error!");
			BackToPos("H9", siso, pep_jnls, databuf, mer_info_t);	
			return -1;
		}
		#ifdef __TEST__
			dcs_debug(tmpbuf, s, "终端报文52域:");
		#endif
		if(memcmp(pep_jnls.trancde, "D01", 3)== 0 || memcmp(pep_jnls.trancde, "D02", 3)== 0)
			s = PosPinConvert(mer_info_t.mgr_pik, "a596ee13f9a93800", "9999990010020030040", tmpbuf, outPin);
		else
			s = PosPinConvert(mer_info_t.mgr_pik, "a596ee13f9a93800", pep_jnls.outcdno, tmpbuf, outPin);
		if(s == 1)
		{
			#ifdef __TEST__
				dcs_debug(outPin, 8, "设置发送核心系统的报文52域:");
			#endif
			setbit(&siso, 52, (unsigned char *)outPin, 8);
			/*
			if(memcmp(pep_jnls.trancde, "03", 2)!= 0)
				setbit(&siso, 53,(unsigned char *)"1000000000000000", 16);*/
			memset(tmpbuf, 0x00, sizeof(tmpbuf));
			s = getbit(&siso, 53, (unsigned char *)tmpbuf);
			if(s>0 && memcmp(pep_jnls.trancde, "03", 2) == 0)
			{
				memcpy(tmpbuf+2, "0", 1);
		#ifdef __LOG__
				dcs_log(0, 0, "tmpbuf=[%s]", tmpbuf);
		#endif
				setbit(&siso, 53, (unsigned char *)tmpbuf, 16);
			}
			else
				setbit(&siso, 53, (unsigned char *)"1000000000000000", 16);
		}
		else
		{
			dcs_log(0, 0, "pin转加密失败。");
			setbit(&siso, 39, (unsigned char *)RET_CODE_F7, 2);
	 		WritePosXpep(siso, pep_jnls, databuf);
			BackToPos(RET_CODE_F7, siso, pep_jnls, databuf, mer_info_t);
			return -1;
		}	
	}
	else
	{
		if(memcmp(pep_jnls.trancde, "001", 3)== 0 && memcmp(bit_22+2, "1", 1) == 0)
		{
			memcpy(bit_22+2, "2", 1);
			setbit(&siso, 22,(unsigned char *)bit_22, 3);
		}
		setbit(&siso, 26,(unsigned char *)NULL, 0);
		setbit(&siso, 52,(unsigned char *)NULL, 0);
		setbit(&siso, 53,(unsigned char *)NULL, 0);
	}	
			
	rtn = GetChannelRestrict(mer_info_t, pep_jnls.trancde);
	#ifdef __LOG__
		dcs_log(0, 0, "rtn:[%d]", rtn);
	#endif
	if(rtn < 0)
	{
		dcs_log(0,0, "错误:   渠道限制交易，交易不受理");
		setbit(&siso, 39, (unsigned char *)"FE", 2);
	 	WritePosXpep(siso, pep_jnls, databuf);
		BackToPos("FE", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
	if(memcmp(pep_jnls.trancde, "D01", 3)!=0 && memcmp(pep_jnls.trancde, "D02", 3)!=0)	
	{
		memset(tmpbuf, 0x00, sizeof(tmpbuf));
		s = getbit(&siso, 60, (unsigned char *)tmpbuf);
		if(s <= 0)
		{
			dcs_log(0, 0, "取终端交易批次号错误");
			setbit(&siso, 39, (unsigned char *)RET_CODE_H9, 2);
	 		WritePosXpep(siso, pep_jnls, databuf);
			BackToPos(RET_CODE_H9, siso, pep_jnls, databuf, mer_info_t);
			return -1;
		}
		memcpy(pep_jnls.batch_no, tmpbuf+2, 6);
	}
	else
	{
		memcpy(pep_jnls.batch_no, "000000", 6);
	}
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	sprintf(tmpbuf, "%s%s%s%s%s", mer_info_t.mgr_mer_id, mer_info_t.mgr_term_id, "3", "03", pep_jnls.trancde);
	sprintf(tmpbuf+29, "%s%s%s%04d", pep_jnls.termtrc, pep_jnls.batch_no, pep_jnls.process, rtn);

/*--- modify begin 140325 ---*/	
	if(getstrlen(mer_info_t.T0_flag)==0)
		memcpy(mer_info_t.T0_flag,"0",1);
	if(getstrlen(mer_info_t.agents_code)==0)
		memcpy(mer_info_t.agents_code,"000000",6);
	sprintf(tmpbuf+51, "%s", mer_info_t.T0_flag);
	sprintf(tmpbuf+52, "%s", mer_info_t.agents_code);
	setbit(&siso, 21, (unsigned char *)tmpbuf, 58);

	#ifdef __LOG__
		dcs_log(0, 0, "21域信息%s",tmpbuf);
	#endif

	memset(tmpbuf, 0, sizeof(tmpbuf));
	if(memcmp(pep_jnls.trancde, "D01", 3)!=0 && memcmp(pep_jnls.trancde, "D02", 3)!=0)	
	{
		s=getbit(&siso, 60, (unsigned char *)tmpbuf);
		if( s<13 )
			memcpy(tmpbuf, "20", 2);
		else 
			memcpy(tmpbuf,tmpbuf+11,2);//取终端上送的60.4,60.5送给核心
		memcpy(tmpbuf+2,"03", 2);//终端类型
		tmpbuf[4]='\0';
		setbit(&siso, 60, (unsigned char *)tmpbuf, 4);
	}
	else
	{
		setbit(&siso, 60, (unsigned char *)"00000000030000000000", 20);
	}
	
	//setbit(&siso, 60, (unsigned char *)"00000000030000000000", 20);
	
	if(memcmp(pep_jnls.trancde, "001", 3) == 0||memcmp(pep_jnls.trancde, "002", 3) == 0
	|| memcmp(pep_jnls.trancde, "051", 3) == 0|| memcmp(pep_jnls.trancde, "031", 3) == 0 
	|| memcmp(pep_jnls.trancde, "032", 3) == 0 || memcmp(pep_jnls.trancde, "033", 3) == 0 ) 
	{
		memset(tmpbuf, 0, sizeof(tmpbuf));
		if(getstrlen(pep_jnls.authcode) != 6)
			sprintf(tmpbuf, "%s%s%s", "000000", pep_jnls.syseqno, pep_jnls.revdate);
		else
			sprintf(tmpbuf, "%s%s%s", pep_jnls.authcode, pep_jnls.syseqno, pep_jnls.revdate); //消费撤销、消费撤销冲正和消费退款的交易授权码从pep_jnls中获取

	#ifdef __LOG__
		dcs_log(0, 0, "tmpbuf 61=[%s]", tmpbuf);
	#endif
		setbit(&siso, 61, (unsigned char *)tmpbuf, 28);//原交易授权号	N6参考号	N12时间N10
	}
	
	if(memcmp(pep_jnls.trancde, "04", 2)==0 && memcmp(pep_jnls.trancde, "046", 3)!=0)
	{
		memset(tmpbuf, 0, sizeof(tmpbuf));
		if(getstrlen(pep_jnls.authcode) != 6)
			sprintf(tmpbuf, "%s%s%s%06d", "000000", pep_jnls.syseqno, pep_jnls.revdate, pep_jnls.trace);
		else
			sprintf(tmpbuf, "%s%s%s%06d", pep_jnls.authcode, pep_jnls.syseqno, pep_jnls.revdate, pep_jnls.trace); //消费撤销、消费撤销冲正和消费退款的交易授权码从pep_jnls中获取

	#ifdef __LOG__
		dcs_log(0, 0, "szTmp=[%s]",tmpbuf);
	#endif
		setbit(&siso, 61, (unsigned char *)tmpbuf, 34);//原交易授权号	N6参考号	N12时间N10
	
	}
	
/*
	if(memcmp(bit_22, "05", 2) == 0 || memcmp(bit_22, "95", 2) == 0 )
    {  
        s = getbit(&siso, 55,(unsigned char *)temp_bcd_55);
		if(s <= 0)
		 	dcs_log(0, 0, "bit55 is null!");	
		else
		{
			bcd_to_asc((unsigned char *)temp_asc_55, (unsigned char *)temp_bcd_55, 2*s, 0);
		}
		
		dcs_log(0, 0, "****szTmp=[%s]",temp_asc_55);
        setbit(&siso, 55, (unsigned char *)temp_asc_55, strlen(temp_asc_55));
    }   
*/		
	//脚本上送通知保存在管理类信息表中
	if(memcmp(pep_jnls.trancde, "04", 2)==0 && memcmp(pep_jnls.trancde, "046", 3)!=0)
	{
		sscanf(trace, "%06d", &pep_jnls.trace);
		rtn = Pos_SavaManageInfo(siso, pep_jnls, currentDate);
		if(rtn == -1)
		{
			dcs_log(0, 0, "保存管理类信息，POS脚本上送通知交易失败");
		}
		else
			dcs_log(0, 0, "保存管理类信息，POS脚本上送通知交易成功");
		
	}
	else
	{
	/*保存pep_jnls数据库*/
		rtn = WritePosXpep(siso, pep_jnls, databuf);
		if(rtn == -1)
		{
			dcs_log(0, 0, "保存pep_jnls数据失败");
			SaveErTradeInfo("FO", currentDate, "保存pep_jnls数据失败", mer_info_t, pep_jnls, trace);
			BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
			return -1;
		}	
	}
	
	setbit(&siso, 20, (unsigned char *)NULL, 0);
	setbit(&siso, 64, (unsigned char *)NULL, 0);
	if(strlen(card_sn) ==3)
		setbit(&siso, 23, (unsigned char *)card_sn, 3);

/*  配合核心测试，生产应当删除
	#ifdef __TEST__
	if(strlen(card_sn) ==3)
	{
		setbit(&siso, 2, (unsigned char *)"6217582000004762449", 19);
		setbit(&siso, 23, (unsigned char *)"001", 3);
		setbit(&siso, 35, (unsigned char *)"6217582000004762449=25032201000090300", 37);
		sprintf(bit_55, "9F26087642529E5AE299069F2701809F101307020103A0A002010A010000000000DD958F599F3704B83F6DF29F3602004D9505002004E0009A031411149C01009F02060000000000015F2A02015682027C009F1A0201569F03060000000000009F3303E0F9C89F34030203009F3501229F1E0838313136383435338408A0000003330101019F090200209F410400000635", 290);
		asc_to_bcd(bit_55_bcd, bit_55, 290, 0);
		setbit(&siso, 55, (unsigned char *)bit_55_bcd, 145);

	}
	#endif
*/
    if(memcmp(pep_jnls.trancde, "050", 3) ==0)
	{
	   memset(g_key, 0, sizeof(g_key));
       sprintf(g_key,"%14s%6s",currentDate,trace);
	}

	/*发送报文*/
	s = WriteXpe(siso);
	if(s == -1)
	{
		dcs_log(0, 0, "发送信息给核心失败");
		SaveErTradeInfo(RET_CODE_F9, currentDate, "发送信息给核心失败", mer_info_t, pep_jnls, trace);
		BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
	dcs_log(0, 0, "数据发送成功[%d]",s);
	return 0;
}

/*初步终端上送的信用卡还款、ATM转账和跨行转账的交易*/
int ProTransfer(ISO_data siso, PEP_JNLS pep_jnls, MER_INFO_T mer_info_t, char *currentDate)
{
	char tmpbuf[127], trace[6+1], outPin[8+1], bit_22[3+1], databuf[10+1], bcd_tmp[2+1], asc_tmp[4+1];
	int s, rtn, n, len;
	char outCardType;
	long transfee;
	MSGCHGINFO	msginfo;
	char feeformula[25+1];
	STR_FEE str_fee;
	char rate[9], amtfee[9], desfee[9];
	unsigned char tmp;
	
	memset(feeformula, 0, sizeof(feeformula));
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(trace, 0, sizeof(trace));
	memset(outPin, 0, sizeof(outPin));
	memset(bit_22, 0, sizeof(bit_22));
	memset(databuf, 0, sizeof(databuf));
	memset(&msginfo, 0, sizeof(MSGCHGINFO));
	memset(&str_fee, 0, sizeof(STR_FEE));
	memset(rate, 0, sizeof(rate));
	memset(amtfee, 0, sizeof(amtfee));
	memset(desfee, 0, sizeof(desfee));
	memset(bcd_tmp, 0, sizeof(bcd_tmp));
	memset(asc_tmp, 0, sizeof(asc_tmp));
	
	memcpy(databuf, currentDate+4, 10);
	
	//取前置系统流水
	if( pub_get_trace_from_seq(trace) < 0)
	{
		dcs_log(0, 0, "取前置系统交易流水错误");
		SaveErTradeInfo(RET_CODE_F8, currentDate, "取前置系统交易流水错误", mer_info_t, pep_jnls, trace);
		BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
	
	setbit(&siso, 7, (unsigned char *)databuf, 10);
	setbit(&siso, 11, (unsigned char *)trace, 6);
	
	/*交易码来查询数据库msgchg_info来读取一些信息*/
	rtn = Readmsgchg(pep_jnls.trancde, &msginfo);
	if(rtn == -1)
	{
		dcs_log(0, 0, "查询msgchg_info失败,交易未开通, trancde=%s", pep_jnls.trancde);
		setbit(&siso, 39, (unsigned char *)RET_CODE_FB, 2);
		//WritePosXpep(siso, pep_jnls, databuf);
		BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
	else
	{	
		if(memcmp(mer_info_t.mgr_tpdu+2, "0726", 4)== 0)/*0726只给我们自己生产测试用的TPDU头中的目的地址*/
		{
			dcs_log(0, 0, "内部生产测试不走交易关闭的风控");
		}
		else
		{
			if(memcmp(msginfo.flagpe, "Y", 1)!=0 && memcmp(msginfo.flagpe, "y", 1)!= 0)
			{
				dcs_log(0, 0, "由于系统或者其他原因，交易暂时关闭");
				//WritePosXpep(siso, pep_jnls, databuf);
				BackToPos(RET_TRADE_CLOSED, siso, pep_jnls, databuf, mer_info_t);
				return -1; 
			}
		}
	}					
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso, 53, (unsigned char *)tmpbuf);
	if(s > 0 && memcmp(tmpbuf+2, "1", 1) == 0)
	{
		s = PosTrackDecrypt(mer_info_t, &siso);
		if(s < 0)
		{
			dcs_log(0, 0, "磁道解密错误");
			setbit(&siso, 39, (unsigned char *)"30", 2);
		 	//WritePosXpep(siso, pep_jnls, databuf);
			BackToPos("F3", siso, pep_jnls, databuf, mer_info_t);
			return -1;
		}
	}
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso, 35, (unsigned char *)tmpbuf);
	if(s > 0)
	{
		for(n=0; n<20; n++)
		{
			if(tmpbuf[n] == '=' )
				break;
		}
		memcpy(pep_jnls.outcdno, tmpbuf, n);
		rtn = 0 ;
	}
	else
		rtn = -1;
	
	if(rtn == -1)
	{
		dcs_log(0, 0, "无二磁道");
		setbit(&siso, 39, (unsigned char *)"56", 2);
	 	//WritePosXpep(siso, pep_jnls, databuf);
		BackToPos("56", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
	s = getstrlen(pep_jnls.outcdno);
	setbit(&siso, 2, (unsigned char *)pep_jnls.outcdno, s);
	pep_jnls.outcdno[s] = '\0';
	outCardType = ReadCardType(pep_jnls.outcdno);
	pep_jnls.outcdtype = outCardType;
	
	s = getbit(&siso, 22, (unsigned char *)bit_22);
	if(s <= 0)
	{
		dcs_log(0, 0, "get bit_22 error!");
		BackToPos("H9", siso, pep_jnls, databuf, mer_info_t);	
		return -1;
	}
	
	if(memcmp(bit_22, "02", 2) != 0 && memcmp(bit_22, "05", 2) != 0 && memcmp(bit_22, "95", 2) != 0  && memcmp(bit_22, "07", 2) != 0 && memcmp(bit_22, "96", 2) != 0)
	{
		dcs_log(0, 0, "不是刷卡交易，暂时不受理");
		setbit(&siso, 39, (unsigned char *)"HC", 2);
	 	//WritePosXpep(siso, pep_jnls, databuf);
		BackToPos("HC", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
	
	if(memcmp(bit_22+2, "1", 1) == 0)
	{	
		memset(tmpbuf, 0, sizeof(tmpbuf));
		s = getbit(&siso, 52, (unsigned char *)tmpbuf);
		if(s < 0)
		{
			dcs_log(0, 0, "get bit_52 error!");
			BackToPos("H9", siso, pep_jnls, databuf, mer_info_t);	
			return -1;
		}
		#ifdef __TEST__
			dcs_debug(tmpbuf, s, "终端报文52域:");
		#endif
		s = PosPinConvert(mer_info_t.mgr_pik, "a596ee13f9a93800", pep_jnls.outcdno, tmpbuf, outPin);
		if(s == 1)
		{
			#ifdef __TEST__
				dcs_debug(outPin, 8, "设置发送核心系统的报文52域:");
			#endif
			setbit(&siso, 52,(unsigned char *)outPin, 8);
			setbit(&siso, 53,(unsigned char *)"1000000000000000", 16);
		}
		else
		{
			dcs_log(0, 0, "pin转加密失败。");
			//setbit(&siso, 39, (unsigned char *)RET_CODE_F7, 2);
	 		//WritePosXpep(siso, pep_jnls, databuf);
			BackToPos(RET_CODE_F7, siso, pep_jnls, databuf, mer_info_t);
			return -1;
		}			
	}
	else
	{
		setbit(&siso, 26,(unsigned char *)NULL, 0);
		setbit(&siso, 52,(unsigned char *)NULL, 0);
		setbit(&siso, 53,(unsigned char *)NULL, 0);
	}		
	
	rtn = GetChannelRestrict(mer_info_t, pep_jnls.trancde);
	#ifdef __LOG__
		dcs_log(0, 0, "rtn:[%d]", rtn);
	#endif

	if(rtn < 0)
	{
		dcs_log(0,0, "渠道限制交易，交易不受理");
		setbit(&siso, 39, (unsigned char *)"FE", 2);
	 	//WritePosXpep(siso, pep_jnls, databuf);
		BackToPos("FE", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
	
	s = getbit(&siso,48,(unsigned char *)pep_jnls.modecde);
	if(s <= 0)
		 dcs_log(0,0,"bit_48 is null!");
	else
		memcpy(pep_jnls.modeflg, "1", 1);
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso, 46, (unsigned char *)tmpbuf);
	if(s <= 0)
	{
		dcs_log(0, 0, "转入卡号不存在！");
		return -1;
	}
	setbit(&siso, 20, (unsigned char *)tmpbuf, s);
	memcpy(pep_jnls.intcdno, tmpbuf, s);
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso, 63, (unsigned char *)tmpbuf);
	if(s <= 0)
	{
		dcs_log(0, 0, "get63 error！");
		return -1;
	}
	#ifdef __LOG__
		dcs_log(0, 0, "get63 [%s]", tmpbuf);
		dcs_debug(tmpbuf, s, "get63 ");
	#endif

	tmp = tmpbuf[3];
	len = ( tmp & 0x0F ) * 100;
	tmp = tmpbuf[4];
	len += (tmp & 0xF0) / 16 * 10 + ( tmp & 0x0F );

	#ifdef __LOG__
		dcs_log(0, 0, "get63 [%d]", len);
	#endif
	if(len>(s-3))
	{
		BackToPos("H9", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
	if(len!=0)
		memcpy(feeformula, tmpbuf+5, len);
	len = getstrlen(feeformula);
	//信用卡还款190000
	if(memcmp(pep_jnls.trancde, "017", 3) == 0)
	{	
		dcs_log(0, 0, "信用卡还款处理");
		if((mer_info_t.tag39&32768)==0)
		{
			dcs_log(0, 0, "暂不支持该交易");
			BackToPos("58", siso, pep_jnls, databuf, mer_info_t);
			return -1;
		}
		n = getstrlen(mer_info_t.creditfeeformula);
		if(len==0 || memcmp(feeformula, mer_info_t.creditfeeformula, len)!=0 || n!=len )
		{	
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy(tmpbuf, "CUP", 3);
			sprintf(asc_tmp, "%04d", n);
			asc_to_bcd((unsigned char *)bcd_tmp, (unsigned char*)asc_tmp, 4, 0);
			memcpy(tmpbuf+3, bcd_tmp, 2);
			/*tmpbuf[3]=((n>>8)&0xFF);
			tmpbuf[4]=(n&0xFF);*/
			memcpy(tmpbuf+5, mer_info_t.creditfeeformula, n);
			setbit(&siso, 63, (unsigned char *)tmpbuf, n+5);
			BackToPos("T5", siso, pep_jnls, databuf, mer_info_t);
			return -1;
		}
		
		setbit(&siso, 41, (unsigned char *)msginfo.termcde, 8);
		setbit(&siso, 42, (unsigned char *)msginfo.mercode, 15);
		memcpy(pep_jnls.process, "190000", 6);
		memset(tmpbuf, 0, sizeof(tmpbuf));
		sprintf(tmpbuf, "%s%s%s", "DY", currentDate+2, trace);
		setbit(&siso, 48, (unsigned char *)tmpbuf, 20);
		mer_info_t.creditfeeformula[n] = 0;
		Splitstr(mer_info_t.creditfeeformula, &str_fee);
		
		/*modify by hw at 20140924  信用卡还款不需要23,55域*/
		//setbit(&siso, 23, (unsigned char *)NULL, 0);
		//setbit(&siso, 55, (unsigned char *)NULL, 0);
	}
	//ATM跨行转账400000
	else if(memcmp(pep_jnls.trancde, "012", 3) == 0)
	{
		setbit(&siso, 41, (unsigned char *)"00000003", 8);
		setbit(&siso, 42, (unsigned char *)"400029000210013", 15);
		memcpy(pep_jnls.process, "400000", 6);
	}
	//转账汇款480000
	else if(memcmp(pep_jnls.trancde, "013", 3) == 0)
	{
		dcs_log(0, 0, "转账汇款处理");
		if((mer_info_t.tag39&16384)== 0)
		{
			dcs_log(0, 0, "暂不支持该交易");
			BackToPos("58", siso, pep_jnls, databuf, mer_info_t);
			return -1;
		}
		n =getstrlen(mer_info_t.remitfeeformula);
		if(len==0 || memcmp(feeformula, mer_info_t.remitfeeformula, len)!=0 || n!=len)
		{
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy(tmpbuf, "CUP", 3);
			/*tmpbuf[3]=((n>>8)&0xFF);
			tmpbuf[4]=(n&0xFF);*/
			sprintf(asc_tmp, "%04d", n);
			asc_to_bcd((unsigned char *)bcd_tmp, (unsigned char*)asc_tmp, 4, 0);
			memcpy(tmpbuf+3, bcd_tmp, 2);
			memcpy(tmpbuf+5, mer_info_t.remitfeeformula, n);
			setbit(&siso, 63, (unsigned char *)tmpbuf, n+5);
			#ifdef __LOG___
				dcs_log(0,0,"mer_info_t.remitfeeformula 11=[%s]", mer_info_t.remitfeeformula);
				dcs_log(0, 0, "tmpbuf = [%s]", tmpbuf);
  			#endif

			BackToPos("T5", siso, pep_jnls, databuf, mer_info_t);
			return -1;
		}
		mer_info_t.remitfeeformula[n] =0;
		Splitstr(mer_info_t.remitfeeformula, &str_fee);
		
		setbit(&siso, 41, (unsigned char *)msginfo.termcde, 8);
		setbit(&siso, 42, (unsigned char *)msginfo.mercode, 15);
		setbit(&siso, 25, (unsigned char *)msginfo.service, 2);
		
		memset(tmpbuf, 0, sizeof(tmpbuf));
		len = getstrlen(pep_jnls.billmsg); 
		if(len==0)
		{
			sprintf(pep_jnls.billmsg, "%s%s%s", "DY", currentDate+2, trace);
			len =getstrlen(pep_jnls.billmsg);
		}
		memcpy(tmpbuf, pep_jnls.billmsg, len);	
		memcpy(tmpbuf+len, "|", 1);	
		len += 1;			
		s = getbit(&siso, 59, (unsigned char *)(tmpbuf+len));
		if(s <= 0)
		{
			dcs_log(0, 0, "bit 59 卡主信息失败！");
			BackToPos("H9", siso, pep_jnls, databuf, mer_info_t);
			return -1;
		
		}
		setbit(&siso, 48, (unsigned char *)tmpbuf, len+s);
		memcpy(pep_jnls.process, "480000", 6);
	}
	#ifdef __TEST__
		dcs_log(0, 0, "str_fee.fee_flag =[%s] ", str_fee.fee_flag);
		dcs_log(0, 0, "str_fee.fee_min =[%s] ", str_fee.fee_min);
		dcs_log(0, 0, "str_fee.fee_max =[%s] ", str_fee.fee_max);
		dcs_log(0, 0, "str_fee.fee_r =[%s] ", str_fee.fee_r);
		dcs_log(0, 0, "str_fee.fee =[%s] ", str_fee.fee);
	#endif
	
	//bit 4 or bit 28
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso, 4, (unsigned char *)tmpbuf);
	if(s <= 0)
	{
		dcs_log(0, 0, "get bit4 error ");
		return -1;
	}
	memcpy(pep_jnls.tranamt, tmpbuf, 12);
	if(memcmp(str_fee.fee_flag, "MTM", 3)==0)
	{
		Decimal(str_fee.fee_r, rate);
		#ifdef __LOG___
			dcs_log(0, 0, "rate =[%s] ", rate);
		#endif
		multiply(rate, tmpbuf, amtfee);
		#ifdef __LOG___
			dcs_log(0, 0, "amtfee =[%s] ", amtfee);
		#endif
		GetFee(str_fee, amtfee, desfee);
		#ifdef __LOG___
		dcs_log(0, 0, "desfee =[%s] ", desfee);
		#endif
	}
	else if(memcmp(str_fee.fee_flag, "SGL", 3)==0)
	{
		#ifdef __TEST___
			dcs_log(0, 0, "SGL");
		#endif
		memset(tmpbuf, 0, sizeof(tmpbuf));
		Decimal(str_fee.fee, tmpbuf);
		#ifdef __LOG___
			dcs_log(0, 0, "tmpbuf =[%s] ", tmpbuf);
		#endif

		PubAddSymbolToStr(tmpbuf, 8, '0', 0);
		strcpy(desfee+1, tmpbuf);
		memcpy(desfee, "C", 1);
		#ifdef __LOG___
			dcs_log(0, 0, "desfee =[%s] ", desfee);
		#endif
	}
		
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso, 28, (unsigned char *)tmpbuf);
	if(s <= 0)
	{
		dcs_log(0, 0, "get bit28 error ");
		return -1;
	}
	memcpy(pep_jnls.filed28, tmpbuf, 9);
	if(memcmp(tmpbuf, desfee, 9)!=0)
	{
		dcs_log(0, 0, "两边计算的手续费不一致");
		if(memcmp(pep_jnls.trancde, "013", 3) == 0)
		{
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy(tmpbuf, "CUP", 3);
			n = getstrlen(mer_info_t.remitfeeformula);
			sprintf(asc_tmp, "%04d", n);
			asc_to_bcd((unsigned char *)bcd_tmp, (unsigned char*)asc_tmp, 4, 0);
			memcpy(tmpbuf+3, bcd_tmp, 2);
			memcpy(tmpbuf+5, mer_info_t.remitfeeformula, n);
			setbit(&siso, 63, (unsigned char *)tmpbuf, n+5);

			#ifdef __LOG___
				dcs_log(0, 0, "tmpbuf = [%s]", tmpbuf);
				dcs_log(0,0,"mer_info_t.remitfeeformula 22=[%s]", mer_info_t.remitfeeformula);
  			#endif
		}
		else if(memcmp(pep_jnls.trancde, "017", 3) == 0)
		{
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy(tmpbuf, "CUP", 3);
			n = getstrlen(mer_info_t.creditfeeformula);
			/*tmpbuf[3]=((n>>8)&0xFF);
			tmpbuf[4]=(n&0xFF);*/
			sprintf(asc_tmp, "%04d", n);
			asc_to_bcd((unsigned char *)bcd_tmp, (unsigned char*)asc_tmp, 4, 0);
			memcpy(tmpbuf+3, bcd_tmp, 2);
			memcpy(tmpbuf+5, mer_info_t.creditfeeformula, n);
			setbit(&siso, 63, (unsigned char *)tmpbuf, n+5);
			#ifdef __LOG___
				dcs_log(0, 0, "tmpbuf = [%s]", tmpbuf);
  			#endif
		}
		BackToPos("T5", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
	
	memset(tmpbuf, '0', sizeof(tmpbuf));
	tmpbuf[12] = 0;
	s = SumAmt(pep_jnls.tranamt, desfee+1, tmpbuf);
	if(s == -1)
	{
		dcs_log(0, 0, "求和错误");
		return -1;
	}	
	#ifdef __LOG___
		dcs_log(0, 0, "tmpbuf = [%s]", tmpbuf);
	#endif
	setbit(&siso, 4, (unsigned char *)tmpbuf, 12);
	memcpy(pep_jnls.tranamt, tmpbuf, 12);
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	sprintf(tmpbuf, "%s%s%s%s%s", mer_info_t.mgr_mer_id, mer_info_t.mgr_term_id, "3", "03", pep_jnls.trancde);
	sprintf(tmpbuf+29, "%s%s%s%04d", pep_jnls.termtrc, "000000", pep_jnls.process, rtn);

/*--- modify begin 140325 ---*/	
	if(getstrlen(mer_info_t.T0_flag)==0)
		memcpy(mer_info_t.T0_flag,"0",1);
	if(getstrlen(mer_info_t.agents_code)==0)
		memcpy(mer_info_t.agents_code,"000000",6);
	sprintf(tmpbuf+51, "%s", mer_info_t.T0_flag);
	sprintf(tmpbuf+52, "%s", mer_info_t.agents_code);
	setbit(&siso, 21, (unsigned char *)tmpbuf, 58);
/*--- end by hw ---*/
	
		
/*--- setbit(&siso, 60, (unsigned char *)"00000000030000000000", 20); ---*/
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s=getbit(&siso, 60, (unsigned char *)tmpbuf);
	if( s<13 )
		memcpy(tmpbuf, "20", 2);
	else 
	{
		memcpy(pep_jnls.batch_no,tmpbuf+2,6);
		memcpy(tmpbuf,tmpbuf+11,2);//取终端上送的60.4,60.5送给核心
	}
	memcpy(tmpbuf+2,"03", 2);//终端类型
	tmpbuf[4]='\0';
	setbit(&siso, 60, (unsigned char *)tmpbuf, 4);
	
	/*保存pep_jnls数据库*/
	rtn = WritePosXpep(siso, pep_jnls, databuf);
	if(rtn == -1)
	{
		dcs_log(0, 0, "保存pep_jnls数据失败");
		SaveErTradeInfo("FO", currentDate, "保存pep_jnls数据失败", mer_info_t, pep_jnls, trace);
		setbit(&siso, 4, (unsigned char *)pep_jnls.tranamt, 12);
		BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}	
	if(memcmp(pep_jnls.trancde, "D01", 3)==0||memcmp(pep_jnls.trancde, "D02", 3)==0)
	{
		setbit(&siso, 20, (unsigned char *)NULL, 0);
	}	
	setbit(&siso, 64, (unsigned char *)NULL, 0);
	setbit(&siso, 45, (unsigned char *)NULL, 0);
	setbit(&siso, 46, (unsigned char *)NULL, 0);
	setbit(&siso, 26, (unsigned char *)NULL, 0);
	setbit(&siso, 63, (unsigned char *)NULL, 0);
	setbit(&siso, 59, (unsigned char *)NULL, 0);
	if(strlen(card_sn) ==3)
		setbit(&siso, 23, (unsigned char *)card_sn, 3);
	
	/*发送报文*/
	s = WriteXpe(siso);
	if(s == -1)
	{
		dcs_log(0, 0, "发送信息给核心失败");
		SaveErTradeInfo(RET_CODE_F9, currentDate, "发送信息给核心失败", mer_info_t, pep_jnls, trace);
		setbit(&siso, 4, (unsigned char *)pep_jnls.tranamt, 12);
		BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
	return 0;
}

//处理积分消费 现在放在消费交易一起处理，以后需要修改可以放在这里单独实现
int ConsumerCredit(ISO_data siso, PEP_JNLS pep_jnls, MER_INFO_T mer_info_t, char *currentDate)
{
	return 0;
}


//处理用户认证(短信认证和二维码的认证)
int UserAuthentication(ISO_data siso, PEP_JNLS pep_jnls, MER_INFO_T mer_info_t, char *currentDate)
{
	char databuf[10+1], trace[6+1], tmpbuf[512], tmp_len[3], bit_22[3], outPin[8+1];
	MSGCHGINFO	msginfo;
	int rtn = 0, s = 0, len = 0;
	unsigned char tmp;
	
	memset(databuf, 0, sizeof(databuf));
	memset(trace, 0, sizeof(trace));
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(tmp_len, 0, sizeof(tmp_len));
	memset(&msginfo, 0, sizeof(MSGCHGINFO));
	memset(bit_22, 0, sizeof(bit_22));
	memset(outPin, 0, sizeof(outPin));

	memcpy(databuf, currentDate+4, 10);
	
	//取前置系统流水
	if( pub_get_trace_from_seq(trace) < 0)
	{
		dcs_log(0, 0, "取前置系统交易流水错误");
		SaveErTradeInfo(RET_CODE_F8, currentDate, "取前置系统交易流水错误", mer_info_t, pep_jnls, trace);
		BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
	
	setbit(&siso, 7, (unsigned char *)currentDate+4, 10);
	setbit(&siso, 11, (unsigned char *)trace, 6);
	
	/*交易码来查询数据库msgchg_info来读取一些信息*/
	rtn = Readmsgchg(pep_jnls.trancde, &msginfo);
	if(rtn == -1)
	{
		dcs_log(0, 0, "查询msgchg_info失败,交易未开通, trancde=%s", pep_jnls.trancde);
		setbit(&siso, 39, (unsigned char *)RET_CODE_FB, 2);
		BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
	else
	{	
		if(memcmp(msginfo.flagpe, "Y", 1)!=0 && memcmp(msginfo.flagpe, "y", 1)!= 0)
		{
			dcs_log(0, 0, "由于系统或者其他原因，交易暂时关闭");
			BackToPos(RET_TRADE_CLOSED, siso, pep_jnls, databuf, mer_info_t);
			return -1;
		}
	}	
	
	setbit(&siso, 3, (unsigned char *)msginfo.process, 6);
	setbit(&siso, 25, (unsigned char *)msginfo.service, 2);
	setbit(&siso, 41, (unsigned char *)msginfo.termcde, 8);
	setbit(&siso, 42, (unsigned char *)msginfo.mercode, 15);
	
	s = getbit(&siso, 22, (unsigned char *)bit_22);
	if(s <= 0)
	{
		dcs_log(0, 0, "get bit_22 error!");
		BackToPos("H9", siso, pep_jnls, databuf, mer_info_t);	
		return -1;
	}
	
	if(memcmp(bit_22+2, "1", 1) == 0)
	{	
		memset(tmpbuf, 0, sizeof(tmpbuf));
		s = getbit(&siso, 52, (unsigned char *)tmpbuf);
		if(s < 0)
		{
			dcs_log(0, 0, "get bit_52 error!");
			BackToPos("H9", siso, pep_jnls, databuf, mer_info_t);	
			return -1;
		}
		#ifdef __TEST__
			dcs_debug(tmpbuf, s, "终端报文52域:");
		#endif
		
		s = PosPinConvert(mer_info_t.mgr_pik, "a596ee13f9a93800", "9999990010020030040", tmpbuf, outPin);
		if(s == 1)
		{
		#ifdef __TEST__
			dcs_debug(outPin, 8, "设置发送核心系统的报文52域:");
		#endif
			setbit(&siso, 52,(unsigned char *)outPin, 8);
			setbit(&siso, 53,(unsigned char *)"1000000000000000", 16);
		}
		else
		{
			dcs_log(0, 0, "pin转加密失败。");
			setbit(&siso, 39, (unsigned char *)RET_CODE_F7, 2);
			BackToPos(RET_CODE_F7, siso, pep_jnls, databuf, mer_info_t);
			return -1;
		}			
	}
	else
	{
		setbit(&siso, 26,(unsigned char *)NULL, 0);
		setbit(&siso, 52,(unsigned char *)NULL, 0);
		setbit(&siso, 53,(unsigned char *)NULL, 0);
	}	
	memcpy(bit_22, "01", 2);	
	setbit(&siso, 22,(unsigned char *)bit_22, 3);
	
	rtn = GetChannelRestrict(mer_info_t, pep_jnls.trancde);
	dcs_log(0, 0, "rtn:[%d]", rtn);	
	if(rtn < 0)
	{
		dcs_log(0,0, "渠道限制交易，交易不受理");
		setbit(&siso, 39, (unsigned char *)"FE", 2);
		BackToPos("FE", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}	
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	sprintf(tmpbuf, "%s%s%s%s%s", mer_info_t.mgr_mer_id, mer_info_t.mgr_term_id, "3", "03", pep_jnls.trancde);
	sprintf(tmpbuf+29, "%s%s%s%04d", pep_jnls.termtrc, "000000", "330000", rtn);
	setbit(&siso, 21, (unsigned char *)tmpbuf, 51);
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso, 63, (unsigned char *)tmpbuf);
	if(s <= 0)
	{
		dcs_log(0, 0, "get63 error！");
		return -1;
	}
	dcs_log(0, 0, "get63 [%s]", tmpbuf);	
	
	tmp = tmpbuf[3];
	len = ( tmp & 0x0F ) * 100;
	tmp = tmpbuf[4];
	len += (tmp & 0xF0) / 16 * 10 + ( tmp & 0x0F );
	setbit(&siso, 48, (unsigned char *)tmpbuf+5, len);
	
	/*
	if(memcmp(pep_jnls.trancde, "D03", 3)==0)
	{
		dcs_log(0, 0, "短信认证的处理");
	}
	else if(memcmp(pep_jnls.trancde, "D04", 3)==0)
	{
		dcs_log(0, 0, "条形码，二维码认证的处理");
	}
	*/
	
	setbit(&siso, 60, (unsigned char *)"00000000030000000000", 20);
	
	/*保存pep_jnls数据库*/
	rtn = WritePosXpep(siso, pep_jnls, databuf);
	if(rtn == -1)
	{
		dcs_log(0, 0, "保存pep_jnls数据失败");
		SaveErTradeInfo("FO", currentDate, "保存pep_jnls数据失败", mer_info_t, pep_jnls, trace);
		BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}		
	setbit(&siso, 64, (unsigned char *)NULL, 0);
	setbit(&siso, 63, (unsigned char *)NULL, 0);
	setbit(&siso, 20, (unsigned char *)NULL, 0);
	/*发送报文*/
	s = WriteXpe(siso);
	if(s == -1)
	{
		dcs_log(0, 0, "发送信息给核心失败");
		SaveErTradeInfo(RET_CODE_F9, currentDate, "发送信息给核心失败", mer_info_t, pep_jnls, trace);
		BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
	return 0;
}

//大数相乘的方法
int  multiply(char* a, char* b, char* c)
{
    int i,j,ca,cb,* s;
    ca=strlen(a);
    cb=strlen(b);
    s=(int*)malloc(sizeof(int)*(ca+cb));
    for (i=0;i<ca+cb;i++)
    	s[i]=0;
    for (i=0;i<ca;i++)
    	for (j=0;j<cb;j++)
    		s[i+j+1]+=(a[i]-'0')*(b[j]-'0');
    for (i=ca+cb-1;i>=0;i--)
    	if (s[i]>=10)
    	{
    	    s[i-1]+=s[i]/10;
    	    s[i]%=10;
        }
    i=0;
    while (s[i]==0)
    	i++;
   	for (j=0;i<ca+cb;i++,j++)
   		c[j]=s[i]+'0';
   	if(j<=4)
    {
         c[0]='0';
         c[1]=0;
    }
    else
    	c[j-4]='\0';
    free(s);
    return 0;
}

//MTM(2.51,25.8,1.15)处理扣率的公式SGL(R)
int Splitstr(char *str, STR_FEE *str_fee)
{
	char *p_str =NULL;
	int i = 0, flag = 0;
	char tmpstr[25];
	
	
	memset(tmpstr, 0, sizeof(tmpstr));
	memcpy(tmpstr, str, strlen(str));
	
    p_str = strtok(tmpstr, ",)(");
//   	dcs_log(0, 0, "p_str = [%s]", p_str);
   	if(memcmp(p_str, "MTM", 3)==0)
    {
        flag = 0;
    }
    else if(memcmp(p_str, "SGL", 3)==0)
    {
         flag = 1;
    }
    memcpy(str_fee->fee_flag, p_str, strlen(p_str));
    while(p_str!=NULL)
   { 
    	i++;
    	p_str=strtok(NULL, ",()");
//    	dcs_log(0, 0, "p_str = [%s]", p_str);
    	if(!flag && i==1)
            memcpy(str_fee->fee_min, p_str, strlen(p_str));
        else if(!flag && i==2)
            memcpy(str_fee->fee_max, p_str, strlen(p_str));
        else if(!flag && i==3)
            memcpy(str_fee->fee_r, p_str, strlen(p_str));
        else if(flag && i==1)
            memcpy(str_fee->fee, p_str, strlen(p_str));
    }
    return 0;
}

//去小数点并使其精确到分
int Decimal(char *source, char *dest)
{
	int i = 0, j = 0, len = 0, flag = 0;
	char *str =NULL;
	len =  strlen(source);
	if(len==0)
		return -1;
	str = strstr(source, ".");
    if(str==NULL)
        flag = 1;
    else if(strlen(str)==2)
        flag = 2;
	for(i = 0; i<len; i++)
	{
		if(source[i]!='.')
		{
			dest[j] = source[i];
			j++;
		}
		else
			continue;
	}
	if(flag ==1)
	{
         dest[j] ='0';
         dest[j+1] ='0';
	}
	else if(flag == 2)
    {
        dest[j] ='0';
    }
    return 0;
}

//根据公式得出最终的手续费
int GetFee(STR_FEE str_fee, char *srcfee, char *desfee)
{
	char tmpbuf[27], str_min[9], str_max[9], src_fee[9];
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(str_min, 0, sizeof(str_min));
	memset(str_max, 0, sizeof(str_max));
	memset(src_fee, 0, sizeof(src_fee));
	
	
//	dcs_log(0, 0, "MTM");
	Decimal(str_fee.fee_min, tmpbuf);
	strcpy(str_min,tmpbuf);
	PubAddSymbolToStr(str_min,8,'0',0);
	//sprintf(str_min, "%08s", tmpbuf);
	strcpy(src_fee,srcfee);
	PubAddSymbolToStr(src_fee,8,'0',0);
	//sprintf(src_fee, "%08s", srcfee);
	if(memcmp(src_fee, str_min, 8)< 0)
		sprintf(desfee, "%s%8s", "C", str_min);
	else
	{
		memset(tmpbuf, 0, sizeof(tmpbuf));
		Decimal(str_fee.fee_max, tmpbuf);
		//sprintf(str_max, "%08s", tmpbuf);
		strcpy(str_max,tmpbuf);
		PubAddSymbolToStr(str_max,8,'0',0);
		if(memcmp(src_fee, str_max, 8)>0)
			sprintf(desfee, "%s%8s", "C", str_max);
		else
			sprintf(desfee, "%s%8s", "C", src_fee);
	}
	return 0;	
}


/*
处理POS主密钥远程下载
*/
int GetRemoteKey(ISO_data siso, char *currentDate, char *termidm, MER_INFO_T mer_info_t, int iFid, char *iccardinfo)
{
	char tmpbuf[512], tmptpdu[23], bcd_buf[128];
	ICCARD_INFO iccard_info;
	int rtn = 0, s=0;

	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(tmptpdu, 0, sizeof(tmptpdu));
	memset(bcd_buf, 0, sizeof(bcd_buf));
	memset(&iccard_info, 0, sizeof(ICCARD_INFO));
	
	memcpy(tmptpdu, mer_info_t.mgr_tpdu, 10);
	
	memcpy(iccard_info.iccard_no, iccardinfo, 8);
	iccard_info.iccard_remain_num = atoi(iccardinfo+8);
	if(iccard_info.iccard_remain_num <=0)
	{
		dcs_log(0, 0, "IC卡剩余次数不足");
		setbit(&siso, 39, (unsigned char *)"96", 2);
	}
	#ifdef __LOG__
		dcs_log(0, 0, "iccard_info.iccard_no [%s]", iccard_info.iccard_no);
		dcs_log(0, 0, "iccard_info.iccard_remain_num [%d]", iccard_info.iccard_remain_num);
		dcs_log(0, 0, "iccardinfo [%s]", iccardinfo);
	#endif
	
	setbit(&siso, 0, (unsigned char *)"0810", 4);
	setbit(&siso, 12, (unsigned char *)currentDate+8, 6);
	setbit(&siso, 13, (unsigned char *)currentDate+4, 4);
	setbit(&siso, 21, (unsigned char *)NULL, 0);
	setbit(&siso, 37, (unsigned char *)currentDate+2, 12);
	setbit(&siso, 39, (unsigned char *)"00", 2);
	/*根据机具编号查询数据表pos_conf_info得到一些参数信息*/
	rtn = GetParameter(termidm, &mer_info_t);
	if(rtn < 0)
	{
		dcs_log(0,0,"获取参数信息错误");
		s = Save_Iccard_Info(&iccard_info, termidm, mer_info_t.mgr_term_id, mer_info_t.mgr_mer_id);	
		if(s < 0)
		{
			dcs_log(0, 0, "保存IC卡信息失败");
			return -1;
		}
		return -1;
	}
	setbit(&siso, 41, (unsigned char *)mer_info_t.mgr_term_id, 8);
	setbit(&siso, 42, (unsigned char *)mer_info_t.mgr_mer_id, 15);
	
	s = Save_Iccard_Info(&iccard_info, termidm, mer_info_t.mgr_term_id, mer_info_t.mgr_mer_id);	
	if(s < 0)
	{
		dcs_log(0, 0, "保存IC卡信息失败");
		setbit(&siso, 39, (unsigned char *)"96", 2);
	}
	
	memcpy(tmpbuf, mer_info_t.pos_kek2, 32);
	memcpy(tmpbuf+32, mer_info_t.kek2_check_value, 16);
	asc_to_bcd((unsigned char *)bcd_buf, (unsigned char*)tmpbuf, 48, 0);
	setbit(&siso, 62,(unsigned char *)bcd_buf, 24);

#ifdef __LOG__
	dcs_debug(bcd_buf, 24, "62bit info");
#endif
	memcpy(mer_info_t.mgr_tpdu, tmptpdu, 10);
	s = sndBackToPos(siso, iFid, mer_info_t);
	if(s < 0 )
	{
		dcs_log(0,0,"posUnpack error!");
		return -1;
	}
	return 0;
}

/*计算mac放在64域*/
int GetPosMacValue(ISO_data iso, MER_INFO_T mer_info_t)
{
	char macblock[2048], posmac[9];
	int len = 0, s = 0;
		
	memset(macblock, 0, sizeof(macblock));
	memset(posmac, 0, sizeof(posmac));
	
	setbit(&iso, 64, (unsigned char *)"00000000", 8);	
	len = GetPospMacBlk(&iso, macblock);
		
//	dcs_debug(macblock, len, "GetPospMacBlk Rtn len=%d.", len );
		
  	dcs_log(0, 0, "mer_info_t.encrpy_flag = [%d]", mer_info_t.encrpy_flag );
	s = GetPospMAC(macblock, len, mer_info_t.mgr_mak, posmac);
	if(s < 0)
	{
		dcs_log(0, 0, "GetPosMacValue 计算mac失败");
		return -1;
	}
#ifdef __LOG___
	dcs_debug( posmac, 8, "posmac :");
#endif
	setbit(&iso, 64, (unsigned char *)posmac, 8);
	return 0;
}


int DownICkeypara(ISO_data siso, PEP_JNLS pep_jnls, MER_INFO_T mer_info_t, int iFid, char *currentDate)
{
	 int rtn = -1, s = -1;
	 
	 char rid[10+1], capki[2+1], aid[32+1];
	 char trace[7], databuf[15], tmpbuf[512],  pubkeybuf[2048], bcd_pubkeybuf[1024];
	 char signtrancode[3+1], signflg[4], tran_batch_no[7], tmstatinfo[3+1], paradownloadflag[256+1];
	 
	 memset(rid, 0x00, sizeof(rid));
	 memset(capki, 0x00, sizeof(capki));
	 memset(aid, 0x00, sizeof(aid));
	 memset(trace, 0x00, sizeof(trace));
	 memset(databuf, 0x00, sizeof(databuf));
	 memset(tmpbuf, 0x00, sizeof(tmpbuf));
	 memset(pubkeybuf, 0x00, sizeof(pubkeybuf));
	 memset(bcd_pubkeybuf, 0x00, sizeof(bcd_pubkeybuf));
	 memset(signtrancode, 0x00, sizeof(signtrancode));
	 memset(signflg, 0x00, sizeof(signflg));
	 memset(tran_batch_no, 0x00, sizeof(tran_batch_no));
	 memset(tmstatinfo, 0x00, sizeof(tmstatinfo));
	 memset(paradownloadflag, 0x00, sizeof(paradownloadflag));

	memcpy(databuf, currentDate+4, 10);

    if(memcmp(pep_jnls.trancde, "901", 3) == 0)
    {
        dcs_log(0, 0, "POS状态上送");
        memset(tmpbuf, 0, sizeof(tmpbuf));
        s = getbit(&siso, 60, (unsigned char *)tmpbuf);
        if( s <= 0)
        {
            dcs_log(0, 0, "报文60域数据信息错误");
            return -1;
        }
        
        memset(signtrancode, 0, sizeof(signtrancode));
        memcpy(signtrancode, tmpbuf, 2);
        memset(tran_batch_no, 0, sizeof(tran_batch_no));
        memcpy(tran_batch_no, tmpbuf+2, 6);
        memset(signflg, 0, sizeof(signflg));
        memcpy(signflg, tmpbuf+8, 3);
        
        memset(tmpbuf, 0, sizeof(tmpbuf));
        s = getbit(&siso, 62, (unsigned char *)tmpbuf);
        if( s <= 0)
        {
            dcs_log(0, 0, "报文62域数据信息错误");
            return -1;
        }
        
        memset(tmstatinfo, 0, sizeof(tmstatinfo));
        memcpy(tmstatinfo, tmpbuf, 3);
        memset(pubkeybuf, 0, sizeof(pubkeybuf));
        
        if(memcmp(signflg, "372", 3) == 0 )/* IC卡公钥下载交易 */
        {
            dcs_log(0, 0, "IC卡公钥下载交易");
            if(memcmp(tmstatinfo, "100", 3) == 0 )/* IC卡公钥下载请求交易 */
            {
                dcs_log(0, 0, "IC卡公钥下载请求");
                rtn = GeticPubkey( pubkeybuf );
                if(rtn == -1)
                {
                    dcs_log(0, 0, "IC卡公钥下载失败");
                    BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
                    return -1;
                }
			#ifdef __LOG___
				dcs_log(0,0,"pubkeybuf[%d][%s]", getstrlen(pubkeybuf), pubkeybuf);
			#endif
                memset(bcd_pubkeybuf, 0x00, sizeof(bcd_pubkeybuf));
                asc_to_bcd((unsigned char *)bcd_pubkeybuf, (unsigned char*)pubkeybuf, getstrlen(pubkeybuf), 0);
			#ifdef __LOG___
                dcs_debug( bcd_pubkeybuf, getstrlen(pubkeybuf)/2, "pubkeybuf:");
			#endif

                setbit(&siso, 0, (unsigned char *)"0830", 4);
                setbit(&siso, 12, (unsigned char *)databuf+4, 6);
                setbit(&siso, 13, (unsigned char *)databuf, 4);
                setbit(&siso, 21, (unsigned char *)NULL, 0);
                setbit(&siso, 39, (unsigned char *)"00", 2);
                setbit(&siso, 62, (unsigned char *)bcd_pubkeybuf, getstrlen(pubkeybuf)/2);
                s = sndBackToPos(siso, iFid, mer_info_t);
                if(s < 0 )
                {
                    dcs_log(0,0,"pos pack error!");
                    return -1;
                }
            }
            return 0;
        }
        else if(memcmp(signflg, "382", 3) == 0 )/* IC卡参数下载交易 */
        {
            dcs_log(0, 0, "IC卡参数下载交易");
             if(memcmp(tmstatinfo, "100", 3) == 0 )/* IC卡公钥下载请求交易 */
            {
                dcs_log(0, 0, "IC卡参数下载请求");
                rtn = GeticPara( pubkeybuf );
                if(rtn == -1)
                {
                    dcs_log(0, 0, "IC卡参数下载失败");
                    SaveErTradeInfo(RET_CODE_HA, currentDate, "IC卡参数下载失败", mer_info_t, pep_jnls, trace);
                    BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
                    return -1;
                }
			#ifdef __LOG___
                dcs_log(0,0,"pubkeybuf[%s]",pubkeybuf);
			#endif
                memset(bcd_pubkeybuf, 0x00, sizeof(bcd_pubkeybuf));
                asc_to_bcd((unsigned char *)bcd_pubkeybuf, (unsigned char*)pubkeybuf, getstrlen(pubkeybuf), 0);
			#ifdef __LOG___
                dcs_debug( bcd_pubkeybuf, getstrlen(pubkeybuf)/2, "getstrlen(pubkeybuf):");
			#endif

                setbit(&siso, 0, (unsigned char *)"0830", 4);
                setbit(&siso, 12, (unsigned char *)databuf+4, 6);
                setbit(&siso, 13, (unsigned char *)databuf, 4);
                setbit(&siso, 21, (unsigned char *)NULL, 0);
                setbit(&siso, 39, (unsigned char *)"00", 2);
                setbit(&siso, 62, (unsigned char *)bcd_pubkeybuf, getstrlen(pubkeybuf)/2);
                s = sndBackToPos(siso, iFid, mer_info_t);
                if(s < 0 )
                {
                    dcs_log(0,0,"pos pack error!");
                    return -1;
                }
            }
        }
        else 
        {
            dcs_log(0, 0, "网络管理信息码错误");
            SaveErTradeInfo(RET_CODE_HA, currentDate, "网络管理信息码错误", mer_info_t, pep_jnls, trace);
            BackToPos("H9", siso, pep_jnls, databuf, mer_info_t);
            return -1;
        }
        
        return 0;
    }
    
    if(memcmp(pep_jnls.trancde, "902", 3) == 0)
    {
    	memset(tmpbuf, 0, sizeof(tmpbuf));
        s = getbit(&siso, 60, (unsigned char *)tmpbuf);
        if( s <= 0)
        {
            dcs_log(0, 0, "报文60域数据信息错误");
            return -1;
        }
        
        memset(signtrancode, 0, sizeof(signtrancode));
        memcpy(signtrancode, tmpbuf, 2);
        memset(tran_batch_no, 0, sizeof(tran_batch_no));
        memcpy(tran_batch_no, tmpbuf+2, 6);
        memset(signflg, 0, sizeof(signflg));
        memcpy(signflg, tmpbuf+8, 3);
        
        memset(tmpbuf, 0, sizeof(tmpbuf));
        s = getbit(&siso, 62, (unsigned char *)tmpbuf);
        if( s <= 0)
        {
            dcs_log(0, 0, "报文62域数据信息错误");
            return -1;
        }
	#ifdef __LOG__
        dcs_debug( tmpbuf, s, "62域请求信息:");
	#endif

        bcd_to_asc((unsigned char *)paradownloadflag, (unsigned char *)tmpbuf, 2*s, 0);
        dcs_log(0, 0, "62域请求信息:%s", paradownloadflag);
        
        if(memcmp(signflg, "370", 3)==0 || memcmp(signflg, "371", 3)==0 )
	    {
	    	memset(rid, 0x00, sizeof(rid));
	        memset(capki,0x00, sizeof(capki));
	        memcpy(rid, paradownloadflag+6, 10);
	        memcpy(capki, paradownloadflag+22, 2);
	        memset(pubkeybuf, 0, sizeof(pubkeybuf));

			#ifdef __LOG___
	        	dcs_log(0, 0, "rid[%s]capki[%s]",rid ,capki);
			#endif
	        rtn = DownLoadPublicKey(pubkeybuf, rid, capki);
	        if(rtn == -1)
	        {
	            dcs_log(0, 0, "IC卡公钥下载失败");
	            SaveErTradeInfo(RET_CODE_HA, currentDate, "IC卡公钥下载失败", mer_info_t, pep_jnls, trace);
	            BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
	            return -1;
	        }
	    }
	    else if( memcmp(signflg, "380", 3)==0 || memcmp(signflg, "381", 3)==0 )
	    {
	    	memset(aid, 0x00, sizeof(aid));
	        memcpy(aid, paradownloadflag+6, getstrlen(paradownloadflag)-6);
	        memset(pubkeybuf, 0, sizeof(pubkeybuf));

			#ifdef __LOG___
	        	dcs_log(0, 0, "aid[%s]",aid);
			#endif
	        rtn = DownLoadPara(pubkeybuf, aid);
	        if(rtn == -1)
	        {
	            dcs_log(0, 0, "IC卡参数下载失败");
	            SaveErTradeInfo(RET_CODE_HA, currentDate, "IC卡公钥下载失败", mer_info_t, pep_jnls, trace);
	            BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
	            return -1;
	        }
	    	
	    }
		#ifdef __LOG___
        	dcs_log(0,0,"[%d][%d]pubkeybuf[%s]",getstrlen(pubkeybuf)/2,strlen(pubkeybuf),pubkeybuf);
		#endif

        asc_to_bcd((unsigned char *)bcd_pubkeybuf, (unsigned char*)pubkeybuf, getstrlen(pubkeybuf), 0);
		#ifdef __LOG___
        	dcs_debug( bcd_pubkeybuf, getstrlen(pubkeybuf)/2, "bcd_pubkeybuf:");
		#endif

        setbit(&siso, 0, (unsigned char *)"0810", 4);
        setbit(&siso, 12, (unsigned char *)databuf+4, 6);
        setbit(&siso, 13, (unsigned char *)databuf, 4);
        setbit(&siso, 21, (unsigned char *)NULL, 0);
        setbit(&siso, 39, (unsigned char *)"00", 2);
        setbit(&siso, 62, (unsigned char *)bcd_pubkeybuf, getstrlen(pubkeybuf)/2);
        s = sndBackToPos(siso, iFid, mer_info_t);
        if(s < 0 )
        {
            dcs_log(0,0,"pos pack error!");
            return -1;
        }
        return 0;
    }
    
    if(memcmp(pep_jnls.trancde, "903", 3) == 0)
    {
        
        s = getbit(&siso, 60, (unsigned char *)paradownloadflag);
        if( s <= 0)
        {
            dcs_log(0, 0, "报文60域数据信息错误");
            return -1;
        }
        dcs_log(0, 0, "60域下载标志位:%s", paradownloadflag);
        
        if(memcmp(paradownloadflag+8,"371", 3) == 0 || memcmp(paradownloadflag+8,"381", 3) == 0)
        {
        	if( memcmp(paradownloadflag+8,"371", 3) == 0 )
        	{
        		rtn = Updatedownloadflag(siso, 1, currentDate);
        	}
        	else if(memcmp(paradownloadflag+8,"381", 3) == 0)
        	{
        		rtn = Updatedownloadflag(siso, 2, currentDate);
        	}
        	
            setbit(&siso, 0, (unsigned char *)"0810", 4);
            setbit(&siso, 12, (unsigned char *)databuf+4, 6);
            setbit(&siso, 13, (unsigned char *)databuf, 4);
            setbit(&siso, 21, (unsigned char *)NULL, 0);
            if( rtn == 0 )
            {
            	setbit(&siso, 39, (unsigned char *)"00", 2);
        	}
        	else 
        	{
        	 	setbit(&siso, 39, (unsigned char *)"96", 2);
        	}
            s = sndBackToPos(siso, iFid, mer_info_t);
            if(s < 0 )
            {
                dcs_log(0,0,"pos pack error!");
                return -1;
            }
        }
        else 
        {
            dcs_log(0, 0, "IC卡公钥下载结束参数错误");
            SaveErTradeInfo(RET_CODE_HA, currentDate, "IC卡公钥下载结束参数错误", mer_info_t, pep_jnls, trace);
            BackToPos("H9", siso, pep_jnls, databuf, mer_info_t);
            return -1;
        }
        return 0;
    }
}



/*处理IC卡交易 TC值上送核心*/
int ProcessTcvalue(ISO_data siso, PEP_JNLS pep_jnls, MER_INFO_T mer_info_t, char *currentDate)
{
	char tmpbuf[127], trace[6+1], databuf[10+1];
	int s, rtn, n;
	char outCardType;
	
	//交易类型配置信息表，余额查询根据交易码查询数据库表msgchg_info，得到的信息放入该结构体中
	MSGCHGINFO	msginfo;
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(trace, 0, sizeof(trace));
	memset(databuf, 0, sizeof(databuf));
	memset(&msginfo, 0, sizeof(MSGCHGINFO));

	memcpy(pep_jnls.termcde, mer_info_t.mgr_term_id, 8);
	memcpy(pep_jnls.mercode, mer_info_t.mgr_mer_id, 15);
	memcpy(databuf, currentDate+4, 10);
	
	//TC值上送核心流水号采用原交易流水号
	sprintf(trace, "%06d",pep_jnls.trace);
	
	/*交易码来查询数据库msgchg_info来读取一些信息*/
	rtn = Readmsgchg(pep_jnls.trancde, &msginfo);
	if(rtn == -1)
	{
		dcs_log(0, 0, "查询msgchg_info失败,交易未开通, trancde=%s", pep_jnls.trancde);
		SaveErTradeInfo(RET_CODE_FB, currentDate, "查询msgchg_info失败,交易未开通", mer_info_t, pep_jnls, trace);
		//BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
	else
	{	
		if(memcmp(mer_info_t.mgr_tpdu+2, "0726", 4)== 0)/*0726只给我们自己生产测试用的TPDU头中的目的地址*/
		{
			dcs_log(0, 0, "内部生产测试不走交易关闭的风控");
		}
		else
		{
			if(memcmp(msginfo.flagpe, "Y", 1)!=0 && memcmp(msginfo.flagpe, "y", 1)!= 0)
			{
				dcs_log(0, 0, "由于系统或者其他原因，交易暂时关闭");
				SaveErTradeInfo(RET_TRADE_CLOSED, currentDate, "交易暂时关闭", mer_info_t, pep_jnls, trace);
				//BackToPos(RET_TRADE_CLOSED, siso, pep_jnls, databuf, mer_info_t);
				return -1;
			}
		}
	}

	setbit(&siso, 0, (unsigned char *)"0320", 2);
	setbit(&siso, 3, (unsigned char *)msginfo.process, 6);
	/*4域终端已上送*/
	setbit(&siso, 7, (unsigned char *)databuf, 10);
	setbit(&siso, 25, (unsigned char *)msginfo.service, 2);

	s = getstrlen(pep_jnls.outcdno);
	setbit(&siso, 2, (unsigned char *)pep_jnls.outcdno, s);
	pep_jnls.outcdno[s] = '\0';
	outCardType = ReadCardType(pep_jnls.outcdno);
	pep_jnls.outcdtype = outCardType;

	s = getbit(&siso, 11, (unsigned char *)pep_jnls.termtrc);
	if(s <= 0)
	{
		dcs_log(0, 0, "get bit_11 error!");
		return -1;
	}
	setbit(&siso, 11, (unsigned char *)trace, 6);
	
	rtn = GetChannelRestrict(mer_info_t, pep_jnls.trancde);
	dcs_log(0, 0, "rtn:[%d]", rtn);	
	if(rtn < 0)
	{
		dcs_log(0,0, "渠道限制交易，交易不受理");
		//setbit(&siso, 39, (unsigned char *)"FE", 2);
	 	//WritePosXpep(siso, pep_jnls, databuf);
		//BackToPos("FE", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}

	s = getbit(&siso, 60, (unsigned char *)tmpbuf);
	if(s <= 0)
	{
		dcs_log(0, 0, "取终端交易批次号错误");
		//setbit(&siso, 39, (unsigned char *)RET_CODE_H9, 2);
 		//WritePosXpep(siso, pep_jnls, databuf);
		//BackToPos(RET_CODE_H9, siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
	else if( s<13 )
	{
		memcpy(pep_jnls.batch_no, tmpbuf+2, 6);
		memcpy(tmpbuf, "20", 2);
	}
	else
	{
		memcpy(pep_jnls.batch_no, tmpbuf+2, 6);
		memcpy(tmpbuf,tmpbuf+11,2);//取终端上送的60.4,60.5送给核心
	}
	memcpy(tmpbuf+2,"03", 2);//终端类型
	tmpbuf[4]='\0';
	setbit(&siso, 60, (unsigned char *)tmpbuf, 4);

	memset(tmpbuf, 0, sizeof(tmpbuf));
	sprintf(tmpbuf, "%s%s%s%s%s", mer_info_t.mgr_mer_id, mer_info_t.mgr_term_id, "3", "03", pep_jnls.trancde);
	sprintf(tmpbuf+29, "%s%s%s%04d", pep_jnls.termtrc, pep_jnls.batch_no, msginfo.process, rtn);
	
	if(getstrlen(mer_info_t.T0_flag)==0)
		memcpy(mer_info_t.T0_flag,"0",1);
	if(getstrlen(mer_info_t.agents_code)==0)
		memcpy(mer_info_t.agents_code,"000000",6);
	sprintf(tmpbuf+51, "%s", mer_info_t.T0_flag);
	sprintf(tmpbuf+52, "%s", mer_info_t.agents_code);
	setbit(&siso, 21, (unsigned char *)tmpbuf, 58);

	//授权码固定填写000000，核心不需要此字段modify at 20141203
	memset(tmpbuf, 0, sizeof(tmpbuf));
	sprintf(tmpbuf, "%s%s%s%06d", "000000", pep_jnls.syseqno, pep_jnls.revdate, pep_jnls.trace);
	setbit(&siso, 61, (unsigned char *)tmpbuf, 34);//原交易授权号	N6参考号	N12时间N10

	setbit(&siso, 20, (unsigned char *)NULL, 0);
	setbit(&siso, 32, (unsigned char *)NULL, 0);
	setbit(&siso, 37, (unsigned char *)NULL, 0);
	setbit(&siso, 39, (unsigned char *)NULL, 0);
	setbit(&siso, 64, (unsigned char *)NULL, 0);
	
	if(strlen(card_sn) ==3)
	{
		setbit(&siso, 23, (unsigned char *)card_sn, 3);
	}
	
	/*发送报文*/
	s = WriteXpe(siso);
	if(s == -1)
	{
		dcs_log(0, 0, "发送信息给核心失败");
		SaveErTradeInfo(RET_CODE_F9, currentDate, "发送信息给核心失败", mer_info_t, pep_jnls, trace);
		//BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
		return -1;
	}
	return 0;
}

/*
* 处理脱机交易多次上送的情况
*
*/
int JudgeOrginTrans(ISO_data siso, PEP_JNLS pep_jnls, MER_INFO_T mer_info_t, char *currentDate)
{
	char tmpbuf[127], databuf[10+1], macblock[1024], posmac[9];
	int s = 0, rtn = 0, channel_id = 0, len = 0;
	
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	memset(databuf, 0x00, sizeof(databuf));
	memset(macblock, 0, sizeof(macblock));
	memset(posmac, 0, sizeof(posmac));	
	
	memcpy(databuf, currentDate+4, 10);
	
	s = getbit(&siso, 55, (unsigned char *)pep_jnls.filed55);
	if(s <= 0)
	{
		dcs_log(0, 0, "bit55 is null!");	
	}
	else
	{	
	#ifdef __LOG__
		dcs_debug(pep_jnls.filed55, s, "解析55域内容:");
	#endif
		rtn = analyse55(pep_jnls.filed55, s, stFd55_IC, 34);
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
	}
	memcpy(pep_jnls.postermcde, mer_info_t.mgr_term_id, 8);
	memcpy(pep_jnls.posmercode, mer_info_t.mgr_mer_id, 15);
	/*查询原笔 */
	rtn = GetOriTuoInfo(&pep_jnls);
	if(rtn == -1)
	{
		dcs_log(0, 0, "脱机上送处理失败[%d]", rtn);
		BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
	}
	//如果已经发送过，取之前发送的信息重组报文再次发送核心
	else if(rtn ==3)
	{
		dcs_log(0, 0, "找到已上送请求[%d]", rtn);
		dcs_log(0, 0, "组包发往核心");
		
		channel_id = GetPosChannelId(pep_jnls.postermcde, pep_jnls.posmercode);
		if(channel_id < 0)
		{	
			dcs_log(0, 0, "取渠道信息失败");
			SaveErTradeInfo(RET_CODE_F9, currentDate, "取POS渠道信息失败", mer_info_t, pep_jnls, pep_jnls.trace);
			BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
			return -1;
		}
		setbit(&siso, 3,(unsigned char *)pep_jnls.process, 6);
		
		memset(tmpbuf, 0, sizeof(tmpbuf));
		sprintf(tmpbuf, "%s%s", pep_jnls.pepdate+4, pep_jnls.peptime);
#ifdef __LOG__
		dcs_log(0, 0, "7域信息[%s],len =[%d]", tmpbuf, strlen(tmpbuf));
#endif

		setbit(&siso, 7,(unsigned char *)tmpbuf, 10);
		memset(tmpbuf, 0, sizeof(tmpbuf));
		sprintf(tmpbuf, "%06d", pep_jnls.trace);
		setbit(&siso, 11,(unsigned char *)tmpbuf, 6);
		
		memset(tmpbuf, 0, sizeof(tmpbuf));
		sprintf(tmpbuf, "%s%s%s%s%s", mer_info_t.mgr_mer_id, mer_info_t.mgr_term_id, "3", "03", pep_jnls.trancde);
		sprintf(tmpbuf+29, "%s%s%s%04d", pep_jnls.termtrc, pep_jnls.batch_no, "000000", channel_id);

		/*--- modify begin 140325 ---*/	
		if(getstrlen(mer_info_t.T0_flag)==0)
			memcpy(mer_info_t.T0_flag,"0",1);
		if(getstrlen(mer_info_t.agents_code)==0)
			memcpy(mer_info_t.agents_code,"000000",6);
		sprintf(tmpbuf+51, "%s", mer_info_t.T0_flag);
		sprintf(tmpbuf+52, "%s", mer_info_t.agents_code);
		setbit(&siso, 21, (unsigned char *)tmpbuf, 58);
#ifdef __LOG__
		dcs_log(0, 0, "21域信息[%s],len =[%d]", tmpbuf, strlen(tmpbuf));
#endif
		memset(tmpbuf, 0, sizeof(tmpbuf));
		s=getbit(&siso, 60, (unsigned char *)tmpbuf);
		if( s<13 )
			memcpy(tmpbuf, "20", 2);
		else 
			memcpy(tmpbuf,tmpbuf+11,2);//取终端上送的60.4,60.5送给核心
		memcpy(tmpbuf+2,"03", 2);//终端类型
		tmpbuf[4]='\0';
		setbit(&siso, 60, (unsigned char *)tmpbuf, 4);
		
		setbit(&siso, 25,(unsigned char *)pep_jnls.poscode, 2);
		setbit(&siso, 20, (unsigned char *)NULL, 0);
		setbit(&siso, 63, (unsigned char *)NULL, 0);
		setbit(&siso, 64, (unsigned char *)NULL, 0);

        memset(g_key, 0, sizeof(g_key));
		/*发送报文*/
		s = WriteXpe(siso);
		if(s == -1)
		{
			dcs_log(0, 0, "发送信息给核心失败");
			SaveErTradeInfo(RET_CODE_F9, currentDate, "发送信息给核心失败", mer_info_t, pep_jnls, pep_jnls.trace);
			BackToPos("96", siso, pep_jnls, databuf, mer_info_t);
			return -1;
		}
	}
	return rtn;
}

//解析二维码信息并保存到数据表two_code_info
int Analy2code(char *srcBuf, int len, char *trace, char *currentDate)
{
	TWO_CODE_INFO two_code_info;
	char tempbuf[1024];
	int templen = 0, i = 0, rtn = 0;
	char *two_info =NULL;
	
	memset(tempbuf, 0x00, sizeof(tempbuf));
	memset(&two_code_info, 0x00, sizeof(TWO_CODE_INFO));
	
	memcpy(tempbuf, srcBuf, 2);
	templen = atoi(tempbuf);
	
	two_info = strtok(srcBuf+2, "|");
	while(two_info)
	{
		if(i ==0)
			memcpy(two_code_info.ordid, two_info, strlen(two_info));
		if(i==1)
			memcpy(two_code_info.amt, two_info, strlen(two_info));
		if(i ==2)
			memcpy(two_code_info.orddesc, two_info, strlen(two_info));
		if(i==3)
			memcpy(two_code_info.ordtime, two_info, strlen(two_info));
		if(i ==4)
			memcpy(two_code_info.hashcode, two_info, strlen(two_info));
		i++;
		two_info = strtok(NULL, "|");
	}
	//保存二维码信息
	rtn = SaveTwoInfo(two_code_info);
	//发送支付成功信息给商城
	//---------------此为测试版本用----------为配合demo演示
	SendPayResult(two_code_info, trace, currentDate);
	//---------------此为测试版本用----------为配合demo演示
	return rtn;
}


/*
//自己模拟测试用没有POS终端，直接从数据表里取数据用
int Analy2code(char *trace, char *currentDate)
{
	TWO_CODE_INFO two_code_info;
	memset(&two_code_info, 0x00, sizeof(TWO_CODE_INFO));
	//获取二维码信息
	GetTwoInfo(&two_code_info);
	//发送支付成功信息给商城
	SendPayResult(two_code_info, trace, currentDate);
	return 0;
}
*/
