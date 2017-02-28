#include "ibdcs.h"
#include "extern_function.h"

typedef struct bitlayaout
{
    int   msg_mti;
    int   msg_proccode;
    unsigned char msg_bitmap[16];
} BITLAYAOUT;

typedef struct questanswermap
{
    int   mti_quest;
    int   mti_answer;
} QUEST_ANSWER_MAP;

static BITLAYAOUT *g_BitLayoutTbl = NULL;
static int  g_nSlotInBitLayoutTbl = 0;

static QUEST_ANSWER_MAP *g_QuestAnswerTbl = NULL;
static int  g_nSlotInQuestAnswerTbl = 0;

static int set_bits_on(unsigned char *bitmap, char *bitSet)
{
    char *p1, *p0, *ptr1, *ptr0;
    int n;

    ptr0 = strchr(bitSet, '{');
    ptr1 = strchr(bitSet, '}');
    if( !ptr0 || !ptr1 )
        return -1;
    ptr0 ++; ptr1 --;
    if(ptr0 == ptr1)  //bits set is empty
        return 0;
        
    for(p0=ptr0; p0 <= ptr1; )
    {
        if( !isdigit(*p0) ) {  p0++; continue; }

        for(p1=p0; p1 <= ptr1; )
            if( isdigit(*p1) ) { p1++; continue; }
            else break;

        *p1 = 0;
        if(p1 - p0 > 0)
        {
            n = atoi( p0 );
//            if( 1<= n && n<= 128) SETBITON(bitmap, n-1); //Freax TO DO:  
        }

        p0 = p1+1;
    }//for

    return 0;
}

