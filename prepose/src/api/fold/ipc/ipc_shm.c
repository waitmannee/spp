//
//  ipc_shm.c
//  iPrepose
//
//  Created by Freax on 12-12-19.
//  Copyright (c) 2012年 Chinaebi. All rights reserved.
//

#include <stdio.h>
#include "unixhdrs.h"
#include "folder.h"
#include "extern_function.h"


/*
 Symbols from ipc_shm.o:
 
 [Index]     Value      Size      Type  Bind  Other Shname         Name
 
 [32]      |1468      |58        |FUNC |GLOB |0    |.text         |shm_get_info
 
 [33]      |0         |0         |NOTY |GLOB |0    |UNDEF         |shmat
 [34]      |0         |0         |NOTY |GLOB |0    |UNDEF         |shmget
 
 [35]      |0         |0         |NOTY |GLOB |0    |UNDEF         |ipc_makekey
 
 [36]      |0         |0         |NOTY |GLOB |0    |UNDEF         |shmctl
 
 [37]      |1312      |26        |FUNC |GLOB |0    |.text         |shm_delete
 
 [38]      |0         |0         |NOTY |GLOB |0    |UNDEF         |shmdt
 
 [39]      |1292      |20        |FUNC |GLOB |0    |.text         |shm_detach
 [40]      |1220      |71        |FUNC |GLOB |0    |.text         |shm_connect
 [41]      |1420      |45        |FUNC |GLOB |0    |.text         |shm_attach
 [42]      |1340      |78        |FUNC |GLOB |0    |.text         |shm_getmem
 [43]      |1148      |72        |FUNC |GLOB |0    |.text         |shm_create
 
 */

/*
 5.2.1.	void * shm_create(const char *name, size_t size, int *shmid)
 功能描述：该函数创建一块命名的共享内存，并将其映射到调用进程的地址空间
 参数说明：
 name	该参数为一个指针，指向一个以NULL字符结尾的字符串，用来指定共享内存块的名称。该字符串只有前4个字符有效，并且对大小写敏感。
 size	该参数指定待创建共享内存块的字节大小
 shmid	若函数调用成功，该指针所指存储包含共享内存块的ID
 返 回 值：该函数成功时返回新建共享内存的首地址；否则返回NULL，errno指明具体的错误原因。
 说    明：shm_create()建立一个系统中尚未存在的共享内存，若系统中已存在给定名字的共享内存，则该函数失败，同时errno为EEXIST。
 */

#define OPEN_MODE 00777

void *shm_create(const  char *name, size_t size, int *shmid)
{
///================ System V IPC API =============//
    key_t key =  ipc_makekey(name) ;    
    
    if(key  < 0)
    {
	 dcs_log(0, 0, "ftok error:%s,errno:%d", strerror(errno),errno);
        return NULL;
    }

    *shmid = shmget(key, size, 0600 | IPC_CREAT | IPC_EXCL); //create share memory
    
    if(*shmid == -1)
    {
	dcs_log(0, 0, "ERROR,GET SHARE MEMEROY FAILED! error:%s,errno:%d", strerror(errno),errno);
        exit(1);
    }
    
    char *shmptr = NULL;    
    
    if((shmptr = (void *)shmat(*shmid,0,0)) == ( void*)-1) //attatch current process address namespace
    {
	 dcs_log(0, 0, "shmat error:%s,errno:%d", strerror(errno),errno);
        exit(1);
    }
    
    return shmptr;
}

/*
 5.2.2.	void *shm_connect(const char *name,int *shmid)
 功能描述：该函数打开一个已经存在的命名共享内存
 参数说明：
 name	该参数为一个指针，指向一个以NULL字符结尾的字符串，用来指定消息队列的名称。该字符串只有前4个字符有效，并且对大小写敏感。
 shmid	若函数调用成功，该指针所指存储包含共享内存块的ID
 返 回 值：该函数成功时返回共享内存的首地址；否则返回NULL，errno指明具体的错误原因。
 说    明：shm_connect()打开系统中已经存在的共享内存，若系统中不存在给定名字的共享内存，则该函数失败，同时errno为ENOENT。
 */
void *shm_connect(const char *name, int *shmid)
{
///============== System V IPC  ==========//
    
    key_t key =  ipc_makekey(name) ;
    
    int shm_id = -1;
    
    if(key  < 0)
    {
	 dcs_log(0, 0, "key error:%s,errno:%d", strerror(errno),errno);
        return NULL;
    }    
    
    if (shmid != NULL)
    {
        *shmid = shmget(key, 0, 0600 | IPC_EXCL);
        shm_id = *shmid;
    }
    else
        shm_id = shmget(key, 0, 0600 | IPC_EXCL);
    
    if(shm_id == -1)
    {
        //printf("error, get share memory failed!\n");
        dcs_log(0, 0, "get share memory failed! error:%s,errno:%d", strerror(errno),errno);
        return NULL;
    }
    
    char *shmptr = NULL;
        

    if((shmptr = (void *)shmat(shm_id,NULL,0)) == ( void*)-1) 
    {
        //fprintf(stderr,"shmat error!\n");
         dcs_log(0, 0, "shmat error! error:%s,errno:%d", strerror(errno),errno);
        return NULL;
    }
    
    return shmptr;
}

/*
 5.2.4.	int shm_detach(char *addr)
 功能描述：该函数解除共享内存在调用进程空间的映射
 参数说明：
 addr	共享内存的首地址，一般为shm_create()或shm_connect()返回值
 返 回 值：该函数成功时返回0；否则返回-1，errno指明具体的错误原因。
 */
int   shm_detach(char *addr)
{
    int ret = -1;
    if( (ret = shmdt(addr)) < 0)
    {
    	dcs_log(0, 0, "shmdt error:%s,errno:%d", strerror(errno),errno);
        return ret;
    }
    
    return 0;
}

/*
 5.2.3.	int shm_delete(int shid)
 功能描述：该函数删除给定ID的共享内存
 参数说明：
 shid	共享内存的标识，一般通过shm_create()或shm_connect()调用得到
 返 回 值：该函数成功时返回0；否则返回-1，errno指明具体的错误原因。
 */
int   shm_delete(int shid)
{
    if((shmctl(shid, IPC_RMID, 0) < 0))
    {
    	dcs_log(0, 0, "shmctl error:%s,errno:%d", strerror(errno),errno);
    }
    
    return 0;
}

void* shm_attach(int shid)
{
    char *shmptr = NULL;
    
    if((shmptr = (void *)shmat(shid,0,0)) == ( void*)-1)
    {
        fprintf(stderr,"shmat error!\n");
        return NULL;
    }
    
    return shmptr;
}

void *shm_get_info(const  char *name)
{
    int rtrn,shm_id = -1;
    struct shmid_ds shmid_ds, *buf;
    buf = & shmid_ds;
    
    key_t key =  ipc_makekey(name) ;
    
    if(key  < 0)
    {
        printf("key error:%s\n", strerror(errno));
        return NULL;
    }
    
    shm_id = shmget(key, 0, 0600 | IPC_EXCL); //find is exist share memory

    if (shm_id > 0) {
        rtrn = shmctl(shm_id, IPC_STAT,buf);
    }
    
    return buf;
}



