
#include "cuftp.h"

int mConnected=0;
int   mSocketControl=0,mSocketData=0;    /*控制/数据socket句柄*/
char  mUserName[32];
char  mPassword[32];
char  mHostName[61];                //主机名称
char  mErrorMsg[4096];              /*存放错误消息或者服务器响应信息*/
int   mReplyCode;                   /*服务器响应代码*/
char  mReplyBuf[4096];
char  mTmp_Reply_Buf[4096];
char  mFtpServerCurrDir[128];
int   mErrorCode;                   /*存放错误代码*/
unsigned short mHostPort=21;           //端口号

unsigned short mPasvHostPort=21;           //端口号

int   mCurrMode;
char  mCur_Remote_Path[100];
char  mServer_Ver[200];

int   mTotalBytes;                  //需要上传或下载文件总字节数
int   mProcessedBytes;              //已经上传或下载字节数

extern int CloseDataConnect();

extern int ReadFtpServerReply();         /*读取FTP服务器的响应*/

extern int Select_Sock(int p_Sock_Handle);

extern int GetReplyCode(char* buf);      /*从响应中读取返回代码*/

extern int LoginToFtpServer();

extern int SendFtpCommand(char* command);

extern int CreateListenSocket();

extern int RequestDataConnect();

extern int AcceptDataConnect();


/*
 函数功能：建立FTP链路
 传入参数：
 host: 主机名称或地址
 prot:端口
 username: 用户名称
 password: 登录密码
 返回参数：
 成功 1
 失败 0
 */
int Connect(char* host,char* username,char* password,int port)
{
	struct sockaddr_in ftpaddr;
    
    Disconnect();
    strcpy(mUserName,username);
    strcpy(mPassword,password);
    strcpy(mHostName,host);
    mHostPort=port;
    if((mSocketControl=socket(PF_INET,SOCK_STREAM,0))<0)
    {
        mErrorCode=1002;
        sprintf(mErrorMsg,"Create socket error");
        return 0;
    }
    ftpaddr.sin_family=AF_INET;
    
    char * pos;
    int hasdot=0;
    struct hostent *hp;
    
    ftpaddr.sin_family = AF_INET ;
    pos = mHostName;
    while(*pos !=0 && *pos !=0x0d && *pos != 0x0a)
    {
        if(*pos=='.')
        {
            hasdot=1;
            pos++;
            continue;
        };
        if((*pos<='9'&&*pos>='0')||*pos==' ')
        {
            pos++;
            continue;
        };
        hasdot=0;
        break;
    };
    if(hasdot)
        ftpaddr.sin_addr.s_addr = inet_addr(mHostName) ;
    else
    {
        if((hp=gethostbyname(mHostName))==NULL)
        {
            Disconnect();
            sprintf(mErrorMsg,"Connect to ftp server error");
            return 0;
        }
        else
            memcpy(&ftpaddr.sin_addr, hp->h_addr, hp->h_length);
    };
    
    ftpaddr.sin_port=htons(mHostPort);
    if(connect(mSocketControl,(struct sockaddr*)&ftpaddr,sizeof(struct sockaddr_in)))
    {
        mErrorCode=1003;
        sprintf(mErrorMsg,"Connect to ftp server error");
        Disconnect();
        return 0;
    }
    if(ReadFtpServerReply()>=400) /*有错误发生*/
    {
        Disconnect();
        return 0;
    };
    if(LoginToFtpServer())
    {
        mConnected=1;
    }
    Get_Server_Ver();
    return mConnected;
}


/*
 函数功能：断开FTP链路
 传入参数：
 返回参数：
 永远为1
 */
int Disconnect()
{
    char  commandbuf[32];
    
    if(mConnected)
    {
        sprintf(commandbuf,"QUIT\r\n");
        mReplyCode=SendFtpCommand(commandbuf);
    }
    if(mSocketControl>0)
    {
#ifdef WINDOW_SYS
        closesocket(mSocketControl);
#else
        close(mSocketControl);
#endif
        mSocketControl=-1;
    };
    CloseDataConnect();
    mConnected=0;
    return 1;
}

