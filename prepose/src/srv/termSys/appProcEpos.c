#include "ibdcs.h"
#include <time.h>
#include "pub_error.h" 
#include "sms.h"
#include "pub_function.h"
#include "memlib.h"

extern struct ISO_8583 *iso8583;
extern struct ISO_8583 iso8583_conf[128];
extern struct ISO_8583 iso8583conf[128];
//extern struct ISO_8583 iso8583posconf[128];
extern pthread_key_t iso8583_conf_key;//线程私有变量
extern pthread_key_t iso8583_key;//线程私有变量

/* 应用核心系统返回的数据 */
void eposAppProc(char * srcBuf, int iFid, int iLen) 
{
	ISO_data iso;
	PEP_JNLS pep_jnls;
	TRANSFER_INFO transfer_info;
	EWP_INFO ewp_info;
	PEP_ICINFO pep_icinfo;
	char tmpbuf[256], buffer[1024], buffer2[1024], new_termcde[9], new_mercode[15];
	char macblock[2048], posmac[9], sendMac[9], ascbuf[512], bcdbuf[512], rcvbuf[128];
	char filed55[256+1];
	int rtn =0 , gs_myFid =0 , s =0 , len =0 ;
	
	MER_INFO_T mer_info_t;
	char flag;
	#ifdef __LOG__
		dcs_debug(srcBuf, iLen, "核心系统返回的数据, iFid=%d, iLen=%d", iFid, iLen);
	#else
		dcs_log(0, 0, "核心系统返回的数据, iFid=%d, iLen=%d", iFid, iLen);
	#endif
	
	memset(&pep_jnls, 0, sizeof(PEP_JNLS));
	memset(&transfer_info, 0, sizeof(TRANSFER_INFO));
	memset(&iso, 0, sizeof(ISO_data));
	memset(&ewp_info, 0, sizeof(EWP_INFO));
	memset(buffer, 0, sizeof(buffer));
	memset(buffer2, 0, sizeof(buffer2));
	memset(ascbuf, 0, sizeof(ascbuf));
	memset(bcdbuf, 0, sizeof(bcdbuf));
	memset(new_termcde, 0, sizeof(new_termcde));
	memset(new_mercode, 0, sizeof(new_mercode));
	memset(&pep_icinfo, 0, sizeof(PEP_ICINFO));
	memset(filed55, 0, sizeof(filed55));
	memset(rcvbuf, 0, sizeof(rcvbuf));
	
	/*解包*/
#ifdef __LOG__
	dcs_log(0, 0, "开始解包!");
#endif
	if (0> eposAppMsgUnpack( srcBuf, &iso, iLen))
	{
		#ifdef __LOG__
			dcs_debug(srcBuf, iLen, "Can not unpacket kernel Msg!datalen=%d", iLen);
		#else
			dcs_log(0, 0, "Can not unpacket kernel Msg!datalen=%d", iLen);
		#endif

		return;
	}
#ifdef __LOG__
	dcs_log(0, 0, "解包成功!");
#endif

	if (getbit(&iso, 0, (unsigned char *)pep_jnls.msgtype)<0)
	{
		 dcs_log(0, 0, "can not get bit_0!");	
		 return;
	}
	if (getbit(&iso, 3, (unsigned char *)pep_jnls.process)<0)
	{
		 dcs_log(0, 0, "can not get bit_3!");	
		 return;
	}
	
	/*TC值上送核心，前置不处理核心返回报文，直接丢弃*/
	if(memcmp(pep_jnls.msgtype,"0330", 4)== 0 && memcmp(pep_jnls.process, "900000", 6) == 0)
	{
		dcs_log(0, 0, "核心TC值上送返回报文，直接丢弃");
		return;	
	}
	
	
	if(memcmp(pep_jnls.msgtype, "0410", 4)==0)//冲正类
	{
		/*处理冲正交易*/
		rtn = ProcessReversed(iso, pep_jnls, ewp_info, buffer);
		if(rtn == -1)
		{
			dcs_log(0, 0, "处理冲正错误!");
			return;
		}
		return;
	}
	if(memcmp(pep_jnls.process, "310000", 6)==0 && memcmp(pep_jnls.msgtype, "0210", 4)==0)//余额查询类
	{
		/*处理余额查询*/
		dcs_log(0, 0, "开始处理余额查询!");
		rtn = ProcessBalance(iso, pep_jnls, ewp_info, buffer);
		if(rtn == -1)	
		{
			dcs_log(0, 0, "处理余额查询错误!");
			return;
		}
		return;
	}
	if(memcmp(pep_jnls.msgtype, "0630", 4)==0)//脚本通知类
	{
		/*更新数据库*/
		dcs_log(0, 0, "saveOrUpdateManageInfo");
		rtn = saveOrUpdateManageInfo(iso, &pep_jnls);
		if(rtn < 0)
		{
			dcs_log(0, 0, "saveOrUpdateManageInfo error!");
			return;
		}
		
	}
	else
	{
		
		/*更新数据库*/
		rtn = saveOrUpdatePepjnls(iso, &pep_jnls);
		if(rtn < 0)
		{
			dcs_log(0, 0, "saveOrUpdatePepjnls error!");
			return;
		}
		
	}
	pub_rtrim(pep_jnls.trnsndp, strlen(pep_jnls.trnsndp), pep_jnls.trnsndp);
	
	#ifdef __LOG__
		dcs_log(0, 0, "pep_jnls.trnsndp = [%s]", pep_jnls.trnsndp);
		dcs_log(0, 0, "pep_jnls.trnsndpid = [%d]", pep_jnls.trnsndpid);
		dcs_log(0, 0, "pep_jnls.aprespn = [%s]", pep_jnls.aprespn);
		dcs_log(0, 0, "pep_jnls.modeflg = [%s]", pep_jnls.modeflg);
	#endif
	
	if(memcmp(pep_jnls.aprespn, "00", 2) == 0 && memcmp(pep_jnls.modeflg, "1", 1) == 0
		&&memcmp("TRNF", pep_jnls.trnsndp, 4)!= 0)
	{
		dcs_log(0, 0, "send msg info");
		SendMsg(pep_jnls);
	}
	
	if(memcmp("TRNF", pep_jnls.trnsndp, 4)== 0)
	{
		dcs_log(0, 0, "转账汇款扣款响应处理");
		/*更新转账汇款表的扣款处理的相关字段更新完之后
		*扣款成功次日到账，返回A6
		*扣款失败次日到账，返回失败
		*扣款成功，普通到账这边发起代付请求之后，直接退出
		*扣款失败，普通到账，返回失败
		*/
		rtn = updateSaleAprespn(pep_jnls, &transfer_info);
		{
			if(rtn < 0)
			{
				dcs_log(0, 0, "updateSaleAprespn error!!");
				return;
			}
		}
		//移除设置超时发冲正的定时任务
		//先移除超时控制
		memset(tmpbuf, 0, sizeof(tmpbuf));
		sprintf(tmpbuf, "%s%s%06ld", "PEX_COM", transfer_info.orderId, pep_jnls.trace);
		rtn = delete_timeout_control(tmpbuf, rcvbuf, sizeof(rcvbuf)); //测试超时控制超时发送的功能
		if(rtn ==-1)
		{
			dcs_log(0, 0, "移除冲正超时控制失败");
			return;
		}
		if(memcmp(rcvbuf+2, "response=00", 11)==0)
		{
			if(memcmp(rcvbuf+14, "status=2", 8)==0)//2 表示 超时控制已超时发起冲正
			{
				dcs_log(0, 0, "移除成功,已超时发起冲正");
				return;
			}
			else if(memcmp(rcvbuf+14, "status=1", 8)==0||memcmp(rcvbuf+14, "status=0", 8)==0)//0 表示 没找到key对应的超时控制；1表示 移除成功
			{
				dcs_log(0, 0, "移除超时控制成功,无需发起冲正");
			}
		}
		memcpy(pep_jnls.transfer_type, transfer_info.transfer_type, 1);
		if(memcmp(pep_jnls.aprespn, "00", 2) == 0&&memcmp(transfer_info.transfer_type, "0", 1) == 0)
		{
			dcs_log(0, 0, "扣款成功，普通到账，发起代付");
			//发起代付请求
			//向超时队列里丢一条记录
			memcpy(pep_jnls.aprespn, "A6", 2);
			rtn = SetTimeOutControl(pep_jnls, iso);
			if(rtn ==-1)
			{
				dcs_log(0, 0, "添加超时控制失败");
			}
			rtn = PayRequest(pep_jnls, transfer_info, iso);
			if(rtn ==-1)
			{
				dcs_log(0, 0, "代付发起失败");
				dealPayFalse(pep_jnls, transfer_info, ewp_info, iso);
			}
			return;
		}
		else if(memcmp(pep_jnls.aprespn, "00", 2) == 0&&memcmp(transfer_info.transfer_type, "1", 1) == 0)
		{
			dcs_log(0, 0, "扣款成功，次日到账，返回A6");
			memcpy(pep_jnls.aprespn, "A6", 2);
		}
		else
		{
			dcs_log(0, 0, "其他情况直接返回扣款结果");
		}
		
	}
	/*
		如果交易有信用卡或者借记卡日限额，那么就更新日限额表
	*/
	if(pep_jnls.camtlimit == 1 && memcmp(pep_jnls.aprespn, "00", 2) == 0){
		#ifdef __LOG__
				dcs_log(0, 0, "信用卡日限额, trade amt = [%s]", pep_jnls.tranamt);
		#endif
		if( pep_jnls.outcdtype == 'C' || pep_jnls.outcdtype == 'c')
		{
			rtn = UpdateTerminalCreditCardTradeAmtLimit(pep_jnls.samid, pep_jnls.tranamt);
			
			if(rtn == -1)
				dcs_log(0, 0, "UpdateTerminalCreditCardTradeAmtLimit错误，pep_jnls.samid=%s, pep_jnls.tranamt=[%s]", pep_jnls.samid, pep_jnls.tranamt);
		}
	}
	
	if(pep_jnls.damtlimit == 1 && memcmp(pep_jnls.aprespn, "00", 2) == 0){
#ifdef __LOG__
		dcs_log(0, 0, "借记卡日限额, trade amt = [%s]", pep_jnls.tranamt);
#endif
		if( pep_jnls.outcdtype == 'D' || pep_jnls.outcdtype == 'd')
		{
			rtn = UpdateTerminalDebitCardTradeAmtLimit(pep_jnls.samid, pep_jnls.tranamt);
			if(rtn == -1)
				dcs_log(0, 0, "UpdateTerminalDebitCardTradeAmtLimit错误，pep_jnls.samid=[%s], pep_jnls.tranamt=[%s]", pep_jnls.samid, pep_jnls.tranamt);
		}
	}
	/*判断该终端在该商户上是否有信用卡日限额的限制*/
	if(pep_jnls.termmeramtlimit == 1 && memcmp(pep_jnls.aprespn, "00", 2) == 0){
#ifdef __LOG__
		dcs_log(0, 0, "该终端在该商户上信用卡日限额, trade amt = [%s]", pep_jnls.tranamt);
#endif
		if( pep_jnls.outcdtype == 'C' || pep_jnls.outcdtype == 'c')
		{
			rtn = UpdateTermMerCreditCardTradeAmtLimit(pep_jnls.samid, pep_jnls.mercode, pep_jnls.tranamt);
			if(rtn == -1)
				dcs_log(0, 0, "UpdateTermMerCreditCardTradeAmtLimit错误，samid=[%s], mercode=[%s], transamt=[%s]", pep_jnls.samid, pep_jnls.mercode, pep_jnls.tranamt);
		}
	}
	
	if(memcmp("H3CP", pep_jnls.trnsndp, 4) == 0   //ewp交易
		|| memcmp("MPOS", pep_jnls.trnsndp, 4) == 0 //mpos交易
		|| memcmp("CUST", pep_jnls.trnsndp, 4) == 0 //管理平台,融易付平台
		|| memcmp("TRNF", pep_jnls.trnsndp, 4) == 0 ) //转账汇款EWP
	{
		memset(buffer, 0, sizeof(buffer));
		
		/*
		rtn = RdDealcodebyOrderNo(pep_jnls, &ewp_info);
		if(rtn < 0)
		{
			dcs_log(0, 0, "Read ewp_info 交易处理码 error!");
			return;
		}
		*/
		//脱机消费去除超时控制
		if(memcmp(pep_jnls.trancde, "211", 3)== 0)
		{
			//超时控制
			char key[20+1];
			//系统自带的时间结构体
			struct  tm *tm;
			time_t  t;

			memset(key, 0, sizeof(key));
			if(memcmp(pep_jnls.aprespn, "00", 2)== 0)
			{
				time(&t);
				tm = localtime(&t);

				sprintf(key, "%04d", tm->tm_year+1900);
				if(getbit(&iso, 7, (unsigned char *)key+4)<0)
				{
					dcs_log(0, 0, "can not get bit_7!");
					return;
				}

				if(getbit(&iso, 11, (unsigned char *)key+14)<0)
				{
					dcs_log(0, 0, "can not get bit_11!");
					return;
				}
				tm_remove_entry(key);
			}
		}
		rtn = GetInfoFromMem(pep_jnls, &ewp_info);
		if(rtn < 0)
		{
			dcs_log(0, 0, "Read ewp_info from memcached error!");
			return;
		}
		
		//盛京惠生活,融易付mpos交易
		if(memcmp("MPOS", pep_jnls.trnsndp, 4) == 0)
		{
			s = DataBackToMPOS(1, buffer, pep_jnls, ewp_info, iso); //return back data length
			if(s <= 0)
			{
				dcs_log(0, 0, "DataBackToEWP error!");
				return;
			}
			
		}
		else if(memcmp("TRNF", pep_jnls.trnsndp, 4) == 0)
		{
			//盛京EWP转账汇汇款返回
			s = DataReturnToEWP(1, buffer, pep_jnls, ewp_info, iso); //return back data length
			if(s <= 0)
			{
				dcs_log(0, 0, "DataReturnToEWP error!");
				return;
			}
		}
		else
		{
			
			s = DataBackToEWP(1, buffer, pep_jnls, ewp_info, iso); //return back data length
			if(s <= 0)
			{
				dcs_log(0, 0, "DataBackToEWP error!");
				return;
			}
		}
		if(memcmp(ewp_info.consumer_sentinstitu, "0200", 4)==0|| memcmp(ewp_info.consumer_sentinstitu, "0700", 4)==0)
		{
			memset(sendMac, 0, sizeof(sendMac));
			rtn = pub_cust_mac(ewp_info.consumer_sentinstitu, buffer, s, sendMac);
			if(rtn !=0)
				return;
			else
			{
				memcpy(buffer+s, sendMac, 8);
				s +=8;
			}
		}
		
		if(getstrlen(ewp_info.tpduheader) != 0)
		{
			memset(buffer2, 0, sizeof(buffer2));
			asc_to_bcd((unsigned char *)buffer2, (unsigned char*)ewp_info.tpduheader, getstrlen(ewp_info.tpduheader), 0);
			
			memcpy(buffer2+5, buffer, s);
			s += 5;
			memcpy(buffer, buffer2, 1);
			memcpy(buffer+1, buffer2+3, 2);
			memcpy(buffer+3, buffer2+1, 2);
			memcpy(buffer+5, buffer2+5, s-5);
		}
	}
	//pos终端发起的交易
	else if(memcmp(pep_jnls.trnsndp, "TPOS", 4) == 0)
		{
		#ifdef __TEST__   
			s= getstrlen(pep_jnls.outcdno);
			if(s!=0)
			{
				setbit(&iso, 2, (unsigned char *)pep_jnls.outcdno, s);
			}
			s = getbit(&iso, 23, (unsigned char *)tmpbuf);
			if(s > 0)
				setbit(&iso, 23, (unsigned char *)pep_jnls.card_sn, 3);
		#endif	
		
		if(memcmp(pep_jnls.process, "910000", 6) == 0 && memcmp(pep_jnls.msgtype,"0200", 4)== 0)//消费交易 脱机消费请求
		{
			if(memcmp(pep_jnls.trancde, "000", 3)== 0 || memcmp(pep_jnls.trancde, "050", 3)== 0)
			{
				setbit(&iso, 3, (unsigned char *)"000000", 6);
				setbit(&iso, 63, (unsigned char *)"CUP", 3);
				if(memcmp(pep_jnls.trancde, "000", 3)== 0)
				{
					memset(tmpbuf, 0, sizeof(tmpbuf));
					memcpy(tmpbuf, "22", 2);
					memcpy(tmpbuf+2, pep_jnls.batch_no, 6);
					setbit(&iso, 60, (unsigned char *)tmpbuf, 8);
					dcs_log(0, 0, "POS联机消费");
				}
				else if(memcmp(pep_jnls.trancde, "050", 3)== 0)
				{
					memset(tmpbuf, 0, sizeof(tmpbuf));
					memcpy(tmpbuf, "36", 2);
					memcpy(tmpbuf+2, pep_jnls.batch_no, 6);
					setbit(&iso, 60, (unsigned char *)tmpbuf, 8);
					dcs_log(0, 0, "POS脱机消费");
					setbit(&iso, 25, (unsigned char *)"00", 2);

					//超时控制
					char key[20+1], aprespn_tmp[2+1];
					//系统自带的时间结构体
					struct  tm *tm;
					time_t  t;

					memset(key, 0, sizeof(key));
					memset(aprespn_tmp, 0, sizeof(aprespn_tmp));

					if(getbit(&iso, 39, (unsigned char *)aprespn_tmp)<0)
					{
						dcs_log(0, 0, "can not get bit_39!");
						return;
					}
					if(memcmp(aprespn_tmp, "00", 2)== 0)
					{
						time(&t);
						tm = localtime(&t);

						sprintf(key, "%04d", tm->tm_year+1900);
						if(getbit(&iso, 7, (unsigned char *)key+4)<0)
						{
							dcs_log(0, 0, "can not get bit_7!");
							return;
						}

						if(getbit(&iso, 11, (unsigned char *)key+14)<0)
						{
							dcs_log(0, 0, "can not get bit_11!");
							return;
						}
						tm_remove_entry(key);
					}
				}
			}
			else
			{
				setbit(&iso, 41, (unsigned char *)pep_jnls.postermcde, 8);
				setbit(&iso, 42, (unsigned char *)pep_jnls.posmercode, 15);
			}
			if(memcmp(pep_jnls.trancde, "D01", 3)== 0)
			{
				dcs_log(0, 0, "积分消费");
				setbit(&iso, 3, (unsigned char *)"000000", 6);
				setbit(&iso, 25, (unsigned char *)"65", 2);
				
				memset(tmpbuf, 0, sizeof(tmpbuf));
				s = getbit(&iso, 48, (unsigned char *)tmpbuf);
				
				if ( s <= 0 )
				{
					 dcs_log(0, 0, "无二维码促销信息。");
					 memset(tmpbuf, 0x00, sizeof(tmpbuf));
					 memcpy(tmpbuf, "DDL", 3);
					 setbit(&iso, 63, (unsigned char *)tmpbuf, 5);
					 
				}else
				{
					dcs_log(0, 0, "促销的二维码信息：%s", tmpbuf);
					
					//sprintf(tmpbuf, "%s%s", "DDL", "亲爱的会员，您在duiduila上购买了买100就送50的兑兑券，验证码为：238182#34，使用时间为2013-07-01到2013-12-31，请及时使用，以免过期。");
					//memset( bcdbuf, 0x0, sizeof( bcdbuf ) );
					//asc_to_bcd( (unsigned char*) bcdbuf, (unsigned char*) tmpbuf, s, 0 );
					//s = (s + 1) / 2;
					//memcpy( buffer + slen, bcdbuf, s );
					
					//TODO ... 暂时写死，不回POS信息，因为核心返回的是UTF-8编码
					memset(tmpbuf, 0x00, sizeof(tmpbuf));
					memcpy(tmpbuf, "DDL", 3);
					setbit(&iso, 63, (unsigned char *)tmpbuf, 5);
				}
			}
			else if(memcmp(pep_jnls.trancde, "D03", 3)==0 || memcmp(pep_jnls.trancde, "D04", 3)==0)
			{
				setbit(&iso, 3, (unsigned char *)"330000", 6);
				setbit(&iso, 25, (unsigned char *)"65", 2);
				//setbit(&iso, 63, (unsigned char *)NULL, 0);
				memset(tmpbuf, 0, sizeof(tmpbuf));
				s = getbit(&iso, 48, (unsigned char *)tmpbuf);
				
				if ( s <= 0 )
				{
					 dcs_log(0, 0, "无商品描述信息。");
					 memset(tmpbuf, 0x00, sizeof(tmpbuf));
					 memcpy(tmpbuf, "DDL", 3);
					 setbit(&iso, 63, (unsigned char *)tmpbuf, 5);
					 
				}else
				{
					dcs_log(0, 0, "商品描述信息：%s", tmpbuf);
					/*
					memset(ascbuf, 0x00, sizeof(ascbuf));
					sprintf(ascbuf, "%04d%s", s, tmpbuf);
					memset(bcdbuf, 0x00, sizeof(bcdbuf));
					asc_to_bcd((unsigned char*) bcdbuf, (unsigned char*)ascbuf, s+4, 0);
					dcs_log(0, 0, "bcdbuf =[%s]", bcdbuf);
					//TODO ... 暂时写死，不回POS信息，因为核心返回的是UTF-8编码
					memset(buffer, 0x00, sizeof(buffer));
					memcpy(buffer, "DDL", 3);
					s = (s+5)/2;
					memcpy(buffer+3, bcdbuf, s);*/
					
					memset(ascbuf, 0x00, sizeof(ascbuf));
					sprintf(ascbuf, "%04d", s);
					memset(bcdbuf, 0x00, sizeof(bcdbuf));
					asc_to_bcd((unsigned char*) bcdbuf, (unsigned char*)ascbuf, 4, 0);
					dcs_log(0, 0, "bcdbuf =[%s]", bcdbuf);
					memset(buffer, 0x00, sizeof(buffer));
					memcpy(buffer, "DDL", 3);
					memcpy(buffer+3, bcdbuf, 2);
					memcpy(buffer+5, tmpbuf, s);
					setbit(&iso, 63, (unsigned char *)buffer, s+5);
				}
				dcs_log(0, 0, "用户认证");
			}
			else if(memcmp(pep_jnls.trancde, "D02", 3)==0)
			{
				setbit(&iso, 3, (unsigned char *)"310000", 6);
				setbit(&iso, 25, (unsigned char *)"65", 2);
				setbit(&iso, 63, (unsigned char *)NULL, 0);
				dcs_log(0, 0, "积分查询");
			}
		}
		if(memcmp(pep_jnls.process, "200000", 6) == 0)//消费撤销交易
		{
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy(tmpbuf, "23", 2);
			memcpy(tmpbuf+2, pep_jnls.batch_no, 6);
			setbit(&iso, 60, (unsigned char *)tmpbuf, 8);
			setbit(&iso, 63, (unsigned char *)"CUP", 3);
		}
		if(memcmp(pep_jnls.process, "270000", 6) == 0)//消费退货交易
		{
			setbit(&iso, 0, (unsigned char *)"0230", 4);
			setbit(&iso, 3, (unsigned char *)"200000", 6);
			setbit(&iso, 63, (unsigned char *)"CUP", 3);
			if(memcmp(pep_jnls.trancde, "002", 3) == 0)
			{
				dcs_log(0, 0, "联机退货交易");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				memcpy(tmpbuf, "25", 2);
				memcpy(tmpbuf+2, pep_jnls.batch_no, 6);
				setbit(&iso, 60, (unsigned char *)tmpbuf, 8);
			}
			else if(memcmp(pep_jnls.trancde, "051", 3) == 0)
			{
				dcs_log(0, 0, "脱机退货交易");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				memcpy(tmpbuf, "27", 2);
				memcpy(tmpbuf+2, pep_jnls.batch_no, 6);
				setbit(&iso, 60, (unsigned char *)tmpbuf, 8);
				setbit(&iso, 25, (unsigned char *)"00", 2);
			}
		}
		if(memcmp(pep_jnls.trancde, "030", 3) == 0)//预授权交易
		{
			dcs_log(0, 0, "预授权交易");
			setbit(&iso, 0, (unsigned char *)"0110", 4);
			setbit(&iso, 3, (unsigned char *)"030000", 6);
			
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy(tmpbuf, "10", 2);
			memcpy(tmpbuf+2, pep_jnls.batch_no, 6);
			setbit(&iso, 60, (unsigned char *)tmpbuf, 8);
			setbit(&iso, 63, (unsigned char *)"CUP", 3);
		}
		else if(memcmp(pep_jnls.trancde, "031", 3) == 0)//预授权撤销交易
		{
			dcs_log(0, 0, "预授权撤销交易");
			setbit(&iso, 0, (unsigned char *)"0110", 4);
			setbit(&iso, 3, (unsigned char *)"200000", 6);
			
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy(tmpbuf, "11", 2);
			memcpy(tmpbuf+2, pep_jnls.batch_no, 6);
			setbit(&iso, 60, (unsigned char *)tmpbuf, 8);
			setbit(&iso, 63, (unsigned char *)"CUP", 3);
		}
		else if(memcmp(pep_jnls.trancde, "032", 3) == 0)//预授权完成交易
		{
			dcs_log(0, 0, "预授权完成请求交易");
			setbit(&iso, 0, (unsigned char *)"0210", 4);
			setbit(&iso, 3, (unsigned char *)"000000", 6);
			
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy(tmpbuf, "20", 2);
			memcpy(tmpbuf+2, pep_jnls.batch_no, 6);
			setbit(&iso, 60, (unsigned char *)tmpbuf, 8);
			setbit(&iso, 63, (unsigned char *)"CUP", 3);
		}
		else if(memcmp(pep_jnls.trancde, "033", 3) == 0)//预授权完成撤销交易
		{
			dcs_log(0, 0, "预授权完成撤销交易");
			setbit(&iso, 0, (unsigned char *)"0210", 4);
			setbit(&iso, 3, (unsigned char *)"200000", 6);
			
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy(tmpbuf, "21", 2);
			memcpy(tmpbuf+2, pep_jnls.batch_no, 6);
			setbit(&iso, 60, (unsigned char *)tmpbuf, 8);
			setbit(&iso, 63, (unsigned char *)"CUP", 3);
		}
		//TODO ... 需要注意这个清算日期，必须返回POS。但是如果银联或者银行没返回的时候，需要考虑；暂时返回0000
		if(getstrlen(pep_jnls.settlementdate) > 0 && getstrlen(pep_jnls.settlementdate) == 4)
			setbit(&iso, 15, (unsigned char *)pep_jnls.settlementdate, 4);
		else
			setbit(&iso, 15, (unsigned char *)"0000", 4);
			
		if(memcmp(pep_jnls.process, "190000", 6) == 0||memcmp(pep_jnls.process, "480000", 6) == 0)//信用卡还款和电银转账
		{
			memset(tmpbuf, 0x30, sizeof(tmpbuf));
			tmpbuf[13] = 0;
			pep_jnls.filed28[getstrlen(pep_jnls.filed28)]=0;
			rtn = DecAmt(pep_jnls.tranamt, pep_jnls.filed28+1, tmpbuf);
			if(rtn == -1)
			{
				dcs_log(0, 0, "求减错误");
				return;
			}
			setbit(&iso, 4, (unsigned char *)tmpbuf+1, 12);
			
			/*setbit(&iso, 4, (unsigned char *)pep_jnls.tranamt, 12);*/
			setbit(&iso, 15, (unsigned char *)NULL, 0);
			setbit(&iso, 60, (unsigned char *)NULL, 0);
			setbit(&iso, 41, (unsigned char *)pep_jnls.postermcde, 8);
			setbit(&iso, 42, (unsigned char *)pep_jnls.posmercode, 15);
			setbit(&iso, 45, (unsigned char *)pep_jnls.outcdno, getstrlen(pep_jnls.outcdno));//转出卡号
			setbit(&iso, 46, (unsigned char *)pep_jnls.intcdno, getstrlen(pep_jnls.intcdno));//转入卡号
			if(memcmp(pep_jnls.process, "480000", 6) == 0)//电银转账
			{
				dcs_log(0, 0, "电银转账");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				sprintf(tmpbuf, "%s%s%s", pep_jnls.intcdbank, "|", pep_jnls.intcdname);
				setbit(&iso, 59, (unsigned char *)tmpbuf, strlen(tmpbuf));//卡主信息
			}
			else
			{
				dcs_log(0, 0, "信用卡还款");
			}
			
			//信用卡还款和转账汇款返回POS机时，不返回63域
			//he.qingqi@chinaebi.com
			setbit(&iso, 63, (unsigned char *)NULL, 0);
			
		}
		if(memcmp(pep_jnls.trancde, "04", 2)==0 && memcmp(pep_jnls.trancde, "046", 3)!=0)//脚本通知交易
		{
			dcs_log(0, 0, "脚本通知交易");
			setbit(&iso, 0, (unsigned char *)"0630", 4);
			setbit(&iso, 3, (unsigned char *)pep_jnls.process, 6);
			setbit(&iso, 41, (unsigned char *)pep_jnls.postermcde, 8);
			setbit(&iso, 42, (unsigned char *)pep_jnls.posmercode, 15);
		}
		setbit(&iso, 7, (unsigned char *)NULL, 0);
		setbit(&iso, 11, (unsigned char *)pep_jnls.termtrc, 6);
		setbit(&iso, 12, (unsigned char *)pep_jnls.peptime, 6);
		setbit(&iso, 13, (unsigned char *)pep_jnls.pepdate+4, 4);
			
		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy(tmpbuf, pep_jnls.trancde, 3);
		if(memcmp(pep_jnls.billmsg, "DY", 2)== 0)
		{
			len = 0;
			sprintf(tmpbuf+3, "%02d", len);
			setbit(&iso, 20, (unsigned char *)tmpbuf, 5);
		}
		else
		{
			len = getstrlen(pep_jnls.billmsg);	
			sprintf(tmpbuf+3, "%02d", len);
			memcpy(tmpbuf+5, pep_jnls.billmsg, len);
			setbit(&iso, 20, (unsigned char *)tmpbuf, 5+len);
		}
		if(getstrlen(pep_jnls.syseqno)==0)
		{
			memset(tmpbuf, 0, sizeof(tmpbuf));
			sprintf(tmpbuf, "%06ld%s", pep_jnls.trace, pep_jnls.peptime);
			setbit(&iso, 37, (unsigned char *)tmpbuf, 12);
		}
		
		if(memcmp(pep_jnls.trancde, "000", 3)==0 
		|| memcmp(pep_jnls.trancde, "001", 3)==0 
		||memcmp(pep_jnls.trancde, "002", 3)==0)
		{
			rtn = GetNewMerInfo(new_mercode, new_termcde, pep_jnls.termidm);
			if(rtn == 0)
			{
				#ifdef __LOG__
					dcs_log(0, 0, "new_mercode=[%s]", new_mercode);
					dcs_log(0, 0, "new_termcde=[%s]", new_termcde);
				#endif
				setbit(&iso, 41, (unsigned char *)new_termcde, 8);
				setbit(&iso, 42, (unsigned char *)new_mercode, 15);
			}
			else if(rtn < 0)
			{
				dcs_log(0, 0, "特殊商户查询数据表失败");
				setbit(&iso, 39, (unsigned char *)"96", 2);
			}	
		}
		
		//addforic at 0611:IC卡余额查询交易返回如果存在55域，则保存，否则跳过保存动作
		/*
		s = getbit(&iso, 22, (unsigned char *)pep_jnls.poitcde);
		if(s<=0)
		{
			dcs_log(0, 0, "get bit_22 error");
			return;
		}
	
		dcs_log(0, 0, "get bit_22 pep_jnls.poitcde[%s]", pep_jnls.poitcde);

		if(memcmp(pep_jnls.poitcde, "05", 2) == 0 || memcmp(pep_jnls.poitcde, "95", 2) == 0)//此交易为IC卡交易
		{

			s = GetIcinfo(pep_jnls, &pep_icinfo);
			if(s < 0)
	      			dcs_log(0, 0, "查询数据库失败!");
		      	else
		      	{
		      		len=getstrlen(pep_icinfo.recv_bit55);
		      		dcs_log(0, 0, "查询数据库成功!");
		      		dcs_log(0, 0, "GetIcinfo pep_icinfo.recv_bit55:[%s], [%d]!\n ", pep_icinfo.recv_bit55, len);
		      		
		      		memset(filed55, 0x00, sizeof(filed55));
		      		asc_to_bcd((unsigned char *)filed55, (unsigned char*)pep_icinfo.recv_bit55, len, 0);
		      		setbit(&iso, 55, (unsigned char *)filed55, len/2);
		      		dcs_debug(filed55, len/2, "filed55 :[%d]:", len/2);
				memset(filed55, 0x00, sizeof(filed55));	
				getbit(&iso, 55, (unsigned char *)filed55);
				dcs_debug(filed55,len+16, "filed55 :[%d]:", len+16);
		      	}
		}
*/	
		setbit(&iso, 21, (unsigned char *)NULL, 0);
		setbit(&iso, 22, (unsigned char *)NULL, 0);
		setbit(&iso, 26, (unsigned char *)NULL, 0);
		setbit(&iso, 48, (unsigned char *)NULL, 0);
		
		setbit(&iso, 64, (unsigned char *)"00000000", 8);

		//重置44域
		reSet44Field(&iso);

		//测试信用卡还款
		//if(memcmp(pep_jnls.aprespn ,"F3",2) == 0)
			//setbit(&iso, 39, (unsigned char *)"00", 2);
		
		memset(macblock, 0, sizeof(macblock));
		memset(posmac, 0, sizeof(posmac));
				
		len = GetPospMacBlk(&iso, macblock);
		
//		dcs_debug(macblock, len, "GetPospMacBlk Rtn len=%d.", len );
		
		memset(&mer_info_t, 0, sizeof(MER_INFO_T));
		memcpy(mer_info_t.mgr_mer_id, pep_jnls.posmercode, 15);
		memcpy(mer_info_t.mgr_term_id, pep_jnls.postermcde, 8);
		
		rtn = GetPosInfo(&mer_info_t, pep_jnls.termidm);
		if(rtn < 0)
      		dcs_log(0,0,"获取终端配置信息错误");
      		//beigin------
      	if(mer_info_t.pos_machine_type == 4)
  			mer_info_t.encrpy_flag = 1;
  		else
  			mer_info_t.encrpy_flag = 0;//lp20121212 end-----

		s = GetPospMAC(macblock, len, mer_info_t.mgr_mak, posmac);

#ifdef __LOG__
		dcs_log(0, 0, "mer_info_t.encrpy_flag = [%d]", mer_info_t.encrpy_flag );
		dcs_debug( posmac, 8, "posmac :");
#endif
		//setbit(&iso, 64, (unsigned char *)posmac, 8);
		
		memcpy(macblock + len, posmac, 8);
		
//		dcs_debug(macblock, len+8, "writePos without tpdu len=%d.", len+8 );
		
		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy(tmpbuf, mer_info_t.mgr_tpdu, 2);
		memcpy(tmpbuf+2, pep_jnls.trnmsgd, 4);
		memcpy(tmpbuf+6, mer_info_t.mgr_tpdu+2, 4);
		
		/*detail*/
		memcpy(tmpbuf+10, "60310", 5);
		if(mer_info_t.pos_update_flag==1 && memcmp(mer_info_t.cur_version_info, mer_info_t.new_version_info, 4)==0)
			UpdatePos_info(mer_info_t, pep_jnls.termidm, 3);
		flag = GetDetailFlag(&mer_info_t);
		tmpbuf[15]= flag ;
		memcpy(tmpbuf+16, "31", 2);
		if(mer_info_t.pos_update_flag==1 && flag == mer_info_t.pos_update_mode)
			memcpy(tmpbuf+18, mer_info_t.new_version_info, 4);
		else
			memcpy(tmpbuf+18, mer_info_t.cur_version_info, 4);

#ifdef __TEST__
		//if(memcmp(pep_jnls.termidm, "NP811051000020", 14)==0 ||memcmp(pep_jnls.termidm, "NP811081326349", 14)==0)
		//if(memcmp(pep_jnls.termidm, "NP81107102125121013152", 22)==0
		//|| memcmp(pep_jnls.termidm, "NP81107102241721017033", 22)==0)
		//if(memcmp(pep_jnls.termidm, "NP81107102241721017033", 22)==0)
		//if(memcmp(pep_jnls.termidm, "VF328-180-606", 13)==0 
		//	&& (memcmp(pep_jnls.trancde, "000", 3)==0
		//		||memcmp(pep_jnls.trancde, "030", 3)==0))
//		if(memcmp(pep_jnls.termidm, "NP811082000618", 14)==0)
//		{
//			return;
//		}
#endif

		s = writePos(macblock, len+8, pep_jnls.trnsndpid, tmpbuf, mer_info_t);//lp20121212
		if(s < 0)
      		dcs_log(0, 0, "pos 交易写folder失败:%s", ise_strerror(errno));
		return;
	}
	
	if(fold_initsys() < 0)
    {
        dcs_log(0, 0, "cannot attach to folder system!");
        return;
    }

    gs_myFid = pep_jnls.trnsndpid;

    if(gs_myFid < 0)        
    {
        gs_myFid = fold_locate_folder(pep_jnls.trnsndp);
        if(gs_myFid < 0){
			dcs_log(0, 0, "cannot get folder '%s':%s\n", pep_jnls.trnsndp, ise_strerror(errno) );
			return;
        }
    }

	#ifdef __TEST__
    if(fold_get_maxmsg(gs_myFid) < 100)
		fold_set_maxmsg(gs_myFid, 100) ;
 	#else
	if(fold_get_maxmsg(gs_myFid) < 2)
		fold_set_maxmsg(gs_myFid, 20) ;
    #endif
	
	//s = strlen(buffer);
#ifdef __LOG__
	dcs_debug(buffer, s, "send foldId %d ,'%s' data %d length ", pep_jnls.trnsndpid, pep_jnls.trnsndp, s);
#else
	dcs_log(0, 0, "send foldId %d ,'%s' data %d length ", pep_jnls.trnsndpid, pep_jnls.trnsndp, s);
#endif

	rtn = fold_write(gs_myFid, -1, buffer, s);
	
	if(rtn < 0)
      	dcs_log(0, 0, "fold_write() failed:%s!", ise_strerror(errno));
	
	return;
}


