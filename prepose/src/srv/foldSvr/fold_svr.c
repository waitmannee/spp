//
//  fold_svr.c
//  iPrepose
//
//  Created by Freax on 13-1-30.
//  Copyright (c) 2013年 Chinaebi. All rights reserved.
//

#include <stdio.h>
#include "folder.h"
#include "i_folder.h"
#include "extern_function.h"

#ifdef __macos__
    #include <sys/semaphore.h>
#elif defined(__linux__)
    #include <semaphore.h>
#elif defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#else
    #include <semaphore.h>
#endif

FOLDCTRLBLK *gl_pFldCtrl = NULL;
FOLDERENTRY gl_pFldArray_[100];

FOLDERENTRY *gl_pFldArray = gl_pFldArray_;

static int g_foldShmId = -1;
int fold_index = -1;
int gs_myFid;
static  pthread_t sthread = 0;

extern struct nlist *shm_hashtab[101]; /* pointer table */

/*Freax Add: MAC_OS 必须有函数声明*/
static int DoLoop();
static void fold_svr_exit(int signo);
static int fold_connect_shm();
static int CreateMyFolder();
static int OpenLog(char *IDENT);
static int create_msg_queue();
int fold_msg_write_test();
extern int  process_foler_system_request();
extern int check_process_exist(int pid);

int folder_qry(char *buf, struct qrypacket* qry_packet );
int folder_io_qry(char *buf);
int fold_qry_cmd_create(struct qrypacket* request_packet, struct qrypacket* response_packet);
int fold_qry_cmd_locate(struct qrypacket* request_packet, struct qrypacket* response_packet);
int check_qry_fold_name_str_or_id(const char *file_name);
int check_folder_exist(const char *file_name);
int check_process_exist(int pid);
int get_folder_id_by_name(const char *file_name);
int check_folder_process_by_fid(int fid);
int get_fold_index_by_fid(int fid);
static  int load_fold_server_cfg();
static void create_fold_by_name(const char * fold_name);
void process_qry_request_thread(void* ptr);

int msg_id;
int msg_qry_buffer_id;
int msg_ipcs_id;
int msg_qry_ipc_id;
static int foldSvr_file_id = -1;
static int fold_response_file_id = -1;

static int whileCnt = 0;

//int main_fold_svr(int argc, char *argv[])
int main(int argc, char *argv[])
{
    
    catch_all_signals(fold_svr_exit);
    
    //prepare the logging stuff
    if(0 > OpenLog(argv[0])) exit(1);
    
    dcs_log(0,0, "fold Server is starting up...\n");
    u_daemonize(NULL);
    
    
    if(load_fold_server_cfg() < 0) //load folder configure
    {
        dcs_log(0,0,"load_fold_server_cfg() failed:%s\n",strerror(errno));
        fold_svr_exit(0);
    }    
    
    //attach to SHM of FoldSvr
    if(fold_connect_shm() < 0)
    {
        dcs_log(0,0,"fold_connect_shm() failed:%s\n",strerror(errno));
        exit(1);
    }
    
    //attach to folder system and create folder of myself
    if( CreateMyFolder() < 0 )  fold_svr_exit(0);
    
    dcs_log(0,0,"*************************************************\n"
            "!********fold svr startup completed.*********!!\n"
            "*************************************************\n");
    
    DoLoop();
    
    fold_svr_exit(0);
    
    return 0;
}//main()

void clear_msg_queue_timer(int msg_id){
    int timer = 0;
    while (queue_status_num_msg(msg_id) > 0) {
        sleep(1);
        timer ++;
        if (timer > 10) {
            queue_empty(msg_id);
        }
    }

}

static int DoLoop()
{ 
	char rd_buf[1024];
	int    iRead;

	while (1) 
	{
		dcs_log(0, 0, "fold server while ....");
		memset(rd_buf, 0x00, sizeof(rd_buf));
		iRead = (int)read(foldSvr_file_id, rd_buf, sizeof(rd_buf));

		if(iRead < 0 && errno != EINTR)
		{
			dcs_log(0,0,"fold server recv failed:%s, fold server exist! \n",strerror(errno));
			break;
		}
		if (errno == EINTR) 
		{
			continue;
		}
		process_foler_system_request(rd_buf);
	}
	return 0;
}

