//
//  ipc_sem.c
//  iPrepose
//
//  Created by Freax on 12-12-19.
//  Copyright (c) 2012年 Chinaebi. All rights reserved.
//

#include "unixhdrs.h"
#include "extern_function.h"

extern int ipc_makekey(const char *name);

/*
 Symbols from ipc_sem.o:
 
 [Index]     Value      Size      Type  Bind  Other Shname         Name
 
 [4]       |1468      |70        |FUNC |LOCL |0    |.text         |sem_getsem
 [33]      |1540      |72        |FUNC |GLOB |0    |.text         |sem_get_info
 
 [35]      |0         |0         |NOTY |GLOB |0    |UNDEF         |ipc_makekey
 [36]      |1440      |28        |FUNC |GLOB |0    |.text         |sem_delete
 [37]      |1364      |75        |FUNC |GLOB |0    |.text         |sem_unlock
 
 [40]      |1268      |96        |FUNC |GLOB |0    |.text         |sem_lock
 [41]      |1240      |27        |FUNC |GLOB |0    |.text         |sem_connect
 [42]      |0         |0         |NOTY |GLOB |0    |UNDEF         |semctl
 
 [43]      |1148      |89        |FUNC |GLOB |0    |.text         |sem_create
 
 */

/*
 5.3.1.	int sem_create(const char *name, int nsem)
 功能描述：该函数创建一个命名的信号量
 参数说明：
 name	该参数为一个指针，指向一个以NULL字符结尾的字符串，用来指定信号量的名称。该字符串只有前4个字符有效，并且对大小写敏感。
 nsem	该参数给出待建立的信号量含有的分量数目
 返 回 值：该函数成功时返回新建信号量的ID；否则返回-1，errno指明具体的错误原因。
 说    明：
 ?	sem_create()建立的信号量为二元信号量，即信号量的每个分量都只能取值1和0，因此这里的信号量主要用来实现互斥(mutex)；
 ?	sem_create()建立一个系统中尚未存在的信号量，若系统中已存在给定名字的消息队列，则该函数失败，同时errno为EEXIST。
 */
int sem_create(const char *name, int nsem)
{
	key_t key =  ipc_makekey(name) ;

	int semid = -1;

	if(( semid = semget(key,nsem,0666|IPC_CREAT))==-1)
	{
		//perror("semget");
		return -1;
	}

	union semum arg;
	arg.val = 1;

	if (semctl(semid, 1, SETVAL, arg) == -1)
	{
		//printf("sem init error");
		if(semctl(semid,0,IPC_RMID,0)!=0)
		{
			perror("semctl");
			return -1;
		}
		return -1;
	}

	return semid;
}

/*
 5.3.2.	int sem_connect(const char *name, int nsems)
 功能函数：该函数打开一个已经存在的命名信号量
 参数说明：
 name	该参数为一个指针，指向一个以NULL字符结尾的字符串，用来指定消息队列的名称。该字符串只有前4个字符有效，并且对大小写敏感。
 nsems	该参数给出信号量含有的分量数目
 返 回 值：该函数成功时返回信号量的ID；否则返回-1，errno指明具体的错误原因。
 说    明：sem_connect()打开系统中已经存在的信号量，若系统中不存在给定名字的信号量，则该函数失败，同时errno为ENOENT。
 */
int sem_connect(const char *name, int nsems)
{
	int semid = -1;
	int sem_key = ipc_makekey(name);
	if(( semid = semget(sem_key,nsems,0666|IPC_EXCL))==-1)
	{
		//perror("semget sem error \n");
		return -1;
	}

	return  semid;
}

/*
 5.3.3.	int sem_delete(int semid)
 功能描述：该函数删除给定ID的信号量
 参数说明：
 sehid	信号量的标识，一般为sem_create()或sem_connect()返回值
 返 回 值：该函数成功时返回0；否则返回-1，errno指明具体的错误原因。
 */
int sem_delete(int semid)
{
    if(semctl(semid,0,IPC_RMID,0)!=0)
    {
        //perror("semctl");
        return -1;
    }
    return  0;
}

