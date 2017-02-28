#include <time.h>
#include <stdio.h>
#include <errno.h>


int GetSystemDate(char *strdate)
{
    time_t tmptime;
    struct tm *tmptm;
    
    time(&tmptime);
    tmptm = (struct tm *)localtime(&tmptime);
    
    sprintf(strdate,"%02d%02d",tmptm->tm_mon+1,tmptm->tm_mday);
    
    return(0);    
}


int GetFullSystemDate(char *strdate)
{
    time_t tmptime;
    struct tm *tmptm;
    int  tmpyear;
    
    time(&tmptime);
    tmptm = (struct tm *)localtime(&tmptime);
    if(tmptm->tm_year > 90)
        tmpyear = 1900 + tmptm->tm_year%1900;
    else
        tmpyear = 2000 + tmptm->tm_year;
    
    sprintf(strdate,"%04d%02d%02d",tmpyear,tmptm->tm_mon+1,tmptm->tm_mday);
    
    return(0);
}

