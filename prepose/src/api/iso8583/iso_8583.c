//
//  iso_8583.c
//  iPrepose
//
//  Created by Freax on 12-12-19.
//
#include "iso8583.h"
#include "unixhdrs.h"
#include "def8583.h"

extern struct ISO_8583 *iso8583;
//extern struct ISO_8583 iso8583_conf[128];
extern struct ISO_8583 iso8583_def[128];
extern struct ISO_8583 iso8583_pep[128];
//char   iso8583name[129][30];
char *trim(char* str);

static int isoHeardFlag = -1;
static int isoFieldLengthFlag = -1;
static int isoMultiThreadFlag = 0;//默认0表示是多进程，1表示多线程，多线程时要使用线程私有数据
static pthread_key_t isoHeardFlag_key;///线程私有数据类型
static pthread_key_t isoFieldLengthFlag_key;///线程私有数据类型

/*
 2.4.	int setbit (ISO_data  *iso ,int n ,unsigned char *str , int len )
 Include ：iso8583.h
 功能描述：设置8583域数据。
 参数说明：
 Iso:iso8583数据结构起始地址
 n：报文域标识。
 str：需存放的数据起始地址
 len: 需存放的数据长度
 返 回 值：大于等于0成功，－1失败。
 */
int setbit (ISO_data  *iso, int n, unsigned char *str, int len )
{
    int  i=0, l=0;
    unsigned char *pt = NULL, tmp[MAXBUFFERLEN]/*, str_buf[MAXBUFFERLEN]*/;
    memset(tmp, 0x00, sizeof tmp );
	/*
     memset(str_buf,0x00, sizeof str_buf );
     memcpy(str_buf, str, len);
     */

    if( n == 0   )
    {
        memcpy( iso->message_id, str, 4);
        iso->message_id[4]=0x00;
        return 0;
    }
    //第1域为位图域,不能用
    if (n == 1 || n > 128)
    {
        return -1;
    }

    n -- ;

	if( len == 0)
    {
	    iso->f[n].len        = len;
        iso->f[n].bitf       = 0;
        return 0; //str为空,不需要打包
    }

	/**
	 * 根据是多线程还是多进程进行不同处理;
	 * 如果是多进程则直接使用全局变量，多线程时使用线程的私有变量;
	 */
	struct ISO_8583 *iso8583_local=NULL;
	if(isoMultiThreadFlag != 0)
	{
		iso8583_local = (struct ISO_8583 *)pthread_getspecific(iso8583_key);
	}
	else
	{
		iso8583_local = iso8583;
	}

    //最大长度不能超过8583包规定的长度
    if( len > iso8583_local[n].len )
    {
        len = iso8583_local[n].len;
    }

    l = len;

    if( iso8583_local[n].flag == LEN_FIXED ) //该域为固定长度
    {
        len = iso8583_local[n].len;
    }

    iso->f[n].len        = len;      /*保存该域的长度*/
    iso->f[n].bitf       = 1;        /*置该域为有值*/
    iso->f[n].dbuf_addr  = iso->off;  /*该域值在dbuf中的起始位置*/ //Freax
	pt = (unsigned char*)&(iso->dbuf[iso->off]);                 //Freax

   	//dbuf的空间已经满
	if( (iso->off+len) >= MAXBUFFERLEN )
	{
		return -1;
	}
	if( iso8583_local[n].type == 1 ) //左对齐  BCD码 压缩
	{
		memcpy(tmp, str, l);
		memset(tmp+l, ' ', len-l);
		asc_to_bcd(pt, tmp, len, 0);
        //iso->off+=(len+1)/2;
        iso->off += len ;      //保证足够的空间
	}
	else if( iso8583_local[n].type == 0 ) //左对齐  ASC码 不压缩
	{
		memcpy(tmp, str, l);
		memset(tmp+l, ' ', len-l);
		memcpy(pt, tmp, len);
		iso->off += len;
	}
	else if( iso8583_local[n].type==2 ) //右对齐  BCD码
	{
		memset(tmp, '0', len-l);
		memcpy(tmp+len-l, str, l);
		asc_to_bcd(pt, tmp, len, 0);
		iso->off += (len+1)/2;
	}
	else if( iso8583_local[n].type==3 ) //右对齐  ASC码
	{
		memset(tmp, ' ', len-l);
		memcpy(tmp+len-l, str, l);
		memcpy(pt, tmp, len);
		iso->off += len;
	}

    return 0;
}