void process_qry_request_thread(void* ptr){
    
    dcs_log(0, 0, "process_qry_request_thread:并发，buffer 中的数据个数 =>> %d", queue_status_num_msg(msg_qry_buffer_id));
    while (queue_status_num_msg(msg_qry_buffer_id) > 0) {
        clear_msg_queue_timer(msg_qry_ipc_id);
        MSGBUF rad_buf;
        queue_recv(msg_qry_buffer_id, &rad_buf, MSGSIZE, 1);
        queue_send(msg_qry_ipc_id, &rad_buf, (int)strlen(rad_buf.mtext) + 1, 1);
        //clear_msg_queue_timer(msg_ipcs_id);
    }
//    pthread_detach(pthread_self());
    sthread = 0;
}


int  process_foler_system_request(char *buf){
    
    char type_buf[4];
    memset(type_buf, 0x00, 4);
    memcpy(type_buf, buf, 1);
    
    int req_type = atoi(type_buf);
    
    switch (req_type) {
        case FOLDER_REQ_FOLDER_QRY_TYPE:  //Folder locate/create cmd
        {
            struct qrypacket response_packet;
            memset(response_packet.fold_name, 0x00, sizeof(response_packet.fold_name));
            
            folder_qry(buf, &response_packet); //process folder request
            
            char qry_str[512];
            memset(qry_str, 0x00, sizeof(qry_str));
            qrypacket_to_str(&response_packet, qry_str);  //** 将 qry_packet 拼装成字符串发出去 **//
            
            if (response_packet.fold_result != -1) {
                dcs_log(0, 0, "Folder: name = %s, result = %d, locate/create : suceess !", response_packet.fold_name, response_packet.fold_result);
            } else {
                dcs_log(0, 0, "Folder: name = %s, result = %d, locate/create : error !", response_packet.fold_name, response_packet.fold_result);
            }
            
            dcs_log(0, 0, "write from fold server... %s", qry_str);
            write(fold_response_file_id, qry_str,strlen(qry_str));
            
        }
            break;
        case FOLDER_REQ_FOLDER_IO_TYPE:   //Folder write/read cmd
        {
            if (folder_io_qry(buf) == 0) { //check_fold_id success
                //int msg_id = queue_connect(FOLDER_SYSTEM_WITH_PROCESS_IPC_MSG_NAME);
                
                int buf_len = 0, data_len = 0;
                char user_buf[1024];
                memset(user_buf, 0x00, sizeof(user_buf));
                
                struct msgpacket msg_packet;
                str_to_msgpacket(buf, user_buf, &msg_packet, &buf_len, &data_len,sizeof(user_buf));
                
                MSGBUF msg_buf;
                memset(msg_buf.mtext, 0x00, sizeof(msg_buf.mtext));
                memcpy(msg_buf.mtext, buf, data_len);
                msg_buf.mtype = 1;
                
            } else { //check_fold_id fail
                
            }
        }
            break;
        default:
            break;
    }
    return 0;
}

int folder_qry(char *buf, struct qrypacket* response_packet ){
    
    struct qrypacket request_pack;
    str_to_qrypacket(buf, &request_pack);
    
    //printf("qry_packet.fold_name =>> %s\n", request_pack->fold_name);
    
    switch (request_pack.fold_command) {
        case QRYPKT_CMD_CREATE:
        {
            dcs_log(0, 0, "Folder:<< create >> folder = %s", request_pack.fold_name);
            fold_qry_cmd_create(&request_pack, response_packet);
        }
            break;
        case QRYPKT_CMD_LOCATE:
        {
            dcs_log(0, 0, "Folder:<< locate >> folder = %s", request_pack.fold_name);
            fold_qry_cmd_locate(&request_pack, response_packet);
        }
            break;
        case QRYPKT_CMD_DELETE:
        {
            
        }
            break;
        case QRYPKT_CMD_EXIT:
        {
            exit(0);
        }
            break;
        default:
            break;
    }
    return 0;
}

int folder_io_qry(char *buf){
    int buf_len = 0, data_len = 0;
    char user_buf[1024];
    memset(user_buf, 0x00, sizeof(user_buf));
    struct msgpacket msg_packet;
    str_to_msgpacket(buf, user_buf, &msg_packet, &buf_len, &data_len,sizeof(user_buf));
    dcs_debug(user_buf,buf_len,"Folder system read/write");
    dcs_log(0, 0, "fold server fold write .msg_dest = %d", msg_packet.msg_dest);
    return check_folder_process_by_fid(msg_packet.msg_dest);
}

