
/* Result Sets Interface */
#ifndef SQL_CRSR
#  define SQL_CRSR
struct sql_cursor
{
    unsigned int curocn;
    void *ptr1;
    void *ptr2;
    unsigned long magic;
};
typedef struct sql_cursor sql_cursor;
typedef struct sql_cursor SQL_CURSOR;
#endif /* SQL_CRSR */

/* Thread Safety */
typedef void * sql_context;
typedef void * SQL_CONTEXT;

/* Object support */
struct sqltvn
{
    unsigned char *tvnvsn;
    unsigned short tvnvsnl;
    unsigned char *tvnnm;
    unsigned short tvnnml;
    unsigned char *tvnsnm;
    unsigned short tvnsnml;
};
typedef struct sqltvn sqltvn;

struct sqladts
{
    unsigned int adtvsn;
    unsigned short adtmode;
    unsigned short adtnum;
    sqltvn adttvn[1];
};
typedef struct sqladts sqladts;

static struct sqladts sqladt = {
    1,1,0,
};

/* Binding to PL/SQL Records */
struct sqltdss
{
    unsigned int tdsvsn;
    unsigned short tdsnum;
    unsigned char *tdsval[1];
};
typedef struct sqltdss sqltdss;
static struct sqltdss sqltds =
{
    1,
    0,
};

/* File name & Package Name */
struct sqlcxp
{
    unsigned short fillen;
    char  filnam[11];
};
static struct sqlcxp sqlfpn =
{
    10,
    "mytbase.pc"
};


static unsigned long sqlctx = 81555;


static struct sqlexd {
    unsigned int   sqlvsn;
    unsigned int   arrsiz;
    unsigned int   iters;
    unsigned int   offset;
    unsigned short selerr;
    unsigned short sqlety;
    unsigned int   occurs;
    short *cud;
    unsigned char  *sqlest;
    char  *stmt;
    sqladts *sqladtp;
    sqltdss *sqltdsp;
    void  **sqphsv;
    unsigned int   *sqphsl;
    int   *sqphss;
    void  **sqpind;
    int   *sqpins;
    unsigned int   *sqparm;
    unsigned int   **sqparc;
    unsigned short  *sqpadto;
    unsigned short  *sqptdso;
    void  *sqhstv[15];
    unsigned int   sqhstl[15];
    int   sqhsts[15];
    void  *sqindv[15];
    int   sqinds[15];
    unsigned int   sqharm[15];
    unsigned int   *sqharc[15];
    unsigned short  sqadto[15];
    unsigned short  sqtdso[15];
} sqlstm = {10,15};

/* SQLLIB Prototypes */
extern sqlcxt (/*_ void **, unsigned long *,
                struct sqlexd *, struct sqlcxp * _*/);
extern sqlcx2t(/*_ void **, unsigned long *,
                struct sqlexd *, struct sqlcxp * _*/);
extern sqlbuft(/*_ void **, char * _*/);
extern sqlgs2t(/*_ void **, char * _*/);
extern sqlorat(/*_ void **, unsigned long *, void * _*/);

/* Forms Interface */
static int IAPSUCC = 0;
static int IAPFAIL = 1403;
static int IAPFTL  = 535;
extern void sqliem(/*_ char *, int * _*/);

