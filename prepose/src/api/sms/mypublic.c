#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/stat.h>
#include <time.h>
#include <string.h>


#include "mypublic.h"

/****************************以下是字符串操作函数***********************************/

/*从指定位置段得到子字符串*/
char *GetFeilderStr(char *tagstr,char *recstr,int startpos,int endpos)
{
	char *rectmp;
	char *tagtmp;
	int len=0,i=0;
	
	if (startpos>endpos)
        return NULL;
	if (startpos<0)
		return NULL;
	rectmp=recstr;
	tagtmp=tagstr;
	len=(int)strlen(rectmp);
	if (endpos>(len-1))
		return NULL;
	for( i=startpos;i<len&&i<=endpos;i++)
	{
        *tagtmp=*(rectmp+i);
        tagtmp++;
	}
	*tagtmp='\0';
	return Allt(tagstr);
}

/*从带有路径的文件名中，提取不带路径的文件名*/

char *getname(char *fname,char *recfname)
{
	char *tmp;
	int len=0,i=0,d=0;
	
	tmp=fname;
	len=(int)strlen(recfname);
	if (len<1)
        return NULL;
	for(i=len-1;i>=0;i--)
	{
        if (*(recfname+i)==47)
        {
            break;
        }
	}
	for(d=i+1;d<len;d++)
	{
        *tmp=*(recfname+d);
        tmp++;
	}
	*tmp='\0';
	return fname;
}

/*去掉字符串开始和结尾存在的空格*/
char *Allt(char *str)
{
	char *tmpstr;
	int d_len,i=0;
	
	tmpstr=str;
	d_len=(int)strlen(tmpstr);
	for(i=d_len-1;i>=0;i--)
	{
		if (*(tmpstr+i)==32)
        {
            *(tmpstr+i)='\0';
        }
        else
            break ;
	}
	d_len=(int)strlen(tmpstr);
	
	for(i=0;i<d_len;i++)
	{
		if (*tmpstr==32)
        {
            *tmpstr++;
        }
        else
        {
            break ;
        }
	}
	strcpy(str,tmpstr);
	return str;
}

/*从最末位取指定位数的子字符串*/
char *GetsubString(char *newstr,char *srcstr,int size)
{
	char *rectmp=NULL;
	int len=0,i=0;
	
	len=(int)strlen(srcstr);
	if (len>size)
		return NULL;
	rectmp=newstr;
	for(i=len-1;i>=0;i--)
	{
		*rectmp=*(srcstr+i);
		rectmp++;
		size--;
		if (size==0)
            break ;
	}
	*rectmp='\0';
	return newstr;
}

/*从目标字符串的指定位置加载指定长度的子字符串*/
char *SetStrstr(char *srcstr,char *dstr,int start,int size)
{
	int len=0,i=0;
	
	len=(int)strlen(dstr);
	for(i=0;i<size && i<len;i++)
	{
		*(srcstr+start+i)=*(dstr+i);
	}
	return srcstr;
}

/*检查字符串各位是否是数字*/
int IsNumber(char *str)
{
	int len=0,i=0;
	
	len=strlen(str);
	
	for(i=0;i<len;i++)
	{
		if ((*(str+i)<48||*(str+i)>57)&&(*(str+i)!=46))
			return 0;
	}
	return 1;
}

/*对实数进行截位操作*/

double Round(double x,int bytes,int mode)
{
	char s_val[256], c[6];
	
	char *tmpstr=NULL;
	
	double reval=0,y=0;
	int d_c=0,i=0;
	
	/*处理为字符串*/
	sprintf(s_val,"%lf",x);
	
	tmpstr = strstr(s_val,".");
	
	if (tmpstr == NULL)
		return x;
	/*判断是否取成整数，如果byete参数小于等于0，则默认为0*/
	if (bytes<=0)
	{
		bytes=0;
	}
	/*保存要舍弃的最高位数字，从指定位置截断字符串*/
	if (bytes==0)
	{
		c[0]=*(tmpstr+1);
		c[1]='\0';
		*tmpstr='\0';
	}
	else
	{
		c[0]=*(tmpstr+bytes+1);
		c[1]='\0';
		*(tmpstr+bytes+1)=0;
	}
	sscanf(s_val,"%lf",&reval);
	d_c=atoi(c);
	
	/*判断舍弃的最高位数字的值*/
	if ((d_c>=5&&mode==1)||(mode==3&&d_c!=0))
	{
		d_c=1;
	}
	else
    {
        d_c=0;
    }
    /*如果高位需要进一，则得到剩余数字最高位加一*/
	if (d_c==1)
	{
		y=1;
		for(i=1;i<=bytes;i++)
		{
			y=y/10;
		}
	}
	/*进位加原数字得到最后数字*/
	if(x<0)
        reval=reval - y;
    else
        reval=reval + y;
	
	return reval;
}

