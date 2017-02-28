//
//  util_signal.h
//  iPrepose
//
//  Created by Freax on 12-12-19.
//  Copyright (c) 2012年 Chinaebi. All rights reserved.
//

#ifndef iPrepose_util_signal_h
#define iPrepose_util_signal_h
#include "unixhdrs.h"

int  catch_all_signals(sigfunc_t  pfn);
int  wait_child_or_signal(int *stat_loc);

#endif
