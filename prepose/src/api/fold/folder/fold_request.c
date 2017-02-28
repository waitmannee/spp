//
//  fold_request.c
//  iPrepose
//
//  Created by Freax on 12-12-19.
//  Copyright (c) 2012年 Chinaebi. All rights reserved.
//

#include <stdio.h>
#include "folder.h"
#include "extern_function.h"


/*
 Symbols from fold_request.o:
 
 [Index]     Value      Size      Type  Bind  Other Shname         Name
 
 [32]      |0         |0         |NOTY |GLOB |0    |UNDEF         |errno
 [33]      |0         |0         |NOTY |GLOB |0    |UNDEF         |queue_recv
 
 [34]      |1208      |102       |FUNC |GLOB |0    |.text         |fold_wait_answer
 
 [35]      |0         |0         |NOTY |GLOB |0    |UNDEF         |queue_send
 [36]      |0         |0         |NOTY |GLOB |0    |UNDEF         |getpid
 [37]      |0         |0         |NOTY |GLOB |0    |UNDEF         |gl_pFldCtrl
 
 [38]      |1148      |59        |FUNC |GLOB |0    |.text         |fold_send_request
 */

//============================================================//
/*
 * fold_send_request,fold_send_msgpacket
 * 发送给folder系统数据。因为，IPC msg 之间的传输不支持自定义的struct,所以之间要想
 * 发消息，现在采取的策略是拼装成一个字符串，信息发送的Folder系统之后，Folder系统解
 * 析数据。
 *
 * 消息体:第一个字节是消息类别，分为两种:
 *        (1)文件本地操作，如: 创建、定位、删除等。 //FOLDER_REQ_FOLDER_QRY_TYPE
 *        (2)文件的IO操作，如: 读、写等。          //FOLDER_REQ_FOLDER_IO_TYPE
 *
 * Floder系统对应的进行解析。
 */
//============================================================//

void clear_queue_by_id(int msg_id){
    int timer = 0;
    while (queue_status_num_msg(msg_id) > 0){
        timer++;
        sleep(2);
        
        if (timer > 10) {
            queue_empty(msg_id);
        }
    }
}


//
// structure used to carry request/response between foldSvr and its clients
//
// Freax:当调用folder的create和locate和delete方法时消息体
//
//** Folder system create or locate request type **//
int fold_send_request(struct qrypacket* qry_packet){
    char qry_str[512];
    memset(qry_str, 0x00, sizeof(qry_str));
    qrypacket_to_str(qry_packet, qry_str);  //** 将 qry_packet 拼装成字符串发出去 **//
    int fid = open(PIPE_FOLD_SERVER_FOLDER_NAME, O_RDWR);
    if (fid > 0) {
        write(fid, qry_str, strlen(qry_str));
        close(fid);
    } else {
        //dcs_log(0, 0, "fold_send_request.open PIPE_FOLD_SERVER_FOLDER_NAME error!");
        printf("fold_send_request.open PIPE_FOLD_SERVER_FOLDER_NAME error!\n");
    }
    return fold_wait_answer();
}

//** Folder system IO request type **//
int fold_send_msgpacket(struct msgpacket *msg, void *buf, int nbytes)
{
    int msg_id = queue_connect(FOLDER_SYSTEM_IPC_MSG_NAME);
    int data_len = 0;
    int buf_len = 0;
    buf_len = nbytes;
    
    char msg_str[512];
    memset(msg_str, 0x00, sizeof(msg_str));
    msgpacket_to_str(msg, buf, msg_str, &buf_len, &data_len);
    
    MSGBUF msg_buf;
    memset(msg_buf.mtext, 0x00, sizeof(msg_buf.mtext));
    memcpy(msg_buf.mtext, msg_str, data_len);
    msg_buf.mtype = 1;        
    
    return queue_send(msg_id, &msg_buf, data_len + 1, 0);
}

/*
 * 等待查询结果
 */
int fold_wait_answer(){

    char rd_buf[512];
    memset(rd_buf, 0x00, sizeof(rd_buf));
    
    int fid = open(PIPE_FOLD_RESPONSE_FOLDER_NAME, O_RDWR);
    
    read(fid, rd_buf, sizeof(rd_buf));
    //dcs_log(0, 0, "fold create folder read from folder system => %s", rd_buf);
    //printf("fold create folder read from folder system => %s\n", rd_buf);
    struct qrypacket qry_pack;
    str_to_qrypacket(rd_buf, &qry_pack);
    
    close(fid);
    
    return qry_pack.fold_result;
}