/*查找字串*/
int SubStrIsExist(char *srcstr,char *tagsubstr,int pos)
{
	int srclen=0;
	int taglen=0;
	int tagstartpos=0;
	int srcstartpos=0;
	int id=0;
	
	srclen=strlen(srcstr);
	taglen=strlen(tagsubstr);
	
	if (taglen>srclen)
		return 0;
	
	srclen--;
	taglen--;
	
	if (pos==0)
	{
		for(id=0;id<=taglen;id++)
		{
			if (*(tagsubstr+id)==*(srcstr+id))
				continue;
			
			return 0;
		}
	}
	else
	{
		for(id=0;id<=taglen;id++)
		{
			if (*(tagsubstr+taglen - id)==*(srcstr+srclen - id))
				continue;
			
			return 0;
		}
	}
	
	return 1;
}

char *Upper(char *str)
{
    char c;
    char tmp[1024];
    int dd=0,i=0;
    
    strcpy(tmp,str);
    
    dd=(int)strlen(tmp);
    for(i=0;i<dd;i++)
    {
        c=tmp[i];
        if (c>=97&&c<=122)
        {
            c = c - 32;
            tmp[i]=c;
        }
    }
    
    strcpy(str,tmp);
    return str;
}

/***********************************基本操作函数*****************************************/
/*取得系统时间*/
char * gettime(char *strdate)
{
	char cdate[20];
	time_t d;
    struct tm *t;
    
    memset(cdate,0,20);
    d=time(NULL);
    t=localtime(&d);
    strftime(cdate,20,"%Y%m%d%H%M%S",t);
    strcpy(strdate,cdate);
    return strdate;
}

/*对24小时制的14位的时间进行秒级别的相加减,sd_val必须小于60*60*/
char *AddTime(char *strdate,int sd_val)
{
	char sdate[16];
	char tmp[12];
	
	int hour,minues,second;
	int forwd;
	
	if (sd_val>=60*60||sd_val<=-60*60)
	{
		strcpy(strdate,"");
		return NULL;
	}
	
	strcpy(sdate,strdate);
	sscanf(sdate,"%8s%2d%2d%2d",tmp,&hour,&minues,&second);
	
	forwd=(second+sd_val)/60;
	second=(second+sd_val)%60;
	
	minues=minues+forwd;
	if (second<0)
	{
		minues--;
		second=second+60;
	}
	forwd=minues/60;
	minues=minues%60;
	hour=hour+forwd;
	if (minues<0)
	{
		hour--;
		minues=minues+60;
	}
	
	forwd=hour/24;
	hour=hour%24;
	if (hour<0)
	{
		forwd--;
		hour=hour+24;
	}
	
	if (forwd!=0)
	{
		AddDate(tmp,forwd);
	}
	sprintf(strdate,"%s%02d%02d%02d",tmp,hour,minues,second);
	return strdate;
}

