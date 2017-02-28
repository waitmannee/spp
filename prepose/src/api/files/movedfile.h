#ifndef move_data_file_H
#define move_data_file_H

#include "unixhdrs.h"

extern int SerchFile(char *fpath);/*查找需要转移的日志文件，并转移到本机上*/

extern int dealfilelist(); /*当前目录下的文件查询转移*/

extern int dealkerswit(char *curdate); /*处理用户kerswit的日志文件*/

extern int dealpayeasy(char *curdate); /*处理用户 payeasy 的日志文件*/

extern int dealdcsp(char *curdate); /*处理用户 dcsp 的日志文件*/

#endif
