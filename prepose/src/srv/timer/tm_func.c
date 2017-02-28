#include <stdio.h>
#include "timer.h"
#include "extern_function.h"

/*
 
Description:
1.compiled with other process, eg: TransRcv
2.offer api service to other process to do out of time control service

 add by daniel 2016.8.15
*/

int tm_free_entry(struct Timer_struct strTimer)
{
    return -1;
}

/*向超时控制里添加一条
*/
int tm_insert_entry(struct Timer_struct strTimer)
{
	int org_folderId= 0;
	int myFid = 0 , length = 0;
	char buf[sizeof(struct Timer_struct)];
	
	myFid = fold_locate_folder(TIMER_FOLDER);
	if(myFid < 0) 
		myFid = fold_create_folder(TIMER_FOLDER);

	if(myFid < 0)
	{
		dcs_log(0,0,"cannot create folder '%s':%s\n",TIMER_FOLDER, ise_strerror(errno) );
	}

	memset(buf, 0, sizeof(buf));
	//memcpy(buf+1, &strTimer,sizeof(struct Timer_struct ));
	sprintf(buf,"%c%-20s%c%c%c%04d",TIMER_ADD,strTimer.key,strTimer.timeout,strTimer.sendnum,strTimer.foldid,strTimer.length);
	length = 28;
	memcpy(buf + length,strTimer.data,strTimer.length);
	length += strTimer.length;
	fold_write(myFid, org_folderId, buf, length);
	
	return 0;
}

/*从超时控制里删除一条
*/
int tm_remove_entry(char * key)
{
	int org_folderId= 0;
	int myFid = 0;
	char buf[100];

	if(strlen(key) > 20)
	{
		dcs_log(0,0,"key 太长.key = %s",key);
		return -1;
	}
	
	myFid = fold_locate_folder(TIMER_FOLDER);
	if(myFid < 0) 
		myFid = fold_create_folder(TIMER_FOLDER);

	if(myFid < 0)
	{
		dcs_log(0,0,"cannot create folder '%s':%s\n",TIMER_FOLDER, ise_strerror(errno) );
	}

	memset(buf, 0, sizeof(buf));
	buf[0] = TIMER_REMOVE;
	strcpy(buf+1, key);
	
	fold_write(myFid, org_folderId, buf, strlen(buf));
	
	return 0;
}


