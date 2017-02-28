//
//  util_misc.h
//  iPrepose
//
//  Created by Freax on 12-12-19.
//  Copyright (c) 2012年 Chinaebi. All rights reserved.
//

#ifndef iPrepose_util_misc_h
#define iPrepose_util_misc_h

int  u_fabricatefile(const char *file_name,char *out_buffer,int nsize);
int  u_daemonize(const char *directory);
int  u_stricmp(char *s1,char *s2);
char *LPAD(char *src, char fillchar, int width);      //左填充
char *RPAD(char *src, char fillchar, int width);      //右填充

#endif
