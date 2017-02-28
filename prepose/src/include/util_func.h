#ifndef  __UTIL__H__
#define  __UTIL__H__

#include "unixhdrs.h"
#include "extern_function.h"

//utilities for profile reading, from util_prof.c or util_conf.c
extern int  conf_open(const char *filename);
extern int  conf_close(int handle);
extern int  conf_get_first_number(int handle, 
            const char *key, int *out_value);
extern int  conf_get_next_number(int handle, 
            const char *key, int *out_value);
extern int  conf_get_first_string(int handle, 
            const char *key, char *out_string);
extern int  conf_get_next_string(int handle, 
            const char *key, char *out_string);

//miscellaneous thing, from util_misc.c
extern int  u_fabricatefile(const char *file_name,
                         char *out_buffer,int nsize);
extern int  u_daemonize(const char *directory); 
extern int  u_stricmp(char *s1,char *s2);                        
extern char *LPAD(char *src, char fillchar, int width);
extern char *RPAD(char *src, char fillchar, int width);
            
//from util_signal.c
extern int  catch_all_signals(sigfunc_t  pfn);
extern int  wait_child_or_signal(int *stat_loc);

//from util_command.c
//typedef int cmdfunc_t(int, char **);
int cmd_init(const char *prompt, sigfunc_t *pFn);
int cmd_add(char *command, cmdfunc_t *pfnHandler, char *prompt);
int cmd_shell(int argc, char *argv[]);

#endif //__UTIL__H__
