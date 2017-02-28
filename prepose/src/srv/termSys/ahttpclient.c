#include "ibdcs.h"
#include "trans.h"
#include <curl/curl.h> 


char wr_buf[1024]; 
int  wr_index = 0; 
/*
 * 使用curl实现http的client端功能
 * 二维码支付通知
 */

 //回调函数
size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) 
{ 
	int segsize = size * nmemb; 
	if(wr_index + segsize > 1024) 
	{ 
		*(int *)userp = 1; 
		return 0; 
	} 

	memcpy((void *)&wr_buf[wr_index], buffer, (size_t)segsize); 
	wr_index += segsize; 
	wr_buf[wr_index] = 0; 
	return segsize; 
} 

//实现金额转化
int GetRealAmt(char *srcAmt, char *resAmt)
{
	int len = 0;
	len = strlen(srcAmt);
	dcs_log(0, 0, "1111111strlen(srcAmt)=[%d]", len);
	if(len>2)
	{
		len -=2;
		memcpy(resAmt, srcAmt, len);
		memcpy(resAmt+len, ".", 1);
		memcpy(resAmt+len+1, srcAmt+len, 2);
		len +=1;
	}
	else
	{
		sprintf(resAmt, "%s%02d", "0.", atoi(srcAmt));
		len = strlen(resAmt);
	}
	return len;
}
int SendPayResult(TWO_CODE_INFO two_code_info, char *trace, char *curDate)
{
	dcs_log(0, 0, "实现二维码支付状态通知");
	
	CURL *curl; 
	CURLcode ret; 
	char resAmt[12], paytime[18], tempbuf[256], sha2_info[40];
	char data[1024];
	int rtn = 0;
	  
	int  wr_error; 

	wr_error = 0; 
	wr_index = 0; 

	curl = curl_easy_init(); 
	if(!curl) 
	{ 
		dcs_log(0, 0, "couldn't init curl"); 
		return -1; 
	} 
	dcs_log(0, 0, "1111111curDate=[%s]", curDate);
	memset(resAmt, 0x00, sizeof(resAmt));
	memset(paytime, 0x00, sizeof(paytime));
	memset(tempbuf, 0x00, sizeof(tempbuf));
	memset(sha2_info, 0x00, sizeof(sha2_info));
	
	memcpy(paytime, curDate, 8);
	memcpy(paytime+8, " ", 1);
	memcpy(paytime+9, curDate+8, 2);
	memcpy(paytime+11, ":", 1);
	memcpy(paytime+12, curDate+10, 2);
	memcpy(paytime+14, ":", 1);
	memcpy(paytime+15, curDate+12, 2);
	dcs_log(0, 0, "22222222two_code_info.amt=[%s]", two_code_info.amt);
	dcs_log(0, 0, "paytime=[%s]", paytime);
	rtn = GetRealAmt(two_code_info.amt, resAmt);
	if(rtn < 0)
	{
		dcs_log(0, 0, "amt transfer error"); 
		return -1;
	}
	dcs_log(0, 0, "333333333");
	sprintf(tempbuf, "%s%s%s%s%s%s", two_code_info.ordid, resAmt, two_code_info.ordtime, "00", paytime, trace);
	rtn = strlen(tempbuf);
	sha2_get(tempbuf, rtn, sha2_info);
	bcd_to_asc(two_code_info.hashcode, sha2_info, 64, 0);
	dcs_log(0, 0, "4444444444444444");
	sprintf(data, "jsonStr={\"OrdId\":\"%s\", \"Amt\":\"%s\", \"OrdTime\":\"%s\", \"Status\":\"00\", \"PayTime\":\"%s\", \
	\"PayStance\":\"%s\", \"HashCode\":\"%s\"}", two_code_info.ordid, resAmt, two_code_info.ordtime, paytime, trace, two_code_info.hashcode);
	//"jsonStr={\"OrdId\":\"%s\", \"Amt\":\"%s\", \"OrdTime\":\"%s\", \"Status\":\"00\", \"PayTime\":\"%s\", \"PayStance\":\"%s\", \"HashCode\":\"%s\"}";
	dcs_log(0, 0, "55555555555555555");
	dcs_log(0, 0, "data=[%s]", data);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
	
	curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.20.140:8999/EC_demo/result"); 
	//curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.20.176:80/index.html"); 
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&wr_error); 
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data); 
	
	ret = curl_easy_perform( curl ); 

	dcs_log(0, 0,  "ret = %d (write_error = %d)", ret, wr_error ); 
	dcs_log(0, 0,  "%s", wr_buf ); 

	if(ret == 0) 
		dcs_log(0, 0,  "%s", wr_buf); 
	curl_easy_cleanup( curl ); 
	return 0; 
}

