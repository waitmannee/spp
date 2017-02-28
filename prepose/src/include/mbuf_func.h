#ifndef _MBUF_H_
#define _MBUF_H_

#include "ipc_func.h"

//
// structure for control block of the buffer-styled
// shared memory 
//
typedef  int _sem_t;

typedef struct _tagShmCtrl
{
    char   shm_name[8];  //name of shared memory
    int    shm_id;       //id of shared memory
    long   shm_magic;    //a magic number,always is SHM_MAGIC
    _sem_t  shm_sem;     //the semaphore associated with memory 
                         //for mutex
//resources:                         
    int    hdr_nalloc;   //how many slots in header array
    int    hdr_nfree;    //how many headers in free header link
    int    buf_nalloc;   //how many slots in buffer array
    int    buf_nfree;    //how many buffers in free buffer link
    int    buf_nsize;    //the space of each buffer
    
//free link pointers:    
    int    hdr_freehead; //pointer to the head of free header link
    int    hdr_freetail; //pointer to the tail of free header link
    int    buf_freehead; //pointer to the head of free buffer link
    int    buf_freetail; //pointer to the tail of free buffer link

//statistics:
    int    shm_alloctimes;    //times of successful allocation
    int    shm_allocfaults;   //times of failed alloction
    int    shm_freetimes;     //times of successful dellocation
    int    shm_statArray[101];//statistics arrays
} SHMCTRL;

#define SHM_MAGIC   0xABCDEF98    

typedef struct _tagShmHdr
{
    int   hdr_id;
    int   hdr_next;
    int   hdr_buf0;
    int   hdr_nbuf;
    int   hdr_nbyte;
    int   hdr_flags;
} SHMHDR;

#define  SLOT_USED    0x00000001

//
//structure for each buffer in buffer-styled
//shared memory
//
typedef struct _tagShmBuf
{
    int   buf_id;
    int   buf_next;
    int   buf_nbyte;
} SHMBUF;


//
//structure for identifying buffer-styled
//shared memory,which is called a "memory object"
//
typedef struct _tagShmObj
{
    SHMCTRL  *shm_pShmCtrl;
    SHMHDR   *shm_pHdrArray;
    SHMBUF   *shm_pBufArray;
    char     *shm_pDataArea;
} SHMOBJ;

//
// internal helpers invisible to outer
//
extern int mbuf_lock(const SHMOBJ *shmobj);
extern int mbuf_unlock(const SHMOBJ *shmobj);

//
// interfaces functions:
//
extern int mbuf_create(SHMOBJ *shmobj,const char *name,
                int nheader,int nbuffer,int nbufsize);
extern int mbuf_connect(SHMOBJ *shmobj,const char *name);
extern int mbuf_delete(const SHMOBJ *shmobj);

extern int mbuf_alloc(const SHMOBJ *shmobj,int nsize);
extern int mbuf_free(const SHMOBJ *shmobj,int cluster_id);

extern int mbuf_putdata(const SHMOBJ *shmobj,int cluster_id,
                 const void *data, int nbyte);
extern int mbuf_countbytes(const SHMOBJ *shmobj,int cluster_id);
extern int mbuf_getdata(const SHMOBJ *shmobj,int cluster_id,
                 void *buffer, int nsize);

extern int mbuf_view_ctrlblk(const SHMOBJ *shmobj, FILE *fp_out);
extern int mbuf_view_link(const SHMOBJ *shmobj, FILE *fp_out);
extern int mbuf_view_statistics(const SHMOBJ *shmobj, FILE *fp_out);     

#endif //_MBUF_H_

