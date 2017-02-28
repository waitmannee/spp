#ifndef msg_api_h
#define msg_api_h

#include "unixhdrs.h"

/*发送短信信
 operid：操作员ID；
 operpwd：操作员密码
 rectype：接收标志类型：B -- 表示短信接收标志为：手机号码，A -- 表示短信接收标志为：组别代码
 recid：短信接收标志：内容由接收标志类型控制。
 msgbuf：短信内容，长度不大于512个字节。
 */
extern int SendMsg_api(char *operid,char *operpwd,char *rectype,char *recid,char *msgbuf,char *routeid,
                       char *ridertype,char *riderinfo);
extern void WriteMsgLog(char *Msg);

#endif
