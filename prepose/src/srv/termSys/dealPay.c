#include "trans.h"
#include "ibdcs.h"
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include "syn_http.h"


extern struct ISO_8583 *iso8583;
extern struct ISO_8583 iso8583_conf[128];
extern struct ISO_8583 iso8583conf[128];


/*
函数功能组代付请求报文
返回值：组包失败返回-1
	组包成功返回0
*/
int getPacket(PEP_JNLS pep_jnls, TRANSFER_INFO transfer_info, char *sendbuf)
{
	/*
		参与数字签名的字符串:version + orderId + transAmt +  transType + rcvAcno
		version="10"&accountId=商户号&orderId=订单号&transAmt=订单金额transType="C1"&dfType="A"
		&cardFlag=0&rcvAcno=卡号&rcvAcname=持卡人姓名&bankNo=卡号联行号&purpose=用途&merPriv=""
		&bgRetUrl=192.168.20.176:9915&dataSource=5&chkValue=商家签名
	*/
	char md5Buf[128];/*参与计算MD5的串*/
	char MD5key[16+1];/*存放MD5key EAAoGBAMkYfh7MeT*/
	char MD5Value[120+1];/*存放计算出的MD5串*/
	int len = 0, rtn = 0;
	char tmpbuf[1024];
	char local_ip[16];/*存放本地IP地址*/
	
	memset(md5Buf, 0x00, sizeof(md5Buf));
	memset(MD5key, 0x00, sizeof(MD5key));
	memset(MD5Value, 0x00, sizeof(MD5Value));
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	memset(local_ip, 0x00, sizeof(local_ip));
	//获取本机IP地址
	rtn = getIP(local_ip);
	if(rtn < 0)
	{
		dcs_log(0, 0, "获取IP地址失败");
		return -1;
	}
	
	//选择MD5key
	#ifdef __TEST__
		memcpy(MD5key, TEST_MD5_KEY, 16);
	#else
		if(memcmp(local_ip, "192.168.30", 10)==0)/*UAT环境使用测试KEY*/
			memcpy(MD5key, TEST_MD5_KEY, 16);
		else
			memcpy(MD5key, PROT_MD5_KEY, 16);
	#endif
	
	sprintf(md5Buf, "10%s%.02fC1%s%s", transfer_info.orderId, atof(transfer_info.transAmt)/100, pep_jnls.intcdno, MD5key);
	len = strlen(md5Buf);
	  
	rtn = getUpMD5(md5Buf, len, MD5Value);
	if(rtn < 0)
		return -1;
	
	sprintf(tmpbuf, "version=10&accountId=%s&orderId=%s&transAmt=%.02f&transType=C1&dfType=A&cardFlag=0&rcvAcno=%s&rcvAcname=%s&bankNo=%s&purpose=%s&merPriv= &bgRetUrl=http://%s:9915&dataSource=5&chkValue=%s", transfer_info.accountId, transfer_info.orderId, \
	atof(transfer_info.transAmt)/100, pep_jnls.intcdno, pep_jnls.intcdname, pep_jnls.intcdbank, "转账汇款代付", \
	local_ip, MD5Value);
	
	memcpy(sendbuf, tmpbuf, strlen(tmpbuf));
	return 0;
}


