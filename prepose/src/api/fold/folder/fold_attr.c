//
//  fold_attr.c
//  iPrepose
//
//  Created by Freax on 12-12-19.
//  Copyright (c) 2012年 Chinaebi. All rights reserved.
//

#include <stdio.h>
#include "folder.h"
#include "extern_function.h"

/*获取folder设置的最大message限制值
*/
int fold_get_maxmsg(int folder_id)
{ 
	int fold_shm_id=0;
	FOLDERENTRY *folder;
	char *ptr;
	int index =-1,max=0;
	ptr=shm_connect(FOLDER_LIST_SHM_NAME,&fold_shm_id);
	if(ptr == NULL){
		dcs_log(0, 0, "get_fold_name_from_share_memory_by_id,error!");
		return -1;
	}
	folder =(FOLDERENTRY *)ptr;
	for (; strlen(folder->fold_name); folder++) 
	{
		index ++;
		if (folder_id == index) 
		{
			max=folder->fold_nmaxmsg;
			//dcs_log(0,0,"getptr[%s]id=[%d]value=[%d]",folder->fold_name,folder_id,max);
			shm_detach(ptr);
			return  max;
		}
	}
	shm_detach(ptr);
	return -1;
}
/*设置folder最大message限制值
*/
int fold_set_maxmsg(int folder_id, int nmaxmsg)
{
	int fold_shm_id=0;
	FOLDERENTRY *folder;
	char *ptr;
	int index =-1;
	ptr=shm_connect(FOLDER_LIST_SHM_NAME,&fold_shm_id);
	if(ptr == NULL){
		dcs_log(0, 0, "get_fold_name_from_share_memory_by_id,error!");
		return -1;
	}
	folder =(FOLDERENTRY *)ptr;
	for (; strlen(folder->fold_name); folder++) 
	{
		index ++;
		if (folder_id == index) 
		{
			//dcs_log(0,0,"setptr[%s]id=[%d]value=[%d]",folder->fold_name,folder_id,nmaxmsg);
			folder->fold_nmaxmsg = nmaxmsg;
			shm_detach(ptr);
			return 0;
		}
	}
	shm_detach(ptr);
	return -1;
}
/*获取folder当前的message数量
*/
int fold_get_msgnum(int folder_id)
{ 
	int fold_shm_id=0;
	FOLDERENTRY *folder;
	char *ptr;
	int index =-1,num=0;
	ptr=shm_connect(FOLDER_LIST_SHM_NAME,&fold_shm_id);
	if(ptr == NULL){
		dcs_log(0, 0, "get_fold_name_from_share_memory_by_id,error!");
		return -1;
	}
	folder =(FOLDERENTRY *)ptr;
	for (; strlen(folder->fold_name); folder++) 
	{
		index ++;
		if (folder_id == index) 
		{
			num=folder->fold_nmsg;
			shm_detach(ptr);
			return  num;
		}
	}
	shm_detach(ptr);
	return -1;
}
/*修改folder当前message数量
*FLAG 增加减少标志,1:数量加1 0:数量减1
*同时增加folder引用次数
*/
int fold_set_msgnum(int folder_id, int flag)
{
	int fold_shm_id=0;
	FOLDERENTRY *folder;
	char *ptr;
	int index =-1;
	int sem_id = -1;
	char sem_name[50+1];
	memset(sem_name,0,sizeof(sem_name));
	ptr=shm_connect(FOLDER_LIST_SHM_NAME,&fold_shm_id);
	if(ptr == NULL){
		dcs_log(0, 0, "get_fold_name_from_share_memory_by_id,error!");
		return -1;
	}
	folder =(FOLDERENTRY *)ptr;
	for (; strlen(folder->fold_name); folder++) 
	{
		index ++;
		if (folder_id == index) 
		{
			sprintf(sem_name,"fold_%s",folder->fold_name);
			sem_id= sem_connect(sem_name, 2);
	              if (sem_id < 0) sem_id = sem_create(sem_name, 2);
	              sem_lock(sem_id, 1, 1);
			folder->fold_nref++;//引用次数加1
			if(flag ==1)
			{
				folder->fold_nmsg ++;
			}
			else if(flag ==0)
			{
				folder->fold_nmsg --;
			}
			else
			{
				sem_unlock(sem_id, 1, 1);
				break;
			}
			sem_unlock(sem_id, 1, 1);
			shm_detach(ptr);
			return 0;
		}
	}
	shm_detach(ptr);
	return -1;
}
int fold_unset_attr(int folder_id, int folder_flag)
{
    return -1;
}

int fold_set_attr(int folder_id, int folder_flag)
{
    return -1;
}

int fold_count_messags(int folder_id){
    
    return -1;
}