/*
 函数功能：关闭数据链路
 传入参数：
 返回参数：
 成功 1
 */
int CloseDataConnect()
{
    if(mSocketData<0)
        return 1;
#ifdef WINDOW_SYS
    closesocket(mSocketData);
#else
    close(mSocketData);
#endif
    mSocketData=-1;
    return 1;
}

int ReadFtpServerReply()
{
	char  tmpbuf[512+1],* pstr;
	int   datalength;
	int   num=0,pos;
    
    strcpy(mReplyBuf,mTmp_Reply_Buf);
    pos = strlen(mReplyBuf);
    datalength = -1;
    while(1)
    {
        if(datalength<0&&!Select_Sock(mSocketControl))
            return 0;
        
        datalength=recv(mSocketControl,tmpbuf,512,0);
		if(datalength==0)
            break;
        else
            if(datalength<0)
            {
                num++;
                printf(".");
                if(num>500)
                {
                    mErrorCode=1004;
                    sprintf(mErrorMsg,"Error occur when recv() reply data");
                    return mErrorCode;
                };
                continue;
            }
        pos+=datalength;
        tmpbuf[datalength]=0;
        strcat(mReplyBuf,tmpbuf);
        if(pos>2)
        {
            if(strncmp(mReplyBuf+pos-2,"\r\n",2)==0)
            {
                pstr = mReplyBuf+pos-2;
                while(*pstr!='\n'&&pstr!=mReplyBuf)
                    pstr--;
                if(*pstr=='\n')
                    pstr++;
                if(*(pstr+3)==' ')
                    break;
            }
        }
    }
    
    mReplyCode = GetReplyCode(mReplyBuf);
    if(mReplyCode>=400)  /*有错误*/
    {
        mErrorCode=mReplyCode;
        strcpy(mErrorMsg,mReplyBuf);
    }
    else
        mErrorCode=0;
    return mReplyCode;
}

int Select_Sock(int p_Sock_Handle)
{
	struct fd_set sset;
	FD_ZERO(&sset);
	FD_SET(p_Sock_Handle,&sset);
	
	struct timeval tv;
	
	int activecount;
	
	tv.tv_sec=5;
	tv.tv_usec=500000;
	
	time_t begin_time,cur_time;
	
	time(&begin_time);
	
	//printf("#");
	for(;;)
	{
        activecount=select(p_Sock_Handle + 1,&sset,NULL,NULL,&tv);
        if(activecount>0)
        {
            if(FD_ISSET(p_Sock_Handle,&sset))
                break;
        }
        printf(".");
        time(&cur_time);
        //处理可能中断的情况
        if(activecount==0&&cur_time - begin_time >FTP_TIMEOUT_SECEND)
        {
            printf("\nErr:timeout %d sec! \n",FTP_TIMEOUT_SECEND);
            return 0;
        }
        fflush(stdout);
        sleep(1);
    }
    
    return 1;
}

int GetReplyCode(char* buf)
{
	int replycode;
	char * pos,tmpbuf[4096];
    
    sscanf(buf,"%3d",&replycode);
    pos=buf;
	while(1)
    {
        if(pos[3]==' ')
		{
            pos=strstr(pos,"\r\n");
            if(pos==NULL) return replycode;
            pos+=2;
            break;
        };
        pos=strstr(pos,"\r\n");
        if(pos==NULL) return replycode;
        pos+=2;
    };
    
    strcpy(tmpbuf,pos);
    strcpy(mTmp_Reply_Buf,tmpbuf);
    return replycode;
}

