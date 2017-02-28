#include "ibdcs.h"
#include "trans.h"
#include "pub_function.h"

/* 
	模块说明：
		此模块主要处理POS管理平台发起密钥文件生成的接口服务
	2015.1.13
 */
void glptProc(char *srcBuf, int iFid, int iMid, int iLen)
{

	#ifdef __LOG__
		dcs_debug(srcBuf, iLen, "MPOS Proc, iFid=%d, iMid=%d, iLen=%d", iFid, iMid, iLen);
	#else
		dcs_log(0, 0, "MPOS Proc, iFid=%d, iMid=%d, iLen=%d", iFid, iMid, iLen);
	#endif
	
	//变量定义
	int rtn = 0, s = 0, code = -1;
	
	int datalen = 0, i = 0, count = 0;
	
	char tmpbuf[1024];
	
	char databuf[PUB_MAX_SLEN + 1];
	
	//KSN结构体
	KSN_INFO ksn_info;

	//数据长度
	int len=0; 
	
	//KSN编号
	char ksn_bcd[10+1];
	char initkey[16+1];
	char sendbuffer[2048];

	char tpdu[5+1];
	char num[3+1];
	char ksnbuf[15+1]; 
	char decinitkey[32+1];
	char checkvalue[8+1];
	
	//初始化变量	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(databuf, 0, sizeof(databuf));
	memset(ksn_bcd, 0, sizeof(ksn_bcd));
	memset(initkey, 0, sizeof(initkey));
	memset(&ksn_info, 0x00, sizeof(KSN_INFO));
	memset(sendbuffer, 0, sizeof(sendbuffer));
	memset(tpdu, 0, sizeof(tpdu));
	memset(num, 0, sizeof(num));
	memset(ksnbuf, 0, sizeof(ksnbuf));
	memset(decinitkey, 0, sizeof(decinitkey));
	memset(checkvalue, 0, sizeof(checkvalue));
	
	datalen = iLen;
	memcpy(databuf, srcBuf, datalen);
	
	memcpy(tpdu, databuf, 5 );
	len= len + 5;
	
	if(memcmp(databuf+len, "AAA", 3)==0)
	{
		dcs_log(0, 0, "解析管理平台MPOS生成密钥报文");
		len= len + 3;
		
		//渠道号
		memset(tmpbuf, 0x00, sizeof(tmpbuf));
		memcpy(tmpbuf, databuf+len, 4);
		ksn_info.channel_id= atoi(tmpbuf);
		len= len + 4;
		dcs_log(0, 0, "渠道id：%04d", ksn_info.channel_id);
	
		//索引1
		memset(tmpbuf, 0x00, sizeof(tmpbuf));
		memcpy(tmpbuf, databuf+len, 3);
		ksn_info.transfer_key_index= atoi(tmpbuf);
		len= len + 3;
		dcs_log(0, 0, "索引1：%03d", ksn_info.transfer_key_index);
		
		//索引2
		memset(tmpbuf, 0x00, sizeof(tmpbuf));
		memcpy(tmpbuf, databuf+len, 3);
		ksn_info.bdk_index= atoi(tmpbuf);
		len= len + 3;
		dcs_log(0, 0, "索引2：%03d", ksn_info.bdk_index);
		
		//索引3
		memset(tmpbuf, 0x00, sizeof(tmpbuf));
		memcpy(tmpbuf, databuf+len, 3);
		ksn_info.bdk_xor_index= atoi(tmpbuf);
		len= len + 3;
		dcs_log(0, 0, "索引3：%03d", ksn_info.bdk_xor_index);
		
		//KSN个数
		memcpy(num, databuf+len, 3);
		len= len + 3;
		dcs_log(0, 0, "KSN个数：%s", num);
		
		int ksnnum= atoi(num);
		
		//判断管理平台上送交易报文是否正常
		if(datalen != 24 + ksnnum*15)
		//if(datalen != 20 + ksnnum*15)
		{
			code= -1;
			dcs_log(0 , 0, "报文长度错误");
			WriteGLPT(tpdu, iFid, sendbuffer, code);
			return;
		}
		
		memset(sendbuffer, 0x00, sizeof(sendbuffer));
		for( i=0; i<ksnnum; i++)
		{
			code = -1;
			memset(ksnbuf, 0x00, sizeof(ksnbuf));
			memcpy(ksnbuf, databuf+len, 15);
			len= len + 15;
			
			char ksntmp1[10+1];
			char ksntmp2[10+1];
			char ksn[5+1];
			
			memset(ksntmp1, 0x00, sizeof(ksntmp1));
			memset(ksntmp2, 0x00, sizeof(ksntmp2));
			memset(ksn, 0x00, sizeof(ksn));
			
			memcpy(ksn, ksnbuf, 5);
			bcd_to_asc((unsigned char *)ksntmp1, ksn, 10, 0);
			dcs_log(0, 0, "ksntmp1:%s", ksntmp1);
			
			memcpy(ksntmp2, ksnbuf+5, 10);
			sprintf(ksn_info.ksn, "%s%s", ksntmp1, ksntmp2);
			dcs_log(0, 0, "ksn_info.ksn：%s", ksn_info.ksn);

			//判断此KSN是否ksn存在，存在直接取数据库数据；不存在，则参与计算
			s = Getksninfo(&ksn_info, ksnbuf);
			if(s<0)
			{			
				//获取initkey
				asc_to_bcd((unsigned char *)ksn_bcd, (unsigned char*)ksn_info.ksn, 20, 0);
				
				dcs_debug(ksn_bcd, 8,"10个字节的KSN编号ksn_bcd:");
				
				while(code == -1 && count < 5)
				{
					s = GetIPEK(ksn_bcd, ksn_info.bdk_index, ksn_info.bdk_xor_index, initkey);
					if(s<0)
					{
						count++;
						dcs_log(0, 0, "GetIPEK计算失败");
						continue;
					}
					
					bcd_to_asc(ksn_info.initkey, initkey, 32,0);
					dcs_log(0, 0, "密钥明文值=[%s]", ksn_info.initkey);
					
					//传输密钥加密initkey得到decinitkey
					s = getdeckey(initkey, ksn_info.transfer_key_index, tmpbuf);
					if(s<0)
					{
						count++;
						dcs_log(0, 0, "getdeckey计算失败");
						continue;
					}
					bcd_to_asc(decinitkey, tmpbuf, 32,0);
					dcs_log(0, 0, "加密后密钥值=[%s]", decinitkey);
					
					//获取密钥校验值checkvalue
					s = getcheckvalue(initkey, tmpbuf);
					if(s<0)
					{
						count++;
						dcs_log(0 , 0, "getcheckvalue计算失败");
						continue;
					}
					bcd_to_asc(checkvalue, tmpbuf, 8,0);
					dcs_log(0, 0, "密钥校验值=[%s]", checkvalue);
					code= 0;
				}
				
				if(code == -1)
				{
					dcs_log(0 , 0, "密钥失败");
					WriteGLPT(tpdu, iFid, sendbuffer, code);
					return;
				}
					
				//组装密钥文件值  15位KSN + 32位加密后密钥值 + 8位密钥校验值
				memset(ksn_info.final_info, 0x00, sizeof(ksn_info.final_info));
				sprintf(ksn_info.final_info, "%s,%s,%s|", ksnbuf, decinitkey, checkvalue);
				dcs_log(0, 0, "密钥文件值:%s", ksn_info.final_info);
				
				sprintf(sendbuffer, "%s%s", sendbuffer, ksn_info.final_info);
	
				//保存 KSN  明文密钥  密钥文件值 到 ksn_info 数据表
				s = Savefinalinfo(ksn_info, ksnbuf);
				if(s<0)
				{
					code= -1;
					dcs_log(0, 0, "Saveinitkey失败");
					WriteGLPT(tpdu, iFid, sendbuffer, code);
					return;
				}
				dcs_log(0, 0, "第[%d]次计算成功,共计[%d]次", i+1, ksnnum);	
			}
			else
			{
				dcs_log(0, 0, "密钥文件值已存在，忽略计算、存储过程");
				dcs_log(0, 0, "第[%d]次计算成功,共计[%d]次", i+1, ksnnum);	
				sprintf(sendbuffer, "%s%s", sendbuffer, ksn_info.final_info);
			}
		}
		code= 0;
	}
	else 
	{
		code= -1;
		dcs_log(0, 0, "交易码错误");
		WriteGLPT(tpdu, iFid, sendbuffer, code);
		return;
	}

	//发送报文
	rtn=WriteGLPT(tpdu, iFid, sendbuffer, code);
	if(rtn == -1)
	{
		dcs_log(0,0,"发送信息给管理平台失败");
		return;
	}
	dcs_log(0,0,"发送信息给管理平台成功");
	return;	
}