int eposAppMsgUnpack( char *srcBuf, ISO_data *iso, int iLen)
{
	char buffer1[1024], tmpbuf[400], bcdbuf[400];
	int s;

	if( iLen < 20 )
	{
		dcs_log(0,0, "eposAppMsgUnpack error");
		return -1;
	}
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(bcdbuf, 0, sizeof(bcdbuf));
	memset(buffer1, 0, sizeof(buffer1));

	memcpy(tmpbuf, srcBuf, 20);
	tmpbuf[19]='0';

	asc_to_bcd((unsigned char *)bcdbuf, (unsigned char *)tmpbuf, 20, 0);
	memcpy(buffer1, bcdbuf, 10);
	

	memcpy(buffer1+10, srcBuf+20, iLen-20);
	
	//if(IsoLoad8583config(&iso8583_conf[0], "iso8583.conf") < 0)
	{
		//dcs_log(0, 0, "load iso8583_pos failed:%s", strerror(errno));
		//return -1;
	}

	//memcpy(iso8583_conf,iso8583conf,sizeof(iso8583conf));
	//iso8583=&iso8583_conf[0];
	pthread_setspecific(iso8583_conf_key, iso8583conf);
	pthread_setspecific(iso8583_key, iso8583conf);
	SetIsoHeardFlag(0);
	SetIsoFieldLengthFlag(1);
	
	if(str_to_iso((unsigned char *)buffer1, iso, iLen-4-10+8)<0) 
	{
		dcs_log(srcBuf, iLen, "cant not analyse got message(%d bytes) from ter-->", iLen);
		return -1;            
	}
	
	return 1;
}

