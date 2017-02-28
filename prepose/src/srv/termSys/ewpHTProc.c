#include "trans.h" 
#include <string.h>
#include "ibdcs.h"
/*
	EWP 历史交易查询 
	@see 《前置系统接入报文规范.docx》 V9.4
*/
void ewpHTProc(char *srcBuf, int iFid, int iMid, int iLen)
{
	EWP_HIST_RQ ewp_hist_rq;
	char buffer[2048];
	int rtn= 0, s=0;
	
	memset(&ewp_hist_rq, 0, sizeof(EWP_HIST_RQ));
	memset(buffer, 0, sizeof(buffer));

	#ifdef __LOG__
		dcs_debug(srcBuf, iLen, "ewpHTProc交易, iFid=%d, iMid=%d, iLen=%d", iFid, iMid, iLen);
	#endif
	
	
	// 一 TODO  解析EWP交易报文
	parsequeryrq(srcBuf, iLen, &ewp_hist_rq);
	if(memcmp(ewp_hist_rq.retcode, "00", 2)!= 0 )
	{
		dcs_log(0, 0, "解析账单信息失败");
		return;
	}
	
	//历史交易信息查询
	if(memcmp(ewp_hist_rq.process, "FF0000", 6)==0 || memcmp(ewp_hist_rq.process, "FF0001", 6)==0 || memcmp(ewp_hist_rq.process, "FF0002", 6)==0)
	{
		// 二 TODO 分页，查询数据库
		if(memcmp(ewp_hist_rq.queryflag, "0", 1)==0||memcmp(ewp_hist_rq.queryflag, "1", 1)==0||memcmp(ewp_hist_rq.queryflag, "2", 1)==0)
		{
			rtn = GetQueryResult(ewp_hist_rq, buffer);
			if(rtn <0)
			{
				dcs_log(0, 0, "查询历史信息失败 rtn = [%d]", rtn);
				return;
			}
		}
		/*可撤销交易查询*/
		else if(memcmp(ewp_hist_rq.queryflag, "3", 1)==0)
		{
			rtn = GetMposQueryResult(ewp_hist_rq, buffer);
			if(rtn <0)
			{
				dcs_log(0, 0, "查询可撤销交易失败 rtn = [%d]", rtn);
				return;
			}
		}
	}
	/*交易状态查询，消费撤销、退货、末笔原笔交易查询*/
	else if(memcmp(ewp_hist_rq.process, "9F0000", 6)==0 || memcmp(ewp_hist_rq.process, "9F0001", 6)==0 
		|| memcmp(ewp_hist_rq.process, "9F0002", 6)==0 || memcmp(ewp_hist_rq.process, "9F0003", 6)==0
		|| memcmp(ewp_hist_rq.process, "9F0004", 6)==0)
	{
		rtn = GetRrEnQueryResult(ewp_hist_rq, buffer);
		if(rtn <0)
		{
			dcs_log(0, 0, "查询撤销退货末笔交易失败 rtn = [%d]", rtn);
			return;
		}
		
	}
	else
	{
		dcs_log(0, 0, "请求参数错误");
		return;
	}
	// 三 返回EWP数据结果
	s = fold_write(iFid, -1, buffer, rtn);
	if(s < 0 )
	{
		dcs_log(0, 0, "写入fold error");
		return;
	}
	return;
}

