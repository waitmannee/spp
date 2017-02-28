
#include "movedfile.h"
#include "cuftp.h"
#include "mypublic.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>

int ConnectFtp(char* host,char* username,char* password,int port)
{
	char msg[1024];
	if (!Connect(host,username,password,port))
	{
		sprintf(msg,"connect %s %d by %s/%s error",host,port,username,password);
		printf("%s\n",msg);
		return -1;
	}
	sprintf(msg,"connect %s %d by %s/%s succeed!",host,port,username,password);
	printf("%s\n",msg);
    
	return 1;
}

int SerchFile(char *fpath)
{
	char msg[1024];
	
    
	dealfilelist();
	
	return 1;
}

int dealfilelist()
{
	char curdate[256];
	
	//得到当前时间
	gettime(curdate);
	AddDate(curdate,-3);
	
	//处理支付子系统
	//dealkerswit(curdate);
	
	//处理pep1.7
	dealpayeasy(curdate);
	
	//处理pep2.0
	//dealdcsp(curdate);
	
	
	return 1;
}


int dealkerswit(char *curdate)
{
	char msg[1024];
	FILE *fp;
	char databuf[2048];
	char locfile[256];
	char tmpfile[256];
	char remotefile[256];
	char stddate[256];
	char nostddate[20];
	char remotepath[256];
	struct dirent * pDirEnt;
    DIR *dir;
	struct stat ftype;
	
	strcpy(stddate,curdate);
	AddDate(stddate,-1);
	
	memset(nostddate,0,20);
	
	memcpy(nostddate,stddate+2,6);
	
	//连接服务器
	
	if (ConnectFtp("10.1.11.7","stlment","abc123",21)<0)
	{
		return 0;
	}
	
	//设置传输方式为ASCII传输
	SetMode(2);
	
	//得到文件夹中文件列表
	
	//打开指定目录
    if((dir = opendir("/home/kerswit/logapp"))==NULL)
    {
        printf("打开指定目录 /home/kerswit/logapp 失败！\n");
        Disconnect();
        return 0;
    }
    /*创建文件夹*/
    sprintf(remotepath,"/home/stlment/logbak/kerswit/log%s",stddate);
	if (Mkdir(remotepath)!=1)
	{
		sprintf(msg,"create %s err!",remotepath);
		printf("err info:%s!\n",msg);
		Disconnect();
		return 0;
	}
	/*循环搜索指定目录*/
    while((pDirEnt = readdir(dir))!=NULL)
    {
		
		sprintf(locfile,"/home/kerswit/logapp/%s",pDirEnt->d_name);
		sprintf(remotefile,"%s/%s",remotepath,pDirEnt->d_name);
		
		/*得到文件信息*/
        if(stat(locfile,&ftype)<0)
            continue;
        if(pDirEnt->d_name[0]=='.')
            continue;
        if(S_ISDIR(ftype.st_mode))
        {
            /*如果是目录文件，则继续*/
            continue;
        }
		if(S_ISREG(ftype.st_mode)) /*普通文件*/
		{
			/*取文件名中的日期*/
			strcpy(databuf,pDirEnt->d_name);
			Allt(databuf);
			/*比较要处理的日期日志*/
			if (strstr(databuf,nostddate)!=NULL||strstr(databuf,stddate)!=NULL)
			{
				/*压缩文件*/
				
				if (locfile[strlen(locfile)-1]!='Z')
				{
					sprintf(tmpfile,"compress %s",locfile);
					system(tmpfile);
					strcat(locfile,".Z");
					strcat(remotefile,".Z");
				}
                
				/*上传到备份机*/
				if (UploadFile_pasv(locfile,remotefile)!=1)
				{
					sprintf(msg,"UploadFile %s err!",locfile);
					printf("err info:%s!\n",msg);
					break ;
				}
				else //下载成功则删除文件
				{
					printf("locfile:%s! remotefile:%s ok!\n",locfile,remotefile);
					sprintf(tmpfile,"rm %s",locfile);
					system(tmpfile);
					printf("rm %s ok!\n",locfile);
				}
			}
			
		}
		
	}
	closedir(dir);
	Disconnect();
	return 1;
}