int getResPonse(char *srcbuf, int len, TRANSFER_INFO *transfer_info)
{
	xmlDocPtr doc;           //定义解析文件指针
    xmlNodePtr curNode;      //定义结点指针(你需要他为了在各个结点间移动)
	xmlNodePtr subcurNode;
    xmlChar *szKey;          //临时字符串变量
   
    doc = xmlParseMemory((const char *) srcbuf, len);
    if(NULL == doc)
    { 
       dcs_log(0, 0, "Document not parsed successfully. \n");    
       return -1;
    }
    curNode = xmlDocGetRootElement(doc); //确定文件根元素
    /*检查确认当前文件中包含内容*/
    if (NULL == curNode)
    {
       dcs_log(0, 0, "empty document\n");
       xmlFreeDoc(doc);
       return -1;
    }
    /*在这个例子中，我们需要确认文件是正确的类型。"res"是在这个示例中使用文件的根类型。*/
    if(xmlStrcmp(curNode->name, BAD_CAST "res"))
    {
       dcs_log(0, 0, "document of the wrong type, root node != res");
       xmlFreeDoc(doc);
       return -1;
    }
    curNode = curNode->xmlChildrenNode;
    xmlNodePtr propNodePtr = curNode;
    while(curNode != NULL)
    {
       //取出status节点中的内容
		if(!xmlStrcmp(curNode->name, (const xmlChar *)"status"))
		{
			//解析status节点，获取每个子域的内容
			subcurNode = curNode->xmlChildrenNode;
			while (subcurNode != NULL)
			{
				//取出value节点中的内容
				if(!xmlStrcmp(subcurNode->name, (const xmlChar *)"value"))
				{
					szKey = xmlNodeGetContent(subcurNode);
					memcpy(transfer_info->payResultcode, szKey, strlen(szKey));
					xmlFree(szKey); 
				}
				else if(!xmlStrcmp(subcurNode->name, (const xmlChar *)"msg"))
				{
					szKey = xmlNodeGetContent(subcurNode);
					memcpy(transfer_info->payResultcode_desc, szKey, strlen(szKey));
					xmlFree(szKey); 
				}
				subcurNode = subcurNode->next;
			}
		}
		if(memcmp(transfer_info->payResultcode+8, "00", 2)==0)
		{
		   if(!xmlStrcmp(curNode->name, (const xmlChar *)"transResult"))
		   {
			   //解析transResult节点，获取每个子域的内容
				subcurNode = curNode->xmlChildrenNode;
				while (subcurNode != NULL)
				{
					//取出accountId节点中的内容
					if(!xmlStrcmp(subcurNode->name, (const xmlChar *)"accountId"))
					{
						szKey = xmlNodeGetContent(subcurNode);
						memcpy(transfer_info->accountId, szKey, strlen(szKey));
						xmlFree(szKey); 
					}
					else if(!xmlStrcmp(subcurNode->name, (const xmlChar *)"orderId"))
					{
						szKey = xmlNodeGetContent(subcurNode);
						memcpy(transfer_info->orderId, szKey, strlen(szKey));
						xmlFree(szKey); 
					}
					else if(!xmlStrcmp(subcurNode->name, (const xmlChar *)"transAmt"))
					{
						szKey = xmlNodeGetContent(subcurNode);
						memcpy(transfer_info->transAmt, szKey, strlen(szKey));
						xmlFree(szKey); 
					}
					else if(!xmlStrcmp(subcurNode->name, (const xmlChar *)"transType"))
					{
						szKey = xmlNodeGetContent(subcurNode);
						memcpy(transfer_info->transType, szKey, strlen(szKey));
						xmlFree(szKey); 
					}
					else if(!xmlStrcmp(subcurNode->name, (const xmlChar *)"tseq"))
					{
						szKey = xmlNodeGetContent(subcurNode);
						memcpy(transfer_info->tseq, szKey, strlen(szKey));
						xmlFree(szKey); 
					}
					else if(!xmlStrcmp(subcurNode->name, (const xmlChar *)"transStatus"))
					{
						szKey = xmlNodeGetContent(subcurNode);
						memcpy(transfer_info->transStatus, szKey, strlen(szKey));
						xmlFree(szKey); 
					}
					else if(!xmlStrcmp(subcurNode->name, (const xmlChar *)"errorMsg"))
					{
						szKey = xmlNodeGetContent(subcurNode);
						memcpy(transfer_info->errorMsg, szKey, strlen(szKey));
						xmlFree(szKey); 
					}
					else if(!xmlStrcmp(subcurNode->name, (const xmlChar *)"sysDate"))
					{
						szKey = xmlNodeGetContent(subcurNode);
						memcpy(transfer_info->sysDate, szKey, strlen(szKey));
						xmlFree(szKey); 
					}
					else if(!xmlStrcmp(subcurNode->name, (const xmlChar *)"sysTime"))
					{
						szKey = xmlNodeGetContent(subcurNode);
						memcpy(transfer_info->sysTime, szKey, strlen(szKey));
						xmlFree(szKey); 
					}
					else if(!xmlStrcmp(subcurNode->name, (const xmlChar *)"merPriv"))
					{
						szKey = xmlNodeGetContent(subcurNode);
						memcpy(transfer_info->merPriv, szKey, strlen(szKey));
						xmlFree(szKey); 
					}
					else if(!xmlStrcmp(subcurNode->name, (const xmlChar *)"chkValue"))
					{
						szKey = xmlNodeGetContent(subcurNode);
						memcpy(transfer_info->chkValue, szKey, strlen(szKey));
						xmlFree(szKey); 
					}
					subcurNode = subcurNode->next;
				}
		   }
		   curNode = curNode->next;
		}
		else 
		{
			dcs_log(0, 0, "解析status OK");
			xmlFreeDoc(doc);
			return 0;
		}
	}
   
	dcs_log(0, 0, "解析应答报文OK");
    xmlFreeDoc(doc);
    return 0;
}

