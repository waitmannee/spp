#ifndef __UNIHDRS__H__
#define __UNIHDRS__H__

#include    <stdio.h>
#include    <stdlib.h>
#include    <stdarg.h>
#include    <string.h>
#include    <time.h>        /* timespec{} for pselect() */
#include    <errno.h>
#include    <fcntl.h>       /* for nonblocking */
#include    <netdb.h>
#include    <signal.h>
#include    <unistd.h>
#include    <ctype.h>   //Freax
#include 	  <utmpx.h>
#include    <langinfo.h>
#include    <locale.h>
#include    <dirent.h>
#include    <math.h>
#include    <pwd.h>
#include 	<pthread.h>

#include    <sys/timeb.h>
#include    <sys/un.h>      /* for Unix domain sockets */
#include    <sys/stat.h>
#include    <sys/mman.h>
#include    <sys/wait.h>
#include    <sys/types.h>   /* basic system data types */
#include    <sys/socket.h>  /* basic socket definitions */
#include    <sys/select.h>
#include    <sys/syscall.h>

#include    <sys/time.h>    /* timeval{} for select() */
#include    <sys/uio.h>     /* for iovec{} and readv/writev */
#include    <sys/ioctl.h>
#include    <sys/file.h>
#include    <netinet/in.h>
#include    <netinet/tcp.h>
#include <net/if.h>
#include    <arpa/inet.h>   /* inet(3) functions */
#include    <sys/ipc.h>
#include    <sys/sem.h>
#include    <sys/shm.h>
#include    <sys/msg.h>
#include    <uuid/uuid.h>
#include  <iconv.h>

#include <linux/limits.h>
#include <regex.h> /*regcomp()、regexec()、regfree()、regerror()*/

#ifndef TRUE
    #define TRUE (1==1)
    #define FALSE (1==0)
#endif

#ifndef max
    #define  max(a,b)   (((a) > (b)) ? (a) : (b))
    #define  min(a,b)   (((a) < (b)) ? (a) : (b))
#endif

typedef void sigfunc_t(int);

//////////// Freax Mac_OS //////////////
typedef unsigned long ulong;

#define ise_strerror(x) strerror(x)

//,user,NULL,"PEP"

#define DB_MYSQL_HOST "localhost"
#define DB_MYSQL_USER "root"
#define DB_MYSQL_PAS "000000"
#define DB_MYSQL_DBNAME "XPEP"

/////////////////////////////////////////

/*
 * 存储客户端信息
 * 用于我方作为服务器端时使用
 */
typedef struct
{
	int sock;
	int location;//与TPDU中客户地址对应
} process;

#define MAX_PORCESS 1024
#define MAX_TPDU 65535
#define NO_SOCK -1
//包的长度最大值
#define PACKAGE_LENGTH  2048

#endif //__UNIHDRS__H__
