//
//  mbuf_func.c
//  iPrepose
//
//  Created by Freax on 12-12-18.
//  Copyright (c) 2012年 Chinaebi. All rights reserved.
//

#include <stdio.h>
#include "mbuf_func.h"

int mbuf_lock(const SHMOBJ *shmobj)
{
    return 0;
}

int mbuf_unlock(const SHMOBJ *shmobj)
{
    return 0;
}


int mbuf_create(SHMOBJ *shmobj,const char *name,
                       int nheader,int nbuffer,int nbufsize)
{
    int shm_id = -1;
    char *pshm =  shm_connect(name, &shm_id);
    
    if (pshm) {
        shmobj->shm_pShmCtrl->shm_id = shm_id;
    }
    
    return 0;
}

int mbuf_connect(SHMOBJ *shmobj,const char *name)
{
    
    return 0;
}

int mbuf_delete(const SHMOBJ *shmobj)
{
    return 0;
}

int mbuf_alloc(const SHMOBJ *shmobj,int nsize)
{
    
    return 0;
}

int mbuf_free(const SHMOBJ *shmobj,int cluster_id)
{
    return 0;
}

int mbuf_putdata(const SHMOBJ *shmobj,int cluster_id,
                        const void *data, int nbyte)
{
    return 0;
}

int mbuf_countbytes(const SHMOBJ *shmobj,int cluster_id)
{
    return 0;
}

int mbuf_getdata(const SHMOBJ *shmobj,int cluster_id,
                        void *buffer, int nsize)
{
    return 0;
}

int mbuf_view_ctrlblk(const SHMOBJ *shmobj, FILE *fp_out)
{
    return 0;
}

int mbuf_view_link(const SHMOBJ *shmobj, FILE *fp_out)
{
    return 0;
}

int mbuf_view_statistics(const SHMOBJ *shmobj, FILE *fp_out)
{
    return 0;
}

