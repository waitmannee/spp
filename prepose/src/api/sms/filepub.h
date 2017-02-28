#ifndef filepub_h
#define filepub_h

#include "unixhdrs.h"

#define MAXITEMID 30
#define MAXITEMVAL 256

struct FileIni
{
    char itemid[MAXITEMID + 1];
    char itemval[MAXITEMVAL + 1];
    struct FileIni * next;
};

struct IniOper
{
	char termflag[MAXITEMID + 1];
	struct IniOper *next;
	struct FileIni *fileini;
};

/*
 参数：
 filename：配置文件名
 返回值：
 1--正确
 -1 -- 失败
 */
extern int LoadFile(char *filename); /*装载ini文件*/
void Freeini();/*释放ini文件所占用的内存*/

/*
 参数：
 keyid：大项名称
 itemid:子项目名称
 返回参数：
 itemval: 子项目值
 返回值：
 1--正确
 -1 -- 失败
 */
extern int GetItemData(char *keyid,char *itemid,char *itemval);/*取键值*/

extern void GoIniHead();/*到文件头*/

/*
 返回值：
 1 -- 成功
 -1 --失败
 */
extern int GONextTerm();/*得到下一个ini文件配置大项*/
/*
 参数：
 itemval：返回参数--子项的值
 itemid：子项标志
 返回值：
 1 -- 成功
 -1 --失败
 */
extern int GetIniCurVal( char *itmeval, char *itemid);/*得到当前项目组的子项目值*/

extern void GetIniErrMsg(char *msg);/*取得错误原因*/


#endif


