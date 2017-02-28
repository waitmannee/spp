//
//  foldercmd.c
//  iPrepose
//
//  Created by Freax on 13-1-29.
//  Copyright (c) 2013年 Chinaebi. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include "extern_function.h"
#include "folder.h"


static void folder_cmd_exit(int signo);
int folder_list_folder_entry   (int arg_c, char *arg_v[]);
static int folder_purge(int arg_c, char *arg_v[]);
static int folder_remove(int arg_c, char *arg_v[]);


static int OpenLog()
{
    char logfile[256];

    if(u_fabricatefile("log/fold_cmd.log",logfile,sizeof(logfile)) < 0)
        return -1;

    return dcs_log_open(logfile, "fold_cmd");
}

int main(int argc, char *argv[])
{
	int s;
	catch_all_signals(folder_cmd_exit);
    	s = OpenLog();
	if(s < 0)
        return -1;
  	//attach to folder system
  	if (fold_initsys() < 0)
  	{
        fprintf(stderr,"cannot attach to FOLDER system!\n");
        exit(1);
  	}    
    
    cmd_init("foldcmd>>",folder_cmd_exit);
    cmd_add("list",     folder_list_folder_entry,   "list [folder entry]--list status of link");
     cmd_add("purge",     folder_purge,  "folder minfo get folder info");
     //cmd_add("remove",     folder_remove,  "remove folder messnum");
    
    
    cmd_shell(argc,argv);
    
  	folder_cmd_exit(0);
    return 0;
}//main()

int folder_list_folder_entry (int arg_c, char *arg_v[])
{
     int fold_shm_id;
    void *ptr = NULL;
     ptr = shm_connect(FOLDER_LIST_SHM_NAME,&fold_shm_id);
    int index = 0;
    printf("Index    Name            PID        address     references  messages   max\n");
    printf("=========================================================================\n");
    
    if(ptr == NULL) {
        printf("未能连接到fold 系统...\n");
        return -1;
    }
    FOLDERENTRY *p = (FOLDERENTRY *)ptr;
    while (strlen(p->fold_name)) {
        printf("[%.4d]  %-16s  %.6d   %.6d  %10d  %6d  %6d\n",index, p->fold_name, p->fold_pid, p->fold_address,p->fold_nref,p->fold_nmsg,p->fold_nmaxmsg);
        p ++;
        index ++;
    }
    p = NULL;
    shm_detach(ptr);
    return 0;
}

static int folder_purge(int arg_c, char *arg_v[])
{
	int folder_Id, org_folderId;
	
	int i=0,j=0;
	int num ;
	int opened_fid = -1,opened_fid2 =-1;
	char f_name[150];
	char folder_name[64];
	char user_buffer[PACKAGE_LENGTH + 33];
	
	lock_unlock_read_fold(1, 0);
	memset(f_name, 0x00, sizeof(f_name));
	memset(folder_name, 0x00, sizeof(folder_name));	

	scanf("%d", &folder_Id);
	
	get_fold_name_from_share_memory_by_id(folder_name,folder_Id);
	sprintf(f_name, "/tmp/%s",folder_name );
	opened_fid = open(f_name, O_RDWR);
	if(opened_fid < 0) {
	    dcs_log(0, 0, "open folder error !!! %s ", f_name);
	    exit(1);
	}
	num =fold_get_msgnum(folder_Id);
	dcs_log(0, 0, "read %s, fid = %d,currentnum=%d", f_name, opened_fid,num );
	char rd_buf[PACKAGE_LENGTH + 100];
	while(j<num)
	{
		memset(rd_buf, 0x00, sizeof(rd_buf));
		memset(user_buffer, 0x00, sizeof(user_buffer));
		int ret = (int)read(opened_fid, rd_buf, 4); /*4 byte; msg len*/
		if (ret < 0) 
		{
			close(opened_fid);
			dcs_log(0,0,"fold_read() failed:%s\n",ise_strerror(errno));
			return-1;
		}

		int msg_len = atoi(rd_buf);
		dcs_log(0, 0, "read first 4 baytes msg_len =%d", msg_len);
		memset(rd_buf, 0x00, sizeof(rd_buf));
		ret = (int)read(opened_fid, rd_buf, msg_len);

		int data_len = 0, buf_len = 0;
		struct msgpacket msg_packet;
		str_to_msgpacket(rd_buf, user_buffer, &msg_packet, &buf_len, &data_len,sizeof(user_buffer));
		org_folderId = msg_packet.msg_org;

		dcs_debug(user_buffer, buf_len, "read data len = %d, content len = %d.", ret, buf_len);
		if(ret == msg_len)
		{
				
			fold_set_msgnum(folder_Id,0);
					
		}
		j++;
	}
	
	close(opened_fid);

	lock_unlock_read_fold(0, 1);

	return 0;
}

static int folder_remove(int arg_c, char *arg_v[])
{
	int iRead, fromFid;
	int index;

	scanf("%d", &index);
	int i=0,j=0;
	int num =fold_get_msgnum(index);
	dcs_log(0, 0, "currentnum=%d",num);
	while(num>i)
	{
		j=0;
		while(j<5)
		{
			if(fold_set_msgnum(index,0)<0)
			{
				j++;
			continue;
			}
			else
				break;		
		}
		i++;	
	}
	dcs_log(0, 0, "succ");
}

static void folder_cmd_exit(int signo)
{
    //  	int i;
    
  	//decrease references
    //  	fold_delete_folder(g_iHstSrvFoldId);
  	exit(signo);
}


