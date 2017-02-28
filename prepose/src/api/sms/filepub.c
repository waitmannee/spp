#include "filepub.h"
#include "mypublic.h"
#include  <unistd.h>
#include "unixhdrs.h"

struct IniOper *IniHead=NULL;
struct IniOper *IniCur=NULL;
static char Errmsg[1024];

int LoadFile(char *filename)
{
	char lenmsg[1024];
    char sKeycode[512],sItemid[100];
    int error_no,id=0;
    char *ret_val=NULL;
    FILE *fp;
    int d_num=0,i=0;
    
    struct IniOper *opertmp=NULL;
    struct IniOper *opertmpnext=NULL;
    
    struct FileIni *initmp=NULL;
    struct FileIni *initmpnext=NULL;
    
    if (access(filename,R_OK)!=0)
	{
        sprintf(Errmsg,"文件不存在或不能被读！！");
        return -1;
	}
    if ((fp=fopen(filename,"r"))==NULL)
    {
        error_no=errno;
        sprintf(Errmsg,"打开文件错误！错误代码：%d",error_no);
        return -1;
    }
    fseek(fp,0L,SEEK_SET);
    memset(lenmsg,0,1024);
    ret_val = fgets(lenmsg,1024,fp);
    
    while(strlen(lenmsg)>1&&ret_val!=NULL)
    {
        /*去掉行末尾的回车换行*/
        if (lenmsg[strlen(lenmsg)-1]=='\n'||lenmsg[strlen(lenmsg)-1]=='\r') lenmsg[strlen(lenmsg)-1]=0;
        if (lenmsg[strlen(lenmsg)-1]=='\n'||lenmsg[strlen(lenmsg)-1]=='\r') lenmsg[strlen(lenmsg)-1]=0;
        
        Allt(lenmsg);
        if (strcmp(lenmsg,"")==0)
        {
            memset(lenmsg,0,1024);
            ret_val=fgets(lenmsg,1024,fp);
            continue ;
        }
        if (lenmsg[0]=='#')
        {
            memset(lenmsg,0,1024);
            ret_val=fgets(lenmsg,1024,fp);
            continue ;
        }
        d_num++;
        if ((d_num==1)&&(lenmsg[0]!='['||lenmsg[strlen(lenmsg)-1]!=']'))
        {
            sprintf(Errmsg,"%s文件的格式错误！",filename);
            fclose(fp);
            return -1;
        }
        if (lenmsg[0]=='[')
        {
            lenmsg[strlen(lenmsg)-1]=0;
            opertmp = (struct IniOper *)malloc(sizeof(struct IniOper));
            if (opertmp==NULL)
			{
				sprintf(Errmsg,"加载新的项目时，申请内存失败！： LoadFile()");
				fclose(fp);
				return -1;
			}
            opertmp->next=NULL;
            opertmp->fileini=NULL;
            memset(opertmp->termflag,0,MAXITEMID+1);
            for(i=0;i<strlen(lenmsg)&&i<MAXITEMID;i++)
    			opertmp->termflag[i]=lenmsg[i+1];
            if (IniHead==NULL)
            {
                IniHead=opertmp;
                IniCur=IniHead;
                opertmpnext=opertmp;
            }
            else
            {
                opertmpnext->next=opertmp;
                opertmpnext=opertmp;
            }
        }
        else
        {
            if (strstr(lenmsg,"=")==NULL)
            {
                memset(lenmsg,0,1024);
                ret_val=fgets(lenmsg,1024,fp);
                continue ;
            }
            
            initmp = (struct FileIni *) malloc(sizeof(struct FileIni));
            if (initmp==NULL)
			{
				sprintf(Errmsg,"加载子项目时，申请内存失败！： LoadFile()");
				fclose(fp);
				return -1;
			}
			initmp->next=NULL;
			memset(sItemid,0,sizeof(sItemid));
			memset(sKeycode,0,sizeof(sKeycode));
			id=0;
			int valflag=0;
			for(i=0;i<strlen(lenmsg);i++)
			{
				if (lenmsg[i]=='=')
				{
					valflag=1;
					id=0;
					continue;
				}
				if (valflag==0)
				{
					if (id <MAXITEMID)
						sItemid[id]=lenmsg[i];
				}
				else
				{
					if (id <MAXITEMVAL)
						sKeycode[id]=lenmsg[i];
				}
				id++;
			}
			Allt(sItemid);
			Allt(sKeycode);
			strcpy(initmp->itemid,sItemid);
			strcpy(initmp->itemval,sKeycode);
			if (opertmpnext->fileini==NULL)
			{
				opertmpnext->fileini=initmp;
				initmpnext=initmp;
			}
			else
			{
				initmpnext->next=initmp;
				initmpnext=initmp;
			}
        }
        memset(lenmsg,0,1024);
        ret_val=fgets(lenmsg,1024,fp);
    }
    fclose(fp);
	return 1;
}

void Freeini()
{
	struct IniOper * info=NULL;
	struct IniOper * nextinfo=NULL;
	struct FileIni *tmp=NULL;
	struct FileIni *nextini=NULL;
	
	info = IniHead;
	if (info==NULL)
		return ;
	while(info!=NULL)
	{
		tmp=info->fileini;
		while(tmp!=NULL)
		{
			nextini=tmp->next;
			free(tmp);
			tmp=nextini;
		}
		info->fileini=NULL;
		nextinfo=info->next;
		free(info);
		info=nextinfo;
	}
	IniHead=NULL;
}

int GetItemData(char *keyid,char *itemid,char *itemval)
{
	struct IniOper *opertmp=NULL;
    struct FileIni *initmp=NULL;
    
	if (IniHead==NULL)
	{
		sprintf(Errmsg,"ini配置文件还未加载，查找失败！： GetItemData()");
		return -1;
	}
    opertmp=IniHead;
    while(opertmp!=NULL)
    {
        if (strcmp(opertmp->termflag,keyid)==0)
        {
            initmp=opertmp->fileini;
            while(initmp!=NULL)
            {
                if (strcmp(initmp->itemid,itemid)==0)
                {
                    strcpy(itemval,initmp->itemval);
                    return 1;
                }
                initmp = initmp->next;
            }
            sprintf(Errmsg,"未找到查找的子项目：%s！： GetItemData()",itemid);
			return -1;
        }
        opertmp = opertmp->next;
    }
    sprintf(Errmsg,"未找到查找的大项：%s！： GetItemData()",keyid);
	return -1;
}
/*到文件头项*/
void GoIniHead()
{
	IniCur=IniHead;
}

/*指定下一个ini文件配置大项*/
int GONextTerm()
{
	if (IniCur->next==NULL)
	{
		return -1;
	}
	IniCur = IniCur->next;
	return 1;
}

/*得到当前项目组的子项目值*/
int GetIniCurVal( char *itmeval, char *itemid)
{
	struct FileIni *initmp=NULL;
	initmp=IniCur->fileini;
	
	while(initmp!=NULL)
	{
		if (strcmp(initmp->itemid,itemid)==0)
		{
			strcpy(itmeval,initmp->itemval);
			return 1;
		}
		initmp=initmp->next;		
	}
	sprintf(Errmsg,"未找到子项目 %s 对应的值！： GetIniCurVal()",itemid);
	return -1;
}

void GetIniErrMsg(char *msg)
{
	strcpy(msg,Errmsg);
}