int dealpayeasy(char *curdate)
{
	char msg[1024];
	FILE *fp;
	char databuf[2048];
	char locfile[256];
	char tmpfile[256];
	char remotefile[256];
	char stddate[256];
	char nostddate[20];
	char remotepath[256];
	struct dirent * pDirEnt;
    DIR *dir;
	struct stat ftype;
	
	strcpy(stddate,curdate);
	AddDate(stddate,-1);
	
	memset(nostddate,0,20);
	
	memcpy(nostddate,stddate+4,4);
	
	//连接服务器
    
	if (ConnectFtp("10.1.11.7","stlment","abc123",21)<0)
	{
		return 0;
	}
	
	//设置传输方式为ASCII传输
	SetMode(2);
	
	//得到文件夹中文件列表
	
	//打开指定目录
    if((dir = opendir("/home/payeasy/run/DATALOG"))==NULL)
    {
        printf("打开指定目录 /home/payeasy/run/DATALOG 失败！\n");
        Disconnect();
        return 0;
    }
	
	/*创建文件夹*/
    sprintf(remotepath,"/home/stlment/logbak/payeasy/log%s",stddate);
	if (Mkdir(remotepath)!=1)
	{
		sprintf(msg,"create %s err!",remotepath);
		printf("err info:%s!\n",msg);
		Disconnect();
		return 0;
	}
	
	printf("1\n");
	/*循环搜索指定目录*/
    while((pDirEnt = readdir(dir))!=NULL)
    {
		
		sprintf(locfile,"/home/payeasy/run/DATALOG/%s",pDirEnt->d_name);
		sprintf(remotefile,"%s/%s",remotepath,pDirEnt->d_name);
		/*得到文件信息*/
        if(stat(locfile,&ftype)<0)
            continue;
        if(pDirEnt->d_name[0]=='.')
            continue;
        if(S_ISDIR(ftype.st_mode))
        {
            /*如果是目录文件，则继续*/
            continue;
        }
		if(S_ISREG(ftype.st_mode)) /*普通文件*/
		{
			/*取文件名中的日期*/
			strcpy(databuf,pDirEnt->d_name);
			Allt(databuf);
			/*比较要处理的日期日志*/
			if (strstr(databuf,nostddate)!=NULL||strstr(databuf,stddate)!=NULL)
			{
				/*压缩文件*/
				printf("2\n");
				if ((locfile[strlen(locfile)-1]!='Z')&&(ftype.st_size!=0))
				{
					sprintf(tmpfile,"compress %s",locfile);
					printf("compress:%s!\n",tmpfile);
					system(tmpfile);
					strcat(locfile,".Z");
					strcat(remotefile,".Z");
				}
				printf("3\n");
				/*上传到备份机*/
				if (UploadFile_pasv(locfile,remotefile)!=1)
				{
					sprintf(msg,"UploadFile_pasv %s err!",locfile);
					printf("err info:%s!\n",msg);
					break ;
				}
				else //下载成功则删除文件
				{
					
					printf("locfile:%s! remotefile:%s ok!\n",locfile,remotefile);
					sprintf(tmpfile,"rm %s",locfile);
					system(tmpfile);
					printf("rm %s ok!\n",locfile);
				}
			}
			
		}
		
	}
	closedir(dir);
	Disconnect();
	return 1;
}


int dealdcsp(char *curdate)
{
	char msg[1024];
	FILE *fp;
	char databuf[2048];
	char locfile[256];
	char tmpfile[256];
	char remotefile[256];
	char nextpath[256];
	char stddate[256];
	char nostddate[20];
	char remotepath[256];
	
	struct dirent * pDirEnt;
    DIR *dir;
	struct stat ftype;
	
	strcpy(stddate,curdate);
	AddDate(stddate,-1);
	
	memset(nostddate,0,20);
	
	memcpy(nostddate,stddate+4,4);
	
	//连接服务器
    
	if (ConnectFtp("10.1.11.7","stlment","abc123",21)<0)
	{
		return 0;
	}
	
	//设置传输方式为ASCII传输
	SetMode(2);
	
	//得到文件夹中文件列表
	
	//打开指定目录
    if((dir = opendir("/home/dcsp/run/log")) == NULL)
    {
        printf("打开指定目录 /home/dcsp/run/log 失败！\n");
        return 0;
    }
    
    /*创建文件夹*/
    sprintf(remotepath,"/home/stlment/logbak/pay20/log%s",stddate);
	if (Mkdir(remotepath)!=1)
	{
		sprintf(msg,"create %s err!",remotepath);
		printf("err info:%s!\n",msg);
		return 0;
	}
    
    /*查找目标文件夹*/
    while((pDirEnt = readdir(dir))!=NULL)
    {
        sprintf(locfile,"/home/dcsp/run/log/%s",pDirEnt->d_name);
        /*得到文件信息*/
        if(stat(locfile,&ftype)<0)
            continue;
        if(pDirEnt->d_name[0]=='.')
            continue;
        if(S_ISDIR(ftype.st_mode))
        {
            /*如果是目录文件，则对比是否是需要处理的文件夹*/
            if (strstr(databuf,nostddate)!=NULL)
            {
                
                sprintf(nextpath,"/home/dcsp/run/log/%s",pDirEnt->d_name);
                break;
            }
            
        }
        else
        {
            continue;
        }
	}
	closedir(dir);
	
	//打开指定目录
    if((dir = opendir(nextpath))==NULL)
    {
        printf("打开指定目录 /home/dcsp/run/log 失败！\n");
        return 0;
    }
	
	/*循环搜索指定目录*/
    while((pDirEnt = readdir(dir))!=NULL)
    {
		
		sprintf(locfile,"%s%s",nextpath,pDirEnt->d_name);
		sprintf(remotefile,"%s/%s",remotepath,pDirEnt->d_name);
		/*得到文件信息*/
        if(stat(locfile,&ftype)<0)
            continue;
        if(pDirEnt->d_name[0]=='.')
            continue;
        if(S_ISDIR(ftype.st_mode))
        {
            /*如果是目录文件，则继续*/
            continue;
        }
		if(S_ISREG(ftype.st_mode)) /*普通文件*/
		{
			/*取文件名中的日期*/
			strcpy(databuf,pDirEnt->d_name);
			Allt(databuf);
			/*比较要处理的日期日志*/
			if (strstr(databuf,nostddate)!=NULL&&strstr(databuf,stddate)!=NULL)
			{
				/*压缩文件*/
				if (locfile[strlen(locfile)-1]!='Z')
				{
					sprintf(tmpfile,"compress %s",locfile);
					system(tmpfile);
					strcat(locfile,".Z");
					strcat(remotefile,".Z");
				}
				/*上传到备份机*/
				if (UploadFile_pasv(locfile,remotefile)!=1)
				{
					sprintf(msg,"UploadFile_pasv %s err!",locfile);
					printf("err info:%s!\n",msg);
					break ;
				}
				else //下载成功则删除文件
				{
					
					printf("locfile:%s! remotefile:%s ok!\n",locfile,remotefile);
					sprintf(tmpfile,"rm %s",locfile);
					system(tmpfile);
					printf("rm %s ok!\n",locfile);
				}
			}
			
		}
		
	}
	closedir(dir);
	sprintf(tmpfile,"rmdir %s",nextpath);
	system(tmpfile);
	Disconnect();
	return 1;
}