//=========== 文件操作 =============//
//** Create **//
int fold_qry_cmd_create(struct qrypacket* request_packet, struct qrypacket* response_packet)
{
	if (check_folder_exist(request_packet->fold_name) == 0) 
	{
		dcs_log(0, 0, "Folder:<< create >> folder = %s failed; 存在", request_packet->fold_name);
		response_packet->fold_result = get_folder_id_by_name(request_packet->fold_name);
	} else 
	{
		void *ptr = NULL;
		FOLDERENTRY fold;
		int g_foldShmId;
		memset(&fold,0,sizeof(FOLDERENTRY));
		ptr = shm_connect(FOLDER_LIST_SHM_NAME,&g_foldShmId);
		if(ptr == NULL)
		{
			dcs_log(0,0,"cannot connect SHM '%s':%s!\n",FOLDER_LIST_SHM_NAME, strerror(errno));
			return -1;
		}
		fold_index ++;		
		sprintf(fold.fold_name,"%s", request_packet->fold_name);
		fold.fold_pid = request_packet->fold_pid;
		fold.fold_address = getpid() + fold_index;
		gl_pFldArray[fold_index] = fold;

		response_packet->fold_result = fold_index;

		dcs_log(0, 0, "Folder:<< create >> folder = %s success;", request_packet->fold_name);
		memcpy(ptr+fold_index* sizeof(FOLDERENTRY), &fold, sizeof(FOLDERENTRY)); //将新加的folder 信息放入共享内存
		dcs_log(0, 0, "folder share memory size = %d", (fold_index + 1) * sizeof(FOLDERENTRY));
		//fold_connect_shm();

		char fname[100];
		memset(fname, 0x0, sizeof(fname));
		sprintf(fname, "/tmp/%s", request_packet->fold_name);
		if( access( fname, F_OK ) == -1 ) {
		    int res = mkfifo( fname, 0777 );
		    if( res != 0 ){
		        dcs_log(0, 0, "Could not create fifo error !!! %s ", request_packet->fold_name );
		    }
		}
		shm_detach(ptr);

    }    
    
    response_packet->fold_msgtype = FOLDER_QRY_MSG_TYPE_RESPONSE;
    response_packet->fold_pid     = request_packet->fold_pid;
    response_packet->fold_command = request_packet->fold_command;
    memset(response_packet->fold_name, 0x00, sizeof(response_packet->fold_name));
    memcpy(response_packet->fold_name, request_packet->fold_name, strlen(request_packet->fold_name));    
    return 0;
}

//** Locate **//
int fold_qry_cmd_locate(struct qrypacket* request_packet, struct qrypacket* response_packet){
    if (check_qry_fold_name_str_or_id(request_packet->fold_name) == 0) { //数字
        int fold_id = atoi(request_packet->fold_name);
        dcs_log(0, 0, "fold_id = %d", fold_id);
        FOLDERENTRY fold = gl_pFldArray[fold_id];
        dcs_log(0, 0, "fold_id = %d, fold_name = %s", fold_id, fold.fold_name);
        
        if(check_process_exist(fold.fold_pid) != 0){
            dcs_log(0, 0, "Folder:<< locate >> folder = %s failed; process exit", request_packet->fold_name);
            response_packet->fold_result = -1;
        } else {
            dcs_log(0, 0, "Folder:<< locate >> folder = %s success;", request_packet->fold_name);
            response_packet->fold_result = fold_id;
            memcpy(response_packet->fold_name, fold.fold_name, strlen(fold.fold_name));
        }
    } else {   //字符
        if (check_folder_exist(request_packet->fold_name) == 0) {
            int fold_id = get_folder_id_by_name(request_packet->fold_name);
            if(fold_id == -1){
                dcs_log(0, 0, "Folder:<< locate >> folder = %s failed; folder is not exist", request_packet->fold_name);
                response_packet->fold_result = -1;
            }
            else {
                FOLDERENTRY fold = gl_pFldArray[fold_id];
                if(check_process_exist(fold.fold_pid) != 0){
                    dcs_log(0, 0, "Folder:<< locate >> folder = %s failed;process exit", request_packet->fold_name);
                    response_packet->fold_result = -1;
                } else {
                    dcs_log(0, 0, "Folder:<< locate >> folder = %s success;", request_packet->fold_name);
                    response_packet->fold_result = fold_id;
                }
            }
        } else {
            dcs_log(0, 0, "Folder:<< locate >> folder = %s failed;process exit", request_packet->fold_name);
            response_packet->fold_result = -1;
        }
        
        memset(response_packet->fold_name, 0x00, sizeof(response_packet->fold_name));
        memcpy(response_packet->fold_name, request_packet->fold_name, strlen(request_packet->fold_name));

    }
    
    response_packet->fold_msgtype = FOLDER_QRY_MSG_TYPE_RESPONSE;
    response_packet->fold_pid     = request_packet->fold_pid;
    response_packet->fold_command = request_packet->fold_command;
    
    return 0;
}


