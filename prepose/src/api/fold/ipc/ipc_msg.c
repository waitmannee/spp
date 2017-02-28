//
//  ipc_msg.c
//  iPrepose
//
//  Created by Freax on 12-12-19.
//  Copyright (c) 2012年 Chinaebi. All rights reserved.
//
#include "unixhdrs.h"
#include "extern_function.h"
#include "folder.h"
/*
 Symbols from ipc_msg.o:
 
 [Index]     Value      Size      Type  Bind  Other Shname         Name
 
 [5]       |0         |4         |OBJT |LOCL |0    |.data         |g_readq_interruptable
 [34]      |1568      |37        |FUNC |GLOB |0    |.text         |queue_readq_interruptable
 [35]      |1496      |72        |FUNC |GLOB |0    |.text         |queue_get_info

 [37]      |0         |0         |NOTY |GLOB |0    |UNDEF         |ipc_makekey
 
 [39]      |1312      |108       |FUNC |GLOB |0    |.text         |queue_recv
 [42]      |1236      |75        |FUNC |GLOB |0    |.text         |queue_send
 
 [43]      |0         |0         |NOTY |GLOB |0    |UNDEF         |msgctl
 
 [44]      |1208      |26        |FUNC |GLOB |0    |.text         |queue_delete
 [45]      |1184      |24        |FUNC |GLOB |0    |.text         |queue_connect
 [46]      |1420      |75        |FUNC |GLOB |0    |.text         |queue_getqueue
 [47]      |1148      |33        |FUNC |GLOB |0    |.text         |queue_create
 */

//#define	PATH_MAX		 1024
#define MSG_RD  0400            /* read permission for the queue */
#define MSG_WR  0200            /* write permission for the queue */
#define MSG_RW	MSG_RD | MSG_WR

//int get_ipc_key()
//{
//	const char a = 'a';
//	int ascii_a = (int)a;
//	char *curdir = NULL;
//	char curdira[PATH_MAX];
//	size_t size = sizeof(curdira);
//	key_t ipc_key;
//	struct timeb time_info;
//    
//	if (NULL == (curdir = getcwd(curdira, size))) {
//		//tst_brkm(TBROK, cleanup, "Can't get current directory "
//          //       "in getipckey()");
//	}
//    
//    printf("curdir====>>%s\n", curdir);
//    
//	/*
//	 * Get a Sys V IPC key
//	 *
//	 * ftok() requires a character as a second argument.  This is
//	 * refered to as a "project identifier" in the man page.  In
//	 * order to maximize the chance of getting a unique key, the
//	 * project identifier is a "random character" produced by
//	 * generating a random number between 0 and 25 and then adding
//	 * that to the ascii value of 'a'.  The "seed" for the random
//	 * number is the millisecond value that is set in the timeb
//	 * structure after calling ftime().
//	 */
//	(void)ftime(&time_info);
//	srandom((unsigned int)time_info.millitm);
//    
//	if ((ipc_key = ftok(curdir, ascii_a + random()%26)) == -1) {
//		//tst_brkm(TBROK, cleanup, "Can't get msgkey from ftok()");
//	}
//    
//	return(ipc_key);
//}

int queue_create(const char *name)
{
    key_t msgkey = ipc_makekey(name);
    int msg_id = -1;
   // dcs_log(0, 0, "msgkey:%d", msgkey);
    /* create a message queue with read/write permissions */
	if ((msg_id = msgget(msgkey, IPC_CREAT | IPC_EXCL | MSG_RW)) == -1) {
		//printf("create mesage queue error !!");
		dcs_log(0, 0, "create msgget queue error:%s,errno:%d", strerror(errno),errno);
	}
    
    return msg_id;
}

int queue_connect(const char *name)
{
	int msg_queue_id = -1;                      /* to hold the message queue id */
	struct msqid_ds qs_buf;
	int msgkey = ipc_makekey(name);

	/* make sure the initial # of bytes is 0 in our buffer */
	qs_buf.msg_qbytes = 0x3000;

	/* now we have a key, so let's create a message queue */
	if ((msg_queue_id = msgget(msgkey, IPC_EXCL)) == -1) 
	{
	   	// printf("Can't connect message queuen\n");
		dcs_log(0, 0, "Can't connect message queue error:%s,errno:%d", strerror(errno),errno);
		return -1;
	}

	/* now stat the queue to get the default msg_qbytes value */
	if ((msgctl(msg_queue_id, IPC_STAT, &qs_buf)) == -1) 
	{
		//tst_brkm(TBROK, cleanup, "Can't stat the message queue");
		dcs_log(0, 0, " queue msgctl error:%s,errno:%d", strerror(errno),errno);
		return -1;
	}

	/* decrement msg_qbytes and copy its value */
	qs_buf.msg_qbytes -= 1;
	//new_bytes = qs_buf.msg_qbytes;
	    
	return msg_queue_id;
}

int queue_delete(int qid)
{
	if (qid == -1) 
	{	/* no queue to remove */
		return -1;
	}

	if (msgctl(qid, IPC_RMID, NULL) == -1) 
	{
		printf("WARNING: message queue deletion failed.");
	}
	return -1;
}

int queue_send(int qid, void *buf, int size, int nowait)
{
	char len[4];
	MSGBUF msg_buf;
	memset(msg_buf.mtext, 0x00, sizeof(msg_buf.mtext));
	if( size > sizeof(msg_buf.mtext) -4 )
	{
		dcs_debug(buf, size, "消息内容太大len=%d!\n",size);
		return -1;
	}
	memset(len, 0x00, 4);
	sprintf(len, "%d", size);
	memcpy(msg_buf.mtext, len, 4);
	memcpy(msg_buf.mtext + 4, buf, size);    

	msg_buf.mtype = 1;

	if (nowait == 1 ) 
	{ //不等待
		if (msgsnd(qid, &msg_buf, size + 4, IPC_NOWAIT) < 0)
		  	return -1;
	}
	else
	{ //等待
		while(1)
		{
			if (msgsnd(qid, &msg_buf, size + 4, 0) < 0) 
			{
				if(errno == EINTR)
					continue;
				else
					return -1;
			}
			else
				break;
		}
	}    
	return 0;
}

