//
//  iso8583_conf.c
//  iPrepose
//
//  Created by Freax on 12-12-20.
//  Copyright (c) 2012年 Chinaebi. All rights reserved.
//

#include "unixhdrs.h"
#include "ibdcs.h"
#include "iso8583.h"
#define MAXSZ 100

/*
 2.1.	int IsoLoad8583config(struct ISO_8583 *pIso8583Bit ,char *pFile )
 Include ：iso8583.h
 功能描述：装载8583配置文件。
 参数说明：
 pIso8583Bit：存放iso8583配置数据存储区地址
 pFile：配置文件绝对路径
 返 回 值：0成功，－1失败。
 */
int IsoLoad8583config(struct ISO_8583 *pIso8583Bit ,char *pFile )
{
    char filename[100];
	//sprintf(filename, "%s/run/config/%s", getenv("HOME"), pFile);
	     sprintf(filename, "%s/run/config/%s", getenv("EXECHOME"),pFile);
    
    //Open file
	FILE *fp = fopen(filename, "ra" );

	if( !fp )
	{
        printf("Can not open file %s \n", pFile);        
        return -1;
	}
    
    //Read file
    char   line[4096]; size_t len; int index = 0;
    
    //信息类型(11)     位元(7)       长度类型flag(15)   最大长度len(7)    数据格式fmt(19)  是否压缩type(8)    中文描述(22)
    char info_[MAXSZ],info1_[MAXSZ], flag_[MAXSZ], /*len_,  */  fmt_[MAXSZ],/*type_[MAXSZ],*/ info2_[MAXSZ];
    int len_,type_;
    
    while(fgets(line,sizeof(line),fp)!=NULL)//fgets遇到换行或EOF会结束
    {
        if( (line[0]=='#') || (line[0]=='\n') ) continue;//继续下一循环
        
        len = strlen(line);
        
        if(line[len-1]=='\n') line[len-1]='\0';//in order to form a string
        
        //if( ( (p = strchr(buf,'=')) == NULL) || (p == buf) ) continue; //if not find =
        
        struct ISO_8583 *p_iso = &pIso8583Bit[index];
        
        if (sscanf(line, "%11s%7s%15s%7d%19s%8d%22s", info_, info1_, flag_, &len_,fmt_, &type_, info2_) != 7)
        {            
            dcs_log(0, 0, "Line '%s' didn't scan properly\n", line);
            return - 1;
        }
                
        if (p_iso == NULL) { 
            dcs_log (0, 0, "Ran out of memory\n");
            return - 1;
        }
        
        p_iso->len = len_;
        p_iso->type = type_;
        
        if (strcmp(flag_, "LEN_fixed") == 0) {
            p_iso->flag =  LEN_FIXED;
        } else if (strcmp(flag_, "LEN_LLVar") == 0){
            p_iso->flag =  LEN_LLVAR;
        } else if (strcmp(flag_, "LEN_LLLVar") == 0){
            p_iso->flag =  LEN_LLLVAR;
        }
        
        
        if (strcmp(fmt_, "FMT_B") == 0) {
            p_iso->fmt =  FMT_B;
        } else if (strcmp(fmt_, "FMT_N") == 0){
            p_iso->fmt =  FMT_N;
        } else if (strcmp(fmt_, "FMT_MMDDHHMMSS") == 0){
            p_iso->fmt =  FMT_MMDDHHMMSS;
        } else if (strcmp(fmt_, "FMT_MMDD") == 0){
            p_iso->fmt =  FMT_MMDD;
        } else if (strcmp(fmt_, "FMT_Z") == 0){
            p_iso->fmt =  FMT_Z;
        } else if (strcmp(fmt_, "FMT_AN") == 0){
            p_iso->fmt =  FMT_AN;
        } else if (strcmp(fmt_, "FMT_ANS") == 0){
            p_iso->fmt =  FMT_ANS;
        }
        
        index ++;
    }
    
    fclose(fp);
    
    return 0;
}
