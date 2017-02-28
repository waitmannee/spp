#include "msg_api.h"
#include "filepub.h"

#define LOGMODE 3		/*日志记录方式*/


int SendMsg_api(char *operid,char *operpwd,char *rectype,char *recid,char *msgbuf,char *routeid,
                char *ridertype,char *riderinfo)
{
	printf("msgbuf = %s\n",msgbuf);
    
  	int sockid;
	char my_addr[20];
	char errmsg[2048],msgdata[2048];
	int my_port;
	int buflen;
	int msglen;
    
	struct sockaddr_in thieraddr;
	
	if (GetConIni("MSGSOCKETSET",my_addr,&my_port)<0)
	{
		return 0;
	}
	/*创建套接字*/
	if ((sockid = socket(AF_INET,SOCK_STREAM,0))<0)
	{
		sprintf(errmsg,"create stream socket error!!");
		writelog(errmsg,LOGMODE);
		return -1;
	}
    
	memset((char *)&thieraddr,0,sizeof(struct sockaddr_in));
	thieraddr.sin_family=AF_INET;
	thieraddr.sin_addr.s_addr = inet_addr(my_addr);
	thieraddr.sin_port=htons(my_port);
	
	if (connect(sockid,(struct sockaddr *)&thieraddr,sizeof(thieraddr))<0)
	{
		sprintf(errmsg,"connect msg error!!");
		writelog(errmsg,LOGMODE);
		close(sockid);
		return -1;
	}
    
	memset(msgdata,0,2048);
	buflen=4;
	/*操作人ID*/
	memset(errmsg,' ',2048);
	memcpy(errmsg,operid,strlen(operid));
	errmsg[12]=0;
	memcpy(msgdata+buflen,errmsg,10);
	buflen=buflen+10;
	memset(errmsg,' ',2048);
	memcpy(errmsg,operpwd,strlen(operpwd));
	errmsg[12]=0;
	memcpy(msgdata+buflen,errmsg,6);
	buflen=buflen+6;
	memcpy(msgdata+buflen,rectype,1);
	buflen=buflen+1;
	/*加载接收标识*/
	msglen=strlen(recid);
	sprintf(errmsg,"%02d",msglen);
	memcpy(msgdata+buflen,errmsg,2);
	buflen=buflen+2;
	memcpy(msgdata+buflen,recid,msglen);
	buflen=buflen+msglen;
	/*加载短信内容*/
	msglen=strlen(msgbuf);
	sprintf(errmsg,"%03d",msglen);
	memcpy(msgdata+buflen,errmsg,3);
	buflen=buflen+3;
	memcpy(msgdata+buflen,msgbuf,msglen);
	buflen=buflen+msglen;
	/*加载路由编号*/
	memset(errmsg,' ',2048);
	memcpy(errmsg,routeid,strlen(routeid));
	errmsg[2]=0;
	memcpy(msgdata+buflen,errmsg,2);
	buflen=buflen+2;
	/*加载附加域类型*/
	memset(errmsg,' ',2048);
	memcpy(errmsg,ridertype,strlen(ridertype));
	errmsg[1]=0;
	memcpy(msgdata+buflen,errmsg,1);
	buflen=buflen+1;
	/*加载附加域长度和内容*/
	msglen=strlen(riderinfo);
	sprintf(errmsg,"%03d",msglen);
	memcpy(msgdata+buflen,errmsg,3);
	buflen=buflen+3;
	memset(errmsg,' ',2048);
	memcpy(msgdata+buflen,riderinfo,msglen);
	buflen=buflen+msglen;
    
	/*加载数据包总长度*/
	sprintf(errmsg,"%4d",buflen);
	memcpy(msgdata,errmsg,4);
    
	if ((msglen=send(sockid,msgdata,buflen,0))<0)
	{
		sprintf(errmsg,"send error!!");
		writelog(errmsg,LOGMODE);
		close(sockid);
		return -1;
	}
 	sprintf(errmsg,"msgdata-->%s,msglen-->%d!",msgdata,buflen);
 	WriteMsgLog(errmsg);
	close(sockid);
	return 1;
}

int GetConIni(char *inihead,char *ipaddr,int *ipport)
{
	char ip_addr[256];
	char ip_port[256];
	char msg[1024];
	
	if (LoadFile("mesg_listen.ini")<0)
	{
		GetIniErrMsg(msg);
		writelog(msg,LOGMODE);
		Freeini();
		return -1;
	}
	
	if (GetItemData(inihead,"adderss",ip_addr)<0)
	{
		GetIniErrMsg(msg);
		writelog(msg,LOGMODE);
		Freeini();
		return -1;
	}
	
	if (GetItemData(inihead,"port",ip_port)<0)
	{
		GetIniErrMsg(msg);
		writelog(msg,LOGMODE);
		Freeini();
		return -1;
	}
	
	Freeini();
	
	strcpy(ipaddr,ip_addr);
	*ipport = atoi(ip_port);
	
	return 1;
}

void WriteMsgLog(char *Msg)
{
	FILE * fp;
    int error_no;
    char filename[256];
    char errmsg[4096];
    char v_date[20];
	
	gettime(filename);
	memset(v_date,0,20);
	memcpy(v_date,filename,8);
	sprintf(errmsg,"时间%s : %s\n",filename,Msg);
    
	sprintf(filename,"SndMsgDetail%s.log",v_date);
	if ((fp=fopen(filename,"a+"))==NULL)
 	{
 		error_no=errno;
        printf("open %s error!,errorcode=%d \n",filename,error_no);
        return ;
 	}
 	else
 	{
        fseek(fp,0L,SEEK_END);
        fputs(errmsg,fp);
        fclose(fp);
 	}
}