/*
 5.3.4.	int sem_lock(int semid, int semnum, int lockflg)
 功能描述：该函数对信号量中的指定分量加锁，以实现独占访问
 参数说明：
 semid	   信号量的ID，一般为sem_create()或sem_connect()的返回值
 semnum	   该参数指定对信号量数组中的哪个分量加锁: ->1: Full;
 lockflag  加锁操作标志。
           当lockflag=1时，函数进行阻塞加锁操作，即如果此时有别的进程已经对信号量(semid,semnum)加上锁，则调用进程阻塞；
           当lockflag=0时进行非阻塞加锁操作
 返 回 值：该函数成功时返回0；否则返回-1，errno指明具体的错误原因。
 说    明：
 ?	调用sem_lock()之前，调用进程必须尚未拥有对信号量(semid,semnum)的独占锁，否则会导致调用进程死锁
 */
int sem_lock(int semid, int semnum, int lockflg)
{
    struct sembuf sb;
    sb.sem_num = semnum;
    sb.sem_op = -1;
    
    if (lockflg) {
        sb.sem_flg = SEM_UNDO;
    } else {
        sb.sem_flg = IPC_NOWAIT;
    }
    
    int retval = -1;
    if((retval = semop(semid,&sb,1)) == -1)
    {
        //printf("semop error: %s.\n",sys_errlist[errno]);
    }
    return retval;
}


/*
 5.3.5.	int sem_unlock(int semid, int semnum, int lockflg)
 功能描述：该函数对信号量中的指定分量解锁
 参数说明：
 semid	   信号量的ID，一般为sem_create()或sem_connect()的返回值
 semnum	   该参数指定对信号量数组中的哪个分量加锁(信号量索引)
 lockflag  加锁操作标志。当lockflag=1时，函数进行阻塞解锁操作；
           当lockflag=0时进行非阻塞解锁操作
 返 回 值：该函数成功时返回0；否则返回-1，errno指明具体的错误原因。
 */
int sem_unlock(int semid, int semnum, int lockflg)
{
    
    struct sembuf sb;
    sb.sem_num = semnum;
    sb.sem_op = 1;
    
    if (lockflg) {
         sb.sem_flg = SEM_UNDO;
    } else {
        sb.sem_flg = IPC_NOWAIT;
    }
    
    int retval = -1;
    if((retval = semop(semid,&sb,1)) == -1)
    {
        //printf("semop error: %s.\n",sys_errlist[errno]);
        return -1;
    }
    return retval;
}

