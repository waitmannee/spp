#ifndef pub_function_h
#define pub_function_h
#include "trans.h"

extern int AmoutLimitDeal(char *camtlimit, char *damtlimit, char *otransamt, char outcdtype);
extern int SumAmt(char *curamt, char *tranamt, char *totalamt);
extern int DecAmt(char *curamt, char *tranamt, char *totalamt);
extern int  ProcessBalance(ISO_data iso, PEP_JNLS pep_jnls, EWP_INFO ewp_info, char *buffer);
extern int GetPospMacBlk(ISO_data *iso, char *macblk);
extern int writePos(char *isodata, int len, int fMid, char *tpdu, MER_INFO_T mer_info_t);
#endif
