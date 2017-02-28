//
//  ipc_fmap.c
//  iPrepose
//
//  Created by Freax on 12-12-19.
//  Copyright (c) 2012年 Chinaebi. All rights reserved.
//

#include <stdio.h>
/*
 Symbols from ipc_fmap.o:
 
 [Index]     Value      Size      Type  Bind  Other Shname         Name
 
 [1]       |0         |0         |FILE |LOCL |0    |ABS           |ipc_fmap.c
 
 [2]       |0         |0         |SECT |LOCL |0    |.text         |.text
 [3]       |0         |0         |SECT |LOCL |0    |.rodata1      |.rodata1
 [4]       |0         |0         |SECT |LOCL |0    |.comment      |.comment
 [33]      |0         |0         |NOTY |GLOB |0    |UNDEF         |munmap
 
 [36]      |0         |0         |NOTY |GLOB |0    |UNDEF         |close
 [37]      |0         |0         |NOTY |GLOB |0    |UNDEF         |mmap
 [38]      |0         |0         |NOTY |GLOB |0    |UNDEF         |write
 [39]      |0         |0         |NOTY |GLOB |0    |UNDEF         |lseek
 [40]      |0         |0         |NOTY |GLOB |0    |UNDEF         |open
 
 [34]      |1460      |25        |FUNC |GLOB |0    |.text         |fmap_unmap
 [35]      |1344      |116       |FUNC |GLOB |0    |.text         |fmap_connect
 [41]      |1148      |194       |FUNC |GLOB |0    |.text         |fmap_create
 
 */


