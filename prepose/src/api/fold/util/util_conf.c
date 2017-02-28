//
//  util_conf.c
//  iPrepose
//
//  Created by Freax on 12-12-19.
//  Copyright (c) 2012年 Chinaebi. All rights reserved.
//

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#define BUF 2048

/*
 Symbols from util_conf.o:
 
 [Index]     Value      Size      Type  Bind  Other Shname         Name
 
 [7]       |2696      |60        |FUNC |LOCL |0    |.text         |IsCommentLine
 [8]       |2632      |61        |FUNC |LOCL |0    |.text         |IsEmptyLine
 [9]       |2144      |488       |FUNC |LOCL |0    |.text         |conf_get_nextline
 [10]      |2836      |75        |FUNC |LOCL |0    |.text         |conf_find_impl
 [11]      |0         |128       |OBJT |LOCL |0    |.bss          |gs_conf_tbl
 [12]      |2756      |79        |FUNC |LOCL |0    |.text         |conf_init_impl
 [13]      |0         |4         |OBJT |LOCL |0    |.data         |gs_fInitialized
 
 [42]      |0         |0         |NOTY |GLOB |0    |UNDEF         |getpid
 [43]      |0         |0         |NOTY |GLOB |0    |UNDEF         |isspace
 [44]      |2912      |73        |FUNC |GLOB |0    |.text         |strip_trailing_comments
 
 [49]      |1896      |245       |FUNC |GLOB |0    |.text         |conf_get_next_string
 [50]      |1512      |124       |FUNC |GLOB |0    |.text         |conf_get_next_number
 

 [52]      |1636      |257       |FUNC |GLOB |0    |.text         |conf_get_first_string
 [53]      |1388      |124       |FUNC |GLOB |0    |.text         |conf_get_first_number
 
 [55]      |1324      |61        |FUNC |GLOB |0    |.text         |conf_close
 
 [56]      |0         |0         |NOTY |GLOB |0    |UNDEF         |fcntl
 [57]      |0         |0         |NOTY |GLOB |0    |UNDEF         |fileno
 [58]      |0         |0         |NOTY |GLOB |0    |UNDEF         |fopen
 [59]      |0         |0         |NOTY |GLOB |0    |UNDEF         |errno
 [60]      |1148      |173       |FUNC |GLOB |0    |.text         |conf_open
 
 */

extern int get_line(int fid, char *line_buf);

/*从字符串的右边截取n个字符*/
char * substring_from_right(char *dst,char *src, int n)
{
    char *p = src;
    char *q = dst;
    int len = strlen(src);
    if(n>len) n = len;
    p += (len-n);   /*从右边第n个字符开始*/
    while(*(q++) = *(p++));
    return dst;
}

/*
 *	将字符串开头与最后(左右两边)的空格符截去.
 */
char * zut_trunc_lr_space(char	*str)
{
	int	i;
	char	*s1, *s2;

	if (str == NULL)
		return("\0");
	s1 = str;
	for (i = 0; i < (int)strlen(str); i++) {
		if (*s1 != ' ')
			break;
		s1++;
	}
	s2 = &str[(int)strlen(str)-1];
	for (i = 0; i < (int)strlen(str); i++) {
		if (*s2 != ' ')
			break;
		s2--;
	}
	*(s2+1) = '\0';
	return(s1);
}

/*
 * 打开配置文件
 */
int  conf_open(const char *filename)
{    
    int fld_id = -1;
    fld_id = open(filename, O_RDONLY);    
    
    return fld_id;
}

/*
 * 关闭配置文件
 */
int  conf_close(int handle)
{
    if(close(handle) < 0){
        //printf("conf_close error: %s.\n",sys_errlist[errno]);
        return -1;
    }    
    return 0;
}


/*
 * 读取一行
 */