//static char *sq0013 =
//"select v_sysstan ,v_txdate ,v_pro_time ,v_txcode ,sms_concode ,nvl(v_callno\
//,' ') ,to_number(nvl(v_txamt,0)) ,nvl(v_describe,' ') ,nvl(v_mobileflg,' ') ,\
//nvl(v_mobilecode,' ') ,nvl(v_mercode,'0') ,nvl(v_trans_mercode,'0') ,nvl(v_cd\
//no,'0') ,nvl(v_incardno,' ') ,nvl(v_samno,' ')  from ctl_trade_list_sms where\
//((((msg_update_time>:b0 and msg_update_time<=:b1) and v_tradestau='00') and \
//v_source='1') and v_succflag='0')           ";
//
//typedef struct { unsigned short len; unsigned char arr[1]; } VARCHAR;
//typedef struct { unsigned short len; unsigned char arr[1]; } varchar;
//
///* CUD (Compilation Unit Data) Array */
//static short sqlcud0[] =
//{10,4130,0,0,0,
//    5,0,0,1,0,0,539,36,0,0,4,4,0,1,0,1,97,0,0,1,97,0,0,1,10,0,0,1,10,0,0,
//    36,0,0,2,0,0,539,38,0,0,4,4,0,1,0,1,97,0,0,1,97,0,0,1,97,0,0,1,10,0,0,
//    67,0,0,3,0,0,544,55,0,0,0,0,0,1,0,
//    82,0,0,4,61,0,516,75,0,0,2,1,0,1,0,2,97,0,0,1,97,0,0,
//    105,0,0,5,0,0,543,83,0,0,0,0,0,1,0,
//    120,0,0,6,0,0,541,91,0,0,0,0,0,1,0,
//    135,0,0,7,60,0,516,113,0,0,2,1,0,1,0,2,3,0,0,1,97,0,0,
//    158,0,0,8,55,0,517,121,0,0,2,2,0,1,0,1,97,0,0,1,97,0,0,
//    181,0,0,9,0,0,543,128,0,0,0,0,0,1,0,
//    196,0,0,10,0,0,541,131,0,0,0,0,0,1,0,
//    211,0,0,11,174,0,516,207,0,0,3,1,0,1,0,2,3,0,0,2,97,0,0,1,97,0,0,
//    238,0,0,12,145,0,516,223,0,0,3,2,0,1,0,2,3,0,0,1,97,0,0,1,97,0,0,
//    265,0,0,13,427,0,521,244,0,0,2,2,0,1,0,1,97,0,0,1,97,0,0,
//    288,0,0,13,0,0,525,249,0,0,15,0,0,1,0,2,97,0,0,2,97,0,0,2,97,0,0,2,97,0,0,2,97,
//    0,0,2,97,0,0,2,3,0,0,2,97,0,0,2,97,0,0,2,97,0,0,2,97,0,0,2,97,0,0,2,97,0,0,2,
//    97,0,0,2,97,0,0,
//    363,0,0,14,234,0,516,313,0,0,5,4,0,1,0,2,3,0,0,1,97,0,0,1,97,0,0,1,97,0,0,1,97,
//    0,0,
//    398,0,0,15,267,0,516,334,0,0,7,5,0,1,0,2,97,0,0,2,97,0,0,1,3,0,0,1,97,0,0,1,97,
//    0,0,1,97,0,0,1,97,0,0,
//    441,0,0,16,215,0,516,413,0,0,4,2,0,1,0,2,3,0,0,2,4,0,0,1,97,0,0,1,97,0,0,
//    472,0,0,17,175,0,516,528,0,0,2,1,0,1,0,2,97,0,0,1,97,0,0,
//    495,0,0,18,165,0,516,544,0,0,2,1,0,1,0,2,97,0,0,1,97,0,0,
//    518,0,0,19,234,0,516,568,0,0,5,4,0,1,0,2,3,0,0,1,97,0,0,1,97,0,0,1,97,0,0,1,97,
//    0,0,
//    553,0,0,20,267,0,516,589,0,0,7,5,0,1,0,2,97,0,0,2,97,0,0,1,3,0,0,1,97,0,0,1,97,
//    0,0,1,97,0,0,1,97,0,0,
//    596,0,0,21,215,0,516,667,0,0,4,2,0,1,0,2,3,0,0,2,4,0,0,1,97,0,0,1,97,0,0,
//    627,0,0,22,75,0,517,712,0,0,1,1,0,1,0,1,97,0,0,
//    646,0,0,23,0,0,543,718,0,0,0,0,0,1,0,
//    661,0,0,24,0,0,541,721,0,0,0,0,0,1,0,
//};
//
//
//#include <stdio.h>
//#include <string.h>
//#include <stdlib.h>
//#include <sqlca.h>
//#include <oraca.h>
//#include <unistd.h>
//#include <sys/time.h>
//
//#include "mytbase.h"
//
///* EXEC SQL INCLUDE SQLCA;
// */
///*
// * $Header: sqlca.h,v 1.3 1994/12/12 19:27:27 jbasu Exp $ sqlca.h
// */
//
///* Copyright (c) 1985,1986, 1998 by Oracle Corporation. */
//
///*
// NAME
// SQLCA : SQL Communications Area.
// FUNCTION
// Contains no code. Oracle fills in the SQLCA with status info
// during the execution of a SQL stmt.
// NOTES
// **************************************************************
// ***                                                        ***
// *** This file is SOSD.  Porters must change the data types ***
// *** appropriately on their platform.  See notes/pcport.doc ***
// *** for more information.                                  ***
// ***                                                        ***
// **************************************************************
// 
// If the symbol SQLCA_STORAGE_CLASS is defined, then the SQLCA
// will be defined to have this storage class. For example:
// 
// #define SQLCA_STORAGE_CLASS extern
// 
// will define the SQLCA as an extern.
// 
// If the symbol SQLCA_INIT is defined, then the SQLCA will be
// statically initialized. Although this is not necessary in order
// to use the SQLCA, it is a good pgming practice not to have
// unitialized variables. However, some C compilers/OS's don't
// allow automatic variables to be init'd in this manner. Therefore,
// if you are INCLUDE'ing the SQLCA in a place where it would be
// an automatic AND your C compiler/OS doesn't allow this style
// of initialization, then SQLCA_INIT should be left undefined --
// all others can define SQLCA_INIT if they wish.
// 
// If the symbol SQLCA_NONE is defined, then the SQLCA variable will
// not be defined at all.  The symbol SQLCA_NONE should not be defined
// in source modules that have embedded SQL.  However, source modules
// that have no embedded SQL, but need to manipulate a sqlca struct
// passed in as a parameter, can set the SQLCA_NONE symbol to avoid
// creation of an extraneous sqlca variable.
// 
// MODIFIED
// lvbcheng   07/31/98 -  long to int
// jbasu      12/12/94 -  Bug 217878: note this is an SOSD file
// losborne   08/11/92 -  No sqlca var if SQLCA_NONE macro set
// Clare      12/06/84 - Ch SQLCA to not be an extern.
// Clare      10/21/85 - Add initialization.
// Bradbury   01/05/86 - Only initialize when SQLCA_INIT set
// Clare      06/12/86 - Add SQLCA_STORAGE_CLASS option.
// */
//
//#ifndef SQLCA
//#define SQLCA 1
//
//struct   sqlca
//{
//    /* ub1 */ char    sqlcaid[8];
//    /* b4  */ int     sqlabc;
//    /* b4  */ int     sqlcode;
//    struct
//    {
//        /* ub2 */ unsigned short sqlerrml;
//        /* ub1 */ char           sqlerrmc[70];
//    } sqlerrm;
//    /* ub1 */ char    sqlerrp[8];
//    /* b4  */ int     sqlerrd[6];
//    /* ub1 */ char    sqlwarn[8];
//    /* ub1 */ char    sqlext[8];
//};
//
//#ifndef SQLCA_NONE
//#ifdef   SQLCA_STORAGE_CLASS
//SQLCA_STORAGE_CLASS struct sqlca sqlca
//#else
//struct sqlca sqlca
//#endif
//
//#ifdef  SQLCA_INIT
//= {
//    {'S', 'Q', 'L', 'C', 'A', ' ', ' ', ' '},
//    sizeof(struct sqlca),
//    0,
//    { 0, {0}},
//    {'N', 'O', 'T', ' ', 'S', 'E', 'T', ' '},
//    {0, 0, 0, 0, 0, 0},
//    {0, 0, 0, 0, 0, 0, 0, 0},
//    {0, 0, 0, 0, 0, 0, 0, 0}
//}
//#endif
//;
//#endif
//
//#endif
//
///* end SQLCA */
///* EXEC SQL INCLUDE ORACA;
// */
///*
// * $Header: oraca.h 31-jul-99.19:33:19 apopat Exp $ oraca.h
// */
//
///* Copyright (c) 1985, 1996, 1998, 1999 by Oracle Corporation. */
//
///*
// NAME
// ORACA : Oracle Communications Area.
// FUNCTION
// Contains no code. Provides supplementary communications to/from
// Oracle (in addition to standard SQLCA).
// NOTES
// **************************************************************
// ***                                                        ***
// *** This file is SOSD.  Porters must change the data types ***
// *** appropriately on their platform.  See notes/pcport.doc ***
// *** for more information.                                  ***
// ***                                                        ***
// **************************************************************
// 
// oracchf : Check cursor cache consistency flag. If set AND oradbgf
// is set, then directs SQLLIB to perform cursor cache
// consistency checks before every cursor operation
// (OPEN, FETCH, SELECT, INSERT, etc.).
// oradbgf : Master DEBUG flag. Used to turn all DEBUG options
// on or off.
// orahchf : Check Heap consistency flag. If set AND oradbgf is set,
// then directs SQLLIB to perform heap consistency checks
// everytime memory is dynamically allocated/free'd via
// sqlalc/sqlfre/sqlrlc. MUST BE SET BEFORE 1ST CONNECT
// and once set cannot be cleared (subsequent requests
// to change it are ignored).
// orastxtf: Save SQL stmt text flag. If set, then directs SQLLIB
// to save the text of the current SQL stmt in orastxt
// (in VARCHAR format).
// orastxt : Saved len and text of current SQL stmt (in VARCHAR
// format).
// orasfnm : Saved len and text of filename containing current SQL
// stmt (in VARCHAR format).
// oraslnr : Saved line nr within orasfnm of current SQL stmt.
// 
// Cursor cache statistics. Set after COMMIT or ROLLBACK. Each
// CONNECT'd DATABASE has its own set of statistics.
// 
// orahoc  : Highest Max Open OraCursors requested. Highest value
// for MAXOPENCURSORS by any CONNECT to this DATABASE.
// oramoc  : Max Open OraCursors required. Specifies the max nr
// of OraCursors required to run this pgm. Can be higher
// than orahoc if working set (MAXOPENCURSORS) was set
// too low, thus forcing the PCC to expand the cache.
// oracoc  : Current nr of OraCursors used.
// oranor  : Nr of OraCursor cache reassignments. Can show the
// degree of "thrashing" in the cache. Optimally, this
// nr should be kept as low as possible (time vs space
// optimization).
// oranpr  : Nr of SQL stmt "parses".
// oranex  : Nr of SQL stmt "executes". Optimally, the relation-
// ship of oranex to oranpr should be kept as high as
// possible.
// 
// 
// If the symbol ORACA_NONE is defined, then there will be no ORACA
// *variable*, although there will still be a struct defined.  This
// macro should not normally be defined in application code.
// 
// If the symbol ORACA_INIT is defined, then the ORACA will be
// statically initialized. Although this is not necessary in order
// to use the ORACA, it is a good pgming practice not to have
// unitialized variables. However, some C compilers/OS's don't
// allow automatic variables to be init'd in this manner. Therefore,
// if you are INCLUDE'ing the ORACA in a place where it would be
// an automatic AND your C compiler/OS doesn't allow this style
// of initialization, then ORACA_INIT should be left undefined --
// all others can define ORACA_INIT if they wish.
// 
// OWNER
// Clare
// DATE
// 10/19/85
// MODIFIED
// apopat     07/31/99 -  [707588] TAB to blanks for OCCS
// lvbcheng   10/27/98 -  change long to int for oraca
// pccint     10/03/96 -  Add IS_OSD for linting
// jbasu      12/12/94 -  Bug 217878: note this is an SOSD file
// losborne   09/04/92 -  Make oraca variable optional
// Osborne    05/24/90 - Add ORACA_STORAGE_CLASS construct
// Clare      02/20/86 - PCC [10101l] Feature: Heap consistency check.
// Clare      03/04/86 - PCC [10101r] Port: ORACA init ifdef.
// Clare      03/12/86 - PCC [10101ab] Feature: ORACA cuc statistics.
// */
///* IS_OSD */
//#ifndef  ORACA
//#define  ORACA     1
//
//struct   oraca
//{
//    /* text */ char oracaid[8];        /* Reserved                            */
//    /* ub4  */ int oracabc;            /* Reserved                            */
//    
//    /*       Flags which are setable by User. */
//    
//    /* ub4 */ int  oracchf;             /* <> 0 if "check cur cache consistncy"*/
//    /* ub4 */ int  oradbgf;             /* <> 0 if "do DEBUG mode checking"    */
//    /* ub4 */ int  orahchf;             /* <> 0 if "do Heap consistency check" */
//    /* ub4 */ int  orastxtf;            /* SQL stmt text flag                  */
//#define  ORASTFNON 0                   /* = don't save text of SQL stmt       */
//#define  ORASTFERR 1                   /* = only save on SQLERROR             */
//#define  ORASTFWRN 2                   /* = only save on SQLWARNING/SQLERROR  */
//#define  ORASTFANY 3                   /* = always save                       */
//    struct
//    {
//        /* ub2  */ unsigned short orastxtl;
//        /* text */ char  orastxtc[70];
//    } orastxt;                  /* text of last SQL stmt               */
//    struct
//    {
//        /* ub2  */   unsigned short orasfnml;
//        /* text */   char       orasfnmc[70];
//    } orasfnm;                  /* name of file containing SQL stmt    */
//    /* ub4 */ int   oraslnr;             /* line nr-within-file of SQL stmt     */
//    
//    /* ub4 */ int   orahoc;              /* highest max open OraCurs requested  */
//    /* ub4 */ int   oramoc;              /* max open OraCursors required        */
//    /* ub4 */ int   oracoc;              /* current OraCursors open             */
//    /* ub4 */ int   oranor;              /* nr of OraCursor re-assignments      */
//    /* ub4 */ int   oranpr;              /* nr of parses                        */
//    /* ub4 */ int   oranex;              /* nr of executes                      */
//};
//
//#ifndef ORACA_NONE
//
//#ifdef ORACA_STORAGE_CLASS
//ORACA_STORAGE_CLASS struct oraca oraca
//#else
//struct oraca oraca
//#endif
//#ifdef ORACA_INIT
//=
//{
//    {'O','R','A','C','A',' ',' ',' '},
//    sizeof(struct oraca),
//    0,0,0,0,
//    {0,{0}},
//    {0,{0}},
//    0,
//    0,0,0,0,0,0
//}
//#endif
//;
//
//#endif
//
//#endif
///* end oraca.h */
//
///* EXEC ORACLE OPTION (ORACA=YES); */
//
///* EXEC ORACLE OPTION (release_cursor=yes); */
//
//
//
//
///************************************基本操作****************************************/
//
//
///*连接数据库*/
//
//int ConnectDb(char *username,char *passwd,char *dbname)
//{
//	/* EXEC SQL BEGIN DECLARE SECTION; */
//    
//    char	v_username[256];
//    char  v_passwd[256];
//    char  v_dbname[256];
//	/* EXEC SQL END DECLARE SECTION; */
//    
//	char msg[2048];
//	
//	strcpy(v_username,username);
//	strcpy(v_passwd,passwd);
//	strcpy(v_dbname,dbname);
//	if (strcmp(v_dbname,"")==0)
//    /* EXEC SQL CONNECT :v_username IDENTIFIED BY :v_passwd; */
//        
//    {
//        struct sqlexd sqlstm;
//        sqlorat((void **)0, &sqlctx, &oraca);
//        sqlstm.sqlvsn = 10;
//        sqlstm.arrsiz = 4;
//        sqlstm.sqladtp = &sqladt;
//        sqlstm.sqltdsp = &sqltds;
//        sqlstm.iters = (unsigned int  )10;
//        sqlstm.offset = (unsigned int  )5;
//        sqlstm.cud = sqlcud0;
//        sqlstm.sqlest = (unsigned char  *)&sqlca;
//        sqlstm.sqlety = (unsigned short)256;
//        sqlstm.occurs = (unsigned int  )0;
//        sqlstm.sqhstv[0] = (         void  *)v_username;
//        sqlstm.sqhstl[0] = (unsigned int  )256;
//        sqlstm.sqhsts[0] = (         int  )256;
//        sqlstm.sqindv[0] = (         void  *)0;
//        sqlstm.sqinds[0] = (         int  )0;
//        sqlstm.sqharm[0] = (unsigned int  )0;
//        sqlstm.sqadto[0] = (unsigned short )0;
//        sqlstm.sqtdso[0] = (unsigned short )0;
//        sqlstm.sqhstv[1] = (         void  *)v_passwd;
//        sqlstm.sqhstl[1] = (unsigned int  )256;
//        sqlstm.sqhsts[1] = (         int  )256;
//        sqlstm.sqindv[1] = (         void  *)0;
//        sqlstm.sqinds[1] = (         int  )0;
//        sqlstm.sqharm[1] = (unsigned int  )0;
//        sqlstm.sqadto[1] = (unsigned short )0;
//        sqlstm.sqtdso[1] = (unsigned short )0;
//        sqlstm.sqphsv = sqlstm.sqhstv;
//        sqlstm.sqphsl = sqlstm.sqhstl;
//        sqlstm.sqphss = sqlstm.sqhsts;
//        sqlstm.sqpind = sqlstm.sqindv;
//        sqlstm.sqpins = sqlstm.sqinds;
//        sqlstm.sqparm = sqlstm.sqharm;
//        sqlstm.sqparc = sqlstm.sqharc;
//        sqlstm.sqpadto = sqlstm.sqadto;
//        sqlstm.sqptdso = sqlstm.sqtdso;
//        sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
//    }
//    
//    
//	else
//    /* EXEC SQL CONNECT :v_username IDENTIFIED BY :v_passwd USING :v_dbname; */
//        
//    {
//        struct sqlexd sqlstm;
//        sqlorat((void **)0, &sqlctx, &oraca);
//        sqlstm.sqlvsn = 10;
//        sqlstm.arrsiz = 4;
//        sqlstm.sqladtp = &sqladt;
//        sqlstm.sqltdsp = &sqltds;
//        sqlstm.iters = (unsigned int  )10;
//        sqlstm.offset = (unsigned int  )36;
//        sqlstm.cud = sqlcud0;
//        sqlstm.sqlest = (unsigned char  *)&sqlca;
//        sqlstm.sqlety = (unsigned short)256;
//        sqlstm.occurs = (unsigned int  )0;
//        sqlstm.sqhstv[0] = (         void  *)v_username;
//        sqlstm.sqhstl[0] = (unsigned int  )256;
//        sqlstm.sqhsts[0] = (         int  )256;
//        sqlstm.sqindv[0] = (         void  *)0;
//        sqlstm.sqinds[0] = (         int  )0;
//        sqlstm.sqharm[0] = (unsigned int  )0;
//        sqlstm.sqadto[0] = (unsigned short )0;
//        sqlstm.sqtdso[0] = (unsigned short )0;
//        sqlstm.sqhstv[1] = (         void  *)v_passwd;
//        sqlstm.sqhstl[1] = (unsigned int  )256;
//        sqlstm.sqhsts[1] = (         int  )256;
//        sqlstm.sqindv[1] = (         void  *)0;
//        sqlstm.sqinds[1] = (         int  )0;
//        sqlstm.sqharm[1] = (unsigned int  )0;
//        sqlstm.sqadto[1] = (unsigned short )0;
//        sqlstm.sqtdso[1] = (unsigned short )0;
//        sqlstm.sqhstv[2] = (         void  *)v_dbname;
//        sqlstm.sqhstl[2] = (unsigned int  )256;
//        sqlstm.sqhsts[2] = (         int  )256;
//        sqlstm.sqindv[2] = (         void  *)0;
//        sqlstm.sqinds[2] = (         int  )0;
//        sqlstm.sqharm[2] = (unsigned int  )0;
//        sqlstm.sqadto[2] = (unsigned short )0;
//        sqlstm.sqtdso[2] = (unsigned short )0;
//        sqlstm.sqphsv = sqlstm.sqhstv;
//        sqlstm.sqphsl = sqlstm.sqhstl;
//        sqlstm.sqphss = sqlstm.sqhsts;
//        sqlstm.sqpind = sqlstm.sqindv;
//        sqlstm.sqpins = sqlstm.sqinds;
//        sqlstm.sqparm = sqlstm.sqharm;
//        sqlstm.sqparc = sqlstm.sqharc;
//        sqlstm.sqpadto = sqlstm.sqadto;
//        sqlstm.sqptdso = sqlstm.sqtdso;
//        sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
//    }
//    
//    
//    if(sqlca.sqlcode!=0)
//    {
//        
//        sprintf(msg,"connect to database error : sqlcode is %d,%s, "
//                "u: %s p: %s,dbn: %s\n",sqlca.sqlcode,sqlca.sqlerrm.sqlerrmc,v_username,v_passwd,v_dbname);
//        writelog(msg,2);
//        return 0;
//    };
//	return 1;
//}
//
///*断开数据库连接*/
//void DisconnectDb()
//{
//    char msg[2048];
//    
//    /* EXEC SQL ROLLBACK WORK RELEASE; */
//    
//    {
//        struct sqlexd sqlstm;
//        sqlorat((void **)0, &sqlctx, &oraca);
//        sqlstm.sqlvsn = 10;
//        sqlstm.arrsiz = 4;
//        sqlstm.sqladtp = &sqladt;
//        sqlstm.sqltdsp = &sqltds;
//        sqlstm.iters = (unsigned int  )1;
//        sqlstm.offset = (unsigned int  )67;
//        sqlstm.cud = sqlcud0;
//        sqlstm.sqlest = (unsigned char  *)&sqlca;
//        sqlstm.sqlety = (unsigned short)256;
//        sqlstm.occurs = (unsigned int  )0;
//        sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
//    }
//    
//    
//    sprintf(msg,"disconnect database successful !\n");
//    /*writelog(msg,2);*/
//}
//
//
//
///*取得参数*/
//int GetPara(char *paraid,char *value)
//{
//    /* EXEC SQL BEGIN DECLARE SECTION; */
//    
//    char v_value[50+1];
//    char v_paraid[30+1];
//    /* EXEC SQL END DECLARE SECTION; */
//    
//    char msg[2048];
//    
//    strcpy(v_paraid,paraid);
//    strcpy(value,"");
//    
//    
//    /* EXEC SQL select para_value into :v_value from basic_para
//     where para_id=:v_paraid; */
//    
//    {
//        struct sqlexd sqlstm;
//        sqlorat((void **)0, &sqlctx, &oraca);
//        sqlstm.sqlvsn = 10;
//        sqlstm.arrsiz = 4;
//        sqlstm.sqladtp = &sqladt;
//        sqlstm.sqltdsp = &sqltds;
//        sqlstm.stmt = "select para_value into :b0  from basic_para where para_id\
//        =:b1";
//        sqlstm.iters = (unsigned int  )1;
//        sqlstm.offset = (unsigned int  )82;
//        sqlstm.selerr = (unsigned short)1;
//        sqlstm.cud = sqlcud0;
//        sqlstm.sqlest = (unsigned char  *)&sqlca;
//        sqlstm.sqlety = (unsigned short)256;
//        sqlstm.occurs = (unsigned int  )0;
//        sqlstm.sqhstv[0] = (         void  *)v_value;
//        sqlstm.sqhstl[0] = (unsigned int  )51;
//        sqlstm.sqhsts[0] = (         int  )0;
//        sqlstm.sqindv[0] = (         void  *)0;
//        sqlstm.sqinds[0] = (         int  )0;
//        sqlstm.sqharm[0] = (unsigned int  )0;
//        sqlstm.sqadto[0] = (unsigned short )0;
//        sqlstm.sqtdso[0] = (unsigned short )0;
//        sqlstm.sqhstv[1] = (         void  *)v_paraid;
//        sqlstm.sqhstl[1] = (unsigned int  )31;
//        sqlstm.sqhsts[1] = (         int  )0;
//        sqlstm.sqindv[1] = (         void  *)0;
//        sqlstm.sqinds[1] = (         int  )0;
//        sqlstm.sqharm[1] = (unsigned int  )0;
//        sqlstm.sqadto[1] = (unsigned short )0;
//        sqlstm.sqtdso[1] = (unsigned short )0;
//        sqlstm.sqphsv = sqlstm.sqhstv;
//        sqlstm.sqphsl = sqlstm.sqhstl;
//        sqlstm.sqphss = sqlstm.sqhsts;
//        sqlstm.sqpind = sqlstm.sqindv;
//        sqlstm.sqpins = sqlstm.sqinds;
//        sqlstm.sqparm = sqlstm.sqharm;
//        sqlstm.sqparc = sqlstm.sqharc;
//        sqlstm.sqpadto = sqlstm.sqadto;
//        sqlstm.sqptdso = sqlstm.sqtdso;
//        sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
//    }
//    
//    
//    
//    if (sqlca.sqlcode!=0)
//    {
//   		strcpy(value,"");
//   		sprintf(msg,"取得参数 %s 失败：失败代码：%d,%s \n",v_paraid,sqlca.sqlcode,sqlca.sqlerrm.sqlerrmc);
//   		writelog(msg,2);
//   		/* EXEC SQL rollback; */
//        
//        {
//            struct sqlexd sqlstm;
//            sqlorat((void **)0, &sqlctx, &oraca);
//            sqlstm.sqlvsn = 10;
//            sqlstm.arrsiz = 4;
//            sqlstm.sqladtp = &sqladt;
//            sqlstm.sqltdsp = &sqltds;
//            sqlstm.iters = (unsigned int  )1;
//            sqlstm.offset = (unsigned int  )105;
//            sqlstm.cud = sqlcud0;
//            sqlstm.sqlest = (unsigned char  *)&sqlca;
//            sqlstm.sqlety = (unsigned short)256;
//            sqlstm.occurs = (unsigned int  )0;
//            sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
//        }
//        
//        
//   		return 0;
//   		
//    }
//    
//    strcpy(value,Allt(v_value));
//    
//    printf("取得参数 %s 的值为：%s \n",v_paraid,value);
//    /* EXEC SQL commit; */
//    
//    {
//        struct sqlexd sqlstm;
//        sqlorat((void **)0, &sqlctx, &oraca);
//        sqlstm.sqlvsn = 10;
//        sqlstm.arrsiz = 4;
//        sqlstm.sqladtp = &sqladt;
//        sqlstm.sqltdsp = &sqltds;
//        sqlstm.iters = (unsigned int  )1;
//        sqlstm.offset = (unsigned int  )120;
//        sqlstm.cud = sqlcud0;
//        sqlstm.sqlest = (unsigned char  *)&sqlca;
//        sqlstm.sqlety = (unsigned short)256;
//        sqlstm.occurs = (unsigned int  )0;
//        sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
//    }
//    
//    
//    return 1;
//    
//}
//
///*修改系统参数*/
//int UpdatePara(char *paraid,char *value)
//{
//    /* EXEC SQL BEGIN DECLARE SECTION; */
//    
//    char v_value[50+1];
//    char v_paraid[30+1];
//    
//    int d_count;
//    /* EXEC SQL END DECLARE SECTION; */
//    
//    
//    char msg[2048];
//    
//    strcpy(v_paraid,paraid);
//    strcpy(v_value,value);
//    
//    d_count=0;
//    
//    /* EXEC SQL select count(*) into :d_count from basic_para
//     where para_id=:v_paraid; */
//    
//    {
//        struct sqlexd sqlstm;
//        sqlorat((void **)0, &sqlctx, &oraca);
//        sqlstm.sqlvsn = 10;
//        sqlstm.arrsiz = 4;
//        sqlstm.sqladtp = &sqladt;
//        sqlstm.sqltdsp = &sqltds;
//        sqlstm.stmt = "select count(*)  into :b0  from basic_para where para_id=\
//        :b1";
//        sqlstm.iters = (unsigned int  )1;
//        sqlstm.offset = (unsigned int  )135;
//        sqlstm.selerr = (unsigned short)1;
//        sqlstm.cud = sqlcud0;
//        sqlstm.sqlest = (unsigned char  *)&sqlca;
//        sqlstm.sqlety = (unsigned short)256;
//        sqlstm.occurs = (unsigned int  )0;
//        sqlstm.sqhstv[0] = (         void  *)&d_count;
//        sqlstm.sqhstl[0] = (unsigned int  )sizeof(int);
//        sqlstm.sqhsts[0] = (         int  )0;
//        sqlstm.sqindv[0] = (         void  *)0;
//        sqlstm.sqinds[0] = (         int  )0;
//        sqlstm.sqharm[0] = (unsigned int  )0;
//        sqlstm.sqadto[0] = (unsigned short )0;
//        sqlstm.sqtdso[0] = (unsigned short )0;
//        sqlstm.sqhstv[1] = (         void  *)v_paraid;
//        sqlstm.sqhstl[1] = (unsigned int  )31;
//        sqlstm.sqhsts[1] = (         int  )0;
//        sqlstm.sqindv[1] = (         void  *)0;
//        sqlstm.sqinds[1] = (         int  )0;
//        sqlstm.sqharm[1] = (unsigned int  )0;
//        sqlstm.sqadto[1] = (unsigned short )0;
//        sqlstm.sqtdso[1] = (unsigned short )0;
//        sqlstm.sqphsv = sqlstm.sqhstv;
//        sqlstm.sqphsl = sqlstm.sqhstl;
//        sqlstm.sqphss = sqlstm.sqhsts;
//        sqlstm.sqpind = sqlstm.sqindv;
//        sqlstm.sqpins = sqlstm.sqinds;
//        sqlstm.sqparm = sqlstm.sqharm;
//        sqlstm.sqparc = sqlstm.sqharc;
//        sqlstm.sqpadto = sqlstm.sqadto;
//        sqlstm.sqptdso = sqlstm.sqtdso;
//        sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
//    }
//    
//    
//    if (d_count==0)
//    {
//   		sprintf(msg,"参数 %s 在系统参数表内不存在！\n",v_paraid);
//   		writelog(msg,2);
//   		return 0;
//    }
//    /* EXEC SQL update basic_para set para_value=:v_value
//     where para_id=:v_paraid; */
//    
//    {
//        struct sqlexd sqlstm;
//        sqlorat((void **)0, &sqlctx, &oraca);
//        sqlstm.sqlvsn = 10;
//        sqlstm.arrsiz = 4;
//        sqlstm.sqladtp = &sqladt;
//        sqlstm.sqltdsp = &sqltds;
//        sqlstm.stmt = "update basic_para  set para_value=:b0 where para_id=:b1";
//        sqlstm.iters = (unsigned int  )1;
//        sqlstm.offset = (unsigned int  )158;
//        sqlstm.cud = sqlcud0;
//        sqlstm.sqlest = (unsigned char  *)&sqlca;
//        sqlstm.sqlety = (unsigned short)256;
//        sqlstm.occurs = (unsigned int  )0;
//        sqlstm.sqhstv[0] = (         void  *)v_value;
//        sqlstm.sqhstl[0] = (unsigned int  )51;
//        sqlstm.sqhsts[0] = (         int  )0;
//        sqlstm.sqindv[0] = (         void  *)0;
//        sqlstm.sqinds[0] = (         int  )0;
//        sqlstm.sqharm[0] = (unsigned int  )0;
//        sqlstm.sqadto[0] = (unsigned short )0;
//        sqlstm.sqtdso[0] = (unsigned short )0;
//        sqlstm.sqhstv[1] = (         void  *)v_paraid;
//        sqlstm.sqhstl[1] = (unsigned int  )31;
//        sqlstm.sqhsts[1] = (         int  )0;
//        sqlstm.sqindv[1] = (         void  *)0;
//        sqlstm.sqinds[1] = (         int  )0;
//        sqlstm.sqharm[1] = (unsigned int  )0;
//        sqlstm.sqadto[1] = (unsigned short )0;
//        sqlstm.sqtdso[1] = (unsigned short )0;
//        sqlstm.sqphsv = sqlstm.sqhstv;
//        sqlstm.sqphsl = sqlstm.sqhstl;
//        sqlstm.sqphss = sqlstm.sqhsts;
//        sqlstm.sqpind = sqlstm.sqindv;
//        sqlstm.sqpins = sqlstm.sqinds;
//        sqlstm.sqparm = sqlstm.sqharm;
//        sqlstm.sqparc = sqlstm.sqharc;
//        sqlstm.sqpadto = sqlstm.sqadto;
//        sqlstm.sqptdso = sqlstm.sqtdso;
//        sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
//    }
//    
//    
//    
//    if (sqlca.sqlcode!=0)
//    {
//   		sprintf(msg,"修改参数 %s 失败：失败代码：%d,%s \n",v_paraid,sqlca.sqlcode,sqlca.sqlerrm.sqlerrmc);
//   		writelog(msg,2);
//   		/* EXEC SQL rollback; */
//        
//        {
//            struct sqlexd sqlstm;
//            sqlorat((void **)0, &sqlctx, &oraca);
//            sqlstm.sqlvsn = 10;
//            sqlstm.arrsiz = 4;
//            sqlstm.sqladtp = &sqladt;
//            sqlstm.sqltdsp = &sqltds;
//            sqlstm.iters = (unsigned int  )1;
//            sqlstm.offset = (unsigned int  )181;
//            sqlstm.cud = sqlcud0;
//            sqlstm.sqlest = (unsigned char  *)&sqlca;
//            sqlstm.sqlety = (unsigned short)256;
//            sqlstm.occurs = (unsigned int  )0;
//            sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
//        }
//        
//        
//   		return 0;
//    }
//    /* EXEC SQL commit; */
//    
//    {
//        struct sqlexd sqlstm;
//        sqlorat((void **)0, &sqlctx, &oraca);
//        sqlstm.sqlvsn = 10;
//        sqlstm.arrsiz = 4;
//        sqlstm.sqladtp = &sqladt;
//        sqlstm.sqltdsp = &sqltds;
//        sqlstm.iters = (unsigned int  )1;
//        sqlstm.offset = (unsigned int  )196;
//        sqlstm.cud = sqlcud0;
//        sqlstm.sqlest = (unsigned char  *)&sqlca;
//        sqlstm.sqlety = (unsigned short)256;
//        sqlstm.occurs = (unsigned int  )0;
//        sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
//    }
//    
//    
//    return 1;
//    
//}
//
//int SndMobileMsg(char * begdatetime)
//{
//	/* EXEC SQL BEGIN DECLARE SECTION; */
//    
//    char v_sysstan[10];
//    char v_txdate[12];
//    char v_pro_time[20];
//    char v_txcode[6];
//    char sms_concode[6];
//    char v_callno[40];
//    char v_describe[128];
//    char v_mobileflg[6];
//    char v_mobilecode[100];
//    char v_trans_mercode[20];
//    char v_mercode[20];
//    char v_cdno[50];
//    char v_incardno[60];
//    char v_routeid[10];
//    char v_samno[20+1];
//    
//    float totalamount;
//    int d_num;
//    int dd_num;
//    long l_amount=0;
//    
//    
//    char v_sndtimectl[20];
//    int ctl_num;
//    int total_num;
//    
//    char v_sndmsg[1024];
//    float nowamount;
//    
//    char v_djtdt[20];
//    char enddttime[20];
//    
//	/* EXEC SQL END DECLARE SECTION; */
//    
//	
//	int msg_num=0;
//	int tmp_num=0;
//	int djt_num=0;
//	int cd_num=0;
//	int cd_len=0;
//	char tmp[256];
//	char tmp_cdno[20];
//	
//	char riderinfo[256];
//	
//	char months[4];
//	char day[4];
//	char hours[4];
//	char mins[4];
//	char msg[2048];
//	char djt_str[256];
//	int i=0;
//	
//	strcpy(v_sndtimectl,begdatetime);
//	
//    
//	/*取系统参数send_msg_trd_date_ctl*/
//	if (v_sndtimectl[0]=='A')
//	{
//		if (GetPara("send_msg_trd_date_ctl",v_sndtimectl)!=1)
//		{
//			return 0;
//		}
//	}
//	
//	memset(v_djtdt,0,20);
//	memcpy(v_djtdt,v_sndtimectl,8);
//	
//	/*检索交易中的成功交易*/
//	/* EXEC SQL select count(*),nvl(max(msg_update_time),'0') into :d_num,:enddttime
//     from ctl_trade_list_sms
//     where msg_update_time>:v_sndtimectl and v_tradestau='00' and v_source='1'
//     and v_succflag='0'; */
//    
//    {
//        struct sqlexd sqlstm;
//        sqlorat((void **)0, &sqlctx, &oraca);
//        sqlstm.sqlvsn = 10;
//        sqlstm.arrsiz = 4;
//        sqlstm.sqladtp = &sqladt;
//        sqlstm.sqltdsp = &sqltds;
//        sqlstm.stmt = "select count(*)  ,nvl(max(msg_update_time),'0') into :b0,:b\
//        1  from ctl_trade_list_sms where (((msg_update_time>:b2 and v_tradestau='00')\
//        and v_source='1') and v_succflag='0')";
//        sqlstm.iters = (unsigned int  )1;
//        sqlstm.offset = (unsigned int  )211;
//        sqlstm.selerr = (unsigned short)1;
//        sqlstm.cud = sqlcud0;
//        sqlstm.sqlest = (unsigned char  *)&sqlca;
//        sqlstm.sqlety = (unsigned short)256;
//        sqlstm.occurs = (unsigned int  )0;
//        sqlstm.sqhstv[0] = (         void  *)&d_num;
//        sqlstm.sqhstl[0] = (unsigned int  )sizeof(int);
//        sqlstm.sqhsts[0] = (         int  )0;
//        sqlstm.sqindv[0] = (         void  *)0;
//        sqlstm.sqinds[0] = (         int  )0;
//        sqlstm.sqharm[0] = (unsigned int  )0;
//        sqlstm.sqadto[0] = (unsigned short )0;
//        sqlstm.sqtdso[0] = (unsigned short )0;
//        sqlstm.sqhstv[1] = (         void  *)enddttime;
//        sqlstm.sqhstl[1] = (unsigned int  )20;
//        sqlstm.sqhsts[1] = (         int  )0;
//        sqlstm.sqindv[1] = (         void  *)0;
//        sqlstm.sqinds[1] = (         int  )0;
//        sqlstm.sqharm[1] = (unsigned int  )0;
//        sqlstm.sqadto[1] = (unsigned short )0;
//        sqlstm.sqtdso[1] = (unsigned short )0;
//        sqlstm.sqhstv[2] = (         void  *)v_sndtimectl;
//        sqlstm.sqhstl[2] = (unsigned int  )20;
//        sqlstm.sqhsts[2] = (         int  )0;
//        sqlstm.sqindv[2] = (         void  *)0;
//        sqlstm.sqinds[2] = (         int  )0;
//        sqlstm.sqharm[2] = (unsigned int  )0;
//        sqlstm.sqadto[2] = (unsigned short )0;
//        sqlstm.sqtdso[2] = (unsigned short )0;
//        sqlstm.sqphsv = sqlstm.sqhstv;
//        sqlstm.sqphsl = sqlstm.sqhstl;
//        sqlstm.sqphss = sqlstm.sqhsts;
//        sqlstm.sqpind = sqlstm.sqindv;
//        sqlstm.sqpins = sqlstm.sqinds;
//        sqlstm.sqparm = sqlstm.sqharm;
//        sqlstm.sqparc = sqlstm.sqharc;
//        sqlstm.sqpadto = sqlstm.sqadto;
//        sqlstm.sqptdso = sqlstm.sqtdso;
//        sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
//    }
//    
//    
//	
//	if (sqlca.sqlcode!=0)
//	{
//		sprintf(msg,"检索新增交易失败,起始时间:%s！,错误代码:%s!",v_sndtimectl,sqlca.sqlcode);
//		writelog(msg,2);
//		return 0;
//	}
//	if (d_num<=0)
//		return 1;
//	
//	sleep(1);
//	
//	/* EXEC SQL select count(*) into :d_num from ctl_trade_list_sms
//     where (msg_update_time > :v_sndtimectl) and (msg_update_time<=:enddttime)
//     and v_tradestau='00' and v_source='1'; */
//    
//    {
//        struct sqlexd sqlstm;
//        sqlorat((void **)0, &sqlctx, &oraca);
//        sqlstm.sqlvsn = 10;
//        sqlstm.arrsiz = 4;
//        sqlstm.sqladtp = &sqladt;
//        sqlstm.sqltdsp = &sqltds;
//        sqlstm.stmt = "select count(*)  into :b0  from ctl_trade_list_sms where ((\
//        (msg_update_time>:b1 and msg_update_time<=:b2) and v_tradestau='00') and v_so\
//        urce='1')";
//        sqlstm.iters = (unsigned int  )1;
//        sqlstm.offset = (unsigned int  )238;
//        sqlstm.selerr = (unsigned short)1;
//        sqlstm.cud = sqlcud0;
//        sqlstm.sqlest = (unsigned char  *)&sqlca;
//        sqlstm.sqlety = (unsigned short)256;
//        sqlstm.occurs = (unsigned int  )0;
//        sqlstm.sqhstv[0] = (         void  *)&d_num;
//        sqlstm.sqhstl[0] = (unsigned int  )sizeof(int);
//        sqlstm.sqhsts[0] = (         int  )0;
//        sqlstm.sqindv[0] = (         void  *)0;
//        sqlstm.sqinds[0] = (         int  )0;
//        sqlstm.sqharm[0] = (unsigned int  )0;
//        sqlstm.sqadto[0] = (unsigned short )0;
//        sqlstm.sqtdso[0] = (unsigned short )0;
//        sqlstm.sqhstv[1] = (         void  *)v_sndtimectl;
//        sqlstm.sqhstl[1] = (unsigned int  )20;
//        sqlstm.sqhsts[1] = (         int  )0;
//        sqlstm.sqindv[1] = (         void  *)0;
//        sqlstm.sqinds[1] = (         int  )0;
//        sqlstm.sqharm[1] = (unsigned int  )0;
//        sqlstm.sqadto[1] = (unsigned short )0;
//        sqlstm.sqtdso[1] = (unsigned short )0;
//        sqlstm.sqhstv[2] = (         void  *)enddttime;
//        sqlstm.sqhstl[2] = (unsigned int  )20;
//        sqlstm.sqhsts[2] = (         int  )0;
//        sqlstm.sqindv[2] = (         void  *)0;
//        sqlstm.sqinds[2] = (         int  )0;
//        sqlstm.sqharm[2] = (unsigned int  )0;
//        sqlstm.sqadto[2] = (unsigned short )0;
//        sqlstm.sqtdso[2] = (unsigned short )0;
//        sqlstm.sqphsv = sqlstm.sqhstv;
//        sqlstm.sqphsl = sqlstm.sqhstl;
//        sqlstm.sqphss = sqlstm.sqhsts;
//        sqlstm.sqpind = sqlstm.sqindv;
//        sqlstm.sqpins = sqlstm.sqinds;
//        sqlstm.sqparm = sqlstm.sqharm;
//        sqlstm.sqparc = sqlstm.sqharc;
//        sqlstm.sqpadto = sqlstm.sqadto;
//        sqlstm.sqptdso = sqlstm.sqtdso;
//        sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
//    }
//    
//    
//	
//	if (sqlca.sqlcode!=0)
//	{
//		sprintf(msg,"检索新增交易失败,起始时间:%s！,错误代码:%s!",v_sndtimectl,sqlca.sqlcode);
//		writelog(msg,2);
//		return 0;
//	}
//	
//	/* EXEC SQL declare msg_cur cursor for
//     select v_sysstan,v_txdate,v_pro_time,v_txcode,sms_concode,
//     nvl(v_callno,' '),to_number(nvl(v_txamt,0)),nvl(v_describe,' '),
//     nvl(v_mobileflg,' '),nvl(v_mobilecode,' '),nvl(v_mercode,'0'),
//     nvl(v_trans_mercode,'0'),nvl(v_cdno,'0'),nvl(v_incardno,' '),
//     nvl(v_samno,' ')
//     from ctl_trade_list_sms
//     where (msg_update_time > :v_sndtimectl) and (msg_update_time<=:enddttime)
//     and v_tradestau='00'
//     and v_source='1' and v_succflag='0'; */
//    
//	/* EXEC SQL open msg_cur; */
//    
//    {
//        struct sqlexd sqlstm;
//        sqlorat((void **)0, &sqlctx, &oraca);
//        sqlstm.sqlvsn = 10;
//        sqlstm.arrsiz = 4;
//        sqlstm.sqladtp = &sqladt;
//        sqlstm.sqltdsp = &sqltds;
//        sqlstm.stmt = sq0013;
//        sqlstm.iters = (unsigned int  )1;
//        sqlstm.offset = (unsigned int  )265;
//        sqlstm.selerr = (unsigned short)1;
//        sqlstm.cud = sqlcud0;
//        sqlstm.sqlest = (unsigned char  *)&sqlca;
//        sqlstm.sqlety = (unsigned short)256;
//        sqlstm.occurs = (unsigned int  )0;
//        sqlstm.sqhstv[0] = (         void  *)v_sndtimectl;
//        sqlstm.sqhstl[0] = (unsigned int  )20;
//        sqlstm.sqhsts[0] = (         int  )0;
//        sqlstm.sqindv[0] = (         void  *)0;
//        sqlstm.sqinds[0] = (         int  )0;
//        sqlstm.sqharm[0] = (unsigned int  )0;
//        sqlstm.sqadto[0] = (unsigned short )0;
//        sqlstm.sqtdso[0] = (unsigned short )0;
//        sqlstm.sqhstv[1] = (         void  *)enddttime;
//        sqlstm.sqhstl[1] = (unsigned int  )20;
//        sqlstm.sqhsts[1] = (         int  )0;
//        sqlstm.sqindv[1] = (         void  *)0;
//        sqlstm.sqinds[1] = (         int  )0;
//        sqlstm.sqharm[1] = (unsigned int  )0;
//        sqlstm.sqadto[1] = (unsigned short )0;
//        sqlstm.sqtdso[1] = (unsigned short )0;
//        sqlstm.sqphsv = sqlstm.sqhstv;
//        sqlstm.sqphsl = sqlstm.sqhstl;
//        sqlstm.sqphss = sqlstm.sqhsts;
//        sqlstm.sqpind = sqlstm.sqindv;
//        sqlstm.sqpins = sqlstm.sqinds;
//        sqlstm.sqparm = sqlstm.sqharm;
//        sqlstm.sqparc = sqlstm.sqharc;
//        sqlstm.sqpadto = sqlstm.sqadto;
//        sqlstm.sqptdso = sqlstm.sqtdso;
//        sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
//    }
//    
//    
//	
//	dd_num=0;
//	while(1)
//	{
//		/* EXEC SQL fetch msg_cur into :v_sysstan,:v_txdate,:v_pro_time,:v_txcode,:sms_concode,
//         :v_callno,:l_amount,:v_describe,:v_mobileflg,:v_mobilecode,:v_mercode,:v_trans_mercode,
//         :v_cdno,:v_incardno,:v_samno; */
//        
//        {
//            struct sqlexd sqlstm;
//            sqlorat((void **)0, &sqlctx, &oraca);
//            sqlstm.sqlvsn = 10;
//            sqlstm.arrsiz = 15;
//            sqlstm.sqladtp = &sqladt;
//            sqlstm.sqltdsp = &sqltds;
//            sqlstm.iters = (unsigned int  )1;
//            sqlstm.offset = (unsigned int  )288;
//            sqlstm.cud = sqlcud0;
//            sqlstm.sqlest = (unsigned char  *)&sqlca;
//            sqlstm.sqlety = (unsigned short)256;
//            sqlstm.occurs = (unsigned int  )0;
//            sqlstm.sqhstv[0] = (         void  *)v_sysstan;
//            sqlstm.sqhstl[0] = (unsigned int  )10;
//            sqlstm.sqhsts[0] = (         int  )0;
//            sqlstm.sqindv[0] = (         void  *)0;
//            sqlstm.sqinds[0] = (         int  )0;
//            sqlstm.sqharm[0] = (unsigned int  )0;
//            sqlstm.sqadto[0] = (unsigned short )0;
//            sqlstm.sqtdso[0] = (unsigned short )0;
//            sqlstm.sqhstv[1] = (         void  *)v_txdate;
//            sqlstm.sqhstl[1] = (unsigned int  )12;
//            sqlstm.sqhsts[1] = (         int  )0;
//            sqlstm.sqindv[1] = (         void  *)0;
//            sqlstm.sqinds[1] = (         int  )0;
//            sqlstm.sqharm[1] = (unsigned int  )0;
//            sqlstm.sqadto[1] = (unsigned short )0;
//            sqlstm.sqtdso[1] = (unsigned short )0;
//            sqlstm.sqhstv[2] = (         void  *)v_pro_time;
//            sqlstm.sqhstl[2] = (unsigned int  )20;
//            sqlstm.sqhsts[2] = (         int  )0;
//            sqlstm.sqindv[2] = (         void  *)0;
//            sqlstm.sqinds[2] = (         int  )0;
//            sqlstm.sqharm[2] = (unsigned int  )0;
//            sqlstm.sqadto[2] = (unsigned short )0;
//            sqlstm.sqtdso[2] = (unsigned short )0;
//            sqlstm.sqhstv[3] = (         void  *)v_txcode;
//            sqlstm.sqhstl[3] = (unsigned int  )6;
//            sqlstm.sqhsts[3] = (         int  )0;
//            sqlstm.sqindv[3] = (         void  *)0;
//            sqlstm.sqinds[3] = (         int  )0;
//            sqlstm.sqharm[3] = (unsigned int  )0;
//            sqlstm.sqadto[3] = (unsigned short )0;
//            sqlstm.sqtdso[3] = (unsigned short )0;
//            sqlstm.sqhstv[4] = (         void  *)sms_concode;
//            sqlstm.sqhstl[4] = (unsigned int  )6;
//            sqlstm.sqhsts[4] = (         int  )0;
//            sqlstm.sqindv[4] = (         void  *)0;
//            sqlstm.sqinds[4] = (         int  )0;
//            sqlstm.sqharm[4] = (unsigned int  )0;
//            sqlstm.sqadto[4] = (unsigned short )0;
//            sqlstm.sqtdso[4] = (unsigned short )0;
//            sqlstm.sqhstv[5] = (         void  *)v_callno;
//            sqlstm.sqhstl[5] = (unsigned int  )40;
//            sqlstm.sqhsts[5] = (         int  )0;
//            sqlstm.sqindv[5] = (         void  *)0;
//            sqlstm.sqinds[5] = (         int  )0;
//            sqlstm.sqharm[5] = (unsigned int  )0;
//            sqlstm.sqadto[5] = (unsigned short )0;
//            sqlstm.sqtdso[5] = (unsigned short )0;
//            sqlstm.sqhstv[6] = (         void  *)&l_amount;
//            sqlstm.sqhstl[6] = (unsigned int  )sizeof(long);
//            sqlstm.sqhsts[6] = (         int  )0;
//            sqlstm.sqindv[6] = (         void  *)0;
//            sqlstm.sqinds[6] = (         int  )0;
//            sqlstm.sqharm[6] = (unsigned int  )0;
//            sqlstm.sqadto[6] = (unsigned short )0;
//            sqlstm.sqtdso[6] = (unsigned short )0;
//            sqlstm.sqhstv[7] = (         void  *)v_describe;
//            sqlstm.sqhstl[7] = (unsigned int  )128;
//            sqlstm.sqhsts[7] = (         int  )0;
//            sqlstm.sqindv[7] = (         void  *)0;
//            sqlstm.sqinds[7] = (         int  )0;
//            sqlstm.sqharm[7] = (unsigned int  )0;
//            sqlstm.sqadto[7] = (unsigned short )0;
//            sqlstm.sqtdso[7] = (unsigned short )0;
//            sqlstm.sqhstv[8] = (         void  *)v_mobileflg;
//            sqlstm.sqhstl[8] = (unsigned int  )6;
//            sqlstm.sqhsts[8] = (         int  )0;
//            sqlstm.sqindv[8] = (         void  *)0;
//            sqlstm.sqinds[8] = (         int  )0;
//            sqlstm.sqharm[8] = (unsigned int  )0;
//            sqlstm.sqadto[8] = (unsigned short )0;
//            sqlstm.sqtdso[8] = (unsigned short )0;
//            sqlstm.sqhstv[9] = (         void  *)v_mobilecode;
//            sqlstm.sqhstl[9] = (unsigned int  )100;
//            sqlstm.sqhsts[9] = (         int  )0;
//            sqlstm.sqindv[9] = (         void  *)0;
//            sqlstm.sqinds[9] = (         int  )0;
//            sqlstm.sqharm[9] = (unsigned int  )0;
//            sqlstm.sqadto[9] = (unsigned short )0;
//            sqlstm.sqtdso[9] = (unsigned short )0;
//            sqlstm.sqhstv[10] = (         void  *)v_mercode;
//            sqlstm.sqhstl[10] = (unsigned int  )20;
//            sqlstm.sqhsts[10] = (         int  )0;
//            sqlstm.sqindv[10] = (         void  *)0;
//            sqlstm.sqinds[10] = (         int  )0;
//            sqlstm.sqharm[10] = (unsigned int  )0;
//            sqlstm.sqadto[10] = (unsigned short )0;
//            sqlstm.sqtdso[10] = (unsigned short )0;
//            sqlstm.sqhstv[11] = (         void  *)v_trans_mercode;
//            sqlstm.sqhstl[11] = (unsigned int  )20;
//            sqlstm.sqhsts[11] = (         int  )0;
//            sqlstm.sqindv[11] = (         void  *)0;
//            sqlstm.sqinds[11] = (         int  )0;
//            sqlstm.sqharm[11] = (unsigned int  )0;
//            sqlstm.sqadto[11] = (unsigned short )0;
//            sqlstm.sqtdso[11] = (unsigned short )0;
//            sqlstm.sqhstv[12] = (         void  *)v_cdno;
//            sqlstm.sqhstl[12] = (unsigned int  )50;
//            sqlstm.sqhsts[12] = (         int  )0;
//            sqlstm.sqindv[12] = (         void  *)0;
//            sqlstm.sqinds[12] = (         int  )0;
//            sqlstm.sqharm[12] = (unsigned int  )0;
//            sqlstm.sqadto[12] = (unsigned short )0;
//            sqlstm.sqtdso[12] = (unsigned short )0;
//            sqlstm.sqhstv[13] = (         void  *)v_incardno;
//            sqlstm.sqhstl[13] = (unsigned int  )60;
//            sqlstm.sqhsts[13] = (         int  )0;
//            sqlstm.sqindv[13] = (         void  *)0;
//            sqlstm.sqinds[13] = (         int  )0;
//            sqlstm.sqharm[13] = (unsigned int  )0;
//            sqlstm.sqadto[13] = (unsigned short )0;
//            sqlstm.sqtdso[13] = (unsigned short )0;
//            sqlstm.sqhstv[14] = (         void  *)v_samno;
//            sqlstm.sqhstl[14] = (unsigned int  )21;
//            sqlstm.sqhsts[14] = (         int  )0;
//            sqlstm.sqindv[14] = (         void  *)0;
//            sqlstm.sqinds[14] = (         int  )0;
//            sqlstm.sqharm[14] = (unsigned int  )0;
//            sqlstm.sqadto[14] = (unsigned short )0;
//            sqlstm.sqtdso[14] = (unsigned short )0;
//            sqlstm.sqphsv = sqlstm.sqhstv;
//            sqlstm.sqphsl = sqlstm.sqhstl;
//            sqlstm.sqphss = sqlstm.sqhsts;
//            sqlstm.sqpind = sqlstm.sqindv;
//            sqlstm.sqpins = sqlstm.sqinds;
//            sqlstm.sqparm = sqlstm.sqharm;
//            sqlstm.sqparc = sqlstm.sqharc;
//            sqlstm.sqpadto = sqlstm.sqadto;
//            sqlstm.sqptdso = sqlstm.sqtdso;
//            sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
//        }
//        
//        
//        Allt(v_sysstan);
//        Allt(v_txdate);
//        Allt(v_pro_time);
//        Allt(v_txcode);
//        Allt(sms_concode);
//        Allt(v_callno);
//        Allt(v_describe);
//        
//        Allt(v_mobileflg);
//        Allt(v_mobilecode);
//        Allt(v_mercode);
//        Allt(v_trans_mercode);
//        Allt(v_cdno);
//        Allt(v_incardno);
//        Allt(v_samno);
//        
//		memset(months,0,4);
//		memset(day,0,4);
//		memset(hours,0,4);
//		memset(mins,0,4);
//		if (v_txdate!=""&&strlen(v_txdate)>=10)
//		{
//			memcpy(months,v_txdate,2);
//			memcpy(day,v_txdate+2,2);
//			memcpy(hours,v_txdate+4,2);
//			memcpy(mins,v_txdate+6,2);
//		}
//		
//		memset(riderinfo,' ',256);
//		msg_num=strlen(v_sysstan);
//		if (msg_num>6)
//			msg_num=6;
//		memcpy(riderinfo,v_sysstan,msg_num);
//		msg_num=strlen(v_txdate);
//		if (msg_num>10)
//			msg_num=10;
//		memcpy(riderinfo+6,v_txdate,msg_num);
//		msg_num=strlen(v_txcode);
//		if (msg_num>3)
//			msg_num=3;
//		memcpy(riderinfo+16,v_txcode,msg_num);
//		msg_num=strlen(sms_concode);
//		if (msg_num>3)
//			msg_num=3;
//		memcpy(riderinfo+19,sms_concode,msg_num);
//		msg_num=strlen(v_mercode);
//		if (msg_num>15)
//			msg_num=15;
//		memcpy(riderinfo+22,v_mercode,msg_num);
//		msg_num=strlen(v_samno);
//		if (msg_num>16)
//			msg_num=16;
//		memcpy(riderinfo+37,v_samno,msg_num);
//		msg_num=strlen(v_callno);
//		if (msg_num>40)
//			msg_num=40;
//		memcpy(riderinfo+53,v_callno,msg_num);
//		riderinfo[93]=0;
//		
//        if ((strcmp(v_mobileflg,"1")==0)&&(strcmp(v_mobilecode,"")!=0))
//        {
//            /* EXEC SQL select nvl(max(ctl_num),-1) into :ctl_num from mobile_msg_ctl
//             where get_mobile_type='1'
//             and (ctl_mercode=:v_mercode or ctl_mercode=:v_trans_mercode or ctl_mercode ='-')
//             and ((ctl_txcode=:v_txcode and ctl_concode=:sms_concode) or(ctl_txcode='-' and ctl_concode='-')); */
//            
//            {
//                struct sqlexd sqlstm;
//                sqlorat((void **)0, &sqlctx, &oraca);
//                sqlstm.sqlvsn = 10;
//                sqlstm.arrsiz = 15;
//                sqlstm.sqladtp = &sqladt;
//                sqlstm.sqltdsp = &sqltds;
//                sqlstm.stmt = "select nvl(max(ctl_num),(-1)) into :b0  from mobile_msg_\
//                ctl where ((get_mobile_type='1' and ((ctl_mercode=:b1 or ctl_mercode=:b2) or \
//                ctl_mercode='-')) and ((ctl_txcode=:b3 and ctl_concode=:b4) or (ctl_txcode='-\
//                ' and ctl_concode='-')))";
//                sqlstm.iters = (unsigned int  )1;
//                sqlstm.offset = (unsigned int  )363;
//                sqlstm.selerr = (unsigned short)1;
//                sqlstm.cud = sqlcud0;
//                sqlstm.sqlest = (unsigned char  *)&sqlca;
//                sqlstm.sqlety = (unsigned short)256;
//                sqlstm.occurs = (unsigned int  )0;
//                sqlstm.sqhstv[0] = (         void  *)&ctl_num;
//                sqlstm.sqhstl[0] = (unsigned int  )sizeof(int);
//                sqlstm.sqhsts[0] = (         int  )0;
//                sqlstm.sqindv[0] = (         void  *)0;
//                sqlstm.sqinds[0] = (         int  )0;
//                sqlstm.sqharm[0] = (unsigned int  )0;
//                sqlstm.sqadto[0] = (unsigned short )0;
//                sqlstm.sqtdso[0] = (unsigned short )0;
//                sqlstm.sqhstv[1] = (         void  *)v_mercode;
//                sqlstm.sqhstl[1] = (unsigned int  )20;
//                sqlstm.sqhsts[1] = (         int  )0;
//                sqlstm.sqindv[1] = (         void  *)0;
//                sqlstm.sqinds[1] = (         int  )0;
//                sqlstm.sqharm[1] = (unsigned int  )0;
//                sqlstm.sqadto[1] = (unsigned short )0;
//                sqlstm.sqtdso[1] = (unsigned short )0;
//                sqlstm.sqhstv[2] = (         void  *)v_trans_mercode;
//                sqlstm.sqhstl[2] = (unsigned int  )20;
//                sqlstm.sqhsts[2] = (         int  )0;
//                sqlstm.sqindv[2] = (         void  *)0;
//                sqlstm.sqinds[2] = (         int  )0;
//                sqlstm.sqharm[2] = (unsigned int  )0;
//                sqlstm.sqadto[2] = (unsigned short )0;
//                sqlstm.sqtdso[2] = (unsigned short )0;
//                sqlstm.sqhstv[3] = (         void  *)v_txcode;
//                sqlstm.sqhstl[3] = (unsigned int  )6;
//                sqlstm.sqhsts[3] = (         int  )0;
//                sqlstm.sqindv[3] = (         void  *)0;
//                sqlstm.sqinds[3] = (         int  )0;
//                sqlstm.sqharm[3] = (unsigned int  )0;
//                sqlstm.sqadto[3] = (unsigned short )0;
//                sqlstm.sqtdso[3] = (unsigned short )0;
//                sqlstm.sqhstv[4] = (         void  *)sms_concode;
//                sqlstm.sqhstl[4] = (unsigned int  )6;
//                sqlstm.sqhsts[4] = (         int  )0;
//                sqlstm.sqindv[4] = (         void  *)0;
//                sqlstm.sqinds[4] = (         int  )0;
//                sqlstm.sqharm[4] = (unsigned int  )0;
//                sqlstm.sqadto[4] = (unsigned short )0;
//                sqlstm.sqtdso[4] = (unsigned short )0;
//                sqlstm.sqphsv = sqlstm.sqhstv;
//                sqlstm.sqphsl = sqlstm.sqhstl;
//                sqlstm.sqphss = sqlstm.sqhsts;
//                sqlstm.sqpind = sqlstm.sqindv;
//                sqlstm.sqpins = sqlstm.sqinds;
//                sqlstm.sqparm = sqlstm.sqharm;
//                sqlstm.sqparc = sqlstm.sqharc;
//                sqlstm.sqpadto = sqlstm.sqadto;
//                sqlstm.sqptdso = sqlstm.sqtdso;
//                sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
//            }
//            
//            
//            if (sqlca.sqlcode!=0)
//            {
//                sprintf(msg,"检索短信配置表失败,错误代码:%d!",v_sndtimectl,sqlca.sqlcode);
//				writelog(msg,2);
//				dd_num++;
//                if (dd_num>=d_num)
//                    break;
//                continue ;
//            }
//            if (ctl_num==-1)
//            {
//                dd_num++;
//                if (dd_num>=d_num)
//                    break;
//                continue ;
//            }
//            
//            /* EXEC SQL select msg_desc,route_id into :v_sndmsg,:v_routeid
//             from mobile_msg_ctl
//             where get_mobile_type='1' and ctl_num =:ctl_num
//             and (ctl_mercode=:v_mercode or ctl_mercode=:v_trans_mercode or ctl_mercode ='-')
//             and ((ctl_txcode=:v_txcode and ctl_concode=:sms_concode) or(ctl_txcode='-' and ctl_concode='-'))
//             and rownum<2; */
//            
//            {
//                struct sqlexd sqlstm;
//                sqlorat((void **)0, &sqlctx, &oraca);
//                sqlstm.sqlvsn = 10;
//                sqlstm.arrsiz = 15;
//                sqlstm.sqladtp = &sqladt;
//                sqlstm.sqltdsp = &sqltds;
//                sqlstm.stmt = "select msg_desc ,route_id into :b0,:b1  from mobile_msg_\
//                ctl where ((((get_mobile_type='1' and ctl_num=:b2) and ((ctl_mercode=:b3 or c\
//                tl_mercode=:b4) or ctl_mercode='-')) and ((ctl_txcode=:b5 and ctl_concode=:b6\
//                ) or (ctl_txcode='-' and ctl_concode='-'))) and rownum<2)";
//                sqlstm.iters = (unsigned int  )1;
//                sqlstm.offset = (unsigned int  )398;
//                sqlstm.selerr = (unsigned short)1;
//                sqlstm.cud = sqlcud0;
//                sqlstm.sqlest = (unsigned char  *)&sqlca;
//                sqlstm.sqlety = (unsigned short)256;
//                sqlstm.occurs = (unsigned int  )0;
//                sqlstm.sqhstv[0] = (         void  *)v_sndmsg;
//                sqlstm.sqhstl[0] = (unsigned int  )1024;
//                sqlstm.sqhsts[0] = (         int  )0;
//                sqlstm.sqindv[0] = (         void  *)0;
//                sqlstm.sqinds[0] = (         int  )0;
//                sqlstm.sqharm[0] = (unsigned int  )0;
//                sqlstm.sqadto[0] = (unsigned short )0;
//                sqlstm.sqtdso[0] = (unsigned short )0;
//                sqlstm.sqhstv[1] = (         void  *)v_routeid;
//                sqlstm.sqhstl[1] = (unsigned int  )10;
//                sqlstm.sqhsts[1] = (         int  )0;
//                sqlstm.sqindv[1] = (         void  *)0;
//                sqlstm.sqinds[1] = (         int  )0;
//                sqlstm.sqharm[1] = (unsigned int  )0;
//                sqlstm.sqadto[1] = (unsigned short )0;
//                sqlstm.sqtdso[1] = (unsigned short )0;
//                sqlstm.sqhstv[2] = (         void  *)&ctl_num;
//                sqlstm.sqhstl[2] = (unsigned int  )sizeof(int);
//                sqlstm.sqhsts[2] = (         int  )0;
//                sqlstm.sqindv[2] = (         void  *)0;
//                sqlstm.sqinds[2] = (         int  )0;
//                sqlstm.sqharm[2] = (unsigned int  )0;
//                sqlstm.sqadto[2] = (unsigned short )0;
//                sqlstm.sqtdso[2] = (unsigned short )0;
//                sqlstm.sqhstv[3] = (         void  *)v_mercode;
//                sqlstm.sqhstl[3] = (unsigned int  )20;
//                sqlstm.sqhsts[3] = (         int  )0;
//                sqlstm.sqindv[3] = (         void  *)0;
//                sqlstm.sqinds[3] = (         int  )0;
//                sqlstm.sqharm[3] = (unsigned int  )0;
//                sqlstm.sqadto[3] = (unsigned short )0;
//                sqlstm.sqtdso[3] = (unsigned short )0;
//                sqlstm.sqhstv[4] = (         void  *)v_trans_mercode;
//                sqlstm.sqhstl[4] = (unsigned int  )20;
//                sqlstm.sqhsts[4] = (         int  )0;
//                sqlstm.sqindv[4] = (         void  *)0;
//                sqlstm.sqinds[4] = (         int  )0;
//                sqlstm.sqharm[4] = (unsigned int  )0;
//                sqlstm.sqadto[4] = (unsigned short )0;
//                sqlstm.sqtdso[4] = (unsigned short )0;
//                sqlstm.sqhstv[5] = (         void  *)v_txcode;
//                sqlstm.sqhstl[5] = (unsigned int  )6;
//                sqlstm.sqhsts[5] = (         int  )0;
//                sqlstm.sqindv[5] = (         void  *)0;
//                sqlstm.sqinds[5] = (         int  )0;
//                sqlstm.sqharm[5] = (unsigned int  )0;
//                sqlstm.sqadto[5] = (unsigned short )0;
//                sqlstm.sqtdso[5] = (unsigned short )0;
//                sqlstm.sqhstv[6] = (         void  *)sms_concode;
//                sqlstm.sqhstl[6] = (unsigned int  )6;
//                sqlstm.sqhsts[6] = (         int  )0;
//                sqlstm.sqindv[6] = (         void  *)0;
//                sqlstm.sqinds[6] = (         int  )0;
//                sqlstm.sqharm[6] = (unsigned int  )0;
//                sqlstm.sqadto[6] = (unsigned short )0;
//                sqlstm.sqtdso[6] = (unsigned short )0;
//                sqlstm.sqphsv = sqlstm.sqhstv;
//                sqlstm.sqphsl = sqlstm.sqhstl;
//                sqlstm.sqphss = sqlstm.sqhsts;
//                sqlstm.sqpind = sqlstm.sqindv;
//                sqlstm.sqpins = sqlstm.sqinds;
//                sqlstm.sqparm = sqlstm.sqharm;
//                sqlstm.sqparc = sqlstm.sqharc;
//                sqlstm.sqpadto = sqlstm.sqadto;
//                sqlstm.sqptdso = sqlstm.sqtdso;
//                sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
//            }
//            
//            
//            Allt(v_sndmsg);
//            Allt(v_routeid);
//            msg_num=strlen(v_sndmsg);
//            
//            djt_num=0;
//            memset(msg,0,2048);
//            for(i=0;i<msg_num;i++)
//            {
//                memset(tmp,0,256);
//                if(v_sndmsg[i]=='#')
//                {
//                    tmp_num=0;
//                    for(i=i+1;i<msg_num;i++)
//                    {
//                        if (v_sndmsg[i]=='#')
//                        {
//                            i++;
//                            break;
//                        }
//                        tmp[tmp_num]=v_sndmsg[i];
//                        tmp_num++;
//                    }
//                    if (tmp_num!=0)
//                    {
//                        if (strcmp(tmp,"M")==0)/*月*/
//                        {
//                            memcpy(msg+djt_num,months,2);
//                            djt_num=djt_num+2;
//                        }
//                        else if(strcmp(tmp,"D")==0)/*日*/
//                        {
//                            memcpy(msg+djt_num,day,2);
//                            djt_num=djt_num+2;
//                        }
//                        else if(strcmp(tmp,"H")==0)/*小时*/
//                        {
//                            memcpy(msg+djt_num,hours,2);
//                            djt_num=djt_num+2;
//                        }
//                        else if(strcmp(tmp,"m")==0)/*分*/
//                        {
//                            memcpy(msg+djt_num,mins,2);
//                            djt_num=djt_num+2;
//                        }
//                        else if (strcmp(tmp,"CDNO")==0)/*转出卡号*/
//                        {
//                            memcpy(msg+djt_num,v_cdno,strlen(v_cdno));
//                            djt_num=djt_num+strlen(v_cdno);
//                        }
//                        else if (strcmp(tmp,"ICDNO")==0)/*转入卡号*/
//                        {
//                            memcpy(msg+djt_num,v_incardno,strlen(v_incardno));
//                            djt_num=djt_num+strlen(v_incardno);
//                        }
//                        else if (strcmp(tmp,"RMB")==0)/*金额*/
//                        {
//                            nowamount=l_amount/100.00;
//                            sprintf(djt_str,"%.2lf 元",nowamount);
//                            memcpy(msg+djt_num,djt_str,strlen(djt_str));
//                            djt_num=djt_num+strlen(djt_str);
//                        }
//                        else if (strcmp(tmp,"TERM")==0)/*终端号码*/
//                        {
//                            memcpy(msg+djt_num,v_callno,strlen(v_callno));
//                            djt_num=djt_num+strlen(v_callno);
//                        }
//                        else if (strcmp(tmp,"DESC")==0)/*交易描述*/
//                        {
//                            memcpy(msg+djt_num,v_describe,strlen(v_describe));
//                            djt_num=djt_num+strlen(v_describe);
//                        }
//                        else if(strcmp(tmp,"TRMB")==0||strcmp(tmp,"TCOUNT")==0) /*当日总金额，当日总笔数*/
//                        {
//                            /* EXEC SQL select count(*),nvl(sum(to_number(v_txamt))/100,0) into :total_num,:totalamount
//                             from ctl_trade_list_sms
//                             where v_callno=:v_callno
//                             and substr(v_pro_time,1,8)=:v_djtdt and v_tradestau='00'
//                             and v_succflag='0'
//                             and v_proccode<>'300000'; */
//                            
//                            {
//                                struct sqlexd sqlstm;
//                                sqlorat((void **)0, &sqlctx, &oraca);
//                                sqlstm.sqlvsn = 10;
//                                sqlstm.arrsiz = 15;
//                                sqlstm.sqladtp = &sqladt;
//                                sqlstm.sqltdsp = &sqltds;
//                                sqlstm.stmt = "select count(*)  ,nvl((sum(to_number(v_txamt))/100),\
//                                0) into :b0,:b1  from ctl_trade_list_sms where ((((v_callno=:b2 and substr(v_\
//                                pro_time,1,8)=:b3) and v_tradestau='00') and v_succflag='0') and v_proccode<>\
//                                '300000')";
//                                sqlstm.iters = (unsigned int  )1;
//                                sqlstm.offset = (unsigned int  )441;
//                                sqlstm.selerr = (unsigned short)1;
//                                sqlstm.cud = sqlcud0;
//                                sqlstm.sqlest = (unsigned char  *)&sqlca;
//                                sqlstm.sqlety = (unsigned short)256;
//                                sqlstm.occurs = (unsigned int  )0;
//                                sqlstm.sqhstv[0] = (         void  *)&total_num;
//                                sqlstm.sqhstl[0] = (unsigned int  )sizeof(int);
//                                sqlstm.sqhsts[0] = (         int  )0;
//                                sqlstm.sqindv[0] = (         void  *)0;
//                                sqlstm.sqinds[0] = (         int  )0;
//                                sqlstm.sqharm[0] = (unsigned int  )0;
//                                sqlstm.sqadto[0] = (unsigned short )0;
//                                sqlstm.sqtdso[0] = (unsigned short )0;
//                                sqlstm.sqhstv[1] = (         void  *)&totalamount;
//                                sqlstm.sqhstl[1] = (unsigned int  )sizeof(float);
//                                sqlstm.sqhsts[1] = (         int  )0;
//                                sqlstm.sqindv[1] = (         void  *)0;
//                                sqlstm.sqinds[1] = (         int  )0;
//                                sqlstm.sqharm[1] = (unsigned int  )0;
//                                sqlstm.sqadto[1] = (unsigned short )0;
//                                sqlstm.sqtdso[1] = (unsigned short )0;
//                                sqlstm.sqhstv[2] = (         void  *)v_callno;
//                                sqlstm.sqhstl[2] = (unsigned int  )40;
//                                sqlstm.sqhsts[2] = (         int  )0;
//                                sqlstm.sqindv[2] = (         void  *)0;
//                                sqlstm.sqinds[2] = (         int  )0;
//                                sqlstm.sqharm[2] = (unsigned int  )0;
//                                sqlstm.sqadto[2] = (unsigned short )0;
//                                sqlstm.sqtdso[2] = (unsigned short )0;
//                                sqlstm.sqhstv[3] = (         void  *)v_djtdt;
//                                sqlstm.sqhstl[3] = (unsigned int  )20;
//                                sqlstm.sqhsts[3] = (         int  )0;
//                                sqlstm.sqindv[3] = (         void  *)0;
//                                sqlstm.sqinds[3] = (         int  )0;
//                                sqlstm.sqharm[3] = (unsigned int  )0;
//                                sqlstm.sqadto[3] = (unsigned short )0;
//                                sqlstm.sqtdso[3] = (unsigned short )0;
//                                sqlstm.sqphsv = sqlstm.sqhstv;
//                                sqlstm.sqphsl = sqlstm.sqhstl;
//                                sqlstm.sqphss = sqlstm.sqhsts;
//                                sqlstm.sqpind = sqlstm.sqindv;
//                                sqlstm.sqpins = sqlstm.sqinds;
//                                sqlstm.sqparm = sqlstm.sqharm;
//                                sqlstm.sqparc = sqlstm.sqharc;
//                                sqlstm.sqpadto = sqlstm.sqadto;
//                                sqlstm.sqptdso = sqlstm.sqtdso;
//                                sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
//                            }
//                            
//                            
//							if (sqlca.sqlcode!=0)
//							{
//								sprintf(msg,"查询总金额总笔数出错,错误代码：%d ！",sqlca.sqlcode);
//								writelog(msg,2);
//								total_num=-1;
//								totalamount=-1;
//							}
//							if (strcmp(tmp,"TRMB")==0)
//							{
//                                sprintf(djt_str,"%.2lf 元",totalamount);
//                                memcpy(msg+djt_num,djt_str,strlen(djt_str));
//                                djt_num=djt_num+strlen(djt_str);
//							}
//							if (strcmp(tmp,"TCOUNT")==0)
//							{
//                                sprintf(djt_str,"%d 笔",total_num);
//                                memcpy(msg+djt_num,djt_str,strlen(djt_str));
//                                djt_num=djt_num+strlen(djt_str);
//							}
//                        }
//                        else if (strstr(tmp,"ICDNO")!=NULL)/*部分转入卡号*/
//                        {
//                            cd_len=strlen(tmp);
//							memset(tmp_cdno,0,20);
//                            if (cd_len==6)
//                            {
//                                tmp_cdno[0]=tmp[5];
//                                sscanf(tmp_cdno,"%d",&cd_num);
//                                cd_len=strlen(v_incardno);
//                                if ((cd_num>0)&&(cd_num<10)&&(cd_num<cd_len))
//                                {
//                                    memset(tmp_cdno,0,20);
//                                    memcpy(tmp_cdno,v_incardno+(cd_len-cd_num),cd_num);
//                                    memcpy(msg+djt_num,tmp_cdno,cd_num);
//                                    djt_num=djt_num+cd_num;
//                                }
//                                else
//                                {
//                                    sprintf(msg,"配置的转换参数不能识别,参数值：%s！",tmp);
//									writelog(msg,2);
//                                }
//                            }
//                            else
//                            {
//						 		sprintf(msg,"配置的转换参数不能识别,参数值：%s！",tmp);
//								writelog(msg,2);
//						 	}
//                        }
//                        else if (strstr(tmp,"CDNO")!=NULL)/*部分转出卡号*/
//                        {
//                            cd_len=strlen(tmp);
//							memset(tmp_cdno,0,20);
//                            if (cd_len==5)
//                            {
//                                tmp_cdno[0]=tmp[4];
//                                sscanf(tmp_cdno,"%d",&cd_num);
//                                cd_len=strlen(v_cdno);
//                                if ((cd_num>0)&&(cd_num<10)&&(cd_num<cd_len))
//                                {
//                                    memset(tmp_cdno,0,20);
//                                    memcpy(tmp_cdno,v_cdno+(cd_len-cd_num),cd_num);
//                                    memcpy(msg+djt_num,tmp_cdno,cd_num);
//                                    djt_num=djt_num+cd_num;
//                                }
//                                else
//                                {
//                                    sprintf(msg,"配置的转换参数不能识别,参数值：%s！",tmp);
//									writelog(msg,2);
//                                }
//                            }
//                            else
//                            {
//						 		sprintf(msg,"配置的转换参数不能识别,参数值：%s！",tmp);
//								writelog(msg,2);
//						 	}
//                        }
//                        else
//						{
//							sprintf(msg,"配置的转换参数不能识别,参数值：%s！",tmp);
//							writelog(msg,2);
//						}
//                    }
//                }
//                if (i>=msg_num)
//                    break;
//                msg[djt_num]=v_sndmsg[i];
//                djt_num++;
//                
//            }
//            
//            printf("v_mobilecode = %s\n",v_mobilecode);
//            printf("msg = %s\n",msg);
//            printf("v_routeid = %s\n",v_routeid);
//            printf("riderinfo = %s\n",riderinfo);
//            
//            SendMsg_api("12345678","123456","B",v_mobilecode,msg,v_routeid,"1",riderinfo);
//        }	/*从交易表得到手机号结束*/
//        else	/*从资料表得到手机号*/
//        {
//            if (strcmp(v_callno,"")==0)/*没有终端号码*/
//            {
//                dd_num++;
//                if (dd_num>=d_num)
//                    break;
//                continue ;
//            }
//  			
//            if (v_callno[0]=='0')/*终端号码带区号*/
//            {
//                /* EXEC SQL select nvl(bangdingmobile,'-') into :v_mobilecode
//                 from cs_sys_terminal
//                 where areano||terminalno = :v_callno
//                 and rownum<2
//                 and bangdingmsg_flag='1'
//                 and trim(bangdingmobile) is not null; */
//                
//                {
//                    struct sqlexd sqlstm;
//                    sqlorat((void **)0, &sqlctx, &oraca);
//                    sqlstm.sqlvsn = 10;
//                    sqlstm.arrsiz = 15;
//                    sqlstm.sqladtp = &sqladt;
//                    sqlstm.sqltdsp = &sqltds;
//                    sqlstm.stmt = "select nvl(bangdingmobile,'-') into :b0  from cs_sys_te\
//                    rminal where ((((areano||terminalno)=:b1 and rownum<2) and bangdingmsg_flag='\
//                    1') and trim(bangdingmobile) is  not null )";
//                    sqlstm.iters = (unsigned int  )1;
//                    sqlstm.offset = (unsigned int  )472;
//                    sqlstm.selerr = (unsigned short)1;
//                    sqlstm.cud = sqlcud0;
//                    sqlstm.sqlest = (unsigned char  *)&sqlca;
//                    sqlstm.sqlety = (unsigned short)256;
//                    sqlstm.occurs = (unsigned int  )0;
//                    sqlstm.sqhstv[0] = (         void  *)v_mobilecode;
//                    sqlstm.sqhstl[0] = (unsigned int  )100;
//                    sqlstm.sqhsts[0] = (         int  )0;
//                    sqlstm.sqindv[0] = (         void  *)0;
//                    sqlstm.sqinds[0] = (         int  )0;
//                    sqlstm.sqharm[0] = (unsigned int  )0;
//                    sqlstm.sqadto[0] = (unsigned short )0;
//                    sqlstm.sqtdso[0] = (unsigned short )0;
//                    sqlstm.sqhstv[1] = (         void  *)v_callno;
//                    sqlstm.sqhstl[1] = (unsigned int  )40;
//                    sqlstm.sqhsts[1] = (         int  )0;
//                    sqlstm.sqindv[1] = (         void  *)0;
//                    sqlstm.sqinds[1] = (         int  )0;
//                    sqlstm.sqharm[1] = (unsigned int  )0;
//                    sqlstm.sqadto[1] = (unsigned short )0;
//                    sqlstm.sqtdso[1] = (unsigned short )0;
//                    sqlstm.sqphsv = sqlstm.sqhstv;
//                    sqlstm.sqphsl = sqlstm.sqhstl;
//                    sqlstm.sqphss = sqlstm.sqhsts;
//                    sqlstm.sqpind = sqlstm.sqindv;
//                    sqlstm.sqpins = sqlstm.sqinds;
//                    sqlstm.sqparm = sqlstm.sqharm;
//                    sqlstm.sqparc = sqlstm.sqharc;
//                    sqlstm.sqpadto = sqlstm.sqadto;
//                    sqlstm.sqptdso = sqlstm.sqtdso;
//                    sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
//                }
//                
//                
//				if (sqlca.sqlcode!=0)
//				{
//					dd_num++;
//                    if (dd_num>=d_num)
//                        break;
//                    continue ;
//				}
//            }
//            else /*终端号码不带区号*/
//            {
//                /* EXEC SQL select nvl(bangdingmobile,'-') into :v_mobilecode
//                 from cs_sys_terminal
//                 where terminalno =:v_callno
//                 and rownum<2
//                 and bangdingmsg_flag='1'
//                 and trim(bangdingmobile) is not null; */
//                
//                {
//                    struct sqlexd sqlstm;
//                    sqlorat((void **)0, &sqlctx, &oraca);
//                    sqlstm.sqlvsn = 10;
//                    sqlstm.arrsiz = 15;
//                    sqlstm.sqladtp = &sqladt;
//                    sqlstm.sqltdsp = &sqltds;
//                    sqlstm.stmt = "select nvl(bangdingmobile,'-') into :b0  from cs_sys_te\
//                    rminal where (((terminalno=:b1 and rownum<2) and bangdingmsg_flag='1') and tr\
//                    im(bangdingmobile) is  not null )";
//                    sqlstm.iters = (unsigned int  )1;
//                    sqlstm.offset = (unsigned int  )495;
//                    sqlstm.selerr = (unsigned short)1;
//                    sqlstm.cud = sqlcud0;
//                    sqlstm.sqlest = (unsigned char  *)&sqlca;
//                    sqlstm.sqlety = (unsigned short)256;
//                    sqlstm.occurs = (unsigned int  )0;
//                    sqlstm.sqhstv[0] = (         void  *)v_mobilecode;
//                    sqlstm.sqhstl[0] = (unsigned int  )100;
//                    sqlstm.sqhsts[0] = (         int  )0;
//                    sqlstm.sqindv[0] = (         void  *)0;
//                    sqlstm.sqinds[0] = (         int  )0;
//                    sqlstm.sqharm[0] = (unsigned int  )0;
//                    sqlstm.sqadto[0] = (unsigned short )0;
//                    sqlstm.sqtdso[0] = (unsigned short )0;
//                    sqlstm.sqhstv[1] = (         void  *)v_callno;
//                    sqlstm.sqhstl[1] = (unsigned int  )40;
//                    sqlstm.sqhsts[1] = (         int  )0;
//                    sqlstm.sqindv[1] = (         void  *)0;
//                    sqlstm.sqinds[1] = (         int  )0;
//                    sqlstm.sqharm[1] = (unsigned int  )0;
//                    sqlstm.sqadto[1] = (unsigned short )0;
//                    sqlstm.sqtdso[1] = (unsigned short )0;
//                    sqlstm.sqphsv = sqlstm.sqhstv;
//                    sqlstm.sqphsl = sqlstm.sqhstl;
//                    sqlstm.sqphss = sqlstm.sqhsts;
//                    sqlstm.sqpind = sqlstm.sqindv;
//                    sqlstm.sqpins = sqlstm.sqinds;
//                    sqlstm.sqparm = sqlstm.sqharm;
//                    sqlstm.sqparc = sqlstm.sqharc;
//                    sqlstm.sqpadto = sqlstm.sqadto;
//                    sqlstm.sqptdso = sqlstm.sqtdso;
//                    sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
//                }
//                
//                
//				if (sqlca.sqlcode!=0)
//				{
//					dd_num++;
//                    if (dd_num>=d_num)
//                        break;
//                    continue ;
//				}
//            }
//            
//            Allt(v_mobilecode);
//			if (v_mobilecode[0]=='-')
//			{
//				dd_num++;
//                if (dd_num>=d_num)
//                    break;
//                continue ;
//			}
//            
//            /* EXEC SQL select nvl(max(ctl_num),-1) into :ctl_num from mobile_msg_ctl
//             where get_mobile_type='2'
//             and (ctl_mercode=:v_mercode or ctl_mercode=:v_trans_mercode or ctl_mercode ='-')
//             and ((ctl_txcode=:v_txcode and ctl_concode=:sms_concode) or(ctl_txcode='-' and ctl_concode='-')); */
//            
//            {
//                struct sqlexd sqlstm;
//                sqlorat((void **)0, &sqlctx, &oraca);
//                sqlstm.sqlvsn = 10;
//                sqlstm.arrsiz = 15;
//                sqlstm.sqladtp = &sqladt;
//                sqlstm.sqltdsp = &sqltds;
//                sqlstm.stmt = "select nvl(max(ctl_num),(-1)) into :b0  from mobile_msg_\
//                ctl where ((get_mobile_type='2' and ((ctl_mercode=:b1 or ctl_mercode=:b2) or \
//                ctl_mercode='-')) and ((ctl_txcode=:b3 and ctl_concode=:b4) or (ctl_txcode='-\
//                ' and ctl_concode='-')))";
//                sqlstm.iters = (unsigned int  )1;
//                sqlstm.offset = (unsigned int  )518;
//                sqlstm.selerr = (unsigned short)1;
//                sqlstm.cud = sqlcud0;
//                sqlstm.sqlest = (unsigned char  *)&sqlca;
//                sqlstm.sqlety = (unsigned short)256;
//                sqlstm.occurs = (unsigned int  )0;
//                sqlstm.sqhstv[0] = (         void  *)&ctl_num;
//                sqlstm.sqhstl[0] = (unsigned int  )sizeof(int);
//                sqlstm.sqhsts[0] = (         int  )0;
//                sqlstm.sqindv[0] = (         void  *)0;
//                sqlstm.sqinds[0] = (         int  )0;
//                sqlstm.sqharm[0] = (unsigned int  )0;
//                sqlstm.sqadto[0] = (unsigned short )0;
//                sqlstm.sqtdso[0] = (unsigned short )0;
//                sqlstm.sqhstv[1] = (         void  *)v_mercode;
//                sqlstm.sqhstl[1] = (unsigned int  )20;
//                sqlstm.sqhsts[1] = (         int  )0;
//                sqlstm.sqindv[1] = (         void  *)0;
//                sqlstm.sqinds[1] = (         int  )0;
//                sqlstm.sqharm[1] = (unsigned int  )0;
//                sqlstm.sqadto[1] = (unsigned short )0;
//                sqlstm.sqtdso[1] = (unsigned short )0;
//                sqlstm.sqhstv[2] = (         void  *)v_trans_mercode;
//                sqlstm.sqhstl[2] = (unsigned int  )20;
//                sqlstm.sqhsts[2] = (         int  )0;
//                sqlstm.sqindv[2] = (         void  *)0;
//                sqlstm.sqinds[2] = (         int  )0;
//                sqlstm.sqharm[2] = (unsigned int  )0;
//                sqlstm.sqadto[2] = (unsigned short )0;
//                sqlstm.sqtdso[2] = (unsigned short )0;
//                sqlstm.sqhstv[3] = (         void  *)v_txcode;
//                sqlstm.sqhstl[3] = (unsigned int  )6;
//                sqlstm.sqhsts[3] = (         int  )0;
//                sqlstm.sqindv[3] = (         void  *)0;
//                sqlstm.sqinds[3] = (         int  )0;
//                sqlstm.sqharm[3] = (unsigned int  )0;
//                sqlstm.sqadto[3] = (unsigned short )0;
//                sqlstm.sqtdso[3] = (unsigned short )0;
//                sqlstm.sqhstv[4] = (         void  *)sms_concode;
//                sqlstm.sqhstl[4] = (unsigned int  )6;
//                sqlstm.sqhsts[4] = (         int  )0;
//                sqlstm.sqindv[4] = (         void  *)0;
//                sqlstm.sqinds[4] = (         int  )0;
//                sqlstm.sqharm[4] = (unsigned int  )0;
//                sqlstm.sqadto[4] = (unsigned short )0;
//                sqlstm.sqtdso[4] = (unsigned short )0;
//                sqlstm.sqphsv = sqlstm.sqhstv;
//                sqlstm.sqphsl = sqlstm.sqhstl;
//                sqlstm.sqphss = sqlstm.sqhsts;
//                sqlstm.sqpind = sqlstm.sqindv;
//                sqlstm.sqpins = sqlstm.sqinds;
//                sqlstm.sqparm = sqlstm.sqharm;
//                sqlstm.sqparc = sqlstm.sqharc;
//                sqlstm.sqpadto = sqlstm.sqadto;
//                sqlstm.sqptdso = sqlstm.sqtdso;
//                sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
//            }
//            
//            
//            if (sqlca.sqlcode!=0)
//            {
//                sprintf(msg,"检索短信配置表失败,错误代码:%d!",v_sndtimectl,sqlca.sqlcode);
//				writelog(msg,2);
//				dd_num++;
//                if (dd_num>=d_num)
//                    break;
//                continue ;
//            }
//            if (ctl_num==-1)
//            {
//                dd_num++;
//                if (dd_num>=d_num)
//                    break;
//                continue ;
//            }
//            
//            /* EXEC SQL select msg_desc,route_id into :v_sndmsg,:v_routeid 
//             from mobile_msg_ctl
//             where get_mobile_type='2' and ctl_num =:ctl_num
//             and (ctl_mercode=:v_mercode or ctl_mercode=:v_trans_mercode or ctl_mercode ='-') 
//             and ((ctl_txcode=:v_txcode and ctl_concode=:sms_concode) or (ctl_txcode='-' and ctl_concode='-'))
//             and rownum<2; */ 
//            
//            {
//                struct sqlexd sqlstm;
//                sqlorat((void **)0, &sqlctx, &oraca);
//                sqlstm.sqlvsn = 10;
//                sqlstm.arrsiz = 15;
//                sqlstm.sqladtp = &sqladt;
//                sqlstm.sqltdsp = &sqltds;
//                sqlstm.stmt = "select msg_desc ,route_id into :b0,:b1  from mobile_msg_\
//                ctl where ((((get_mobile_type='2' and ctl_num=:b2) and ((ctl_mercode=:b3 or c\
//                tl_mercode=:b4) or ctl_mercode='-')) and ((ctl_txcode=:b5 and ctl_concode=:b6\
//                ) or (ctl_txcode='-' and ctl_concode='-'))) and rownum<2)";
//                sqlstm.iters = (unsigned int  )1;
//                sqlstm.offset = (unsigned int  )553;
//                sqlstm.selerr = (unsigned short)1;
//                sqlstm.cud = sqlcud0;
//                sqlstm.sqlest = (unsigned char  *)&sqlca;
//                sqlstm.sqlety = (unsigned short)256;
//                sqlstm.occurs = (unsigned int  )0;
//                sqlstm.sqhstv[0] = (         void  *)v_sndmsg;
//                sqlstm.sqhstl[0] = (unsigned int  )1024;
//                sqlstm.sqhsts[0] = (         int  )0;
//                sqlstm.sqindv[0] = (         void  *)0;
//                sqlstm.sqinds[0] = (         int  )0;
//                sqlstm.sqharm[0] = (unsigned int  )0;
//                sqlstm.sqadto[0] = (unsigned short )0;
//                sqlstm.sqtdso[0] = (unsigned short )0;
//                sqlstm.sqhstv[1] = (         void  *)v_routeid;
//                sqlstm.sqhstl[1] = (unsigned int  )10;
//                sqlstm.sqhsts[1] = (         int  )0;
//                sqlstm.sqindv[1] = (         void  *)0;
//                sqlstm.sqinds[1] = (         int  )0;
//                sqlstm.sqharm[1] = (unsigned int  )0;
//                sqlstm.sqadto[1] = (unsigned short )0;
//                sqlstm.sqtdso[1] = (unsigned short )0;
//                sqlstm.sqhstv[2] = (         void  *)&ctl_num;
//                sqlstm.sqhstl[2] = (unsigned int  )sizeof(int);
//                sqlstm.sqhsts[2] = (         int  )0;
//                sqlstm.sqindv[2] = (         void  *)0;
//                sqlstm.sqinds[2] = (         int  )0;
//                sqlstm.sqharm[2] = (unsigned int  )0;
//                sqlstm.sqadto[2] = (unsigned short )0;
//                sqlstm.sqtdso[2] = (unsigned short )0;
//                sqlstm.sqhstv[3] = (         void  *)v_mercode;
//                sqlstm.sqhstl[3] = (unsigned int  )20;
//                sqlstm.sqhsts[3] = (         int  )0;
//                sqlstm.sqindv[3] = (         void  *)0;
//                sqlstm.sqinds[3] = (         int  )0;
//                sqlstm.sqharm[3] = (unsigned int  )0;
//                sqlstm.sqadto[3] = (unsigned short )0;
//                sqlstm.sqtdso[3] = (unsigned short )0;
//                sqlstm.sqhstv[4] = (         void  *)v_trans_mercode;
//                sqlstm.sqhstl[4] = (unsigned int  )20;
//                sqlstm.sqhsts[4] = (         int  )0;
//                sqlstm.sqindv[4] = (         void  *)0;
//                sqlstm.sqinds[4] = (         int  )0;
//                sqlstm.sqharm[4] = (unsigned int  )0;
//                sqlstm.sqadto[4] = (unsigned short )0;
//                sqlstm.sqtdso[4] = (unsigned short )0;
//                sqlstm.sqhstv[5] = (         void  *)v_txcode;
//                sqlstm.sqhstl[5] = (unsigned int  )6;
//                sqlstm.sqhsts[5] = (         int  )0;
//                sqlstm.sqindv[5] = (         void  *)0;
//                sqlstm.sqinds[5] = (         int  )0;
//                sqlstm.sqharm[5] = (unsigned int  )0;
//                sqlstm.sqadto[5] = (unsigned short )0;
//                sqlstm.sqtdso[5] = (unsigned short )0;
//                sqlstm.sqhstv[6] = (         void  *)sms_concode;
//                sqlstm.sqhstl[6] = (unsigned int  )6;
//                sqlstm.sqhsts[6] = (         int  )0;
//                sqlstm.sqindv[6] = (         void  *)0;
//                sqlstm.sqinds[6] = (         int  )0;
//                sqlstm.sqharm[6] = (unsigned int  )0;
//                sqlstm.sqadto[6] = (unsigned short )0;
//                sqlstm.sqtdso[6] = (unsigned short )0;
//                sqlstm.sqphsv = sqlstm.sqhstv;
//                sqlstm.sqphsl = sqlstm.sqhstl;
//                sqlstm.sqphss = sqlstm.sqhsts;
//                sqlstm.sqpind = sqlstm.sqindv;
//                sqlstm.sqpins = sqlstm.sqinds;
//                sqlstm.sqparm = sqlstm.sqharm;
//                sqlstm.sqparc = sqlstm.sqharc;
//                sqlstm.sqpadto = sqlstm.sqadto;
//                sqlstm.sqptdso = sqlstm.sqtdso;
//                sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
//            }
//            
//            
//            Allt(v_sndmsg);
//            Allt(v_routeid);
//            msg_num=strlen(v_sndmsg);
//            djt_num=0;
//            memset(msg,0,2048);
//            for(i=0;i<msg_num;i++)
//            {
//                memset(tmp,0,256);
//                if(v_sndmsg[i]=='#')
//                {
//                    tmp_num=0;
//                    for(i=i+1;i<msg_num;i++)
//                    {
//                        if (v_sndmsg[i]=='#')
//                        {
//                            i++;
//                            break;
//                        }
//                        tmp[tmp_num]=v_sndmsg[i];
//                        tmp_num++;
//                    }
//                    if (tmp_num!=0)
//                    {
//                        if (strcmp(tmp,"M")==0)/*月*/
//                        {
//                            memcpy(msg+djt_num,months,2);
//                            djt_num=djt_num+2;
//                        }
//                        else if(strcmp(tmp,"D")==0)/*日*/
//                        {
//                            memcpy(msg+djt_num,day,2);
//                            djt_num=djt_num+2;
//                        }
//                        else if(strcmp(tmp,"H")==0)/*小时*/
//                        {
//                            memcpy(msg+djt_num,hours,2);
//                            djt_num=djt_num+2;
//                        }
//                        else if(strcmp(tmp,"m")==0)/*分*/
//                        {
//                            memcpy(msg+djt_num,mins,2);
//                            djt_num=djt_num+2;
//                        }
//                        else if (strcmp(tmp,"CDNO")==0)/*转出卡号*/
//                        {
//                            memcpy(msg+djt_num,tmp_cdno,4);
//                            djt_num=djt_num+4;
//                        }
//                        else if (strcmp(tmp,"ICDNO")==0)/*转入卡号*/
//                        {
//                            memcpy(msg+djt_num,v_incardno,strlen(v_incardno));
//                            djt_num=djt_num+strlen(v_incardno);
//                        }
//                        else if (strcmp(tmp,"RMB")==0)/*金额*/
//                        {
//                            nowamount=l_amount/100.00;
//                            sprintf(djt_str,"%.2lf",nowamount);
//                            memcpy(msg+djt_num,djt_str,strlen(djt_str));
//                            djt_num=djt_num+strlen(djt_str);
//                        }
//                        else if (strcmp(tmp,"TERM")==0)/*终端号码*/
//                        {
//                            memcpy(msg+djt_num,v_callno,strlen(v_callno));
//                            djt_num=djt_num+strlen(v_callno);
//                        }
//                        else if (strcmp(tmp,"DESC")==0)/*交易描述*/
//                        {
//                            memcpy(msg+djt_num,v_describe,strlen(v_describe));
//                            djt_num=djt_num+strlen(v_describe);
//                        }
//                        else if(strcmp(tmp,"TRMB")==0||strcmp(tmp,"TCOUNT")==0) /*当日总金额，当日总笔数*/
//                        {
//                            /* EXEC SQL select count(*),nvl(sum(to_number(v_txamt))/100,0) into :total_num,:totalamount
//                             from ctl_trade_list_sms
//                             where v_callno=:v_callno
//                             and substr(v_pro_time,1,8)=:v_djtdt and v_tradestau='00'
//                             and v_succflag='0'
//                             and v_proccode<>'300000'; */ 
//                            
//                            {
//                                struct sqlexd sqlstm;
//                                sqlorat((void **)0, &sqlctx, &oraca);
//                                sqlstm.sqlvsn = 10;
//                                sqlstm.arrsiz = 15;
//                                sqlstm.sqladtp = &sqladt;
//                                sqlstm.sqltdsp = &sqltds;
//                                sqlstm.stmt = "select count(*)  ,nvl((sum(to_number(v_txamt))/100),\
//                                0) into :b0,:b1  from ctl_trade_list_sms where ((((v_callno=:b2 and substr(v_\
//                                pro_time,1,8)=:b3) and v_tradestau='00') and v_succflag='0') and v_proccode<>\
//                                '300000')";
//                                sqlstm.iters = (unsigned int  )1;
//                                sqlstm.offset = (unsigned int  )596;
//                                sqlstm.selerr = (unsigned short)1;
//                                sqlstm.cud = sqlcud0;
//                                sqlstm.sqlest = (unsigned char  *)&sqlca;
//                                sqlstm.sqlety = (unsigned short)256;
//                                sqlstm.occurs = (unsigned int  )0;
//                                sqlstm.sqhstv[0] = (         void  *)&total_num;
//                                sqlstm.sqhstl[0] = (unsigned int  )sizeof(int);
//                                sqlstm.sqhsts[0] = (         int  )0;
//                                sqlstm.sqindv[0] = (         void  *)0;
//                                sqlstm.sqinds[0] = (         int  )0;
//                                sqlstm.sqharm[0] = (unsigned int  )0;
//                                sqlstm.sqadto[0] = (unsigned short )0;
//                                sqlstm.sqtdso[0] = (unsigned short )0;
//                                sqlstm.sqhstv[1] = (         void  *)&totalamount;
//                                sqlstm.sqhstl[1] = (unsigned int  )sizeof(float);
//                                sqlstm.sqhsts[1] = (         int  )0;
//                                sqlstm.sqindv[1] = (         void  *)0;
//                                sqlstm.sqinds[1] = (         int  )0;
//                                sqlstm.sqharm[1] = (unsigned int  )0;
//                                sqlstm.sqadto[1] = (unsigned short )0;
//                                sqlstm.sqtdso[1] = (unsigned short )0;
//                                sqlstm.sqhstv[2] = (         void  *)v_callno;
//                                sqlstm.sqhstl[2] = (unsigned int  )40;
//                                sqlstm.sqhsts[2] = (         int  )0;
//                                sqlstm.sqindv[2] = (         void  *)0;
//                                sqlstm.sqinds[2] = (         int  )0;
//                                sqlstm.sqharm[2] = (unsigned int  )0;
//                                sqlstm.sqadto[2] = (unsigned short )0;
//                                sqlstm.sqtdso[2] = (unsigned short )0;
//                                sqlstm.sqhstv[3] = (         void  *)v_djtdt;
//                                sqlstm.sqhstl[3] = (unsigned int  )20;
//                                sqlstm.sqhsts[3] = (         int  )0;
//                                sqlstm.sqindv[3] = (         void  *)0;
//                                sqlstm.sqinds[3] = (         int  )0;
//                                sqlstm.sqharm[3] = (unsigned int  )0;
//                                sqlstm.sqadto[3] = (unsigned short )0;
//                                sqlstm.sqtdso[3] = (unsigned short )0;
//                                sqlstm.sqphsv = sqlstm.sqhstv;
//                                sqlstm.sqphsl = sqlstm.sqhstl;
//                                sqlstm.sqphss = sqlstm.sqhsts;
//                                sqlstm.sqpind = sqlstm.sqindv;
//                                sqlstm.sqpins = sqlstm.sqinds;
//                                sqlstm.sqparm = sqlstm.sqharm;
//                                sqlstm.sqparc = sqlstm.sqharc;
//                                sqlstm.sqpadto = sqlstm.sqadto;
//                                sqlstm.sqptdso = sqlstm.sqtdso;
//                                sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
//                            }
//                            
//                            
//							if (sqlca.sqlcode!=0)
//							{
//								sprintf(msg,"查询总金额总笔数出错,错误代码：%d ！",sqlca.sqlcode);
//								writelog(msg,2);
//								total_num=-1;
//								totalamount=-1;
//							}
//							if (strcmp(tmp,"TRMB")==0)
//							{
//                                sprintf(djt_str,"%.2lf 元",totalamount);
//                                memcpy(msg+djt_num,djt_str,strlen(djt_str));
//                                djt_num=djt_num+strlen(djt_str);
//							}
//							if (strcmp(tmp,"TCOUNT")==0)
//							{
//                                sprintf(djt_str,"%d 笔",total_num);
//                                memcpy(msg+djt_num,djt_str,strlen(djt_str));
//                                djt_num=djt_num+strlen(djt_str);
//							}
//                        }
//                        else
//						{
//							sprintf(msg,"配置的转换参数不能识别,参数值：%s！",tmp);
//							writelog(msg,2);
//						}
//                    }  				
//                }
//                if (i>=msg_num)
//                    break;
//                msg[djt_num]=v_sndmsg[i];
//                djt_num++;
//            }  		
//            SendMsg_api("12345678","123456","B",v_mobilecode,msg,v_routeid,"1",riderinfo);
//        }
//        dd_num++;
//        
//        if (dd_num>=d_num)
//            break;
//	}
//	/* EXEC SQL update basic_para set para_value=:enddttime
//     where para_id='send_msg_trd_date_ctl'; */ 
//    
//    {
//        struct sqlexd sqlstm;
//        sqlorat((void **)0, &sqlctx, &oraca);
//        sqlstm.sqlvsn = 10;
//        sqlstm.arrsiz = 15;
//        sqlstm.sqladtp = &sqladt;
//        sqlstm.sqltdsp = &sqltds;
//        sqlstm.stmt = "update basic_para  set para_value=:b0 where para_id='send_m\
//        sg_trd_date_ctl'";
//        sqlstm.iters = (unsigned int  )1;
//        sqlstm.offset = (unsigned int  )627;
//        sqlstm.cud = sqlcud0;
//        sqlstm.sqlest = (unsigned char  *)&sqlca;
//        sqlstm.sqlety = (unsigned short)256;
//        sqlstm.occurs = (unsigned int  )0;
//        sqlstm.sqhstv[0] = (         void  *)enddttime;
//        sqlstm.sqhstl[0] = (unsigned int  )20;
//        sqlstm.sqhsts[0] = (         int  )0;
//        sqlstm.sqindv[0] = (         void  *)0;
//        sqlstm.sqinds[0] = (         int  )0;
//        sqlstm.sqharm[0] = (unsigned int  )0;
//        sqlstm.sqadto[0] = (unsigned short )0;
//        sqlstm.sqtdso[0] = (unsigned short )0;
//        sqlstm.sqphsv = sqlstm.sqhstv;
//        sqlstm.sqphsl = sqlstm.sqhstl;
//        sqlstm.sqphss = sqlstm.sqhsts;
//        sqlstm.sqpind = sqlstm.sqindv;
//        sqlstm.sqpins = sqlstm.sqinds;
//        sqlstm.sqparm = sqlstm.sqharm;
//        sqlstm.sqparc = sqlstm.sqharc;
//        sqlstm.sqpadto = sqlstm.sqadto;
//        sqlstm.sqptdso = sqlstm.sqtdso;
//        sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
//    }
//    
//    
//	if (sqlca.sqlcode!=0)
//	{
//		sprintf(msg,"更新参数表参值出错，更新值为：%s!,错误代码：%d ！",enddttime,sqlca.sqlcode);
//		writelog(msg,2);
//		/* EXEC SQL rollback; */ 
//        
//        {
//            struct sqlexd sqlstm;
//            sqlorat((void **)0, &sqlctx, &oraca);
//            sqlstm.sqlvsn = 10;
//            sqlstm.arrsiz = 15;
//            sqlstm.sqladtp = &sqladt;
//            sqlstm.sqltdsp = &sqltds;
//            sqlstm.iters = (unsigned int  )1;
//            sqlstm.offset = (unsigned int  )646;
//            sqlstm.cud = sqlcud0;
//            sqlstm.sqlest = (unsigned char  *)&sqlca;
//            sqlstm.sqlety = (unsigned short)256;
//            sqlstm.occurs = (unsigned int  )0;
//            sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
//        }
//        
//        
//		return 0;
//	}
//	/* EXEC SQL commit; */ 
//    
//    {
//        struct sqlexd sqlstm;
//        sqlorat((void **)0, &sqlctx, &oraca);
//        sqlstm.sqlvsn = 10;
//        sqlstm.arrsiz = 15;
//        sqlstm.sqladtp = &sqladt;
//        sqlstm.sqltdsp = &sqltds;
//        sqlstm.iters = (unsigned int  )1;
//        sqlstm.offset = (unsigned int  )661;
//        sqlstm.cud = sqlcud0;
//        sqlstm.sqlest = (unsigned char  *)&sqlca;
//        sqlstm.sqlety = (unsigned short)256;
//        sqlstm.occurs = (unsigned int  )0;
//        sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
//    }
//    
//    
//	return 1;
//}