/*
 * 根据进程id检查进程是否存在
 * 返回值: 0:存在; -1:不存在
 */
int check_qry_fold_name_str_or_id(const char *file_name){
    char br = 0;
    for (; *file_name; file_name++) {
        if (!isdigit(*file_name)) {
            br = -1;
            break;
        }
    }
    return br;
}

/*
 * 根据文件名字检查文件是否存在
 * 返回值: 0:存在; -1:不存在
 */
int check_folder_exist(const char *file_name){
    char ret = -1;
    FOLDERENTRY *pfold = gl_pFldArray;
    
    for (; strlen(pfold->fold_name); pfold++) {
        if (strcmp(file_name, pfold->fold_name) == 0) {
            ret = 0;
            break;
        }
    }
    return ret;
}

/*
 * 根据文件名字检查文件是否存在
 * 返回值: 0:存在; -1:不存在
 */
int get_folder_id_by_name(const char *file_name){
    char ret = -1;
    int index = -1;
    FOLDERENTRY *pfold = gl_pFldArray;
    for (; strlen(pfold->fold_name); pfold++) {
        index ++;
        if (strcmp(file_name, pfold->fold_name) == 0) {
            ret = index;
            break;
        }
    }
    return ret;
}


/*
 * 根据文件id检查文件所在的进程是否存在
 * 返回值: 0:存在; -1:不存在
 */
int check_folder_process_by_fid(int fid){
    int index = get_fold_index_by_fid(fid);
    if (index != -1) {
        FOLDERENTRY fold = gl_pFldArray[fid];
        if(check_process_exist(fold.fold_pid) != 0){
            return -1;
        }
    }
    return 0;
}

/*
 * 根据fold id得到在foldArray中的索引
 * 返回值: -1:这个文件id不存在;
 */
int get_fold_index_by_fid(int fid){
    char ret = -1;
    int index = -1;
    FOLDERENTRY *pfold = gl_pFldArray;
    for (; strlen(pfold->fold_name); pfold++) {
        index ++;
        if (pfold->fold_address == (getpid() + fid)) {
            ret = index;
            break;
        }
    }
    return index;
}

static void create_fold_by_name(const char * fold_name){
    dcs_log(0, 0, "Folder server create folder = %s", fold_name);    
    if (check_folder_exist(fold_name) < 0) {
        char fname[100];
        memset(fname, 0x0, sizeof(fname));
        sprintf(fname, "/tmp/%s", fold_name);
        if( access( fname, F_OK ) == -1 ) {
            int res = mkfifo( fname, 0777 );
            if( res != 0 ){                      
                dcs_log(0,0, "Could not create fifo %s ", fold_name );
            }
        }

        //int fid = open( fname, O_RDWR );
        //dcs_log(0, 0, "File Name = %s, File id = %d", fname, fid);
        
        fold_index ++;
        FOLDERENTRY fold;
        memset(&fold,0,sizeof(FOLDERENTRY));
        memcpy(fold.fold_name, fold_name, strlen(fold_name));
        fold.fold_pid = getpid();
        fold.fold_address = getpid() + fold_index;
        gl_pFldArray[fold_index] = fold;
    }
}

static int create_msg_queue(){
    queue_create(FOLDER_SYSTEM_IPC_MSG_NAME);
    queue_create(FOLDER_SYSTEM_WITH_PROCESS_IPC_MSG_NAME);
    queue_create(FOLDER_SYSTEM_QRY_IPC_MSG_NAME);
    queue_create(FOLDER_SYSTEM_BUFFER_IPC_MSG_NAME);
    return 1;
}