char *AddDate(char *srcdate,int num)
{
	int d_month;
	int d_year;
	int d_date;
    int d_monthdaynum;
	
	char s_time[20];
	
	if (num>30||num<-30)
		return NULL;
	strcpy(s_time,srcdate);
	
	s_time[8]=0;
	
	sscanf(s_time,"%4d%2d%2d",&d_year,&d_month,&d_date);
    if (((d_year%4 == 0)&&(d_year%100 != 0))||(d_year%400 == 0))
    {
        d_monthdaynum=29;
    }
    else
    {
        d_monthdaynum=28;
    }
    
	if(d_month==1)
	{
		if (d_date+num>31)
		{
			d_date=d_date+num-31;
			d_month=2;
            if (d_date>d_monthdaynum)
            {
                d_date=d_date-d_monthdaynum;
                d_month=3;
            }
		}
		else if(d_date+num<1)
		{
			d_year--;
			d_month=12;
			d_date=31+d_date+num;
		}
		else
		{
			d_date=d_date+num;
		}
	}
    else if(d_month==2)
    {
        if (d_date+num>d_monthdaynum)
		{
			d_date=d_date+num-d_monthdaynum;
			d_month=d_month+1;
		}
		else if(d_date+num<1)
		{
			d_month=d_month-1;
			d_date=31+d_date+num;
		}
		else
		{
			d_date=d_date+num;
		}
    }
	else if(d_month==4||d_month==6||d_month==9||d_month==11)
	{
		if (d_date+num>30)
		{
			d_date=d_date+num-30;
			d_month=d_month+1;
		}
		else if(d_date+num<1)
		{
			d_month=d_month-1;
			d_date=31+d_date+num;
		}
		else
		{
			d_date=d_date+num;
		}
	}
    else if(d_month==3)
    {
        if (d_date+num>31)
		{
			d_date=d_date+num-31;
			d_month=d_month+1;
		}
		else if(d_date+num<1)
		{
            
			d_month=d_month-1;
			d_date=d_monthdaynum+d_date+num;
            if (d_date<1)
            {
                d_date=31+d_date;
                d_month=d_month-1;
            }
		}
		else
		{
			d_date=d_date+num;
		}
    }
	else if(d_month==5||d_month==7||d_month==10)
	{
		if (d_date+num>31)
		{
			d_date=d_date+num-31;
			d_month=d_month+1;
		}
		else if(d_date+num<1)
		{
            
			d_month=d_month-1;
			d_date=30+d_date+num;
		}
		else
		{
			d_date=d_date+num;
		}
	}
	else if (d_month==8)
	{
		if (d_date+num>31)
		{
			d_date=d_date+num-31;
			d_month=d_month+1;
		}
		else if(d_date+num<1)
		{
			d_month=d_month-1;
			d_date=31+d_date+num;
		}
		else
		{
			d_date=d_date+num;
		}
	}
	else
	{
		if (d_date+num>31)
		{
			d_year++;
			d_date=d_date+num-31;
			d_month=1;
		}
		else if(d_date+num<1)
		{
			d_month=d_month-1;
			d_date=30+d_date+num;
		}
		else
		{
			d_date=d_date+num;
		}
	}
	sprintf(srcdate,"%4d%02d%02d",d_year,d_month,d_date);
	return srcdate;
}


/*写日志*/
int writelog(char *msg,int mode)
{
    
    FILE * fp;
    int error_no;
    char filename[256];
    char errmsg[4096];
    
    gettime(filename);
    sprintf(errmsg,"时间%s : %s\n",filename,msg);
    
    if (mode==1)
    {
        printf(errmsg);
    }
    else
    {
        strcpy(filename,"gentradetomsg.log");
        if ((fp=fopen(filename,"a+"))==NULL)
 		{
            error_no=errno;
            printf("open %s error! \n errormessage: %d\n",filename,error_no);
            return -1;
 		}
 		else
 		{
            fseek(fp,0L,SEEK_END);
            fputs(errmsg,fp);
            fclose(fp);
 		}
 		if (mode==3)
 		{
 			printf(errmsg);
 		}
    }
    return 1;
}

