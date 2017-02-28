#include "ibdcs.h"

//#define NATIVE_DEBUG_LOG

static int  gs_fd  = -1;
static char gs_ident[64] ="        \0";
static char gs_logfile[256] = {0,0,0,0};


static void _dcs_format(char *, int ,const char *, va_list,char * filename,long  line);
static void _dcs_dump(char *, int , char *, int );

#define ISPRINT(c)  (isprint(c) && isascii(c))


extern void setprogname_ (char *);
extern void pre_dcs_log(char * filename,long line,void *ptrbytes, int nbytes,const char * message,...);
extern void pre_dcs_debug(char * filename,long line,void *ptrbytes, int nbytes,const char * message,...);

int dcs_log_open(const char * logfile, char *ident)
{
	setprogname_(ident); //Freax Add
    if(ident && ident[0])
    {
        char *p = gs_ident;
        for(; p < gs_ident + sizeof(gs_ident) -1;p++,ident++)
        {
            *p = *ident;
            if(*p == 0) break;
        }
    }

    if(gs_fd < 0) gs_fd = open(logfile,O_WRONLY|O_APPEND|O_CREAT,0666);

    if(gs_fd >= 0)
    {
        memset(gs_logfile, 0, sizeof(gs_logfile));   
        strcpy(gs_logfile, logfile);
        close( gs_fd );
        gs_fd = -1;
        return 0;
    }
    
    return -1;
}

void dcs_log_close()
{
    close(gs_fd); 
    gs_fd = -1;
    memset(gs_logfile, 0, sizeof(gs_logfile));   
}

int dcs_set_logfd(int fd)
{
    if(fd < 0)
        return -1;

   if(gs_fd >= 0)
   {
        close(gs_fd);
        memset(gs_logfile, 0, sizeof(gs_logfile));   
   }

   gs_fd = fd;
   return 0;
}

void pre_dcs_log(char * filename,long line,void *ptrbytes, int nbytes,const char * message,...)
{
	char    buffer[1024 * 8],*ptr=NULL;
	va_list ap;
	int     max_size;
	int fd=-1;
	char    *pbytes = (char *)ptrbytes;
	memset(&ap, 0, sizeof(va_list));
#ifndef  NATIVE_DEBUG_LOG
	//gs_fd = open( gs_logfile,O_WRONLY|O_APPEND|O_CREAT,0666);
	fd = open( gs_logfile,O_WRONLY|O_APPEND|O_CREAT,0666);
	// if( gs_fd < 0 ) return;
	if( fd < 0 ) return;

	va_start(ap, message);
	memset(buffer, 0, sizeof(buffer));
	_dcs_format(buffer, sizeof(buffer)-2,message,ap,filename,line);
	va_end(ap);

	max_size = sizeof(buffer) - 2 - (int)strlen(buffer);
	ptr      = buffer + strlen(buffer);

	if(pbytes && nbytes)
	_dcs_dump(ptr, max_size, pbytes,nbytes);

	//write the logging message into file
	ptr = buffer + strlen(buffer);
	*ptr ='\n';
	// write(gs_fd, buffer, strlen(buffer));
	//close( gs_fd );
	write(fd, buffer, strlen(buffer));
	close( fd );
	//gs_fd = -1;
#else
	va_start(ap, message);
	memset(buffer, 0, sizeof(buffer));
	_dcs_format(buffer, sizeof(buffer)-2,message,ap,filename,line);
	va_end(ap);

	max_size = sizeof(buffer) - 2 - (int)strlen(buffer);
	ptr      = buffer + strlen(buffer);

	if(pbytes && nbytes)
	_dcs_dump(ptr, max_size, pbytes,nbytes);

	//write the logging message into file
	ptr = buffer + strlen(buffer);
	*ptr ='\n';
	printf("%s\n", buffer);
#endif
	return;
}

