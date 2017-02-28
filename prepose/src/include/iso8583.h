#ifndef ISO8583
#define ISO8583

#define  LEN_FIXED          0
#define  LEN_LLVAR          1
#define  LEN_LLLVAR         2

#define  FMT_MMDDHHMMSS     8
#define  FMT_YYMMDDHHMMSS   1
#define  FMT_YYMM           2
#define  FMT_YYYYMMDD       3
#define  FMT_YYMMDD         4
#define  FMT_MMDD           5
#define  FMT_HHMMSS         6
#define  FMT_MONEY          7
#define  FMT_B              0
#define  FMT_N              9
#define  FMT_A              10
#define  FMT_AN             11
#define  FMT_ANS            12
#define  FMT_Z              13


#define CHK_LENGTH     1
#define CHK_FORMAT     2
#define CHK_DATETIME   3
#define MAXBUFFERLEN     2048 /*定义ISO_data结构中最长的缓冲区*/

struct  ISO_8583 {              /* ISO 8583 Message Structure Definitions */
        int    len;             /* data element max length */
        unsigned char    type;  /* bit 0--x, bit 1--n 左对齐, bit 2--z bit 3--b*/
        unsigned char    flag;  /* length field length: 1--LLVAR型 2--LLLVAR型*/
        unsigned char    fmt;
};


struct len_str {
        short  len;
        char   *str;
};

struct data_element_flag {
        short bitf;
        short len;
        int   dbuf_addr;
};

typedef struct  {
        struct  data_element_flag f[128];
        short   off;
        char    dbuf[2030];
        char    message_id[10];
} ISO_data;

typedef  struct _tagIso8583Bit
{
  int    bit_index;     /*bit order , 0..127 */
  int    bit_fixedvar;  /*flag for fixed/variable length */
                        /*be one of (LEN_FIXED,LEN_LLVAR,LEN_LLLVAR) */
  int    bit_length;    /*lenght of fixed-length bit or max length of */
                        /*variable-length bit */
  int    bit_format;    /*format of this bit,such as FMT_ANS,FMT_YYMMDD */
  int    bit_compress;  /*  */
  char   bit_name[40];      /*name of this bit */
}ISO8583BIT;

struct element_struc {
/*        short bitno; */          /* element no */
/*     short type; */           /* 0--default value, 1,2--process function */
        short flag;  
        short len;             /* when type=0, len=length of defaule value */
        int  pointer;        /* pointer of value or process function */
};

struct  trans_bitmap_struc {
/*        short trans_type;  */              /* transaction type */
/*        char  message_id[10]; */
        short   off;
        char buf[2048];            /* number of elememts */
        struct element_struc element[129];   /* transaction element ally */

};

int setbit (ISO_data  *iso, int n, unsigned char *str, int len );
int getbit (ISO_data  *iso , int n ,unsigned char *str );
char* r_trim(char* str);
char* l_trim(char* str);
char *trim(char* str);
void clearbit(ISO_data * iso);
int SetIsoHeardFlag( int  type );
int SetIsoFieldLengthFlag ( int type );
int GetIsoHeardFlg();
int GetFieldLeagthFlag();
int SetIsoMultiThreadFlag(int type);
int GetIsoMultiThreadFlag();
void clearbit(ISO_data *);
void asc_to_bcd(unsigned char *,unsigned  char *, int, unsigned char);
void bcd_to_asc(unsigned char *,unsigned  char *, int, unsigned char);
#endif