int ProcessReversed(ISO_data iso, PEP_JNLS pep_jnls, EWP_INFO ewp_info, char *buffer)
{
	char tmpbuf[200], batchno[6+1];
	char macblock[1024], posmac[9];
	int rtn, s, len;
	char flag;
	
	MER_INFO_T mer_info_t;
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(batchno, 0, sizeof(batchno));
	memset(&mer_info_t, 0, sizeof(MER_INFO_T));
	
	if(getbit(&iso, 39, (unsigned char *)pep_jnls.ReversedAns)<0)
	{
		dcs_log(0, 0, "get bit_39 error"); 
		return -1;
	}
	if(memcmp(pep_jnls.process, "000000", 6)==0||memcmp(pep_jnls.process, "200000", 6)==0
	|| memcmp(pep_jnls.process, "590000", 6)==0 || memcmp(pep_jnls.process, "500000", 6)==0
	||memcmp(pep_jnls.process, "510000", 6)==0||memcmp(pep_jnls.process, "570000", 6)==0)//冲正交易
	{
		dcs_log(0,0,"处理冲正");
		if(getbit(&iso, 11, (unsigned char *)tmpbuf)<0)
		{
		 	dcs_log(0, 0, "can not get bit_11!");	
		 	return -1;
		}
		sscanf(tmpbuf, "%06ld", &pep_jnls.trace);
		//根据前置系统流水找到原笔交易并更新相应的值
		rtn = Updatepep_jnls(iso, &pep_jnls);
		if(rtn == -1)
		{
			dcs_log(0, 0, "冲正更新数据失败");
			return -1;
		}
		if(memcmp("TRNF", pep_jnls.trnsndp, 4) == 0 ) //转账汇款扣款交易的冲正
		{
			//需要更新转账汇款表的冲正相关字段，更新之后直接退出即可，不需要返回相应系统
			rtn = updateReversal(iso, pep_jnls);
			if(rtn == -1)
			{
				dcs_log(0, 0, "冲正更新数据失败");
				return -1;
			}

			
			//移除设置超时发冲正的定时任务
			char rcvbuf[1024];
			memset(rcvbuf, 0, sizeof(rcvbuf));
			memset(tmpbuf, 0, sizeof(tmpbuf));
			sprintf(tmpbuf, "%s%s%06ld", "PEX_COM", pep_jnls.billmsg, pep_jnls.trace);
			rtn = delete_timeout_control(tmpbuf, rcvbuf, sizeof(rcvbuf)); //测试超时控制超时发送的功能
			if(rtn ==-1)
			{
				dcs_log(0, 0, "移除冲正超时控制失败");
				return -1;
			}	
			if(memcmp(rcvbuf+2, "response=00", 11)==0)
			{
				if(memcmp(rcvbuf+14, "status=2", 8)==0)//2 表示 超时控制已超时发起冲正
				{
					dcs_log(0, 0, "移除成功,已超时发起冲正完毕");
					return -1;
				}	
				else if(memcmp(rcvbuf+14, "status=1", 8)==0||memcmp(rcvbuf+14, "status=0", 8)==0)//0 表示 没找到key对应的超时控制；1表示 移除成功
				{	
					dcs_log(0, 0, "移除超时控制成功,无需继续发起冲正");
				}
			}	
			return 0;
		}
		else if(memcmp("H3CP", pep_jnls.trnsndp, 4) == 0 ) //ewp交易
		{
			/*
			rtn = RdDealcodebyOrderNo(pep_jnls, &ewp_info);
			if(rtn < 0)
			{
				dcs_log(0, 0, "Read ewp_info 交易处理码 error!");
				return -1;
			}
			*/
			rtn = GetInfoFromMem(pep_jnls, &ewp_info);
			if(rtn < 0)
			{
				dcs_log(0, 0, "Read ewp_info from memcached error!");
				return;
			}
			
			memset(buffer, 0, sizeof(buffer));
			//memcpy(ewp_info.consumer_transdealcode, "910000", 6);
			rtn = DataBackToEWP(1, buffer, pep_jnls, ewp_info, iso);
			if(rtn < 0)
			{
				dcs_log(0, 0, "DataBackToEWP error!");
				return -1;
			}
			s = rtn;
		
			if(getstrlen(ewp_info.tpduheader) != 0)
			{
		
				asc_to_bcd((unsigned char *)macblock, (unsigned char*)ewp_info.tpduheader, getstrlen(ewp_info.tpduheader), 0);
			
				memcpy(macblock+5, buffer, s);
				s += 5;
			
				memcpy(buffer, macblock, 1);
				memcpy(buffer+1, macblock+3, 2);
				memcpy(buffer+3, macblock+1, 2);
				memcpy(buffer+5, macblock+5, s-5);
			#ifdef __LOG__
				dcs_debug(buffer, s, "ewp tpdu trade, send foldId %d, '%s' data %d length ", pep_jnls.trnsndpid, pep_jnls.trnsndp, s);
			#endif
			}
			else{
			#ifdef __LOG__
				dcs_debug(buffer, s, "send foldId %d, '%s' data %d length ", pep_jnls.trnsndpid, pep_jnls.trnsndp, s);
			#endif
			}

			rtn = fold_write(pep_jnls.trnsndpid, -1, buffer, s);
			if(rtn < 0)
				dcs_log(0, 0, "fold_write() failed:%s!", ise_strerror(errno));
      		
			return 0;
		}
		//返回pos终端冲正应答0,1,2,3,4,11,12,13,15,25,32,37,39,41,42,49,60,64
		if(memcmp(pep_jnls.ReversedAns, "00", 2)!= 0)
		{
			memset(tmpbuf, 0, sizeof(tmpbuf));
			getbit(&iso, 13, (unsigned char *)tmpbuf);
			setbit(&iso, 15, (unsigned char *)tmpbuf, 4);
						
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy(tmpbuf, pep_jnls.termtrc,6);
			getbit(&iso, 12, (unsigned char *)tmpbuf+6);	
			setbit(&iso, 37, (unsigned char *)tmpbuf, 12);	
			setbit(&iso, 32, (unsigned char *)"40002900", 8);	
			setbit(&iso, 44, (unsigned char *)"00000000   00000000   ", 22);	
		}
#ifdef __LOG__
		dcs_log(0, 0, "pep_jnls.trancde=[%s]", pep_jnls.trancde);	
		dcs_log(0, 0, "pep_jnls.process=[%s]", pep_jnls.process);	
#endif
		memset(tmpbuf, 0, sizeof(tmpbuf));
		if(memcmp(pep_jnls.trancde, "000", 3)==0 || memcmp(pep_jnls.trancde, "C00", 3)==0)
		{
			dcs_log(0, 0, "pos消费冲正");
			memcpy(tmpbuf, "22", 2);
		}
		else if(memcmp(pep_jnls.trancde, "001", 3)==0|| memcmp(pep_jnls.trancde, "C01", 3)==0)
		{
			dcs_log(0, 0, "pos消费撤销冲正");
			memcpy(tmpbuf, "23", 2);
		}
		else if(memcmp(pep_jnls.trancde, "C30", 3)==0)
		{
			dcs_log(0, 0, "pos预授权冲正");
			setbit(&iso, 3, (unsigned char *)"030000", 6);	
			memcpy(tmpbuf, "10", 2);
		}
		else if(memcmp(pep_jnls.trancde, "C31", 3)==0)
		{
			dcs_log(0, 0, "pos预授权撤销冲正");
			setbit(&iso, 3, (unsigned char *)"200000", 6);	
			memcpy(tmpbuf, "11", 2);
		}
		else if(memcmp(pep_jnls.trancde, "C32", 3)==0)
		{
			dcs_log(0, 0, "pos预授权完成冲正");
			setbit(&iso, 3, (unsigned char *)"000000", 6);
			memcpy(tmpbuf, "20", 2);
		}
		else if(memcmp(pep_jnls.trancde, "C33", 3)==0)
		{
			dcs_log(0, 0, "pos预授权完成撤销冲正");
			setbit(&iso, 3, (unsigned char *)"200000", 6);
			memcpy(tmpbuf, "21", 2);
		}
		memcpy(tmpbuf+2, pep_jnls.batch_no, 6);
		setbit(&iso, 60, (unsigned char *)tmpbuf, 8);	
		
		setbit(&iso, 7, (unsigned char *)NULL, 0);		
		setbit(&iso, 11, (unsigned char *)pep_jnls.termtrc, 6);	
		#ifdef __TEST__   	
			setbit(&iso, 2, (unsigned char *)pep_jnls.outcdno, getstrlen(pep_jnls.outcdno));
			s = getbit(&iso, 23, (unsigned char *)tmpbuf);
			if(s > 0)
				setbit(&iso, 23, (unsigned char *)pep_jnls.card_sn, 3);
		#endif
		
		memset(tmpbuf, 0, sizeof(tmpbuf));
		getbit(&iso, 39, (unsigned char *)tmpbuf);
		if(memcmp(tmpbuf, "C5", 2)==0)
			setbit(&iso, 39, (unsigned char *)"25", 2);	
			
		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy(tmpbuf, pep_jnls.trancde, 3);
		if(memcmp(pep_jnls.billmsg, "DY", 2)== 0)
		{
			len = 0;
			sprintf(tmpbuf+3, "%02d", len);
			setbit(&iso, 20, (unsigned char *)tmpbuf, 5);
		}
		else
		{
			len = getstrlen(pep_jnls.billmsg);	
			sprintf(tmpbuf+3, "%02d", len);
			memcpy(tmpbuf+5, pep_jnls.billmsg, len);
			setbit(&iso, 20, (unsigned char *)tmpbuf, 5+len);
		}
		
		setbit(&iso, 21, (unsigned char *)NULL, 0);
		setbit(&iso, 22, (unsigned char *)NULL, 0);
		setbit(&iso, 26, (unsigned char *)NULL, 0);
		setbit(&iso, 48, (unsigned char *)NULL, 0);
		
		setbit(&iso, 64, (unsigned char *)"00000000", 8);
		memset(macblock, 0, sizeof(macblock));
		memset(posmac, 0, sizeof(posmac));
				
		len = GetPospMacBlk(&iso, macblock);
		
//		dcs_debug(macblock, len, "GetPospMacBlk Rtn len=%d.", len );
		
		memset(&mer_info_t, 0, sizeof(MER_INFO_T));
		memcpy(mer_info_t.mgr_mer_id, pep_jnls.mercode, 15);
		memcpy(mer_info_t.mgr_term_id, pep_jnls.termcde, 8);
		
		rtn = GetPosInfo(&mer_info_t, pep_jnls.termidm);
		if(rtn < 0)
      		dcs_log(0, 0, "获取终端配置信息错误");
      		//begin-----
      	if(mer_info_t.pos_machine_type == 4)
  			mer_info_t.encrpy_flag = 1;
  		else
  			mer_info_t.encrpy_flag = 0;	//lp20121212end--------
		s = GetPospMAC(macblock, len, mer_info_t.mgr_mak, posmac);

#ifdef __LOG__
		dcs_debug( posmac, 8, "posmac :");
#endif
		memcpy(macblock + len, posmac, 8);
		
//		dcs_debug(macblock, len+8, "writePos without tpdu len=%d.", len+8 );

		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy(tmpbuf, mer_info_t.mgr_tpdu, 2);
		memcpy(tmpbuf+2, pep_jnls.trnmsgd, 4);
		memcpy(tmpbuf+6, mer_info_t.mgr_tpdu+2, 4);
		
		/*detail*/
		memcpy(tmpbuf+10, "60310", 5);
		if(mer_info_t.pos_update_flag==1 && memcmp(mer_info_t.cur_version_info, mer_info_t.new_version_info, 4)==0)
			UpdatePos_info(mer_info_t, pep_jnls.termidm, 3);
		flag = GetDetailFlag(&mer_info_t);
		tmpbuf[15]= flag ;
		memcpy(tmpbuf+16, "31", 2);
		if(mer_info_t.pos_update_flag==1 && flag == mer_info_t.pos_update_mode)
			memcpy(tmpbuf+18, mer_info_t.new_version_info, 4);
		else
			memcpy(tmpbuf+18, mer_info_t.cur_version_info, 4);
		s = writePos(macblock, len+8, pep_jnls.trnsndpid, tmpbuf, mer_info_t);//lp20121212
		if(s < 0)
      		dcs_log(0, 0, "pos 冲正交易写folder失败:%s", ise_strerror(errno));
		return 0;
	}
	return -1;
}

