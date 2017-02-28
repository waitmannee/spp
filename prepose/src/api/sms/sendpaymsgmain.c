//#include <sys/types.h>
//#include <sys/wait.h>
//#include <stdio.h>
//#include <sys/stat.h>
//#include <time.h>
//
//#include "getdata.h"
//#include "mypublic.h"
//
//int main(int argc,char *argv[])
//{
//	
//	char username[256];
//	char passwd[128];
//	char dbname[256];
//
//	strcpy(username,"settlement");
//	strcpy(passwd,"abc123");
//	strcpy(dbname,"");
//	/*
//	if (!GetKeyDatabyFile("GETPAYDATA","username",username,"./stlm.ini"))
//	{
//		printf("get username fail!!\n");
//		return 0;
//	}
//	if (!GetKeyDatabyFile("GETPAYDATA","dbname",dbname,"./stlm.ini"))
//	{
//		strcpy(dbname,"");
//	}
//	
//	if (!GetKeyDatabyFile("GETPAYDATA","passwd",passwd,"./stlm.ini"))
//	{
//		printf("get passwd fail!!\n");
//		return 0;
//	}
//	else
//		Unencrypt(passwd);
//*/
//	switch (fork())
//  {
//		case -1:
//			printf("unable to fork daemon!\n");
//			DisconnectDb();
//			exit(0);
//		case 0:
//      printf("start fetch behalf.............\n");
//      FetchTradeLst(username,passwd,dbname);
//      break;
//  	default:
//  	  printf("mian end!\n");
//      exit(0);
//      
//  }
//	exit(0);
//}
