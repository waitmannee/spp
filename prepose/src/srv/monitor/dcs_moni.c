#include "ibdcs.h"
#include <sys/signal.h>
#include  "tmcibtms.h"
#include "dccdcs.h"

//globals
static int g_myfid    = -1;
static struct ServerAst *g_pserverAstShm=NULL;

//forward declaration
static void dcmMoniExit(int signo);
static int OpenLog(char *IDENT);
static int CreateMyFolder();
static int  StartAllProcesses();
static int LoadidServerCnt();
static int dcs_create_servers_shm(int svrCnt);

//int main_moni(int argc, char *argv[])
int main(int argc, char *argv[])
{
	//prepare the logging stuff
	if(0 > OpenLog(argv[0]))
		exit(1);

	dcs_log(0,0, "Monitor is starting up...\n");
	u_daemonize(NULL);

	catch_all_signals(dcmMoniExit);

	//load idserver config
	//struct  ServerAst serverAstshm;
	int serverCnt = LoadidServerCnt();  
	dcs_create_servers_shm(serverCnt);

	g_pserverAstShm->serverNum = serverCnt;	
	dcs_load_serverast_config(g_pserverAstShm);

	if( 0 > StartAllProcesses())
		dcmMoniExit(0);

	dcs_log(0,0,"*************************************************\n"
	        "!!        Monitor startup completed.           !!\n"
	        "*************************************************\n");

	return 0;
}

static void dcmMoniExit(int signo)
{
	dcs_debug(0,0,"catch a signal %d\n",signo);
	dcs_log(0,0,"monitor terminated.\n");
	exit(signo);
}

static int OpenLog(char *IDENT)
{
	char logfile[256];
	memset(logfile, 0, sizeof(logfile));
	if(u_fabricatefile("log/monitor.log",logfile,sizeof(logfile)) < 0)  
		return -1;

	return dcs_log_open(logfile, IDENT);
}

static int CreateMyFolder()
{  
	if(fold_initsys() < 0)
	{
		dcs_log(0,0, "cannot attach to folder system!");
		return -1;
	}

	g_myfid = fold_create_folder(MONITOR_FOLD_NAME);
	if(g_myfid < 0)
		g_myfid = fold_locate_folder(MONITOR_FOLD_NAME);
	if(g_myfid < 0)
	{
		dcs_log(0,0, "cannot create folder '%s':%s!",  MONITOR_FOLD_NAME,strerror(errno));
		return -1;
	}

	fold_purge_msg( g_myfid );
	return 0;
}

static int StartProcess(char *strExeFile)
{
	dcs_log(0, 0, "StartProcess ........%s", strExeFile);
	int  ret;
	int iCnt = 1;
	int pidChld = -1;
	int i=0;
	char path_exe[512];
	if (g_pserverAstShm != NULL) 
	{	
		for (i=0; i < g_pserverAstShm->serverNum; i ++) 
		{
			struct serverconf * serverCnf = &g_pserverAstShm->server[i];
			//dcs_log(0, 0, "serverCnf->command = %s", serverCnf->command);
			if (serverCnf->max > 1 && strcmp(strExeFile, serverCnf->command) == 0) 
			{
				iCnt = serverCnf->max;
				break;
			}
		}
	}

	dcs_log(0, 0, "StartProcess.iCnt =>> %d", iCnt);

	for (i=0 ; i < iCnt; i++) 
	{
		pidChld = fork();
		if(pidChld == 0)
		{
			memset(path_exe, 0x00, sizeof(path_exe));
			sprintf(path_exe, "%s/run/bin/%s", getenv("EXECHOME"),strExeFile);
			ret = execlp (path_exe, strExeFile,(char *)0);
			if(ret < 0)
			{
				dcs_log(0,0,"cannot exec executable '%s':%s!",strExeFile,strerror(errno));
				exit(249);
			}
		}
	}

	return 0;
}

static int  StartAllProcesses()
{
	int i;
	dcs_log(0, 0, "StartAllProcesses ....");

	printf("\n\n\n\n\n");
	printf("System Starting up ... \n");

	if (g_pserverAstShm != NULL) 
	{		
		for (i=0; i < g_pserverAstShm->serverNum; i ++) 
		{
			struct serverconf * serverCnf = &g_pserverAstShm->server[i];
			dcs_log(0, 0, "serverCnf->command = %s, serverCnf->max = %d", serverCnf->command, serverCnf->max);

			if (serverCnf->max >= 1 && strlen(serverCnf->command) > 0) 
			{
				printf("Start %s subsystem ...\n", serverCnf->para_area);
				StartProcess( serverCnf->command );
			}
			u_sleep_us(100000);/*休息100ms*/
		}
	}
	printf("\n\nSystem startup finish!\n");
	return 0;
}