/*处理余额查询*/
int  ProcessBalance(ISO_data iso, PEP_JNLS pep_jnls, EWP_INFO ewp_info, char *buffer)
{
	char tmpbuf[256+1];
	char macblock[1024], posmac[9];
	char filed55[512+1];
	int rtn=0, s=0, len=0;
	char flag;
	
	MER_INFO_T mer_info_t;
	PEP_ICINFO pep_icinfo;
	
	memset(macblock, 0, sizeof(macblock));
	memset(posmac, 0, sizeof(posmac));
	memset(&mer_info_t, 0, sizeof(MER_INFO_T));
	memset(&pep_icinfo, 0, sizeof(PEP_ICINFO));
	memset(filed55, 0, sizeof(filed55));
	
	rtn = saveOrUpdatePepjnls(iso, &pep_jnls);
	if(rtn == -1)
	{
		dcs_log(0, 0, "余额查询更新数据失败");
		return -1;
	}
	if(memcmp("H3CP", pep_jnls.trnsndp, 4) == 0   //ewp交易
		|| memcmp("MPOS", pep_jnls.trnsndp, 4) == 0 )//mpos交易
	{
		/*
		rtn = RdDealcodebyOrderNo(pep_jnls, &ewp_info);
		if(rtn < 0)
		{
			dcs_log(0, 0, "Read ewp_info 交易处理码 error!");
			return -1;
		}
		*/
		
		rtn = GetInfoFromMem(pep_jnls, &ewp_info);
		if(rtn < 0)
		{
			dcs_log(0, 0, "Read ewp_info from memcached error!");
			return -1;
		}
		
		memset(buffer, 0, sizeof(buffer));
		//盛京惠生活,融易付mpos交易
		if(memcmp("MPOS", pep_jnls.trnsndp, 4) == 0)
		{
			rtn = DataBackToMPOS(1, buffer, pep_jnls, ewp_info, iso);
			if(rtn < 0)
			{
				dcs_log(0, 0, "DataBackToMPOS error!");
				return -1;
			}
		}
		else
		{
			rtn = DataBackToEWP(1, buffer, pep_jnls, ewp_info, iso);
			if(rtn < 0)
			{
				dcs_log(0, 0, "DataBackToEWP error!");
				return -1;
			}
		}
		s = rtn;
		
		if(getstrlen(ewp_info.tpduheader) != 0)
		{	
			asc_to_bcd((unsigned char *)macblock, (unsigned char*)ewp_info.tpduheader, getstrlen(ewp_info.tpduheader), 0);
			
			memcpy(macblock+5, buffer, s);
			s += 5;
			
			memcpy(buffer, macblock, 1);
			memcpy(buffer+1, macblock+3, 2);
			memcpy(buffer+3, macblock+1, 2);
			memcpy(buffer+5, macblock+5, s-5);
			#ifdef __LOG__
				dcs_debug(buffer, s, "ewp tpdu trade, send foldId %d, '%s' data %d length ", pep_jnls.trnsndpid, pep_jnls.trnsndp, s);
			#endif
		}
		else
		{
			#ifdef __LOG__
				dcs_debug(buffer, s, "send foldId %d, '%s' data %d length ", pep_jnls.trnsndpid, pep_jnls.trnsndp, s);
			#endif
		}
		
		rtn = fold_write(pep_jnls.trnsndpid, -1, buffer, s);
		
		if(rtn < 0)
      		dcs_log(0, 0, "fold_write() failed:%s!", ise_strerror(errno));
      		
		return 0;
	}
	
	setbit(&iso, 41, (unsigned char *)pep_jnls.postermcde, 8);
	setbit(&iso, 42, (unsigned char *)pep_jnls.posmercode, 15);
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memcpy(tmpbuf, "01", 2);
	memcpy(tmpbuf+2, pep_jnls.batch_no, 6);
	setbit(&iso, 60, (unsigned char *)tmpbuf, 8);
	
	#ifdef __LOG__
		dcs_log(0, 0, "pep_jnls.termtrc=[%s]", pep_jnls.termtrc);
		dcs_log(0, 0, "pep_jnls.trnsndp=[%s]", pep_jnls.trnsndp);
	#endif
	
	setbit(&iso, 11, (unsigned char *)pep_jnls.termtrc, 6);	
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memcpy(tmpbuf, pep_jnls.trancde, 3);
	if(memcmp(pep_jnls.billmsg, "DY", 2)== 0)
	{
		len = 0;
		sprintf(tmpbuf+3, "%02d", len);
		setbit(&iso, 20, (unsigned char *)tmpbuf, 5);
	}
	else
	{
		len = getstrlen(pep_jnls.billmsg);	
		sprintf(tmpbuf+3, "%02d", len);
		memcpy(tmpbuf+5, pep_jnls.billmsg, len);
		setbit(&iso, 20, (unsigned char *)tmpbuf, 5+len);
	}
	#ifdef __TEST__  	
		setbit(&iso, 2, (unsigned char *)pep_jnls.outcdno, getstrlen(pep_jnls.outcdno));	
		s = getbit(&iso, 23, (unsigned char *)tmpbuf);
		if(s > 0)
			setbit(&iso, 23, (unsigned char *)pep_jnls.card_sn, 3);
	#endif	
	
	//后来规则修改：23域和55域直接透传，不需要执行保存和更新的操作
	/*
	//addforic at 0611:IC卡余额查询交易返回如果存在55域，则保存，否则跳过保存动作
	s = getbit(&iso, 22, (unsigned char *)pep_jnls.poitcde);
	if(s<0)
	{
		dcs_log(0, 0, "get bit_22 error");
		return -1;
	}
	
	dcs_log( 0, 0, "getbit 22 pep_jnls.poitcde[%s]", pep_jnls.poitcde );
     */
	/*	if( memcmp(pep_jnls.poitcde, "05", 2) == 0 || memcmp(pep_jnls.poitcde, "95", 2) == 0)//此交易为IC卡交易
	{

		s = GetIcinfo(pep_jnls, &pep_icinfo);
		if(s < 0)
	    		dcs_log(0, 0, "查询数据库失败!");
	    	else
	   	{
	    	len=getstrlen(pep_icinfo.recv_bit55);
	      	dcs_log(0, 0, "查询数据库成功!");
	      	dcs_log(0, 0, "GetIcinfo pep_icinfo.recv_bit55:[%s],[%d]!\n ", pep_icinfo.recv_bit55, len);
	      		
	      	memset(filed55, 0x00, sizeof(filed55));
	      	asc_to_bcd((unsigned char *)filed55, (unsigned char*)pep_icinfo.recv_bit55, len, 0);
	      	setbit(&iso, 55, (unsigned char *)filed55, len/2);
	      	dcs_debug(filed55, len/2, "filed55 :[%d]:", len/2);
	    	}
	}
   */
	setbit(&iso, 7, (unsigned char *)NULL, 0);	
	setbit(&iso, 21, (unsigned char *)NULL, 0);
	setbit(&iso, 22, (unsigned char *)NULL, 0);
	setbit(&iso, 26, (unsigned char *)NULL, 0);
	setbit(&iso, 48, (unsigned char *)NULL, 0);
	setbit(&iso, 64, (unsigned char *)"00000000", 8);
	
	memset(macblock, 0, sizeof(macblock));
	memset(posmac, 0, sizeof(posmac));
				
	len = GetPospMacBlk(&iso, macblock);
//	dcs_debug(macblock, len, "GetPospMacBlk Rtn len=%d.", len );

	memset(&mer_info_t, 0, sizeof(MER_INFO_T));
	memcpy(mer_info_t.mgr_mer_id, pep_jnls.posmercode, 15);
	memcpy(mer_info_t.mgr_term_id, pep_jnls.postermcde, 8);	
	
	rtn = GetPosInfo(&mer_info_t, pep_jnls.termidm);
	if(rtn < 0)
      	dcs_log(0,0,"获取终端配置信息错误");
    //begin--------
    if(mer_info_t.pos_machine_type == 4)
  		mer_info_t.encrpy_flag = 1;
  	else
  		mer_info_t.encrpy_flag = 0;//lp20121212 end-----
     		
	s = GetPospMAC(macblock, len, mer_info_t.mgr_mak, posmac);
#ifdef __LOG__
	dcs_debug( posmac, 8, "posmac :");
#endif
	memcpy(macblock + len, posmac, 8);
		
//	dcs_debug(macblock, len+8, "writePos without tpdu len=%d.", len+8 );
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memcpy(tmpbuf, mer_info_t.mgr_tpdu, 2);
	memcpy(tmpbuf+2, pep_jnls.trnmsgd, 4);
	memcpy(tmpbuf+6, mer_info_t.mgr_tpdu+2, 4);
	/*detail*/
	memcpy(tmpbuf+10, "60310", 5);
	if(mer_info_t.pos_update_flag==1 && memcmp(mer_info_t.cur_version_info, mer_info_t.new_version_info, 4)==0)
		UpdatePos_info(mer_info_t, pep_jnls.termidm, 3);
	flag = GetDetailFlag(&mer_info_t);
	tmpbuf[15]= flag ;	
	
	/*		
	if(mer_info_t.pos_update_flag==1&&(mer_info_t.para_update_flag=='1'||mer_info_t.para_update_flag=='0'))
		tmpbuf[15]= mer_info_t.pos_update_mode;
	else if(mer_info_t.para_update_flag=='1')
		tmpbuf[15]= mer_info_t.para_update_mode;
	else
		tmpbuf[15]='0';*/
	memcpy(tmpbuf+16, "31", 2);
	if(mer_info_t.pos_update_flag==1 && flag == mer_info_t.pos_update_mode)
		memcpy(tmpbuf+18, mer_info_t.new_version_info, 4);
	else
		memcpy(tmpbuf+18, mer_info_t.cur_version_info, 4);
	
	/*if(mer_info_t.pos_update_flag==1)
		UpdatePos_info(mer_info_t, pep_jnls.termidm, 3);*/
	//memcpy(tmpbuf+10, mer_info_t.mgr_tpdu+10, 12);
	
	s = writePos(macblock, len+8, pep_jnls.trnsndpid, tmpbuf, mer_info_t);//lp20121212
	//s = writePos(macblock, len+8, pep_jnls.trnsndpid, tmpbuf);
		
	//setbit(&iso, 64, (unsigned char *)posmac, 8);
	//s = sndBackToPos(iso, pep_jnls.trnsndpid, mer_info_t);
	
	if(s < 0)
      	dcs_log(0, 0, "pos 交易写folder失败:%s", ise_strerror(errno));
	return 0;
}

