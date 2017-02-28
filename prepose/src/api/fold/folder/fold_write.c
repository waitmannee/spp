//
//  fold_write.c
//  iPrepose
//
//  Created by Freax on 12-12-19.
//  Copyright (c) 2012年 Chinaebi. All rights reserved.
//

#include <stdio.h>
#include "unixhdrs.h"
#include "extern_function.h"

extern SHMOBJ       gl_shmobj;
static unsigned long get_file_size(const char *path);
/*
 int fold_write(int dest_folderId,int org_folderId,void *user_data,int nbytes)
 
 功能描述：该函数将数据写入到指定接收者的文件夹中。
 参数说明：dest_folderId  接收者的文件夹Id
 org_folderId   发送者的文件夹Id，-1表示不关心发送者
 user_data      调用者的数据缓冲区，这些数据将写入文件夹dest_folderId中
 nbytes         缓冲区user_data中含有的字节数
 返 回 值：成功时函数返回实际写入到文件夹dest_folderId中的字节数；
 否则返回-1，errno指明具体错误原因：
 FOLD_ENOSYS   进程没有连上Folder系统
 FOLD_EINVAL   无效的参数
 FOLD_ENOENT   给定Id的文件夹并不存在
 FOLD_ENOMBUF  系统没有空闲的共享内存
 其它值        操作系统给出的错误码
 */

int fold_write(int dest_folderId,int org_folderId,void *user_data,int nbytes)
{
	int opened_fid = -1;
	char f_name[50];
	char folder_name[33];
	int max=0,num=0;
	int data_len = 0;
	int buf_len = 0;
	int retVal = 0;
	int flag =1;
	char msg_buf[PACKAGE_LENGTH+100];//长度包含在报文前加的folder报文头
	char len[4+1];
	struct msgpacket msg_packet;
	memset(&msg_packet, 0, sizeof(struct msgpacket));
	memset(f_name, 0x00, sizeof(f_name));
	memset(folder_name, 0x00, sizeof(folder_name));

	//报文长度限制
	if(nbytes > PACKAGE_LENGTH + 33) 
	{
		dcs_log(0, 0, "write data len = %d too long .", nbytes);
		return -1;
	}
	
	retVal = get_fold_name_from_share_memory_by_id(folder_name,dest_folderId);
	if(retVal < 0)
	{
		dcs_log(0, 0, "获取folder %d名失败", dest_folderId);
		return -1;
	}
	sprintf(f_name, "/tmp/%s",folder_name );

	msg_packet.msg_dest = dest_folderId;
	msg_packet.msg_org = org_folderId;

	buf_len = nbytes;
	memset(msg_buf, 0x00, sizeof(msg_buf));
	msgpacket_to_str(&msg_packet, user_data, msg_buf+4, &buf_len, &data_len);
	//管道的原子操作有限制
	if(data_len +4 > PIPE_BUF) 
	{
		dcs_log(0, 0, "write data len = %d too long .", nbytes);
		return -1;
	}
	memset(len,0,sizeof(len));
	sprintf(len, "%04d", data_len);           /*Msg len*/
	memcpy(msg_buf, len, 4);        

	dcs_log(0, 0, "open folder %s", f_name);

	//opened_fid = open(f_name, O_RDWR);
	opened_fid = open(f_name, O_WRONLY);
	if(opened_fid < 0) 
	{
		dcs_log(0, 0, "open folder error !!! %s ", f_name);
		exit(1);
	}  
	max = fold_get_maxmsg(dest_folderId);
	if(max < 0)
	{
		close(opened_fid);
		return -1;
	}
	//判断fold里的message 是否超过最大值
	while(1)
	{	
		num = fold_get_msgnum(dest_folderId);
		if(max >0 && num >= max)
		{
			if(flag ==1)
			{
				dcs_log(0, 0, "fold %s内message数已超过最大值, 等待处理....", f_name);
				flag =0;
			}
			u_sleep_us(10000); //sleep 10ms 
			continue;
		}
		break;
	}
	//dcs_log(0, 0, "write %s, fid = %d,data_len=%d,len=%s", f_name, opened_fid,data_len,len);
	while(1)
	{
		retVal = (int)write(opened_fid, msg_buf, data_len + 4);
		if( retVal < 0 )
		{
			if(errno == EINTR) /* 中断错误 我们继续写*/
				continue;
			else
			{
				dcs_log(0, 0, "write %s, fid = %d,data_len=%d error :%s,retVal=%d", f_name, opened_fid,data_len,strerror(errno),retVal);
				break;
			}
		}
		else
		{
			dcs_log(0, 0, "write %s, fid = %d,data_len=%d,retVal=%d", f_name, opened_fid,data_len,retVal);
			fold_set_msgnum(dest_folderId,1);
			break;
		}
	}
	
	close(opened_fid);
	return retVal;
}


