//
//  extern_function.h
//  iPrepose
//
//  Created by Freax on 13-1-29.
//  Copyright (c) 2013年 Chinaebi. All rights reserved.
//

#ifndef iPrepose_extern_function_h
#define iPrepose_extern_function_h

#define PREPOSE_RELEASE

/*
 * Add this file for less warning
 */
#include "timer.h"
#include "iso8583.h"
#include "folder.h"

//log
#define dcs_log(ptrbytes, nbytes,message,...)   pre_dcs_log(__FILE__,__LINE__,(ptrbytes),(nbytes),(message),##__VA_ARGS__)
#define dcs_debug(ptrbytes, nbytes,message,...)  pre_dcs_debug(__FILE__,__LINE__,(ptrbytes),(nbytes),(message),##__VA_ARGS__)

extern int dcs_log_open(const char * logfile, char *ident);

//confige
extern int  conf_get_first_string(int handle, const char *key, char *out_string);
extern int  conf_get_next_string(int handle, const char *key, char *out_string);
extern int  conf_open(const char *filename);
extern int  conf_get_first_number(int handle, const char *key, int *out_value);
extern int  conf_close(int handle);
extern int  conf_get_next_number(int handle, const char *key, int *out_value);

//Util
extern int  u_stricmp(char *s1,char *s2);
extern int  u_fabricatefile(const char *file_name, char *out_buffer,int nsize);
extern int  u_daemonize(const char *directory);
extern void u_sleep_us(int time);
/*
 * 根据进程id检查进程是否存在
 * 返回值: 0:存在; -1:不存在
 */
extern int check_process_exist(int pid);

//Folder
extern int  fold_create_folder(const char *folder_name);
extern int fold_sys_active();
extern int fold_initsys();
extern int  fold_locate_folder(const char *folder_name);
extern int fold_get_maxmsg(int folder_id);
extern int fold_set_maxmsg(int folder_id, int nmaxmsg);
extern int fold_get_msgnum(int folder_id);
extern int fold_set_msgnum(int folder_id, int flag);
extern int fold_read(int folder_Id,int open_fid,int* org_folderId,void *user_buffer,int nsize,int fBlocked);
extern int fold_open(int folder_Id);
extern int fold_write(int dest_folderId,int org_folderId,void *user_data,int nbytes);
extern int fold_delete_folder(int folder_id);
extern int fold_purge_msg(int foldid);
extern int fold_unset_attr(int folder_id, int folder_flag);
extern int fold_set_attr(int folder_id, int folder_flag);
extern int fold_check_Id(int folder_id);
extern int fold_get_name (int folder_id, char *folder_name,int nsize);
extern int fold_count_messags(int folder_id);
extern int get_fold_name_from_share_memory_by_id(char *fold_name,int  fid);
extern int get_fold_id_from_share_memory_by_name(char *fold_name, int * fid);
//===== Folder send request
extern int fold_send_request(struct qrypacket* qry_packet);
extern int fold_send_msgpacket(struct msgpacket *msg, void *buf, int nbytes);
extern int fold_wait_answer();

extern void qrypacket_to_str(struct qrypacket* qry_packet, char *ret_buf);
extern void str_to_qrypacket( char *qry_buf, struct qrypacket* qry_packet_respon);

extern void msgpacket_to_str(struct msgpacket* msg_packet, char *user_buf, char *ret_buf, int *buf_len, int *data_len);
extern void str_to_msgpacket(char *msg_buf, char *user_buf, struct msgpacket* msg_pkt, int *buf_len, int *data_len,int nsize);

//common
extern void bcd_to_asc (unsigned char* ascii_buf ,unsigned char* bcd_buf , int conv_len ,unsigned char type );
extern void asc_to_bcd ( unsigned char* bcd_buf , unsigned char* ascii_buf , int conv_len ,unsigned char type );
extern int IsoLoad8583config(struct ISO_8583 *pIso8583Bit ,char *pFile );
extern int iso_to_str( unsigned char * dstr , ISO_data * iso, int flag );
extern int str_to_iso( unsigned char * dstr, ISO_data * iso, int Len );
extern int  getstrlen(const char *str);

//signal
extern int  catch_all_signals(sigfunc_t  pfn);
extern int  wait_child_or_signal(int *stat_loc);

//shared memory
extern void* shm_attach(int shid);
extern void *shm_create(const  char *name, size_t size, int *shmid);
extern void *shm_connect(const char *name, int *shmid);
extern int   shm_detach(char *addr);
extern int   shm_delete(int shid);
extern void *shm_get_info(const  char *name);


//semaphore
extern int sem_create(const char *name, int nsem);
extern int sem_connect(const char *name, int nsems);
extern int sem_delete(int semid);
extern int sem_lock(int semid, int semnum, int lockflg);
extern int sem_unlock(int semid, int semnum, int lockflg);


//message queue
extern int queue_create(const char *name);
extern int queue_connect(const char *name);
extern int queue_delete(int qid);
extern int queue_send(int qid, void *buf, int size, int nowait);
extern int queue_recv(int qid, void *msg, int size, int nowait);
extern int queue_readq_interruptable(int flag);
extern int queue_empty(int msg_id);
extern int queue_status_num_msg(int msg_id);

//make a key from a 'name'
extern int ipc_makekey(const char *name);
extern char* ipc_namefromkey(int key);


//file mapping
extern char *fmap_create(char *mappedfile, int nsize);
extern char *fmap_connect(char *mappedfile, int nsize);
extern int fmap_unmap(char *pAddr, int nsize);

//from util_command.c
typedef int cmdfunc_t(int, char **);
extern int cmd_init(const char *prompt, sigfunc_t *pFn);
extern int cmd_add(char *command, cmdfunc_t *pfnHandler, char *prompt);
extern int cmd_shell(int argc, char *argv[]);

//Dictionary
extern void dic_put(char *key, char *value);
extern char* dic_value_for_key(char *key);

//Socket
extern int read_msg_from_SWnet(int conn_sockfd, void *msg_buffer, int nbufsize,int ntimeout);
extern int read_msg_from_AMP(int conn_sockfd, void *msg_buffer, int nbufsize,int ntimeout);
extern int write_msg_to_SWnet(int conn_sockfd, void *msg_buffer, int nbytes,int ntimeout);
extern int write_msg_to_AMP(int conn_sockfd, void *msg_buffer, int nbytes,int ntimeout);

//Database
extern int DbConnect(char* cUserDb , int iDebug);

//Secu
extern int sjl05_terminal_mac( int iSockId, char *caInbuf, int iLen, char *iIndex, char *key, char *psam, char *caMac);

//public.c
extern int PsamPikGen(char *psam, char *workey);

//TCP/IP
extern int write_msg_to_NAC(int conn_sockfd, void *msg_buffer, int nbytes,int ntimeout);
extern int write_msg_to_net(int conn_sockfd, void *msg_buffer, int nbytes,int ntimeout);
extern int write_msg_to_AMP(int conn_sockfd, void *msg_buffer, int nbytes,int ntimeout);
extern int write_msg_to_CON(int conn_sockfd, void *msg_buffer, int nbytes,int ntimeout);

//libbsd
extern char *getprogname_ ();
extern void setprogname_ (char *new);

extern char * ise_strerror(int errno_);
extern void *Check_Queue(void * qid);


#endif