int SendMsg(PEP_JNLS pep_jnls)
{
	int smsDataLen = 0, smsFolderId = 0, result;
	char smsData[150], tmpbuf[27];
	SMSBody smsbody;
	
	memset(smsData, 0, sizeof(smsData));
	memset(&smsbody, 0, sizeof(SMSBody));
	memset(tmpbuf, 0, sizeof(tmpbuf));
	
	memcpy(smsbody.trancde, pep_jnls.trancde, 3);
	
	if(getstrlen(pep_jnls.outcdno) != 0 && getstrlen(pep_jnls.outcdno) <= 19)
		memcpy(smsbody.outcard, pep_jnls.outcdno, getstrlen(pep_jnls.outcdno));
	
	if(getstrlen(pep_jnls.intcdno) != 0 && getstrlen(pep_jnls.intcdno) <= 19)
		memcpy(smsbody.incard, pep_jnls.intcdno, getstrlen(pep_jnls.intcdno));
		
	if(getstrlen(pep_jnls.tranamt) == 12)
		memcpy(smsbody.tradeamt, pep_jnls.tranamt, 12);
	
	if(getstrlen(pep_jnls.filed28) == 9)
	{
		memcpy(smsbody.tradefee, pep_jnls.filed28+1, 8);
	}
	
	memcpy(tmpbuf, pep_jnls.pepdate, 8);
	memcpy(tmpbuf+8, pep_jnls.peptime, 6);
	memcpy(smsbody.tradedate, tmpbuf, 14);
	
	memcpy(smsbody.traderesp, pep_jnls.aprespn, 2);
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	sprintf(tmpbuf, "%06ld", pep_jnls.trace);
	memcpy(smsbody.tradetrace, tmpbuf, 6);
	
	memcpy(smsbody.traderefer, pep_jnls.syseqno, 12);
	
	if(getstrlen(pep_jnls.billmsg) > 0 && getstrlen(pep_jnls.billmsg) <= 30)
		memcpy(smsbody.tradebillmsg, pep_jnls.billmsg, getstrlen(pep_jnls.billmsg));
	
	if(getstrlen(pep_jnls.modecde) != 11)
	{
		dcs_log(0,0,"sms phone no length error.");
		return -1;
	}
	memcpy(smsbody.userphone, pep_jnls.modecde, 11);
	
	smsDataLen = SmsBody2Byte(smsbody, smsData);
	if(smsDataLen > 0)
	{
		smsFolderId = fold_locate_folder("SMSRCV");
		if(smsFolderId < 0)
		{
			dcs_log(0,0, "fold_locate_folder SMSRCV error , resend nextime ...");
			return -1;	   		
		}
		else
		{
			result = fold_write(smsFolderId, -1, smsData, smsDataLen);
		    if(result <0)
		    {
		    	dcs_log(0,0, "write to folder error resend nextime ...");	
		    	return -1;
		    }
	    }
    }
    else
    {
		dcs_log(0,0, "SmsBody2Byte Length Error.");	
		return -1;
    }
    dcs_log(0,0, "SendMsg success!");
    return 0;
}