/*
 todo根据传入的路径解析XML文件
*/

/*
**实时代付失败，需要发起冲正
**向核心发起冲正交易
**
*/
int sendReversal(PEP_JNLS pep_jnls)
{
	int channel_id = 0;
	char T0_flag[2], agent_code[6+1], tmpbuf[128];
	//ISO8583结构体，送往核心系统的数据
	ISO_data siso;
	char currentDate[14+1];
	int s = 0, rtn = 0;
    
	struct tm *tm;
	time_t t;
    
    time(&t);
    tm = localtime(&t);
	
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	memset(T0_flag, 0x00, sizeof(T0_flag));
	memset(agent_code, 0x00, sizeof(agent_code));
	memset(currentDate, 0x00, sizeof(currentDate));
	memset(&siso, 0x00, sizeof(ISO_data));

	sprintf(currentDate, "%04d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
    sprintf(currentDate+8, "%02d%02d%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
	
	//加载前置与核心的8583配置文件
	if(IsoLoad8583config(&iso8583conf[0], "iso8583.conf") < 0)
	{
		dcs_log(0, 0, "Load iso8583.conf failed:%s",strerror(errno));
		return -1;
	}
	
	memcpy(iso8583_conf, iso8583conf, sizeof(iso8583conf));
	iso8583 = &iso8583_conf[0];
  	SetIsoHeardFlag(0);
    SetIsoFieldLengthFlag(1);
	#ifdef __TEST__
		dcs_log(0, 0, "currentDate=[%s]", currentDate);
		dcs_log(0, 0, "pep_jnls.outcdno=[%s]", pep_jnls.outcdno);
		dcs_log(0, 0, "pep_jnls.tranamt=[%s]", pep_jnls.tranamt);
		dcs_log(0, 0, "pep_jnls.trace=[%ld]", pep_jnls.trace);
		dcs_log(0, 0, "pep_jnls.samid=[%s]", pep_jnls.samid);
		dcs_log(0, 0, "pep_jnls.termid=[%s]", pep_jnls.termid);
	#endif
	//发起冲正交易
	setbit(&siso, 0, (unsigned char *)"0400", 4);
	//卡号
	setbit(&siso, 2, (unsigned char *)pep_jnls.outcdno, strlen(pep_jnls.outcdno));

	/*3域重新赋值*/
	setbit(&siso, 3, (unsigned char *)"000000", 6);
	/*交易金额*/
	setbit(&siso, 4, (unsigned char *)pep_jnls.tranamt, 12);
	
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	memcpy(tmpbuf, currentDate+4, 4);
	memcpy(tmpbuf+4, currentDate+8, 6);
	setbit(&siso, 7, (unsigned char *)tmpbuf, 10);
	
	/*原前置流水号*/
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	sprintf(tmpbuf, "%06ld", pep_jnls.trace);
	setbit(&siso, 11, (unsigned char *)tmpbuf, 6);

	channel_id = GetChannelId(pep_jnls.samid);
	if(channel_id < 0)
	{
		dcs_log(0, 0, "获取渠道信息失败");
		return -1;
	}
	
	memset(T0_flag, 0x00, sizeof(T0_flag));
	memset(agent_code, 0x00, sizeof(agent_code));
	rtn = GetAgentInfo(pep_jnls.termid, pep_jnls.samid, T0_flag, agent_code);
	if(rtn ==-1)
	{
		dcs_log(0, 0, "获取代理商信息失败");
		return -1;
	}
	if(strlen(T0_flag)==0)
	{
		memcpy(agent_code, "000000", 6);
		memcpy(T0_flag, "0", 1);
	}
	else if(memcmp(T0_flag, "0", 1)==0)
	{
		memcpy(agent_code, "000000", 6);
	}
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	sprintf(tmpbuf, "%s%-15s%s01%s%s%04d%s%s",pep_jnls.samid, pep_jnls.termid, pep_jnls.translaunchway+1, pep_jnls.trancde, "000000", channel_id, T0_flag, agent_code);
	setbit(&siso, 21,(unsigned char *)tmpbuf, strlen(tmpbuf));
	
	/*服务点输入方式码*/
	setbit(&siso, 22, (unsigned char *)pep_jnls.poitcde, 3);
	/*卡片序列号*/
	if(getstrlen(pep_jnls.card_sn)!=0)
		setbit(&siso, 23, (unsigned char *)pep_jnls.card_sn, 3);

	setbit(&siso,25,(unsigned char *)pep_jnls.poscode, 2);
	setbit(&siso,28,(unsigned char *)pep_jnls.filed28, strlen(pep_jnls.filed28));
	setbit(&siso, 39,(unsigned char *)"98", 2);
	/*重组冲正交易41,42域*/
	setbit(&siso, 41, (unsigned char *)pep_jnls.termcde, 8);
	setbit(&siso, 42, (unsigned char *)pep_jnls.mercode, 15);

	s = getstrlen(pep_jnls.billmsg);
	if( s > 0)
		setbit(&siso, 48, (unsigned char *)pep_jnls.billmsg, s);	
	else
	{
		memset(tmpbuf, 0, sizeof(tmpbuf));
		sprintf(tmpbuf, "%s%s%06ld", "DY", currentDate+2, pep_jnls.trace);
		setbit(&siso, 48, (unsigned char *)tmpbuf, 20);
	}
	#ifdef __TEST__
		dcs_log(0, 0, "22 pep_jnls.poitcde=[%s]", pep_jnls.poitcde);
		dcs_log(0, 0, "23 pep_jnls.card_sn=[%s]", pep_jnls.card_sn);
		dcs_log(0, 0, "25 pep_jnls.poscode=[%s]", pep_jnls.poscode);
		dcs_log(0, 0, "41 pep_jnls.termcde=[%s]", pep_jnls.termcde);
		dcs_log(0, 0, "42 pep_jnls.mercode=[%s]", pep_jnls.mercode);
		dcs_log(0, 0, "48 pep_jnls.billmsg=[%s]", pep_jnls.billmsg);
	#endif
	setbit(&siso, 49, (unsigned char *)"156", 3);
	setbit(&siso, 60, (unsigned char *)"6011", 4);

	//1：更新原笔交易pep_jnls的冲正标识字段
	//2: 更新转账汇款表里的冲正标识字段
	//冲正应答码为空
	memset(pep_jnls.aprespn, 0x00, 2);
	rtn = updateReverFlg(siso, pep_jnls);
	if(rtn ==-1)
	{
		dcs_log(0, 0, "更新冲正标识失败");
		return -1;
	}
	//保存数据库记录，保存一条新的冲正记录
	rtn = SaveReversalInfo(siso, pep_jnls);
	if(rtn ==-1)
	{
		dcs_log(0, 0, "保存冲正信息失败");
		return -1;
	}
	/*组消费包发送给核心*/
	rtn = WriteXpe(siso);
	if(rtn == -1)
	{
		dcs_log(0,0,"发送信息给核心失败");
		//更新pep_jnls中该笔冲正的状态
		//更新转账汇款表中该笔冲正的状态
		memcpy(pep_jnls.aprespn, "F9", 2);
		rtn = updateReverFlg(siso, pep_jnls);
		if(rtn ==-1)
		{
			dcs_log(0, 0, "更新冲正状态失败");
			return -1;
		}
		return -1;
	}
	return 0;
}

/*
**校验代付应答的签名值
**参与数字签名的字符串：orderId + transAmt + tseq + transStatus
*/
int checkMD5value(TRANSFER_INFO transfer_info)
{
	char md5Buf[128];/*参与计算MD5的串*/
	char MD5key[16+1];/*存放MD5key EAAoGBAMkYfh7MeT*/
	char MD5Value[120+1];/*存放计算出的MD5串*/
	int len = 0, rtn = 0;
	
	char local_ip[16];/*存放本地IP地址*/

	memset(md5Buf, 0x00, sizeof(md5Buf));
	memset(MD5key, 0x00, sizeof(MD5key));
	memset(MD5Value, 0x00, sizeof(MD5Value));
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
	sprintf(md5Buf, "%s%s%s%s%s", transfer_info.orderId, transfer_info.transAmt, transfer_info.tseq, transfer_info.transStatus, MD5key);
	len = strlen(md5Buf);
	rtn = getUpMD5(md5Buf, len, MD5Value);
	if(rtn ==-1)
	{
		dcs_log(0, 0, "计算MD5失败");
		return -1;
	}
	if(memcmp(transfer_info.chkValue, MD5Value, strlen(transfer_info.chkValue))!=0)
	{
		dcs_log(0, 0, "应答：chkValue=[%s], 平台计算[%s]", transfer_info.chkValue, MD5Value);
		dcs_log(0, 0, "签名字校验失败");
		return -1;
	}
	return 0;
}

/*
函数功能：处理代付返回
处理失败：返回-1；成功返回0
*/
int dealResponse(PEP_JNLS pep_jnls, TRANSFER_INFO transfer_info, char *rcvbuf, int rcvlen, ISO_data siso)
{
	EWP_INFO ewp_info;
	TRANSFER_INFO trans_info;
	int len = 0, rtn = 0;
	char result[128];
	memset(&trans_info, 0, sizeof(TRANSFER_INFO));
	memset(&ewp_info, 0, sizeof(EWP_INFO));
	memset(result, 0x00, sizeof(result));
	
	memcpy(pep_jnls.transfer_type, transfer_info.transfer_type, 1);
	//实时代付失败需要发起冲正交易
	//解析返回数据rcvbuf若代付成功更新记录，若代付失败则需要发起冲正，若代付受理中则需要等待异步处理结果
	rtn = getResPonse(rcvbuf, rcvlen, &trans_info);
	if(rtn ==-1)
	{
		dcs_log(0, 0, "解析应答失败");
		return -1;
	}
	//解析之后验证签名 参与数字签名的字符串：orderId + transAmt + tseq + transStatus
	if(memcmp(trans_info.payResultcode+8, "00", 2)==0)
	{
		rtn = checkMD5value(trans_info);
		if(rtn ==-1)
		{
			return -1;
		}
		//更新代付状态，返回EWP
		if(trans_info.transStatus[0] == 'S')
		{
			//代付成功，更新记录，返回EWP
			memcpy(trans_info.pay_respn, "00", 2);
			memcpy(trans_info.pay_respn_desc, "代付成功", strlen("代付成功"));
			rtn = updatePayInfo(&trans_info);//更新代付记录
			if(rtn ==-1)
			{
				dcs_log(0, 0, "更新代付状态失败");
				return -1;
			}
			//发送短信
			if( memcmp(pep_jnls.modeflg, "1", 1) == 0)
			{
				dcs_log(0, 0, "send msg info");
				memcpy(pep_jnls.syseqno, trans_info.tseq, 12);/*短信参考号为代付流水号*/
				strcpy(pep_jnls.filed28, transfer_info.fee);
				strcpy(pep_jnls.pepdate, transfer_info.sysDate);
				strcpy(pep_jnls.peptime, transfer_info.sysTime);
				SendMsg(pep_jnls);
			}
		}
		else if(trans_info.transStatus[0] == 'F')
		{
			//受理成功，代付失败，更新记录，返回EWP，需要发起冲正
			memcpy(trans_info.pay_respn, "0F", 2);
			memcpy(trans_info.pay_respn_desc, trans_info.errorMsg, strlen(trans_info.errorMsg));
			sendReversal(pep_jnls);//向核心发起冲正交易
			rtn = updatePayInfo(&trans_info);//更新代付记录
			if(rtn ==-1)
			{
				dcs_log(0, 0, "更新代付状态失败");
				return -1;
			}
			memcpy(pep_jnls.aprespn, "0F", 2);
		}
		else if(trans_info.transStatus[0] == 'W')
		{
			//代付受理中，更新记录，返回EWP
			memcpy(trans_info.pay_respn, "A6", 2);
			memcpy(trans_info.pay_respn_desc, "受理中", strlen("受理中"));
			rtn = updatePayInfo(&trans_info);//更新代付记录
			if(rtn ==-1)
			{
				dcs_log(0, 0, "更新代付状态失败");
				return -1;
			}
			return 0; 
		}
		else
		{
			dcs_log(0, 0, "不合法的应答");
			return -1;
		}
	}
	else
    {
		//受理失败
		memcpy(transfer_info.pay_respn, trans_info.payResultcode+8, 2);
		memcpy(transfer_info.pay_respn_desc, trans_info.payResultcode_desc, strlen(trans_info.payResultcode_desc));
		sendReversal(pep_jnls);//向核心发起冲正交易
		rtn = updatePayInfo(&transfer_info);//更新代付记录
		if(rtn ==-1)
		{
			dcs_log(0, 0, "更新代付状态失败");
			return -1;
		}
		memcpy(pep_jnls.aprespn, trans_info.payResultcode+8, 2);
		strcpy(trans_info.orderId, transfer_info.orderId);
		strcpy(trans_info.pay_respn, transfer_info.pay_respn);
	}
	
	//先移除超时控制
	rtn = delete_timeout_control(trans_info.orderId, result, sizeof(result)); //测试超时控制超时发送的功能
	if(rtn ==-1)
	{
		dcs_log(0, 0, "移除超时控制失败");
		return -1;
	}
	if(memcmp(result+2, "response=00", 11)==0)
	{
		if(memcmp(result+14, "status=2", 8)==0)//2 表示 超时控制已超时返回
		{
			dcs_log(0, 0, "移除成功,已超时返回,此处不再返回");
			return 0;
		}
		else if(memcmp(result+14, "status=1", 8)==0||memcmp(result+14, "status=0", 8)==0)//0 表示 没找到key对应的超时控制；1表示 移除成功
		{
			//更新返回终端的应答码
			updateResponseCode(trans_info);
			dcs_log(0, 0, "移除超时控制成功");
		}
	}
	rtn = RetToEWP(siso, ewp_info, pep_jnls, 1);//返回EWP
	if(rtn ==-1)
	{
		dcs_log(0, 0, "返回EWP失败");
		return -1;
	}
	return 0;
}

/*
	处理转账汇款向融易付系统发起代付请求
*/
int PayRequest(PEP_JNLS pep_jnls, TRANSFER_INFO transfer_info, ISO_data iso)
{
	int	len = 0, rtn = 0;
	char rcvbuf[2048];
	char sendmess[1024];
	int queue_id = -1;
	long msgtype = 0;

	memset(rcvbuf, 0x00, sizeof(rcvbuf));
	memset(sendmess, 0x00, sizeof(sendmess));
	
	rtn = updatePayReqDate(transfer_info);//更新代付请求日期
	if(rtn ==-1)
	{
		dcs_log(0, 0, "更新代付请求日期失败");
		return -1;
	}
	/*调用融易付系统的代付接口*/
	//组包key=value的方式
	rtn = getPacket(pep_jnls, transfer_info, sendmess);
	if(rtn ==-1)
	{
		dcs_log(0, 0, "组代付请求包失败");
		return -1;
	}
	len = strlen(sendmess);
	
	//发送报文到队列
	queue_id = queue_connect("DDFSC");
	if(queue_id <0)
	{
		dcs_log(0, 0, "connect msg queue fail!");
		return -1;
	}
	msgtype = getpid();
	rtn = queue_send_by_msgtype(queue_id, sendmess, len, msgtype, 0);
	if(rtn< 0)
	{
		dcs_log(0, 0, "queue_send_by_msgtype err");
		return -1;
	}
	//接收队列
	queue_id = queue_connect("R_DFSC");
	if ( queue_id <0)
	{
		dcs_log(0, 0, "connect msg queue fail!");
		return -2;
	}
	memset(rcvbuf, 0x00, sizeof(rcvbuf));
	rtn = queue_recv_by_msgtype(queue_id, rcvbuf, sizeof(rcvbuf), &msgtype, 0);
	if(rtn< 0)
	{
		dcs_log(0, 0, "queue_recv_by_msgtype err");
		return -2;
	}
	if(memcmp(rcvbuf, "00", 2) == 0)//返回成功
	{
		dcs_log(0, 0, "返回应答成功[%s]", rcvbuf+2);
		dealResponse(pep_jnls, transfer_info, rcvbuf+2, rtn-2, iso);
		return 0;
	}
	else
	{
	  	dcs_log(0, 0, "返回应答[%s]", rcvbuf);
		return -2;
	}
	return 0;
}
