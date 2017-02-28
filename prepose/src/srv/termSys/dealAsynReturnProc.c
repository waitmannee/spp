#include "ibdcs.h"
#include "trans.h"
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

/*
	代付异步处理通知处理模块
*/
void dealAsynReturnProc(char *srcBuf, int iFid, int iMid, int iLen)
{
	TRANSFER_INFO transfer_info;
	ISO_data siso;
	PEP_JNLS pep_jnls; 
	EWP_INFO ewp_info; 
	//系统自带的时间结构体
	struct  tm *tm;   
	time_t  t;
	//当前日期
	char currentDate[14+1];
	char databuf[PUB_MAX_SLEN + 1], rcvbuf[128], tm_tpdu[6];
	int datalen = 0, rtn = 0;
	
	memset(&transfer_info, 0x00, sizeof(TRANSFER_INFO));
	memset(&siso, 0x00, sizeof(ISO_data));
	memset(&pep_jnls, 0x00, sizeof(PEP_JNLS));
	memset(&ewp_info, 0x00, sizeof(EWP_INFO));
	memset(currentDate, 0x00, sizeof(currentDate));
	memset(databuf, 0x00, sizeof(databuf));
	memset(rcvbuf, 0x00, sizeof(rcvbuf));
	memset(tm_tpdu, 0x00, sizeof(tm_tpdu));
	
	dcs_log(0, 0, "异步通知结果报文, iFid=%d, iMid=%d, iLen=%d\n%s", iFid, iMid, iLen-5,srcBuf+5);

	time(&t);
    tm = localtime(&t);
    
    sprintf(currentDate, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
    sprintf(currentDate+8, "%02d%02d%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);

	memcpy(tm_tpdu, srcBuf, 5);
	
	datalen = iLen - 5;
	memcpy(databuf, srcBuf+5, datalen);

	//解析异步通知结果报文
	rtn = analysisAsynNotice(databuf, datalen, &transfer_info);
	if(rtn < 0 )
	{
		return;
	}
	
	//解析完数据之后需要更新转账汇款表相应的字段
	rtn = updatePayInfo(&transfer_info);
	if(rtn<0)
	{
		dcs_log(0, 0, "更新转账汇款信息失败");
		return;
	}
	//查询原笔交易信息查询pep_jnls数据表
	rtn = getOrgSaleInfo(transfer_info, &pep_jnls);
	if(rtn<0)
	{
		dcs_log(0, 0, "获取原笔交易信息失败");
		return;
	}
	//若是次日到账，则无需处理。
	if(memcmp(transfer_info.transfer_type, "1", 1)==0)
	{
		dcs_log(0, 0, "次日到账，只更新数据库记录即可");
	}
	else if(memcmp(transfer_info.transfer_type, "0", 1)==0)
	{
		//更新完之后需要判断，若是实时代付，代付成功直接写回EWP；
		if(memcmp(transfer_info.transStatus, "S", 1)==0)
		{
			dcs_log(0, 0, "实时到账，代付成功，直接写返回EWP");
			memcpy(pep_jnls.aprespn, "00", 2);
			//代付成功需要发送短信
			if(memcmp(pep_jnls.modeflg, "1", 1) == 0)
			{
				dcs_log(0, 0, "send msg info");
				memcpy(pep_jnls.syseqno, transfer_info.tseq, 12);/*短信参考号为代付流水号*/
				strcpy(pep_jnls.filed28, transfer_info.fee);
				strcpy(pep_jnls.pepdate, transfer_info.sysDate);
				strcpy(pep_jnls.peptime, transfer_info.sysTime);
				SendMsg(pep_jnls);
			}
		}
		//若是代付失败，发起冲正，返回EWP
		//更新完之后需要判断，若是实时代付，代付成功直接写回EWP；
		else if(memcmp(transfer_info.transStatus, "F", 1)==0)
		{
			dcs_log(0, 0, "实时到账，代付失败，先发起冲正，直接写返回EWP");
			sendReversal(pep_jnls);
			memcpy(pep_jnls.aprespn, "0F", 2);
		}
		else
		{
			dcs_log(0, 0, "不合法的应答");
			return;
		}
		memcpy(pep_jnls.transfer_type, transfer_info.transfer_type, strlen(transfer_info.transfer_type));
		
		//先移除超时控制
		rtn = delete_timeout_control(transfer_info.orderId, rcvbuf, sizeof(rcvbuf)); //测试超时控制超时发送的功能
		if(rtn ==-1)
		{
			return ;
		}
		if(memcmp(rcvbuf+2, "response=00", 11)==0)
		{
			if(memcmp(rcvbuf+14, "status=2", 8)==0)//2 表示 超时控制已超时返回
			{
				dcs_log(0, 0, "移除成功,已超时返回,此处不再返回");
				return ;
			}
			else if(memcmp(rcvbuf+14, "status=1", 8)==0||memcmp(rcvbuf+14, "status=0", 8)==0)//0 表示 没找到key对应的超时控制；1表示 移除成功
			{
				//更新返回终端的应答码
				updateResponseCode(transfer_info);
				dcs_log(0, 0, "移除超时控制成功");
			}
		}
		rtn = RetToEWP(siso, ewp_info, pep_jnls, 1);//返回EWP
		if(rtn ==-1)
		{
			dcs_log(0, 0, "返回EWP失败");
			return ;
		}
	}
	//返回异步通知结果
	dcs_log(0, 0, "返回异步通知结果");
	memset(databuf, 0x00, sizeof(databuf));
	memset(rcvbuf, 0x00, sizeof(rcvbuf));
	memcpy(databuf, tm_tpdu, 1);
	memcpy(databuf+1, tm_tpdu+3, 2);
	memcpy(databuf+3, tm_tpdu+1, 2);
	
	sprintf(rcvbuf, "RECV_RYT_ORD_ID_%s", transfer_info.orderId);
	datalen = strlen(rcvbuf);
	memcpy(databuf+5, rcvbuf, datalen);
	rtn = fold_write(iFid, iMid, databuf, datalen+5);
	return;	
}

/*根据key获取到value的值*/
int GetValueByKey(char *source, char *key, char *value, int value_len)
{
	char *delims = "&";
	char *result = NULL;
	char *saveptr= NULL;
	char *saveptr1= NULL;
	char *sub_delims = "=";
	char *temp_key =NULL;
	char *temp_value =NULL;
	int temp_len = 0;
	char buf[2048];
	int flag = 0;
	memset(buf, 0x00, sizeof(buf));
	strcpy(buf, source);
	result = strtok_r(buf, delims, &saveptr);
	while(result != NULL) 
	{
		temp_key = strtok_r(result, sub_delims, &saveptr1);
		if(temp_key == NULL)
        {
			result = strtok_r(NULL, delims, &saveptr);
            continue;
        }
		temp_value = strtok_r(NULL, sub_delims, &saveptr1);
		if(temp_value == NULL)
        {
			result = strtok_r(NULL, delims, &saveptr);
            continue;
        }
		if(strlen(key)==strlen(temp_key) && memcmp(temp_key, key, strlen(temp_key))==0)
		{
			temp_len =  strlen(temp_value);
			if(temp_len > value_len)
				temp_len = value_len;
			if(value_len == 1)
				*value = *temp_value;
			else
			{
				memcpy(value, temp_value, temp_len);
				value[temp_len] =0;
			}
			flag =1;
			break;
		}
		result = strtok_r(NULL, delims, &saveptr);
	} 
	if(flag == 0)
		return -1;
	return 0;	
}

/*解析从ryf那里接收到的代付的异步通知结果的报文key=value的方式 存放到结构体TRANSFER_INFO中*/
int analysisAsynNotice(char *rcvbuf, int rcvlen, TRANSFER_INFO *transfer_info)
{
	//accountId=""&orderId=""&&transAmt=""&transType=""&sysDate=""&sysTime=""&transStatus=""&
	//tseq=""&errorMsg=""&merPriv=""&chkValue=""
	int rtn = 0, len = 0;
	char MD5value[120+1];/*存放计算出来的签名值MD5*/
	char MD5buf[60];/*存放计算MD5字符串*/
	char MD5key[16+1];/*存放MD5 key测试 EAAoGBAMkYfh7MeT*/
	char buffer[1024];
	char local_ip[16];/*存放本地IP地址*/
	
	memset(MD5value, 0x00, sizeof(MD5value)); 
	memset(MD5buf, 0x00, sizeof(MD5buf)); 
	memset(MD5key, 0x00, sizeof(MD5key)); 
	memset(buffer, 0x00, sizeof(buffer));
	memset(local_ip, 0x00, sizeof(local_ip));	
	//获取本机IP地址
	getIP(local_ip);
	//选择MD5key
#ifdef __TEST__
	memcpy(MD5key, TEST_MD5_KEY, 16);
#else
	if(memcmp(local_ip, "192.168.30", 10)==0)/*UAT环境使用测试KEY*/
		memcpy(MD5key, TEST_MD5_KEY, 16);
	else
		memcpy(MD5key, PROT_MD5_KEY, 16);
#endif	
	rtn = URLDecode(rcvbuf, rcvlen, buffer, sizeof(buffer));
	if(rtn == -1)
	{
		dcs_log(0, 0, "解码失败");
		memcpy(transfer_info->retcode, "E1", 2);
		return -1;
	}
	dcs_log(0, 0, "解码后的报文%s",buffer);
	//解析accountId数据
	rtn = GetValueByKey(buffer, "accountId", transfer_info->accountId, sizeof(transfer_info->accountId));
	if(rtn == -1)
	{
		dcs_log(0, 0, "解析accountId失败");
		memcpy(transfer_info->retcode, "E1", 2);
		return -1;
	}
	//解析orderId数据
	rtn = GetValueByKey(buffer, "orderId", transfer_info->orderId, sizeof(transfer_info->orderId));
	if(rtn == -1)
	{
		dcs_log(0, 0, "解析orderId失败");
		memcpy(transfer_info->retcode, "E1", 2);
		return -1;
	}
	//解析transAmt数据
	rtn = GetValueByKey(buffer, "transAmt", transfer_info->transAmt, sizeof(transfer_info->transAmt));
	if(rtn == -1)
	{
		dcs_log(0, 0, "解析transAmt失败");
		memcpy(transfer_info->retcode, "E1", 2);
		return -1;
	}
	//解析transType数据
	rtn = GetValueByKey(buffer, "transType", transfer_info->transType, sizeof(transfer_info->transType));
	if(rtn == -1)
	{
		dcs_log(0, 0, "解析transType失败");
		memcpy(transfer_info->retcode, "E1", 2);
		return -1;
	}
	//解析sysDate数据
	rtn = GetValueByKey(buffer, "sysDate", transfer_info->sysDate, sizeof(transfer_info->sysDate));
	if(rtn == -1)
	{
		dcs_log(0, 0, "解析sysDate失败");
		memcpy(transfer_info->retcode, "E1", 2);
		return -1;
	}
	//解析sysTime数据
	rtn = GetValueByKey(buffer, "sysTime", transfer_info->sysTime, sizeof(transfer_info->sysTime));
	if(rtn == -1)
	{
		dcs_log(0, 0, "解析sysTime失败");
		memcpy(transfer_info->retcode, "E1", 2);
		return -1;
	}
	//解析transStatus数据
	rtn = GetValueByKey(buffer, "transStatus", transfer_info->transStatus, sizeof(transfer_info->transStatus));
	if(rtn == -1)
	{
		dcs_log(0, 0, "解析transStatus失败");
		memcpy(transfer_info->retcode, "E1", 2);
		return -1;
	}
	//解析tseq数据
	rtn = GetValueByKey(buffer, "tseq", transfer_info->tseq, sizeof(transfer_info->tseq));
	if(rtn == -1)
	{
		dcs_log(0, 0, "解析tseq失败");
		memcpy(transfer_info->retcode, "E1", 2);
		return -1;
	}
	if(memcmp(transfer_info->transStatus, "F", 1)==0)
	{
		//解析errorMsg数据
		rtn = GetValueByKey(buffer, "errorMsg", transfer_info->errorMsg, sizeof(transfer_info->errorMsg));
		if(rtn == -1)
		{
			dcs_log(0, 0, "解析errorMsg失败");
			memcpy(transfer_info->retcode, "E1", 2);
			return -1;
		}
	}
	//解析merPriv数据
	rtn = GetValueByKey(buffer, "merPriv", transfer_info->merPriv, sizeof(transfer_info->merPriv));
	if(rtn == -1)
	{
		dcs_log(0, 0, "解析merPriv失败");
		memcpy(transfer_info->retcode, "E1", 2);
		return -1;
	}
	//解析chkValue数据
	rtn = GetValueByKey(buffer, "chkValue", transfer_info->chkValue, sizeof(transfer_info->chkValue));
	if(rtn == -1)
	{
		dcs_log(0, 0, "解析chkValue失败");
		memcpy(transfer_info->retcode, "E1", 2);
		return -1;
	}
	//参与数字签名的字符串：orderId + transAmt + tseq + transStatus + MD5key
	sprintf(MD5buf, "%s%s%s%s%s", transfer_info->orderId, transfer_info->transAmt, transfer_info->tseq, transfer_info->transStatus, MD5key);
	len  = strlen(MD5buf);
	rtn = getUpMD5(MD5buf, len, MD5value);
	if(rtn == -1)
	{
		dcs_log(0, 0, "计算签名值失败");
		memcpy(transfer_info->retcode, "E2", 2);
		return -1;
	}
	if(memcmp(MD5value, transfer_info->chkValue, strlen(transfer_info->chkValue))!=0)
	{	
		dcs_log(0, 0, "chkValue=[%s], MD5value=[%s]签名值校验失败", transfer_info->chkValue, MD5value);
		memcpy(transfer_info->retcode, "E0", 2);
		return -1;
	}
	if(memcmp(transfer_info->transStatus, "S", 1)==0)
	{
		dcs_log(0, 0, "代付成功");
		memcpy(transfer_info->pay_respn, "00", 2);
		memcpy(transfer_info->pay_respn_desc, "代付成功", strlen("代付成功"));
		memcpy(transfer_info->response_code, "00", 2);
	}
	if(memcmp(transfer_info->transStatus, "F", 1)==0)
	{
		dcs_log(0, 0, "代付失败");
		memcpy(transfer_info->pay_respn, "0F", 2);
		memcpy(transfer_info->pay_respn_desc, transfer_info->errorMsg, strlen(transfer_info->errorMsg));
		memcpy(transfer_info->response_code, "0F", 2);
	}
	memcpy(transfer_info->retcode, "00", 2);
	return 0;
}