//从memcached服务器获取信息
int GetInfoFromMem(PEP_JNLS pep_jnls, EWP_INFO *ewp_info)
{
	//memcached存取数据
	char value[1024], tmpbuf[1024];
	int *value_len, datalen = 0;
	int rtn = 0;
	
	//保存到缓存数据memcached结构体
	KEY_VALUE_INFO key_value_info;
	
	memset(&key_value_info, 0, sizeof(KEY_VALUE_INFO));
	memset(value, 0x00, sizeof(value));
	
	sprintf(key_value_info.keys, "%s%s%s", pep_jnls.trndate, pep_jnls.trntime, pep_jnls.billmsg);
	rtn = Mem_GetKey(key_value_info.keys, value, value_len, sizeof(value));
	if(rtn == -1)
	{
		dcs_log(0, 0, "get memcached value error");
		return -1;
	}
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	datalen = getnode(value, "transdealcode", tmpbuf);
	tmpbuf[datalen] =0;
	memcpy(ewp_info->consumer_transdealcode, tmpbuf, datalen);
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	datalen = getnode(value, "translaunchway", tmpbuf);
	tmpbuf[datalen] =0;
	memcpy(ewp_info->translaunchway, tmpbuf, datalen);
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	datalen = getnode(value, "selfdefine", tmpbuf);
	tmpbuf[datalen] =0;
	memcpy(ewp_info->selfdefine, tmpbuf, datalen);
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	datalen = getnode(value, "incdname", tmpbuf);
	tmpbuf[datalen] =0;
	memcpy(ewp_info->incdname, tmpbuf, datalen);
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	datalen = getnode(value, "incdbkno", tmpbuf);
	tmpbuf[datalen] =0;
	memcpy(ewp_info->incdbkno, tmpbuf, datalen);
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	datalen = getnode(value, "transfee", tmpbuf);
	tmpbuf[datalen] =0;
	memcpy(ewp_info->transfee, tmpbuf, datalen);
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	datalen = getnode(value, "psamid", tmpbuf);
	tmpbuf[datalen] =0;
	memcpy(ewp_info->consumer_psamno, tmpbuf, datalen);
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	datalen = getnode(value, "tpduheader", tmpbuf);
	tmpbuf[datalen] =0;
	memcpy(ewp_info->tpduheader, tmpbuf, datalen);
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	datalen = getnode(value, "sentinstitu", tmpbuf);
	tmpbuf[datalen] =0;
	memcpy(ewp_info->consumer_sentinstitu, tmpbuf, datalen);
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	datalen = getnode(value, "poitcde_flag", tmpbuf);
	tmpbuf[datalen] =0;
	memcpy(ewp_info->poitcde_flag, tmpbuf, datalen);
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	datalen = getnode(value, "transtype", tmpbuf);
	tmpbuf[datalen] =0;
	memcpy(ewp_info->consumer_transtype, tmpbuf, datalen);
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	datalen = getnode(value, "orderno", tmpbuf);
	tmpbuf[datalen] =0;
	if(memcmp(pep_jnls.billmsg, tmpbuf, datalen) ==0)
		dcs_log(0, 0, "set or get success");
	return 0;
}

