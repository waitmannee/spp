//
//  util_misc.c
//  iPrepose
//
//  Created by Freax on 12-12-19.
//  Copyright (c) 2012年 Chinaebi. All rights reserved.
//

#include <stdio.h>
#include "util_misc.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/*
 Symbols from util_misc.o:
 
 [Index]     Value      Size      Type  Bind  Other Shname         Name
 
 [33]      |1984      |49        |FUNC |GLOB |0    |.text         |u_filesize
 [34]      |1904      |78        |FUNC |GLOB |0    |.text         |RPAD

 [36]      |1816      |88        |FUNC |GLOB |0    |.text         |LPAD

 [38]      |1752      |64        |FUNC |GLOB |0    |.text         |u_sleep_us
 [39]      |1392      |359       |FUNC |GLOB |0    |.text         |u_stricmp
 [46]      |1288      |102       |FUNC |GLOB |0    |.text         |u_daemonize
  
 [48]      |1148      |140       |FUNC |GLOB |0    |.text         |u_fabricatefile
 
  */

int  u_fabricatefile(const char *file_name, char *out_buffer,int nsize)
{
	char tmp[256];
	memset(tmp, 0, sizeof(tmp));
	sprintf ( tmp, "%s/run/%s", getenv("EXECHOME"),file_name );
	if (strlen(tmp) > nsize)
		return -1;
	strcpy ( out_buffer, tmp);
    
    return 0;
}

/*
 * 守护进程
 */
int  u_daemonize(const char *directory)
{
    return -1;
}

int  u_stricmp(char *s1,char *s2)
{
    return strcmp(s1,s2);
}

/*
 * 左填充
 */
char *LPAD(char *src, char fillchar, int width)
{
    return "";
}

/*
 * 右填充
 */
char *RPAD(char *src, char fillchar, int width)
{
    return "";
}
//睡眠一段时间,单位为微秒(百万分之一秒)
void u_sleep_us(int time){
    usleep(time);
}
