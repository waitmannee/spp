//
//  util_signal.c
//  iPrepose
//
//  Created by Freax on 12-12-19.
//  Copyright (c) 2012年 Chinaebi. All rights reserved.
//

#include <stdio.h>
#include "util_signal.h"
#include <signal.h>
#include <errno.h>
/*
 Symbols from util_signal.o:
 
 [Index]     Value      Size      Type  Bind  Other Shname         Name
 
 
 [32]      |0         |0         |NOTY |GLOB |0    |UNDEF         |waitpid
 [33]      |0         |0         |NOTY |GLOB |0    |UNDEF         |errno
 [34]      |0         |0         |NOTY |GLOB |0    |UNDEF         |wait
 
 [35]      |1316      |88        |FUNC |GLOB |0    |.text         |wait_child_or_signal
 [36]      |1236      |79        |FUNC |GLOB |0    |.text         |catch_all_signals
 
 [37]      |0         |0         |NOTY |GLOB |0    |UNDEF         |sigaction
 [38]      |0         |0         |NOTY |GLOB |0    |UNDEF         |sigemptyset
 
 [39]      |1148      |87        |FUNC |GLOB |0    |.text         |catch_signal
 
  */

int  catch_all_signals(sigfunc_t  pfn)
{
	//留给用户使用10,12
	if(signal(SIGUSR1,pfn) == SIG_ERR)
	{
		return -1;
	}
	if(signal(SIGUSR2,pfn) == SIG_ERR)
	{
		return -1;
	}
	//15,程序结束(terminate)信号, 与SIGKILL不同的是该信号可以被阻塞和处理。
	//通常用来要求程序自己正常退出，shell命令kill缺省产生这个信号。如果进程终止不了，我们才会尝试SIGKILL。
	if(signal(SIGTERM,pfn) == SIG_ERR)
	{
		return -1;
	}
	//信号 11,试图访问未分配给自己的内存, 或试图往没有写权限的内存地址写数据.
	if(signal(SIGSEGV,pfn) == SIG_ERR)
	{
		return -1;
	}
	//stop 20 停止进程的运行, 但该信号可以被处理和忽略. 用户键入SUSP字符时(通常是Ctrl-Z)发出这个信号
	if(signal(SIGTSTP,pfn) == SIG_ERR)
	{
		return -1;
	}

	if(signal(SIGPIPE, SIG_IGN) == SIG_ERR)
	{
		return -1;
	}
	//17,子进程结束时, 父进程会收到这个信号
	//if(signal(SIGCHLD,pfn) == SIG_ERR)
	{
		//return -1;
	}
	
	return  0;
}

int  wait_child_or_signal(int *stat_loc)
{
    return -1;
}