/* 重置返回POS的报文44域
 * 当核心返回的44域中的前8个字节为全0表示没有返回接收机构即发卡行机构号，此时需要前置系统根据2域卡号查询卡宾表（card_transfer）获取发卡行机构号的前4位
 * 并将后四位补0作为44域前8个字节返回POS机，用于打印在签购单上的收单行部分
 * */
int reSet44Field(ISO_data *iso)
{
	char buf[25+1],cardNo[20+1],bankNo[8+1];
	int s=0;
	memset(buf,0x00, sizeof(buf));
	memset(cardNo,0x00, sizeof(cardNo));
	memset(bankNo,0x00, sizeof(bankNo));
	//首先判断是否需要重新给44域赋值
	s= getbit(iso, 44, (unsigned char *)buf);
	if(s <22  )
	{
	#ifdef __LOG__
	    dcs_log(0, 0, "无44域");
	#endif
	    return 0;
	}
	if(memcmp(buf,"00000000",8) !=0)
	{
	#ifdef __LOG__
	    dcs_log(0, 0, "44域发卡行有信息，无需重新赋值");
	#endif
	    return 0;
	}
	s= getbit(iso, 2, (unsigned char *)cardNo);
	if(s <= 0  )
	{
	#ifdef __LOG__
		dcs_log(0, 0, "无2域");
	#endif
		return 0;
	}
	//根据卡号查询
	s= ReadCardBankNo(cardNo,bankNo);
	if(s<0 || strlen(bankNo)<8)
	{
		return -1;
	}
	//重置44域
	memcpy(buf, bankNo,4);
	#ifdef __LOG__
		dcs_log(0, 0, "新的44域[%s]",buf);
	#endif
	setbit(iso, 44, (unsigned char *)buf, strlen(buf));
	return 0;
}

