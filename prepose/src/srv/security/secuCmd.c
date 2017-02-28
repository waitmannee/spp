#define _MainProg

#include  "dccglobl.h"
#include  "dccpgdef.h"
#include  "dccdcs.h"
#include  "secuSrv.h"
#include  "dccsock.h"

static struct SECSRVCFG *g_pHstCtrl = NULL;
static int g_iHstSrvFoldId = -1;
static   void   DcmCmdExit(int signo);

//forward declaration
int ListHst  (int arg_c, char *arg_v[]);
int secu_StopLink  (int arg_c, char *arg_v[]);
int StartLink (int arg_c, char *arg_v[]);
int secu_AddLink   (int arg_c, char *arg_v[]);
int secu_ModifyLink   (int arg_c, char *arg_v[]);
int secu_DeleteLink   (int arg_c, char *arg_v[]);


int main(int argc, char *argv[])
{
	catch_all_signals(DcmCmdExit);

	//attach to folder system
	if (fold_initsys() < 0)
	{
		fprintf(stderr,"cannot attach to FOLDER system!\n");
		exit(1);
	}

	cmd_init("seccmd>>",DcmCmdExit);
	cmd_add("list",     ListHst,   "list [linkName]--list status of link");
	cmd_add("stop",     secu_StopLink,  "stop linkName [no]--stop a link");
	cmd_add("start",    StartLink, "star linkName [no]--start a link");
	cmd_add("add",      secu_AddLink, "Add linkName [no]--add a link");
	cmd_add("update",   secu_ModifyLink, "update linkName no --modify a link");
	cmd_add("delete",   secu_DeleteLink, "delete linkName no--modify a link");
	// cmd_add("help",  KillHst, "killhst--kill HstSvr");

	cmd_shell(argc,argv);

	DcmCmdExit(0);
   	 return 0;
}//main()

static void DcmCmdExit(int signo)
{  	
  	exit(signo);
}

int ListHst(int arg_c, char *arg_v[])
{
	int  i, find = -1;
	char *strDesc;
	
	//connect the shared memory HSTCOM,so we can dump its content
	g_pHstCtrl = (struct SECSRVCFG *)shm_connect("SECSRV2", NULL);

	if (g_pHstCtrl == NULL)
	{
		fprintf(stderr,"HstSvr not active!\n");
		return 0;
	}

	//dump status of the sepecifed link(s)
	find = -1;

	for (i=0; i <= g_pHstCtrl->maxLink-1; i++)
	{
		if (!(g_pHstCtrl->secLink[i].lFlags & DCS_FLAG_USED))
			continue;

		if (find <0)
		{
			fprintf(stderr,"序号 类型   链路状态  工作状态 加密机IP：PORT         超时时间\n");
			fprintf(stderr,"===============================================================\n");
		}

		find = i;
		/*
		序号   类型   链路状态  工作状态  加密机IP：PORT         超时时间
		===============================================================
		[001]  01    conncted    0     192.168.20.162:6666    1000
		[002]  01    conncted    0     192.168.20.162:6666    1000
		[003]  01    conncted    0     192.168.20.162:6666    1000
		[004]  01    conncted    0     192.168.20.162:6666    1000
		*/
		switch (g_pHstCtrl->secLink[i].iStatus) 
		{
			case DCS_STAT_DISCONNECTED:
				strDesc="discon ";
				break;

			case DCS_STAT_CONNECTED:
				strDesc="conncted";
				break;

			case DCS_STAT_LISTENING:
				strDesc="listen ";
				break;

			case DCS_STAT_CALLING:
				strDesc="call   ";
				break;

			case DCS_STAT_STOPPED:
				strDesc="stopped ";
				break;
			default:
				strDesc="       ";
				break;
		}//switch

		if (g_pHstCtrl->secLink[i].iStatus == DCS_STAT_CONNECTED)
			fprintf(stderr,"[%.2d] %3.3s    %8.8s    %d     %s:%d    %d\n",
			g_pHstCtrl->secLink[i].iNum,
			g_pHstCtrl->secLink[i].cEncType,
			strDesc,
			g_pHstCtrl->secLink[i].iWorkFlag,
			g_pHstCtrl->secLink[i].caRemoteIp,
			g_pHstCtrl->secLink[i].iRemotePort,
			g_pHstCtrl->secLink[i].iTimeOut);
		else
			fprintf(stderr,"[%.2d] %3.3s    %8.8s    %d                      %d\n",
			g_pHstCtrl->secLink[i].iNum,
			g_pHstCtrl->secLink[i].cEncType,
			strDesc,
			g_pHstCtrl->secLink[i].iWorkFlag,
			g_pHstCtrl->secLink[i].iTimeOut);


	}
	shm_detach((char *)g_pHstCtrl);			// Mary add, 2001-7-13
	if (find < 0)
	{
		fprintf(stderr,"Link '%s' not exists!\n",arg_v[1]);
		return 0;
	}
	return 0;
}

