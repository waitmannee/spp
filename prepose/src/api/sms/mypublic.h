#ifndef mypublic_h
#define mypublic_h


/****************************以下是字符串操作函数***********************************/
extern int GetKeyDatabyFile(char *keyid,char *keycode,char *data,char *filename);
extern char *GetFeilderStr(char *tagstr,char *recstr,int startpos,int endpos);
extern char *GetsubString(char *newstr,char *srcstr,int size);
extern char *getname(char *fname,char *recfname);
/*把字符串左右的空格去掉*/
extern char *Allt(char *str);

/*把系统时间转换为14位的字符串*/
extern char *gettime(char *strdate);
/*计算日期的加减*/
extern char *AddDate(char *srcdate,int num);

/*对24小时制的14位的时间进行秒级别的相加减,秒数必须小于一个小时*/
extern char *AddTime(char *strdate,int sd_val);

/*从目标字符串的指定位置加载指定长度的子字符串*/
extern char *SetStrstr(char *srcstr,char *dstr,int start,int size);

/*取小数点后指定位数，舍去位按：四舍五入、丢弃、进一*/
/*
 参数bytes：保留小数点后的位数
 参数mode：1代表四舍五入，2代表舍去，3代表进一
 */
extern double Round(double x,int bytes,int mode);

/*检查字符串各位是否是数字*/
extern int IsNumber(char *str);

/*  换算str字符串为大写   */
extern char *Upper(char *str);

/*查找字串*/
extern int SubStrIsExist(char *srcstr,char *tagsubstr,int pos);/*pos：0 -- 从第一位开始比较，1 -- 从末尾开始比较*/

/*密码加密*/
extern int Encrypt(char *paswd);
/*密码解密*/
extern int Unencrypt(char *paswd);
/*写密码文件*/
int WriteIni(char * filename, char *sname, char *pwd);


/*********************************操作日志**************************************/
/*
 参数msg：要写的内容
 参数mode：1 代表输出到屏幕
 2 代表写到系统指定的文件中
 3 代表既输出到屏幕，又写到系统指定文件中
 */
extern int writelog(char *msg,int mode);

#endif