void pre_dcs_debug(char * filename,long line,void *ptrbytes, int nbytes,const char * message,...)
{
	char    buffer[(PACKAGE_LENGTH/16+1)*74+200],*ptr=NULL;
	va_list ap;
	int     max_size;
	int fd=-1;
	char    *pbytes = (char *)ptrbytes;
	memset(&ap, 0, sizeof(va_list));
#ifndef  NATIVE_DEBUG_LOG    
	if(g_pIbdcsCfg == NULL || g_pIbdcsCfg->iRunMode == 0) //debug mode
	    ;
	else  //product mode
	    return;
	    
	fd = open( gs_logfile,O_WRONLY|O_APPEND|O_CREAT,0666);
	if( fd < 0 ) return;
	
	va_start(ap, message);
	memset(buffer, 0, sizeof(buffer));
	_dcs_format(buffer, sizeof(buffer)-2,message,ap,filename,line);
	va_end(ap);

	max_size = sizeof(buffer) - 2 - (int)strlen(buffer);
	ptr      = buffer + strlen(buffer);

    //todo debug 不打印
	//if(pbytes && nbytes)
    //    _dcs_dump(ptr, max_size, pbytes,nbytes);
    //ptr = buffer + strlen(buffer);
	*ptr ='\n';
    //write the logging message into file
	write(fd, buffer, strlen(buffer));
	close(fd );
#else
	va_start(ap, message);
	memset(buffer, 0, sizeof(buffer));
	_dcs_format(buffer, sizeof(buffer)-2,message,ap,filename,line);
	va_end(ap);

	max_size = sizeof(buffer) - 2 - (int)strlen(buffer);
	ptr      = buffer + strlen(buffer);

	if(pbytes && nbytes)  _dcs_dump(ptr, max_size, pbytes,nbytes);

	//write the logging message into file
	ptr      = buffer + strlen(buffer);
	*ptr ='\n';    
	printf("%s\n", buffer);
#endif
	return;
}

void _dcs_format(char *ptr, int max_size,const char *message, va_list ap,char * filename,long  line)
{	
	time_t  t;
	memset(&t,0,sizeof(time_t));
	time(&t);
#if 0
	struct  tm *tm=NULL;   
	//format the message
	tm = localtime(&t);
	sprintf(ptr,"%4d/%02d/%02d %02d:%02d:%02d %s(%.6d)(%.6d) \n",
        tm->tm_year+1900,
        tm->tm_mon + 1,
        tm->tm_mday,
        tm->tm_hour,
        tm->tm_min,
        tm->tm_sec,
        gs_ident,
        getpid(),
        syscall(SYS_gettid));

#else
	 struct tm tm = {0};  
	localtime_r(&t, &tm);  
	sprintf(ptr,"%4d/%02d/%02d %02d:%02d:%02d %s %ld %s(%.6d)(%.6d) \n",
        tm.tm_year+1900,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec,
        filename,
        line,
        gs_ident,
        getpid(),
        syscall(SYS_gettid));
#endif


	ptr      += strlen(ptr);
	max_size -= strlen(ptr);

	//print the true logging message
	vsnprintf(ptr, max_size,message, ap);

	ptr += strlen(ptr);
	if( *(ptr - 1) != '\n')
	    *ptr = '\n';
}

void _dcs_dump(char *ptr, int max_size, char *pbytes, int nbytes)
{
    int  r,i;
    unsigned char chr;

    int round = nbytes / 16;

    for(r=0; max_size > 7 && pbytes && r < round; r++)
    {
        //dump a line
        sprintf(ptr, "%.4Xh: ", r * 16);
        max_size -= strlen(ptr); ptr += strlen(ptr);

        for(i=0; max_size > 0 && i < 16; i++)
        {
            chr = pbytes[r * 16 + i];
            sprintf(ptr, "%.2X ", chr & 0xFF);
            max_size -= strlen(ptr); ptr += strlen(ptr);
        }

        if(max_size < 2) break;
        sprintf(ptr, "%s", "; "); ptr += 2; max_size -= 2;

        for(i=0; max_size > 0 && i < 16; i++)
        {
            chr = pbytes[r * 16 + i] & 0xFF;
            *ptr = ISPRINT(chr) ? chr : '.';
            ptr ++; max_size --;
        }

        if(max_size < 2) goto lblReturn;

        *ptr = '\n'; ptr++; max_size --;
    }//for each round

    if(max_size > 7 && pbytes && (nbytes % 16) )
    {
        //dump the last line
        sprintf(ptr, "%.4Xh: ", r * 16);
        max_size -= strlen(ptr); ptr += strlen(ptr);

        for(i=0; max_size > 0 && i < nbytes % 16; i++)
        {
            chr = pbytes[r * 16 + i];
            sprintf(ptr, "%.2X ", chr & 0xFF);
            max_size -= strlen(ptr); ptr += strlen(ptr);
        }

        for(chr=' '; max_size > 0 && i < 16; i++)
        {
            sprintf(ptr, "%c%c ", chr & 0xFF, chr & 0xFF);
            max_size -= strlen(ptr); ptr += strlen(ptr);
        }

        if(max_size < 2) goto lblReturn;

        sprintf(ptr, "%s", "; ");
        ptr += 2; max_size -= 2;

        for(i=0; max_size > 0 && i < (nbytes % 16); i++)
        {
            chr = pbytes[r * 16 + i] & 0xFF;
            *ptr = ISPRINT(chr) ? chr : '.';
            ptr ++;  max_size --;
        }
    }//last line

lblReturn:
    ptr += strlen(ptr);
    *ptr = '\n';
}

