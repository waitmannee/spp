#include "ibdcs.h"

int dcs_create_shm(int tblrows)
{
    char *ptr = NULL;
    int  size, shmid = -1, semid = -1;
    int  i;
            
    //calculate the size and try to create SHM
    if(tblrows <= 0)   tblrows = 1;
    
    size = sizeof(IBDCSSHM) + sizeof(TBT_AREA) + (tblrows-1) * sizeof(TBT_BUF);
    
    ptr = shm_create(IBDCS_SHM_NAME, size, &shmid);
    
    dcs_log(0, 0, "shm create IBDCS_SHM_NAME shm_id = %d, memory size = %d", shmid, size);
    
    if(ptr == NULL)
    {
        dcs_log(0,0,"cannot create SHM '%s':%s!\n", IBDCS_SHM_NAME,strerror(errno));
        return -1;
    }
    
    //try to create semaphore
    semid = sem_create(IBDCS_SHM_NAME, 3);
    
    if(semid < 0)
    {
        dcs_log(0,0,"cannot create Semaphore '%s':%s!\n",IBDCS_SHM_NAME, strerror(errno));
        goto lblFailure;
    }
    
    //get all the pointers and do initialization
    memset(ptr, 0, size);        
    g_pIbdcsShm   = (IBDCSSHM *)ptr;
    g_pIbdcsCfg   = &(g_pIbdcsShm->is_config);
    g_pIbdcsStat  = &(g_pIbdcsShm->is_stat);
    
    for(i=0; i < MAX_CHILDREN_NUM; i++)
        g_pIbdcsShm->is_children[i] = -1;
    
    g_pTimeoutTbl = &(g_pIbdcsShm->is_tmTbl);
    
    tm_init_queue( tblrows );
    
    g_pIbdcsShm->is_semid = semid;

    return 0;

lblFailure:
    if(ptr)    
    {
        shm_detach(ptr);
        shm_delete(shmid);
    }
    return -1;    
}

int dcs_connect_shm()
{
    char *ptr = NULL;
    
    ptr = shm_connect(IBDCS_SHM_NAME,NULL);
    if(ptr == NULL)
    {
        dcs_log(0,0,"cannot connect SHM '%s':%s!\n",IBDCS_SHM_NAME, strerror(errno));
        return -1;
    }
    
    //get all the pointers
    g_pIbdcsShm   = (IBDCSSHM *)ptr;
    g_pIbdcsCfg   = &(g_pIbdcsShm->is_config);
    g_pIbdcsStat  = &(g_pIbdcsShm->is_stat);
    g_pTimeoutTbl = &(g_pIbdcsShm->is_tmTbl);
    return 0;
}

int dcs_delete_shm()
{
    IBDCSSHM  *pIbdcsShm = NULL;
    int       shmid;
    
    pIbdcsShm = (IBDCSSHM *)shm_connect(IBDCS_SHM_NAME,&shmid);
    if(pIbdcsShm == NULL)
    {
        dcs_log(0,0,"cannot connect SHM '%s':%s!\n",IBDCS_SHM_NAME, strerror(errno));
        return -1;
    }
    
    //remove the kernel objects
    sem_delete(pIbdcsShm->is_semid);
    shm_delete(shmid);
    shm_detach((char *)pIbdcsShm);
    return 0;
}
