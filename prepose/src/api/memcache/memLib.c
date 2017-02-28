/*
文件说明：
使用memcached数据缓存服务存取数据
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "memSrv.h"
#include "dccdcs.h"
#include "folder.h"
#include "ibdcs.h"

static int recv_queue_id =  -1; /*接收队列*/
static int send_queue_id = -1;  /*发送队列*/

/*
    memcached保存数据
    参数：expiration_time表示数据保存时间，单位秒。默认为0，表示永远。最大2592000秒，memcache支持最大30天。
 */
int Mem_SavaKey(char *key, char *value, int value_len, int expiration_time)
{
	int offset = 0, rcvlen = 0, temp_len = 0;
	char sndbuf[1024], caBuf[2048];
	long MsgType;
    
    //如果时间格式错误，那么永远保存
    if(expiration_time < 0)
        expiration_time = 0;
    if(expiration_time > 2592000)
        expiration_time = 2592000;

	memset(sndbuf, 0, sizeof(sndbuf));

	/* 指令名称*/
	memcpy(sndbuf, "SET", 3);
	offset += 3;
    
    /*expiration time*/
    sprintf(sndbuf+offset, "%010d", expiration_time);
    offset += 10;

    
	/*key length*/
	temp_len = strlen(key);
	sprintf(sndbuf+offset, "%03d", temp_len);
	offset += 3;
	
	/*key*/
	memcpy(sndbuf+offset, key, temp_len);
	offset += temp_len;
	
	/*value length*/
	sprintf(sndbuf+offset, "%04d", value_len);
	offset += 4;
		
	/*value*/
	memcpy(sndbuf+offset, value, value_len);
	offset +=value_len;
	
	//dcs_debug(sndbuf, offset, "Mem_SavaKey:");

	MsgType = syscall(SYS_gettid);//存放当前线程的tid,如果是进程，tid等于pid
	if(recv_queue_id <0)
	  	recv_queue_id = queue_connect("MEMRVR");
	if(recv_queue_id <0)
	{
		dcs_log(0, 0, "connect mem recv queue fail!");
		return -1;
	}
	
	send_queue_id = queue_connect("MEMRVS");
	if(send_queue_id <0)
	{
		dcs_log(0, 0, "connect mem send queue fail!");
		return -1;
	}
	//dcs_log(0, 0, "send MsgType=[%ld]", MsgType);
	if(queue_send_by_msgtype(recv_queue_id, sndbuf, offset, MsgType, 0)<0)
	{
		recv_queue_id=-1;
	  	dcs_log(0, 0, "send msg to queue fail!");
	  	return -1;
	}
	
	memset(caBuf, 0, sizeof(caBuf));
	if(0> (rcvlen=queue_recv_by_msgtype(send_queue_id, caBuf, sizeof(caBuf), &MsgType, 0)))
	{
		send_queue_id =-1;
	  	dcs_log(0, 0, "recv msg from msg queue fail!");
	  	return -1;
	}
		
	//dcs_debug(caBuf, rcvlen, "queue_recv:");
	//dcs_log(0, 0, "recv MsgType=[%ld]", MsgType);
	if(memcmp(caBuf, "00", 2)== 0)
	{
		dcs_log(0, 0, "save Key success");
		return 0;
	}
	else
	{
		dcs_log(0, 0, "save Key error");
		return -1;
	}
	return 0;
}