/*解析从ewp那里接收到的数据历史交易查询请求，存放到结构体EWP_HIST_RQ中*/
int parsequeryrq(char *rcvbuf, int rcvlen, EWP_HIST_RQ *ewp_hist_rq)
{
	int check_len, offset;

	check_len = 0;
	offset = 0;
	check_len = rcvlen;

	/*交易类型*/
	memcpy(ewp_hist_rq->transtype, rcvbuf + offset, 4);
	ewp_hist_rq->transtype[4] = 0;
	offset += 4;
	check_len -= 4;
	
	/*交易处理码*/
	memcpy(ewp_hist_rq->process, rcvbuf + offset, 6);
	ewp_hist_rq->process[6] = 0;
	offset += 6;
	check_len -= 6;
	
	#ifdef __LOG__
		dcs_log(0, 0, "transtype=[%s], process=[%s]", ewp_hist_rq->transtype, ewp_hist_rq->process);
	#endif
	
	//新增一体机原笔交易查询接口
	if(memcmp(ewp_hist_rq->process, "9F000", 5) == 0)
	{
		/*查询交易日期 8*/
		memcpy(ewp_hist_rq->selectdate, rcvbuf + offset, 8);
		ewp_hist_rq->selectdate[8] = 0;
		offset += 8;
		check_len -= 8;
		
		/*发送时间 6*/
		memcpy(ewp_hist_rq->sendtime, rcvbuf + offset, 8);
		ewp_hist_rq->sendtime[6] = 0;
		offset += 6;
		check_len -= 6;
		
		#ifdef __LOG__
			dcs_log(0, 0, "selectdate=[%s], sendtime=[%s]", ewp_hist_rq->selectdate, ewp_hist_rq->sendtime);
		#endif
		
		/*发送机构号*/
		memcpy(ewp_hist_rq->sendcode, rcvbuf + offset, 4);
		ewp_hist_rq->sendcode[4] = 0;
		offset += 4;
		check_len -= 4;
		#ifdef __LOG__
			dcs_log(0, 0, "sendcode=[%s]", ewp_hist_rq->sendcode);
		#endif
		
		/*交易码3*/
		memcpy(ewp_hist_rq->trancde, rcvbuf + offset, 3);
		ewp_hist_rq->trancde[3] = 0;
		offset += 3;
		check_len -= 3;
		
		/*PSAM卡号16*/
		memcpy(ewp_hist_rq->psam, rcvbuf + offset, 16);
		ewp_hist_rq->psam[16] = 0;
		offset += 16;
		check_len -= 16;
		
		#ifdef __LOG__
			dcs_log(0, 0, "trancde=[%s], psam=[%s]", ewp_hist_rq->trancde, ewp_hist_rq->psam);
		#endif
		
		/*机具编号20*/
		memcpy(ewp_hist_rq->termidm, rcvbuf + offset, 20);
		ewp_hist_rq->termidm[20] = 0;
		offset += 20;
		check_len -= 20;
		
		/*用户名N11*/
		memcpy(ewp_hist_rq->username, rcvbuf + offset, 11);
		ewp_hist_rq->username[11] = 0;
		offset += 11;
		check_len -= 11;
		#ifdef __LOG__
			dcs_log(0, 0, "termidm=[%s], username=[%s]", ewp_hist_rq->termidm, ewp_hist_rq->username);
		#endif
	
		/*交易状态查询接口,电子现金的脱机退货时也要上送原交易的订单号用于查询原笔脱机消费*/
		if(memcmp(ewp_hist_rq->process, "9F0000", 6)==0||memcmp(ewp_hist_rq->process, "9F0004", 6)==0)
		{
			/*订单号ANS20*/
			memcpy(ewp_hist_rq->billmsg, rcvbuf + offset, 20);
			ewp_hist_rq->billmsg[20] = 0;
			pub_rtrim_lp(ewp_hist_rq->billmsg, 20, ewp_hist_rq->billmsg,1);
			offset += 20;
			check_len -= 20;
			#ifdef __LOG__
				dcs_log(0, 0, "billmsg=[%s]", ewp_hist_rq->billmsg);
			#endif
		}
		
		/*商户编号20*/
		memcpy(ewp_hist_rq->merid, rcvbuf + offset, 20);
		ewp_hist_rq->merid[20] = 0;
		offset += 20;
		check_len -= 20;	
		
		/*交易卡号N19*/
		char  tmpbuf[20];
		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy(tmpbuf, rcvbuf + offset, 19);
		tmpbuf[19] = 0;
		pub_rtrim_lp(tmpbuf, 19, ewp_hist_rq->querycdno, 1);
		offset += 19;
		check_len -= 19;
	
		#ifdef __LOG__
			dcs_log(0, 0, "merid=[%s]", ewp_hist_rq->merid);
		#endif
		
		/*交易撤销退货查询接口*/
		if(memcmp(ewp_hist_rq->process, "9F0001", 6)==0||memcmp(ewp_hist_rq->process, "9F0002", 6)==0)
		{
			/*交易流水N6*/
			memcpy(ewp_hist_rq->trace, rcvbuf + offset, 6);
			ewp_hist_rq->trace[6] = 0;
			offset += 6;
			check_len -= 6;
	
			#ifdef __LOG__
				dcs_log(0, 0, "trace=[%s]", ewp_hist_rq->trace);
			#endif
		}
	}
	else  if(memcmp(ewp_hist_rq->process, "FF0000", 6) ==0
		||memcmp(ewp_hist_rq->process, "FF0001", 6) ==0
		||memcmp(ewp_hist_rq->process, "FF0002", 6) ==0)
	{		
		/*发送机构号*/
		memcpy(ewp_hist_rq->sendcode, rcvbuf + offset, 4);
		ewp_hist_rq->sendcode[4] = 0;
		offset += 4;
		check_len -= 4;
		#ifdef __LOG__
			dcs_log(0, 0, "sendcode=[%s]", ewp_hist_rq->sendcode);
		#endif
		
		/*交易码*/
		memcpy(ewp_hist_rq->trancde, rcvbuf + offset, 3);
		ewp_hist_rq->trancde[3] = 0;
		offset += 3;
		check_len -= 3;
		#ifdef __LOG__
			dcs_log(0, 0, "trancde=[%s]", ewp_hist_rq->trancde);
		#endif
	
		/*查询开始日期*/
		memcpy(ewp_hist_rq->startdate, rcvbuf + offset, 8);
		ewp_hist_rq->startdate[8] = 0;
		offset += 8;
		check_len -= 8;
	
		/*查询结束日期*/
		memcpy(ewp_hist_rq->enddate, rcvbuf + offset, 8);
		ewp_hist_rq->enddate[8] = 0;
		offset += 8;
		check_len -= 8;
	
		#ifdef __LOG__
			dcs_log(0, 0, "startdate=[%s], enddate=[%s]", ewp_hist_rq->startdate, ewp_hist_rq->enddate);
		#endif
	
		/*每页显示条数N1*/
		memcpy(ewp_hist_rq->perpagenum, rcvbuf + offset, 1);
		ewp_hist_rq->perpagenum[1] = 0;
		offset += 1;
		check_len -= 1;
	
		/*当前页N3*/
		memcpy(ewp_hist_rq->curpage, rcvbuf + offset, 3);
		ewp_hist_rq->curpage[3] = 0;
		offset += 3;
		check_len -= 3;
	
		#ifdef __LOG__
			dcs_log(0, 0, "perpagenum=[%s], curpage=[%s]", ewp_hist_rq->perpagenum, ewp_hist_rq->curpage);
		#endif
	
		/*用户名N11*/
		memcpy(ewp_hist_rq->username, rcvbuf + offset, 11);
		ewp_hist_rq->username[11] = 0;
		offset += 11;
		check_len -= 11;
	
		/*查询条件N1*/
		memcpy(ewp_hist_rq->queryflag, rcvbuf + offset, 1);
		ewp_hist_rq->queryflag[1] = 0;
		offset += 1;
		check_len -= 1;
	
		/*交易卡号ANS19*/
		memcpy(ewp_hist_rq->querycdno, rcvbuf + offset, 19);
		ewp_hist_rq->querycdno[19] = 0;
		offset += 19;
		check_len -= 19;
	
		#ifdef __LOG__
			dcs_log(0, 0, "queryflag=[%s]", ewp_hist_rq->queryflag);
		#endif
		
	}
	
	/*自定义域ANS20*/
	memcpy(ewp_hist_rq->selfewp, rcvbuf + offset, 20);
	ewp_hist_rq->selfewp[20] = 0;
	offset += 20;
	check_len -= 20;
	#ifdef __LOG__
		dcs_log(0, 0, "username=[%s], selfewp=[%s]", ewp_hist_rq->username, ewp_hist_rq->selfewp);
	#endif
	
	#ifdef __LOG__
		dcs_log(0, 0, "解析EWP历史交易查询数据, check_len=[%d]", check_len);
		dcs_log(0, 0, "offset=[%d]", offset);
	#endif
	
	if(check_len != 0 )
		strcpy(ewp_hist_rq->retcode, "F1");
	else
	{
		strcpy(ewp_hist_rq->retcode, "00");
		ewp_hist_rq->retcode[2] = 0;
	}

	if(	memcmp(ewp_hist_rq->retcode, "00", 2) == 0)
		dcs_log(0, 0, "EWP数据解析成功, retcode=[%s]", ewp_hist_rq->retcode);
	else
		dcs_log(0, 0, "EWP数据不正确, 直接丢弃该包, retcode=[%s]", ewp_hist_rq->retcode);
	return 0;
}
