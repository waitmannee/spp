#include "ibdcs.h"

//
// this C file containes the definitions of all global 
// variables used in IBDCS system
//

IBDCSSHM         *g_pIbdcsShm  =NULL;
IBDCS_CFG        *g_pIbdcsCfg  =NULL;
IBDCS_STAT       *g_pIbdcsStat =NULL;
TBT_AREA         *g_pTimeoutTbl=NULL;
unsigned long     g_seqNo      =0;
