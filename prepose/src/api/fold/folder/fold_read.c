//
//  fold_read.c
//  iPrepose
//
//  Created by Freax on 12-12-19.
//  Copyright (c) 2012 Chinaebi. All rights reserved.
//

#include <stdio.h>
#include <dirent.h>
#include "extern_function.h"
#include "ibdcs.h"
#include <stdlib.h>

static int readWhileCnt = 0;  //开发测试

static int file_compare( const char **a, const char **b )
{
    return strcmp( *a, *b );
}



/* 进程并发控制 */
void lock_unlock_read_fold(int lock, int unlock)
{
	int shm_id;
	struct ServerAst * g_pServerAst = NULL;
	char *ptr = shm_connect(ISDSERVER_SHM_NAME ,&shm_id);

	if(ptr != NULL)
	{
		g_pServerAst =  (struct ServerAst *)ptr;
	}
	if (g_pServerAst != NULL) 
	{
		const char* currentprocname = getprogname_(); //Linux not support this function
		//dcs_log(0, 0, "currentprocname = %s", currentprocname);
		int i=0;
		for (; i < g_pServerAst->serverNum; i ++) 
		{
			struct serverconf * serverCnf = &g_pServerAst->server[i];
			//Linux not support this function getprogname()
			if (serverCnf->max > 1 && strcmp(currentprocname, serverCnf->command) == 0) 
			{
				int sem_id = sem_connect(serverCnf->command, 2);
				if (sem_id < 0) 
					sem_id = sem_create(serverCnf->command, 2);

				if (sem_id > 0) 
				{
					if (lock) 
					{
						sem_lock(sem_id, 1, 1);
						dcs_log(0, 0, "fold_read_lock_sem_id = %d", sem_id);
					}
					if (unlock) 
					{
						dcs_log(0, 0, "fold_read_unlock_sem_id = %d", sem_id);
						sem_unlock(sem_id, 1, 1);
					}
				}
				break;
			}
		}
	}
	shm_detach(ptr);
}

/*
/*功能描述：该函数打开指定的fold
*/
int fold_open(int folder_Id)
{    
	int opened_fid = -1;
	char f_name[150];
	char folder_name[64];
	int fid=-1;
	memset(f_name, 0x00, sizeof(f_name));
	memset(folder_name, 0x00, sizeof(folder_name));	
	get_fold_name_from_share_memory_by_id(folder_name,folder_Id);
	sprintf(f_name, "/tmp/%s",folder_name );
	//opened_fid = open(f_name, O_RDWR);
	//opened_fid = open(f_name, O_RDONLY);
	opened_fid = open(f_name, O_RDONLY|O_NONBLOCK);
	if(opened_fid < 0) 
	{
	    dcs_log(0, 0, "open folder error !!! %s ", f_name);
	    exit(1);
	}  
	return opened_fid;
}
/*
 int fold_read(int folder_Id,int opened_fid,int* org_folderId,void *user_buffer,int nsize,int fBlocked)
 
 功能描述：该函数从指定的文件夹中读取数据
 参数说明：folder_Id      接收文件夹Id，调用进程从该文件夹读取数据
 opened_fid  fold_open()函数返回的id
 org_folderId   函数成功时该参数返回发送者的文件夹Id
 user_buffer    调用者的数据缓冲区，用来放置读到的数据
 nsize          缓冲区user_buffer字节大小
 fBlocked       若为非零，则函数执行阻塞操作，即如果文件夹folder_Id中暂时没有可读的数据时，调用进程将被挂起；
 若为零，则函数执行非阻塞操作，即如果文件夹folder_Id中暂时没有可读的数据时，函数立即返回0
 返 回 值：成功时函数返回实际读得的字节数；
 否则返回-1，errno指明具体错误原因：
 FOLD_ENOSYS   进程没有连上Folder系统
 FOLD_EINVAL   无效的参数FOLD_ENOENT   给定Id的文件夹并不存在
 ENOMSG        给定Id的文件夹中暂时无可读的数据
 EINTR         读操作被一个Unix信号(signal)中断
 其它值        操作系统给出的其它错误码
 */

int fold_read(int folder_Id,int opened_fid,int* org_folderId,void *user_buffer,int nsize,int fBlocked)
{    
	char rd_buf[PACKAGE_LENGTH+100];//长度包含在报文前加的folder报文头
	char mess_buf[PACKAGE_LENGTH+33];
	int ret =0;
	int data_len = 0, buf_len = 0;
	int msg_len=0;
	struct msgpacket msg_packet;
	memset(rd_buf, 0x00, sizeof(rd_buf));
	lock_unlock_read_fold(1, 0);
	while(1)
	{
		ret = (int)read(opened_fid, rd_buf, 4); /*4 byte; msg len*/
		if (ret <=0) 
		{
			if(errno==EINTR||errno==EAGAIN||ret == 0)
			{  
				if(fBlocked)
				{
					u_sleep_us(10000); //sleep 10ms 
					continue; 
				}
			     else
			     {
			     	    lock_unlock_read_fold(0, 1);
			     	    return 0;//非阻塞 ,无数据
			     } 
			}  
			else /* 其他错误 没有办法,只好退了*/   
			{  
				dcs_log(0,0,"errno=%d",errno);
				close(opened_fid);
				lock_unlock_read_fold(0, 1);
				return -1;
			}  
	    }
	    if(ret >0)
	         break;
	}

	msg_len = atoi(rd_buf);
	//dcs_log(0, 0, "read first 4 baytes msg =%s", rd_buf);
	memset(rd_buf, 0x00, sizeof(rd_buf));
	ret = (int)read(opened_fid, rd_buf, msg_len);
	if(ret <=0)
	{
		dcs_log(0, 0, "read second error");
		lock_unlock_read_fold(0, 1);
		return -1;
	}
	//dcs_log(0, 0, "read data len = %d.", ret);
	fold_set_msgnum(folder_Id,0);
	lock_unlock_read_fold(0, 1);
	//dcs_log(0, 0, "str_to_msgpacket.", ret);
	memset(&msg_packet, 0x00, sizeof(struct msgpacket));
	memset(mess_buf, 0x00, sizeof(mess_buf));
	str_to_msgpacket(rd_buf, mess_buf, &msg_packet, &buf_len, &data_len,nsize);
		
	*org_folderId = msg_packet.msg_org;

	//dcs_log(0, 0, "read data len = %d, content len = %d.", ret, buf_len);
	if(buf_len > nsize )
	{	
		dcs_debug(mess_buf, buf_len, "read mesg too long, msg_len=%d,return 0",buf_len);
		return 0;
	}
	memcpy(user_buffer, mess_buf, buf_len);
	return buf_len;
}


/*
 * 清除msg
 */
int fold_purge_msg(int foldid)
{
    return -1;
}



