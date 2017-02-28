#ifndef  __FOLDER__H__
#define  __FOLDER__H__

#include "mbuf_func.h"

//
// name of shared memory
//
#define   BUFFER_POOL_SHM_NAME      "BfPl"
#define   FOLDER_LIST_SHM_NAME      "FdLt"

//
// control block structure for Folder list which resides
// in shared memory. there is just ONE structure in whole
// Folder system
//
typedef struct _tagFoldCtrlBlk
{
    int    shm_id;      //id of shared memory to hold Folder list
    long   shm_magic;   //a constant, is always FOLD_MAGIC
    _sem_t  sem_id;     //id of semaphore associated with Folder list as mutex
    short  sem_count;   //semaphore number in set 'sem_id'
    int    foldsvr_pid; //pid of foldSvr
    int    msg_reqqid;  //message queue for foldSvr and its requestor
    int    msg_viaqid;  //message queue to send/receive msgpacket{}
    int    fold_nalloc; //folder# allocated in folder list
    int    fold_nfree;  //free folder number

    ulong   fold_systickcount; //tick counter to provide clock for foldSys
} FOLDCTRLBLK;

#define  FOLD_MAGIC   0xdeadbeef  //magic number associated with folder SM

//
// structure of each Folder
//

#define  MAX_FOLDNAME_SIZE   32   //max characters in a folder name

typedef struct _tagFolderEntry
{
    char  fold_name[MAX_FOLDNAME_SIZE+1];//name of folder
    int   fold_pid;      //pid of owner of this folder
    int   fold_address;  //address of folder
    long  fold_flags;    //flags on this folder entry, such as FOLD_FLAG_USED
    short fold_type;     //type of this folder
    int   fold_nref;     //reference count
    int   fold_nmsg;     //message# contained in this folder currently
    int   fold_nmaxmsg;  //max message# allowed in this folder
} FOLDERENTRY;

//constants for fold_flags:
#define  FOLD_FLAG_USED    0x00000001  //this entry is used
#define  FOLD_FLAG_NEEDSIG 0x00000002  //after writing to this folder,
                                       //sender should notify
                                       //the receiver with Unix signal SIGUSR1
//constants for fold_type:
#define  FOLD_TYPE_NORMAL     1        //normal folder
#define  FOLD_TYPE_ANONYMOUS  2        //anonymous folder   

//
// structure used to carry request/response between foldSvr
// and its clients
// Freax:当调用folder的create和locate和delete方法时消息体
struct qrypacket
{
    long  fold_msgtype;     //message type of request/response packet
    int   fold_pid;         //pid of requestor                        
    short fold_command;     //command sent to foldSvr to committ
    long  fold_flags;       //flags on this request packet
    int   fold_result;      //result from foldSvr
    int   fold_errno;       //error code if processing failed
    
    char  fold_name[MAX_FOLDNAME_SIZE+1];//name of folder, or Id of folder
};

// constants for fold_command
#define  QRYPKT_CMD_CREATE     1  //ask foldSvr to create a brand new folder
#define  QRYPKT_CMD_LOCATE     2  //ask foldSvr to locate a folder by its name
#define  QRYPKT_CMD_DELETE     3  //ask foldSvr to delete a folder by its Id
#define  QRYPKT_CMD_EXIT       4  //ask foldSvr to exit()

// constants for fold_flags:
#define  QRYPKT_FLG_NORMAL       0x00000001  //normal folder
#define  QRYPKT_FLG_ANONYMOUS    0x00000002  //anonymous folder
#define  QRYPKT_FLG_DELBYREF     0x00000004  //decrease ref. count of a folder
#define  QRYPKT_FLG_DELBYFORCE   0x00000008  //delete a folder by violence