//** Freax 读写文件操作 **//
static void setConfig(void)
{
    FILE  *fp;
    char   buf[4096];
    char  *p;
    int   i,len;
    static char name[32];
    static char password[32];
    static char nic[32];
    static char fakeAddress[32];
    int  intelligentReconnect=-1;
    int  echoInterval=-1;
    int  authenticationMode=-1;
    
    //the check and anylysis against Dot1xClient.conf  *don't*  work perfectly.
    //this may be improved in the later version.
    if( (fp=fopen("Dot1xClient.conf","r"))==NULL )
    {
        //err_quit("cannot open file Dot1xClient.conf ! check it.\n");
    }
    
    while(fgets(buf,sizeof(buf),fp)!=NULL)//fgets遇到换行或EOF会结束
    {
        if( (buf[0]=='#') || (buf[0]=='\n') )
            continue;//继续下一循环
        len = strlen(buf);
        if(buf[len-1]=='\n')
            buf[len-1]='\0';//in order to form a string
        if( ( (p=strchr(buf,'=')) == NULL) || (p==buf) )//if not find =
            continue;
        //the code above doesn't detect ALL the errors!! it should be improved in future.
        
        *p='\0';//break the string into 2 parts.
        p++;//p ponit to the value now
        
        for(i=0; i<strlen(buf); i++)
            buf[i]=tolower(buf[i]);
        
        if(strcmp(buf,"name")==0)
        {
            strncpy(name,p,sizeof(name)-1); //char *strncpy(char *dest, const char *src, size_t n);
            name[sizeof(name)-1]=0;
         //   m_name=name; 
        }
        else if(strcmp(buf,"password")==0)
        {
            strncpy(password,p,sizeof(password)-1);
            password[sizeof(password)-1]=0;
            //m_password=password;
        }
        else if(strcmp(buf,"authenticationmode")==0)
        { authenticationMode=atoi(p);
           // m_authenticationMode=authenticationMode;
        }
        else if(strcmp(buf,"nic")==0)
        {
            for(i=0; i<strlen(p); i++) p[i]=tolower(p[i]);
            strncpy(nic,p,sizeof(nic)-1); nic[sizeof(nic)-1]=0; //m_nic=nic;
        }
        else if(strcmp(buf,"echointerval")==0)
        {
            //echoInterval=atoi(p);   m_echoInterval=echoInterval;
        }
        else if(strcmp(buf,"intelligentreconnect")==0)
        {
            intelligentReconnect=atoi(p);
            //m_intelligentReconnect=intelligentReconnect;
        }
        else if(strcmp(buf,"fakeaddress")==0)
        {
            strncpy(fakeAddress,p,sizeof(fakeAddress)-1);
            fakeAddress[sizeof(fakeAddress)-1]=0;
            
            //if( inet_pton(AF_INET,fakeAddress,m_ip)<=0 )
              //  err_msg("invalid fakeAddress found in Dot1xClient.conf, ignored...\n");
            //else m_fakeAddress=fakeAddress;
        }
        else continue;
    }
    if(ferror(fp)) //err_quit("cannot read Dot1xClient.conf ! check it.\n");
    fclose(fp);
    
//    if((m_name==NULL)||(m_name[0]==0)) err_quit("invalid name found in Dot1xClient.conf!\n");
//    if((m_password==NULL)||(m_password[0]==0)) err_quit("invalid password found in Dot1xClient.conf!\n");
//    if((m_authenticationMode<0)||(m_authenticationMode>1))
//        err_quit("invalid authenticationMode found in Dot1xClient.conf!\n");
//    if( (m_nic==NULL) || (strcmp(m_nic,"")==0) ||  (strcmp(m_nic,"any")==0) )
//        err_quit("invalid nic found in Dot1xClient.conf!\n");
//    if((m_echoInterval<0)||(m_echoInterval>100))
//        err_quit("invalid echo interval found in Dot1xClient.conf!\n" );
//    if((m_intelligentReconnect<0)||(m_intelligentReconnect>1))
//        err_quit("invalid intelligentReconnect found in Dot1xClient.conf!\n");
    
    /*printf("m_name=%s\n",m_name);
     printf("m_password=%s\n",m_password);
     printf("m_nic=%s\n",m_nic);
     printf("m_authenticationMode=%d\n",m_authenticationMode);
     printf("m_echoInterval=%d\n",m_echoInterval);
     printf("m_intelligentReconnect=%d\n",m_intelligentReconnect);//NOT supported now!!
     printf("m_fakeAddress=%s\n",m_fakeAddress); */
    
    //just set them to zero since they don't seem to be important.
//    memset(m_netgate,0,sizeof(m_netgate));  memset(m_dns1,0,sizeof(m_dns1));
}