int LoginToFtpServer()
{
    char  commandbuf[64];
    
    /*准备登录到FTP服务器*/
    sprintf(commandbuf,"USER %s\r\n",mUserName);
    mReplyCode = SendFtpCommand(commandbuf);
    
    if(mReplyCode>=400)  /*有错误*/
        return 0;
    
    switch(mReplyCode)
    {
        case 331: /*需要口令*/
            sprintf(commandbuf,"PASS %s\r\n",mPassword);
            mReplyCode=SendFtpCommand(commandbuf);
            if(mReplyCode!=230)
            {
                mErrorCode=1008;
                sprintf(mErrorMsg,"Login to ftp server fail");
                return 0;
            }
            return 1;
        case 230: /*登录成功*/
            return 1;
        default:
            return 0; /*失败*/
    }
}

int SendFtpCommand(char* command)
{
    int len;
    
    len = strlen(command);
    if(send(mSocketControl,command,strlen(command),0)==0)
    {
        mErrorCode=1005;
        sprintf(mErrorMsg,"Send command '%s' to ftp server error",command);
        return mErrorCode;
    }
    return ReadFtpServerReply();
}

int Get_Server_Ver()
{
	char  commandbuf[512];
    
    sprintf(commandbuf,"SYST\r\n");
    mReplyCode=SendFtpCommand(commandbuf);
    if(mReplyCode!=215)
        return 0;
    strcpy(mServer_Ver,mReplyBuf);
    printf("server_ver is '%s'\n",mServer_Ver);
    return 1;
}

/*
 函数功能：改变当前目录
 传入参数：
 newdir: 新的目录路径
 返回参数：
 成功 1
 失败 0
 */
int ChangeDir(char* newdir)
{
    char  commandbuf[512];
    
    sprintf(commandbuf,"CWD %s\r\n",newdir);
    
    mReplyCode = SendFtpCommand(commandbuf);
    
    if(mReplyCode!=250)
        return 0;
    
    strcpy(mCur_Remote_Path,newdir);
    return 1;
}


/*
 函数功能：询问当前目录
 传入参数：
 返回参数：
 成功 返回目录
 失败 ""
 */
char * GetCurrentDir()
{
    char  commandbuf[32],*pbegin,*pend;
    
    mFtpServerCurrDir[0]=0;
    sprintf(commandbuf,"XPWD\r\n");
    mReplyCode=SendFtpCommand(commandbuf);
    if(mReplyCode!=257)
        return mFtpServerCurrDir;
    /*分析缓冲内容*/
    pbegin=strchr(mReplyBuf,'\"');
    pend=strrchr(mReplyBuf,'\"');
    mFtpServerCurrDir[0]=0;
    if(pbegin==NULL||pend==NULL)
        return mFtpServerCurrDir;
    strncpy(mFtpServerCurrDir,pbegin+1,pend - pbegin - 1);
    mFtpServerCurrDir[pend - pbegin -1]=0;
    return mFtpServerCurrDir;
}

/*
 函数功能：上载文件
 传入参数：
 localfile:  本地文件名称
 remotefile：主机文件名称
 返回参数：
 1    成功
 0    失败
 */
int UploadFile(char* localfile,char* remotefile)
{
    char  commandbuf[1024];
    FILE  *fp;
    char  sendbuf[2048];
    int   length,sendlength;
    //struct stat statbuf;
    
    fp=fopen(localfile,"rb");
    if(fp==NULL)
    {
        mErrorCode=1040;
        sprintf(mErrorMsg,"open localfile %s error",localfile);
        puts(mErrorMsg);
        return 0;
    }
    if(!CreateListenSocket())
    {
        fclose(fp);
        return 0;
    };
    sprintf(commandbuf,"STOR %s\r\n",remotefile);
    mReplyCode=SendFtpCommand(commandbuf);
    if(mReplyCode!=150)
    {
        fclose(fp);
        return 0;
    };
    /*分析有多少数据需要接受*/
    //stat(localfile,&statbuf);
    //mTotalBytes=statbuf.st_size;
    if(!AcceptDataConnect())
    {
        fclose(fp);
        return 0;
    };
    mProcessedBytes=0;
    fseek(fp,0,2);
    mTotalBytes=ftell(fp);
    fseek(fp,0,0);
    
    //printf("mTotalBytes is %d mProcessedBytes is %d\n",mTotalBytes,mProcessedBytes);
    while(mProcessedBytes<mTotalBytes)
    {
        length=fread(sendbuf,1,2048,fp);
        //sendlength=Net_SafeSend(mSocketData,sendbuf,length);
        sendlength=send(mSocketData,sendbuf,length,0);
        //printf("leng is %d and send %d\n",length,sendlength);
        if(sendlength!=length)
        {
            mErrorCode=1041;
            sprintf(mErrorMsg,"Send file data to ftp server error");
            fclose(fp);
            return 0;
        }
        mProcessedBytes += length;
    }
    fclose(fp);
    CloseDataConnect();
    if(ReadFtpServerReply()!=226)
        return 0;
    return 1;
}

