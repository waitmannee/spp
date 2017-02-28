//
//  fold.c
//  iPrepose
//
//  Created by Freax on 13-1-3.
//  Copyright (c) 2013年 Chinaebi. All rights reserved.
//
#include "ibdcs.h"
#include <string.h>
#include <stdio.h>
#include "folder.h"
#include <dirent.h>

static int obj_id = -1;

static char filepath[1024];
static int isFound = 0;

int  fold_get_id(char *dir, int fold_id)
{
    int depth = 0;
    obj_id = -1;

    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;

    if((dp = opendir(dir)) == NULL) {
        fprintf(stderr,"cannot open directory: %s\n", dir);
        return -1;
    }

    chdir(dir);

    while((entry = readdir(dp)) != NULL) {
        lstat(entry->d_name,&statbuf);

        if(S_ISDIR(statbuf.st_mode)) {
            /* Found a directory, but ignore . and .. */
            if(strcmp(".",entry->d_name) == 0 || strcmp("..",entry->d_name) == 0) continue;

            printf("%*s%s %lld\n",depth,"",entry->d_name, entry->d_ino);


            if (fold_id == entry->d_ino) {

                printf("%*s%s %lld\n",depth,"",entry->d_name, entry->d_ino);

                chdir("..");
                closedir(dp);
                obj_id = (int)entry->d_ino;
                return obj_id;

            }

            /* Recurse at a new indent level */
            fold_get_id(entry->d_name, fold_id);
        }
        else
        {
            if (fold_id == entry->d_ino) {
                printf("%*s%s %lld\n",depth,"",entry->d_name, entry->d_ino);

                chdir("..");
                closedir(dp);
                obj_id = (int)entry->d_ino;
                return obj_id;
            }
        }
    }

    chdir("..");
    closedir(dp);

    return obj_id;
}

/*
 * 根据文件名称得到文件ID
 */
int  fold_get_id_by_name(char *dir, const char *filename)
{
    int depth = 0;
    obj_id = -1;

    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;

    if((dp = opendir(dir)) == NULL) {
        fprintf(stderr,"cannot open directory: %s\n", dir);
        return -1;
    }

    chdir(dir);

    while((entry = readdir(dp)) != NULL) {
        lstat(entry->d_name,&statbuf);

        if(S_ISDIR(statbuf.st_mode)) {
            /* Found a directory, but ignore . and .. */
            if(strcmp(".",entry->d_name) == 0 || strcmp("..",entry->d_name) == 0) continue;

            printf("%*s%s %lld\n",depth,"",entry->d_name, entry->d_ino);


            if (strcmp(filename,entry->d_name) == 0) {

                printf("%*s%s %lld\n",depth,"",entry->d_name, entry->d_ino);

                chdir("..");
                closedir(dp);
                obj_id = (int)entry->d_ino;
                return obj_id;

            }

            /* Recurse at a new indent level */
            fold_get_id_by_name(entry->d_name, filename);
        }
        else
        {
            if (strcmp(filename,entry->d_name) == 0) {
                printf("%*s%s %lld\n",depth,"",entry->d_name, entry->d_ino);

                chdir("..");
                closedir(dp);
                obj_id = (int)entry->d_ino;
                return obj_id;
            }
        }
    }

    chdir("..");
    closedir(dp);

    return obj_id;
}

/*
 * 根据文件的名字得到路径
 */
int  fold_get_path_by_name(const char *dir, const char *filename)
{
    int depth = 0;

    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;

    if((dp = opendir(dir)) == NULL) {
        fprintf(stderr,"cannot open directory: %s\n", dir);
        return -1;
    }

    chdir(dir);

    while((entry = readdir(dp)) != NULL) {
        lstat(entry->d_name,&statbuf);

        if(S_ISDIR(statbuf.st_mode)) {
            /* Found a directory, but ignore . and .. */
            if(strcmp(".",entry->d_name) == 0 || strcmp("..",entry->d_name) == 0) continue;

            printf("%*s%s %lld\n",depth,"",entry->d_name, entry->d_ino);


            if (strcmp(filename,entry->d_name) == 0) {

                isFound = 1;

                printf("%*s%s %lld\n",depth,"",entry->d_name, entry->d_ino);

                sprintf(filepath, "%s/%s", filepath,entry->d_name);
                printf("filepath ===>> %s\n", filepath);

                chdir("..");
                closedir(dp);

                obj_id = (int)entry->d_ino;
                return obj_id;
            }

            if(!isFound) sprintf(filepath, "%s/%s", filepath,entry->d_name);

            printf("filepath ===>> %s\n", filepath);

            /* Recurse at a new indent level */
            fold_get_path_by_name(entry->d_name, filename);
        }
        else
        {
            if (strcmp(filename,entry->d_name) == 0) {
                isFound = 1;
                sprintf(filepath, "%s/%s", filepath,entry->d_name);
                printf("filepath ===>> %s\n", filepath);

                printf("%*s%s %lld\n",depth,"",entry->d_name, entry->d_ino);

                chdir("..");
                closedir(dp);

                obj_id = (int)entry->d_ino;
                return obj_id;
            }
        }
    }

    chdir("..");
    closedir(dp);
//    memset(filepath, 0x00, sizeof(filepath));
    return obj_id;
}


int get_fold_name_from_share_memory_by_id(char *fold_name,int  fid){
        int fold_shm_id;
        void *ptr = NULL;
        ptr = shm_connect(FOLDER_LIST_SHM_NAME,&fold_shm_id);
	if(ptr == NULL){
	   dcs_log(0, 0, "get_fold_name_from_share_memory_by_id,error!");
	   return -1;
	}

    FOLDERENTRY *tmp_p = (FOLDERENTRY *)ptr;

    int index = -1;
    for (; strlen(tmp_p->fold_name); tmp_p++) 
    {
        index ++;
        if (fid == index) 
	{
		strcpy(fold_name,tmp_p->fold_name);
		shm_detach(ptr);
            return 0;
        }
    }
    memcpy(fold_name,"",1);
    shm_detach(ptr);
    return -1;
}

int get_fold_id_from_share_memory_by_name(char *fold_name, int * fid)
{
        int fold_shm_id;
        void *ptr = NULL;
        ptr = shm_connect(FOLDER_LIST_SHM_NAME,&fold_shm_id);
	if(ptr == NULL){
	   dcs_log(0, 0, "get_fold_id_from_share_memory_by_name,error!");
	   return -1;
	}

    FOLDERENTRY *tmp_p =(FOLDERENTRY *)ptr;
    int rtn_id = -1;
    int index = -1;
    for (; strlen(tmp_p->fold_name); tmp_p++) {
        index ++;
        if (strcmp(fold_name, tmp_p->fold_name) == 0) {
            rtn_id = index;
            break;
        }
    }
    //printf("get_fold_id_from_share_memory_by_name = %d\n", rtn_id);
    *fid = rtn_id;
    shm_detach(ptr);
     return rtn_id;
}