void checkconfig()
{
    char line[100];
    printf("checking user config...\n");
    FILE * file;
    while(fgets(line,100,file))
    {
        line[strlen(line)-1]='\0';//add a flag of string end
        if(!memcmp("username",line,strlen("username")))//if find username
        {
           // memcpy(usrData.username,(line+sizeof("username")),sizeof(usrData.username));
            //printf("%s\n",usrData.username);
            continue;
        }
        if(!memcmp("password",line,strlen("password")))
        {
            //memcpy(usrData.password,(line+sizeof("password")),sizeof(usrData.password));
            printf("********\n");
            continue;
        }
        if(!memcmp("Ip",line,strlen("Ip")))
        {
         //   memcpy(usrData.Ip,(unsigned char *)(line+sizeof("Ip")),sizeof(usrData.Ip));
           // sprintf(strIP,"%d.%d.%d.%d",usrData.Ip[0],usrData.Ip[1],usrData.Ip[2],usrData.Ip[3]);//先格式化一下字符窜
           // printf("%s\n",strIP);
            continue;
        }
        if(!memcmp("Mac",line,strlen("Mac")))
        {
            //memcpy(usrData.Mac,(unsigned char *)(line+sizeof("Mac")),sizeof(usrData.Mac));
            //printf("%02x-%02x-%02x-%02x-%02x-%02x\n",usrData.Mac[0],usrData.Mac[1],
              //     usrData.Mac[2],usrData.Mac[3],usrData.Mac[4],usrData.Mac[5]);
            continue;
        }
        if(!memcmp("nic",line,strlen("nic")))
        {
           // memcpy(usrData.nic,(line+sizeof("nic")),sizeof(usrData.nic));
            //printf("%s\n",usrData.nic);
            break;
        }
    }
    fclose(file);
}

void writeconfig()
{
    FILE * file;
    file=fopen("dot1xClient.conf","w");
    fputs("username=",file);
    //fputs(usrData.username,file);
    fputs("\n",file);
    
    fputs("password=",file);
    //fputs(usrData.password,file);
    fputs("\n",file);
    
    fputs("Ip=",file);
    //fputc(usrData.Ip[0],file);
    //fputc(usrData.Ip[1],file);
    //fputc(usrData.Ip[2],file);
    //fputc(usrData.Ip[3],file);
    fputs("\n",file);
    
    fputs("Mac=",file);
    //fputc(usrData.Mac[0],file);
    //fputc(usrData.Mac[1],file);
    //fputc(usrData.Mac[2],file);
    //fputc(usrData.Mac[3],file);
    //fputc(usrData.Mac[4],file);
    //fputc(usrData.Mac[5],file);
    fputs("\n",file);
    
    fputs("nic=",file);
    //fputs(usrData.nic,file);
    fputs("\n",file);
    fclose(file);
}