int CreateListenSocket()
{
	struct sockaddr_in listenaddr;
    
    //puts("CreateListenSocket");
    CloseDataConnect();
    if((mSocketData=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
    {
        mErrorCode=1009;
        sprintf(mErrorMsg,"Create listen socket error");
        puts(mErrorMsg);
        return 0;
    }
    listenaddr.sin_family=AF_INET;
    listenaddr.sin_addr.s_addr=INADDR_ANY;
    listenaddr.sin_port=htons(0);
    //listenaddr.sin_port=24567;
    if(bind(mSocketData,(struct sockaddr*)&listenaddr,sizeof(struct sockaddr_in)))
    {
        mErrorCode=1010;
        sprintf(mErrorMsg,"Bind data socket error");
        puts(mErrorMsg);
        CloseDataConnect();
        return 0;
    }
    if(listen(mSocketData,4))
    {
        mErrorCode=1011;
        sprintf(mErrorMsg,"Listen data socket error");
        puts(mErrorMsg);
        CloseDataConnect();
        return 0;
    }
    return RequestDataConnect();
}

int RequestDataConnect()
{
    struct sockaddr_in tmpaddr;
    int length;
    unsigned short nlocalport;
    char  commandbuf[32],ip[100];
    
    //puts("RequestDataConnect");
    length=sizeof(struct sockaddr_in);
    if(getsockname(mSocketData,(struct sockaddr *)&tmpaddr,&length)<0)
    {
        mErrorCode=1012;
        sprintf(mErrorMsg,"Get listen socket name error");
        puts(mErrorMsg);
        return 0;
    }
    nlocalport=tmpaddr.sin_port;
    
    if(getsockname(mSocketControl,(struct sockaddr*)&tmpaddr,&length)<0)
    {
        mErrorCode=1013;
        sprintf(mErrorMsg,"Get control socket name error");
        puts(mErrorMsg);
        return 0;
    }
    
    sprintf(ip,"%s",inet_ntoa(tmpaddr.sin_addr));
    char *tmpipdjt;
    tmpipdjt=strstr(ip,".");
    while(tmpipdjt!=NULL)
    {
        *tmpipdjt=',';
        tmpipdjt=strstr(ip,".");
    }
    
#ifdef old_code_20011121
    sprintf(commandbuf,"PORT %d,%d,%d,%d,%d,%d\r\n",
            tmpaddr.sin_addr.S_un.S_un_b.s_b1,
            tmpaddr.sin_addr.S_un.S_un_b.s_b2,
            tmpaddr.sin_addr.S_un.S_un_b.s_b3,
            tmpaddr.sin_addr.S_un.S_un_b.s_b4,
            nlocalport&0xff,
            nlocalport>>8);
#endif
    
#ifdef UNIX_SYS
    sprintf(commandbuf,"PORT %s,%d,%d\r\n",
            ip, nlocalport>>8, nlocalport&0xff);
#else
    sprintf(commandbuf,"PORT %s,%d,%d\r\n",
            ip, nlocalport&0xff, nlocalport>>8);
#endif
    
#ifdef SCOUNIX_SYS
    sprintf(commandbuf,"PORT %s,%d,%d\r\n",
            ip, nlocalport&0xff, nlocalport>>8);
#endif
#ifdef DIGITAL_SYS
    sprintf(commandbuf,"PORT %s,%d,%d\r\n",
            ip, nlocalport&0xff, nlocalport>>8);
#endif
    
    
    //printf("commandbuf is '%s' localport is %d\n",commandbuf,nlocalport);
    mReplyCode=SendFtpCommand(commandbuf);
    
    if(mReplyCode!=200)
    {
        mErrorCode=1014;
        sprintf(mErrorMsg,"PORT command to server error:%d,%s",mReplyCode,mErrorMsg);
        puts(mErrorMsg);
        return 0;
    }
    else
        return 1;
}

int AcceptDataConnect()
{
	int tmpsock;
	struct sockaddr_in tmpaddr;
	int length;
    
    length=sizeof(struct sockaddr_in);
    //puts("begin to accept data sock\n");
    if((tmpsock=accept(mSocketData,(struct sockaddr*)&tmpaddr,&length))<0)
    {
        mErrorCode=1020;
        sprintf(mErrorMsg,"Accept server data socket error");
        CloseDataConnect();
        return 0;
    }
    //printf("begin to accept data sock over and sock is %d\n",tmpsock);
    CloseDataConnect();
    mSocketData=tmpsock;
    return 1;
}

int SetMode(int transmode)
{
    char  commandbuf[32];
    switch(transmode)
    {
        case 1:
            sprintf(commandbuf,"TYPE A\r\n");
            break;
        case 2:
            sprintf(commandbuf,"TYPE I\r\n");
            break;
    }
    mReplyCode=SendFtpCommand(commandbuf);
    if(mReplyCode!=200)
        return 0;
    else
        return 1;
}

/*
 函数功能：创建目录
 传入参数：
 pathname:  目录名称
 返回参数：
 1    成功
 0    失败
 */
int Mkdir(char * pathname)
{
	char  commandbuf[512];
    
    sprintf(commandbuf,"MKD %s\r\n",pathname);
    mReplyCode = SendFtpCommand(commandbuf);
    if(mReplyCode!=257)
        return 0;
    return 1;
}

/*
 函数功能：删除目录
 传入参数：
 pathname:  目录名称
 返回参数：
 1    成功
 0    失败
 */

int Rmdir(char * pathname)
{
	char  commandbuf[512];
    
    sprintf(commandbuf,"RMD %s\r\n",pathname);
    mReplyCode=SendFtpCommand(commandbuf);
    if(mReplyCode!=250)
        return 0;
    return 1;
}

/*
 函数功能：删除文件
 传入参数：
 pathname:  删除文件
 返回参数：
 1    成功
 0    失败
 */

int RmFile(char * filename)
{
	char  commandbuf[512];
    
    sprintf(commandbuf,"DELE %s\r\n",filename);
    mReplyCode = SendFtpCommand(commandbuf);
    
    if(mReplyCode!=250)
        return 0;
    return 1;
}


/*
 函数功能：下载文件
 传入参数：
 remotefile：主机文件名称
 localfile:  本地文件名称
 返回参数：
 1    成功
 0    失败
 */
int DownloadFile(char* remotefile,char* localfile)
{
	char  commandbuf[1024];
	FILE  *fp;
	char  recvbuf[2049];
	char  *pbegin,*pend;
	int   length,errcount=0;
	
    if((fp = fopen(localfile,"wb"))==NULL)
    {
        mErrorCode=1030;
        sprintf(mErrorMsg,"Create localfile %s error",localfile);
        return 0;
    }
    if(!CreateListenSocket())
    {
        printf("CreateListenSocket error\n");
        fclose(fp);
        return 0;
    }
    
    sprintf(commandbuf,"RETR %s\r\n",remotefile);
    mReplyCode=SendFtpCommand(commandbuf);
    if(mReplyCode!=150)
    {
        printf("%s \n",mReplyBuf);
        fclose(fp);
        return 0;
    }
    
    mProcessedBytes=0;
    if(!AcceptDataConnect())
    {
        fclose(fp);
        return 0;
    }
    /*
     #ifdef UNIX_SYS
     m_usleep(200);
     #else
     sleep(200);
     #endif
     */
    errcount=0;
    
    length= -1;
    do
    {
        
        if(length<0&&!Select_Sock(mSocketData))
        {
     		CloseDataConnect();
     		return 0;
        }
        
   	 	length = recv(mSocketData,recvbuf,2048,0);
    	if(length>0)
        {
      		if(fwrite(recvbuf,1,length,fp)!=length)
      		{
                printf("***ERROR:write to file error!!!");
                CloseDataConnect();
     			return 0;
            };
            mProcessedBytes += length;
        }
        else
        {
            if(length==0)
                break;
     	 	else
                errcount++;
            printf("try recv file data %d times error \n",errcount);
        }
    } while (errcount<2000);
    
    fclose(fp);
    
    CloseDataConnect();
    if(errcount>=2000)
    {
        printf("recv data has return -1 num 2000,may has error\n");
        return 0;
  	}
    
    if(ReadFtpServerReply()!=226)
        return 0;
    return 1;
}

void PrintMsg()
{
	printf("%s errorcode:%d\n",mErrorMsg,mErrorCode);
}

int PasvMode()
{
	int len;
	char msg[256],tmpbuf[512+1];
	int datalength=-1;
	char *tmp=NULL;
	int num=0;
	char posstr1[512],posstr2[512];
	int pos1=0,pos2=0;
	
	strcpy(msg,"PASV\r\n");
    
	len = strlen(msg);
    
    if(send(mSocketControl,msg,strlen(msg),0)==0)
    {
        printf("Send msg '%s' to ftp server error",msg);
        return -1;
    }
    sleep(1);
	datalength = recv(mSocketControl,tmpbuf,512,0);
    if(datalength==0)
        return -1;
    else
        if(datalength<0)
        {
            printf(".");
            printf("Error occur when recv() reply data");
            return -1;
            
        }
    tmpbuf[datalength]=0;
    
    if ((tmp=strstr(tmpbuf,","))==NULL)
    {
    	return -1;
    }
    memset(posstr1,0,512);
    memset(posstr2,0,512);
    while(1)
    {
    	if (*tmp==41||*tmp==0)
    		break;
    	if (*tmp==44)
    		num++;
    	
    	if (num==4)
    	{
    		if (*tmp>=48&&*tmp<=57)
    		{
    			posstr1[pos1]=*tmp;
    			pos1++;
    		}
    	}
    	if (num==5)
    	{
    		if (*tmp>=48&&*tmp<=57)
    		{
    			posstr2[pos2]=*tmp;
    			pos2++;
    		}
    	}
    	tmp++;
    }
    if ((strcmp(posstr1,"")==0)||(strcmp(posstr2,"")==0))
    	return -1;
    
    pos1=atoi(posstr1);
    pos2=atoi(posstr2);
    mPasvHostPort=pos1*256+pos2;
    return 1;
}

/*
 函数功能：被动模式：：上载文件
 传入参数：
 localfile:  本地文件名称
 remotefile：主机文件名称
 返回参数：
 1    成功
 0    失败
 */
int UploadFile_pasv(char* localfile,char* remotefile)
{
    char  commandbuf[1024];
    FILE  *fp;
    char  sendbuf[2048];
    int   length,sendlength;
    //struct stat statbuf;
    
    if (PasvMode()!=1)
    {
        sprintf(mErrorMsg,"get ftp server'port error");
        puts(mErrorMsg);
        return 0;
    }
    
    fp=fopen(localfile,"rb");
    if(fp==NULL)
    {
        mErrorCode=1040;
        sprintf(mErrorMsg,"open localfile %s error",localfile);
        puts(mErrorMsg);
        return 0;
    }
    if(!CreatePasvConnect())
    {
        fclose(fp);
        return 0;
    };
    sprintf(commandbuf,"STOR %s\r\n",remotefile);
    mReplyCode=SendFtpCommand(commandbuf);
    if(mReplyCode!=150)
    {
        fclose(fp);
        CloseDataConnect();
        return 0;
    };
    /*分析有多少数据需要接受*/
    //stat(localfile,&statbuf);
    //mTotalBytes=statbuf.st_size;
    mProcessedBytes=0;
    fseek(fp,0,2);
    mTotalBytes=ftell(fp);
    fseek(fp,0,0);
    
    //printf("mTotalBytes is %d mProcessedBytes is %d\n",mTotalBytes,mProcessedBytes);
    while(mProcessedBytes<mTotalBytes)
    {
        length=fread(sendbuf,1,2048,fp);
        //sendlength=Net_SafeSend(mSocketData,sendbuf,length);
        sendlength=send(mSocketData,sendbuf,length,0);
        //printf("leng is %d and send %d\n",length,sendlength);
        if(sendlength!=length)
        {
            mErrorCode=1041;
            sprintf(mErrorMsg,"Send file data to ftp server error");
            CloseDataConnect();
            fclose(fp);
            return 0;
        }
        mProcessedBytes += length;
    }
    fclose(fp);
    CloseDataConnect();
    if(ReadFtpServerReply()!=226)
        return 0;
    return 1;
}

int CreatePasvConnect()
{
	struct sockaddr_in acceptaddr;
    
    //puts("CreateListenSocket");
    CloseDataConnect();
    if((mSocketData=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
    {
        mErrorCode=1009;
        sprintf(mErrorMsg,"Create listen socket error");
        puts(mErrorMsg);
        return 0;
    }
    acceptaddr.sin_family=AF_INET;
    acceptaddr.sin_addr.s_addr=inet_addr(mHostName);
    acceptaddr.sin_port=htons(mPasvHostPort);
    
    if(connect(mSocketData,(struct sockaddr*)&acceptaddr,sizeof(struct sockaddr_in)))
    {
        mErrorCode=2003;
        sprintf(mErrorMsg,"create mSocketData to ftp server error");
        CloseDataConnect();
        return 0;
    }
    return 1;
}

int FileList(char* curpwd, char * logfile)
{
	char  commandbuf[1024];
	char  recvbuf[2049];
	int   length,errcount=0;
	FILE  *fp;
	
	//写日志文件：
	if((fp=fopen(logfile,"wb"))==NULL)
    {
        mErrorCode=1030;
        sprintf(mErrorMsg,"Create logfile %s error!",logfile);
        return 0;
    }
    
    
    /*建立数据监听*/
    if(!CreateListenSocket())
    {
        fclose(fp);
        printf("CreateListenSocket error\n");
        return 0;
    }
	
	/*发送命令到服务器*/
	
	
	sprintf(commandbuf,"LIST %s\r\n",curpwd);
    mReplyCode=SendFtpCommand(commandbuf);
    if(mReplyCode!=150)
    {
        printf("err:%s  \n",mErrorMsg);
        fclose(fp);
        CloseDataConnect();
        return 0;
    };
    
    
    /*检索连接上的数据链路*/
    if(!AcceptDataConnect())
    {
        fclose(fp);
        return 0;
    };
    
    
    errcount=0;
    
    length= -1;
    do
    {
        if(length<0&&!Select_Sock(mSocketData))
        {
     		fclose(fp);
     		CloseDataConnect();	
     		return 0;
    	}
        
   	 	length=recv(mSocketData,recvbuf,2048,0);
        
    	if(length>0)
    	{
            if(fwrite(recvbuf,1,length,fp)!=length)
            {
                printf("***ERROR:write to file error!!!");
                fclose(fp);
                CloseDataConnect();	
     			return 0;
    		}
        }	
        else
        {
            if(length==0)
                break;
     	 	else
                errcount++;
            printf("try recv file data %d times error \n",errcount);
        }   
    }while(errcount<200);
    fclose(fp);
    CloseDataConnect();
    if(errcount>=200)
    {
        printf("recv data has return -1 num 2000,may has error\n");
        return 0;
    }
    
    if(ReadFtpServerReply()!=226)
        return 0;
    return 1;
}



