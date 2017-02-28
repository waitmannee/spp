//
//  fold_locate.c
//  iPrepose
//
//  Created by Freax on 12-12-19.
//  Copyright (c) 2012年 Chinaebi. All rights reserved.
//

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
//#include "extern_function.h"

/*
 Symbols from fold_locate.o:
 
 [Index]     Value      Size      Type  Bind  Other Shname         Name
  
 [35]      |1336      |177       |FUNC |GLOB |0    |.text         |fold_get_name
 [42]      |1148      |188       |FUNC |GLOB |0    |.text         |fold_locate_folder
 
 [33]      |0         |0         |NOTY |GLOB |0    |UNDEF         |gl_pFldArray
 [34]      |0         |0         |NOTY |GLOB |0    |UNDEF         |fold_check_Id
 [36]      |0         |0         |NOTY |GLOB |0    |UNDEF         |fold_wait_answer
 [37]      |0         |0         |NOTY |GLOB |0    |UNDEF         |fold_send_request
 [41]      |0         |0         |NOTY |GLOB |0    |UNDEF         |fold_isattach
 */


/*
 4.5.	int  fold_locate_folder(const char *folder_name)
 
 功能描述：该函数定位一个给定名字的文件夹。
 参数说明：folder_name:  待定位文件夹的名字
 返 回 值：若建立成功，函数返回文件夹的Id；
 
 否则返回-1，errno给出具体地原因：
    FOLD_ENOSYS   进程没有连上Folder系统
    FOLD_EINVAL   无效的参数
    FOLD_ENOENT   给定名字的文件夹并不存在
 */

int  fold_locate_folder(const char *folder_name)
{
	int ret_id = -1;
	get_fold_id_from_share_memory_by_name((char *)folder_name, &ret_id);
    return ret_id;
}

extern void clear_queue_by_id(int msg_id);

int fold_get_name (int folder_id, char *folder_name,int nsize)
{
   return get_fold_name_from_share_memory_by_id(folder_name,folder_id);
}


