#include  "ibdcs.h"

#include "extern_function.h"

static int dcs_init_seqNo()
{
    char cfgfile[256], tmpbuf[32];
    int  fd, ret;

    if(u_fabricatefile("config/seqNo.conf",cfgfile,sizeof(cfgfile)) < 0) 
    {
        dcs_log(0,0,"cannot get full path name of 'seqNo.conf'\n");
        g_seqNo = 1;
        return 0;
    }

    if(0 > (fd = conf_open(cfgfile)) )
    {
        dcs_log(0,0,"open file '%s' failed: %s.\n",cfgfile, strerror(errno));
        g_seqNo = 1;
        return 0;
    }

    memset(tmpbuf, 0, sizeof(tmpbuf) );
    ret=conf_get_first_string(fd,"seqNo",  tmpbuf); 
    conf_close(fd); //Freax TO DO:  

    if(ret < 0)
    {
        dcs_log(0,0,"loading config item 'seqNo' failed.\n");
        g_seqNo = 1;
        return 0;
    }

    g_seqNo = strtoul( tmpbuf, (char **)0, 10 );
    if( g_seqNo == 0 )
        g_seqNo = 1;
    return 0;
}

static char *strNotice =
    "#\n"
    "# NOTICE:\n"
    "#     NEVER to edit this file manually, its content will be dynamically\n"
    "#     updated by FEP while running\n"
    "#\n"
    "\n";

int dcs_save_seqNo()
{
    char cfgfile[256], tmpLine[32];
    int fd = -1;

    if(u_fabricatefile("config/seqNo.conf",cfgfile,sizeof(cfgfile)) < 0)
    {
        dcs_log(0,0,"WARN: cannot get full path name of 'seqNo.conf'\n");
        return -1;
    }

    fd = open(cfgfile,O_WRONLY|O_CREAT, 0666);
    if(fd  < 0)
    {
        dcs_log(0,0,"open file '%s' failed: %s.\n",cfgfile,strerror(errno));
        return -1;
    }

    sprintf(tmpLine, "seqNo     %lu\n",  g_seqNo);
    write(fd, strNotice, strlen( strNotice ) );
    write(fd, tmpLine, strlen( tmpLine ) );
    close( fd );
    return 0;
}

unsigned long dcs_make_seqNo()
{
    static long ntimes = 1;

    if( g_seqNo <= 0 )
        dcs_init_seqNo();

    if( ntimes % 60 == 0 )
        dcs_save_seqNo();

    ntimes ++;  g_seqNo++;
    return (g_seqNo - 1);
}
