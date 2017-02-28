//
//  util_common.c
//
//  Created by Freax
//  Copyright (c) Chinaebi. All rights reserved.
//

/*
 * 公共的工具函数库
 */
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

/*
 * 分割形如xxx:xxx的字符串
 * 参数:
 *      1. ip_with_port_str 形如xxx:xxx的字符串
 *      2. dst_ip 分割出来要赋值的字符buf
 *      3. dst_port 分割出来要赋值的字符buf
 * 返回值:
 *      无.
 */
void util_split_ip_with_port_address(char *ip_with_port_str, char *dst_ip, char*dst_port){
    char ip_buf[256];
    memset(ip_buf, 0x00, sizeof(ip_buf));
    memcpy(ip_buf, ip_with_port_str, strlen(ip_with_port_str));
    //char delims[] = ":";
    char *result = NULL;
    result = strtok( ip_buf, ":" );
    if( result != NULL )
    {
         memcpy(dst_ip, result, strlen(result));
        result = strtok( NULL, ":" );
	if( result != NULL )
       {
           memcpy(dst_port, result, strlen(result));
       }
    }
}