int queue_recv(int qid, void *msg, int size, int nowait)
{
	/*
	* remove the message by reading from the queue
	*/
	MSGBUF rd_buf;
	int rtn  = -1;
	char len[4];
	memset(rd_buf.mtext, 0x00, sizeof(rd_buf.mtext));

	if (nowait == 1 ) 
	{ //不等待
		if (msgrcv(qid, &rd_buf, size, 1, IPC_NOWAIT|MSG_NOERROR) == -1)
		{
			//dcs_log(0, 0, "qid = %d, Could not read from queue  nowait\n", qid);
			return -1;
		}
	} 
	else  
	{ //等待
		while(1)
		{
			if (msgrcv(qid, &rd_buf, size, 1, MSG_NOERROR) == -1) 
			{
				if(errno == EINTR)
					continue;
				else
					return -1;
			}
			else
				break;
		}
	}

	memset(len, 0x00, 4);
	memcpy(len, rd_buf.mtext, 4);
	rtn = atoi(len) ;
	memcpy(msg, rd_buf.mtext + 4, rtn);

	//    dcs_debug(rd_buf.mtext,256,"recv收到消息队列的数据...加密机的数据包 sockid=%d",0);
	//    dcs_debug(msg,rtn,"recv ,recv size=%d",rtn);
	//    
	return rtn;
}
/*
*功能:发送信息到队列中
*参数: qid  队列id,buf 要发送的信息, size 要发送信息的长度
* msgtype 要接收信息的消息类型,若为0表示是第一条
* nowait 是否阻塞标识1表示不阻塞,0表示阻塞
*/
int queue_send_by_msgtype(int qid, void *buf, int size, long msgtype,int nowait)
{  
	MSGBUF msg_buf;
	char len[4];
	
	memset(msg_buf.mtext, 0x00, sizeof(msg_buf.mtext));
	memset(len, 0x00, 4);
	if( size > sizeof(msg_buf.mtext) -4 )
	{
		dcs_debug(buf, size, "消息内容太大len=%d!\n",size);
		return -1;
	}
	sprintf(len, "%d", size);	
	memcpy(msg_buf.mtext, len, 4);
	memcpy(msg_buf.mtext + 4, buf, size);    

	msg_buf.mtype = msgtype;

	if (nowait == 1 ) 
	{ //不等待
		if (msgsnd(qid, &msg_buf, size + 4, IPC_NOWAIT) < 0)
		  	return -1;
	}
	else
	{ //等待
		while(1)
		{
			if (msgsnd(qid, &msg_buf, size + 4, 0) < 0) 
			{
				if(errno == EINTR)
					continue;
				else
					return -1;
			}
			else
				break;
		}
	}    
	return 0;
}
/*
*功能:从队列中读取信息
*参数: qid  队列id,buf  接收的信息, size 为BUF的长度,
* msgtype 要接收信息的消息类型,若为0表示是第一条
* nowait 是否阻塞标识1表示不阻塞,0表示阻塞
* 返回值:
*	-1 表示操作失败,大于0表示读出信息的长度
*      成功时msgtype 会返回信息的类型
*/
int queue_recv_by_msgtype(int qid, void *msg, int size,long *msgtype, int nowait)
{
	MSGBUF rd_buf;
	int rtn  = -1;
	char len[4];
	memset(rd_buf.mtext, 0x00, sizeof(rd_buf.mtext));
	if (nowait == 1 ) 
	{ //不等待
		if (msgrcv(qid, &rd_buf, size, *msgtype, IPC_NOWAIT|MSG_NOERROR) == -1)
			return -1;
	} 
	else  
	{ //等待
		while(1)
		{
			if (msgrcv(qid, &rd_buf, size, *msgtype, MSG_NOERROR) == -1)
			{
				if(errno == EINTR)
					continue;
				else
					return -1;
			}
			else
				break;
		}
	}
	*msgtype = rd_buf.mtype;

	memset(len, 0x00, 4);
	memcpy(len, rd_buf.mtext, 4);
	rtn = atoi(len) ;
	memcpy(msg, rd_buf.mtext + 4, rtn);

	return rtn;
}

int queue_readq_interruptable(int flag)
{
	return -1;
}

/*
 * 清空队列中的message
 */
int queue_empty(int msg_id)
{
	struct msqid_ds info;
	if (msgctl(msg_id, IPC_STAT, &info)) 
		perror("msgctl IPC_STAT error ");

	while (info.msg_qnum > 0) 
	{
		msgrcv(msg_id, NULL, MSGSIZE, 1, IPC_NOWAIT);
		msgctl(msg_id, IPC_STAT, &info);
	}

	return 0;
}

/*
 * 取得消息队列状态
 */
int queue_status(int msg_id)
{
	struct msqid_ds info;
	if (msgctl(msg_id, IPC_STAT, &info)) 
		perror("msgctl IPC_STAT error ");
	return 0;
}

/*
 * 取得消息队列消息的数量
 */
int queue_status_num_msg(int msg_id)
{
	struct msqid_ds info;
	if (msgctl(msg_id, IPC_STAT, &info)) 
		perror("msgctl IPC_STAT error ");

	return  (int)info.msg_qnum;
}
