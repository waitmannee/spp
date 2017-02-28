/*
 * Freax
 *
 */

#include <stdlib.h>

static char *__progname = NULL;

char * getprogname_ ()
{
  return __progname;
}

void setprogname_ (char *progname)
{
  __progname = progname;
}

/*
 * 判断字符串是数字还是字符
 * 返回值: 0:数字; -1:字符
 */
int check_str_or_int(const char *file_name){
    char br = 0;
    for (; *file_name; file_name++) {
        if (!isdigit(*file_name)) {
            br = -1;
            break;
        }
    }
    return br;
}

int getstrlen(const char * StrBuf)
{
	int	i;

	for(i=0; i<2048; i++)
	{
		if( (StrBuf[i] == 0x20) || (StrBuf[i] == 0x00) ) break;
	}

	return i;
}


