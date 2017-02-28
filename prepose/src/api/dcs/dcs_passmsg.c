#include "ibdcs.h"
#include "extern_function.h"

int Forward_To_Switch(void *msg, int nbytes)
{
    char   caBuffer[MAX_IBDCS_MSG_SIZE];
    int    nwritten = 0, n;
    struct IBDCSPacket *pktptr;
    static int gs_comfid = -1;
            
    // locate folder id of DcsComm

    if( gs_comfid < 0) gs_comfid = fold_locate_folder(DCSCOM_FOLD_NAME);
    
    if( gs_comfid < 0)
    {
        dcs_log(0,0,"locate folder '%s' failed.\n",DCSCOM_FOLD_NAME);    
        return -1;                
    }                    
    
    memset(caBuffer, 0, sizeof(caBuffer));
    pktptr = (struct IBDCSPacket *)caBuffer;
    if( nbytes > sizeof(caBuffer) - sizeof(struct IBDCSPacket) + 1)
        nbytes = sizeof(caBuffer) - sizeof(struct IBDCSPacket) + 1;
    memcpy( pktptr->pkt_buf, msg, nbytes);
    pktptr->pkt_bytes = nbytes;
    pktptr->pkt_cmd   = PKT_CMD_DATATOSWITCH;
    
    //forward this message to DcsComm who will send all data to switch
    //for us
    n = nbytes + sizeof(struct IBDCSPacket) - 1;
    nwritten = fold_write( gs_comfid, -1, pktptr, n);
    if(nwritten < 0)
    {
        dcs_log(0,0,"fold_write() failed:%s!\n",strerror(errno));
        return -1;
    }

    dcs_debug(pktptr,nwritten,"write DcsComm %.4d bytes data!\n",nwritten);
    return 0;    
}

int Forward_To_ECR(int comfid, void *msg, int nbytes)
{
    char   caBuffer[MAX_IBDCS_MSG_SIZE];
    int    nwritten = 0, n;
    struct IBDCSPacket *pktptr;
            
    memset(caBuffer, 0, sizeof(caBuffer));
    pktptr = (struct IBDCSPacket *)caBuffer;
    if( nbytes > sizeof(caBuffer) - sizeof(struct IBDCSPacket) + 1)
        nbytes = sizeof(caBuffer) - sizeof(struct IBDCSPacket) + 1;

    memcpy( pktptr->pkt_buf, msg, nbytes);
    pktptr->pkt_bytes = nbytes;
    pktptr->pkt_cmd   = PKT_CMD_DATAFROMSWITCH;
    
    //forward this message to DcsComm who will send all data to switch
    //for us
    n = nbytes + sizeof(struct IBDCSPacket) - 1;
    nwritten = fold_write( comfid, -1, pktptr, n);
    if(nwritten < 0)
    {
        dcs_log(0,0,"fold_write() failed:%s!\n",strerror(errno));
        return -1;
    }

    dcs_debug(pktptr,nwritten,"write DcsComm %.4d bytes data!\n",nwritten);
    return 0;    
}