/* Freax TO DO*/
int dcs_load_txnconf()
{
    // this function load configuration from file
    // "$IBDCS_RUNPATH/config/trans.conf",and placed
    // all information into the buffer given by caller

    char cfgfile[256], strLine[256];
    int  fd, ret, i;

    if(u_fabricatefile("config/trans.conf",cfgfile,sizeof(cfgfile)) < 0)
    {
        dcs_log(0,0,"cannot get full path name of 'trans.conf'\n");
        return -1;
    }

    if(0 > (fd = conf_open(cfgfile)) )
    {
        dcs_log(0,0,"open file '%s' failed.\n",cfgfile);
        return -1;
    }

    //how many transaction-bits-set configured ?
    ret=conf_get_first_string(fd,"trans.bitset",strLine);
    for(;ret >= 0;)
    {
        g_nSlotInBitLayoutTbl ++;
        ret=conf_get_next_string(fd,"trans.bitset",strLine);
    }

    if( g_nSlotInBitLayoutTbl <= 0)
    {
        dcs_log(0,0,"no trans.bitset configured in '%s'\n",cfgfile);
        goto lblFailure;
    }

    //how many request-answer pairs configured ?
    ret=conf_get_first_string(fd,"trans.askanswer",strLine);
    for(;ret >= 0;)
    {
        g_nSlotInQuestAnswerTbl ++;
        ret=conf_get_next_string(fd,"trans.askanswer",strLine);
    }
    if( g_nSlotInQuestAnswerTbl <= 0)
    {
        dcs_log(0,0,"no trans.askanswert configured in '%s'\n",cfgfile);
        goto lblFailure;
    }

    //allocate memory to hold all configuration
    g_BitLayoutTbl = (BITLAYAOUT *)
                calloc(g_nSlotInBitLayoutTbl + 1, sizeof(BITLAYAOUT) );
    g_QuestAnswerTbl = (QUEST_ANSWER_MAP *)
                calloc(g_nSlotInQuestAnswerTbl + 1, sizeof(QUEST_ANSWER_MAP));
    if(!g_BitLayoutTbl || !g_QuestAnswerTbl)
    {
        dcs_log(0,0,"out of memory when loading trans.conf\n");
        goto lblFailure;
    }

    //load each transaction-bits-set configuration line
    ret=conf_get_first_string(fd,"trans.bitset",strLine);
    for(i=0; i < g_nSlotInBitLayoutTbl;)
    {
        char *msgtype = strtok(strLine, " \t");
        char *procCode= strtok(NULL, " \t");
        char *bitSet  = strtok(NULL, "\n");

        if( msgtype && procCode && bitSet)
        {
            g_BitLayoutTbl[i].msg_mti = atoi(msgtype);
            g_BitLayoutTbl[i].msg_proccode = atoi(procCode);
            if( 0 > set_bits_on(g_BitLayoutTbl[i].msg_bitmap,bitSet))
            {
                dcs_log(0,0,"'bitSet' misformat in some line in trans.conf\n");
                goto lblFailure;
            }

            i ++;
        }

        ret=conf_get_next_string(fd,"trans.bitset",strLine);
        if( ret < 0)
            break;
    }
    for(; i <= g_nSlotInBitLayoutTbl; i++)
        g_BitLayoutTbl[i].msg_mti = -1,
        g_BitLayoutTbl[i].msg_proccode =-1;

    //load each transaction-request-answer configuration line
    ret=conf_get_first_string(fd,"trans.askanswer",strLine);
    for(i=0; i < g_nSlotInQuestAnswerTbl;)
    {
        char *asktype = strtok(strLine, " \t");
        char *answertype = strtok(NULL, " \t\n\t");

        if( asktype && answertype)
        {
            g_QuestAnswerTbl[i].mti_quest = atoi(asktype);
            g_QuestAnswerTbl[i].mti_answer = atoi(answertype);
            i++;
        }

        ret=conf_get_next_string(fd,"trans.askanswer",strLine);
        if( ret < 0)
            break;
    }
    for(; i <= g_nSlotInQuestAnswerTbl; i++)
        g_QuestAnswerTbl[i].mti_quest = -1,
        g_QuestAnswerTbl[i].mti_answer = -1;

    conf_close(fd);
    return 0;

lblFailure:
    conf_close(fd);
    if(g_BitLayoutTbl) free(g_BitLayoutTbl);
    g_BitLayoutTbl = 0;
    if(g_QuestAnswerTbl) free(g_QuestAnswerTbl);
    g_QuestAnswerTbl = 0;
    return -1;
}
//
//////////////////////////////////////////////////////////////////////
//
//int dcs_ValMsgBits(IBDCSSTRUCT *pMsg, char *XmfMsg, int nMsgLen, int nFromWho)
//{
//    int nMTI, i, nProcCode, fCanMissing;
//    char *ptr;
//    unsigned char *pbitMap;
//    char buf[6];
//    
//    memset(buf,0,sizeof(buf));
//    IbdcsMsg_GetMTI(pMsg,buf);
//    nMTI = atol(buf);
//    for(fCanMissing = FALSE, i = 0;  g_BitLayoutTbl[i].msg_mti > 0; i++ )
//    {
//        if(nMTI != g_BitLayoutTbl[i].msg_mti)
//            continue;
//
//        if( g_BitLayoutTbl[i].msg_proccode == 0)
//            fCanMissing = TRUE;
//        break;
//    }
//
//    if( g_BitLayoutTbl[i].msg_mti < 0 )
//    {
//        //processing code missed
//        dcs_log(0,0,"MTI %.4d not supported by IBDCS and discard it.\n", nMTI);
//        goto lbl_ValErr;
//    }
//
//    ptr = IbdcsMsg_GetBitVal(pMsg, 3);
//    if(ptr == NULL)
//    {
//        if( !fCanMissing )
//        {
//            //processing code missed
//            dcs_log(0,0,"bit003(processing code) missing "
//                        "in %.4d msg and discard it.\n", nMTI);
//            goto lbl_ValErr;
//        }
//        else
//            nProcCode = 0;
//    }
//    else
//        nProcCode = atoi(ptr);
//
//    for(pbitMap = NULL, i = 0;  g_BitLayoutTbl[i].msg_mti > 0; i++ )
//    {
//        if(nMTI != g_BitLayoutTbl[i].msg_mti)
//            continue;
//        if(nProcCode != g_BitLayoutTbl[i].msg_proccode)
//            continue;
//
//        pbitMap = g_BitLayoutTbl[i].msg_bitmap;
//        break;
//    }
//
//    if(!pbitMap)
//    {
//        //the (MTI, ProcCode) not supported by us
//        dcs_log(0,0,"message (%.4d, %.6d) not supported by "
//                     "IBDCS and discard it.\n",
//                     nMTI, nProcCode);
//        goto lbl_ValErr;
//    }
//
//    ptr = IbdcsMsg_GetBitVal(pMsg, 39);
//    if( ptr==NULL || memcmp(ptr, "00", 2)==0 )
//    {
//        //all predefined bits must present in message
//        for(i=0; i<128; i++)
//        {
//            if( !ISBITSET(pbitMap, i) ) continue;
//            if(IbdcsMsg_IsBitExist(pMsg,i+1)) continue;
//
//            dcs_log(0,0,"bit%.3d missing in (%.4d,%.6d) "
//                         "message and discard it.\n",
//                         i+1,nMTI,nProcCode);
//            goto lbl_ValErr;
//        }
//    }
//    else
//    {
//        for(i=0; i<128; i++)
//        {
//            if( !ISBITSET(pbitMap, i) ) continue;
//            if(IbdcsMsg_IsBitExist(pMsg,i+1)) continue;
//
//            dcs_log(0,0,"WARNING: bit%.3d missing in (%.4d,%.6d)\n",
//                         i+1,nMTI,nProcCode);
//        }
//    }
//
//    return 0;
//
//lbl_ValErr:
//    if(nFromWho == INST_SWITCH && nMTI != 250)
//    {
//        memcpy(XmfMsg, "0250", 4);  //0250 misformat message notification
//        Forward_To_Switch(XmfMsg,  nMsgLen);
//        return -1;
//    }
//
//    return -1;
//}
//
//////////////////////////////////////////////////////////////////////
//
//#define CHK_LENGTH     1
//#define CHK_FORMAT     2
//#define CHK_DATETIME   3
//
//static  int g_nChkWhich = CHK_LENGTH;
//
//int dcs_ValBitLex(IBDCSSTRUCT *pMsg, char *XmfMsg, int nMsgLen, int nFromWho)
//{
//    int i, nLen;
//    char   *pBit;
//    char   buf[6];
//    int nMTI ;
//    memset(buf,0,sizeof(buf));
//    IbdcsMsg_GetMTI(pMsg,buf);
//    nMTI     = atoi(buf);
//
//    for(i = 1; i < 128;i++)
//    {
//        if(!IbdcsMsg_IsBitExist(pMsg,i+1))
//            continue;
//
//        nLen = IbdcsMsg_GetBitLength(pMsg,i+1);
//        pBit = IbdcsMsg_GetBitVal(pMsg,i+1);
//        g_nChkWhich = CHK_LENGTH;
//
//        switch(i+1)
//        {
//            case 2: //PAN (N..19)
//                if( nLen > 19) goto lbl_ValErr;
//                //if(!_IsNumericString(pBit, nLen))  goto lbl_ValErr;
//                break;
//
//            case 3: //processing code(N6)
//                if( nLen != 6) goto lbl_ValErr;
//                if(!_IsNumericString(pBit, nLen))  goto lbl_ValErr;
//                break;
//
//            case 4: //amount, transaction(N12)
//                if( nLen != 12) goto lbl_ValErr;
//                if(!_IsNumericString(pBit, nLen))  goto lbl_ValErr;
//                break;
//
//            case 5: //amount, settlement
//            case 6: //amount, cardholder billing
//                break;
//
//            case 7: //transmission date & time(N10)
//                if( nLen != 10) goto lbl_ValErr;
//                if(!_IsDateTime(pBit, nLen, FMT_MMDDHHMMSS))
//                    goto lbl_ValErr;
//                break;
//
//            case 8:  //amount, cardholder billing fee
//            case 9:  //conversion rate, settlement
//            case 10: //conversion rate, cardholder billing
//                break;
//
//            case 11: //systems trace audit number(N6)
//                if( nLen != 6) goto lbl_ValErr;
//                if(!_IsNumericString(pBit, nLen))  goto lbl_ValErr;
//                break;
//
//            case 12: //time, local transaction(N6)
//                if( nLen != 6) goto lbl_ValErr;
//                //if(!_IsDateTime(pBit, nLen, FMT_HHMMSS))
//                  //  goto lbl_ValErr;
//                break;
//
//            case 13: //date, local transaction(N4)
//            case 15: //date, settlement(N4)
//            case 17: //date, capture(N4)
//                if( nLen != 4) goto lbl_ValErr;
//                //if(!_IsDateTime(pBit, nLen, FMT_MMDD))
//                  //  goto lbl_ValErr;
//                break;
//
//            case 14: //date, expiration
//            case 16: //date, conversion
//                break;
//
//            case 18: //merchant type(N4)
//                if( nLen != 4) goto lbl_ValErr;
//                if(!_IsNumericString(pBit, nLen))  goto lbl_ValErr;
//                break;
//
//            case 19: //acquiring institution country code
//            case 20: //pan extended, country code
//            case 21: //forwarding institution. country code
//                break;
//
//            case 22: //point of service entry mode(N3)
//                if( nLen != 3) goto lbl_ValErr;
//                if(!_IsNumericString(pBit, nLen))  goto lbl_ValErr;
//                break;
//
//            case 23:
//            case 24:
//            case 25:
//            case 26:
//            case 27:
//            case 29:
//                break;
//
//            case 28: //amount, transaction fee(X+N8)
//            case 30: //amount, transaction processing fee(X+N8)
//                if( nLen != 9) goto lbl_ValErr;
//                if(!_IsNumericString(pBit+1, nLen-1))  goto lbl_ValErr;
//                break;
//
//            case 31:
//                break;
//
//            case 32: //acquiring institution id code(N..11)
//            case 33: //forwarding institution id code(N..11)
//                if( nLen > 11) goto lbl_ValErr;
//                if(!_IsNumericString(pBit, nLen))  goto lbl_ValErr;
//                break;
//
//            case 34:
//                break;
//
//            case 35: //track 2 data(LLVAR..37)
//                if( nLen > 37) goto lbl_ValErr;
//                break;
//
//            case 36: //track 3 data(LLLVAR..104)
//                if( nLen > 104) goto lbl_ValErr;
//                break;
//
//            case 37: //retrieval reference number(AN12)
//                if( nLen != 12) goto lbl_ValErr;
//                break;
//
//            case 38: //authorisation id response(AN6)
//                if( nLen != 6) goto lbl_ValErr;
//                break;
//
//            case 39: //response code(AN2)
//                if( nLen != 2) goto lbl_ValErr;
//                break;
//
//            case 40: //service restriction code(AN3)
//                if( nLen != 3) goto lbl_ValErr;
//                break;
//
//            case 41: //card acceptor terminal identification(ANS8)
//                if( nLen != 8) goto lbl_ValErr;
//                break;
//
//            case 42: //card acceptor identification code(ANS15)
//                if( nLen != 15) goto lbl_ValErr;
//                break;
//
//            case 43: //Card acceptor name/location(ANS40)
//                if( nLen != 40) goto lbl_ValErr;
//                break;
//
//            case 44: //additional response data(ANS..25)
//                if( nLen > 25) goto lbl_ValErr;
//                break;
//
//            case 45:
//            case 46:
//            case 47:
//                break;
//
//            case 48: //additional data(ANS...999)
//                if( nLen > 999) goto lbl_ValErr;
//                break;
//
//            case 49: //currency code, transaction(ANS3)
//            case 50: //currency code, settlement(ANS3)
//                if( nLen != 3) goto lbl_ValErr;
//                break;
//
//            case 51:
//                break;
//
//            case 53:
//                break;
//
//            case 54: //additional amounts(ANS..120)
//                if( nLen > 120) goto lbl_ValErr;
//                break;
//
//            case 60: //MAC used flag(N1)
//                if( nLen != 1) goto lbl_ValErr;
//                if(!_IsNumericString(pBit, nLen))  goto lbl_ValErr;
//                break;
//
//            case 62: //cause code(N4)
//                if( nLen != 4) goto lbl_ValErr;
//                if(!_IsNumericString(pBit, nLen))  goto lbl_ValErr;
//                break;
//
//            case 63: //settlement batch number(N6)
//                if( nLen != 6) goto lbl_ValErr;
//                if(!_IsNumericString(pBit, nLen))  goto lbl_ValErr;
//                break;
//
//            case 66: //settlement result code(N1)
//                if( nLen != 1) goto lbl_ValErr;
//                if(!_IsNumericString(pBit, nLen))  goto lbl_ValErr;
//                break;
//
//            case 70: //network management information code(N3)
//                if( nLen != 3) goto lbl_ValErr;
//                if(!_IsNumericString(pBit, nLen))  goto lbl_ValErr;
//                break;
//
//            case 71: //message number(N4)
//            case 72: //message number, last(N4)
//                if( nLen != 4) goto lbl_ValErr;
//                if(!_IsNumericString(pBit, nLen))  goto lbl_ValErr;
//                break;
//
//            case 73: //date, action(N6)
//                if( nLen != 4) goto lbl_ValErr;
//                if(!_IsDateTime(pBit, nLen, FMT_YYMMDD))
//                    goto lbl_ValErr;
//                break;
//
//            case 74: //N10
//            case 75:
//            case 76:
//            case 77:
//            case 78:
//            case 79:
//            case 81:
//                if( nLen != 10) goto lbl_ValErr;
//                if(!_IsNumericString(pBit, nLen))  goto lbl_ValErr;
//                break;
//
//            case 82: //N12
//            case 84:
//                if( nLen != 12) goto lbl_ValErr;
//                if(!_IsNumericString(pBit, nLen))  goto lbl_ValErr;
//                break;
//
//            case 86: //N16
//            case 87:
//            case 88:
//            case 89:
//                if( nLen != 16) goto lbl_ValErr;
//                if(!_IsNumericString(pBit, nLen))  goto lbl_ValErr;
//                break;
//
//            case 90:
//                if( nLen != 42) goto lbl_ValErr;
//                break;
//
//            case 91:
//                if( nLen != 1) goto lbl_ValErr;
//                break;
//
//            case 97: //x+N16
//                if( nLen != 17) goto lbl_ValErr;
//                if(!_IsNumericString(pBit+1, nLen-1))  goto lbl_ValErr;
//                break;
//                break;
//
//            case 100: //receiving institution id code(N..11)
//                if( nLen > 11) goto lbl_ValErr;
//                if(!_IsNumericString(pBit, nLen))  goto lbl_ValErr;
//                break;
//
//            case 101: //file name(ANS..17)
//                if( nLen != 17) goto lbl_ValErr;
//                break;
//
//            case  52:  //personal identification number data
//            case  64:  //message authentication code
//            case  96:  //message security code
//            case  128: //message authentication code
//                if( nLen != 16) goto lbl_ValErr;
//                if(!_IsHexString(pBit, nLen))  goto lbl_ValErr;
//                break;
//         }//switch
//    }//for
//
//    return 0;
//
//lbl_ValErr:
//
//    switch( g_nChkWhich )
//    {
//        case CHK_LENGTH:
//            dcs_log(pBit,nLen,"ERR:bytes number of bit_%.3d incorrect.\n",i+1);
//            break;
//
//        case CHK_FORMAT:
//            dcs_log(pBit,nLen,"ERR:invalid char existing in bit_%.3d.\n",i+1);
//            break;
//
//        case CHK_DATETIME:
//        default:
//            dcs_log(pBit,nLen,"ERR:invalid date-time string of bit_%.3d.\n",i+1);
//            break;
//
//    }//switch
//
//    if(nFromWho == INST_SWITCH && nMTI != 250)
//    {
//        memcpy(XmfMsg, "0250", 4);  //0250 misformat message notification
//
//        Forward_To_Switch(XmfMsg,  nMsgLen);
//        return -1;
//    }
//
//    return -1;
//}
//
//static int _IsNumericString(char *str, int n)
//{
//    int i;
//
//    for(i=0; i<n; i++)
//        if(!isdigit(str[i]))
//        {
//            g_nChkWhich = CHK_FORMAT;
//            return 0;
//        }
//
//    return 1;
//}
//
//static int _IsHexString(char *str, int n)
//{
//    int i;
//
//    for(i=0; i<n; i++)
//        if(!isxdigit(str[i]))
//        {
//            g_nChkWhich = CHK_FORMAT;
//            return 0;
//        }
//
//    return 1;
//}
//
//#define  CHAR2_TO_INT(a, b)  (((a) - '0') * 10 + ((b) - '0'))
//
//static int _IsDateTime(char *str, int n, int format)
//{
//    struct tm  *ptm;
//    time_t  t;
//    int year, month, day, hour, minute, second;
//
//    if( !_IsNumericString(str, n) )
//        goto lbl_Err;
//
//    time(&t);
//    ptm = localtime(&t);
//
//    switch(format)
//    {
//        case FMT_MMDDHHMMSS:
//            month = CHAR2_TO_INT(str[0] , str[1]);
//            day   = CHAR2_TO_INT(str[2] , str[3]);
//            if(!IsValidYYMMDD(ptm->tm_year+1900, month, day))
//                goto lbl_Err;
//
//            hour  = CHAR2_TO_INT(str[4] , str[5]);
//            minute= CHAR2_TO_INT(str[6] , str[7]);
//            second= CHAR2_TO_INT(str[8] , str[9]);
//
//            if(!IsValidHHMMSS(hour, minute, second))
//                goto lbl_Err;
//            return 1;
//
//        case FMT_HHMMSS:
//            hour  = CHAR2_TO_INT(str[0] , str[1]);
//            minute= CHAR2_TO_INT(str[2] , str[3]);
//            second= CHAR2_TO_INT(str[4] , str[5]);
//
//            if(!IsValidHHMMSS(hour, minute, second))
//                goto lbl_Err;
//            return 1;
//
//        case FMT_MMDD:
//            month = CHAR2_TO_INT(str[0] , str[1]);
//            day   = CHAR2_TO_INT(str[2] , str[3]);
//
//            if(!IsValidYYMMDD(ptm->tm_year+1900, month, day))
//                goto lbl_Err;
//            return 1;
//
//
//        case FMT_YYMMDD:
//            year = CHAR2_TO_INT(str[0] , str[1]) + 2000;
//            month= CHAR2_TO_INT(str[2] , str[3]);
//            day  = CHAR2_TO_INT(str[4] , str[5]);
//
//            if(!IsValidYYMMDD(year, month, day))
//                goto lbl_Err;
//            return 1;
//
//        default:
//            return 1;
//    }//switch
//
//lbl_Err:
//    g_nChkWhich = CHK_DATETIME;
//    return 0;
//}

