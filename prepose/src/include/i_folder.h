#ifndef  __FOLDER_FREAX__H__
#define  __FOLDER_FREAX__H__


//=============================================================///
//========================= Freax Add =========================///
//=============================================================///

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
#if 0
//=============== IPC Message ==================//
#define MSGSIZE	1024		/* a resonable size for a message */
#define MSGTYPE 1		/* a type ID for a message */

typedef struct mbuf {		/* a generic message structure */
	long mtype;
	char mtext[MSGSIZE + 1];  /* add 1 here so the message can be 1024   */
} MSGBUF;			  /* characters long with a '\0' termination */
#endif
//MSGBUF msg_buf, rd_buf;

//=============================================================///
//=============================================================///
#endif  //__FOLDER__H__