//constants for fold_errno;they are error code for whole Folder system too
#define FOLD_ERROR     1000
#define FOLD_EEXIST     -(FOLD_ERROR+1)   //the specified folder exists already
#define FOLD_ENOFOLD    -(FOLD_ERROR+2)   //no free entry for creating new folder
#define FOLD_ENOENT     -(FOLD_ERROR+3)   //the specified folder not existing
#define FOLD_ENOSYS     -(FOLD_ERROR+4)   //calling process not attached to Folder system yet
#define FOLD_ENOMBUF    -(FOLD_ERROR+5)   //not enough buffers in shared memory
#define FOLD_EINVAL     -(FOLD_ERROR+6)   //invalid argument
#define FOLD_EMSGOVFLW  -(FOLD_ERROR+7)   //message in folder overflow

//
// structure of message packet,this packet holds reference to shared 
// memory where the very data reside
// Freax:当调用folder的write、read方法时的消息体
struct msgpacket
{
    long  msg_address;      //address of sender/receiver
    int   msg_dest;         //Id of receiver
    int   msg_org;          //Id of sender
    int   msg_clusterid;    //Id of cluster, this cluster contains the very data 
                            //to communicate
    int   msg_command;      //cmd to distinguish packet with different purpose
    long  msg_flags;        //flags on this packet
    ulong msg_tickcount;    //folder system clock when msg put into system 
};

//constants for msg_command:
#define  FOLD_MSG_DATA       1000  //ordinary data
#define  FOLD_MSG_PROBE_REQ  1002  //probe request
#define  FOLD_MSG_PROBE_ACK  1003  //probe ack
#define  FOLD_MSG_EXIT       1004  //termination notification

//
// all globals declaration
//
//
extern SHMOBJ       gl_shmobj;
extern FOLDCTRLBLK *gl_pFldCtrl;
extern FOLDERENTRY *gl_pFldArray;
extern struct msgpacket gl_msgpacket;

//extern char *strerror(int errno);

//=============================================================///
//========================= Freax Add =========================///
//=============================================================///

int  fold_get_id(char *dir, int fold_id);
int  fold_get_id_by_name(char *dir, const char *filename);
int  fold_get_path_by_name(const char *dir, const char *filename);
//extern char *ise_strerror(int errno);

#define FREAX_FOLDER_TEST "/Users/Freax/Desktop/prepose"

#define FOLDER_NAME_MAX_LENGTH 256

//** IPC msg id
#define FOLDER_SYSTEM_IPC_MSG_NAME "fold_system_ipc_msg"
#define FOLDER_SYSTEM_WITH_PROCESS_IPC_MSG_NAME "fold_system_ipc_with_process_msg"
#define FOLDER_SYSTEM_QRY_IPC_MSG_NAME "fold_system_qry_msg"
#define FOLDER_SYSTEM_BUFFER_IPC_MSG_NAME "fold_system_buffer_ipc_msg"
#define FOLDER_SYSTEM_BUFFER_IPCS_MSG_NAME "fold_system_buffer_ipcs_msg"            //I/O buffer

#define FOLDER_SYSTEM_IPC_SEM_NAME "fold_system_ipc_sem"
#define PIPE_FOLD_SERVER_FOLDER_NAME "/tmp/foldSvr"
#define PIPE_FOLD_RESPONSE_FOLDER_NAME "/tmp/fold_response_pipe_file"

//
#define FOLDER_REQ_FOLDER_QRY_TYPE 1
#define FOLDER_REQ_FOLDER_IO_TYPE 2

#define FOLDER_QRY_MSG_TYPE_REQUEST 1
#define FOLDER_QRY_MSG_TYPE_RESPONSE 2

//=============== IPC Message ==================//
#define MSGSIZE	PACKAGE_LENGTH		/* a resonable size for a message */
#define MSGTYPE 1		/* a type ID for a message */

typedef struct mbuf {		/* a generic message structure */
	long mtype;
	char mtext[MSGSIZE + 1];  /* add 1 here so the message can be 1024   */
} MSGBUF;			  /* characters long with a '\0' termination */


/*
* 定义接收队列结构体
*/
typedef struct Recv_Mess_struct {
	long mestype;
	int  len;
	char mtext[MSGSIZE + 1];  
} RECV_MESS_T;	

//MSGBUF msg_buf, rd_buf;

//=============================================================///
//=============================================================///
#endif  //__FOLDER__H__
