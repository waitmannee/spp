/*  
 *文件名称：http_server_httpd.h
 *功能描述：
 *支持http协议的服务器端短链服务需要的头文件
*/
#include "http_server.h"
#include "httpd.h"
/*
 * 存储客户端信息
 * 用于我方作为服务器端时使用
 */
typedef struct
{
	httpReq	*request;
	int location;//与TPDU中客户地址对应
} httpd_process;

#define HTTP_SERVER_HTTPD_SHM_NAME "HTTP_HTTPD_SERVER"

