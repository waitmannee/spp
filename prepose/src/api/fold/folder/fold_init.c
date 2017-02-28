//
//  fold_init.c
//  iPrepose
//
//  Created by Freax on 12-12-18.
//  Copyright (c) 2012年 Chinaebi. All rights reserved.
//

#include <stdio.h>

#include  <unistd.h>
#include "extern_function.h"
#include "util_func.h"


/*
 
 [32]      |0         |25        |FUNC |LOCL |0    |.text         |getlogin_r
 
 [35]      |1496      |57        |FUNC |GLOB |0    |.text         |fold_sys_tickcount
 [37]      |1448      |45        |FUNC |GLOB |0    |.text         |fold_sys_active
 [38]      |1380      |67        |FUNC |GLOB |0    |.text         |fold_check_Id
 [43]      |1328      |50        |FUNC |GLOB |0    |.text         |fold_isattach
 [47]      |1148      |180       |FUNC |GLOB |0    |.text         |fold_initsys
 

 [41]      |0         |0         |NOTY |GLOB |0    |UNDEF         |shm_connect
 [45]      |0         |0         |NOTY |GLOB |0    |UNDEF         |shm_detach
 [42]      |0         |0         |NOTY |GLOB |0    |UNDEF         |mbuf_connect
 [33]      |0         |0         |NOTY |GLOB |0    |UNDEF         |fold_unlock_folderlist
 [34]      |0         |0         |NOTY |GLOB |0    |UNDEF         |fold_lock_folderlist

 
 [40]      |0         |0         |NOTY |GLOB |0    |UNDEF         |gl_pFldArray
 [44]      |0         |0         |NOTY |GLOB |0    |UNDEF         |gl_pFldCtrl
 
 [46]      |0         |0         |NOTY |GLOB |0    |UNDEF         |gl_shmobj
 
 */

static int check_process_exist_(int pid){
    if (kill(pid, 0) == 0) { /* process is running or a zombie */
        return 0;
    } else if (errno == ESRCH) { /* no such process with the given pid is running */
        return -1;
    } else {  /* some other error... use perror("...") or strerror(errno) to report */
        perror("some other error..\n");
        return -1;
    }
    return -1;
}

int fold_sys_active()
{
    return fold_initsys();
}

/*
 int fold_initsys()
功能描述：该函数建立调用进程和Folder系统的关联，以便调用进程可以借助文件夹机制来进行数据交换。
返 回 值：函数成功时返回0；
否则返回-1，同时置errno为FOLD_ENOSYS。
说    明：一个进程希望利用Folder机制的服务，必须要首先成功地调用了该函数。
 */
int fold_initsys()
{  
	char *ptr = NULL;
	int shm_id = -1,ret;
	ptr = shm_connect(FOLDER_LIST_SHM_NAME, &shm_id);

	if(ptr == NULL)
	{
	//dcs_log(0,0,"fold_initsys error");
		return -1;
	} 

	FOLDCTRLBLK *gl_pFldCtrl = (FOLDCTRLBLK *)ptr;

	if(!gl_pFldCtrl){
		return -1;
	}

	ret=check_process_exist_(gl_pFldCtrl->foldsvr_pid);
	shm_detach(ptr);
	return ret;
}

/* 
 int fold_isattach()
 功能描述：该函数判断调用者是否已经和Folder系统建立了关联。
 返 回 值：若调用者和Folder系统建立了关联，此函数返回0，否则返回-1
 */
int fold_isattach()
{
    
    return -1;
}

int fold_check_Id(int folder_id)
{
    struct qrypacket pqry;
    pqry.fold_pid = getpid();
    pqry.fold_command = QRYPKT_CMD_LOCATE;
    memset(pqry.fold_name, 0x00, sizeof(pqry.fold_name));
    sprintf(pqry.fold_name, "%d", folder_id);
    
    int ret = fold_send_request(&pqry);    
    
    return ret;
}

