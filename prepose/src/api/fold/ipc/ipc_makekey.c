//
//  ipc_makekey.c
//  iPrepose
//
//  Created by Freax on 12-12-19.
//  Copyright (c) 2012年 Chinaebi. All rights reserved.
//

#include <stdio.h>

#include "unixhdrs.h"
#include "folder.h"

#define LEN_PATH  100

////////////////// C dictionary  data structure //////////////////////////
//struct nlist { /* table entry: */
//    struct nlist *next; /* next entry in chain */
//    char *name; /* defined name */
//    char *defn; /* replacement text */
//};

#define HASHSIZE 1000000
//struct nlist *shm_hashtab[HASHSIZE]; /* pointer table */

/* hash: form hash value for string s */
unsigned hash(char *s)
{
    unsigned hashval;
    for (hashval = 0; *s != '\0'; s++)
        hashval = *s + 31 * hashval;
    return hashval % HASHSIZE;
}

/* lookup: look for s in hashtab */
//struct nlist *lookup(char *s)
//{
//    struct nlist *np;    
//    
//    for (np = shm_hashtab[hash(s)]; np != NULL; np = np->next)
//    {
//        if (strcmp(s, np->name) == 0)
//            return np; /* found */
//    }
//    return NULL; /* not found */
//}
//
//extern char *strdup(char *);
///* install: put (name, defn) in hashtab */
//struct nlist *install(char *name, char *defn)
//{
//    struct nlist *np;
//    unsigned hashval;
//    if ((np = lookup(name)) == NULL) { /* not found */
//        np = (struct nlist *) malloc(sizeof(*np));
//        if (np == NULL || (np->name = strdup(name)) == NULL)
//            return NULL;
//        hashval = hash(name);
//        np->next = shm_hashtab[hashval];
//        shm_hashtab[hashval] = np;
//    } else /* already there */
//        free((void *) np->defn); /*free previous defn */
//    if ((np->defn = strdup(defn)) == NULL)
//        return NULL;
//    return np;
//}
//
//char *strdup(char *s) /* make a duplicate of s */
//{
//    char *p;
//    p = (char *) malloc(strlen(s)+1); /* +1 for ’\0’ */
//    if (p != NULL)
//        strcpy(p, s);
//    return p;
//}
//
///*dic opreation interface */
//void dic_put(const char *key, const char *value)
//{
//    install((char *)key,(char *)value);
//}
//
//char* dic_value_for_key(const char *key)
//{
//    char *ret = "";
//    struct nlist *np = lookup((char *)key);
//    if (np != NULL) {
//        ret = np->defn;
//    } 
//    return ret;
//}

///////////////////////////// C dictionary  data structure end /////////////////////////////////////

/*
 * 根据key取得name
 */
//char* ipc_namefromkey(int key)
//{
//    char key_[50]; sprintf(key_, "%d", key);
//    return dic_value_for_key(key_);
//}

key_t key = 5679;


/*
 * 根据name制作key
 */
int ipc_makekey(const char *name)
{
	int key_ipc = 0;
	if(memcmp(name, "SECSRV2", 4) == 0) key_ipc = 980724530;
	else if(memcmp(name, "FdLt", 4) == 0) 	key_ipc = 416884340;
	else if(memcmp(name, "BfPl", 4) == 0) 	key_ipc = 408529004;
	else if(memcmp(name, "ISDSRVER", 4) == 0) 	key_ipc = 408529111;
	else if(memcmp(name, "SC_SERVER", 9) == 0) 	key_ipc = 408529222;
	else if(memcmp(name, "SC_CLIENT", 9) == 0)       key_ipc = 408529333;
	else if(memcmp(name, "HTTP_CLIENT", 11) == 0)       key_ipc = 408529344;
	else if(memcmp(name, "HTTP_SERVER", 11) == 0)       key_ipc = 408529355;
	else if(memcmp(name, "SYN_TCP", 7) == 0)       key_ipc = 408529366;
	else if(memcmp(name, "SYN_HTTP", 8) == 0)       key_ipc = 408529367;
	else if(check_str_or_int(name) == 0)      key_ipc =atoi(name);
	else  key_ipc = 1115700 + hash((char *)name);
	
	return key_ipc;
}

////////////////////////////////////////////////////////////////////////////////////

char	* filename = NULL;

char	* getfname ( char	* env, char	* fname )
{
    char	* workdir;
    
	if ( env == NULL || fname == NULL )	return ( NULL );
    
	if ( filename == NULL ) filename = ( char * ) malloc ( ( size_t ) LEN_PATH );
    
	if ( filename == NULL )	return ( NULL );
    
	if ( ( workdir = getenv ( env ) ) == NULL )	return ( NULL );
    
	sprintf ( filename, "%s/%s", workdir, fname );
	
	return ( filename );
}

key_t	getipckey ( msgfile, id )
char	* msgfile;
char	id;
{
    int 	fd;
    char 	*filename;
    
#ifndef wyz_mod_0518
	filename = (char *) getfname("WORKDIR", msgfile);
#else
	filename = (char *) getfname("TrCPDIR" , msgfile);
#endif
    
#ifdef DEBUG1_del
	printf("getipckey file %s\n", filename);
#endif
    
	fd = open(filename, O_CREAT|O_EXCL);
    
	if (fd == -1 && errno != EEXIST) return((key_t)-1);
    
	close(fd);
	return (ftok(filename, id));
}