/*与memcached交互取数据Mem_GetKey*/   
int Mem_GetKey(char *key, char *value, int *value_len, int value_buffer)
{
	int offset = 0, rcvlen = 0, temp_len = 0;
	char sndbuf[1024], caBuf[2048];
	long MsgType;

	memset(sndbuf, 0, sizeof(sndbuf));

	/* 指令名称*/
	memcpy(sndbuf, "GET", 3);
	offset += 3;
	
	/*key*/
	temp_len = strlen(key);
	memcpy(sndbuf+offset, key, temp_len);
	offset += temp_len;
	
	//dcs_debug(sndbuf, offset, "Mem_GetKey:");

	MsgType = syscall(SYS_gettid);//存放当前线程的tid,如果是进程，tid等于pid
	if(recv_queue_id <0)
	  	recv_queue_id = queue_connect("MEMRVR");
	if(recv_queue_id <0)
	{
		dcs_log(0, 0, "connect mem recv queue fail!");
		return -1;
	}
	
	send_queue_id = queue_connect("MEMRVS");
	if(send_queue_id <0)
	{
		dcs_log(0, 0, "connect mem send queue fail!");
		return -1;
	}
	//dcs_log(0, 0, "send MsgType=[%ld]", MsgType);
	if(queue_send_by_msgtype(recv_queue_id, sndbuf, offset, MsgType, 0)<0)
	{
		recv_queue_id=-1;
	  	dcs_log(0, 0, "send msg to queue fail!");
	  	return -1;
	}
	
	memset(caBuf, 0x00, sizeof(caBuf));
	if(0> (rcvlen=queue_recv_by_msgtype(send_queue_id, caBuf, sizeof(caBuf), &MsgType, 0)))
	{
		send_queue_id =-1;
	  	dcs_log(0, 0, "recv msg from msg queue fail!");
	  	return -1;
	}
		
	//dcs_debug(caBuf, rcvlen, "queue_recv:");
	//dcs_log(0, 0, "recv MsgType=[%ld]", MsgType);
	if(memcmp(caBuf, "00", 2)== 0)
	{
		if(value_buffer >= (rcvlen -2))
		{
			memcpy(value, caBuf+2, rcvlen-2);
			*value_len = rcvlen -2;
		}
		else
		{
			dcs_log(0, 0, "value_buffer len =[%d], rcvlen-2=[%d]", value_buffer, rcvlen-2);
			dcs_log(0, 0, "用来存放value的buffer过小");
			*value_len = value_buffer;
			return -1;
		}
	}
	else
	{
		dcs_log(0, 0, "Get Key error");
		return -1;
	}
	return 0;
}

/*与memcached交互删除数据*/
int Mem_DeleteKey(char *key)
{
	int offset = 0, rcvlen = 0, temp_len = 0;
	char sndbuf[1024], caBuf[2048];
	long MsgType;

	memset(sndbuf, 0, sizeof(sndbuf));

	/* 指令名称*/
	memcpy(sndbuf, "DEL", 3);
	offset += 3;
	
	/*key*/
	temp_len = strlen(key);
	memcpy(sndbuf+offset, key, temp_len);
	offset += temp_len;
	
	dcs_debug(sndbuf, offset, "Mem_DeleteKey:");

	MsgType = syscall(SYS_gettid);//存放当前线程的tid,如果是进程，tid等于pid
	if(recv_queue_id <0)
	  	recv_queue_id = queue_connect("MEMRVR");
	if(recv_queue_id <0)
	{
		dcs_log(0, 0, "connect mem recv queue fail!");
		return -1;
	}
	
	send_queue_id = queue_connect("MEMRVS");
	if(send_queue_id <0)
	{
		dcs_log(0, 0, "connect mem send queue fail!");
		return -1;
	}
	dcs_log(0, 0, "send MsgType=[%ld]", MsgType);
	if(queue_send_by_msgtype(recv_queue_id, sndbuf, offset, MsgType, 0)<0)
	{
		recv_queue_id=-1;
	  	dcs_log(0, 0, "send msg to queue fail!");
	  	return -1;
	}
	
	memset(caBuf, 0, sizeof(caBuf));
	if(0> (rcvlen=queue_recv_by_msgtype(send_queue_id, caBuf, sizeof(caBuf), &MsgType, 0)))
	{
		send_queue_id =-1;
	  	dcs_log(0, 0, "recv msg from msg queue fail!");
	  	return -1;
	}
		
	//dcs_debug(caBuf, rcvlen, "queue_recv:");
	dcs_log(0, 0, "recv MsgType=[%ld]", MsgType);
	if(memcmp(caBuf, "00", 2)== 0)
	{
		dcs_log(0, 0, "Delete Key sucess");
	}
	else
	{
		dcs_log(0, 0, "Delete Key error");
		return -1;
	}
	return 0;
}
