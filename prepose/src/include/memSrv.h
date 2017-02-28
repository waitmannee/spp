#ifndef __MEMSRV_H__
#define __MEMSRV_H__

struct memServerInfo
{
    char caRemoteIp[20];   /*memcached服务器IP */
    int iRemotePort;      /*memcached服务器PORT */
};

struct MEMSRVCFG
{
    int maxLink;    /*配置的最大服务器连接数*/
    struct memServerInfo memServerInfo[1];
};

#endif