int secu_StopLink  (int arg_c, char *arg_v[])
{
	char Cmd[20];
	char para[20];

	if (g_iHstSrvFoldId < 0)
	{
		g_iHstSrvFoldId = fold_locate_folder("SECSRV2");
		if (g_iHstSrvFoldId < 0)
		{
			fprintf(stderr,"SecSrv2 not active!\n");
			return 0;
		}
	}
	
	scanf("%s",para);
	sprintf(Cmd,"STOP%2.2s",para);
	if (fold_write(g_iHstSrvFoldId,-1,&Cmd,6) < 6)
		fprintf(stderr,"request to SecSrv2 failed,errno=%d\n",errno);
	return 0;
}

int StartLink (int arg_c, char *arg_v[])
{
	char Cmd[20];
	char para[20];
	memset(&Cmd,0,sizeof(Cmd));
	if (g_iHstSrvFoldId < 0)
	{
		//loate the folder of HstSrv
		g_iHstSrvFoldId = fold_locate_folder("SECSRV2");
		if (g_iHstSrvFoldId < 0)
		{
			fprintf(stderr,"SecSrv2 not active!\n");
			return 0;
		}
	}
	scanf("%s",para);
	sprintf(Cmd,"STAR%2.2s",para);
	if (fold_write(g_iHstSrvFoldId,-1,&Cmd,6) < 6)
		fprintf(stderr,"request to SecSrv2 failed,errno=%d\n",errno);

	return 0;
}

int secu_ModifyLink (int arg_c, char *arg_v[])
{
	char Cmd[20];
	char para[20];

	/* Mary add end, 2001-6-19 */
	memset(&Cmd,0,sizeof(Cmd));
	if (g_iHstSrvFoldId < 0)
	{
		g_iHstSrvFoldId = fold_locate_folder("SECSRV2");
		if (g_iHstSrvFoldId < 0)
		{
			fprintf(stderr,"SecSrv2 not active!\n");
			return 0;
		}
	}
	scanf("%s",para);
	sprintf(Cmd,"MODI%2.2s",para);

	if (fold_write(g_iHstSrvFoldId,-1,&Cmd,6) < 6)
		fprintf(stderr,"request to SecSrv2 failed,errno=%d\n",errno);
	return 0;
}

int secu_DeleteLink (int arg_c, char *arg_v[])
{
	char Cmd[20];
	char para[20];

	/* Mary add end, 2001-6-19 */
	memset(&Cmd,0,sizeof(Cmd));
	if (g_iHstSrvFoldId < 0)
	{
		//loate the folder of HstSrv
		g_iHstSrvFoldId = fold_locate_folder("SECSRV2");
		if (g_iHstSrvFoldId < 0)
		{
			fprintf(stderr,"SecSrv not active!\n");
			return 0;
		}
	}
	scanf("%s",para);
	sprintf(Cmd,"DELE%2.2s",para);
	if (fold_write(g_iHstSrvFoldId,-1,&Cmd,6) < 6)
		fprintf(stderr,"request to SecSrv2 failed,errno=%d\n",errno);

	return 0;
}

int secu_AddLink (int arg_c, char *arg_v[])
{
	char Cmd[20];
	char para[20];

	/* Mary add end, 2001-6-19 */
	memset(&Cmd,0,sizeof(Cmd));
	if (g_iHstSrvFoldId < 0)
	{
		//loate the folder of HstSrv
		g_iHstSrvFoldId = fold_locate_folder("SECSRV2");
		if (g_iHstSrvFoldId < 0)
		{
			fprintf(stderr,"SecSrv2 not active!\n");
			return 0;
		}
	}
	scanf("%s",para);
	sprintf(Cmd,"ADDE%2.2s",para);
	if (fold_write(g_iHstSrvFoldId,-1,&Cmd,6) < 6)
		fprintf(stderr,"request to SecSrv2 failed,errno=%d\n",errno);

  	return 0;
}


