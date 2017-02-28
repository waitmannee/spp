#include <stdio.h>
#include "timer.h"
#include "extern_function.h"
#include "syn_http.h"

/*
 
Description:
1.compiled with other process, eg: XPEPTransRcv
2.offer api service to other process to do out of time control service

 add by li 2017.01.04
*/

/*
向超时控制里添加一条
组一个key=value串
*/
int add_timeout_control(struct TIMEOUT_INFO timeout_info, char *rcvbuf, int rcvlen)
{
	int queue_id = -1;
	long msgtype = 0;
	int ret = 0, len = 0;
	char sendbuf[2048];
	
	memset(sendbuf, 0x00, sizeof(sendbuf));
	
	//组报文
	sprintf(sendbuf, "operation=%s&key=%s&timeout=%d&sendnum=%d&foldid=%d&length=%d&data=", "ADD", timeout_info.key, \
	timeout_info.timeout, timeout_info.sendnum, timeout_info.foldid, timeout_info.length);
	len = strlen(sendbuf);
	memcpy(sendbuf+len,timeout_info.data,timeout_info.length);
	len +=timeout_info.length;
	//发送报文到队列
	queue_id = queue_connect("DTMSC");
	if(queue_id <0)
	{
		dcs_log(0, 0, "connect msg queue fail!");
		return -1;
	}
	msgtype = getpid();
	ret = queue_send_by_msgtype(queue_id, sendbuf, len, msgtype, 0);
	if(ret< 0)
	{
		dcs_log(0, 0, "queue_send_by_msgtype err");
		return -1;
	}
	//接收队列
	queue_id = queue_connect("R_TMSC");
	if ( queue_id <0)
	{
		dcs_log(0, 0, "connect msg queue fail!");
		return -1;
	}
	
	ret = queue_recv_by_msgtype(queue_id, rcvbuf, rcvlen, &msgtype, 0);
	if(ret< 0)
	{
		dcs_log(0, 0, "queue_recv_by_msgtype err");
		return -1;
	}
	return ret;
}

/*从超时控制里删除一条
 返回值:rcvbuf     "response=00&status=2"   
	说明：response 00 表示请求处理成功，其他表示失败
		  status 0 表示 没找到key对应的超时控制；1表示 移除成功  ； 2 表示 超时控制已超时返回
*/
int delete_timeout_control(char *key, char *rcvbuf, int rcvlen)
{
	int queue_id = -1;
	long msgtype = 0;
	int ret = 0, len = 0;
	char sendbuf[1024];
	
	memset(sendbuf, 0x00, sizeof(sendbuf));
	
	//组报文
	sprintf(sendbuf, "operation=%s&key=%s", "DELETE", key);
	len = strlen(sendbuf);
	
	//发送报文到队列
	queue_id = queue_connect("DTMSC");
	if(queue_id <0)
	{
		dcs_log(0, 0, "connect msg queue fail!");
		return -1;
	}
	msgtype = getpid();
	ret = queue_send_by_msgtype(queue_id, sendbuf, len, msgtype, 0);
	if(ret< 0)
	{
		dcs_log(0, 0, "queue_send_by_msgtype err");
		return -1;
	}
	//接收队列
	queue_id = queue_connect("R_TMSC");
	if ( queue_id <0)
	{
		dcs_log(0, 0, "connect msg queue fail!");
		return -1;
	}
	
	ret = queue_recv_by_msgtype(queue_id, rcvbuf, rcvlen, &msgtype, 0);
	if(ret< 0)
	{
		dcs_log(0, 0, "queue_recv_by_msgtype err");
		return -1;
	}
	return ret;
}


