//
//  util_command.c
//  iPrepose
//
//  Created by Freax on 12-12-19.
//  Copyright (c) 2012年 Chinaebi. All rights reserved.
//

#include <stdio.h>
#include "unixhdrs.h"
#include "extern_function.h"

#define COMMAND_HELP_STR "help"
#define COMMAND_BYE_STR "quit"

static char _prompt[256];

typedef struct fun_describe
{
    char command[20];
    cmdfunc_t *pfnHandler;
    char prompt[256];
} fun_describe;

struct fun_describe fun_buf[100];
static int fun_inx = 0;

int cmd_init(const char *prompt, sigfunc_t *pFn)
{
    memset(_prompt, 0x00, sizeof(_prompt));
    strcpy(_prompt, prompt);
    return 0;
}

int cmd_add(char *command, cmdfunc_t *pfnHandler, char *prompt)
{
    fun_describe fun;
    strcpy(fun.command, command);
    fun.pfnHandler = pfnHandler;
    strcpy(fun.prompt, prompt);
    fun_buf[fun_inx] = fun;
    fun_inx ++;

    return 0;
}

int get_input_command_idx(char *command){
    int inx = -1;
    int _idx = -1;
    fun_describe *pfun = &fun_buf[0];
    for (; pfun && strlen(pfun->command); pfun ++) {
        inx ++;
        if (strcmp(pfun->command, command) == 0) {
            _idx = inx;
            break;
        }
    }
    return _idx;
} 

void help_fun()
{
    int str_len = 0;
    char help_buf[512];

    memset(help_buf, 0x00, sizeof(help_buf));

    fun_describe *pfun = &fun_buf[0];
    for (; pfun && strlen(pfun->command); pfun ++) {
        sprintf(help_buf + str_len, "%s    ", pfun->command);
        str_len = (int)strlen(help_buf);
    }
    printf("%s\n", help_buf);
}


int cmd_shell(int argc, char *argv[])
{
	int idx = 0;
	char o_input[200];
	char *input=NULL;
	char sBuf[100];
	while (1) 
	{
		system("stty erase ^H");
		//清空stdin
		setbuf(stdin,NULL);
		
		printf("%s", _prompt);
		memset(o_input, 0, sizeof(o_input));
		scanf("%s", o_input);
		//input = strtok(o_input, " ");
		//printf("%s, o_input =[%s]\n", input, o_input);
		if(strcmp(o_input, COMMAND_BYE_STR) == 0)
		{
			break;
		} 
		else if(strcmp(o_input, COMMAND_HELP_STR) == 0)
		{
	
			//清空stdin
			memset(sBuf, 0, sizeof(sBuf));
			fgets(sBuf,sizeof(sBuf),stdin);
			help_fun();
			continue;
		}
		idx = get_input_command_idx(o_input);
		if (idx == -1)
		{
			//清空stdin
			memset(sBuf, 0, sizeof(sBuf));
			fgets(sBuf,sizeof(sBuf),stdin);
			printf("请使用以下命令:\n");
			printf("-------------------\n");
			help_fun();
			printf("\n");
			continue;
		}
		fun_buf[idx].pfnHandler(argc, argv);
	}
	return -1;
}

void util_trim(char	*s)
{
	while (s[strlen(s)-1] == ' ' || s[strlen(s)-1] == '\t')
		s[strlen(s)-1] = '\0';
}



