//
//  util_conf.h
//  iPrepose
//
//  Created by Freax on 12-12-19.
//  Copyright (c) 2012年 Chinaebi. All rights reserved.
//

#ifndef iPrepose_util_conf_h
#define iPrepose_util_conf_h


int  conf_open(const char *filename);
int  conf_close(int handle);
int  conf_get_first_number(int handle, const char *key, int *out_value);
int  conf_get_next_number(int handle, const char *key, int *out_value);
int  conf_get_first_string(int handle, const char *key, char *out_string);
int  conf_get_next_string(int handle, const char *key, char *out_string);

#endif