/*
 2.5.	int getbit (ISO_data    *iso ,int n ,unsigned char *str )
 Include ：iso8583.h
 功能描述：获取8583域数据。
 参数说明：
 Iso:iso8583数据结构起始地址
 n：报文域标识。
 str：获取到的数据存放地址
 返 回 值：大于等于0成功，－1失败。
 */

int getbit (ISO_data  *iso , int n ,unsigned char *str )
{
    int           len=0;
    unsigned char *pt=NULL;

    if( n==0 )
    {
        memcpy(str, iso->message_id,4);
        str[4]='\0';
        len = 4;
        return len;
    }

    if( n == 1 || n > 128 )   return 0;

    n--;

    if( iso->f[n].bitf == 0 )
    {
        if( n==127 )
        {
            if (iso->f[63].bitf==0)  return 0;
        }
        str[0] = 0x00;
        return 0;
    }
    /**
     * 根据是多线程还是多进程进行不同处理;
     * 如果是多进程则直接使用全局变量，多线程时使用线程的私有变量;
     */
    struct ISO_8583 *iso8583_local=NULL;
    if(isoMultiThreadFlag != 0)
    {
        iso8583_local = (struct ISO_8583 *)pthread_getspecific(iso8583_key);
    }
    else
    {
        iso8583_local = iso8583;
    }
    pt  = (unsigned char*) &iso->dbuf[iso->f[n].dbuf_addr]; //Freax
    len = iso->f[n].len;
    memset(str, 0x00, strlen((const char *)str));

    if(iso8583_local[n].type == 1) {
        memcpy(str, pt, len);
		str[len] = 0x00;
        int bitValLen = (int)strlen((const char *)str);
        if(bitValLen == len) {
            goto getbit_end;
        }
	}


    if( iso8583_local[n].type == 1 ) //左对齐  BCD码 压缩
	{
		bcd_to_asc(str, pt, len, 0);
	}
	else if( iso8583_local[n].type == 0 ) //左对齐  ASC码 不压缩
	{
		memcpy(str , pt, len);
	}
	else if( iso8583_local[n].type == 2 ) //右对齐  BCD码
	{
		bcd_to_asc(str, pt, len,0);
	}
	else if( iso8583_local[n].type == 3 ) //右对齐  ASC码
	{
		memcpy(str, pt, len);
	}
	else
	{
		str = NULL;	//目前还不支持这种类型
		return -1;
	}
    if(n != 61 && n != 51 && n !=54 && n != 62 && iso8583_local[n].type == 0 && (int)strlen((const char *)str) < len) {
		//dcs_log(0,0, "strlen=[%d],len=[%d]",(int)strlen((const char *)str) ,len);
		bcd_to_asc(str,pt,len,0);
	}

getbit_end:
	if(n!=51 && n!=43 && n!=61)//52域有可能会第一字节为0x20,44域不处理(修复实达机器消费报解包错)
		trim((char*)str);
	str[len]=0x00;

	if ( n + 1 == 35 || n + 1 == 36 ){
	    int i = 0;
		for ( i = 0; i < len; i ++ ){
			if ( str[i] == 'D' )
				str[i] = '=';
		}
	}

	str[len]='\0';
    return len;
}

/*=================================================================
 * Function ID :  r_trim
 * Input       :  char* str
 * Output      :  char* str
 * Author      :  DengJun
 * Date        :  Dec  12  2004
 * Return      :  str
 * Description :
 * Notice      :  去掉字符串右边的空格
 *
 *=================================================================*/
