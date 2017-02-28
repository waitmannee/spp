#ifndef __SECUSRV_H__
#define __SECUSRV_H__

struct secLinkInfo
{
    long   lFlags;      //该link的标志字
    /*    short  iRole;       该link在连接建立过程中的角色 */
    short  iStatus;     //该link当前 的状态
    short  iWorkFlag;
    /*    char   caLocalIp[20]; 本地的IP地址 */
    char   caRemoteIp[20];//期望的远端的IP地址
    char   cEncType[3];//加密机类型
    int    iLocalPort;   // 本地的端口号
    int    iRemotePort;   //期望的远端的端口号
    int    iSocketId;
    int    iTimeOut;    //超时时间
    int    iNum;
};

struct SECSRVCFG
{
    int secSemId;
    int srvStatus;
    int maxLink;
    int pNextFreeSlot;
    struct secLinkInfo secLink[1];
};
char *g_pSecSrvShmPtr;   //安全服务器共享内存首地址

struct MSG_STRU
{
    long iPid;
    char caSocketId[5];
    //	  int iNum;
    char caMsg[1];
};
#endif
