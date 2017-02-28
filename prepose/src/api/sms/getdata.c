#include "unixhdrs.h"
//
//#include "getdata.h"
//
//void FetchTradeLst(char *username,char *passwd,char *dbname)
//{
//	char tradeno[40];
//	char s_value[20];
//	char v_source[4];
//	
//	int d_time;
//	int deal_time;
//	char msg[1024];
//	
//	strcpy(tradeno,"A");
//	int ret_val=0;
//	
//	deal_time=0;
//	if (ConnectDb(username,passwd,dbname)!=1)
//	{
//        printf("connect %s by %s fail!\n",username,passwd);
//        return ;
//	}
//    
//	/*取数据处理间隔时间*/
//	if (!GetPara("ctrl_sleep_time",s_value))
//	{
//        strcpy(s_value,"120");
//	}
//	sscanf(s_value,"%d",&d_time);
//	
//	DisconnectDb();
//	for(;;)
//	{
//		if (ConnectDb(username,passwd,dbname)!=1)
//		{
//			printf("connect %s by %s fail!\n",username,passwd);
//			sleep(60);
//			continue ;
//		}
//		/*取数据处理标志*/
//		if (!GetPara("payeasydata_deal_ctl",s_value))
//		{
//	 	 	strcpy(s_value,"1");
//		}
//		if (s_value[0]=='0')/*暂停处理*/
//		{
//			printf("flag:0 sleep %d  times!\n",d_time);
//			sleep(d_time);
//			DisconnectDb();
//			continue ;
//		}
//		if (s_value[0]=='4')/*退出处理*/
//			break;
//		
//		ret_val=SndMobileMsg(tradeno);
//		if (ret_val==0)
//		{
//            
//			strcpy(tradeno,"A");
//			sprintf(msg,"flag:SndMobileMsg  err, sleep %d  times!\n",d_time);
//			writelog(msg,3);
//			sleep(d_time);
//			DisconnectDb();
//			continue ;
//		}
//        
//		printf("sleep %d  times!\n",d_time);
//		sleep(d_time);
//		DisconnectDb();
//	}
//	DisconnectDb();
//	printf("exit.....\n");
//}
//