int GetKeyDatabyFile(char *keyid,char *keycode,char *data,char *filename)
{
    char lenmsg[2048];
    char sKeycode[2048],sKeyid[2048];
    int error_no,id=0;
    char *ret_val=NULL;
    FILE *fp;
    char *tmp=NULL;
    
    int noid=0;
    
	
    if (access(filename,R_OK)!=0)
    {
        printf("文件不存在或不能被读！！\n");
        return 0;
    }
    
    if ((fp=fopen(filename,"r"))==NULL)
    {
        error_no=errno;
        printf("打开文件错误！错误代码：%d",error_no);
        return 0;
    }
    sprintf(sKeyid,"[%s]",keyid);
    fseek(fp,0L,SEEK_SET);
    memset(lenmsg,0,2048);
    ret_val=fgets(lenmsg,2048,fp);
    while(lenmsg[0]!='\n'&&strlen(lenmsg)>1&&ret_val!=NULL)
    {
        
        if (lenmsg[strlen(lenmsg)-1]=='\n')
            lenmsg[strlen(lenmsg)-1]=0;
        
        if (strcmp(sKeyid,lenmsg)==0)
        {
            while(strlen(lenmsg)>1&&ret_val!=NULL)
            {
                memset(lenmsg,0,2048);
                ret_val=fgets(lenmsg,2048,fp);
                if (lenmsg[strlen(lenmsg)-1]=='\n')
                    lenmsg[strlen(lenmsg)-1]=0;
                if ((tmp=strstr(lenmsg,"="))==NULL)
                {
         			fclose(fp);
         			return 0;
                }
                if (strstr(lenmsg,keycode)==NULL)
                {
                    continue;
                }
                for(id=0;;id++)
                {
                    if (*(tmp+1+id)==32||*(tmp+1+id)==9)
                        continue;
                    
                    sKeycode[noid]=*(tmp+1+id);
                    noid++;
                    if (*(tmp+1+id)==0)
                        break ;
                }
                strcpy(data,sKeycode);
                fclose(fp);
                return 1;
            }
        }
        memset(lenmsg,0,2048);
        ret_val=fgets(lenmsg,2048,fp);
    }
    fclose(fp);
    return 0;
}

int WriteIni(char * filename, char *sname, char *pwd)
{
	char username[128];
    char password[128];
    char tmpbuf[1024];
    int error_no;
    int d_num;
    char c;
    FILE * fp=NULL;
    
    strcpy(username,"");
    strcpy(password,"");
    d_num=0;
    
    printf("\n数据库用户名: ");
    fflush(stdin);
    while (1)
    {
        c = getchar();
        if (c=='\n')
        {
            username[d_num]='\0';
            break ;
        }
        username[d_num]=c;
        d_num++;
    }
    printf("\n数据库用户密码: ");
    d_num=0;
    while (1)
    {
        c = getchar();
        if (c=='\n')
        {
            password[d_num]='\0';
            break ;
        }
        password[d_num]=c;
        d_num++;
    }
    if ((fp=fopen(filename,"w"))==NULL)
    {
        error_no=errno;
        printf("打开文件错误！错误代码：%d",error_no);
        return 0;
    }
    strcpy(sname,username);
    strcpy(pwd,password);
    fputs("[GETPAYDATA]\n",fp);
    sprintf(tmpbuf,"username= %s\n",username);
    fputs(tmpbuf,fp);
    Encrypt(password);
    sprintf(tmpbuf,"passwd= %s\n",password);
    fputs(tmpbuf,fp);
    fclose(fp);
    return 1;
}

int Encrypt(char *paswd)
{
    char tmp[256];
    char newpaswd[256];
    int d_c=0,d_h=0;
    int d_pos=0,i=0;
    
    strcpy(tmp,paswd);
    strcpy(newpaswd,"");
    for(i=1;i<=strlen(tmp);i++)
    {
        d_c=tmp[i-1];
        d_c=d_c+i*2;
        d_h=1+d_c/128;
        d_c=d_c%128;
        if (d_c==0)
        {
            d_c=128;
            d_h=1;
        }
        
        newpaswd[d_pos]=d_c;
        d_pos++;
        newpaswd[d_pos]=d_h;
        d_pos++;    
    }
    newpaswd[d_pos]='\0';
    strcpy(paswd,newpaswd);
    return 1;
}

int Unencrypt(char *paswd)
{
    char tmp[256];
    char newpaswd[256];
    int d_c=0,d_h=0;
    int d_pos=0,i=0;
    
    strcpy(tmp,paswd);
    strcpy(newpaswd,"");
    for(i=1;i<strlen(tmp);)
    {
        d_c=tmp[i-1];
        i++;
        d_h=tmp[i-1];
        d_c=d_h-1+d_c-i;
        newpaswd[d_pos]=d_c;
        i++;
        d_pos++;
    }
    newpaswd[d_pos]='\0';
    strcpy(paswd,newpaswd);
    return 1;
}

