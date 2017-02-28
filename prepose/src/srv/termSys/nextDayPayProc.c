#include "ibdcs.h"
#include "syn_http.h"
/*
	次日代付请求处理模块
*/
void nextDayPayProc(char *srcBuf, int iFid, int iMid, int iLen)
{
	char currentDate[14+1], tm_time[6+1], tm_tpdu[6], tm_buf[128];
	//系统自带的时间结构体
	struct  tm *tm;   
	time_t  t;
	int rtn = 0;
	int type = 0;
	char beginDate[14+1], endDate[14+1];
	
	memset(currentDate, 0x00, sizeof(currentDate));
	memset(tm_time, 0x00, sizeof(tm_time));
	memset(tm_tpdu, 0x00, sizeof(tm_tpdu));
	memset(tm_buf, 0x00, sizeof(tm_buf));
	memset(beginDate, 0x00, sizeof(beginDate));
	memset(endDate, 0x00, sizeof(endDate));

	dcs_log(0, 0, "次日代付请求, srcBuf=[%s]", srcBuf+5);//前面有5个字节自己添加的TPDU头
	
	time(&t);
    tm = localtime(&t);
    
    sprintf(currentDate, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
    sprintf(currentDate+8, "%02d%02d%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
	
	/*前天*/
    t = time(NULL) - 86400*2;
    tm = localtime(&t);
    sprintf(beginDate, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
    /*昨天*/
    t = time(NULL) - 86400;
    tm = localtime(&t);
    sprintf(endDate, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
	
	memcpy(tm_tpdu, srcBuf, 5);
	//解析发起的类型
	memset(tm_buf, 0x00, sizeof(tm_buf));
	rtn = GetValueByKey(srcBuf+5, "payType", tm_buf, sizeof(tm_buf));
	if(rtn < 0)
	{
		dcs_log(0, 0, "解析payType失败");
		writeBack(tm_tpdu, "E0", currentDate, "解析payType失败", iFid, iMid);
		return;
	}
	type = atoi(tm_buf);
	if(type == 0)
	{
		//解析发起的时间
		rtn = GetValueByKey(srcBuf+5, "payTime", tm_time, sizeof(tm_time));
		if(rtn < 0)
		{
			dcs_log(0, 0, "解析payTime失败");
			writeBack(tm_tpdu, "E0", currentDate, "解析payTime失败", iFid, iMid);
			return;
		}
		sprintf(beginDate+8, "%s", tm_time);
		sprintf(endDate+8, "%s", tm_time);
	}
	else if(type == 1)
	{
		//解析需要代付的起始日期
		memset(beginDate, 0x00, sizeof(beginDate));
		rtn = GetValueByKey(srcBuf+5, "beginTime", beginDate, sizeof(beginDate));
		if(rtn < 0)
		{
			dcs_log(0, 0, "解析beginTime失败");
			writeBack(tm_tpdu, "E0", currentDate, "解析beginTime失败", iFid, iMid);
			return;
		}
		//解析需要代付的终止日期
		memset(endDate, 0x00, sizeof(endDate));
		rtn = GetValueByKey(srcBuf+5, "endTime", endDate, sizeof(endDate));
		if(rtn < 0)
		{
			dcs_log(0, 0, "解析endTime失败");
			writeBack(tm_tpdu, "E0", currentDate, "解析endTime失败", iFid, iMid);
			return;
		}
		if(strcmp(endDate, beginDate)<0)
		{
			dcs_log(0, 0, "无效起始终止日期");
			writeBack(tm_tpdu, "E0", currentDate, "无效起始终止日期", iFid, iMid);
			return;
		}
	}
	else
	{
		dcs_log(0, 0, "payType无效取值");
		writeBack(tm_tpdu, "E0", currentDate, "payType无效取值", iFid, iMid);
		return;
	}
	//先返回受理成功
	writeBack(tm_tpdu, "00", currentDate, "受理成功", iFid, iMid);
	//查询transfer_info数据库表得到需要次日发起代付的，扣款时间间隔24小时的，发起代付请求。
	#ifdef __TEST__
		dcs_log(0, 0, "beginDate =[%s]", beginDate);
		dcs_log(0, 0, "endDate =[%s]", endDate);
	#endif
	rtn = DealNextDayPay(beginDate, endDate);
	if(rtn < 0)
	{
		dcs_log(0, 0, "处理失败");
		return;
	}
	return;	
}

/*
函数说明：http应答返回
*/
int writeBack(char *tpdu, char *retcode, char *currentDate, char *msg, int iFid, int iMid)
{
	char tm_buf[1024], sendbuf[1024], curDate[8+1];
	int sendlen = 0;
	memset(tm_buf, 0x00, sizeof(tm_buf));
	memset(sendbuf, 0x00, sizeof(sendbuf));
	memset(curDate, 0x00, sizeof(curDate));
	
	memcpy(curDate, currentDate, 8);
	memcpy(sendbuf, tpdu, 1);
	memcpy(sendbuf+1, tpdu+3, 2);
	memcpy(sendbuf+3, tpdu+1, 2);
	sprintf(tm_buf, "dealDate=%s&dealTime=%s&response=%s&msg=%s\n", curDate, currentDate+8, retcode, msg);
    sprintf(sendbuf+5, "%s", tm_buf);
    sendlen =strlen(tm_buf)+5;

	if(fold_write(iFid, iMid, sendbuf, sendlen) < 0)
	{
		dcs_log(0, 0, "写返回失败");
	}
	return 0;
}
