#include "ibdcs.h"
#include "tmcibtms.h"
#include "iso8583.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "extern_function.h"
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<pthread.h>
#include "dccsock.h"

static int OpenLog()
{  
    char path[500];
    char file[520];
    memset(path, 0, sizeof(path));
    memset(file, 0, sizeof(file));
    getcwd(path, 500);
    sprintf(file, "%s/test_syn_http_client.log", path);
    return dcs_log_open(file, "test_syn_http_client");
}

int main(int argc, char *argv[])
{
	int	len, s = 0;
	char	send_buffer[250], recv_buf[250];
	int qid = 0;
	long msgtype = 0;

	s = OpenLog();
	if(s < 0)
        return -1;
	
	memset(send_buffer, 0, sizeof(send_buffer));
	memset(recv_buf, 0, sizeof(recv_buf));
	
	sprintf(send_buffer, "POST / HTTP/1.1\r\nAccept: */*\r\nUser-Agent: Mozilla/4.0\r\nContent-Length:40\r\nConnection:close\r\nAccept-Language:zh-cn\r\nAccept-Encoding: gzip, deflate\r\nHost:%s\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n", "192.168.20.159");

	
	memcpy(send_buffer+strlen(send_buffer), "6022000000000860010030210046112233441314", 40);
	s = 40+strlen(send_buffer);
	//qid = queue_connect("RECV2");
	//qid = queue_connect("recv");
	qid = queue_connect("recv2");
	if(qid < 0)
	{
		dcs_log(0, 0, "queue_create err");
		exit(0);
	}
	dcs_debug(send_buffer, s, "send buffer len:%d, 发送队列[%d]", s, qid);
	msgtype = getpid();
	len = queue_send_by_msgtype(qid, send_buffer, s, msgtype, 0);
	
	if(len < 0)
	{
		dcs_log(0, 0, "queue_send_by_msgtype err");
		exit(0);
	}

	//qid = queue_connect("SEND2");
	//qid = queue_connect("send1");
	qid = queue_connect("send2");
	if(qid < 0)
	{
		dcs_log(0, 0, "queue_connect err");
		exit(0);
	}
	memset(recv_buf, 0, sizeof(recv_buf));
	len = queue_recv_by_msgtype(qid, recv_buf ,  sizeof(recv_buf), &msgtype, 0);
	if(len < 0)
	{
		dcs_log(0, 0, "queue_recv_by_msgtype err");
		return 0;
	}
	dcs_debug(recv_buf, len, "recv buf length =%d, 接收队列[%d], msgtype[%ld]", len, qid, msgtype);
    return 0;
}