char* r_trim(char* str)
{
    char buf[2048];
    int  len;

    memset(buf,0x00,sizeof buf);
    strcpy(buf,str);
    len = (int)strlen(str);
    while(len)
    {
        if( buf[len-1] != ' ' )
        {
            break;
        }
        len--;
    }
    memset(str,0x00, strlen(str));
    memcpy(str,buf,len);
    return str;
}

/*=================================================================
 * Function ID :  l_trim
 * Input       :  char* str
 * Output      :  char* str
 * Author      :  DengJun
 * Date        :  Dec  12  2004
 * Return      :  str
 * Description :
 * Notice      :  去掉字符串左边的空格
 *
 *=================================================================*/
char* l_trim(char* str)
{
    char buf[2048];
    int  i=0,len=0;

    memset(buf, 0x00, sizeof buf );
    strcpy(buf, str);
    memset(str, 0x00, strlen(str) );
    len = (int)strlen(buf);
    for(i=0; i<len; i++)
    {
        if( buf[i] !=' ')
            break;
    }
    memcpy(str,buf+i,len-i);
    return str;
}

/*=================================================================
 * Function ID :  trim
 * Input       :  char* str
 * Output      :  char* str
 * Author      :  DengJun
 * Date        :  Dec  12  2004
 * Return      :  str
 * Description :
 * Notice      :  去掉字符串左右两边的空格
 *
 *=================================================================*/
char *trim(char* str)
{
    r_trim(str);
    l_trim(str);
    return str;
}


/*
 2.6.	void clearbit (ISO_data * iso )
 Include ：iso8583.h
 功能描述：清除8583存储区信息。
 参数说明：Iso:iso8583数据结构起始地址
 返 回 值：
 */
void clearbit(ISO_data * iso)
{
    if (iso == NULL) return;

    int i;
    for (i=0; i<128; i++)
    {
        iso->f[i].bitf=0;
    }
    iso->off=0;

}

/*
 2.9.	int SetIsoHeardFlag( int  type )
 Include ：iso8583.h
 功能描述：设置8583报文类型是否压缩。。如果是多线程，此函数要在子线程中调用
 参数说明：type：0为压速,1为不压缩
 返 回 值：1成功，0失败.
 */
int SetIsoHeardFlag( int  type )
{
	isoHeardFlag = type;
	if(isoMultiThreadFlag != 0)//多线程需要将其设置为线程私有数据变量
		pthread_setspecific(isoHeardFlag_key,&isoHeardFlag);
    return 1;
}

/*
 2.11.	int SetIsoFieldLengthFlag ( int type )
 Include ：iso8583.h
 功能描述：设置域长度是否压缩。。如果是多线程，此函数要在子线程中调用
 参数说明：
 type：0为压速,1为不压缩
 返 回 值：1成功，0失败。
 */
int SetIsoFieldLengthFlag ( int type )
{
	isoFieldLengthFlag = type;
	if(isoMultiThreadFlag != 0)//多线程需要将其设置为线程私有数据变量
		pthread_setspecific(isoFieldLengthFlag_key,&isoFieldLengthFlag);
	return 1;
}

/*
 2.10.	int GetIsoHeardFlg()
 Include ：iso8583.h
 功能描述：获取8583报文类型是否压缩标志。。如果是多线程，此函数要在子线程中调用
 参数说明：
 返 回 值：1报文不压缩，0为压缩。
 */
int GetIsoHeardFlg()
{
	if(isoMultiThreadFlag != 0)//多线程需要取的是线程私有数据变量
		return *((int *)pthread_getspecific(isoHeardFlag_key));
	else
		return isoHeardFlag;
}

/*
 2.12.	int GetFieldLeagthFlag()
 Include ：iso8583.h
 功能描述：获取域长度是否压缩。如果是多线程，此函数要在子线程中调用
 参数说明：
 返 回 值：1报文不压缩，0为压缩。
 */