/*向超时控制里添加一条记录*/
int SetTimeOutControl(PEP_JNLS pep_jnls, ISO_data iso)
{
	int rtn = 0, s = 0;
	EWP_INFO ewp_info;
	struct TIMEOUT_INFO timeout_info;
	char buffer[1024], tempbuf[2048], rcvbuf[128];
	
	memset(buffer, 0x00, sizeof(buffer));
	memset(tempbuf, 0x00, sizeof(tempbuf));
	memset(&ewp_info, 0x00, sizeof(EWP_INFO));
	memset(&timeout_info, 0x00, sizeof(struct TIMEOUT_INFO));
	memset(rcvbuf, 0x00, sizeof(rcvbuf));
	
	rtn = GetInfoFromMem(pep_jnls, &ewp_info);
	if(rtn < 0)
	{
		dcs_log(0, 0, "Read ewp_info from memcached error!");
		return -1;
	}
		
	rtn = DataReturnToEWP(1, buffer, pep_jnls, ewp_info, iso); //return back data length
	if(rtn <= 0)
	{
		dcs_log(0, 0, "DataReturnToEWP error!");
		return -1;
	}
	s = rtn;	
	if(getstrlen(ewp_info.tpduheader) != 0)
	{	
		asc_to_bcd((unsigned char *)tempbuf, (unsigned char*)ewp_info.tpduheader, getstrlen(ewp_info.tpduheader), 0);	
		memcpy(tempbuf+5, buffer, s);
		s += 5;
			
		memcpy(buffer, tempbuf, 1);
		memcpy(buffer+1, tempbuf+3, 2);
		memcpy(buffer+3, tempbuf+1, 2);
		memcpy(buffer+5, tempbuf+5, s-5);	
	}	
	
	memcpy(timeout_info.key, pep_jnls.billmsg, strlen(pep_jnls.billmsg));//主键
	timeout_info.timeout = 60;
	timeout_info.sendnum = 1;
	timeout_info.foldid = pep_jnls.trnsndpid;
	timeout_info.length = 2*s;
	
	bcd_to_asc((unsigned char *)timeout_info.data, (unsigned char*)buffer, 2*s, 0);

	add_timeout_control(timeout_info, rcvbuf, sizeof(rcvbuf));

	if(memcmp(rcvbuf+2, "response=00", 11)==0)
	{
		dcs_log(0, 0, "代付失败冲正超时控制添加 success");
	}
	else
	{
		dcs_log(0, 0, "代付失败冲正超时控制添加 false rcvbuf =[%s]", rcvbuf);
		return -1;
	}	
	return 0;
}

/*
处理代付失败的情况
*/
int dealPayFalse(PEP_JNLS pep_jnls, TRANSFER_INFO transfer_info, EWP_INFO ewp_info, ISO_data iso)
{
	char rcvbuf[128];
	int rtn = 0;
	
	memset(rcvbuf, 0x00, sizeof(rcvbuf));
	
	sendReversal(pep_jnls);//发起冲正
	memcpy(pep_jnls.aprespn, "0F", 2);
	
	//先移除超时控制
	rtn = delete_timeout_control(transfer_info.billmsg, rcvbuf, sizeof(rcvbuf));
	if(rtn ==-1)
	{
		return -1;
	}
	if(memcmp(rcvbuf+2, "response=00", 11)==0)
	{
		if(memcmp(rcvbuf+14, "status=2", 8)==0)//2 表示 超时控制已超时返回
		{
			dcs_log(0, 0, "移除成功,已超时返回,此处不再返回");
			return 0;
		}
		else if(memcmp(rcvbuf+14, "status=1", 8)==0||memcmp(rcvbuf+14, "status=0", 8)==0)//0 表示 没找到key对应的超时控制；1表示 移除成功
		{
			//更新返回终端的应答码
			memcpy(transfer_info.pay_respn, "0F", 2);
			memcpy(transfer_info.orderId, transfer_info.billmsg, strlen(transfer_info.billmsg));
			updateResponseCode(transfer_info);
			dcs_log(0, 0, "移除超时控制成功");
		}
	}
	rtn = RetToEWP(iso, ewp_info, pep_jnls, 1);//返回EWP
	if(rtn ==-1)
	{
		dcs_log(0, 0, "返回EWP失败");
		return -1;
	}
}
