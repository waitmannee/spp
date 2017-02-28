//
//  fold_lock.c
//  iPrepose
//
//  Created by Freax on 12-12-18.
//  Copyright (c) 2012年 Chinaebi. All rights reserved.
//

#include <stdio.h>

/*
 [1]       |0         |0         |FILE |LOCL |0    |ABS           |fold_lock.c
 
 [32]      |1328      |97        |FUNC |GLOB |0    |.text         |fold_unlock_folder
 [33]      |1228      |97        |FUNC |GLOB |0    |.text         |fold_lock_folder
 [35]      |1188      |40        |FUNC |GLOB |0    |.text         |fold_unlock_folderlist
 [38]      |1148      |40        |FUNC |GLOB |0    |.text         |fold_lock_folderlist
 
 [34]      |0         |0         |NOTY |GLOB |0    |UNDEF         |sem_unlock
 [36]      |0         |0         |NOTY |GLOB |0    |UNDEF         |sem_lock
 
 [37]      |0         |0         |NOTY |GLOB |0    |UNDEF         |gl_pFldCtrl

 */