int  conf_get_first_number(int fid, const char *key, int *out_value)
{
    char out_string[512];
	char out_val_buf[10];
    int rtn;
	
	memset(out_string, 0, sizeof(out_string));
	memset(out_val_buf, 0, sizeof(out_val_buf));
	
    rtn = get_line(fid, out_string);
	while(1)
	{
		if (rtn == EOF)
			goto END;
		if (memcmp(out_string, "#", 1) == 0)
		{
            rtn = get_line(fid, out_string);
			continue;
		}
        
        if (strstr(out_string, key))
		{          
            if (sscanf(out_string, "%*s%s", out_val_buf) == 1)
            {
                *out_value = atoi(out_val_buf);
                return 0;
            }
        }      
        rtn = get_line(fid, out_string);
    }
    return -1;   
END:
    return -1;
}

int  conf_get_next_number(int fid, const char *key, int *out_value)
{  
    char out_string[512];
	char out_val_buf[10];
    int rtn;
	
	memset(out_string, 0, sizeof(out_string));
	memset(out_val_buf, 0, sizeof(out_val_buf));
	
    rtn = get_line(fid, out_string);
	while(1)
	{
		if (rtn == EOF)
			goto END;
		if (memcmp(out_string, "#", 1) == 0)
		{
            rtn = get_line(fid, out_string);
			continue;
		}  
        if (strstr(out_string, key))
		{
            if (sscanf(out_string, "%*s%s", out_val_buf) == 1)
            {
                *out_value = atoi(out_val_buf);
                return 0;
            }
        } 
        rtn = get_line(fid, out_string);
    }
    return -1;
    
END:
    return -1;
}

/*
 * 读取一条字符串
 */
int  conf_get_first_string(int fid, const char *key, char *value)
{
    int rtn;
	char *tmp_buf = NULL;
	char out_string[512];
	
	memset(out_string, 0, sizeof(out_string));
	
    rtn = get_line(fid, out_string);
	while(1)
	{
		if (rtn == EOF)
			goto END;
		if (memcmp(out_string, "#", 1) == 0)
		{
            rtn = get_line(fid, out_string);
			continue;
		} 
        if (strstr(out_string, key))
		{
			tmp_buf = strtok(out_string, "#");  
			substring_from_right(value, tmp_buf, strlen(tmp_buf) - strlen(key));
			char *dst_str = zut_trunc_lr_space(value);
			memcpy(value, dst_str, strlen(dst_str));
			value[strlen(dst_str)] = '\0';
            return 0;
        }
        rtn = get_line(fid, out_string);
    }
    return -1;  
END:
    return -1;
}

int  conf_get_next_string(int fid, const char *key, char *value)
{
    int rtn;
	char *tmp_buf = NULL;
	char out_string[512];
	memset(out_string, 0, sizeof(out_string));
	
    rtn = get_line(fid, out_string);
	while(1)
	{
		if (rtn == EOF)
			goto END;
		if (memcmp(out_string, "#", 1) == 0)
		{
            rtn = get_line(fid, out_string);
			continue;
		}
       
        if (strstr(out_string, key))
		{
			tmp_buf = strtok (out_string, "#");  
			substring_from_right(value, tmp_buf, strlen(tmp_buf) - strlen(key));
			char *dst_str = zut_trunc_lr_space(value);
			memcpy(value, dst_str, strlen(dst_str));
			value[strlen(dst_str)] = '\0';
            return 0;
        }
         rtn = get_line(fid, out_string);
    }
    return -1;
END:
    return -1;
}

int get_line(int fid, char *line_buf)
{
	int rtn = 0;
	char ch;
	int i = 0;
    
	rtn = (int)read(fid, &ch, 1);
	while((ch != '\n') && (rtn != 0))
	{
		line_buf[i] = ch;
		i++;
		rtn = (int)read(fid, &ch, 1);
	}
	if(rtn == 0)
		return(EOF);
	else
	{
		line_buf[i] = '\0';
		return(i);
	}
}
