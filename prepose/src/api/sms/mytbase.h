#ifndef mytbase_h
#define mytbase_h

#include "mypublic.h"
#include "msg_api.h"

#define COMTRADEBLOCK 2048
#define TRADNOENDESTAU	'1','3','5'

/**************************基本操作函数****************************/

extern int ConnectDb(char *username,char *passwd,char *dbname);
extern void DisconnectDb();
/*取得参数*/
extern int GetPara(char *paraid,char *value);

/*修改系统参数*/
extern int UpdatePara(char *paraid,char *value);

/*发短信*/
extern int SndMobileMsg(char * begdatetime);


#endif