void qrypacket_to_str(struct qrypacket* qry_packet, char *ret_buf){
    char qry_buf[1024];
    memset(qry_buf, 0x00, sizeof(qry_buf));
    sprintf(qry_buf, "%d|%ld|%d|%d|%ld|%d|%d|%s",
            FOLDER_REQ_FOLDER_QRY_TYPE,
            qry_packet->fold_msgtype,
            qry_packet->fold_pid,
            qry_packet->fold_command,
            qry_packet->fold_flags,
            qry_packet->fold_result,
            qry_packet->fold_errno,
            qry_packet->fold_name);
    sprintf(ret_buf, "%s", qry_buf);
}


void str_to_qrypacket( char *qry_buf, struct qrypacket* qry_packet_respon){
    int qry_type;
    memset(qry_packet_respon->fold_name, 0x00, sizeof(qry_packet_respon->fold_name));
    
    sscanf(qry_buf, "%d|%ld|%d|%hd|%ld|%d|%d|%s",
           &qry_type,
           &qry_packet_respon->fold_msgtype,
           &qry_packet_respon->fold_pid,
           &qry_packet_respon->fold_command,
           &qry_packet_respon->fold_flags,
           &qry_packet_respon->fold_result,
           &qry_packet_respon->fold_errno,
           qry_packet_respon->fold_name);
}

/*
 //
 // structure of message packet,this packet holds reference to shared
 // memory where the very data reside
 //
 */
void msgpacket_to_str(struct msgpacket* msg_packet, char *user_buf, char *ret_buf, int *buf_len, int *data_len){
    char len_buf[100];
    memset(len_buf,0x00,sizeof(len_buf));
    sprintf(len_buf, "%d|%ld|%d|%d|%d|%d|%ld|%ld|%d|",
            FOLDER_REQ_FOLDER_IO_TYPE,
            msg_packet->msg_address,
            msg_packet->msg_dest,
            msg_packet->msg_org,
            msg_packet->msg_clusterid,
            msg_packet->msg_command,
            msg_packet->msg_flags,
            msg_packet->msg_tickcount,
            *buf_len);
    
    *data_len = (int)strlen(len_buf) + *buf_len;
    memcpy(ret_buf,len_buf,(int)strlen(len_buf));
/*
    sprintf(ret_buf, "%d|%ld|%d|%d|%d|%d|%ld|%ld|%d|",
            FOLDER_REQ_FOLDER_IO_TYPE,
            msg_packet->msg_address,
            msg_packet->msg_dest,
            msg_packet->msg_org,
            msg_packet->msg_clusterid,
            msg_packet->msg_command,
            msg_packet->msg_flags,
            msg_packet->msg_tickcount,
            *buf_len);
    */
    memcpy(ret_buf + strlen(len_buf), user_buf, *buf_len);
}

void str_to_msgpacket(char *msg_buf, char *user_buf, struct msgpacket* msg_pkt, int *buf_len, int *data_len,int nsize){
    int qry_type;
    
    sscanf(msg_buf, "%d|%ld|%d|%d|%d|%d|%ld|%ld|%d|",
           &qry_type,
           &msg_pkt->msg_address,
           &msg_pkt->msg_dest,
           &msg_pkt->msg_org,
           &msg_pkt->msg_clusterid,
           &msg_pkt->msg_command,
           &msg_pkt->msg_flags,
           &msg_pkt->msg_tickcount,
           buf_len
           );
    
    char len_buf[100];
    memset(len_buf,0x00,sizeof(len_buf));
    sprintf(len_buf, "%d|%ld|%d|%d|%d|%d|%ld|%ld|%d|",
            qry_type,
            msg_pkt->msg_address,
            msg_pkt->msg_dest,
            msg_pkt->msg_org,
            msg_pkt->msg_clusterid,
            msg_pkt->msg_command,
            msg_pkt->msg_flags,
            msg_pkt->msg_tickcount,
            *buf_len
            );
	
    *data_len = *buf_len + (int)strlen(len_buf);
    
    memcpy(user_buf, msg_buf + strlen(len_buf), *buf_len);
    
}

