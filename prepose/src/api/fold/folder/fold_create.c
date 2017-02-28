//
//  fold_create.c
//  iPrepose
//
//  Created by Freax on 12-12-19.
//  Copyright (c) 2012年 Chinaebi. All rights reserved.
//

#include <stdio.h>
#include <dirent.h>
#include "extern_function.h"

extern SHMOBJ       gl_shmobj;
extern FOLDCTRLBLK *gl_pFldCtrl;
extern FOLDERENTRY *gl_pFldArray;
extern struct msgpacket gl_msgpacket;
extern int fold_index;

/*
 void  lock_unlock_fold_request(int lock, int unlock)
 功能描述：该函数用户请求的同步。
 参数说明：lock  1: 上锁 0:解锁
 返 回 值：无；
 */
void lock_unlock_fold_request(int lock, int unlock){
    int sem_id = sem_connect(FOLDER_SYSTEM_IPC_SEM_NAME, 2);
    if (sem_id < 0) sem_id = sem_create(FOLDER_SYSTEM_IPC_SEM_NAME, 2);
    if (sem_id > 0) {
        if (lock) {
            //dcs_log(0, 0, "lock_fold_request.semid = %d", sem_id);
            sem_lock(sem_id, 1, 1);
        }
        if (unlock) {
            //dcs_log(0, 0, "unlock_fold_request.semid = %d", sem_id);
            sem_unlock(sem_id, 1, 1);
        }
    }
}

/*
 int  fold_create_folder(const char *folder_name)
 功能描述：该函数以给定的名字建立一个普通文件夹。
 参数说明：folder_name  待建立的文件夹的名字
 返 回 值：若建立成功，函数返回新建文件夹的Id；
 否则返回-1，errno给出具体地原因：
 FOLD_ENOSYS   进程没有连上Folder系统
 FOLD_EINVAL   无效的参数
 FOLD_ENOFOLD  Folder系统已没有空闲表项用来建立新的文件夹
 FOLD_EEXIST   给定名字的文件夹已经存在
 */
int  fold_create_folder(const char *folder_name)
{
    int fid; 
    lock_unlock_fold_request(1, 0);
    
    fid = fold_locate_folder(folder_name);
    if (fid != -1) {
        //dcs_log(0, 0, "fold_create_folder......lock,unlock....get from shm! fid = %d", fid);
        lock_unlock_fold_request(0, 1);
        return fid;
    }
    
    //dcs_log(0, 0, "fold_create_folder......lock,unlock....");
    
    struct qrypacket pqry;
    pqry.fold_msgtype = FOLDER_QRY_MSG_TYPE_REQUEST;
    pqry.fold_pid = getpid();
    pqry.fold_command = QRYPKT_CMD_CREATE;
    memset(pqry.fold_name, 0x00, sizeof(pqry.fold_name));
    memcpy(pqry.fold_name, folder_name, strlen(folder_name));
    int rtn = fold_send_request(&pqry);
    
    lock_unlock_fold_request(0, 1);
    return rtn;
}

/*
 int fold_create_anonymous()
 功能描述：该函数字建立一个匿名文件夹。
 返 回 值：若建立成功，函数返回新建文件夹的Id；
 否则返回-1，errno给出具体地原因：
 FOLD_ENOSYS   进程没有连上Folder系统
 FOLD_ENOFOLD  Folder系统已没有空闲表项用来建立新的文件夹
 说    明：有时进程希望建立一个文件夹用来和其它进程进行临时的通信，但又不知用什么样的文件夹名字时，可使用该函数来建立一个匿名的文件夹
 */

int  fold_create_anonymous()
{
    return -1;
}

/*
 4.6.	int fold_delete_folder(int folder_id)
 功能描述：该函数删除一个给定Id的文件夹。
 参数说明：
 folder_Id:  待删除文件夹的Id
 返 回 值：若删除成功，函数返回0;
 否则返回-1，errno给出具体地原因：
 FOLD_ENOSYS   进程没有连上Folder系统
 FOLD_EINVAL   无效的参数
 FOLD_ENOENT   给定Id的文件夹并不存在
 说    明：文件夹的生命期基于引用计数，只有当其引用计数为0时，系统才真正地删除指定的文件夹。fold_delete_folder()只是将文件夹的引用计数减一，若要物理地删除一个文件夹，应该调用函数fold_destroy_folder()。
 */

int fold_delete_folder(int folder_id)
{
    
    return  -1;
}


/*
 4.7.	int fold_destroy_folder(int folder_id)
 功能描述：该函数强制性地删除一个给定Id的文件夹。
 参数说明：
 folder_Id:  待删除文件夹的Id
 返 回 值：若删除成功，函数返回0;
 否则返回-1，errno给出具体地原因：
 FOLD_ENOSYS   进程没有连上Folder系统
 FOLD_EINVAL   无效的参数
 FOLD_ENOENT   给定Id的文件夹并不存在
 */

int fold_destroy_folder(int folder_id)
{
    
    return -1;
}
