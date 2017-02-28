#ifndef __TIMER__H__
#define __TIMER__H__

#include  "unixhdrs.h"
// defines a new type for event key and lock
//
#define  EV_K_LEN   72

typedef struct
{
    int  k_len;
    unsigned char  k_val[EV_K_LEN];
} evkey_t;

#define EVKEY_INITIALIZER  {0, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0} }

// each event item
//
struct eventitem
{
    long    ei_id;       //id of this event item
    long    ei_expire;   //when does message expire
    long    ei_timein;   //when did message arrive
    int     ei_flags;    //flags which can be set
    short   ei_action;   //what to do when msg expires
    int     ei_fid;      //which folder to send time out's to
    evkey_t ei_key;      //key by which sender knows message
    short   ei_bufid;    //which buffer cluster contains data of event

    short   ei_next;     //id of next event item
    short   ei_prev;     //id of previous event item
};

// defines action for event
//
#define EV_ACT_TXN      1   //transaction
#define EV_ACT_TIMER    3   //timer

// defines flags for event
//
#define EV_FLG_NOTIFIED  0x00000001 //owner of expired event notified already
#define EV_FLG_USED      0x00000008

struct eventqueue
{
    long eq_magic;
    long eq_nevents;   //how many event items alloacted in event queue
    long eq_nfrees;    //how many free event items available

    int  eq_freehead;  //head event item in free event list
    int  eq_freetail;  //tail event item in free event list
    int  eq_usedhead;  //head event item in used event list
    int  eq_usedtail;  //tail event item in used event list

    struct eventitem eq_evarray[1];
};

#define  EVENT_QUEUE_MAGIC    0x20001009

#define is_valid_evqueue( evq ) \
    ( (evq) && ((struct eventqueue *)evq)->eq_magic==EVENT_QUEUE_MAGIC )

#define is_valid_event(evq, ev) \
        ( (ev) && ((ev)->ei_id >=0) && ((ev)->ei_id < (evq)->eq_nevents) )

#define is_valid_eventid(evq, id) \
        ( ((id) >=0) && ((id) < (evq)->eq_nevents) )

//add by wu at 2014/12/30
#define  TIMER_FOLDER  "TIMERPROC"
#define   TIMER_ADD  '0'
#define   TIMER_REMOVE  '1'

struct Timer_struct{
	char key[20+1];   /*每个包有一个key唯一标示,由业务层产生*/
	char  timeout;/*超时重发时间间隔*/
	char  sendnum;/*超时重发次数*/
	char data[PACKAGE_LENGTH+1];/*报文内容*/
	int length;/*报文长度*/
	char foldid;/*报文要写入的foldid*/
};

struct TIMEOUT_INFO{
    char operation[28];              /*操作类型*/
	char key[33+1];   				/*每个包有一个key唯一标示,由业务层产生*/
	int timeout;					/*超时重发时间间隔*/
	char sendnum;					/*超时重发次数*/
	char data[PACKAGE_LENGTH+1];	/*报文内容*/
	int  length;					/*报文长度*/
	char foldid;					/*报文要写入的foldid*/
	char status;					/*处理状态初始0,1:超时已经发送2：移除*/
	pthread_t pid;					/*线程ID*/
};

#endif  //__TIMER__H__