/*
组返回管理平台的报文
code = 0 返回成功处理
code !=0 返回失败应答
*/
int WriteGLPT(char *tpdu, int trnsndpid, char *buffer, int code)
{
	int gs_comfid = -1, s=0;
	char buffer_send[2048];
	
	memset(buffer_send, 0, sizeof(buffer_send));

	s= strlen(buffer);
    gs_comfid = trnsndpid;
	
	#ifdef __TEST__
    if(fold_get_maxmsg(gs_comfid) < 100)
		fold_set_maxmsg(gs_comfid, 500) ;
 	#else
	if(fold_get_maxmsg(gs_comfid) < 2)
		fold_set_maxmsg(gs_comfid, 20) ;
    #endif

	memcpy(buffer_send, tpdu, 1);
	memcpy(buffer_send+1, tpdu+3, 2);
	memcpy(buffer_send+3, tpdu+1, 2);
	
	if(code == 0)
	{
		memcpy(buffer_send+5, buffer, s);
		#ifdef __TEST__
			dcs_debug(buffer_send, s+5, "data_buf len=%d, foldId=%d", s+5, gs_comfid);
		#endif
		fold_write(gs_comfid, -1, buffer_send, s+5);
	}
	else 
	{
		memcpy(buffer_send+5, "0A", 2);
		#ifdef __TEST__
			dcs_debug(buffer_send, 7, "data_buf len=%d, foldId=%d", 7, gs_comfid);
		#endif
		fold_write(gs_comfid, -1, buffer_send, 7);
	}
	return 0;
}


