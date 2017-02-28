//
//  fold_log.c
//  iPrepose
//
//  Created by Freax on 12-12-19.
//  Copyright (c) 2012 Chinaebi. All rights reserved.
//

#include <stdio.h>
/*
 Symbols from fold_log.o:
 
 [Index]     Value      Size      Type  Bind  Other Shname         Name 
 
 [6]       |8         |4         |OBJT |LOCL |0    |.data         |gs_qid
 [7]       |4         |4         |OBJT |LOCL |0    |.data         |gs_tid
 [8]       |0         |4         |OBJT |LOCL |0    |.data         |gs_pid
 
 [37]      |0         |0         |NOTY |GLOB |0    |UNDEF         |write
 [38]      |0         |0         |NOTY |GLOB |0    |UNDEF         |vsprintf
 [39]      |0         |0         |NOTY |GLOB |0    |UNDEF         |strlen
 [40]      |0         |0         |NOTY |GLOB |0    |UNDEF         |sprintf
 [41]      |0         |0         |NOTY |GLOB |0    |UNDEF         |localtime
 [42]      |0         |0         |NOTY |GLOB |0    |UNDEF         |time
 [43]      |0         |0         |NOTY |GLOB |0    |UNDEF         |getpid
 [45]      |0         |0         |NOTY |GLOB |0    |UNDEF         |close
 [47]      |0         |0         |NOTY |GLOB |0    |UNDEF         |open
 
 [44]      |1216      |346       |FUNC |GLOB |0    |.text         |log_jotdown
 [46]      |1196      |18        |FUNC |GLOB |0    |.text         |log_close
 [48]      |1148      |48        |FUNC |GLOB |0    |.text         |log_open
 
 */