int IsValidYYMMDD(int yy, int mm, int dd)
{
    int dayTbl[2][13] = {{0, 31, 28, 31, 30, 31, 30, 31, 31,30,31, 30, 31},
                         {0, 31, 29, 31, 30, 31, 30, 31, 31,30,31, 30, 31}};
    int leap;

    if(mm <0 || mm > 12)
        return 0;

    if( yy % 100 == 0)
        leap = (yy % 400 == 0) ? 1 : 0;
    else
        leap = (yy % 4   == 0) ? 1 : 0;

    if( 1 <= dd  && dd <= dayTbl[leap][mm])
        return 1;
    return  0;
}

int IsValidHHMMSS(int hh, int mm, int ss)
{
    if(hh <0 || hh>23)  return 0;
    if(mm <0 || mm>59)  return 0;
    if(ss <0 || ss>59)  return 0;
    return 1;
}

int dcs_print_tbl(FILE *fp)
{
    int i, j, k;

    for(i=0; g_BitLayoutTbl[i].msg_mti > 0; i++)
    {
        fprintf(fp, "trans.bitset %.4d  %.6d  {",
                g_BitLayoutTbl[i].msg_mti, g_BitLayoutTbl[i].msg_proccode);
        for(k=0, j=0; j<128; j++)
//            if( ISBITSET(g_BitLayoutTbl[i].msg_bitmap, j)) //Freax TO DO: 
            {
                if(k==0)
                    fprintf(fp, "%d", j+1);
                else
                    fprintf(fp, ",%d", j+1);
                k++;
            }


        fprintf(fp, "}\n");
    }

    fprintf(fp, "\n\n\n");
    for(i=0; g_QuestAnswerTbl[i].mti_quest > 0; i++)
        fprintf(fp, "trans.askanswer %.4d  %.4d \n",
                g_QuestAnswerTbl[i].mti_quest,g_QuestAnswerTbl[i].mti_answer);
    
    return 0;
    
}
