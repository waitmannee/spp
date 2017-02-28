
#ifndef ftp_cu_H
#define ftp_cu_H

#include "unixhdrs.h"

#define DEFAULT_FTPPORT           21
#define Ftp_File_Mask             1


#define Ftp_Mode_Ascii            1
#define Ftp_Mode_Binary           2

#define FTP_TIMEOUT_SECEND  			60

extern int Connect(char* host,char* username,char* password,int port);

extern int Disconnect();

extern int Get_Server_Ver();

extern int ChangeDir(char* newdir);
extern char* GetCurrentDir();

extern int UploadFile(char* localfile,char* remotefile);

extern void PrintMsg();

extern int SetMode(int transmode);/*1-- ASCII，2 -- BINARY*/

extern int Mkdir(char * pathname);

extern int Rmdir(char * pathname);

extern int DownloadFile(char* remotefile,char* localfile);

extern int PasvMode();

extern int UploadFile_pasv(char* localfile,char* remotefile);

/*
 函数功能：删除文件
 传入参数：
 pathname:  删除文件
 返回参数：
 1    成功
 0    失败
 */

extern int RmFile(char * filename);

extern int ChangeDir(char* newdir);


/*得到当前目录下的文件列表*/
extern int FileList(char* curpwd, char * logfile);

#endif


