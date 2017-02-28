#include "ibdcs.h"
#include "trans.h"
#include "memlib.h"

//
//  memcProc.c
//  xpep2.2
//
//  Created by he.qingqi on 16/8/17.
//
//

//数据格式：add|key|value
//        get|key
void memcProc(char *srcBuf, int iFid, int iMid, int iLen)
{
    
    dcs_debug(srcBuf, iLen, "memc Proc, iFid=%d, iMid=%d, iLen=%d", iFid, iMid, iLen);
    
    int datalen=0,result = 0;
    char databuf[1024];
    
    char* token;
    char* key;
    char* value;
    
    char memdata[512];
    int memdataLength;
    
    char tpdu[5+1];

    //初始化变量
    memset(databuf, 0, sizeof(databuf));
    memset(memdata, 0, sizeof(memdata));
    memset(tpdu, 0, sizeof(tpdu));
    
    
    memcpy(tpdu, srcBuf, 5);
    
    dcs_debug(tpdu, 5, "tpdu:");

    datalen = iLen-5;
    memcpy(databuf, srcBuf+5, datalen);
    
    token = strtok( databuf, "|");
    if (token == NULL) {
        return;
    } else {
        if(memcmp(token, "add", 3)==0){
            dcs_log(0, 0, " 识别请求 %s", token);
            key = strtok( NULL, "|");
            if (key == NULL)
                return;
            value = strtok( NULL, "|");
            if (value == NULL)
                return;
            
            result = Mem_SavaKey(key, value, getstrlen(value), 12345678);
            if(result < 0){
                dcs_log(0, 0, " 保存失败，key（%s）value（%s）", key, value);
                WriteBack(tpdu, iFid, "保存失败");
                return;
            }else{
                dcs_log(0, 0, " 保存成功，key（%s）value（%s）", key, value);
                WriteBack(tpdu, iFid, "保存成功");
                return;
            }
            
        }else if(memcmp(token, "get", 3)==0){
            dcs_log(0, 0, " 识别请求 %s", token);
            key = strtok( NULL, "|");
            if (key == NULL)
                return;

            result = Mem_GetKey(key, memdata, &memdataLength, 512);
            if(result < 0){
                dcs_log(0, 0, " 获取失败，key（%s）", key);
                WriteBack(tpdu, iFid, "获取失败");
                return;
            }else{
                dcs_log(0, 0, " 获取成功，key（%s）value（%s）length(%d)", key, memdata, memdataLength);
                WriteBack(tpdu, iFid, memdata);
                return;
            }

        }else{
            dcs_log(0, 0, " 无法识别请求 %s", token);
        }
    }
    
}

int WriteBack(char *tpdu, int trnsndpid, char *buffer)
{
    int gs_comfid = -1, s=0;
    char buffer_send[2048];
    
    memset(buffer_send, 0, sizeof(buffer_send));
    
    s= strlen(buffer);
    gs_comfid = trnsndpid;
    

    if(fold_get_maxmsg(gs_comfid) < 2)
        fold_set_maxmsg(gs_comfid, 20) ;
    
    memcpy(buffer_send, tpdu, 1);
    memcpy(buffer_send+1, tpdu+3, 2);
    memcpy(buffer_send+3, tpdu+1, 2);
    
    memcpy(buffer_send+5, buffer, s);
    dcs_debug(buffer_send, s+5, "data_buf len=%d, foldId=%d", s+5, gs_comfid);
    fold_write(gs_comfid, -1, buffer_send, s+5);
    
    return 0;
}
