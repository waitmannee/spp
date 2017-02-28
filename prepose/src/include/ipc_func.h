#ifndef  __IPC__FUNC__H__
#define  __IPC__FUNC__H__

#include "unixhdrs.h"

#ifndef  _UNION_SEMUM
#define  _UNION_SEMUM
    union semum
    {
        int	  val;
        struct semid_ds *buf;
        ushort *array;
    };
#endif  //_UNION_SEMUM

//semaphore
extern int sem_create(const char *name, int nsem);
extern int sem_connect(const char *name, int nsems);
extern int sem_delete(int semid);
extern int sem_lock(int semid, int semnum, int lockflg);
extern int sem_unlock(int semid, int semnum, int lockflg);

//shared memory
extern void *shm_create(const  char *name, size_t size, int *shmid);
extern void *shm_connect(const char *name, int *shmid);
extern int   shm_detach(char *addr);
extern int   shm_delete(int shid);

//message queue
extern int queue_create(const char *name);
extern int queue_connect(const char *name);
extern int queue_delete(int qid);
extern int queue_send(int qid, void *buf, int size, int nowait);
extern int queue_recv(int qid, void *msg, int size, int nowait);
extern int queue_readq_interruptable(int flag);

//make a key from a 'name'
extern int ipc_makekey(const char *name);

//file mapping
extern char *fmap_create(char *mappedfile, int nsize);
extern char *fmap_connect(char *mappedfile, int nsize);
extern int fmap_unmap(char *pAddr, int nsize);

#endif  //__IPC__FUNC__H__