//=====================================================
////////////////////////////// example ////////////////
//=====================================================
/*
#define SHMDATASIZE 1000
#define BUFFERSIZE (SHMDATASIZE-sizeof(int))
#define SN_EMPTY 0
#define SN_FULL 1
int deleteSemid=0;

void server(void);
void client(int shmid);
void delete(void);
void sigdelete(int signum);
void locksem(int semid,int semnum);
void unlocksem(int semid,int semnum);
void waitzero(int semid,int semnum);
void clientwrite(int shmid,int semid,char *buffer);

int safesemget(key_t key,int nsems,int semflg);
int safesemctl(int semid,int semnum,int cmd,union semun arg);
int safesemop(int semid,struct sembuf *sops,unsigned nsops);
int safeshmget(key_t key,int size,int shmflg);
void* safeshmat(int shmid,const void *shmaddr,int shmflg);
int safeshmctl(int shmid,int cmd,struct shmid_ds *buf);

int sem_shm_main(int argc,char *argv[])
{
    if(argc<2)
        server();
    else
        client(atoi(argv[1]));
    return 0;
}

void server(void)
{
    union semun sunion;
    int semid,shmid;
    void *shmdata;
    char *buffer;
    semid = safesemget(IPC_PRIVATE,2,SHM_R|SHM_W);
    deleteSemid = semid;
    atexit(&delete);
    signal(SIGINT,&sigdelete);
    sunion.val = 1;
    safesemctl(semid,SN_EMPTY,SETVAL,sunion);
    sunion.val=0;
    
    safesemctl(semid,SN_FULL,SETVAL,sunion);
    shmid = safeshmget(IPC_PRIVATE,SHMDATASIZE,IPC_CREAT|SHM_R|SHM_W);
    shmdata = safeshmat(shmid,0,0);
    safeshmctl(shmid,IPC_RMID,NULL);
    *(int*)shmdata = semid;
    
    buffer = shmdata + sizeof(int);
    
    printf("Server is running with SHM id ** %d **\n",shmid);
    
    while(1)
    {
        printf("Waiting until full...\n");
        fflush(stdout);
        locksem(semid,SN_FULL);
        printf("done.\n");
        printf("Message received: %s.\n",buffer);
        unlocksem(semid,SN_EMPTY);
    }
}
 
void client(int shmid)
{
    int semid;
    void *shmdata;
    char *buffer;
    shmdata=safeshmat(shmid,0,0);
    semid=*(int*)shmdata;
    buffer=shmdata+sizeof(int);
    printf("Client operational: shm id is %d,sem id is %d\n",shmid,semid);
    while(1)
    {
        char input[3];
        printf("\n\nMenu\n1.Send a message\n");
        printf("2.Exit\n");
        fgets(input,sizeof(input),stdin);
        switch(input[0])
        {
            case '1':
                clientwrite(shmid,semid,buffer);
                break;
            case '2':
                exit(0);
                break;
        }
    }
}
void delete(void)
{
    printf("\nMaster exiting; deleting semaphore %d.\n",deleteSemid);
    if(semctl(deleteSemid,0,IPC_RMID,0)==-1)
        printf("Error releasing semaphore.\n");
}

void sigdelete(int signum)
{
    exit(0);
}

void locksem(int semid,int semnum)
{
    struct sembuf sb;
    sb.sem_num=semnum;
    sb.sem_op=-1;
    sb.sem_flg = SEM_UNDO;
    safesemop(semid,&sb,1);
}

void unlocksem(int semid,int semnum)
{
    struct sembuf sb;
    sb.sem_num = semnum;
    sb.sem_op = 1;
    sb.sem_flg = SEM_UNDO;
    safesemop(semid,&sb,1);
}

void waitzero(int semid,int semnum)
{
    struct sembuf sb;
    sb.sem_num=semnum;
    sb.sem_op=0;
    sb.sem_flg=0;
    safesemop(semid,&sb,1);
}

void clientwrite(int shmid,int semid,char *buffer)
{
    printf("Waiting until empty?-");
    fflush(stdout);
    locksem(semid,SN_EMPTY);
    printf("done.\n");
    printf("Enter Message: ");
    fgets(buffer,BUFFERSIZE,stdin);
    unlocksem(semid,SN_FULL);
}

int safesemget(key_t key,int nsems,int semflg)
{
    int retval;
    if((retval=semget(key,nsems,semflg)) == -1)
    {
        printf("semget error: %s.\n",sys_errlist[errno]);
        exit(254);
    }
    return retval;
}
int safesemctl(int semid,int semnum,int cmd,union semun arg)
{
    int retval;
    if((retval=semctl(semid,semnum,cmd,arg)) == -1)
    {
        printf("semctl error: %s.\n",sys_errlist[errno]);
        exit(254);
    }
    return retval;
}

int safesemop(int semid,struct sembuf *sops,unsigned nsops)
{
    int retval;
    if((retval = semop(semid,sops,nsops)) == -1)
    {
        printf("semop error: %s.\n",sys_errlist[errno]);
        exit(254);
    }
    return retval;
}

int safeshmget(key_t key,int size,int shmflg)
{
    int retval;
    if((retval=shmget(key,size,shmflg)) == -1)
    {
        printf("shmget error: %s.\n",sys_errlist[errno]);
        exit(254);
    }
    return retval;
}

void* safeshmat(int shmid,const void *shmaddr,int shmflg)
{
    int retval;
    if((retval=shmat(shmid,shmaddr,shmflg)) == -1)
    {
        printf("shmat error: %s.\n",sys_errlist[errno]);
        exit(254);
    }
    return (void*)retval;
}

int safeshmctl(int shmid,int cmd,struct shmid_ds *buf)
{
    int retval;
    if((retval=shmctl(shmid,cmd,buf)) == -1)
    {
        printf("shmctl error: %s.\n",sys_errlist[errno]);
        exit(254);
    }
    return retval;
}

*/