static int fold_connect_shm()
{
    void *ptr = NULL;
    ptr = shm_connect(FOLDER_LIST_SHM_NAME,&g_foldShmId);
    
    if (ptr == NULL) {
        ptr = shm_create(FOLDER_LIST_SHM_NAME, 4096, &g_foldShmId);
        if (!ptr) dcs_log(0,0,"create SHM '%s':%s is OK!\n",FOLDER_LIST_SHM_NAME, strerror(errno));
    }
    
    if(ptr == NULL)
    {
        dcs_log(0,0,"cannot connect SHM '%s':%s!\n",FOLDER_LIST_SHM_NAME, strerror(errno));
        return -1;
    }
    
    memcpy(ptr, gl_pFldArray, (fold_index + 1) * sizeof(FOLDERENTRY)); //put fold server into share memory
    
    dcs_log(0, 0, "folde index = %d", fold_index);
    dcs_log(0, 0, "sizeof(FOLDERENTRY) size = %d", sizeof(FOLDERENTRY));
    dcs_log(0, 0, "folder share memory size = %d", (fold_index + 1) * sizeof(FOLDERENTRY));
    dcs_log(0,0,"connect SHM '%s' is OK, folder ShmId = %d!\n",FOLDER_LIST_SHM_NAME, g_foldShmId);
    shm_detach(ptr);
    return 0;
}


static int load_fold_server_cfg(){
    char   caBuffer[512];
  	int    iFd=-1,iRc,iCnt;
    char cfgfile[256];
    
    if(u_fabricatefile("config/folder.conf",cfgfile,sizeof(cfgfile)) < 0)
    {
        dcs_log(0,0,"cannot get full path name of 'folder.conf'\n");
        return -1;
    }
    
    if(0 > (iFd = conf_open(cfgfile)) )
    {
        dcs_log(0,0,"open file '%s' failed.\n",cfgfile);
        return -1;
    }
    
    //读取配置项中的配置项
    if(0 > conf_get_first_number(iFd,"nFolders",&iCnt)|| iCnt < 0 )
    {
        dcs_log(0,0,"<PosCom> load config item 'comm.maxgrp' failed!");
        conf_close(iFd);
        return -1;
    }
    //printf("iCnt =>> %d\n", iCnt);
    
    char *foldname;
    int index = 0;
	iRc = conf_get_first_string(iFd, "folder.entry",caBuffer);
    for( ;
        iRc == 0 && index < iCnt;
        iRc = conf_get_next_string(iFd, "folder.entry",caBuffer))
    {

        foldname   = caBuffer;     //文件名字
        
        if (foldname == NULL )
        {
			dcs_log(0, 0, "invalid address:skip this line 1....");
            continue; //invalid address:skip this line
        }
        
        create_fold_by_name(foldname);
        
        index ++;
    }
    
    conf_close(iFd);
    
    return 0;
}



static int CreateMyFolder()
{
    if( access( PIPE_FOLD_RESPONSE_FOLDER_NAME, F_OK ) == -1 ) {
        int res = mkfifo( PIPE_FOLD_RESPONSE_FOLDER_NAME, 0777 );
        if( res != 0 ){
            dcs_log( 0, 0, "Could not create fifo %s ",  PIPE_FOLD_RESPONSE_FOLDER_NAME);
        }
    }
    
    fold_response_file_id = open( PIPE_FOLD_RESPONSE_FOLDER_NAME, O_RDWR );
    
    if( access( PIPE_FOLD_SERVER_FOLDER_NAME, F_OK ) == -1 ) {
        int res = mkfifo( PIPE_FOLD_SERVER_FOLDER_NAME, 0777 );
        if( res != 0 ){
            dcs_log(0,0 , "Could not create fifo %s ",  PIPE_FOLD_SERVER_FOLDER_NAME);
        }
    }
    
    foldSvr_file_id = open( PIPE_FOLD_SERVER_FOLDER_NAME, O_RDWR );
    
    return foldSvr_file_id;
}

static void fold_svr_exit(int signo)
{
	dcs_log(0, 0, "folder svr catch a sinal %d", signo);

	close(foldSvr_file_id);
	close(fold_response_file_id);

	exit(signo);
}

static int OpenLog(char *IDENT)
{
    char logfile[256];
    if(u_fabricatefile("log/foldSvr.log",logfile,sizeof(logfile)) < 0)
        return -1;
    return dcs_log_open(logfile, IDENT);
}
