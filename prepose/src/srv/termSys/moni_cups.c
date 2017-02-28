#include "ibdcs.h"
#include "trans.h"

extern struct ISO_8583 *iso8583;
extern struct ISO_8583 iso8583posconf[128];
extern struct ISO_8583 iso8583_conf[128];
extern struct ISO_8583 iso8583conf[128];

/* 
	模拟CUPS
 */
void moni_cups(char * srcBuf,int iFid , int iMid, int iLen)
{
	#ifdef __TEST__
		dcs_debug(srcBuf, iLen,"模拟CUPS, iFid=%d, iMid=%d, iLen=%d", iFid, iMid, iLen);
	#endif
	
	int rtn, s, numcase,n;
	char folderName[64],tmpbuf[512],bit_22[4],trace[7],extkey[20],databuf[20],cdno[20],macblock[1024],posmac[9];
	char signtrancode[3],signflg[4],tran_batch_no[7];
	
	//交易配置表
	MSGCHGINFO	msginfo;
	TERMMSGSTRUCT termbuf;
	
	//终端密钥表
	SECUINFO secuinfo;
	ISO_data siso;
	
	//交易记录
	PEP_JNLS pep_jnls;
	
	//终端商户记录表
	MER_INFO_T mer_info_t;
	
	struct  tm *tm;   
	time_t  t;

	memset(tmpbuf,0,sizeof(tmpbuf));
	memset(trace,0,sizeof(trace));
	memset(bit_22,0,sizeof(bit_22));
	memset(folderName,0,sizeof(folderName));
	memset(&msginfo,0,sizeof(MSGCHGINFO));
	memset(&siso,0,sizeof(ISO_data));
	memset(&pep_jnls,0,sizeof(PEP_JNLS));
	memset(&termbuf,0,sizeof(TERMMSGSTRUCT));
	memset(extkey, 0, sizeof(extkey));
	memset(&secuinfo, 0, sizeof(SECUINFO));
	memset(&mer_info_t, 0, sizeof(MER_INFO_T));
	memset(databuf, 0, sizeof(databuf));
	
	
	time(&t);
    tm = localtime(&t);
    
    sprintf(databuf,"%02d%02d%02d%02d%02d", tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
    dcs_log(0,0,"databuf1=[%s]",databuf);
    
    s = posUnpackwu(srcBuf, &siso, &mer_info_t, iLen);
    if(s < 0)
    {
    	dcs_log(0,0,"posUnpack error!");
    	return;
    }
    
    memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(&siso,0,(unsigned char *)tmpbuf);
	
	sscanf(tmpbuf, "%d", &numcase);
	switch(numcase)
	{
		case 800:		
			memset(tmpbuf, 0, sizeof(tmpbuf));
			s = getbit(&siso, 60, (unsigned char *)tmpbuf);
			if(s >0)
			{
				memset(signtrancode, 0, sizeof(signtrancode));
				memset(tran_batch_no, 0, sizeof(tran_batch_no));
				memset(signflg, 0, sizeof(signflg));
				
				memcpy(signtrancode, tmpbuf, 2);
				memcpy(tran_batch_no, tmpbuf+2, 6);
				memcpy(signflg, tmpbuf+8, 3);
				
				dcs_log(0,0,"交易类型码:%s",signtrancode);
				dcs_log(0,0,"批次号:%s",tran_batch_no);
				dcs_log(0,0,"密钥长度:%s",signflg);
				
			}
			//终端签到
			if(memcmp(signtrancode, "00", 2) == 0 && ( memcmp(signflg, "001", 3) == 0 || memcmp(signflg, "003", 3) == 0 || memcmp(signflg, "004", 3) == 0) )
			{
				setbit(&siso,0,(unsigned char *)"0810", 4);
				setbit(&siso,12,(unsigned char *)databuf+4, 6);
				setbit(&siso,13,(unsigned char *)databuf, 4);
				
				memset(tmpbuf, 0, sizeof(tmpbuf));
				getbit(&siso,11,(unsigned char *)tmpbuf);
				memcpy(tmpbuf+6, databuf+4,6);
				setbit(&siso,37,(unsigned char *)tmpbuf, 12);
				
				setbit(&siso,32,(unsigned char *)"40002900", 8);
				setbit(&siso,39,(unsigned char *)"00", 2);
				
				//单倍长
				if(memcmp(signflg, "001", 3) == 0)
				{
					memset(tmpbuf, 0, sizeof(tmpbuf));
					asc_to_bcd( (unsigned char *)tmpbuf, (unsigned char*)"F8944E67A86060AE82E13665F8944E67A86060AE82E13665", 48, 0 );
					setbit(&siso,62,(unsigned char *)tmpbuf, 24);
				}else if(memcmp(signflg, "003", 3) == 0) //双倍长
				{
					memset(tmpbuf, 0, sizeof(tmpbuf));
					asc_to_bcd( (unsigned char *)tmpbuf, (unsigned char*)"F8944E67A86060AEC7A68E81D07274F8D2B91CC5F8944E67A86060AE000000000000000082E13665", 80, 0 );
					setbit(&siso,62,(unsigned char *)tmpbuf, 40);
				}else //磁道双倍长
				{
					memset(tmpbuf, 0, sizeof(tmpbuf));
					asc_to_bcd( (unsigned char *)tmpbuf, (unsigned char*)"F8944E67A86060AEC7A68E81D07274F8D2B91CC5F8944E67A86060AE000000000000000082E13665F8944E67A86060AEC7A68E81D07274F8D2B91CC5", 120, 0 );
					setbit(&siso,62,(unsigned char *)tmpbuf, 60);
				}
				setbit(&siso,63,(unsigned char *)NULL, 0);
				s = sndBackToPoswu(siso, iFid, mer_info_t);
				if(s < 0 )
				{
					dcs_log(0,0,"posUnpack error!");
				}
				break;
			}
		break;
		case 100:
			memset(tmpbuf, 0, sizeof(tmpbuf));
			getbit(&siso, 3, (unsigned char *)tmpbuf);
			
			//预授权
			if( memcmp(tmpbuf, "030000", 6) == 0 )
			{	
				setbit(&siso,0,(unsigned char *)"0110", 4);
				setbit(&siso,12,(unsigned char *)databuf+4, 6);
				setbit(&siso,13,(unsigned char *)databuf, 4);
				setbit(&siso,15,(unsigned char *)databuf, 4);
				
				memset(tmpbuf, 0, sizeof(tmpbuf));
				getbit(&siso,11,(unsigned char *)tmpbuf);
				memcpy(tmpbuf+6, databuf+4,6);
//				setbit(&siso,37,(unsigned char *)tmpbuf, 12);
				//setbit(&siso,38,(unsigned char *)"123456", 6);
				setbit(&siso,38,(unsigned char *)tmpbuf+6, 6);
				memset(tmpbuf, 0, sizeof(tmpbuf));
				s = getbit(&siso, 35, (unsigned char *)tmpbuf);
				if(s > 0)
				{
					for(n=0; n<20; n++)
					{
						if(tmpbuf[n] == '=' )
							break;
					}
				   	memset(cdno, 0, sizeof(cdno));
					memcpy(cdno, tmpbuf, n);
				        	
					dcs_log(0,0, "cdno=%s", cdno);
					setbit(&siso, 2, (unsigned char *)cdno, n);
					
				}else
				{
					dcs_log(0,0, "二磁不存在");
				}
				
				
				setbit(&siso,32,(unsigned char *)"40002900", 8);

				setbit(&siso,39,(unsigned char *)"00", 2);
				
				/*
				在交易响应消息中返回接收机构和收单机构的标识码。
				——	接收机构标识码      AN11	表示发卡行标识码（左靠，右部多余部分填空格）
				——	收单机构标识码      AN11	表示商户结算行标识码（左靠，右部多余部分填空格）
				*/
				setbit(&siso,44,(unsigned char *)"03012900   04032900   ", 22);				
				setbit(&siso,63,(unsigned char *)"CUP", 3);
				
				
				setbit(&siso,22,(unsigned char *)NULL, 0);
				setbit(&siso,26,(unsigned char *)NULL, 0);
				setbit(&siso,35,(unsigned char *)NULL, 0);
				setbit(&siso,36,(unsigned char *)NULL, 0);
				setbit(&siso,52,(unsigned char *)NULL, 0);
				//配合核心返回60,100域写死
				setbit(&siso,60,(unsigned char *)"000005000300000000000111000000", 30);
				setbit(&siso,100,(unsigned char *)"44200000", 8);

				memset(macblock, 0, sizeof(macblock));
				memset(posmac, 0, sizeof(posmac));
				
				s = GetPospMacBlk(&siso, macblock);
				//dcs_debug( macblock, s, "GetPospMacBlk Rtn len=%d.", s );
				
				s = GetPospMAC( macblock, s, "1111111111111111", posmac);
				//dcs_debug( posmac, 8, "posmac :");
				
				setbit(&siso, 128, (unsigned char *)posmac, 8);
				s = sndBackToPoswu(siso, iFid, mer_info_t);
				if(s < 0 )
				{
					dcs_log(0,0,"posUnpack error!");
				}
				break;
			}
			//预授权撤销
			if( memcmp(tmpbuf, "200000", 6) == 0 )
			{	
				setbit(&siso,0,(unsigned char *)"0110", 4);
				setbit(&siso,12,(unsigned char *)databuf+4, 6);
				setbit(&siso,13,(unsigned char *)databuf, 4);
				setbit(&siso,15,(unsigned char *)databuf, 4);
				
//				memset(tmpbuf, 0, sizeof(tmpbuf));
//				getbit(&siso,11,(unsigned char *)tmpbuf);
//				memcpy(tmpbuf+6, databuf+4,6);
//				setbit(&siso,37,(unsigned char *)tmpbuf, 12);
				
				memset(tmpbuf, 0, sizeof(tmpbuf));
				s = getbit(&siso, 35, (unsigned char *)tmpbuf);
				if(s > 0)
				{
					for(n=0; n<20; n++)
					{
						if(tmpbuf[n] == '=' )
							break;
					}
				   	memset(cdno, 0, sizeof(cdno));
					memcpy(cdno, tmpbuf, n);
				        	
					dcs_log(0,0, "cdno=%s", cdno);
					setbit(&siso, 2, (unsigned char *)cdno, n);
					
				}else
				{
					dcs_log(0,0, "二磁不存在");
				}
				
				
				setbit(&siso,32,(unsigned char *)"40002900", 8);
				setbit(&siso,39,(unsigned char *)"00", 2);
				
				/*
				在交易响应消息中返回接收机构和收单机构的标识码。
				——	接收机构标识码      AN11	表示发卡行标识码（左靠，右部多余部分填空格）
				——	收单机构标识码      AN11	表示商户结算行标识码（左靠，右部多余部分填空格）
				*/
				setbit(&siso,44,(unsigned char *)"03012900   04032900   ", 22);				
				setbit(&siso,63,(unsigned char *)"CUP", 3);
				
				
				setbit(&siso,22,(unsigned char *)NULL, 0);
				setbit(&siso,26,(unsigned char *)NULL, 0);
				setbit(&siso,35,(unsigned char *)NULL, 0);
				setbit(&siso,36,(unsigned char *)NULL, 0);
				setbit(&siso,52,(unsigned char *)NULL, 0);
				//配合核心返回60,100域写死
				setbit(&siso,60,(unsigned char *)"000005000300000000000111000000", 30);
				setbit(&siso,100,(unsigned char *)"44200000", 8);
				memset(macblock, 0, sizeof(macblock));
				memset(posmac, 0, sizeof(posmac));
				
				s = GetPospMacBlk(&siso, macblock);
				//dcs_debug( macblock, s, "GetPospMacBlk Rtn len=%d.", s );
				
				s = GetPospMAC( macblock, s, "1111111111111111", posmac);
				//dcs_debug( posmac, 8, "posmac :");
				
				setbit(&siso, 128, (unsigned char *)posmac, 8);
				
				s = sndBackToPoswu(siso, iFid, mer_info_t);
				if(s < 0 )
				{
					dcs_log(0,0,"posUnpack error!");
				}
				break;
			}
			break;
		
		case 200:		
			memset(tmpbuf, 0, sizeof(tmpbuf));
			getbit(&siso, 3, (unsigned char *)tmpbuf);
			
			//消费交易
			if( memcmp(tmpbuf, "000000", 6) == 0 )
			{
				setbit(&siso,0,(unsigned char *)"0210", 4);
				setbit(&siso,12,(unsigned char *)databuf+4, 6);
				setbit(&siso,13,(unsigned char *)databuf, 4);
				setbit(&siso,15,(unsigned char *)databuf, 4);
				setbit(&siso,14,(unsigned char *)"3712", 4);
				
				memset(tmpbuf, 0, sizeof(tmpbuf));
				getbit(&siso,11,(unsigned char *)tmpbuf);
				memcpy(tmpbuf+6, databuf+4,6);
//				setbit(&siso,37,(unsigned char *)tmpbuf, 12);

				char tmp[10];
				memset(tmp , 0, sizeof(tmp));
				getbit(&siso,25,(unsigned char *)tmp);
				if(memcmp(tmp, "00", 2) == 0)//消费
					setbit(&siso, 38,(unsigned char *)tmpbuf+6, 6);
				//setbit(&siso, 38,(unsigned char *)"123456", 6);

				memset(tmpbuf, 0, sizeof(tmpbuf));
				s = getbit(&siso, 35, (unsigned char *)tmpbuf);
				if(s > 0)
				{
					for(n=0; n<20; n++)
					{
						if(tmpbuf[n] == '=' )
							break;
					}
				   	memset(cdno, 0, sizeof(cdno));
					memcpy(cdno, tmpbuf, n);
				        	
					dcs_log(0,0, "cdno=%s", cdno);
					setbit(&siso, 2, (unsigned char *)cdno, n);
					
				}else
				{
					dcs_log(0,0, "二磁不存在");
				}
				
				
				setbit(&siso,32,(unsigned char *)"40002900", 8);
				setbit(&siso,39,(unsigned char *)"00", 2);
				
				/*
				在交易响应消息中返回接收机构和收单机构的标识码。
				——	接收机构标识码      AN11	表示发卡行标识码（左靠，右部多余部分填空格）
				——	收单机构标识码      AN11	表示商户结算行标识码（左靠，右部多余部分填空格）
				*/
				setbit(&siso,44,(unsigned char *)"03012900   04032900   ", 22);
								
				//setbit(&siso,44,(unsigned char *)"chinaebi", 8);
				
				setbit(&siso,63,(unsigned char *)"CUP", 3);
				
				
				setbit(&siso,22,(unsigned char *)NULL, 0);
				setbit(&siso,26,(unsigned char *)NULL, 0);
				setbit(&siso,35,(unsigned char *)NULL, 0);
				setbit(&siso,36,(unsigned char *)NULL, 0);
				setbit(&siso,52,(unsigned char *)NULL, 0);
				//配合核心返回60,100域写死
				setbit(&siso,60,(unsigned char *)"000005000300000000000111000000", 30);
				setbit(&siso,100,(unsigned char *)"44200000", 8);
				memset(macblock, 0, sizeof(macblock));
				memset(posmac, 0, sizeof(posmac));
				
				s = GetPospMacBlk(&siso, macblock);
				dcs_debug( macblock, s, "GetPospMacBlk Rtn len=%d.", s );
				
				s = GetPospMAC( macblock, s, "1111111111111111", posmac);
				dcs_debug( posmac, 8, "posmac :");
				
				setbit(&siso, 128, (unsigned char *)posmac, 8);
				//sleep(300000);
				s = sndBackToPoswu(siso, iFid, mer_info_t);
				if(s < 0 )
				{
					dcs_log(0,0,"posUnpack error!");
				}
				break;
			}
			//余额查询类的交易
			if( memcmp(tmpbuf, "300000", 6) == 0 )
			{
				setbit(&siso,0,(unsigned char *)"0210", 4);
				setbit(&siso,12,(unsigned char *)databuf+4, 6);
				setbit(&siso,13,(unsigned char *)databuf, 4);
//
//				memset(tmpbuf, 0, sizeof(tmpbuf));
//				getbit(&siso,11,(unsigned char *)tmpbuf);
//				memcpy(tmpbuf+6, databuf+4,6);
//				setbit(&siso,37,(unsigned char *)tmpbuf, 12);
				
				memset(tmpbuf, 0, sizeof(tmpbuf));
				s = getbit(&siso, 35, (unsigned char *)tmpbuf);
				if(s > 0)
				{
					for(n=0; n<20; n++)
					{
						if(tmpbuf[n] == '=' )
							break;
					}
				   	memset(cdno, 0, sizeof(cdno));
					memcpy(cdno, tmpbuf, n);
				        	
					dcs_log(0,0, "cdno=%s", cdno);
					setbit(&siso, 2, (unsigned char *)cdno, n);
					
				}else
				{
					dcs_log(0,0, "二磁不存在");
				}
				
				
				setbit(&siso,32,(unsigned char *)"40002900", 8);
				setbit(&siso,39,(unsigned char *)"00", 2);
				/*
				在交易响应消息中返回接收机构和收单机构的标识码。
				——	接收机构标识码      AN11	表示发卡行标识码（左靠，右部多余部分填空格）
				——	收单机构标识码      AN11	表示商户结算行标识码（左靠，右部多余部分填空格）
				*/
				setbit(&siso,44,(unsigned char *)"03012900   04032900   ", 22);
				//setbit(&siso,44,(unsigned char *)"chinaebi", 8);	
				setbit(&siso,54,(unsigned char *)"0210156C000002700000", 20);
				
				setbit(&siso,22,(unsigned char *)NULL, 0);
				setbit(&siso,26,(unsigned char *)NULL, 0);
				setbit(&siso,35,(unsigned char *)NULL, 0);
				setbit(&siso,36,(unsigned char *)NULL, 0);
				setbit(&siso,52,(unsigned char *)NULL, 0);
				setbit(&siso,57,(unsigned char *)"   ", 3);
				setbit(&siso,62,(unsigned char *)NULL, 0);
				//配合核心返回60,100域写死
				setbit(&siso,60,(unsigned char *)"000005000300000000000111000000", 30);
				setbit(&siso,100,(unsigned char *)"44200000", 8);
				memset(macblock, 0, sizeof(macblock));
				memset(posmac, 0, sizeof(posmac));
				
				s = GetPospMacBlk(&siso, macblock);
				dcs_debug( macblock, s, "GetPospMacBlk Rtn len=%d.", s );
				
				s = GetPospMAC( macblock, s, "1111111111111111", posmac);
				dcs_debug( posmac, 8, "posmac :");
				
				setbit(&siso, 128, (unsigned char *)posmac, 8);
				
				s = sndBackToPoswu(siso, iFid, mer_info_t);
				if(s < 0 )
				{
					dcs_log(0,0,"posUnpack error!");
				}
			
				break;
			}
			//消费撤销
			if( memcmp(tmpbuf, "200000", 6) == 0 )
			{
				dcs_log(0,0, "消费撤销=[%s]",tmpbuf);
				setbit(&siso,0,(unsigned char *)"0210", 4);
				setbit(&siso,12,(unsigned char *)databuf+4, 6);
				setbit(&siso,13,(unsigned char *)databuf, 4);
				setbit(&siso,15,(unsigned char *)databuf, 4);
//				memset(tmpbuf, 0, sizeof(tmpbuf));
//				getbit(&siso,11,(unsigned char *)tmpbuf);
//				memcpy(tmpbuf+6, databuf+4,6);
//				setbit(&siso,37,(unsigned char *)tmpbuf, 12);
				
				memset(tmpbuf, 0, sizeof(tmpbuf));
				s = getbit(&siso, 35, (unsigned char *)tmpbuf);
				if(s > 0)
				{
					for(n=0; n<20; n++)
					{
						if(tmpbuf[n] == '=' )
							break;
					}
				   	memset(cdno, 0, sizeof(cdno));
					memcpy(cdno, tmpbuf, n);
				        	
					dcs_log(0,0, "cdno=%s", cdno);
					setbit(&siso, 2, (unsigned char *)cdno, n);
					
				}
				else
				{
					dcs_log(0,0, "二磁不存在");
				}
				
				
				setbit(&siso,32,(unsigned char *)"40002900", 8);
				setbit(&siso,39,(unsigned char *)"00", 2);
				
				/*
				在交易响应消息中返回接收机构和收单机构的标识码。
				——	接收机构标识码      AN11	表示发卡行标识码（左靠，右部多余部分填空格）
				——	收单机构标识码      AN11	表示商户结算行标识码（左靠，右部多余部分填空格）
				*/
				setbit(&siso,44,(unsigned char *)"03012900   04032900   ", 22);
				//setbit(&siso,44,(unsigned char *)"chinaebi", 8);
				setbit(&siso,63,(unsigned char *)"CUP", 3);
				
				setbit(&siso,22,(unsigned char *)NULL, 0);
				setbit(&siso,26,(unsigned char *)NULL, 0);
				setbit(&siso,35,(unsigned char *)NULL, 0);
				setbit(&siso,36,(unsigned char *)NULL, 0);
				setbit(&siso,52,(unsigned char *)NULL, 0);
				setbit(&siso,61,(unsigned char *)NULL, 0);
				//配合核心返回60,100域写死
				setbit(&siso,60,(unsigned char *)"000005000300000000000111000000", 30);
				setbit(&siso,100,(unsigned char *)"44200000", 8);
				memset(macblock, 0, sizeof(macblock));
				memset(posmac, 0, sizeof(posmac));
				
				s = GetPospMacBlk(&siso, macblock);
				dcs_debug( macblock, s, "GetPospMacBlk Rtn len=%d.", s );
				
				s = GetPospMAC( macblock, s, "1111111111111111", posmac);
				dcs_debug( posmac, 8, "posmac :");
				
				setbit(&siso, 128, (unsigned char *)posmac, 8);
				//sleep(300000);
				s = sndBackToPoswu(siso, iFid, mer_info_t);
				if(s < 0 )
				{
					dcs_log(0,0,"posUnpack error!");
				}
				break;
			}
		break;
		//退货
		
		case 220:
			memset(tmpbuf, 0, sizeof(tmpbuf));
			getbit(&siso, 3, (unsigned char *)tmpbuf);
			
			//退货类的交易交易处理码是200000
			if( memcmp(tmpbuf, "200000", 6) == 0 )
			{
				setbit(&siso,0,(unsigned char *)"0230", 4);
				setbit(&siso,12,(unsigned char *)databuf+4, 6);
				setbit(&siso,13,(unsigned char *)databuf, 4);
				setbit(&siso,15,(unsigned char *)databuf, 4);
//				memset(tmpbuf, 0, sizeof(tmpbuf));
//				getbit(&siso,11,(unsigned char *)tmpbuf);
//				memcpy(tmpbuf+6, databuf+4,6);
//				setbit(&siso,37,(unsigned char *)tmpbuf, 12);
				
				memset(tmpbuf, 0, sizeof(tmpbuf));
				s = getbit(&siso, 35, (unsigned char *)tmpbuf);
				if(s > 0)
				{
					for(n=0; n<20; n++)
					{
						if(tmpbuf[n] == '=' )
							break;
					}
				   	memset(cdno, 0, sizeof(cdno));
					memcpy(cdno, tmpbuf, n);
				        	
					dcs_log(0,0, "cdno=%s", cdno);
					setbit(&siso, 2, (unsigned char *)cdno, n);
					
				}
				else
				{
					dcs_log(0,0, "二磁不存在");
				}
				
				setbit(&siso,32,(unsigned char *)"40002900", 8);
				setbit(&siso,39,(unsigned char *)"00", 2);
				/*
				在交易响应消息中返回接收机构和收单机构的标识码。
				——	接收机构标识码      AN11	表示发卡行标识码（左靠，右部多余部分填空格）
				——	收单机构标识码      AN11	表示商户结算行标识码（左靠，右部多余部分填空格）
				*/
				setbit(&siso,44,(unsigned char *)"03012900   04032900   ", 22);
				//setbit(&siso,44,(unsigned char *)"chinaebi", 8);
				setbit(&siso,63,(unsigned char *)"CUP", 3);
				
				setbit(&siso,22,(unsigned char *)NULL, 0);
				setbit(&siso,26,(unsigned char *)NULL, 0);
				setbit(&siso,35,(unsigned char *)NULL, 0);
				setbit(&siso,36,(unsigned char *)NULL, 0);
				setbit(&siso,61,(unsigned char *)NULL, 0);
				//配合核心返回60,100域写死
				setbit(&siso,60,(unsigned char *)"000005000300000000000111000000", 30);
				setbit(&siso,100,(unsigned char *)"44200000", 8);
				memset(macblock, 0, sizeof(macblock));
				memset(posmac, 0, sizeof(posmac));
				
				s = GetPospMacBlk(&siso, macblock);
				dcs_debug( macblock, s, "GetPospMacBlk Rtn len=%d.", s );
				
				s = GetPospMAC( macblock, s, "1111111111111111", posmac);
				dcs_debug( posmac, 8, "posmac :");
				
				setbit(&siso, 128, (unsigned char *)posmac, 8);
				
				s = sndBackToPoswu(siso, iFid, mer_info_t);
				if(s < 0 )
				{
					dcs_log(0,0,"posUnpack error!");
				}
				break;
			}
			// 脱机消费上送 000000
			else if( memcmp(tmpbuf, "000000", 6) == 0 )
			{
				setbit(&siso,0,(unsigned char *)"0230", 4);
				setbit(&siso,12,(unsigned char *)databuf+4, 6);
				setbit(&siso,13,(unsigned char *)databuf, 4);
				setbit(&siso,15,(unsigned char *)databuf, 4);

				setbit(&siso,32,(unsigned char *)"40002900", 8);
				setbit(&siso,39,(unsigned char *)"00", 2);
				setbit(&siso,63,(unsigned char *)"CUP", 3);

				setbit(&siso,14,(unsigned char *)NULL, 0);
				setbit(&siso,22,(unsigned char *)NULL, 0);
				setbit(&siso,35,(unsigned char *)NULL, 0);
				setbit(&siso,36,(unsigned char *)NULL, 0);
				setbit(&siso,43,(unsigned char *)NULL, 0);
				setbit(&siso,55,(unsigned char *)NULL, 0);
				setbit(&siso,61,(unsigned char *)NULL, 0);
				//配合核心返回60,100域写死
				setbit(&siso,60,(unsigned char *)"000005000300000000000111000000", 30);
				setbit(&siso,100,(unsigned char *)"44200000", 8);
				memset(macblock, 0, sizeof(macblock));
				memset(posmac, 0, sizeof(posmac));

				s = GetPospMacBlk(&siso, macblock);
				dcs_debug( macblock, s, "GetPospMacBlk Rtn len=%d.", s );

				s = GetPospMAC( macblock, s, "1111111111111111", posmac);
				dcs_debug( posmac, 8, "posmac :");

				setbit(&siso, 128, (unsigned char *)posmac, 8);

				s = sndBackToPoswu(siso, iFid, mer_info_t);
				if(s < 0 )
				{
					dcs_log(0,0,"posUnpack error!");
				}
				break;
			}
			break;
		case 420:
			memset(tmpbuf, 0, sizeof(tmpbuf));
			getbit(&siso, 3, (unsigned char *)tmpbuf);
			
			//消费冲正交易
			if( memcmp(tmpbuf, "000000", 6) == 0 )
			{
				setbit(&siso,0,(unsigned char *)"0430", 4);
				setbit(&siso,12,(unsigned char *)databuf+4, 6);//时间
				setbit(&siso,13,(unsigned char *)databuf, 4);//日期
				setbit(&siso,15,(unsigned char *)databuf, 4);
				
//				memset(tmpbuf, 0, sizeof(tmpbuf));
//				getbit(&siso,11,(unsigned char *)tmpbuf);
//				memcpy(tmpbuf+6, databuf+4,6);
//				setbit(&siso,37,(unsigned char *)tmpbuf, 12);
				
				
				//setbit(&siso,32,(unsigned char *)"40002900", 8);
				setbit(&siso,39,(unsigned char *)"00", 2);
				
				setbit(&siso,44,(unsigned char *)"03012900   04032900   ", 22);
				
				
				setbit(&siso,22,(unsigned char *)NULL, 0);
				setbit(&siso,62,(unsigned char *)NULL, 0);
				//配合核心返回60,100域写死
				setbit(&siso,60,(unsigned char *)"000005000300000000000111000000", 30);
				setbit(&siso,100,(unsigned char *)"44200000", 8);
				memset(macblock, 0, sizeof(macblock));
				memset(posmac, 0, sizeof(posmac));
				
				s = GetPospMacBlk(&siso, macblock);
				//dcs_debug( macblock, s, "GetPospMacBlk Rtn len=%d.", s );
				
				s = GetPospMAC( macblock, s, "1111111111111111", posmac);
				//dcs_debug( posmac, 8, "posmac :");
				
				setbit(&siso, 128, (unsigned char *)posmac, 8);
				
				s = sndBackToPoswu(siso, iFid, mer_info_t);
				if(s < 0 )
				{
					dcs_log(0,0,"posUnpack error!");
				}
				break;
			}
			//消费撤销冲正
			if(memcmp(tmpbuf, "200000", 6) == 0)
			{
				setbit(&siso,0,(unsigned char *)"0430", 4);
				setbit(&siso,12,(unsigned char *)databuf+4, 6);
				setbit(&siso,13,(unsigned char *)databuf, 4);
				setbit(&siso,15,(unsigned char *)databuf, 4);
				
//				memset(tmpbuf, 0, sizeof(tmpbuf));
//				getbit(&siso,11,(unsigned char *)tmpbuf);
//				memcpy(tmpbuf+6, databuf+4,6);
//				setbit(&siso,37,(unsigned char *)tmpbuf, 12);
				
				
				setbit(&siso,32,(unsigned char *)"40002900", 8);
				setbit(&siso,39,(unsigned char *)"00", 2);
				
				setbit(&siso,44,(unsigned char *)"03012900   04032900   ", 22);
				
				setbit(&siso,22,(unsigned char *)NULL, 0);
				setbit(&siso,61,(unsigned char *)NULL, 0);
				//配合核心返回60,100域写死
				setbit(&siso,60,(unsigned char *)"000005000300000000000111000000", 30);
				setbit(&siso,100,(unsigned char *)"44200000", 8);
				memset(macblock, 0, sizeof(macblock));
				memset(posmac, 0, sizeof(posmac)); 
				
				s = GetPospMacBlk(&siso, macblock);
				//dcs_debug( macblock, s, "GetPospMacBlk Rtn len=%d.", s );
				
				s = GetPospMAC( macblock, s, "1111111111111111", posmac);
				//dcs_debug( posmac, 8, "posmac :");
				
				setbit(&siso, 128, (unsigned char *)posmac, 8);
				
				s = sndBackToPoswu(siso, iFid, mer_info_t);
				if(s < 0 )
				{
					dcs_log(0,0,"消费撤销冲正s=[%d]",s);
					dcs_log(0,0,"posUnpack error!");
				}
				break;
			}
			//预授权冲正
			if(memcmp(tmpbuf, "030000", 6) == 0)
			{
				setbit(&siso,0,(unsigned char *)"0430", 4);
				setbit(&siso,12,(unsigned char *)databuf+4, 6);
				setbit(&siso,13,(unsigned char *)databuf, 4);
				setbit(&siso,15,(unsigned char *)databuf, 4);
				
//				memset(tmpbuf, 0, sizeof(tmpbuf));
//				getbit(&siso,11,(unsigned char *)tmpbuf);
//				memcpy(tmpbuf+6, databuf+4,6);
//				setbit(&siso,37,(unsigned char *)tmpbuf, 12);
				
				
//				setbit(&siso,32,(unsigned char *)"48720000", 8);
//				setbit(&siso,33,(unsigned char *)"03010001", 8);

				setbit(&siso,39,(unsigned char *)"00", 2);
	
				setbit(&siso,22,(unsigned char *)NULL, 0);
				setbit(&siso,61,(unsigned char *)NULL, 0);

				setbit(&siso,43,(unsigned char *)NULL, 0);
				setbit(&siso,100,(unsigned char *)"48720000", 8);
				//配合核心返回60,100域写死
				setbit(&siso,60,(unsigned char *)"000005000300000000000111000000", 30);
				setbit(&siso,100,(unsigned char *)"44200000", 8);
				memset(macblock, 0, sizeof(macblock));
				memset(posmac, 0, sizeof(posmac)); 
				
				s = GetPospMacBlk(&siso, macblock);
				//dcs_debug( macblock, s, "GetPospMacBlk Rtn len=%d.", s );
				
				s = GetPospMAC( macblock, s, "1111111111111111", posmac);
				//dcs_debug( posmac, 8, "posmac :");
				
				setbit(&siso, 128, (unsigned char *)posmac, 8);
				
				s = sndBackToPoswu(siso, iFid, mer_info_t);
				if(s < 0 )
				{
					dcs_log(0,0,"posUnpack error!");
				}
				break;
			}
		break;
	}  
}

//解包核心,银联CUPS的报文形式
 int posUnpackwu(char *srcBuf, ISO_data *iso, MER_INFO_T *mer_info_t, int iLen)
 {
 	char tmp1[23], tmp2[23];
 	int tt = 0,headLen = 0;

 	if(iLen < 50)
 	{
 		dcs_log(0, 0, "posUnpack error");
 		return -1;
 	}

 	memset(tmp1, 0, sizeof(tmp1));
 	memset(tmp2, 0, sizeof(tmp2));
 	iLen -= 6;
	headLen += 6;
	srcBuf += 6;
 	//去掉目的ID和原ID
 	memcpy(tmp1, srcBuf, 22);
 	dcs_log(0, 0, "TPDU：%s", tmp1);
 	memcpy(mer_info_t->mgr_tpdu, tmp1+11, 11);
 	memcpy(mer_info_t->mgr_tpdu+11, tmp1, 11);

 	iLen -= 40;
 	headLen += 40;
 	srcBuf += 40;

 	memcpy(iso8583_conf, iso8583conf, sizeof(iso8583conf));
 	iso8583=&iso8583_conf[0];
 	SetIsoHeardFlag(1);
 	SetIsoFieldLengthFlag(1);


 	if(str_to_iso((unsigned char *)srcBuf, iso, iLen)<0)
 	{
 		dcs_log(0, 0, "str_to_iso error ");
 		return -1;
 	}

 	return headLen;
  }


  int sndBackToPoswu(ISO_data iso, int fMid, MER_INFO_T mer_info_t)
 {
 	char buffer[1024], sendBuff[2048], tmp1[11], tmpbuf[37];
 	int s;

 	memset(buffer, 0, sizeof(buffer));
 	memset(sendBuff, 0, sizeof(sendBuff));
 	memset(tmp1, 0, sizeof(tmp1));
 	memset(tmpbuf, 0, sizeof(tmpbuf));

 	memcpy(iso8583_conf, iso8583conf, sizeof(iso8583conf));
 	iso8583=&iso8583_conf[0];
 	SetIsoHeardFlag(1);
 	SetIsoFieldLengthFlag(1);
 	setbit(&iso,63,(unsigned char *)NULL, 0);
 	setbit(&iso,64,(unsigned char *)NULL, 0);
 	s = iso_to_str((unsigned char *)buffer, &iso, 1);
 	if( s < 0 )
 	{
 		dcs_log(0, 0, "iso_to_str error ");
 		return -2;
 	}
 	memset(sendBuff, '0',46);
 	sendBuff[0] = '\x2E';
 	sendBuff[1] = '\x02';
 	sprintf(sendBuff +2, "%04d", s+46);
 	memcpy(sendBuff +4 ,mer_info_t.mgr_tpdu, 22);
 	memcpy(sendBuff +46 ,buffer, s);
   	s = s+46;

 	dcs_debug(sendBuff, s, "POS数据返回核心, foldId =%d, len =%d", fMid, s);

 	s = fold_write(fMid, -1, sendBuff, s);
 	if(s < 0)
 	{
       	dcs_log(0, 0, "fold_write() failed:%s", ise_strerror(errno));
       	return -3;
     }
 	return 0;
  }