void get_para_area_str(char *dst_str, char *org_str)
{
	char org_buf[256];
	memset(org_buf, 0x00, sizeof(org_buf));
	memcpy(org_buf, org_str, strlen(org_str));

	char *result = NULL;
	result = strtok(org_buf,"*");
	if( result != NULL )
	{
		result = strtok( NULL, "*" );
		//printf("result = %s\n", result);
		memcpy(dst_str, result, strlen(result));
	}
}

/* load isdsrver.conf */
int dcs_load_serverast_config(struct ServerAst *serverAstShm){
	char cfgfile[256];
	int  iFd, iRc = 0, iCnt, i=0;
	char   caBuffer[512];

	memset(cfgfile, 0x00, sizeof(cfgfile));

	if(u_fabricatefile("config/isdsrver.conf",cfgfile,sizeof(cfgfile)) < 0)
	{
		dcs_log(0,0,"cannot get full path name of 'isdsrver.conf', path = %s\n", cfgfile);
		return -1;
	}

	dcs_log(0,0,"[isdsrver.conf] full path name of 'isdsrver.conf', path = %s\n", cfgfile);

	if(0 > (iFd = conf_open(cfgfile)) )
	{
		dcs_log(0,0,"open file '%s' failed.\n",cfgfile);
		return -1;
	}

	iCnt = serverAstShm->serverNum;
	dcs_log(0, 0, "serverAstshm.serverNum = %d", iCnt);

	int size = sizeof(struct ServerAst) + iCnt * sizeof(struct ServerAst);
	memset(serverAstShm,0,size);
	serverAstShm->serverNum = iCnt;

	char *subsystem, *command, *max, *min, *para_area;
	iRc = conf_get_first_string(iFd, " ",caBuffer);
	dcs_log(0, 0, "serverAstShm ------>caBuffer = %s", caBuffer);
	for(;iRc == 0 ;iRc = conf_get_next_string(iFd, " ",caBuffer))
	{
		subsystem      = strtok(caBuffer," \t\r"); //文件名字
		command     = strtok(NULL," \t\r");
		max     = strtok(NULL," \t\r");
		min  =  strtok(NULL," \t\r");
		para_area         =  strtok(NULL,"\t\r\n");

		if (subsystem == NULL || command == NULL || max == NULL || min == NULL || para_area == NULL )
		{
			dcs_log(0, 0, "skip this line------>caBuffer = %s", caBuffer);
			continue;
		}

		dcs_log(0, 0, "subsystem = %s, command = %s, max = %s, min = %s, para_area = %s", subsystem, command, max, min, para_area);

		strcpy(serverAstShm->server[i].subsystem , subsystem);
		strcpy(serverAstShm->server[i].command , command);
		serverAstShm->server[i].max = atoi(max);
		serverAstShm->server[i].min = atoi(min);
		get_para_area_str(serverAstShm->server[i].para_area, para_area);
		i++;
	}

	conf_close(iFd);

	return 0;
    
}

int dcs_create_servers_shm(int svrCnt)
{
	char *ptr = NULL;
	int  size, shmid = -1;

	size = sizeof(struct ServerAst) + svrCnt * sizeof(struct ServerAst);

	ptr = shm_create(ISDSERVER_SHM_NAME, size, &shmid);

	dcs_log(0, 0, "shm create ISDSERVER_SHM_NAME shm_id = %d", shmid);

	if(ptr == NULL)
	{
		dcs_log(0,0,"cannot create SHM '%s':%s!\n", ISDSERVER_SHM_NAME,strerror(errno));
		return -1;
	}

	//get all the pointers and do initialization
	memset(ptr, 0, size);
	g_pserverAstShm = (struct ServerAst*)ptr;

	return 0;
    
}

int LoadidServerCnt()
{	
	char cfgfile[256];
	int  iFd, iCnt=0;

	if(u_fabricatefile("config/isdsrver.conf",cfgfile,sizeof(cfgfile)) < 0)
	{
		dcs_log(0,0,"cannot get full path name of 'isdsrver.conf'\n");
		return -1;
	}

	if(0 > (iFd = conf_open(cfgfile)) )
	{
		dcs_log(0,0,"open file '%s' failed.\n",cfgfile);
		return -1;
	}

	//读取配置项中的配置项
	if(0 > conf_get_first_number(iFd,"Subsystem.max",&iCnt)|| iCnt < 0 )
	{
		dcs_log(0,0,"<dcs moni> load config item 'Subsystem.max' failed!");
		conf_close(iFd);
		return -1;
	}
	conf_close(iFd);

	return(iCnt);
}


