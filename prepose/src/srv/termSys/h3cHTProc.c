#include "trans.h" 
#include <string.h>
#include "ibdcs.h"
/*
	H3C 历史交易数据查询 
	@see 《前置系统接入报文规范.docx》 V9.4
*/
void h3cHTProc(char *srcBuf, int iFid, int iMid, int iLen)
{
	EWP_HIST_RQ ewp_hist_rq;
	char buffer[2048], buffer2[2048];
	int rtn, s;
	char tpduHead[5];
	
	memset(&ewp_hist_rq, 0, sizeof(EWP_HIST_RQ));
	memset(buffer, 0, sizeof(buffer));
	memset(tpduHead, 0, sizeof(tpduHead));
	
#ifdef __LOG__
	dcs_debug(srcBuf, iLen, "h3cHTProc交易, iFid=%d, iMid=%d, iLen=%d", iFid, iMid, iLen);
#else
	dcs_log(0, 0, "h3cHTProc交易, iFid=%d, iMid=%d, iLen=%d", iFid, iMid, iLen);
#endif
	memcpy(tpduHead, srcBuf, 5);
	
	srcBuf += 5;
	iLen -= 5;
	
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
	memset(buffer2, 0, sizeof(buffer2));
	memcpy(buffer2, tpduHead, 1);
	memcpy(buffer2+1, tpduHead+3, 2);
	memcpy(buffer2+1+2, tpduHead+1, 2);
	#ifdef __LOG__
		dcs_debug(buffer2, 5, "返回的TPDU头");
	#endif
	
	memcpy(buffer2+1+2+2, buffer, rtn);
	rtn += 5;
	
	#ifdef __LOG__
		dcs_debug(buffer2, rtn, "返回H3C的数据(包含TPDU头)");
	#endif
	
	s = fold_write(iFid, -1, buffer2, rtn);
	if(s < 0 )
	{
		dcs_log(0, 0, "写入fold error");
		return;
	}
	return;
}