int GetFieldLeagthFlag()
{
	if(isoMultiThreadFlag != 0)//多线程需要取的是线程私有数据变量
		return *((int *)pthread_getspecific(isoFieldLengthFlag_key));
	else
		return isoFieldLengthFlag;
}

/*
 功能描述：设置是否是多线程标志
 备注：1.该函数一定要在SetIsoHeardFlag,SetIsoFieldLengthFlag之前设置。不设置就默认为多进程
 2.如果是多线程，此函数要在主进程中调用
 参数说明：
 type：0为默认情况是多进程,其他为多线程
 返 回 值：1成功，0失败。
 */
int SetIsoMultiThreadFlag(int type)
{
    isoMultiThreadFlag = type;
    if(isoMultiThreadFlag != 0)//多线程需要设置几个线程私有数据变量
    {
    	pthread_key_create(&isoFieldLengthFlag_key,NULL);
    	pthread_key_create(&isoHeardFlag_key,NULL);
    }
    return 1;
}

/*
 功能描述：获取是否是多线程标志
 参数说明：
 返 回 值：0为默认情况是多进程,其他为多线程
 */
int GetIsoMultiThreadFlag()
{
    return isoMultiThreadFlag;
}

/*
 2.7.	void asc_to_bcd ( unsigned char* bcd_buf , unsigned char* ascii_buf , int conv_len ,unsigned char type )
 Include ：iso8583.h
 功能描述：将asc码转换成bcd码。
 参数说明：
 bcd_buf:bcd码数据存放地址
 ascii_buf：asc码数据存放地址
 len：asc码长度
 type：0为右补0,1为左补零
 返 回 值：大于等于0成功，－1失败。
 */
void asc_to_bcd ( unsigned char* bcd_buf , unsigned char* ascii_buf , int conv_len ,unsigned char type )
{
    int    cnt;
    char   ch, ch1;

    if (conv_len&0x01 && type )
        ch1 = 0;
    else
        ch1=0x55;
    //printf("ch1 = %d\n", ch1);

    for (cnt=0; cnt<conv_len; ascii_buf++, cnt++) {

        if (*ascii_buf >= 'a' )
        {
            ch = *ascii_buf - 'a' + 10;
        }
        else if ( *ascii_buf >= 'A' )
        {
            ch = *ascii_buf- 'A' + 10;
        }
        else if ( *ascii_buf >= '0' )
        {
            ch = *ascii_buf - '0';
        }
        else
        {
            ch = 0;
        }

        if (ch1==0x55)
            ch1 = ch;
        else {
            *bcd_buf++ = ch1<<4 | ch;
            ch1 = 0x55;
        }
    }
    if (ch1!=0x55) *bcd_buf = ch1<<4;
}


/*
 2.8.	void bcd_to_asc (unsigned char* ascii_buf ,unsigned char* bcd_buf , int conv_len ,unsigned char type )
 Include ：iso8583.h
 功能描述：将bcd码转换成asc码。
 参数说明：
 ascii_buf：asc码数据存放地址
 bcd_buf:bcd码数据存放地址
 len：asc码长度
 type：0为右补0,1为左补零
 返 回 值：大于等于0成功，－1失败。
 */
void bcd_to_asc (unsigned char* ascii_buf, unsigned char* bcd_buf, int conv_len, unsigned char type )
{
    int cnt;
    if (conv_len & 0x01 && type) {
        cnt = 1;
        conv_len++;
    } else
        cnt = 0;

    for (; cnt < conv_len; cnt++, ascii_buf++){
        *ascii_buf = ((cnt & 0x01) ? (*bcd_buf++ & 0x0f) : (*bcd_buf >> 4));
        *ascii_buf += ((*ascii_buf > 9) ? ('A'-10) : '0');
    }
}

int strtoiso(unsigned char * dstr, ISO_data *iso, int flag)
{
    return 0;
}

int isotostr(unsigned char *dstr, ISO_data *iso)
{
    return 0;
}

