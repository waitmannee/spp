#include "ibdcs.h"
#include "tmcibtms.h"
#include "iso8583.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "extern_function.h"
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<pthread.h>
#include "dccsock.h"
static int num=1;//每秒发送个数
static int sendCycle=1;//发送周期,以秒为单位
static int recvnum=0;//成功接收到返回包的报文个数
//static char gIp[25]="";
static int gPort=18880;
static int gSendNum=0;
static int packLen;
static char gIp[20]="192.168.20.176";
static char TPDU[11] ="6008230000";
static char FOLDNAME[20]="MYFOLD";
//static char FOLDNAME2[20]="CL_TPOS_001";
static char FOLDNAME2[20]="TRANSRCV";
#define MAX_PROCE 250

static int gType =1;//报文类型,1表示传统POS,2表示EWP


typedef struct child
{
	int pid[10];
	int num;
}child;

static int moniSale();
static int moniCancelSale();
static int moniQuery();
static int SetPara();
static int Makesocket(void *trace);
static int Unpackpos(char *srcBuf,int len);
static int recvLOng(int fd);
static int PosLongLianSale();
static int EwpLongLianSale();
static int pos_ewp();
static int wbpos();
static int recvLOng3();
static int clientsocket();
static int server();
static int  EwpSale();
static int SendDataToFold();
static int setServer();
static int moniFold();
static int SetFoldPara();
int monihttp();
int monihttp2();
int moniHttpClient( );

int moniTcpSyn( );
int moniHttpSyn( );
int moniTime( );
int EwpZhuanZhang();
int moniMpos();
int testxml();
int moniXpeBalance();
int moniXpeSale();
extern struct ISO_8583 *iso8583;
struct ISO_8583 iso8583_conf[128];
typedef struct packBuf
{
	char buf[500];
	int trace;
}packBuf;
struct TPDU
{
	int port;
	char tpdu[11];
}stTpdu[4]={
	{9988,"6008230000"},//159前置
	{18880,"6008230000"},//159前置
	{9090,"6008190000"},//
	{9700,"6009150000"},//uat
	};
char iso8583name[129][30] = {
	"消息类型",			/*bit 0 */
	"bitmap",
	"主帐号",
	"处理代码",
	"交易金额",
	"自定义",
	"自定义",
	"交易日期和时间",
	"自定义",
	"自定义",
	"自定义",
	"系统流水号",
	"本地交易时间",
	"本地交易日期",
	"自定义",
	"结算日期",
	"自定义",
	"受理日期",
	"商户类型",
	"自定义",
	"自定义",
	"终端信息",
	"服各点进入方式",
	"自定义",
	"自定义",
	"服务点条件码",
	"自定义",
	"自定义",
	"交易费",
	"自定义",
	"交易处理费",
	"自定义",
	"代理方机构标识码",
	"发送机构标识码",
	"自定义",
	"第二磁道数据",
	"第三磁道数据",
	"检索参考号",
	"授权标识响应",
	"响应代码",					/* bit 39 */
	"服务限制代码",
	"受卡方终端标识",
	"受卡方标识代码",
	"受卡方名称/地点",
	"附加响应数据",
	"自定义",
	"自定义",
	"自定义",
	"自定义附加数据",
	"交易货币代号",
	"结算货币代号",
	"自定义",
	"个人识别号数据",
	"安全控制信息",
	"附加金额",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"MACkey使用标志",
	"自定义",
	"原因码",
	"结算批次号",
	"MAC值",						/* bit 64 */
	"自定义",
	"结算代号",
	"自定义",
	"自定义",
	"自定义",
	"网络管理功能码",
	"报文编号",
	"后报文编号",
	"动作日期",
	"贷记笔数",
	"撤消贷记笔数",
	"借记笔数",
	"撤消借记笔数",
	"转帐笔数",
	"撤消转帐笔数",
	"自定义",
	"授权笔数",
	"贷记手续费金额",
	"自定义",
	"借记手续费金额",
	"自定义",
	"贷记交易金额",
	"撤消贷记金额",
	"借记交易金额",
	"撤消借记金额",
	"原始数据",
	"文件更新代码",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"报文保密代码",
	"净清算额",
	"自定义",
	"自定义",
	"接收机构标识代码",
	"文件名称",
	"转出账户",						/* bit 102 */
	"转入账户",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"自定义",
	"MAC值"						/* bit 128 */
};

static void TransRcvSvrExit(int signo)
{
    if(signo > 0)
        dcs_log(0,0,"catch a signal %d\n",signo);
    if(signo !=0 && signo != SIGTERM && signo != SIGSEGV)
        return;

    dcs_log(0,0,"AppServer terminated.\n");
    exit(signo);
}


static int OpenLog()
{
    char path[500];
    char file[520];
    memset(path,0,sizeof(path));
    memset(file,0,sizeof(file));
    getcwd(path,500);
    sprintf(file,"%s/moniSale.log",path);
    return dcs_log_open(file, "moniSale");
}
static int OpenLog1()
{
    char path[500];
    char file[520];
    memset(path,0,sizeof(path));
    memset(file,0,sizeof(file));
    getcwd(path,500);
    sprintf(file,"%s/moniSale1.log",path);
    return dcs_log_open(file, "moniSale");
}
int main(int argc, char *argv[])
{
	catch_all_signals(TransRcvSvrExit);

    	//memset(child,0,sizeof(child));
    	cmd_init("moniSalecmd>>",TransRcvSvrExit);
  	cmd_add("monisale",     moniSale,   " test send message,short link");
	cmd_add("moniquery",     moniQuery,   " test send message,short link");
    	cmd_add("set",     SetPara,   " set para");
	cmd_add("long",     PosLongLianSale,   " test long link for pos sale");
	cmd_add("ewplong",     EwpLongLianSale,   " test long link for ewp sale");
	cmd_add("pos_ewp",     pos_ewp,   " test for ewp sale and pos sale");
	cmd_add("wbpos",     wbpos,   " test for pos sale ");
	cmd_add("server",    server,   " test for pos sale ");
	cmd_add("client",    clientsocket,   " test for pos sale ");
	cmd_add("ewp",    EwpSale,   " test for ewp sale ");
	cmd_add("setserver",    setServer,   " set ip");
	cmd_add("SendToFold",    SendDataToFold,   " SendDataToFold");
	cmd_add("moniFold",    moniFold,   " moniFold");
	cmd_add("setnum",    SetFoldPara,   " SetFoldPara");
	cmd_add("monihttp",    monihttp,   " monihttp");
	cmd_add("monihttp2",    monihttp2,   " monihttp2");
	cmd_add("moniHttpClient",    moniHttpClient,   " moniHttpClient");
	cmd_add("moniTcpSyn",    moniTcpSyn,   " moniTcpSyn");
	cmd_add("moniHttpSyn",    moniHttpSyn,   " moniHttpSyn");
	cmd_add("moniTime",    moniTime,   " 测试超时控制");
	cmd_add("moniMpos",    moniMpos,   " moniMpos");
	cmd_add("zhuanzhang",    EwpZhuanZhang,   " EwpZhuanZhang");
	cmd_add("monicancelsale",    moniCancelSale,   " moniCancelSale");
	cmd_add("testxml",    testxml,   " testxml");
	cmd_add("moniXpeBalance",    moniXpeBalance,   " moniXpeBalance");
	cmd_add("monixpeSale",    moniXpeSale,   " moniXpeSale");
   	cmd_shell(argc,argv);

  	TransRcvSvrExit(0);
    return 0;
}//main()

/**xml格式 相关函数文件
 * add by wu at 2015/08/10
 */

#include "ibdcs.h"
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

/**
 * 将8583结构体转换为xml 格式
 * param:	iso  传入的8583结构，xmlBuf 生成的对应的xml格式字符串
 * return：   -1 转换失败，0表示转换成功
 */
int iso_to_xml(ISO_data * iso, char * xmlBuf)
{
	char tmpbuf[1024];
	char ascii_buf[1024];
	char index[3 + 1];

	int i = 0, s = 0, flagmap = 0;
	int outlen = 0;
	//xml相关变量
	xmlDocPtr doc; //定义解析文档指针
	xmlNodePtr root_node, bits_node, bit_node, node; //定义结点指针
	xmlChar *outbuf;

	doc = xmlNewDoc((xmlChar *) "1.0");
	if(doc == NULL)
	{
		dcs_log(0, 0, "Fail to create new XML doc");
		return -1;
	}
	root_node = xmlNewNode(NULL, (xmlChar *) "iso8583");
	if(root_node == NULL)
	{
		dcs_log(0, 0, "Fail to create new node");
		xmlFreeDoc(doc);
		return -1;
	}
	xmlDocSetRootElement(doc, root_node); //设置根节点

	bits_node = xmlNewChild(root_node, NULL, (xmlChar *) "bits", NULL); //添加bits节点
	if(bits_node == NULL)
	{
		dcs_log(0, 0, "Fail to create new node");
		xmlFreeDoc(doc);
		return -1;
	}
	//设置交易类型
	memset(tmpbuf, 0, sizeof(tmpbuf));
	s = getbit(iso, 0, (unsigned char*) tmpbuf);
	if (s < 4)
	{
		dcs_log(0, 0, "get bit 0 fail");
		xmlFreeDoc(doc);
		return -1;
	}
	bit_node = xmlNewChild(bits_node, NULL, (xmlChar *) "bit", NULL); //添加bit节点
	if(bit_node == NULL)
	{
		dcs_log(0, 0, "Fail to create new node");
		xmlFreeDoc(doc);
		return -1;
	}
	node = xmlNewChild(bit_node, NULL, (xmlChar *) "index", (xmlChar *) "0"); //添加index节点
	if(node == NULL)
	{
		dcs_log(0, 0, "Fail to create new node");
		xmlFreeDoc(doc);
		return -1;
	}
	node = xmlNewChild(bit_node, NULL, (xmlChar *) "value", (xmlChar *) tmpbuf); //添加value节点
	if(node == NULL)
	{
		dcs_log(0, 0, "Fail to create new node");
		xmlFreeDoc(doc);
		return -1;
	}
	for (i = 2; i <= 128; i++)
	{
		memset(tmpbuf, 0, sizeof(tmpbuf));
		s = getbit(iso, i, (unsigned char *) tmpbuf);
		if (s > 0)
		{
			dcs_log(0, 0, "i=%d,tmpbuf=[%s],s=%d", i, tmpbuf, s);
			if (i > 64)
			{
				flagmap = 1;
			}
			//二磁道和三磁道 需要将‘=’ 转换为'D'
			if (i == 35 || i == 36)
			{
				int j = 0;
				for (j = 0; j < s; j++)
				{
					if (tmpbuf[j] == '=')
						tmpbuf[j] = 'D';
				}
			}
			//将可能含有不可见字符的域转换为可见字符
			if (i == 52 || i == 55 || i == 64 || i == 128)
			{
				memset(ascii_buf, 0x0, sizeof(ascii_buf));
				bcd_to_asc((unsigned char*) ascii_buf, (unsigned char*) tmpbuf, s * 2, 0);
				memset(tmpbuf, 0x0, sizeof(tmpbuf));
				strcpy(tmpbuf, ascii_buf);
			}

			memset(index, 0, sizeof(index));
			sprintf(index, "%d", i);
			if(i == 64)
			{
				bit_node = xmlNewChild(bits_node, NULL, (xmlChar *) "bit", NULL); //添加bit节点
				if(bit_node == NULL)
				{
					dcs_log(0, 0, "Fail to create new node");
					xmlFreeDoc(doc);
					return -1;
				}
				if (xmlNewChild(bit_node, NULL, (xmlChar *) "index", (xmlChar *) index) == NULL) //添加index节点
				{
					dcs_log(0, 0, "xmlNewChild nodename=[index],text=[%s] ", index);
					xmlFreeDoc(doc);
					return -1;
				}
				if (xmlNewChild(bit_node, NULL, (xmlChar *) "value", (xmlChar *) tmpbuf) == NULL) //添加value节点
				{
					dcs_log(0, 0, "xmlNewChild  nodename=[value],text=[%s]", tmpbuf);
					xmlFreeDoc(doc);
					return -1;
				}
				xmlNewProp(bit_node, (xmlChar *) "name", (xmlChar *) "mac域");
			}
			else
			{
				bit_node = xmlNewChild(bits_node, NULL, (xmlChar *) "bit", NULL); //添加bit节点
				if(bit_node == NULL)
				{
					dcs_log(0, 0, "Fail to create new node");
					xmlFreeDoc(doc);
					return -1;
				}
				if (xmlNewChild(bit_node, NULL, (xmlChar *) "index", (xmlChar *) index) == NULL) //添加index节点
				{
					dcs_log(0, 0, "xmlNewChild nodename=[index],text=[%s] ", index);
					xmlFreeDoc(doc);
					return -1;
				}
				if (xmlNewChild(bit_node, NULL, (xmlChar *) "value", (xmlChar *) tmpbuf) == NULL) //添加value节点
				{
					dcs_log(0, 0, "xmlNewChild  nodename=[value],text=[%s]", tmpbuf);
					xmlFreeDoc(doc);
					return -1;
				}
			}
		}
		else if (s < 0) //取域失败
		{
			dcs_log(0, 0, "get bit(%d) fail",i );
			xmlFreeDoc(doc);
			return -1;
		}
	}
	//根节点的属性 赋值为8583的类型 ，64个域或者128域
	memset(tmpbuf, 0, sizeof(tmpbuf));
	if (flagmap == 1)
	{
		strcpy(tmpbuf, "128");
	}
	else
	{
		strcpy(tmpbuf, "64");
	}
	if(xmlNewProp(root_node, (xmlChar *) "type", (xmlChar *) tmpbuf) == NULL)
	{
		dcs_log(0, 0, "xmlNewProp fail");
		xmlFreeDoc(doc);
		return -1;
	}
	xmlDocDumpFormatMemoryEnc(doc, &outbuf, &outlen, "utf-8", 1);
	memcpy(xmlBuf, outbuf, outlen);

	s = xmlSaveFormatFileEnc("/var/xpep/run/log/8583.xml", doc, "utf-8", 1);
	if (s < 0)
	{
		dcs_log(0, 0, "create 8583.xml fail");
		xmlFree(outbuf);
		xmlFreeDoc(doc);
		return -1;
	}
	xmlFree(outbuf);
	xmlFreeDoc(doc);
	return 0;
}
/**
 * 将xml格式内容转换为8583结构体
 * param:	iso  转换后的8583结构，xmlBuf 传入的存放XML格式数据的内存区,size内存中XML格式数据的长度
 * return：   -1 转换失败，0表示转换成功
 */
int xml_to_iso(ISO_data * iso, char * xmlBuf, int size)
{
	int flag = 0, i = 0, j = 0;
	int len = 0;
	char tmpbuf[1024 + 1];
	char buf[1024];

	//xml 相关定义
	xmlDocPtr doc;
	xmlNodePtr rootNode, bitNode, cur;
	xmlChar * content;

	clearbit(iso);
	doc = xmlParseMemory((const char *) xmlBuf, size);
	if (doc == NULL)
	{
		dcs_log( 0, 0, "Fail to parse XML buffer");
		return -1;
	}

	//获取根结点
	rootNode = xmlDocGetRootElement(doc);
	if (rootNode == NULL)
	{
		dcs_log( 0, 0, "get root element fail,empty document");
		xmlFreeDoc(doc);
		return -1;
	}
	//判断根节点是否正确
	if (strcmp((char *) rootNode->name, "iso8583") != 0)
	{
		dcs_log( 0, 0, "boot element[%s] not true", rootNode->name);
		xmlFreeDoc(doc);
		return -1;
	}

	//从二级节点中获取bits  节点
	cur = rootNode->xmlChildrenNode;
	while (cur != NULL)
	{
		if (strcmp((char *) cur->name, "bits") == 0)
			break;
		else
			cur = cur->next;
	}
	//未找到bits  节点
	if (cur == NULL)
	{
		dcs_log( 0, 0, "can not get bits element");
		xmlFreeDoc(doc);
		return -1;
	}

	//解析bits节点，获取每个子域的内容
	bitNode = cur->xmlChildrenNode;
	while (bitNode != NULL )
	{
		if(strcmp((char *) bitNode->name, "bit") == 0 )
		{
			//获得bit节点的子节点index和value
			cur = bitNode->xmlChildrenNode;
			while (cur != NULL)
			{
				//获取结点内容
				if (strcmp((char *) cur->name, "index") == 0)
				{
					content = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
					//判断索引是否是数字
					for(j=0 ; j<strlen(content); j++)
					{
						if(content[j] >'9' || content[j] <'0')
						{
							dcs_log(0, 0, "无效index节点值[%s]",content);
							break;
						}
					}
					//如果非数字则解析下一个bit节点
					if(j < strlen(content))
					{
						flag = 0;
						break;
					}
					i = atoi((char *) content);
					xmlFree(content);
					flag++;
				}
				else if (strcmp((char *) cur->name, "value") == 0)
				{
					memset(tmpbuf, 0, sizeof(tmpbuf));
					content = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
					strcpy(tmpbuf, (char *) content);
					xmlFree(content);
					flag++;
				}
				cur = cur->next;
			}
			if (flag == 2 && strlen(tmpbuf) != 0)
			{
				len = strlen(tmpbuf);
				if (i == 35 || i == 36)
				{
					for (j = 0; j < len; j++)
					{
						if (tmpbuf[j] == 'D')
							tmpbuf[j] = '=';
					}
				}

				//之前这些域将不可见字符转换为可见字符,现需转换为之前的格式
				if (i == 52 || i == 55 || i == 64 || i == 128)
				{
					memset(buf, 0x0, sizeof(buf));
					asc_to_bcd((unsigned char*) buf, (unsigned char*) tmpbuf, len, 0);
					memset(tmpbuf, 0x0, sizeof(tmpbuf));
					memcpy(tmpbuf, buf, (len + 1) / 2);
					len = (len + 1) / 2;
				}

				setbit(iso, i, (unsigned char*) tmpbuf, len);
				dcs_log(0, 0, "获得 [%d]域[%s]", i, tmpbuf);
			}
		}
		flag = 0;
		bitNode = bitNode->next;
	}
	xmlFreeDoc(doc);
	return 0;
}


/**
 * 根据xpath路径获取某个节点的值
 * param: xmlBuf 传入的存放XML格式数据的内存区,size内存中XML格式数据的长度,
 * 		  szXpath 要查询的xpath路径,buf 成功时返回对应内容字符串
 * return： -1表示失败,0表示成功
 */
int getNodeValueFromXml(char * xmlBuf, int size, const xmlChar * szXpath,char *buf)
{
	//xml 相关定义
	xmlDocPtr doc;
	xmlXPathContextPtr context; //XPATH上下文指针
	xmlXPathObjectPtr result; //XPATH对象指针，用来存储查询结果

	doc = xmlParseMemory((const char *) xmlBuf, size);
	if (doc == NULL)
	{
		dcs_log( 0, 0, "Fail to parse XML buffer");
		return -1;
	}

	context = xmlXPathNewContext(doc); //创建一个XPath上下文指针
	if (context == NULL)
	{
		dcs_log( 0, 0, "context is NULL\n");
		xmlFreeDoc(doc);
		return -1;
	}
	result = xmlXPathEvalExpression(szXpath, context); //查询XPath表达式，得到一个查询结果

	if (result == NULL)
	{
		dcs_log( 0, 0, "xmlXPathEvalExpression return NULL\n");
		xmlXPathFreeContext(context); //释放上下文指针
		xmlFreeDoc(doc);
		return -1;
	}
	if (xmlXPathNodeSetIsEmpty(result->nodesetval)) //检查查询结果是否为空
	{
		xmlXPathFreeObject(result);
		xmlXPathFreeContext(context); //释放上下文指针
		xmlFreeDoc(doc);
		dcs_log( 0, 0, "nodeset is empty\n");
		return -1;
	}
	//返回查到的结果
	strcpy(buf, xmlNodeGetContent(result->nodesetval->nodeTab[0]));
	xmlXPathFreeObject(result);
	xmlXPathFreeContext(context); //释放上下文指针
	xmlFreeDoc(doc);
	return 0;
}

/*
模拟消费交易
*/
int moniSale()
{
 	ISO_data siso;
       struct  tm *tm;
	time_t  t;
	char    *ltime;
	char tmpbuf[40];
	int s = 0, ret=0;
    char	buffer[400], newbuf[550];
	int seconds=0,i=0;
	int currentseconds=0,sendnum=0;
	int trace=1;
	int starttime=0,currenttime=0;
	int totaltime=0;
	char szTpdu[11]={0};
	int pid,cpid;
	s = OpenLog();
	if(s < 0)
        return -1;
	gType =1;
	pid = fork();
	if(pid<0)
	{
		 dcs_log(0,0,"fork child err");
		 return -1;
	}
	else if(pid >0)
	{
		waitpid(pid, NULL, 0);
		 dcs_log(0,0,"fork child succ,pid=[%d]",pid);
		 return 0;
	}

	cpid = fork();
	if(cpid<0)
	{
		 dcs_log(0,0,"fork child err");
		 return -1;
	}
	else if(cpid >0)
	{
		 dcs_log(0,0,"fork child succ,cpid=[%d]",cpid);
		 exit(0);
	}
	struct packBuf *pack;
	if(IsoLoad8583config(&iso8583_conf[0],"iso8583_pos.conf") < 0)
   	 {
	        dcs_log(0,0,"IsoLoad8583config() failed:%s\n",strerror(errno));

	        printf("IsoLoad8583config failed \n");

	        return -1;
    	}

	iso8583=&iso8583_conf[0];
	SetIsoHeardFlag(0);
	SetIsoFieldLengthFlag(0);
	recvnum =0;
	gSendNum =0;
	for(i=0;i<10;i++)
	{
		if(stTpdu[i].port == gPort)
		{
			memcpy(szTpdu,stTpdu[i].tpdu,10);
			break;
		}

	}
	if(i>=10)
	{
		dcs_log(0,0,"port=[%d] not ture!",gPort);
		printf("port=[%d] not ture!",gPort);
		return 0;
	}
	i=0;

	dcs_log(0,0," 模拟POS消费交易发送给前置 start");//0,1,3,4,11,22,25,26,35,36,41,42,49,52,53,60,64
	starttime= time((time_t*)NULL);
	currenttime = starttime;
	if(sendCycle == 0)
	{
		totaltime =100*24*60*60;//100天
	}
	else
	{
		//totaltime =sendCycle*60;
		totaltime =sendCycle;
	}

	while(currenttime-starttime<totaltime)//发送周期循环
	{
		seconds= time((time_t*)NULL);
		currentseconds=seconds;
    	for(sendnum=1;sendnum<=num;sendnum++)//每秒发送循环
	    {
	       	memset(&siso,0,sizeof(ISO_data));
			memset(tmpbuf,0,sizeof(tmpbuf));
			pack=(struct packBuf *)malloc(sizeof(struct packBuf));
			if(pack == NULL)
			{
				dcs_log(0, 0, "malloc error.");
				return -1;
			}
			setbit(&siso,0,(unsigned char *)"0200", 4);
			setbit(&siso,3,(unsigned char *)"000000", 6);
			setbit(&siso,4,(unsigned char *)"000000000001", 12);//消费金额
			memset(tmpbuf, 0, sizeof(tmpbuf));
			time(&t);
    			tm = localtime(&t);
			sprintf(tmpbuf,"%02d%02d%02d%02d%02d",tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
			setbit(&siso,7,(unsigned char *)tmpbuf, 10);
			if(trace >999999)
				trace=1;
			memset(tmpbuf, 0, sizeof(tmpbuf));
			sprintf(tmpbuf,"%06d",trace);
			dcs_log(0,0,"trace=[%d],tpdu=[%s]",trace,szTpdu);//0,1,3,4,11,22,25,26,35,36,41,42,49,52,53,60,64
			setbit(&siso,11,(unsigned char *)tmpbuf, 6);//pos终端上送流水
			setbit(&siso,20,(unsigned char *)"00000", 5);
			setbit(&siso,21,(unsigned char *)"14NP811081326349320000088800000697000001cc000000002089860114003100149051", 72);
			setbit(&siso,22,(unsigned char *)"022", 3);
			setbit(&siso,25,(unsigned char *)"00", 2);
			setbit(&siso,35,(unsigned char *)"6228412320121305911D49121207693980000", 37);
			setbit(&siso,36,(unsigned char *)"996228412320121305911D156156000000000000000000000011414144912DD000000000000D000000000000D030705800000000", 104);
			setbit(&siso,41,(unsigned char *)"12347356",8);
			setbit(&siso,42,(unsigned char *)"999899898760004",15);
			setbit(&siso,53,(unsigned char *)"0600000000000000" , 16);

//			//测试失败
//			setbit(&siso,22,(unsigned char *)"052", 3);
//			setbit(&siso,23,(unsigned char *)"005", 3);
//			setbit(&siso,25,(unsigned char *)"00", 2);
//			setbit(&siso,35,(unsigned char *)"6214441000010053=301022000000", 29);
//			setbit(&siso,41,(unsigned char *)"12347356",8);
//			setbit(&siso,42,(unsigned char *)"999899898760004",15);
//			setbit(&siso,55,(unsigned char *)"9F2608FF4F8CC0465979689F2701809F100807000103A0A000019F37045D139ADB9F360204FC950508800080009A031511129C01009F02060000000000015F2A02015682023C009F1A0201569F03060000000000009F3303E0F9C89F34031E03009F3501229F1E0838313332363334398408A0000003330101029F090200209F410400001115",134);
//			setbit(&siso,53,(unsigned char *)"0600000000000000" , 16);

			setbit(&siso,49,(unsigned char *)"156",3);
		    setbit(&siso,60,(unsigned char *)"22000004000601",14);
			setbit( &siso, 64,(unsigned char *)"00000000000", 8 );

			iso8583 =&iso8583_conf[0];
		  	SetIsoHeardFlag(0);
		   	 SetIsoFieldLengthFlag(0);

			memset(buffer, 0, sizeof(buffer));
			s = iso_to_str( (unsigned char *)buffer, &siso, 0);
			if ( s < 0 )
			{
				dcs_log(0, 0, "iso_to_str error.");
				return -1;
			}

			memset(newbuf, 0, sizeof(newbuf));
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy( newbuf, szTpdu,10);
			memcpy( newbuf+10, "602200000000",12 );
			asc_to_bcd( (unsigned char *) tmpbuf, (unsigned char*) newbuf, 22, 0 );
			memset(newbuf, 0, sizeof(newbuf));
			memcpy(newbuf,"11",2);//放报文长度
			memcpy( newbuf+2, tmpbuf, 11);
			sprintf(newbuf+13,"086001003021004611223344%04d",s);//添加报文头2
			memcpy( newbuf+41, buffer, s);
			s = s + 41;
			newbuf[0]=(s-2)/256;
			newbuf[1]=(s-2)%256;
			memcpy(pack->buf,newbuf,s);
			packLen =s;
			pack->trace=trace;
		 	trace++;

			pthread_t sthread;
			if ( pthread_create(&sthread, NULL,( void * (*)(void *))Makesocket, (void *)pack) !=0)
			{
			   dcs_log(0,0,"Create thread fail ");
			}
			pthread_detach(sthread);
			gSendNum ++;
	    }
		dcs_log(0,0,"端口%d本次send num=[%d]",gPort,sendnum-1);
		dcs_log(0,0,"端口%d总共send num=[%d]",gPort,gSendNum);
		currentseconds= time((time_t*)NULL);
		while(currentseconds==seconds)
		{
			u_sleep_us(100000);
			currentseconds= time((time_t*)NULL);
		}
		currenttime= time((time_t*)NULL);
	}

	dcs_log(0,0,"发送报文结束end");
	while(recvnum != gSendNum )
	{
		sleep(1);
	}
	exit(1);

}



/*
模拟撤销交易
*/
int moniCancelSale()
{
 	ISO_data siso;
       struct  tm *tm;
	time_t  t;
	char    *ltime;
	char tmpbuf[40];
	int s = 0;
    	char	buffer[500], newbuf[450];
	int seconds=0,i=0;
	int currentseconds=0,sendnum=0;
	int trace=1;
	int starttime=0,currenttime=0;
	int totaltime=0;
	char szTpdu[11]={0};

	int pid,cpid;
	s = OpenLog();
	if(s < 0)
        return -1;
	gType =1;
	pid = fork();
	if(pid<0)
	{
		 dcs_log(0,0,"fork child err");
		 return -1;
	}
	else if(pid >0)
	{
		waitpid(pid, NULL, 0);
		 dcs_log(0,0,"fork child succ,pid=[%d]",pid);
		 return 0;
	}

	cpid = fork();
	if(cpid<0)
	{
		 dcs_log(0,0,"fork child err");
		 return -1;
	}
	else if(cpid >0)
	{
		 dcs_log(0,0,"fork child succ,cpid=[%d]",cpid);
		 exit(0);
	}
	struct packBuf *pack;
	if(IsoLoad8583config(&iso8583_conf[0],"iso8583_pos.conf") < 0)
   	 {
	        dcs_log(0,0,"IsoLoad8583config() failed:%s\n",strerror(errno));

	        printf("IsoLoad8583config failed \n");

	        return -1;
    	}

    	iso8583=&iso8583_conf[0];
  	SetIsoHeardFlag(0);
    	SetIsoFieldLengthFlag(0);
    	recvnum =0;
	gSendNum =0;
	for(i=0;i<10;i++)
	{
		if(stTpdu[i].port == gPort)
		{
			memcpy(szTpdu,stTpdu[i].tpdu,10);
			break;
		}

	}
	if(i>=10)
	{
		dcs_log(0,0,"port=[%d] not ture!",gPort);
		printf("port=[%d] not ture!",gPort);
		return 0;
	}
	i=0;

	dcs_log(0,0," POS消费撤销发送给前置 start");
	starttime= time((time_t*)NULL);
	currenttime = starttime;
	if(sendCycle == 0)
	{
		totaltime =100*24*60*60;//100天
	}
	else
	{
		//totaltime =sendCycle*60;
		totaltime =sendCycle;
	}

	while(currenttime-starttime<totaltime)//发送周期循环
	{
		seconds= time((time_t*)NULL);
		currentseconds=seconds;
    	for(sendnum=1;sendnum<=num;sendnum++)//每秒发送循环
	    {
	       	memset(&siso,0,sizeof(ISO_data));
			memset(tmpbuf,0,sizeof(tmpbuf));
			pack=(struct packBuf *)malloc(sizeof(struct packBuf));
			if(pack == NULL)
			{
				dcs_log(0, 0, "malloc error.");
				return -1;
			}
			setbit(&siso,0,(unsigned char *)"0200", 4);
			setbit(&siso,3,(unsigned char *)"200000", 6);
			setbit(&siso,4,(unsigned char *)"000000000001", 12);//原笔交易金额
			memset(tmpbuf, 0, sizeof(tmpbuf));
			time(&t);
    			tm = localtime(&t);
			sprintf(tmpbuf,"%02d%02d%02d%02d%02d",tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
			setbit(&siso,7,(unsigned char *)tmpbuf, 10);
			if(trace >999999)
				trace=1;
			memset(tmpbuf, 0, sizeof(tmpbuf));
			sprintf(tmpbuf,"%06d",trace);
			dcs_log(0,0,"trace=[%d],tpdu=[%s]",trace,szTpdu);//0,1,3,4,11,22,25,26,35,36,41,42,49,52,53,60,64
			setbit(&siso,11,(unsigned char *)tmpbuf, 6);//pos终端上送流水
			setbit(&siso,20,(unsigned char *)"00100", 5);
			//setbit(&siso,21,(unsigned char *)"14NP811081168453320000088800000697000001cc000000002089860114003100149051", 72);
			setbit(&siso,21,(unsigned char *)"14NP811081168453320000088800000697000001cc000000002089860114003100149051", 72);
			setbit(&siso,22,(unsigned char *)"022", 3);
			setbit(&siso,25,(unsigned char *)"00", 2);
			setbit(&siso,35,(unsigned char *)"6228412320121305911D49121207693980000", 37);
			setbit(&siso,36,(unsigned char *)"996228412320121305911D156156000000000000000000000011414144912DD000000000000D000000000000D030705800000000", 104);
			setbit(&siso,37,(unsigned char *)"182906154030",12); //原交易参考号
			setbit(&siso,38,(unsigned char *)"123456",6);
			setbit(&siso,41,(unsigned char *)"12345914",8);
			setbit(&siso,42,(unsigned char *)"999682907440095",15);
			setbit(&siso,49,(unsigned char *)"156",3);
			setbit(&siso,53,(unsigned char *)"0600000000000000" , 16);

		    setbit(&siso,60,(unsigned char *)"22000079000601",14);
		    setbit(&siso,61,(unsigned char *)"000079000001",12);//原授权码，原交参考号，原交易时间月日时分秒
			setbit( &siso, 64,(unsigned char *)"00000000000", 8 );

			memset(buffer, 0, sizeof(buffer));
			s = iso_to_str( (unsigned char *)buffer, &siso, 0);
			if ( s < 0 )
			{
				dcs_log(0, 0, "iso_to_str error.");
				return -1;
			}
			memset(newbuf, 0, sizeof(newbuf));
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy( newbuf, szTpdu,10);
			memcpy( newbuf+10, "602200000000",12 );
			asc_to_bcd( (unsigned char *) tmpbuf, (unsigned char*) newbuf, 22, 0 );
			memset(newbuf, 0, sizeof(newbuf));
			memcpy(newbuf,"11",2);//放报文长度
			memcpy( newbuf+2, tmpbuf, 11);
			sprintf(newbuf+13,"086001003021004611223344%04d",s);//添加报文头2
			memcpy( newbuf+41, buffer, s);
			s = s + 41;
			newbuf[0]=(s-2)/256;
			newbuf[1]=(s-2)%256;
			memcpy(pack->buf,newbuf,s);
			packLen =s;
			pack->trace=trace;
		 	trace++;

			pthread_t sthread;
			if ( pthread_create(&sthread, NULL,( void * (*)(void *))Makesocket, (void *)pack) !=0)
			{
			   dcs_log(0,0,"Create thread fail ");
			}
			pthread_detach(sthread);
			gSendNum ++;
	    }
		dcs_log(0,0,"端口%d本次send num=[%d]",gPort,sendnum-1);
		dcs_log(0,0,"端口%d总共send num=[%d]",gPort,gSendNum);
		currentseconds= time((time_t*)NULL);
		while(currentseconds==seconds)
		{
			u_sleep_us(100000);
			currentseconds= time((time_t*)NULL);
		}
		currenttime= time((time_t*)NULL);
	}

	dcs_log(0,0,"发送报文结束end");
	while(recvnum != gSendNum )
	{
		sleep(1);
	}
	exit(1);

}

/*
模拟余额查询交易
*/
int moniQuery()
{
 	ISO_data siso;
       struct  tm *tm;
	time_t  t;
	char    *ltime;
	char tmpbuf[40];
	int s = 0;
    	char	buffer[250], newbuf[250];
	int seconds=0,i=0;
	int currentseconds=0,sendnum=0;
	int trace=1;
	int starttime=0,currenttime=0;
	int totaltime=0;
	char szTpdu[11]={0};

	int pid,cpid;
	s = OpenLog();
	if(s < 0)
        return -1;
	gType =1;
	pid = fork();
	if(pid<0)
	{
		 dcs_log(0,0,"fork child err");
		 return -1;
	}
	else if(pid >0)
	{
		waitpid(pid, NULL, 0);
		 dcs_log(0,0,"fork child succ,pid=[%d]",pid);
		 return 0;
	}

	cpid = fork();
	if(cpid<0)
	{
		 dcs_log(0,0,"fork child err");
		 return -1;
	}
	else if(cpid >0)
	{
		 dcs_log(0,0,"fork child succ,cpid=[%d]",cpid);
		 exit(0);
	}
	struct packBuf *pack;
	if(IsoLoad8583config(&iso8583_conf[0],"iso8583_pos.conf") < 0)
   	 {
	        dcs_log(0,0,"IsoLoad8583config() failed:%s\n",strerror(errno));

	        printf("IsoLoad8583config failed \n");

	        return -1;
    	}

    	iso8583=&iso8583_conf[0];
  	SetIsoHeardFlag(0);
    	SetIsoFieldLengthFlag(0);
    	recvnum =0;
	gSendNum =0;
	for(i=0;i<10;i++)
	{
		if(stTpdu[i].port == gPort)
		{
			memcpy(szTpdu,stTpdu[i].tpdu,10);
			break;
		}

	}
	if(i>=10)
	{
		dcs_log(0,0,"port=[%d] not ture!",gPort);
		printf("port=[%d] not ture!",gPort);
		return 0;
	}
	i=0;
	dcs_log(0,0," 模拟POS余额查询交易发送给前置 start");//0,1,3,4,11,22,25,26,35,36,41,42,49,52,53,60,64
	starttime= time((time_t*)NULL);
	currenttime = starttime;
	if(sendCycle == 0)
	{
		totaltime =100*24*60*60;//100天
	}
	else
	{
		//totaltime =sendCycle*60;
		totaltime =sendCycle;
	}

	while(currenttime-starttime<totaltime)//发送周期循环
	{
		seconds= time((time_t*)NULL);
		currentseconds=seconds;
    		for(sendnum=1;sendnum<=num;sendnum++)//每秒发送循环
	       {
	       	memset(&siso,0,sizeof(ISO_data));
			memset(tmpbuf,0,sizeof(tmpbuf));
			pack=(struct packBuf *)malloc(sizeof(struct packBuf));
			if(pack == NULL)
			{
				dcs_log(0, 0, "malloc error.");
				return -1;
			}
			setbit(&siso,0,(unsigned char *)"0200", 4);
			setbit(&siso,3,(unsigned char *)"310000", 6);
			memset(tmpbuf, 0, sizeof(tmpbuf));
			time(&t);
    			tm = localtime(&t);
			sprintf(tmpbuf,"%02d%02d%02d%02d%02d",tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
			setbit(&siso,7,(unsigned char *)tmpbuf, 10);//测试需要
			if(trace >999999)
				trace=1;
			memset(tmpbuf, 0, sizeof(tmpbuf));
			sprintf(tmpbuf,"%06d",trace);
			dcs_log(0,0,"trace=[%d],tpdu=[%s]",trace,szTpdu);//0,1,3,4,11,22,25,26,35,36,41,42,49,52,53,60,64
			setbit(&siso,11,(unsigned char *)tmpbuf, 6);//pos终端上送流水
			setbit(&siso,20,(unsigned char *)"01000", 5);
			setbit(&siso,21,(unsigned char *)"14NP811081326349320000088800000697000001cc000000002089860114003100149051", 72);
			setbit(&siso,22,(unsigned char *)"022", 3);
			setbit(&siso,25,(unsigned char *)"00", 2);
			setbit(&siso,26,(unsigned char *)"06", 2);
			setbit(&siso,35,(unsigned char *)"6228412320121305911D49121207693980000", 37);
			setbit(&siso,36,(unsigned char *)"996228412320121305911D156156000000000000000000000011414144912DD000000000000D000000000000D030705800000000", 104);
			setbit(&siso,41,(unsigned char *)"12347356",8);
			setbit(&siso,42,(unsigned char *)"999899898760004",15);
			setbit(&siso,49,(unsigned char *)"156",3);
			setbit(&siso,53,(unsigned char *)"0600000000000000" , 16);

		    	setbit(&siso,60,(unsigned char *)"22000004000601",14);
			setbit( &siso, 64,(unsigned char *)"00000000000", 8 );

			iso8583 =&iso8583_conf[0];
		  	SetIsoHeardFlag(0);
		   	 SetIsoFieldLengthFlag(0);

			memset(buffer, 0, sizeof(buffer));
			s = iso_to_str( (unsigned char *)buffer, &siso, 0);
			if ( s < 0 )
			{
				dcs_log(0, 0, "iso_to_str error.");
				return -1;
			}
			memset(newbuf, 0, sizeof(newbuf));
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy( newbuf, szTpdu,10);
			memcpy( newbuf+10, "602200000000",12 );
			asc_to_bcd( (unsigned char *) tmpbuf, (unsigned char*) newbuf, 22, 0 );
			memset(newbuf, 0, sizeof(newbuf));
			memcpy(newbuf,"11",2);//放报文长度
			memcpy( newbuf+2, tmpbuf, 11);
			sprintf(newbuf+13,"086001003021004611223344%04d",s);//添加报文头2
			memcpy( newbuf+41, buffer, s);
			s = s + 41;
			newbuf[0]=(s-2)/256;
			newbuf[1]=(s-2)%256;
			memcpy(pack->buf,newbuf,s);
			packLen =s;
			pack->trace=trace;
		 	trace++;

			 pthread_t sthread;
		        if ( pthread_create(&sthread, NULL,( void * (*)(void *))Makesocket, (void *)pack) !=0)
		        {
		           dcs_log(0,0,"Create thread fail ");
		        }
		        pthread_detach(sthread);
			gSendNum ++;
	       }
		dcs_log(0,0,"端口%d本次send num=[%d]",gPort,sendnum-1);
		dcs_log(0,0,"端口%d总共send num=[%d]",gPort,gSendNum);
		currentseconds= time((time_t*)NULL);
		while(currentseconds==seconds)
		{
			u_sleep_us(100000);
			currentseconds= time((time_t*)NULL);
		}
		currenttime= time((time_t*)NULL);
	}

	dcs_log(0,0,"发送报文结束end");

	while(recvnum != gSendNum )
	{
		sleep(1);
	}
	exit(1);

}


int iso_to_str( unsigned char * dstr , ISO_data * iso, int flag )
{
	int flaghead, flaglength, flagmap;
	int len, slen, i, s;
	char buffer[2048], tmpbuf[999], bcdbuf[999], newbuf[999];
	char map[25];
	char dispinfo[8192];

	iso8583=&iso8583_conf[0];
	flaghead = GetIsoHeardFlg();
	flaglength = GetFieldLeagthFlag();
	//dcs_log( 0, 0, "iso_to_str:flaghead=%d, flaglength=%d.", flaghead, flaglength );
	sprintf( dispinfo, "iso_to_str: flaghead=%d, flaglength=%d.\n", flaghead, flaglength );
	sprintf( dispinfo, "%s-------------------------------------------------------------------\n", dispinfo );
	flagmap = 0;
	memset( buffer, 0x0, sizeof(buffer) );
	memset( map, 0x0, sizeof( map ) );
	memset( tmpbuf, 0x0, sizeof( tmpbuf ) );
	s = getbit( iso, 0, (unsigned char*) tmpbuf );
	//dcs_debug( tmpbuf, s, "Get bit[0  ] len=%d.", s );
	sprintf( dispinfo, "%s[%3.3d] [%-23.23s] [%3.3d] [%s]\n", dispinfo, 0, iso8583name[0], s, tmpbuf );
	if ( s < 4 )
	{
		len = -1;
		goto isoend;
	}
	if ( flaghead == 1 )  ///不压缩
	{
		memcpy( dstr, tmpbuf, 4 );
		len = 4;
	} else	///压缩
	{
		memset( bcdbuf, 0x0, sizeof( bcdbuf ) );
		asc_to_bcd( (unsigned char*) bcdbuf, (unsigned char*) tmpbuf, 4, 0 );  //暂时不考虑左边加0的情况
		memcpy( dstr, bcdbuf, 2 );
		len = 2;
	}
	slen = 0;
	for ( i = 1; i < 192; i++ )
	{
		s = getbit( iso, i + 1, (unsigned char *)tmpbuf );
		if ( s > 0 )
		{
			tmpbuf[ s] = 0x0;
			if ( i == 63 || i == 127 || i == 191 )
			{
				memset( bcdbuf, 0x0, sizeof( bcdbuf ) );
				bcd_to_asc( (unsigned char*) bcdbuf, (unsigned char*) tmpbuf, s  * 2, 0 );
				sprintf( newbuf, bcdbuf );
			} else
			{
				memset( newbuf, 0x0, sizeof( newbuf ) );
				memcpy( newbuf, tmpbuf, s );
			}
			//if ( i == 34 || i == 35 )
			//{
			//	newbuf[19] = 0;
			//}
			if ( i == 51 )
			{
				memset( bcdbuf, 0x0, sizeof( bcdbuf ) );
				bcd_to_asc( (unsigned char*) bcdbuf, (unsigned char*) tmpbuf, s  * 2, 0 );
				sprintf( newbuf, bcdbuf );
				//newbuf[0] = 0;
			}
			sprintf( dispinfo, "%s[%3.3d] [%-23.23s] [%3.3d] [%1.1d] [%1.1d] [%s]\n", dispinfo, i + 1, iso8583name[i + 1], s, iso8583_conf[i].flag, iso8583_conf[i].type, newbuf );
			//dcs_debug( tmpbuf, s, "Get bit[%3.3d] len=%d. flag=%d, type=%d.", i + 1, s, iso8583_conf[i].flag, iso8583_conf[i].type );
			map[ i / 8 ] = map[ i / 8 ] | (0x80 >> ( i % 8 ) );
			if ( i > 64 )
			{
				map[0] = map[0] | 0x80;
				flagmap = 1;
			}
			if ( i > 128 )
			{
				map[8] = map[8] | 0x80;
				flagmap = 2;
			}
			if ( iso8583_conf[i].flag == 0 )	////固定长
			{
				if ( iso8583_conf[i].type == 0 )  ///不压缩
				{
					memcpy( buffer + slen, tmpbuf, s );
					slen += s;
					if ( iso8583_conf[i].len != s )	//长度不够(多或少）
					{
						slen += iso8583_conf[i].len - s;
					}
				} else	///压缩
				{
					memset( bcdbuf, 0x0, sizeof( bcdbuf ) );
					asc_to_bcd( (unsigned char*) bcdbuf, (unsigned char*) tmpbuf, s, 0 );  //暂时不考虑左边加0的情况
					s = (s + 1) / 2;
					memcpy( buffer + slen, bcdbuf, s );
					slen += s;
				}
			} else		////可变长
			{
				if ( iso8583_conf[i].flag == 1 ) //2位可变长
				{
					if ( flaglength == 1 )
					{
						sprintf( buffer + slen, "%02d", s );
						slen += 2;
					} else
					{
						buffer[slen] = (unsigned char)( (s / 10) *16 + s % 10);
						slen += 1;
					}
				} else
				{
					if ( flaglength == 1 )
					{
						sprintf( buffer + slen, "%03d", s );
						slen += 3;
					} else
					{
						buffer[slen] = (unsigned char)(s / 100) ;
						slen += 1;
						buffer[slen] = (unsigned char)( ((s % 100) / 10) *16 + s % 10);
						slen += 1;
					}
				}
				if ( iso8583_conf[i].type == 0 )  ///不压缩
				{
					memcpy( buffer + slen, tmpbuf, s );
					slen += s;
				} else	///压缩
				{
					memset( bcdbuf, 0x0, sizeof( bcdbuf ) );
					asc_to_bcd( (unsigned char*) bcdbuf, (unsigned char*) tmpbuf, s, 0 );  //暂时不考虑左边加0的情况
					s = (s + 1) / 2;
					memcpy( buffer + slen, bcdbuf, s );
					slen += s;
				}
			}
		} else if ( s < 0 )  ////getbit()
		{
			len = -1;
			goto isoend;
		}
	} // end of for
	memset( tmpbuf, 0x0, sizeof( tmpbuf ) );
	bcd_to_asc( (unsigned char *) tmpbuf, (unsigned char *)map, (flagmap + 1 ) * 8 * 2, 0 );
	sprintf( dispinfo, "%s[%3.3d] [%-23.23s] [%3.3d] [%s]\n", dispinfo, 1, iso8583name[1], (flagmap + 1 ) * 8, tmpbuf );
	//dcs_debug( map, (flagmap + 1) * 8, "Get bitmap len=%d.", (flagmap + 1) * 8 );
	memcpy( dstr + len, map, (flagmap + 1) * 8 );
	len += (flagmap + 1) * 8;
	memcpy( dstr + len, buffer, slen );
	len += slen;
isoend:
	sprintf( dispinfo, "%s-------------------------------------------------------------------", dispinfo );
	if ( flag )
		dcs_log( 0, 0, "%s", dispinfo );

	return len;
}

int str_to_iso( unsigned char * dstr, ISO_data * iso, int Len )
{
	int flaghead, flaglength, flagmap;
	int len, slen, i, s, j;
	char buffer[2048], tmpbuf[999], bcdbuf[999];
	char dispinfo[8192];
	unsigned char bit[193];
	unsigned char tmp;

	iso8583=&iso8583_conf[0];
	flaghead = GetIsoHeardFlg();
	flaglength = GetFieldLeagthFlag();
	sprintf( dispinfo, "str_to_iso:flaghead=%d, flaglength=%d.\n", flaghead, flaglength );
	sprintf( dispinfo, "%s-------------------------------------------------------------------\n", dispinfo );
	//dcs_log( 0, 0, "str_to_iso:flaghead=%d, flaglength=%d.", flaghead, flaglength );
	if ( Len < 10 )
		return -1;
	clearbit( iso );
	memset( bit, 0x0, sizeof(bit) );
	flagmap = 0;
	slen = 0;
	memset( tmpbuf, 0x0, sizeof( tmpbuf ) );
	if ( flaghead == 0 )	///压缩
	{
		bcd_to_asc( (unsigned char*) tmpbuf, (unsigned char*)dstr, 4, 0 );
		slen += 2;
	} else		///不压缩
	{
		memcpy( tmpbuf, dstr, 4 );
		slen += 4;
	}
	setbit( iso, 0, (unsigned char*) tmpbuf, 4 );
	//dcs_log( tmpbuf, 4, "bit_%-3.3d [%-3.3d]: %s", 0, 4, tmpbuf );
	sprintf( dispinfo, "%s[%3.3d] [%-23.23s] [%3.3d] [%s]\n", dispinfo, 0, iso8583name[0], 4, tmpbuf );
	if ( (dstr[slen] & 0x80) != 0 )
	{
		flagmap = 1;
		if ( (dstr[slen + 8] & 0x80) != 0 )
			flagmap = 2;
	}
	for ( i = 0; i <= flagmap; i++ )
	{
		for ( j = 1; j < 64; j++ )
		{
			bit[ i * 8 * 8 + j ] = (0x80 >> (j % 8) ) & dstr[ slen + i * 8 + j / 8 ];
		}
	}
	//dcs_log( dstr + slen, (flagmap + 1 ) * 8, "bit_%-3.3d [%-3.3d]", 1, (flagmap + 1 ) * 8 );
	memset( tmpbuf, 0x0, sizeof( tmpbuf ) );
	bcd_to_asc( (unsigned char *) tmpbuf, (unsigned char *)dstr + slen, (flagmap + 1 ) * 8 * 2, 0 );
	sprintf( dispinfo, "%s[%3.3d] [%-23.23s] [%3.3d] [%s]\n", dispinfo, 1, iso8583name[1], (flagmap + 1 ) * 8, tmpbuf );
	slen += (flagmap + 1 ) * 8;
	if ( slen > Len )
		return -1;

	for ( i = 1; i < (flagmap + 1 ) * 8 * 8; i++ )
	{
		if ( bit[i] == 0 )
			continue;
		//dcs_log( dstr + slen, 3, "Set bit[%d] slen=%d. flag=%d, type=%d.", i + 1, slen, iso8583_conf[i].flag, iso8583_conf[i].type );
		if ( iso8583_conf[i].flag == 0 )	////固定长
		{
			len = iso8583_conf[i].len;
		} else if ( iso8583_conf[i].flag == 1 )	////LLVAR可变长
		{
			if ( flaglength == 0 )		///长度压缩
			{
				tmp = dstr[slen];
				len = (tmp & 0xF0) / 16 * 10 + ( tmp & 0x0F );
				slen += 1;
			} else
			{
				sscanf( (const char *)dstr + slen, "%2d", &len );
				slen += 2;
			}
		} else		///LLLVAR可变长
		{
			if ( flaglength == 0 )		///长度压缩
			{
				tmp = dstr[slen];
				len = ( tmp & 0x0F ) * 100;
				tmp = dstr[slen + 1];
				len += (tmp & 0xF0) / 16 * 10 + ( tmp & 0x0F );
				slen += 2;
			} else
			{
				sscanf( (const char *) dstr + slen, "%3d", &len );
				slen += 3;
			}
		}
		//dcs_log( 0, 0, "slen=%d, len=%d.", slen, len );
		memset( tmpbuf, 0x0, sizeof(tmpbuf) );
		if ( iso8583_conf[i].type == 0 )  ///不压缩
		{
			memcpy( tmpbuf, dstr + slen, len );
			slen += len;
		} else	///压缩	暂不考虑左添加
		{
			memset( bcdbuf, 0x0, sizeof( bcdbuf ) );
			memcpy( bcdbuf, dstr + slen, (len + 1 ) / 2 );
			bcd_to_asc( (unsigned char*) tmpbuf, (unsigned char*) bcdbuf, ((len + 1 ) / 2) * 2, 0 );  //暂时不考虑左边加0的情况
			slen += (len + 1 ) / 2;
			tmpbuf[len] = 0x0;
			if ( i + 1 == 35 || i + 1 == 36 )
			{
				for ( s = 0; s < len; s ++ )
				{
					if ( tmpbuf[s] == 'D' )
						tmpbuf[s] = '=';
				}
			}
		}
		//dcs_log( tmpbuf, len, "bit_%-3.3d [%-3.3d]: %s", i + 1, len, tmpbuf );
		s = setbit( iso, i+1, (unsigned char *)tmpbuf, len );
		if ( slen > Len || s < 0 )
		{
			dcs_log( 0, 0, "Analyse Packet length error: bit[%d], len=%d. slen[%d] Len[%d] s[%d]", i + 1, len, slen, Len, s );
			dcs_log( 0, 0, "%s", dispinfo );
			return -1;
		}
		if ( i == 63 || i == 127 || i == 191 )
		{
			memset( bcdbuf, 0x0, sizeof( bcdbuf ) );
			bcd_to_asc( (unsigned char*) bcdbuf, (unsigned char*) tmpbuf, len  * 2, 0 );
			sprintf( tmpbuf, bcdbuf );
		}
		//if ( i == 34 || i == 35 )
		//{
		//	tmpbuf[19] = 0;
		//}
		if ( i == 51 )
		{
			if ( strlen(tmpbuf) == 8 ){	///压缩
				memset( bcdbuf, 0x0, sizeof( bcdbuf ) );
				bcd_to_asc( (unsigned char*) bcdbuf, (unsigned char*) tmpbuf, 16, 0 );
				sprintf( tmpbuf, bcdbuf );
			}
			//tmpbuf[0] = 0;
		}
		sprintf( dispinfo, "%s[%3.3d] [%-23.23s] [%3.3d] [%s]\n", dispinfo, i + 1, iso8583name[i + 1], len, tmpbuf );
	}
	//dcs_log( 0, 0, "bit_end" );
	sprintf( dispinfo, "%s-------------------------------------------------------------------", dispinfo );
	dcs_log( 0, 0, "%s", dispinfo );
	return 0;
}

int Makesocket(void *trace)
{
	int sockfd,len;
	char    recvline[350];
	struct sockaddr_in servaddr;
	struct packBuf pack;
	memset(&pack,0,sizeof(packBuf));
	memcpy(pack.buf,((struct packBuf*)trace)->buf,sizeof(pack.buf));
	pack.trace = ((struct packBuf*)trace)->trace;
	free(trace);
	trace =NULL;
	int s;
	memset(recvline,0,sizeof(recvline));
	if((sockfd = socket(AF_INET,SOCK_STREAM,0))<0)
	{
		 dcs_log(0,0,"create socket error: %s(errno: %d)", strerror(errno),errno);
		 gSendNum--;
    		return -1;
	}
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(gPort);
	if( inet_pton(AF_INET, gIp, &servaddr.sin_addr) <= 0)
	{
		dcs_log(0,0,"inet_pton error for %s",gIp);
		gSendNum--;
		return -1;
	}

	if( connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
	{
		dcs_log(0,0,"connect error: %s(errno: %d)",strerror(errno),errno);
		close(sockfd);
		gSendNum--;
		return -1;
	}
	//dcs_log(0,0,"send msg to server:");
	//dcs_debug( pack.buf, packLen, "send msg:len=%d",packLen);

	if( send(sockfd, pack.buf, packLen, 0) < 0)
	{
		dcs_log(0,0,"send msg error: %s(errno: %d)", strerror(errno), errno);
		close(sockfd);
		gSendNum--;
		return -1;
	}
	//printf("send msg to server: succ\n");
	dcs_debug(pack.buf, packLen,"send msg to server succ: len=%d,trace[%d]",packLen,pack.trace);
	if((len=recv(sockfd, recvline, sizeof(recvline), MSG_WAITALL)) <0)
	{
		dcs_log(0,0,"recv msg error: %s(errno: %d)", strerror(errno), errno);
		close(sockfd);
		return -1;
	}
	if(len == 0)
	{
		dcs_log(0,0,"recv msg from server: err,链接终止trace[%d]",pack.trace);
		close(sockfd);
		return 0;
	}
	s=Unpackpos(recvline,len);
	if(s!= pack.trace)
	{
		dcs_log(0,0,"返回流水不符返回流水=[%d],发送流水=[%d]",s,pack.trace);
	}
	else
	{
		dcs_log(0,0,"返回流水相同=[%d]",s);
	}
	recvnum++;
	close(sockfd);
	return 0;
}

int SetPara()
{
	printf("当前设置:发送周期%d秒,每秒发送%d笔.\n",sendCycle,num);
	printf("请输入参数1,每秒发送报文个数:");
	scanf("%d",&num);
	printf("请输入参数2,发送周期(以秒为单位):");
	scanf("%d",&sendCycle);
//	printf("请输入参数3,接收端口:");
	//scanf("%d",&gPort);
	return 0;
}

int Unpackpos(char *srcBuf,int len)
{
	ISO_data siso;
	char szTrance[6+1];
	memset(szTrance,0,sizeof(szTrance));
	memset(&siso,0,sizeof(ISO_data));
	if(IsoLoad8583config(&iso8583_conf[0], "iso8583_pos.conf") < 0)
	{
		dcs_log(0, 0, "load iso8583_pos failed:%s", strerror(errno));
		return -1;
	}

	iso8583=&iso8583_conf[0];
	SetIsoHeardFlag(0);
	SetIsoFieldLengthFlag(0);

	if(str_to_iso((unsigned char *)srcBuf+41, &siso, len-41)<0)
	{
		dcs_log(0, 0, "str_to_iso error ");
		return -1;
	}

//	//1.测试8583结构和xml格式的转换函数
//	char xmlbuf[4048];
//	int ret =0;
//	memset(xmlbuf,0,sizeof(xmlbuf));
//	ret = iso_to_xml(&siso,xmlbuf );
//	if ( ret < 0 )
//	{
//		dcs_log(0, 0, "iso_to_xml error.");
//		return -1;
//	}
//	dcs_log(0, 0, "iso_to_xml [%s]",xmlbuf);

	getbit(&siso,11,szTrance);

	return atoi(szTrance);
 }

/*
模拟POS消费交易,长链
*/
int PosLongLianSale()
{
 	ISO_data siso;
       struct  tm *tm;
	time_t  t;
	char    *ltime;
	char tmpbuf[40];
	int s = 0;
    	char	buffer[250], newbuf[250];
	int seconds=0,i=0;
	int currentseconds=0,sendnum=0;
	int trace=0;
	int pid,fd=0;
	s = OpenLog();
	if(s < 0)
        return -1;

	fd=Longsocket();
	if(fd <0)
		return -1;
	pid = fork();
	if(pid<0)
	{
		 dcs_log(0,0,"fork child err");
		 return -1;
	}
	else if(pid ==0)
	{
		recvLOng(fd);
		return 0;
	}
	pid = fork();
	if(pid<0)
	{
		 dcs_log(0,0,"fork child err");
		 return -1;
	}
	else if(pid >0)
	{
		dcs_log(0,0,"fork child succ");
		return 0;
	}
	if(IsoLoad8583config(&iso8583_conf[0],"iso8583_pos.conf") < 0)
   	 {
	        dcs_log(0,0,"IsoLoad8583config() failed:%s\n",strerror(errno));

	        printf("IsoLoad8583config failed \n");

	        return -1;
    	}

    	iso8583=&iso8583_conf[0];
  	SetIsoHeardFlag(0);
    	SetIsoFieldLengthFlag(0);
    	recvnum =0;
	time(&t);
    	tm = localtime(&t);
   	ltime = ctime(&t);

    	sprintf(tmpbuf,"000000");
    	memcpy(tmpbuf+6, ltime+11, 2);
    	memcpy(tmpbuf+8, ltime+14, 2);
    	memcpy(tmpbuf+10, ltime+17, 2);

	i=0;
	dcs_log(0,0,"datatime=%s", tmpbuf);


	//printf("just a test\n");
	dcs_log(0,0," 模拟POS消费交易发送给前置 start");

	while(1)
	{
		sendnum=1;
		seconds= time((time_t*)NULL);
		currentseconds=seconds;
    		while(currentseconds <=seconds+1 && sendnum <=num)
	       {

	       	memset(&siso,0,sizeof(ISO_data));
			memset(tmpbuf,0,sizeof(tmpbuf));

			setbit(&siso,0,(unsigned char *)"0200", 4);
			setbit(&siso,3,(unsigned char *)"000000", 6);
			setbit(&siso,4,(unsigned char *)"000000800000", 12);//消费金额
			sprintf(tmpbuf,"%06d",trace);
			dcs_log(0,0,"trace=[%d]",trace);
			setbit(&siso,11,(unsigned char *)tmpbuf, 6);//pos终端上送流水
			setbit(&siso,20,(unsigned char *)"00000", 5);
			setbit(&siso,21,(unsigned char *)"22NP811071033395210248790000", 28);
			setbit(&siso,22,(unsigned char *)"022", 3);
			setbit(&siso,25,(unsigned char *)"00", 2);
			setbit(&siso,35,(unsigned char *)"6228210660015825514D49121206363770000", 37);
			setbit(&siso,36,(unsigned char *)"996228210660015825514D156156000000000000000000000011414144912DD000000000000D000000000000D095302700000000", 104);
			setbit(&siso,41,(unsigned char *)"12346427",8);
			setbit(&siso,42,(unsigned char *)"999899848110001",15);
			sprintf( tmpbuf, "0600000000000000" );
			setbit(&siso,53,(unsigned char *)tmpbuf, strlen(tmpbuf) );

			setbit(&siso,49,(unsigned char *)"156",3);
		    	setbit(&siso,60,(unsigned char *)"22000013000601",14);
			setbit( &siso, 64,(unsigned char *)"00000000000", 8 );

			iso8583 =&iso8583_conf[0];
		  	SetIsoHeardFlag(0);
		   	 SetIsoFieldLengthFlag(0);

			memset(buffer, 0, sizeof(buffer));
			s = iso_to_str( (unsigned char *)buffer, &siso, 1);
			if ( s < 0 )
			{
				dcs_log(0, 0, "iso_to_str error.");
				return -1;
			}
			memset(newbuf, 0, sizeof(newbuf));
			memset(tmpbuf, 0, sizeof(tmpbuf));
			memcpy( newbuf, "6000050000",10);
			memcpy( newbuf+10, "602200000000",12 );
			asc_to_bcd( (unsigned char *) tmpbuf, (unsigned char*) newbuf, 22, 0 );
			memset(newbuf, 0, sizeof(newbuf));
			//memcpy(newbuf,"TPOS",4);//pos发起
			memcpy(newbuf,"11",2);//放报文长度
			memcpy( newbuf+2, tmpbuf, 11);
			sprintf(newbuf+13,"086001003021004611223344%04d",s);//添加报文头2
			memcpy( newbuf+41, buffer, s);
			s = s + 41;
			newbuf[0]=(s-2)/256;
			newbuf[1]=(s-2)%256;
		 	trace++;
			if( send(fd, newbuf, s, 0) < 0)
			{
				dcs_log(0,0,"send msg error: %s(errno: %d)", strerror(errno), errno);
				close(fd);
				return -1;
			}
			dcs_log(0,0,"send msg to server succ: trace[%d]",trace);
			sendnum++;
			i++;
			gSendNum ++;
	       	currentseconds= time((time_t*)NULL);
	       }
		dcs_log(0,0,"pos本次send num=[%d]",sendnum-1);
		dcs_log(0,0,"pos总共send num=[%d]",gSendNum);
		sleep(sendCycle);//睡一会，再发
	}
	return 0;

}

int Longsocket(void)
{
	int sockfd=0,len;
	//int gPort=8888;
	char *ip="192.168.20.93";
	struct sockaddr_in servaddr;
	int s;
	if((sockfd = socket(AF_INET,SOCK_STREAM,0))<0)
	{
		 dcs_log(0,0,"create socket error: %s(errno: %d)", strerror(errno),errno);
		 gSendNum--;
    		return -1;
	}
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(gPort);
	if( inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0)
	{
		dcs_log(0,0,"inet_pton error for %s",ip);
		gSendNum--;
		return -1;
	}
       while(1)
       {
       	 if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))>=0)
		 	break;
       }
	dcs_log(0,0,"create socket succ: %d", sockfd);
	return sockfd;
}

int recvLOng(int fd)
{
	char recvline[500];
	int len;

     int   arg=1;
    int   sock;
    struct sockaddr_in	sin;
    struct sockaddr_in cliaddr;
    int  addr_len;
    int  conn_sock;

    //create the socket
    sock=socket(AF_INET,SOCK_STREAM,0);
    if(sock < 0)
	return -1;

    //get the IP address and port#
    memset(&sin,0,sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port   = htons(gPort);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);


    //set option for socket and bind the (IP,port#) with it
    arg=1;
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char *)&arg,sizeof(arg));
    if(bind(sock,(struct sockaddr *)&sin,sizeof(sin)) < 0)
	close(sock);

    //put the socket into passive open status
    if(listen(sock,5) < 0)
	close(sock);


    for(;;)
    {
        //try accepting a connection request from clients
		dcs_log(0, 0, "hst.....dcssock....tcp_accept_client...before, listen_sock = %d", sock);
        addr_len  = sizeof(cliaddr);
        conn_sock = accept(sock, (struct sockaddr *)&cliaddr, &addr_len);
		dcs_log(0, 0, "hst.....dcssock....tcp_accept_client...before, listen_sock = [%d], conn_sock = [%d]", sock, conn_sock);

        if(conn_sock >=0)
            break;
        if(conn_sock < 0 && errno == EINTR)
            continue;
        else
            return -1;
    }
	close(sock);//关闭listen
	while(1)
	{
		memset(recvline,0,sizeof(recvline));
		dcs_log( 0, 0, "wait recv....,");
		//tcp_check_readable(fd,-1);
		if((len=recv(conn_sock, recvline, sizeof(recvline), 0)) <0)
		{
			dcs_log(0,0,"recv msg error: %s(errno: %d)", strerror(errno), errno);
			close(conn_sock);
			return -1;
		}
		if(len == 0)
		{
			//printf("recv msg from server: err,链接终止\n");
			dcs_log(0,0,"recv msg from server: err,链接终止");
			close(conn_sock);
			return -1;
		}
		dcs_debug( recvline, len, "recvbuffer len:%d,", len);
	}

}

int recvLOng3()
{
	char recvline[500];
	int len;

     int   arg=1;
    int   sock;
    struct sockaddr_in	sin;
    struct sockaddr_in cliaddr;
    int  addr_len;
    int  conn_sock;

    //create the socket
    sock=socket(AF_INET,SOCK_STREAM,0);
    if(sock < 0)
	return -1;

    //get the IP address and port#
    memset(&sin,0,sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port   = htons(gPort);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);


    //set option for socket and bind the (IP,port#) with it
    arg=1;
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char *)&arg,sizeof(arg));
    if(bind(sock,(struct sockaddr *)&sin,sizeof(sin)) < 0)
	close(sock);

    //put the socket into passive open status
    if(listen(sock,5) < 0)
	close(sock);


    for(;;)
    {
        //try accepting a connection request from clients
		dcs_log(0, 0, "hst.....dcssock....tcp_accept_client...before, listen_sock = %d", sock);
        addr_len  = sizeof(cliaddr);
        conn_sock = accept(sock, (struct sockaddr *)&cliaddr, &addr_len);
		dcs_log(0, 0, "hst.....dcssock....tcp_accept_client...before, listen_sock = [%d], conn_sock = [%d]", sock, conn_sock);

        if(conn_sock >=0)
            break;
        if(conn_sock < 0 && errno == EINTR)
            continue;
        else
            return -1;
    }
	close(sock);//关闭listen
	while(1)
	{
		memset(recvline,0,sizeof(recvline));
		dcs_log( 0, 0, "wait recv....,");
		//tcp_check_readable(fd,-1);
		if((len=recv(conn_sock, recvline, sizeof(recvline), 0)) <0)
		{
			dcs_log(0,0,"recv msg error: %s(errno: %d)", strerror(errno), errno);
			close(conn_sock);
			return -1;
		}
		if(len == 0)
		{
			//printf("recv msg from server: err,链接终止\n");
			dcs_log(0,0,"recv msg from server: err,链接终止");
			close(conn_sock);
			return -1;
		}
		dcs_debug( recvline, len, "recvbuffer len:%d,", len);
		if( send(conn_sock, "98765", 5, 0) < 0)
		{
			dcs_log(0,0,"send msg error: %s(errno: %d)", strerror(errno), errno);
			close(conn_sock);
			return -1;
		}
		//printf("send msg to server: succ\n");
		dcs_log(0,0,"send msg to server succ: recvline[98765]");
	}

}

int recvLOng2(int fd)
{
	char recvline[500];
	int len;
	while(1)
	{
		memset(recvline,0,sizeof(recvline));
		dcs_log( 0, 0, "wait recv....,");
		//tcp_check_readable(fd,-1);
		if((len=recv(fd, recvline, sizeof(recvline), 0)) <0)
		{
			dcs_log(0,0,"recv msg error: %s(errno: %d)", strerror(errno), errno);
			close(fd);
			return -1;
		}
		if(len == 0)
		{
			//printf("recv msg from server: err,链接终止\n");
			dcs_log(0,0,"recv msg from server: err,链接终止");
			close(fd);
			return -1;
		}
		dcs_debug( recvline, len, "recvbuffer len:%d,", len);
	}

}

/*
模拟EWP消费交易,长链
*/
int EwpLongLianSale()
{
        struct  tm *tm;
	time_t  t;
	char    *ltime;
	char tmpbuf[250];
	int s = 0;
    	char	buffer[550], newbuf[550];
	int seconds=0,i=0;
	int currentseconds=0,sendnum=0;
	int starttime=0,currenttime=0;
 	int pid,fd=0;
 	int trace=0;
	s = OpenLog();
	if(s < 0)
        return -1;

	fd=Longsocket();
	if(fd <0)
		return -1;
#if 1
	pid = fork();
	if(pid<0)
	{
		 dcs_log(0,0,"fork child err");
		 return -1;
	}
	else if(pid ==0)
	{
		recvLOng(fd);
		return 0;
	}
#endif
    	recvnum =0;
	gSendNum =0;
	time(&t);
    	tm = localtime(&t);
   	ltime = ctime(&t);

    	sprintf(tmpbuf,"000000");
    	memcpy(tmpbuf+6, ltime+11, 2);
    	memcpy(tmpbuf+8, ltime+14, 2);
    	memcpy(tmpbuf+10, ltime+17, 2);

	dcs_log(0,0,"start datatime=%s", tmpbuf);
	dcs_log(0,0," 模拟ewp消费交易发送给前置 start");//0,1,3,4,11,22,25,26,35,36,41,42,49,52,53,60,64
	starttime= time((time_t*)NULL);
	currenttime = starttime;
	while(currenttime-starttime<sendCycle)//发送周期循环
	{
		time(&t);
    		tm = localtime(&t);
		seconds= time((time_t*)NULL);
		currentseconds=seconds;
    		for(sendnum=1;sendnum<=num;sendnum++)//每秒发送循环
	       {
			memset(buffer,0,sizeof(buffer));
                     trace++;
			if(trace >=99999999)
				trace =0;
			sprintf( buffer, "%s", "0200");   /*4个字节的交易类型数据*/
			sprintf( buffer, "%s%s",buffer, "000000");   /*6个字节的交易处理码*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			sprintf(tmpbuf,"%04d%02d%02d%02d%02d%02d",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
			sprintf( buffer, "%s%s",buffer, tmpbuf);   /*14个字节  接入系统发送交易的日期时间*/
			sprintf( buffer, "%s%s",buffer, "0100");   /*4个字节  接入系统的编号EWP：0100 sk EWP 0400*/
			sprintf( buffer, "%s%s",buffer, "102");   /*3个字节  交易码 区分业务类型订单支付：210  200老板收款*/

			sprintf( buffer, "%s%s" ,buffer, "1");/*交易发起方式 1:pos支付 skEWP: 2*/
			/*16个字节刷卡设备编号*/
			sprintf( buffer,"%s%s",buffer,"CADE000012345678");
			sprintf( buffer, "%s%-20s", buffer, "A00000228867EF");//机具编号 20左对齐

			sprintf( buffer,"%s%s",buffer,"15026513236");/*11个字节 手机号，例如：15026513236*/

			memset(tmpbuf,0,sizeof(tmpbuf));
			sprintf(tmpbuf,"%s%02d%02d%08d","ORDERNO",tm->tm_mon+1,tm->tm_mday,trace);
			sprintf(buffer,"%s%-20s",buffer,tmpbuf);

			/*20个字节订单号 左对齐 不足补空格*/
 			sprintf(buffer,"%s%020d",buffer,524);/*20个字节商户编号 不足左补空格*/

			sprintf(buffer,"%s%-19s",buffer,"6222021001098550814");/*19个字节卡号*/
 			sprintf(buffer,"%s%012d",buffer,20);/*12个字节 金额 右对齐 不足补0*/
 			sprintf(buffer,"%s%012d",buffer,"200");/*12个字节 优惠金额 右对齐 不足0*/
			sprintf( buffer,"%s%-160s",buffer,"D7E2636B605A14278596BCD7877B19DFA2309AF2141C8AE42466118C25D54E548E3B29E20C2C821748BB41114500397F1B41EF750A050DFE9C9D9BA81764B25625DD7C5D3AB2D8B2C4145B4825E8EEF1");
			sprintf( buffer,"%s%s",buffer,"4321432143214321");/*16个字节的磁道密钥随机数*/
			sprintf( buffer,"%s%s",buffer,"DF3C3D60764B2BF6");
			sprintf( buffer,"%s%s",buffer,"1234123412341234");/*16个字节pin密钥随机数*/
			sprintf( buffer,"%s%s",buffer,"0");/*是否发短信的标记*/

			sprintf( buffer,"%s%s",buffer,"selfdefineselfdefine");/*20个字节的自定义域*/
			memset(newbuf,0,sizeof(newbuf));
			s= strlen(buffer);
			memcpy( newbuf+2, buffer, s);
			s=s+2;
			newbuf[0]=(s-2)/256;
			newbuf[1]=(s-2)%256;
			if( send(fd, newbuf, s, 0) < 0)
			{
				dcs_log(0,0,"send msg error: %s(errno: %d)", strerror(errno), errno);
				close(fd);
				return -1;
			}
			dcs_log(0,0,"send msg to server succ: len[%d]",s );
			gSendNum ++;
			currentseconds= time((time_t*)NULL);
			if(currentseconds>seconds)
				break;
	       }
		dcs_log(0,0,"端口%d本次send num=[%d]",gPort,sendnum-1);
		dcs_log(0,0,"端口%d总共send num=[%d]",gPort,gSendNum);
		currentseconds= time((time_t*)NULL);
		while(currentseconds==seconds)
		{
			u_sleep_us(100000);
			currentseconds= time((time_t*)NULL);
		}
		currenttime= time((time_t*)NULL);
	}

	time(&t);
    	tm = localtime(&t);
   	ltime = ctime(&t);

    	sprintf(tmpbuf,"000000");
    	memcpy(tmpbuf+6, ltime+11, 2);
    	memcpy(tmpbuf+8, ltime+14, 2);
    	memcpy(tmpbuf+10, ltime+17, 2);

	dcs_log(0,0,"end datatime=%s", tmpbuf);
	return 0;

}


/*
模拟EWP消费交易,短链
*/
int EwpSale()
{
       struct  tm *tm;
	time_t  t;
	char    *ltime;
	char tmpbuf[40];
	int s = 0;
    	char	buffer[500], newbuf[500];
	int seconds=0,i=0;
	int currentseconds=0,sendnum=0;
	int trace=0;
	int starttime=0,currenttime=0;
	int totaltime=0;
	char szTpdu[11]={0};

	int pid,cpid;
	s = OpenLog();
	if(s < 0)
        return -1;
	gType =2;
	pid = fork();
	if(pid<0)
	{
		 dcs_log(0,0,"fork child err");
		 return -1;
	}
	else if(pid >0)
	{
		waitpid(pid, NULL, 0);
		 dcs_log(0,0,"fork child succ,pid=[%d]",pid);
		 return 0;
	}

	cpid = fork();
	if(cpid<0)
	{
		 dcs_log(0,0,"fork child err");
		 return -1;
	}
	else if(cpid >0)
	{
		 dcs_log(0,0,"fork child succ,cpid=[%d]",cpid);
		 exit(0);
	}
	struct packBuf *pack;
    	recvnum =0;
	gSendNum =0;
	for(i=0;i<10;i++)
	{
		if(stTpdu[i].port == gPort)
		{
			memcpy(szTpdu,stTpdu[i].tpdu,10);
			break;
		}

	}
	if(i>=10)
	{
		dcs_log(0,0,"port=[%d] not ture!",gPort);
		printf("port=[%d] not ture!",gPort);
		return 0;
	}
	i=0;
	time(&t);
    	tm = localtime(&t);
   	ltime = ctime(&t);
	memset(tmpbuf, 0, sizeof(tmpbuf));
    	sprintf(tmpbuf,"000000");
    	memcpy(tmpbuf+6, ltime+11, 2);
    	memcpy(tmpbuf+8, ltime+14, 2);
    	memcpy(tmpbuf+10, ltime+17, 2);

	dcs_log(0,0,"start datatime=%s", tmpbuf);


	//printf("just a test\n");
	dcs_log(0,0," 模拟ewp消费交易发送给前置 start");//0,1,3,4,11,22,25,26,35,36,41,42,49,52,53,60,64
	starttime= time((time_t*)NULL);
	currenttime = starttime;
	if(sendCycle == 0)
	{
		totaltime =100*24*60*60;//100天
	}
	else
	{
		//totaltime =sendCycle*60;
		totaltime =sendCycle;
	}

	while(currenttime-starttime<totaltime)//发送周期循环
	{
		seconds= time((time_t*)NULL);
		currentseconds=seconds;
    		for(sendnum=1;sendnum<=num;sendnum++)//每秒发送循环
	       {
	       	memset(buffer,0,sizeof(buffer));
			memcpy( buffer,  "0200",4);   /*4个字节的交易类型数据*/
			sprintf( buffer, "%s%s",buffer, "000000");   /*6个字节的交易处理码*/
			memset(tmpbuf,0,sizeof(tmpbuf));
			time(&t);
    			tm = localtime(&t);
			sprintf(tmpbuf,"%04d%02d%02d%02d%02d%02d",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
			sprintf( buffer, "%s%s",buffer, tmpbuf);   /*14个字节  接入系统发送交易的日期时间*/
			sprintf( buffer, "%s%s",buffer, "0500");   /*4个字节  接入系统的编号EWP：0100 sk EWP 0400*/
			sprintf( buffer, "%s%s",buffer, "102");   /*3个字节  交易码 区分业务类型订单支付：210  200老板收款*/

			sprintf( buffer, "%s%s" ,buffer, "1");/*交易发起方式 1:pos支付 skEWP: 2*/
			/*16个字节刷卡设备编号*/
			sprintf( buffer,"%s%s",buffer,"CADE000012345678");
			sprintf( buffer, "%s%-20s", buffer, "A00000228867EF");//机具编号 20左对齐

			sprintf( buffer,"%s%s",buffer,"cz000022500");/*11个字节 手机号，用户名例如：15026513236*/

			memset(tmpbuf,0,sizeof(tmpbuf));
			sprintf(tmpbuf,"%s%02d%02d%08d","ORDERNO",tm->tm_mon+1,tm->tm_min,tm->tm_sec);
			sprintf(buffer,"%s%-20s",buffer,tmpbuf);

			/*20个字节订单号 左对齐 不足补空格*/
				sprintf(buffer,"%s%020d",buffer,524);/*20个字节商户编号 不足左补空格*/

			sprintf(buffer,"%s%-19s",buffer,"6222021001098550814");/*19个字节卡号*/
				sprintf(buffer,"%s%012d",buffer,20);/*12个字节 金额 右对齐 不足补0*/
				sprintf(buffer,"%s%012d",buffer,"200");/*12个字节 优惠金额 右对齐 不足0*/
			sprintf( buffer,"%s%s",buffer,"D7E2636B605A14278596BCD7877B19DFA2309AF2141C8AE42466118C25D54E548E3B29E20C2C821748BB41114500397F1B41EF750A050DFE9C9D9BA81764B25625DD7C5D3AB2D8B2C4145B4825E8EEF1");
			sprintf( buffer,"%s%s",buffer,"4321432143214321");/*16个字节的磁道密钥随机数*/
			sprintf( buffer,"%s%s",buffer,"DF3C3D60764B2BF6");
			sprintf( buffer,"%s%s",buffer,"1234123412341234");/*16个字节pin密钥随机数*/
			sprintf( buffer,"%s%s",buffer,"94AF4043");/*8个字节MAC值, 接入系统的编号为0400和0500时需要*/
			sprintf( buffer,"%s%s",buffer,"0");/*是否发短信的标记*/

			sprintf( buffer,"%s%s",buffer,"selfdefineselfdefine");/*20个字节的自定义域*/
			memset(newbuf,0,sizeof(newbuf));
			s= strlen(buffer);
			memset(tmpbuf,0,sizeof(tmpbuf));
			memcpy( tmpbuf,  szTpdu,10);   /*5个字节的tpdu*/
			asc_to_bcd( (unsigned char *) newbuf+2, (unsigned char*) tmpbuf, 10, 0 );
			memcpy( newbuf+7, buffer, s);

			s=s+7;
			newbuf[0]=(s-2)/256;
			newbuf[1]=(s-2)%256;
			dcs_debug(newbuf, s, "newbuf ,len=%d",s);
			pack =(struct packBuf *)malloc(sizeof(struct packBuf));
			if(pack ==NULL)
			{
				dcs_log(0, 0, "malloc error.");
						return -1;
			}
			memcpy(pack->buf,newbuf,s);
			packLen =s;
			pack->trace=12345;


			pthread_t sthread;
			if ( pthread_create(&sthread, NULL,( void * (*)(void *))Makesocket, (void *)pack) !=0)
			{
			   dcs_log(0,0,"Create thread fail ");
			}
			pthread_detach(sthread);
			i++;
			if(i>=MAX_PROCE)
			{
				i=0;
			}
			gSendNum ++;
	    }
		dcs_log(0,0,"端口%d本次send num=[%d]",gPort,sendnum-1);
		dcs_log(0,0,"端口%d总共send num=[%d]",gPort,gSendNum);
		currentseconds= time((time_t*)NULL);
		while(currentseconds==seconds)
		{
			u_sleep_us(100000);
			currentseconds= time((time_t*)NULL);
		}
		currenttime= time((time_t*)NULL);
	}

	time(&t);
    	//tm = localtime(&t);
   	ltime = ctime(&t);
	memset(tmpbuf, 0, sizeof(tmpbuf));
    	sprintf(tmpbuf,"000000");
    	memcpy(tmpbuf+6, ltime+11, 2);
    	memcpy(tmpbuf+8, ltime+14, 2);
    	memcpy(tmpbuf+10, ltime+17, 2);

	dcs_log(0,0,"end datatime=%s", tmpbuf);

	exit(1);

}

/*
模拟EWP转账汇款交易,写folder
*/
int EwpZhuanZhang()
{
	int s = 0;
    char	buffer[1000];
	int gs_myFid,fid,openfid;
	char tmpbuf[256];
	struct  tm *tm;   time_t  t;
	time(&t);
	tm = localtime(&t);
	int num;
	s = OpenLog();
	if(s < 0)
        return -1;


	gs_myFid = fold_create_folder("TRANSRCV");

	if(gs_myFid < 0)
		gs_myFid = fold_locate_folder("TRANSRCV");

	if(gs_myFid < 0)
	{
		dcs_log(0,0,"cannot create folder '%s':%s\n", "TRANSRCV", ise_strerror(errno) );
		return -1;
	}

	dcs_log(0,0," start");

	sprintf( buffer, "%s", "EWPP");   /*4个字节的报文类型数据*/
	sprintf( buffer, "%s%s",buffer, "12345");   /*5个字节的TPDU*/
	sprintf( buffer, "%s%s",buffer, "0200");   /*4个字节的交易类型数据*/
	sprintf( buffer, "%s%s",buffer, "480000");   /*6个字节的交易处理码*/
	sprintf(tmpbuf,"%04d%02d%02d",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday);
	sprintf( buffer, "%s%s",buffer, tmpbuf);   /*8个字节  接入系统发送交易的日期*/
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%02d%02d%02d",tm->tm_hour,tm->tm_min,tm->tm_sec);
	sprintf( buffer, "%s%s",buffer, tmpbuf);   /*6个字节  接入系统发送交易的时间*/
	sprintf( buffer, "%s%s",buffer, "0900");   /*4个字节  接入系统的编号EWP：0100 sk EWP 0400*/
	sprintf( buffer, "%s%s",buffer, "205");   /*转账汇款：205*/
	sprintf( buffer, "%s%s" ,buffer, "1");/*交易发起方式 1:pos支付 skEWP: 2*/
	//sprintf( buffer,"%s%s",buffer,"CADE000012345678");
	sprintf( buffer,"%s%s",buffer,"0DF0110030000002");
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%s%02d%02d%02d","MOBILEIMEI",tm->tm_hour,tm->tm_min,tm->tm_sec);
	sprintf(buffer,"%s%-20s",buffer,tmpbuf);/*20个字节 手机编号IMEI左对齐 不足补空格*/
	sprintf( buffer,"%s%s",buffer,"15026513236");/*11个字节 手机号，例如：15026513236*/
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%s%02d%02d%02d%02d%02d","ORDERNO",tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
	sprintf(buffer,"%s%-20s",buffer,tmpbuf);
	num=524;
	sprintf(buffer,"%s%020d",buffer,num);/*20个字节商户编号 不足左补空格*/
	sprintf(buffer, "%s%-19s", buffer, "621444100001003016");/*转入卡号*/
	sprintf(buffer, "%s%-19s", buffer, "621444100001003015");/*转出卡号*/
	sprintf(buffer, "%s%-50s", buffer, "吴振飞");/*转入卡姓名*/
	sprintf(buffer, "%s%-25s", buffer, "6214441000");/*转入卡行号*/
	num =200000;
	sprintf(buffer,"%s%012d",buffer,num);/*12个字节 金额 右对齐 不足补0*/
	num =200;
	sprintf(buffer,"%s%012d",buffer,"200");/*12个字节 优惠金额 右对齐 不足0*/
	//sprintf( buffer,"%s%-160s",buffer,"D7E2636B605A14278596BCD7877B19DFA2309AF2141C8AE42466118C25D54E548E3B29E20C2C821748BB41114500397F1B41EF750A050DFE9C9D9BA81764B25625DD7C5D3AB2D8B2C4145B4825E8EEF1");
	sprintf( buffer,"%s%-160s",buffer,"72FBA9BE5416224A3FC09548DEB57101");
	//sprintf( buffer,"%s%s",buffer,"4321432143214321");/*16个字节的磁道密钥随机数*/
	sprintf( buffer,"%s%s",buffer,"CD6FF07F2C11A821");/*16个字节的磁道密钥随机数*/
	//sprintf( buffer,"%s%s",buffer,"DF3C3D60764B2BF6");
	sprintf( buffer,"%s%s",buffer,"5BFDECA98E2DCE34");
	//sprintf( buffer,"%s%s",buffer,"1234123412341234");/*16个字节pin密钥随机数*/
	sprintf( buffer,"%s%s",buffer,"52CFAD07B4EEBB08");/*16个字节pin密钥随机数*/
	sprintf( buffer,"%s%s",buffer,"0");/*是否发短信的标记*/
	sprintf( buffer,"%s%s",buffer,"selfdefineselfdefine");/*20个字节的自定义域*/

	fid = fold_create_folder("MYFOLD2");
	if(fid < 0)
		fid = fold_locate_folder("MYFOLD2");
	if(fid < 0)
	{
		dcs_log(0,0,"cannot get folder '%s':%s\n", "MYFOLD2", ise_strerror(errno) );
		return -1;
	}

	s= strlen(buffer);
	fold_write(gs_myFid,fid, buffer, s );
	dcs_log( 0,0, "Write finished,Write length =%d",s);

	openfid= fold_open(fid);

	memset(buffer,0,sizeof(buffer));
	s = fold_read(openfid, openfid, &gs_myFid, buffer, sizeof(buffer),1);
	if(s <0 )
	{
		dcs_log(0,0, "read fold fail");
		return -1;
	}
	dcs_log( 0,0, "read finished,read length =%d,buf=[%s]",s,buffer);
	return 1;

}
int pos_ewp()
{
	int pid=0;
	pid =fork();
	if(pid <0)
	{
		printf("fork fail!");
		return -1;
	}
	if(pid ==0)
	{
		//子进程进行短链，pos 消费
		moniSale();
		return 0;
	}
	usleep(1000);
	//父进程进行长链,ewp消费
	gPort =19551;
	EwpLongLianSale();
	return 0;
}

int wbpos()
{
	int pid=0;
	int i=0;
	int sum=5;
	while(1)
	{
		printf("请输入参数1,测试端口数(最大为5):");
		scanf("%d",&sum);
		if(sum>=1&&sum<=5)
			break;
	}
	printf("请输入参数2,每个端口每秒发送报文个数:");
	scanf("%d",&num);
	printf("请输入参数3,发送周期(以分为单位):");
	scanf("%d",&sendCycle);
	for(i=0;i<sum;i++)
	{
		pid =fork();
		if(pid <0)
		{
			printf("fork fail!");
			return -1;
		}
		if(pid ==0)
		{
			gPort =stTpdu[i].port;
			//子进程进行短链，pos 消费
			moniSale();
			return 0;
		}
	}

	return 0;
}


int clientsocket()
{
	int sockfd,len;
	char    recvline[300];
	int port=8899;
	//char *ip="112.65.46.59";
	char *ip="192.168.20.159";
	struct sockaddr_in servaddr;
       // pack.buf= (struct packBuf*)trace;
	int s;
	   int pid =0;


	pid=fork();
	if(pid<0)
	{
		printf("fork fail!\n");
	}
	if(pid >0)
		return 0;
	  s = OpenLog1();
	if(s < 0)
		return -1;
	gPort =62222;
	memset(recvline,0,sizeof(recvline));
	if((sockfd = socket(AF_INET,SOCK_STREAM,0))<0)
	{
		 dcs_log(0,0,"create socket error: %s(errno: %d)", strerror(errno),errno);
    		return -1;
	}
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(gPort);
	if( inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0)
	{
		dcs_log(0,0,"inet_pton error for %s",ip);
		return -1;
	}

	if( connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
	{
		dcs_log(0,0,"connect error: %s(errno: %d)",strerror(errno),errno);
		close(sockfd);
		return -1;
	}
	//dcs_log(0,0,"send msg to server:");
	//dcs_debug( pack.buf, packLen, "send msg");
	dcs_log(0,0,"sleep.............");
	sleep(2*60*60);
	memcpy(recvline,"123456",6);
	if( send(sockfd, recvline, 6, 0) < 0)
	{
		dcs_log(0,0,"send msg error: %s(errno: %d)", strerror(errno), errno);
		close(sockfd);
		return -1;
	}
	//printf("send msg to server: succ\n");
	dcs_log(0,0,"send msg to server succ: recvline[%s]",recvline);
	if((len=recv(sockfd, recvline, sizeof(recvline), 0)) <0)
	{
		dcs_log(0,0,"recv msg error: %s(errno: %d)", strerror(errno), errno);
		close(sockfd);
		return -1;
	}
	if(len == 0)
	{
		//dcs_log(0,0,"recv msg from server: err,链接终止trace[%d]",pack.trace);
		close(sockfd);
		return 0;
	}
	dcs_debug( recvline, len, "recvbuffer len:%d,", len);
	//dcs_log(0,0,"recv msg from server succ len=[%d]",len); 
	close(sockfd);
	return 0;
}

int server()
{
	int pid =0;
	int s;
	s = OpenLog();
	if(s < 0)
		return -1;
	gPort =62222;
	pid=fork();
	if(pid<0)
	{
		printf("fork fail!\n");
	}
	if(pid ==0)
		recvLOng3();
	return 0;
}


int SendDataToFold( )
{
	char    tmpbuf[500];
	int	len, s,gs_myFid;
	s = OpenLog();
	if(s < 0)
        return -1;

	dcs_log(0,0, "send data to fold smsc !\n");
	memset(tmpbuf,0,sizeof(tmpbuf));

	//strcpy(tmpbuf,"MPOS1234506200000002015061714475506000413837323131000ba0004803375126            144752540561201506173069f26086642b6af647af0cd9f2701409f101307000103602010010a010000088800d44508009f3704600c987b9f36020b63950508800480009a031506179c01009f02060000000011005f2a02015682027c009f1a0201569f03060000000000009f33036040c89f34030203009f3501229f1e0830333337353132368408a0000003330101029f090200209f410400000000df31052000000000870e47fe000000000<0.14306.0>");
	strcpy(tmpbuf,"MPOS1234506200000002015061714475506000413837323131000ba0004803375126            144752540561201506171829F33036040C8950508800480009F3704600C987B9F1E0830333337353132369F101307000103602010010A010000088800D44508009F26084DFC07A61F3198589F36020B6382027C00DF310520000000009F1A0201569A03150617870e47fe000000000<0.14306.0>");
	s=strlen(tmpbuf);
	dcs_debug( tmpbuf, s, "buffer len:%d,", s);


	dcs_log(0,0, "attach to folder system ok !\n");

	gs_myFid = fold_locate_folder("TRANSRCV");

	if(gs_myFid < 0)
	{
		dcs_log(0,0,"cannot get folder '%s':%s\n", "TRANSRCV", ise_strerror(errno) );
		return -1;
	}

	dcs_log(0,0, "folder myFid=%d", gs_myFid);


	fold_write(gs_myFid,-1,tmpbuf, s );

	dcs_log( 0,0, "Write finished,Write length =%d",s);

	return 0;
}
int setServer()
{
	printf("当前请求的服务器:\n");
	printf("IP:%s,port:%d\n",gIp, gPort);
	printf("请输入发送IP:");
	scanf("%s",gIp);
	printf("\n请输入发送port:");
	scanf("%d",&gPort);
	return 0;
}


int moniFold( )
{
	char    tmpbuf[310];
	int	len, s,gs_myFid,fid,openfid,org_folderId,trace=1;
	int pid;
	char	buffer[500], newbuf[500], buf[6+1],timebuf[14+1];
	ISO_data siso;
	struct  tm *tm;
	time_t  t;

	time(&t);
    	tm = localtime(&t);
    	memset(timebuf, 0, sizeof(timebuf));
	sprintf(timebuf, "%04d%02d%02d%02d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	s = OpenLog();
	if(s < 0)
        return -1;
	if (fold_initsys() < 0)
	{
		dcs_log(0,0,"cannot attach to FOLDER !");
		return -1;
	}

	gs_myFid = fold_create_folder(FOLDNAME);

	if(gs_myFid < 0)
		gs_myFid = fold_locate_folder(FOLDNAME);

	if(gs_myFid < 0)
	{
		dcs_log(0,0,"cannot create folder '%s':%s\n", FOLDNAME, ise_strerror(errno) );
		return -1;
	}

	dcs_log(0,0, "folder myFid=%d\n", gs_myFid);


	if(IsoLoad8583config(&iso8583_conf[0],"iso8583_pos.conf") < 0)
   	 {
	        dcs_log(0,0,"IsoLoad8583config() failed:%s\n",strerror(errno));

	        printf("IsoLoad8583config failed \n");

	        return -1;
    	}
	iso8583=&iso8583_conf[0];
	SetIsoHeardFlag(0);
    	SetIsoFieldLengthFlag(0);
	pid = fork();
	if(pid <0)
	{
		dcs_log(0,0,"fork failed:%s\n",strerror(errno));
		return -1;
	}
	if(pid == 0)
	{
		//子进程读取返回报文
		//从myfold内读出内容
		openfid= fold_open(gs_myFid);
		trace = 1;
		while(trace <=num)
		{
			memset(tmpbuf,0,sizeof(tmpbuf));
			len = fold_read(gs_myFid, openfid, &org_folderId, tmpbuf, sizeof(tmpbuf),1);
			if(len <0 )
			{
				dcs_log(0,0, "read fold fail");
				return -1;
			}
			dcs_debug( tmpbuf, len, "read the buf len:%d,", len);
			trace++;
		}
		exit(0);
	}
		//向fold内写入内容
	dcs_log(0,0, "send data to fold %s !\n",FOLDNAME2);
	while(trace <=num)
	{
		memset(&siso,0,sizeof(ISO_data));
		memset(tmpbuf,0,sizeof(tmpbuf));
		//setbit(&siso,0,(unsigned char *)"0200", 4);
		setbit(&siso,0,(unsigned char *)"0400", 4);
		setbit(&siso,3,(unsigned char *)"000000", 6);
		setbit(&siso,4,(unsigned char *)"000000800000", 12);//消费金额
		//setbit(&siso,4,(unsigned char *)timebuf+2, 12);//消费金额
		memset(buf, 0 ,sizeof(buf));
		//sprintf(buf, "%6d",trace);
		sprintf(buf, "%6d",1);
		setbit(&siso,11,(unsigned char *)buf, 6);//pos终端上送流水
		setbit(&siso,20,(unsigned char *)"00000", 5);
		setbit(&siso,21,(unsigned char *)"22NP811071033395210248790000", 28);
		setbit(&siso,22,(unsigned char *)"022", 3);
		setbit(&siso,25,(unsigned char *)"00", 2);
		setbit(&siso,35,(unsigned char *)"6228210660015825514D49121206363770000", 37);
		setbit(&siso,36,(unsigned char *)"996228210660015825514D156156000000000000000000000011414144912DD000000000000D000000000000D095302700000000", 104);
		setbit(&siso,41,(unsigned char *)"12346427",8);
		setbit(&siso,42,(unsigned char *)"999899848110001",15);
		sprintf( tmpbuf, "0600000000000000" );
		setbit(&siso,53,(unsigned char *)tmpbuf, strlen(tmpbuf) );
		//模拟55域只为了报文更长,不保证55域有效
		memset(tmpbuf, 0, sizeof(tmpbuf));
		memset(newbuf, 0, sizeof(newbuf));
		sprintf( tmpbuf, "9f2608a8670404869b5a2d9f2701809f101307000103a0a000010a0100000888008d751c6c9f37043f3d0cf79f360206f2950508800488009a031505189c01309f02060000000000005f2a02015682027c009f1a0201569f03060000000000009f33036040c89f34034203009f3501229f1e0830333337353637328408a0000003330101019f090200209f410400000000" );
		asc_to_bcd( (unsigned char *) newbuf, (unsigned char*) tmpbuf, 290, 0 );
		setbit(&siso,55,(unsigned char *)newbuf,145);
		setbit(&siso,49,(unsigned char *)"156",3);
	    	setbit(&siso,60,(unsigned char *)"22000014000601",14);
		setbit( &siso, 64,(unsigned char *)"00000000000", 8 );
		memset(buffer, 0, sizeof(buffer));
		s = iso_to_str( (unsigned char *)buffer, &siso, 0);
		if ( s < 0 )
		{
			dcs_log(0, 0, "iso_to_str error.");
			return -1;
		}
		memset(newbuf, 0, sizeof(newbuf));
		memset(tmpbuf, 0, sizeof(tmpbuf));
#if 1  /*测试TCP短链客户端服务时 需要TPDU*/
		memcpy( newbuf, TPDU,10);
		memcpy( newbuf+10, "602200000000",12 );
		asc_to_bcd( (unsigned char *) tmpbuf, (unsigned char*) newbuf, 22, 0 );
		memset(newbuf, 0, sizeof(newbuf));
		memcpy(newbuf,"TPOS",4);
		memcpy( newbuf+4, tmpbuf, 11);
		sprintf(newbuf+15,"086001003021004611223344%04d",s);//添加报文头2
		memcpy( newbuf+43, buffer, s);
		s = s + 43;
#else    /*测试http服务服务时不 需要TPDU*/
		memcpy( newbuf, "602200000000",12 );
		asc_to_bcd( (unsigned char *) tmpbuf, (unsigned char*) newbuf, 12, 0 );
		memset(newbuf, 0, sizeof(newbuf));
		memcpy( newbuf, tmpbuf, 6);
		sprintf(newbuf+6,"086001003021004611223344%04d",s);//添加报文头2
		memcpy( newbuf+34, buffer, s);
		s = s + 34;
#endif

		dcs_debug( newbuf, s, "buffer len:%d,", s);


		dcs_log(0,0, "attach to folder system ok !\n");

		fid = fold_locate_folder(FOLDNAME2);

		if(fid < 0)
		{
			dcs_log(0,0,"cannot get folder '%s':%s\n", FOLDNAME2, ise_strerror(errno) );
			return -1;
		}

		fold_write(fid,gs_myFid,newbuf, s );
		trace ++;
		dcs_log( 0,0, "Write finished,Write length =%d",s);
	}
	return 0;
}

int SetFoldPara()
{
	printf("请输入要发送的报文个数:");
	scanf("%d",&num);
	printf("请输入要发送的报文的TPDU:");
	scanf("%s",&TPDU);
	printf("请输入要写入的FOLD名:");
	scanf("%s",&FOLDNAME2);

	return 0;
}
int monihttp( )
{
	char    tmpbuf[250];
	int	len, s,gs_myFid,fid,openfid,org_folderId,trace=1;
	int pid;
	char	buffer[250], newbuf[250], buf[6+1];
	ISO_data siso;
	s = OpenLog();
	if(s < 0)
        return -1;
	if (fold_initsys() < 0)
	{
		dcs_log(0,0,"cannot attach to FOLDER !");
		return -1;
	}

	gs_myFid = fold_create_folder(FOLDNAME);

	if(gs_myFid < 0)
		gs_myFid = fold_locate_folder(FOLDNAME);

	if(gs_myFid < 0)
	{
		dcs_log(0,0,"cannot create folder '%s':%s\n", FOLDNAME, ise_strerror(errno) );
		return -1;
	}

	dcs_log(0,0, "folder myFid=%d\n", gs_myFid);


	if(IsoLoad8583config(&iso8583_conf[0],"iso8583_pos.conf") < 0)
   	 {
	        dcs_log(0,0,"IsoLoad8583config() failed:%s\n",strerror(errno));

	        printf("IsoLoad8583config failed \n");

	        return -1;
    	}
	iso8583=&iso8583_conf[0];
	SetIsoHeardFlag(0);
    	SetIsoFieldLengthFlag(0);
		//向fold内写入内容
	dcs_log(0,0, "send data to fold %s !\n","HC001");
	while(trace <=num)
	{

		memset(newbuf,0,sizeof(newbuf));

		strcpy(newbuf,"xmlString=%3CprepositionHttpDemoReq%20xmlns=%22http://www.chinaebi.com.cn/webservice%22%3E%3Ctrace%3E1234567890%3C/trace%3E%3Ccontext%3Etest%3C/context%3E%3C/prepositionHttpDemoReq%3E");
		//strcpy(newbuf,"xmlString=<xpepModelChangeReq xmlns=%22http://www.pos.chinaebi.com.cn/webservice%22>%3E%3C<trace>201408290943054366</trace>%3E%3C</xpepModelChangeReq>");

		s= strlen(newbuf);
		dcs_debug( newbuf, s, "buffer len:%d,", s);


		dcs_log(0,0, "attach to folder system ok !\n");

		fid = fold_locate_folder("HC001");

		if(fid < 0)
		{
			dcs_log(0,0,"cannot get folder '%s':%s\n", "HC001", ise_strerror(errno) );
			return -1;
		}

		fold_write(fid,-1,newbuf, s );
		trace ++;
		dcs_log( 0,0, "Write finished,Write length =%d",s);
	}

	//从myfold内读出内容

	openfid= fold_open(gs_myFid);
	trace = 1;
	while(trace <=num)
	{
		memset(tmpbuf,0,sizeof(tmpbuf));
		len = fold_read(gs_myFid, openfid, &org_folderId, tmpbuf, sizeof(tmpbuf),1);
		if(len <0 )
		{
			dcs_log(0,0, "read fold fail");
			return -1;
		}
		dcs_log(0,0, "read the buf len:\n%s",tmpbuf);
		trace++;
	}
	return 0;
}


int monihttp2( )
{
	char    tmpbuf[440];
	int	len, s,gs_myFid,fid,openfid,org_folderId,trace=1;
	int pid;
	char	buffer[440], newbuf[440], buf[6+1];
	ISO_data siso;
	s = OpenLog();
	if(s < 0)
        return -1;
	if (fold_initsys() < 0)
	{
		dcs_log(0,0,"cannot attach to FOLDER !");
		return -1;
	}

	gs_myFid = fold_create_folder(FOLDNAME);

	if(gs_myFid < 0)
		gs_myFid = fold_locate_folder(FOLDNAME);

	if(gs_myFid < 0)
	{
		dcs_log(0,0,"cannot create folder '%s':%s\n", FOLDNAME, ise_strerror(errno) );
		return -1;
	}

	dcs_log(0,0, "folder myFid=%d\n", gs_myFid);


	if(IsoLoad8583config(&iso8583_conf[0],"iso8583_pos.conf") < 0)
   	 {
	        dcs_log(0,0,"IsoLoad8583config() failed:%s\n",strerror(errno));

	        printf("IsoLoad8583config failed \n");

	        return -1;
    	}
	iso8583=&iso8583_conf[0];
	SetIsoHeardFlag(0);
    	SetIsoFieldLengthFlag(0);
		//向fold内写入内容
	dcs_log(0,0, "send data to fold %s !\n","CL_TPOS_002");
	while(trace <=num)
	{

		memset(newbuf,0,sizeof(newbuf));

		strcpy(newbuf,"GET http://192.168.20.177:8080/signService/windControlService?xmlString=%3CprepositionHttpDemoReq%20xmlns=%22http://www.chinaebi.com.cn/webservice%22%3E%3Ctrace%3E1234567890%3C/trace%3E%3Ccontext%3Etest%3C/context%3E%3C/prepositionHttpDemoReq%3E HTTP/1.1\r\nAccept: */*\r\nAccept-Language: zh-cn\r\nUser-Agent: Mozilla/4.0 (compatible; MSIE 5.01; Windows NT 5.0)\r\nHost: 192.168.20.177:8080\r\nConnection: Close\r\n\r\n");
		s= strlen(newbuf);
		dcs_debug( newbuf, s, "buffer len:%d,", s);


		dcs_log(0,0, "attach to folder system ok !\n");

		fid = fold_locate_folder("CL_TPOS_002");

		if(fid < 0)
		{
			dcs_log(0,0,"cannot get folder '%s':%s\n", "CL_TPOS_002", ise_strerror(errno) );
			return -1;
		}

		fold_write(fid,-1,newbuf, s );
		trace ++;
		dcs_log( 0,0, "Write finished,Write length =%d",s);
	}

	//从myfold内读出内容

	openfid= fold_open(gs_myFid);
	trace = 1;
	while(trace <=num)
	{
		memset(tmpbuf,0,sizeof(tmpbuf));
		len = fold_read(gs_myFid, openfid, &org_folderId, tmpbuf, sizeof(tmpbuf),1);
		if(len <0 )
		{
			dcs_log(0,0, "read fold fail");
			return -1;
		}
		dcs_log(0,0, "read the buf len:\n%s",tmpbuf);
		trace++;
	}
	return 0;
}

int Unpack_Http(char * buffer,int len)
{
	char buf[2400];
	char length[5];
	char * pszContentLenStart = NULL ,*pszContentLenEnd = NULL ,*pszHttpEnd = NULL ;
	int len1= 0;

	memset(buf, 0x00 ,sizeof(buf));
	memcpy(buf,buffer,len);
	//从head里找长度标签
	pszContentLenStart = strstr(buf, "Content-Length:");
	if(pszContentLenStart == NULL)
	{
		pszContentLenStart = strstr(buf, "content-length:");
	}
	if(pszContentLenStart == NULL)
	{
		dcs_log(0, 0, "请求包格式错误!");
		return -1;
	}
	//处理长度的结束符
	pszContentLenStart += strlen("Content-Length:");
	pszContentLenEnd = strstr(pszContentLenStart, "\x0D\x0A");
	if(pszContentLenEnd == NULL)
	{
		dcs_log(0, 0, "请求包格式错误!");
		return -1;
	}

	//取得内容长度
	memset(length, 0 ,sizeof(length));
	memcpy(length, pszContentLenStart, pszContentLenEnd - pszContentLenStart);
	len1 = atoi(length);

	//取内容
	pszHttpEnd = strstr(buf, "\x0D\x0A\x0D\x0A");
	if(pszHttpEnd == NULL)
	{
		dcs_log(0, 0, "请求包格式错误!");
		return -1;
	}
	//模拟5个字节的TPDU和2个字节长度
	memcpy(buffer+2,"12345",5);
	memcpy(buffer+7,pszHttpEnd +4,len1);
	buffer[0] =(len1+5)/256 ;
	buffer[0] =(len1+5)%256 ;
	return len1+7;
}

int MakeHttpsocket(void *trace)
{
	int sockfd = 0,len = 0, len11 = 0;
	char    buf[500];
	struct sockaddr_in servaddr;
	struct packBuf pack;
	memset(&pack,0,sizeof(packBuf));
	memcpy(pack.buf,((struct packBuf*)trace)->buf,packLen);
	pack.trace = ((struct packBuf*)trace)->trace;

	free(trace);
	trace =NULL;
	int s;
	dcs_log(0,0,"before socket ");
	if((sockfd = socket(AF_INET,SOCK_STREAM,0))<0)
	{
		 dcs_log(0,0,"create socket error: %s(errno: %d)", strerror(errno),errno);
		 recvnum++;
    		return -1;
	}
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(gPort);
	if( inet_pton(AF_INET, gIp, &servaddr.sin_addr) <= 0)
	{
		dcs_log(0,0,"inet_pton error for %s",gIp);
		 recvnum++;
		return -1;
	}
	dcs_log(0,0,"before connect ");
	if( connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
	{
		dcs_log(0,0,"connect error: %s(errno: %d)",strerror(errno),errno);
		close(sockfd);
		 recvnum++;
		return -1;
	}
	dcs_log(0,0,"send msg to server:");
	//发送报文
	memset(buf,0,sizeof(buf));
	sprintf(buf, "POST HTTP/1.1\r\nAccept: */*\r\nUser-Agent: Mozilla/4.0\r\nContent-Length:%d\r\nConnection:close\r\nAccept-Language:zh-cn\r\nAccept-Encoding: gzip, deflate\r\nHost:%s\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n", packLen,gIp);
	len = strlen(buf);
	memcpy(buf+len,pack.buf,packLen);
	len = len+packLen;
	dcs_debug( buf, len, "send msg:len=%d",len);

	if( (len =write(sockfd, buf, len) )< 0)
	{
		dcs_log(0,0,"send msg error: %s(errno: %d)", strerror(errno), errno);
		close(sockfd);
		 recvnum++;
		return -1;
	}
	//printf("send msg to server: succ\n");
	dcs_log(0,0,"send msg to server succ: len(%d)trace[%d]",len,pack.trace);
	memset(buf,0,sizeof(buf));
	if((len=read(sockfd, buf, sizeof(buf))) <0)
	{
		dcs_log(0,0,"recv msg error: %s(errno: %d)", strerror(errno), errno);
		close(sockfd);
		return -1;
	}
	if(len == 0)
	{
		dcs_log(0,0,"recv msg from server: err,链接终止trace[%d]",pack.trace);
		close(sockfd);
		return 0;
	}
	dcs_debug( buf, len, "recvbuffer len:%d,", len);

	len11 =Unpack_Http(buf, len);
	if(len11 <0)
		return 0;

	dcs_debug( buf, len11, "recvbuffer len:%d,", len11);
	s=Unpackpos(buf,len11);
	if(s!= pack.trace)
	{
		dcs_log(0,0,"返回流水不符返回流水=[%d],发送流水=[%d]",s,pack.trace);
	}
	else
	{
		dcs_log(0,0,"返回流水相同=[%d]",s);
	}

	recvnum++;
	close(sockfd);
	return 0;
}


/*模拟http客户端,向前置发起请求
*/
int moniHttpClient( )
{
	char    tmpbuf[250];
	int	len, s,trace=1;
	char	buffer[250], newbuf[250];
	struct packBuf *pack =NULL;

	ISO_data siso;

	int pid,cpid;
	s = OpenLog();
	if(s < 0)
        return -1;

	pid = fork();
	if(pid<0)
	{
		 dcs_log(0,0,"fork child err");
		 return -1;
	}
	else if(pid >0)
	{
		waitpid(pid, NULL, 0);
		 dcs_log(0,0,"fork child succ,pid=[%d]",pid);
		 return 0;
	}
	if(IsoLoad8583config(&iso8583_conf[0],"iso8583_pos.conf") < 0)
   	 {
	        dcs_log(0,0,"IsoLoad8583config() failed:%s\n",strerror(errno));

	        printf("IsoLoad8583config failed \n");

	        return -1;
    	}

	iso8583=&iso8583_conf[0];
	SetIsoHeardFlag(0);
    	SetIsoFieldLengthFlag(0);
	 recvnum =0;
	while(trace <=num)
	{
		memset(&siso,0,sizeof(ISO_data));
		setbit(&siso,0,(unsigned char *)"0200", 4);
		setbit(&siso,3,(unsigned char *)"000000", 6);
		setbit(&siso,4,(unsigned char *)"000000800000", 12);//消费金额
		memset(buffer, 0 ,sizeof(buffer));
		sprintf(buffer, "%6d",trace);
		setbit(&siso,11,(unsigned char *)buffer, 6);//pos终端上送流水
		setbit(&siso,20,(unsigned char *)"00000", 5);
		setbit(&siso,21,(unsigned char *)"22NP811071033395210248790000", 28);
		setbit(&siso,22,(unsigned char *)"022", 3);
		setbit(&siso,25,(unsigned char *)"00", 2);
		setbit(&siso,35,(unsigned char *)"6228210660015825514D49121206363770000", 37);
		setbit(&siso,36,(unsigned char *)"996228210660015825514D156156000000000000000000000011414144912DD000000000000D000000000000D095302700000000", 104);
		setbit(&siso,41,(unsigned char *)"12346427",8);
		setbit(&siso,42,(unsigned char *)"999899848110001",15);

		memset(buffer,0,sizeof(buffer));
		sprintf( buffer, "0600000000000000" );
		setbit(&siso,53,(unsigned char *)buffer, strlen(buffer) );
		setbit(&siso,49,(unsigned char *)"156",3);
	    	setbit(&siso,60,(unsigned char *)"22000014000601",14);
		setbit( &siso, 64,(unsigned char *)"00000000000", 8 );
		memset(buffer, 0, sizeof(buffer));
		s = iso_to_str( (unsigned char *)buffer, &siso, 0);
		if ( s < 0 )
		{
			dcs_log(0, 0, "iso_to_str error.");
			return -1;
		}
		memset(newbuf, 0, sizeof(newbuf));
		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy( newbuf, "602200000000",12 );
		asc_to_bcd( (unsigned char *) tmpbuf, (unsigned char*) newbuf, 12, 0 );
		memset(newbuf, 0, sizeof(newbuf));
		memcpy( newbuf, tmpbuf, 6);
		sprintf(newbuf+6,"086001003021004611223344%04d",s);//添加报文头2
		memcpy( newbuf+34, buffer, s);
		s = s + 34;
		pack=(struct packBuf *)malloc(sizeof(struct packBuf));
		if(pack == NULL)
		{
			 dcs_log(0,0,"malloc error:%s,errno:%d", strerror(errno),errno);
			 return -1;
		}

		dcs_debug( newbuf, s, "buffer len:%d,", s);
		memset(pack->buf, 0, sizeof(pack->buf));
		memcpy(pack->buf,newbuf, s );
		pack->trace = trace;
		packLen = s;

		 pthread_t sthread;
	        if (pthread_create(&sthread, NULL,( void * (*)(void *))MakeHttpsocket, (void *)pack) !=0)
	        {
	           dcs_log(0,0,"Create thread fail");
	        }
	        pthread_detach(sthread);

		trace ++;
		dcs_log( 0,0, "Write finished,Write length =%d",s);
	}

	dcs_log(0,0,"end exit");
	if( recvnum >=num)
	 	exit(1);
}


/*
* 模拟同步tcp通讯,通过读写队列实现
*/
int moniTcpSyn( )
{
	char    tmpbuf[250];
	int	len, s,trace=0;
	char	buffer[250], newbuf[250], buf[6+1];
	int pid,cpid;
	long msgtype =0;

	ISO_data siso;
	s = OpenLog();
	if(s < 0)
        return -1;

	pid = fork();
	if(pid<0)
	{
		 dcs_log(0,0,"fork child err");
		 return -1;
	}
	else if(pid >0)
	{
		waitpid(pid, NULL, 0);
		 dcs_log(0,0,"fork child succ,pid=[%d]",pid);
		exit(0);
	}

	if(IsoLoad8583config(&iso8583_conf[0],"iso8583_pos.conf") < 0)
   	 {
	        dcs_log(0,0,"IsoLoad8583config() failed:%s\n",strerror(errno));

	        printf("IsoLoad8583config failed \n");

	        return -1;
    	}
	iso8583=&iso8583_conf[0];
	SetIsoHeardFlag(0);
    	SetIsoFieldLengthFlag(0);
	while(trace <num)
	{
		trace ++;
		if(num >1 )
		{
			pid = fork();
			if(pid<0)
			{
				 dcs_log(0,0,"fork child err");
				 return -1;
			}
			else if(pid >0)
			{
				//waitpid(pid, NULL, 0);
				 dcs_log(0,0,"fork child succ,pid=[%d]",pid);
				//exit(0);
				continue;
			}
		}
		memset(&siso,0,sizeof(ISO_data));
		memset(tmpbuf,0,sizeof(tmpbuf));
		setbit(&siso,0,(unsigned char *)"0200", 4);
		setbit(&siso,3,(unsigned char *)"000000", 6);
		setbit(&siso,4,(unsigned char *)"000000800000", 12);//消费金额
		memset(buf, 0 ,sizeof(buf));
		sprintf(buf, "%6d",trace);
		setbit(&siso,11,(unsigned char *)buf, 6);//pos终端上送流水
		setbit(&siso,20,(unsigned char *)"00000", 5);
		setbit(&siso,21,(unsigned char *)"22NP811071033395210248790000", 28);
		setbit(&siso,22,(unsigned char *)"022", 3);
		setbit(&siso,25,(unsigned char *)"00", 2);
		setbit(&siso,35,(unsigned char *)"6228210660015825514D49121206363770000", 37);
		setbit(&siso,36,(unsigned char *)"996228210660015825514D156156000000000000000000000011414144912DD000000000000D000000000000D095302700000000", 104);
		setbit(&siso,41,(unsigned char *)"12346427",8);
		setbit(&siso,42,(unsigned char *)"999899848110001",15);
		sprintf( tmpbuf, "0600000000000000" );
		setbit(&siso,53,(unsigned char *)tmpbuf, strlen(tmpbuf) );
		setbit(&siso,49,(unsigned char *)"156",3);
	    	setbit(&siso,60,(unsigned char *)"22000014000601",14);
		setbit( &siso, 64,(unsigned char *)"00000000000", 8 );
		memset(buffer, 0, sizeof(buffer));
		s = iso_to_str( (unsigned char *)buffer, &siso, 0);
		if ( s < 0 )
		{
			dcs_log(0, 0, "iso_to_str error.");
			exit(0);
		}
		memset(newbuf, 0, sizeof(newbuf));
		memset(tmpbuf, 0, sizeof(tmpbuf));
 /*测试TCP协议的同步通讯,测试服务器端需要TPDU*/
		memcpy( newbuf, "6008230000",10);
		memcpy( newbuf+10, "602200000000",12 );
		asc_to_bcd( (unsigned char *) tmpbuf, (unsigned char*) newbuf, 22, 0 );
		memset(newbuf, 0, sizeof(newbuf));
		memcpy( newbuf, tmpbuf, 11);
		sprintf(newbuf+11,"086001003021004611223344%04d",s);//添加报文头2
		memcpy( newbuf+39, buffer, s);
		s = s + 39;

		int qid =queue_connect("recv");
		if(qid < 0)
		{
			dcs_log(0,0, "queue_create err");
			exit(0);
		}
		dcs_debug( newbuf, s, "send buffer len:%d,发送队列[%d]", s,qid);
		msgtype = getpid();
		len=queue_send_by_msgtype(qid,newbuf , s , msgtype,0);
		if(len< 0)
		{
			dcs_log(0,0, "queue_send_by_msgtype err");
			exit(0);
		}
#if 0
		//测试队列阻包情况
		//sleep(5);
		if (trace%20 ==0)
			exit(0);
#endif
		qid =queue_connect("send12");
		if(qid < 0)
		{
			dcs_log(0,0, "queue_connect err");
			exit(0);
		}
		memset(newbuf, 0, sizeof(newbuf));
		len =queue_recv_by_msgtype(qid,newbuf ,  sizeof(newbuf) , &msgtype,0);
		if(s < 0)
		{
			dcs_log(0,0, "queue_recv_by_msgtype err");
			return 0;
		}
		dcs_debug( newbuf,len, "recv buf length =%d,接收队列[%d],msgtype[%ld]",len,qid,msgtype);

		if(memcmp(newbuf,"FF",2) ==0)
		{
			dcs_log(0,0,"返回信息表示报文处理失败.");
		}
		else
		{
			s=Unpackpos(newbuf,len);
			if(s!= trace)
			{
				dcs_log(0,0,"返回流水不符返回流水=[%d],发送流水=[%d]",s,trace);
			}
			else
			{
				dcs_log(0,0,"返回流水相同=[%d]",s);
			}
		}

		exit(0);
	}

	exit(0);
}

/*
* 模拟同步HTTP通讯,通过读写队列实现
*/
int moniHttpSyn( )
{
	char    tmpbuf[250];
	int	len, s,trace=0;
	char	buffer[250], newbuf[250], buf[6+1];
	int pid,cpid;
	long msgtype =0;

	ISO_data siso;
	s = OpenLog();
	if(s < 0)
        return -1;

	pid = fork();
	if(pid<0)
	{
		 dcs_log(0,0,"fork child err");
		 return -1;
	}
	else if(pid >0)
	{
		waitpid(pid, NULL, 0);
		 dcs_log(0,0,"fork child succ,pid=[%d]",pid);
		exit(0);
	}

	if(IsoLoad8583config(&iso8583_conf[0],"iso8583_pos.conf") < 0)
   	 {
	        dcs_log(0,0,"IsoLoad8583config() failed:%s\n",strerror(errno));

	        printf("IsoLoad8583config failed \n");

	        return -1;
    	}
	iso8583=&iso8583_conf[0];
	SetIsoHeardFlag(0);
    	SetIsoFieldLengthFlag(0);
	while(trace <num)
	{
		trace ++;
		if(num >1 )
		{
			pid = fork();
			if(pid<0)
			{
				 dcs_log(0,0,"fork child err");
				 return -1;
			}
			else if(pid >0)
			{
				//waitpid(pid, NULL, 0);
				 dcs_log(0,0,"fork child succ,pid=[%d]",pid);
				//exit(0);
				continue;
			}
		}
		memset(&siso,0,sizeof(ISO_data));
		memset(tmpbuf,0,sizeof(tmpbuf));
		setbit(&siso,0,(unsigned char *)"0200", 4);
		setbit(&siso,3,(unsigned char *)"000000", 6);
		setbit(&siso,4,(unsigned char *)"000000800000", 12);//消费金额
		memset(buf, 0 ,sizeof(buf));
		sprintf(buf, "%6d",trace);
		setbit(&siso,11,(unsigned char *)buf, 6);//pos终端上送流水
		setbit(&siso,20,(unsigned char *)"00000", 5);
		setbit(&siso,21,(unsigned char *)"22NP811071033395210248790000", 28);
		setbit(&siso,22,(unsigned char *)"022", 3);
		setbit(&siso,25,(unsigned char *)"00", 2);
		setbit(&siso,35,(unsigned char *)"6228210660015825514D49121206363770000", 37);
		setbit(&siso,36,(unsigned char *)"996228210660015825514D156156000000000000000000000011414144912DD000000000000D000000000000D095302700000000", 104);
		setbit(&siso,41,(unsigned char *)"12346427",8);
		setbit(&siso,42,(unsigned char *)"999899848110001",15);
		sprintf( tmpbuf, "0600000000000000" );
		setbit(&siso,53,(unsigned char *)tmpbuf, strlen(tmpbuf) );
		setbit(&siso,49,(unsigned char *)"156",3);
	    	setbit(&siso,60,(unsigned char *)"22000014000601",14);
		setbit( &siso, 64,(unsigned char *)"00000000000", 8 );
		memset(buffer, 0, sizeof(buffer));
		s = iso_to_str( (unsigned char *)buffer, &siso, 0);
		if ( s < 0 )
		{
			dcs_log(0, 0, "iso_to_str error.");
			exit(0);
		}
		memset(newbuf, 0, sizeof(newbuf));
		memset(tmpbuf, 0, sizeof(tmpbuf));
  /*测试HTTP协议的同步通讯,测试服务器端不需要TPDU*/
		memcpy( newbuf, "602200000000",12 );
		asc_to_bcd( (unsigned char *) tmpbuf, (unsigned char*) newbuf, 12, 0 );
		memset(newbuf, 0, sizeof(newbuf));
		memcpy( newbuf, tmpbuf, 6);
		sprintf(newbuf+6,"086001003021004611223344%04d",s);//添加报文头2
		memcpy( newbuf+34, buffer, s);
		s = s + 34;
		int qid =queue_connect("RECV1");
		if(qid < 0)
		{
			dcs_log(0,0, "queue_create err");
			exit(0);
		}
		dcs_debug( newbuf, s, "send buffer len:%d,发送队列[%d]", s,qid);
		msgtype = getpid();
		len=queue_send_by_msgtype(qid,newbuf , s , msgtype,0);
		//测试不返回的现象
		//len=queue_send_by_msgtype(qid,newbuf , 22 , msgtype,0);
		if(len< 0)
		{
			dcs_log(0,0, "queue_send_by_msgtype err");
			exit(0);
		}
#if 0
		//测试队列阻包情况
		//sleep(5);
		sleep(num-trace);
		//if (trace%20 ==0)
			//exit(0);
#endif
		qid =queue_connect("SEND1");
		if(qid < 0)
		{
			dcs_log(0,0, "queue_connect err");
			exit(0);
		}
		memset(newbuf, 0, sizeof(newbuf));
		//为了解包成功前5个字节表示TPDU
		len =queue_recv_by_msgtype(qid,newbuf+5 ,  sizeof(newbuf) , &msgtype,0);
		if(s < 0)
		{
			dcs_log(0,0, "queue_recv_by_msgtype err");
			return 0;
		}
		dcs_debug( newbuf+5,len, "recv buf length =%d,接收队列[%d],msgtype[%ld]",len,qid,msgtype);

		if(memcmp(newbuf+5,"FF",2) ==0)
		{
			dcs_log(0,0,"返回信息表示报文处理失败.");
		}
		else
		{
			len+=5;//为了解包成功
			s=Unpackpos(newbuf,len);
			if(s!= trace)
			{
				dcs_log(0,0,"返回流水不符返回流水=[%d],发送流水=[%d]",s,trace);
			}
			else
			{
				dcs_log(0,0,"返回流水相同=[%d]",s);
			}
		}

		exit(0);
	}

	exit(0);
}

/*
* 模拟超时控制的测试
*/
int moniTime( )
{
	char    tmpbuf[20];
	int	len, s,trace=0;
	char	buffer[250], newbuf[250];
	int pid,cpid;
	int openid = 0 , myFid = 0, orgfid = 0;
	struct Timer_struct strTimer;

	time_t  t;
	struct tm * tm  = NULL;

	ISO_data siso;
	s = OpenLog();
	if(s < 0)
        return -1;

	pid = fork();
	if(pid<0)
	{
		 dcs_log(0,0,"fork child err");
		 return -1;
	}
	else if(pid >0)
	{
		waitpid(pid, NULL, 0);
		 dcs_log(0,0,"fork child succ,pid=[%d]",pid);
		exit(0);
	}

	if(IsoLoad8583config(&iso8583_conf[0],"iso8583_pos.conf") < 0)
   	 {
	        dcs_log(0,0,"IsoLoad8583config() failed:%s\n",strerror(errno));

	        printf("IsoLoad8583config failed \n");

	        return -1;
    	}
	int fid = fold_locate_folder("TRANSRCV");

	if(fid < 0)
	{
		dcs_log(0,0,"cannot get folder '%s':%s\n", "TRANSRCV", ise_strerror(errno) );
		return -1;
	}
	myFid = fold_create_folder("TIMETEST");
	if( myFid <0 )
		myFid = fold_locate_folder("TIMETEST");
	if(myFid < 0)
	{
		dcs_log(0,0,"cannot get folder '%s':%s\n", "TIMETEST", ise_strerror(errno) );
		return -1;
	}
	openid = fold_open(myFid);
	if(openid < 0)
	{
		dcs_log(0,0,"open fold fail myFid = %d",myFid);
		exit(0);
	}
	iso8583=&iso8583_conf[0];
	SetIsoHeardFlag(0);
    	SetIsoFieldLengthFlag(0);
	while(trace <num)
	{
		trace ++;
		memset(&siso,0,sizeof(ISO_data));
		setbit(&siso,0,(unsigned char *)"0200", 4);
		setbit(&siso,3,(unsigned char *)"000000", 6);
		setbit(&siso,4,(unsigned char *)"000000800000", 12);//消费金额
		memset(tmpbuf,0,sizeof(tmpbuf));
		sprintf(tmpbuf, "%6d",trace);
		setbit(&siso,11,(unsigned char *)tmpbuf, 6);//pos终端上送流水
		setbit(&siso,20,(unsigned char *)"00000", 5);
		setbit(&siso,21,(unsigned char *)"22NP811071033395210248790000", 28);
		setbit(&siso,22,(unsigned char *)"022", 3);
		setbit(&siso,25,(unsigned char *)"00", 2);
		setbit(&siso,35,(unsigned char *)"6228210660015825514D49121206363770000", 37);
		setbit(&siso,36,(unsigned char *)"996228210660015825514D156156000000000000000000000011414144912DD000000000000D000000000000D095302700000000", 104);
		setbit(&siso,41,(unsigned char *)"12346427",8);
		setbit(&siso,42,(unsigned char *)"999899848110001",15);
		sprintf( tmpbuf, "0600000000000000" );
		setbit(&siso,53,(unsigned char *)tmpbuf, strlen(tmpbuf) );
		setbit(&siso,49,(unsigned char *)"156",3);
	    	setbit(&siso,60,(unsigned char *)"22000014000601",14);
		setbit( &siso, 64,(unsigned char *)"00000000000", 8 );
		memset(buffer, 0, sizeof(buffer));
		s = iso_to_str( (unsigned char *)buffer, &siso, 0);
		if ( s < 0 )
		{
			dcs_log(0, 0, "iso_to_str error.");
			exit(0);
		}
		memset(newbuf, 0, sizeof(newbuf));
		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy( newbuf, "6008230000",10);
		memcpy( newbuf+10, "602200000000",12 );
		asc_to_bcd( (unsigned char *) tmpbuf, (unsigned char*) newbuf, 22, 0 );
		memset(newbuf, 0, sizeof(newbuf));

		//memcpy(newbuf,"TPOS",4);//pos发起
		memcpy(newbuf,"TPOS",4);
		memcpy( newbuf+4, tmpbuf, 11);
		sprintf(newbuf+15,"086001003021004611223344%04d",s);//添加报文头2
		memcpy( newbuf+43, buffer, s);
		s = s + 43;

		memset(&strTimer, 0 ,sizeof(struct Timer_struct));
		memcpy(strTimer.data , newbuf,s);
		strTimer.length = s;
		strTimer.sendnum = 1;
		strTimer.timeout = 5;
		strTimer.foldid =(char)fid ;//目标fordid
		time(&t);
   		tm= localtime(&t);
		sprintf(strTimer.key,"%04d%02d%02d%02d%02d%02d%06d",tm->tm_year+1990,tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,trace);
		//sprintf(strTimer.key,"%06d",trace);

		fold_write(fid, myFid, newbuf, s);

		dcs_debug( strTimer.data, strTimer.length, "send buffer len:%d,发送folder[%d]", strTimer.length,strTimer.foldid);

		tm_insert_entry(strTimer);
		memset(newbuf, 0, sizeof(newbuf));
		len = fold_read(myFid, openid, &orgfid, newbuf+2, sizeof(newbuf), 1);
		if(len > 0)
		{
			s=Unpackpos(newbuf,len+2);
			if(s!= trace)
			{
				dcs_log(0,0,"返回流水不符返回流水=[%d],发送流水=[%d]",s,trace);
			}
			else
			{
				dcs_log(0,0,"返回流水相同=[%d]",s);
			}
		}
		else
		{
			dcs_log(0,0,"read_fold fail");
		}

		if(trace%2)
		{
			tm_remove_entry(strTimer.key);
		}


	}
exit(0);
	int i=0;
	dcs_log(0,0," 次数%d",(strTimer.sendnum +1)* num);
	//for(i=0 ;i< (strTimer.sendnum +1)* num ;i++ )
	{
	/*
		if( i ==2 )
		{
			memset(tmpbuf,0,sizeof(tmpbuf));
			memcpy(tmpbuf, strTimer.key,19);
			tm_remove_entry(tmpbuf);
			tm_remove_entry(strTimer.key);
		}
	*/
		len = fold_read(myFid, openid, &orgfid, newbuf+2, sizeof(newbuf), 1);
		if(len > 0)
		{
			s=Unpackpos(newbuf,len+2);
			if(s!= trace)
			{
				dcs_log(0,0,"返回流水不符返回流水=[%d],发送流水=[%d],i=%d",s,trace,i);
			}
			else
			{
				dcs_log(0,0,"返回流水相同=[%d],i=%d",s,i);
			}
		}
		else
		{
			dcs_log(0,0,"read_fold fail");
		}
	}


	exit(0);
}


//模拟管理平台发起的KSN生成报文
int moniMpos()
{
	int	len;
	char	buffer[250];
	int pid;
	int openid = 0 , myFid = 0, orgfid = 0;

	len = OpenLog();
	if(len < 0)
        return -1;

	pid = fork();
	if(pid<0)
	{
		 dcs_log(0,0,"fork child err");
		 return -1;
	}
	else if(pid >0)
	{
		waitpid(pid, NULL, 0);
		 dcs_log(0,0,"fork child succ,pid=[%d]",pid);
		exit(0);
	}
	int fid = fold_locate_folder("TRANSRCV");

	if(fid < 0)
	{
		dcs_log(0,0,"cannot get folder '%s':%s\n", "TRANSRCV", ise_strerror(errno) );
		return -1;
	}
	myFid = fold_create_folder("TMPOSETEST");
	if( myFid <0 )
		myFid = fold_locate_folder("TMPOSETEST");
	if(myFid < 0)
	{
		dcs_log(0,0,"cannot get folder '%s':%s\n", "TMPOSETEST", ise_strerror(errno) );
		return -1;
	}
	openid = fold_open(myFid);
	if(openid < 0)
	{
		dcs_log(0,0,"open fold fail myFid = %d",myFid);
		exit(0);
	}
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "GLPT12345AAA0106802802901001872110000600000");
	len = strlen(buffer);

	fold_write(fid, myFid, buffer, len);

	dcs_debug(buffer, len, "send buffer len:%d", len);

	memset(buffer, 0, sizeof(buffer));
	len = fold_read(myFid, openid, &orgfid,buffer, sizeof(buffer), 1);
	if(len > 0)
	{
		dcs_debug(buffer, len, "read buffer len:%d", len);
	}
	else
	{
		dcs_log(0,0,"read_fold fail");
	}

	exit(0);
}


/*
测试xml函数功能
*/
int testxml()
{
 	ISO_data siso;
       struct  tm *tm;
	time_t  t;
	char    *ltime;
	char tmpbuf[40];
	int s = 0, ret=0;
    char	buffer[400], newbuf[400];
	char szTpdu[11]={0};
	char xmlbuf[2550];
	int pid,i;

	struct packBuf *pack =NULL;

	s = OpenLog();
	if(s < 0)
        return -1;
	gType =1;
	pid = fork();
	if(pid<0)
	{
		 dcs_log(0,0,"fork child err");
		 return -1;
	}
	else if(pid >0)
	{
		waitpid(pid, NULL, 0);
		 dcs_log(0,0,"fork child succ,pid=[%d]",pid);
		 return 0;
	}

	if(IsoLoad8583config(&iso8583_conf[0],"iso8583_pos.conf") < 0)
   	 {
	        dcs_log(0,0,"IsoLoad8583config() failed:%s\n",strerror(errno));

	        printf("IsoLoad8583config failed \n");

	        return -1;
    	}

	iso8583=&iso8583_conf[0];
	SetIsoHeardFlag(0);
	SetIsoFieldLengthFlag(0);
	recvnum =0;
	gSendNum =0;
	for(i=0;i<10;i++)
	{
		if(stTpdu[i].port == gPort)
		{
			memcpy(szTpdu,stTpdu[i].tpdu,10);
			break;
		}

	}
	if(i>=10)
	{
		dcs_log(0,0,"port=[%d] not ture!",gPort);
		printf("port=[%d] not ture!",gPort);
		return 0;
	}

	dcs_log(0,0," 模拟POS消费交易发送给前置 start");//0,1,3,4,11,22,25,26,35,36,41,42,49,52,53,60,64

	memset(&siso,0,sizeof(ISO_data));
	memset(tmpbuf,0,sizeof(tmpbuf));
	setbit(&siso,0,(unsigned char *)"0200", 4);
	setbit(&siso,3,(unsigned char *)"000000", 6);
	setbit(&siso,4,(unsigned char *)"000000000001", 12);//消费金额
	memset(tmpbuf, 0, sizeof(tmpbuf));
	time(&t);
		tm = localtime(&t);
	sprintf(tmpbuf,"%02d%02d%02d%02d%02d",tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
	setbit(&siso,7,(unsigned char *)tmpbuf, 10);
	memset(tmpbuf, 0, sizeof(tmpbuf));
	sprintf(tmpbuf,"%06d",111);
	dcs_log(0,0,"trace=[%d],tpdu=[%s]",111,szTpdu);//0,1,3,4,11,22,25,26,35,36,41,42,49,52,53,60,64
	setbit(&siso,11,(unsigned char *)tmpbuf, 6);//pos终端上送流水
	setbit(&siso,20,(unsigned char *)"00000", 5);
	//setbit(&siso,21,(unsigned char *)"14NP811081168453320000088800000697000001cc000000002089860114003100149051", 72);
	setbit(&siso,21,(unsigned char *)"14NP811081168453320000088800000697000001cc000000002089860114003100149051", 72);
	setbit(&siso,22,(unsigned char *)"022", 3);
	setbit(&siso,25,(unsigned char *)"00", 2);
	setbit(&siso,35,(unsigned char *)"6228412320121305911D49121207693980000", 37);
	setbit(&siso,36,(unsigned char *)"996228412320121305911D156156000000000000000000000011414144912DD000000000000D000000000000D030705800000000", 104);
	setbit(&siso,41,(unsigned char *)"12345914",8);
	setbit(&siso,42,(unsigned char *)"999682907440095",15);
	setbit(&siso,53,(unsigned char *)"0600000000000000" , 16);
	//测试
	setbit(&siso,54,(unsigned char *)"你好啊" , 6);
	setbit(&siso,49,(unsigned char *)"156",3);
	setbit(&siso,60,(unsigned char *)"22000079000601",14);
	setbit( &siso, 64,(unsigned char *)"00000000000", 8 );

	iso8583 =&iso8583_conf[0];
	SetIsoHeardFlag(0);
	 SetIsoFieldLengthFlag(0);

	memset(buffer, 0, sizeof(buffer));
	s = iso_to_str( (unsigned char *)buffer, &siso, 0);
	if ( s < 0 )
	{
		dcs_log(0, 0, "iso_to_str error.");
		return -1;
	}
	//1.测试8583结构和xml格式的转换函数
	memset(xmlbuf,0,sizeof(xmlbuf));
	ret = iso_to_xml(&siso,xmlbuf );
	if ( ret < 0 )
	{
		dcs_log(0, 0, "iso_to_xml error.");
		return -1;
	}
	dcs_log(0, 0, "iso_to_xml [%s]",xmlbuf);
	//2.测试xml格式和8583结构的转换函数
	memset(&siso,0,sizeof(ISO_data));
	ret = xml_to_iso(&siso,xmlbuf, strlen(xmlbuf));
	if ( ret < 0 )
	{
		dcs_log(0, 0, "xml_to_iso error.");
		return -1;
	}
	//3. 再次将iso转换为str
	memset(buffer, 0, sizeof(buffer));
	s = iso_to_str( (unsigned char *)buffer, &siso, 0);
	if ( s < 0 )
	{
		dcs_log(0, 0, "2222iso_to_str error.");
		return -1;
	}
	//4.根据xpath查找 指定节点,需指定具体的路径
	memset(newbuf, 0, sizeof(newbuf));
	ret =  getNodeValueFromXml(xmlbuf, strlen(xmlbuf),(char *)"/iso8583/bits/bit[last()]/value",newbuf);
	//ret =  getNodeValueFromXml(xmlbuf, strlen(xmlbuf),(char *)"/iso8583/bits/bit[last()]/@name",newbuf);
	if(ret == 0)
	{
		dcs_log(0, 0, "getNodeSet: %s\n",newbuf);
	}
	memset(newbuf, 0, sizeof(newbuf));
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memcpy( newbuf, szTpdu,10);
	memcpy( newbuf+10, "602200000000",12 );
	asc_to_bcd( (unsigned char *) tmpbuf, (unsigned char*) newbuf, 22, 0 );
	memset(newbuf, 0, sizeof(newbuf));
	memcpy(newbuf,"11",2);//放报文长度
	memcpy( newbuf+2, tmpbuf, 11);
	sprintf(newbuf+13,"086001003021004611223344%04d",s);//添加报文头2
	memcpy( newbuf+41, buffer, s);
	s = s + 41;
	newbuf[0]=(s-2)/256;
	newbuf[1]=(s-2)%256;
	pack = (struct packBuf *)malloc(sizeof(struct packBuf ));
	memcpy(pack->buf,newbuf,s);
	packLen =s;
	pack->trace=111;
	Makesocket((void *)pack);

	dcs_log(0,0,"发送报文结束end");
	exit(1);

}

/*
模拟核心返回余额查询交易
*/
int moniXpeBalance()
{
 	ISO_data siso;
       struct  tm *tm;
	time_t  t;
	char    *ltime;
	char tmpbuf[40];
	int s = 0, ret=0;
    char	buffer[400], newbuf[400];
	int seconds=0,i=0;
	int currentseconds=0,sendnum=0;
	int trace=1;
	int starttime=0,currenttime=0;
	int totaltime=0;
	int pid,s_myFid;
	s = OpenLog();
	if(s < 0)
        return -1;
	gType =1;
	pid = fork();
	if(pid<0)
	{
		 dcs_log(0,0,"fork child err");
		 return -1;
	}
	else if(pid >0)
	{
		waitpid(pid, NULL, 0);
		 dcs_log(0,0,"fork child succ,pid=[%d]",pid);
		 return 0;
	}

	struct packBuf *pack;
	if(IsoLoad8583config(&iso8583_conf[0],"iso8583.conf") < 0)
   	 {
	        dcs_log(0,0,"IsoLoad8583config() failed:%s\n",strerror(errno));
	        return -1;
    	}

	iso8583=&iso8583_conf[0];
	SetIsoHeardFlag(0);
	SetIsoFieldLengthFlag(0);
	i=0;

	dcs_log(0,0," 模拟POS余额查询交易核心返回发送给前置 start");//0,1,3,4,11,22,25,26,35,36,41,42,49,52,53,60,64
	starttime= time((time_t*)NULL);
	currenttime = starttime;
	if(sendCycle == 0)
	{
		totaltime =100*24*60*60;//100天
	}
	else
	{
		//totaltime =sendCycle*60;
		totaltime =sendCycle;
	}

	s_myFid = fold_create_folder("XPETRANSRCV");

	if(s_myFid < 0)
		s_myFid = fold_locate_folder("XPETRANSRCV");

	if(s_myFid < 0)
	{
		dcs_log(0,0,"cannot create folder '%s':%s\n", "XPETRANSRCV", ise_strerror(errno) );
		return -1;
	}

	//组报文
	memset(&siso,0,sizeof(ISO_data));
	setbit(&siso,0,(unsigned char *)"0210", 4);
	setbit(&siso,2,(unsigned char *)"6214441000010129", 16);
	setbit(&siso,3,(unsigned char *)"310000", 6);
	setbit(&siso,7,(unsigned char *)"0827091910", 10);
	setbit(&siso,11,(unsigned char *)"185238", 6);//pos终端上送流水
	setbit(&siso,12,(unsigned char *)"091915", 6);
	setbit(&siso,13,(unsigned char *)"0827", 4);
	setbit(&siso,15,(unsigned char *)"0827", 4);
	setbit(&siso,22,(unsigned char *)"051", 3);
	setbit(&siso,25,(unsigned char *)"00", 2);
	setbit(&siso,37,(unsigned char *)"091910185238", 12);
	setbit(&siso,39,(unsigned char *)"00", 2);
	setbit(&siso,41,(unsigned char *)"00000001",8);
	setbit(&siso,42,(unsigned char *)"400029000210001",15);
	setbit(&siso,49,(unsigned char *)"156",3);
	setbit(&siso,53,(unsigned char *)"2600000000000000" , 16);
	setbit(&siso,54,(unsigned char *)"1002156C000018531926" , 20);
	setbit(&siso,55,(unsigned char *)"9F36020430910A0EAD737A4C1E00733030",17);

	iso8583 =&iso8583_conf[0];
	SetIsoHeardFlag(0);
	 SetIsoFieldLengthFlag(1);

	memset(buffer, 0, sizeof(buffer));
	s = iso_to_str( (unsigned char *)buffer, &siso, 0);
	if ( s < 0 )
	{
		dcs_log(0, 0, "iso_to_str error.");
		return -1;
	}
	memset(newbuf, 0, sizeof(newbuf));
	memcpy(newbuf, "BANK", 4);
	bcd_to_asc((unsigned char *)newbuf+4, (unsigned char *)buffer, 20 ,0);
	memcpy(newbuf+24, buffer+10, s-10);

	s = s+14;

	while(currenttime-starttime<totaltime)//发送周期循环
	{
		seconds= time((time_t*)NULL);
		currentseconds=seconds;
    	for(sendnum=1;sendnum<=num;sendnum++)//每秒发送循环
	    {

			dcs_log(0,0, "folder myFid=%d\n", s_myFid);

			fold_write(s_myFid, 0, newbuf, s);
		 	trace++;
		 	u_sleep_us(10000);//先睡一会0.01s

	    }
		dcs_log(0,0,"本次写folder XPETRANSRCV send num=[%d]",sendnum-1);
		dcs_log(0,0,"总共写folder XPETRANSRCV send num=[%d]",trace -1);
		currentseconds= time((time_t*)NULL);
		while(currentseconds==seconds)
		{
			u_sleep_us(100000);
			currentseconds= time((time_t*)NULL);
		}
		currenttime= time((time_t*)NULL);
	}
	dcs_log(0,0,"发送报文结束end");
	exit(1);

}


/*
模拟核心返回消费交易
*/
int moniXpeSale()
{
 	ISO_data siso;
       struct  tm *tm;
	time_t  t;
	char    *ltime;
	char tmpbuf[40];
	int s = 0, ret=0;
    char	buffer[400], newbuf[400];
	int seconds=0,i=0;
	int currentseconds=0,sendnum=0;
	int trace=1;
	int starttime=0,currenttime=0;
	int totaltime=0;
	int pid,s_myFid;
	s = OpenLog();
	if(s < 0)
        return -1;
	gType =1;
	pid = fork();
	if(pid<0)
	{
		 dcs_log(0,0,"fork child err");
		 return -1;
	}
	else if(pid >0)
	{
		waitpid(pid, NULL, 0);
		 dcs_log(0,0,"fork child succ,pid=[%d]",pid);
		 return 0;
	}

	struct packBuf *pack;
	if(IsoLoad8583config(&iso8583_conf[0],"iso8583.conf") < 0)
   	 {
	        dcs_log(0,0,"IsoLoad8583config() failed:%s\n",strerror(errno));
	        return -1;
    	}

	iso8583=&iso8583_conf[0];
	SetIsoHeardFlag(0);
	SetIsoFieldLengthFlag(0);
	i=0;

	dcs_log(0,0," 模拟POS消费交易核心返回发送给前置 start");//0,1,3,4,11,22,25,26,35,36,41,42,49,52,53,60,64
	starttime= time((time_t*)NULL);
	currenttime = starttime;
	if(sendCycle == 0)
	{
		totaltime =100*24*60*60;//100天
	}
	else
	{
		//totaltime =sendCycle*60;
		totaltime =sendCycle;
	}

	s_myFid = fold_create_folder("XPETRANSRCV");

	if(s_myFid < 0)
		s_myFid = fold_locate_folder("XPETRANSRCV");

	if(s_myFid < 0)
	{
		dcs_log(0,0,"cannot create folder '%s':%s\n", "XPETRANSRCV", ise_strerror(errno) );
		return -1;
	}

	//组报文
	memset(&siso,0,sizeof(ISO_data));
	setbit(&siso,0,(unsigned char *)"0210", 4);
	setbit(&siso,2,(unsigned char *)"6214441000010053", 16);
	setbit(&siso,3,(unsigned char *)"910000", 6);
	setbit(&siso,4,(unsigned char *)"000000000001", 12);
	setbit(&siso,7,(unsigned char *)"1026105450", 10);
	setbit(&siso,11,(unsigned char *)"188615", 6);//流水
	setbit(&siso,12,(unsigned char *)"105451", 6);
	setbit(&siso,13,(unsigned char *)"1026", 4);
	setbit(&siso,15,(unsigned char *)"0828", 4);
	setbit(&siso,22,(unsigned char *)"052", 3);
	setbit(&siso,25,(unsigned char *)"00", 2);
	setbit(&siso,37,(unsigned char *)"185241134034", 12);
	setbit(&siso,38,(unsigned char *)"123456", 6);
	setbit(&siso,39,(unsigned char *)"00", 2);
	setbit(&siso,41,(unsigned char *)"12347356",8);
	setbit(&siso,42,(unsigned char *)"999899898760004",15);
	setbit(&siso,44,(unsigned char *)"03012900   04032900   ",22);
	setbit(&siso,49,(unsigned char *)"156",3);
	setbit(&siso,53,(unsigned char *)"1000000000000000" , 16);
	setbit(&siso,55,(unsigned char *)"9F2608C76DF69B805BF33B9F2701809F100807000103A02000019F3704DE14898B9F360204E9950508800080009A031510269C01009F02060000000000015F2A02015682023C009F1A0201569F03060000000000009F3303E0F9C89F34031E03009F3501229F1E0838313332363334398408A0000003330101029F090200209F410400001086",134);

	iso8583 =&iso8583_conf[0];
	SetIsoHeardFlag(0);
	 SetIsoFieldLengthFlag(1);

	memset(buffer, 0, sizeof(buffer));
	s = iso_to_str( (unsigned char *)buffer, &siso, 0);
	if ( s < 0 )
	{
		dcs_log(0, 0, "iso_to_str error.");
		return -1;
	}
	memset(newbuf, 0, sizeof(newbuf));
	memcpy(newbuf, "BANK", 4);
	bcd_to_asc((unsigned char *)newbuf+4, (unsigned char *)buffer, 20 ,0);
	memcpy(newbuf+24, buffer+10, s-10);

	s = s+14;

	while(currenttime-starttime<totaltime)//发送周期循环
	{
		seconds= time((time_t*)NULL);
		currentseconds=seconds;
    	for(sendnum=1;sendnum<=num;sendnum++)//每秒发送循环
	    {

			dcs_log(0,0, "folder myFid=%d\n", s_myFid);

			fold_write(s_myFid, 0, newbuf, s);
		 	trace++;
		 	u_sleep_us(10000);//先睡一会0.01s
	    }
		dcs_log(0,0,"本次写folder XPETRANSRCV send num=[%d]",sendnum-1);
		dcs_log(0,0,"总共写folder XPETRANSRCV send num=[%d]",trace -1);
		currentseconds= time((time_t*)NULL);
		while(currentseconds==seconds)
		{
			u_sleep_us(100000);
			currentseconds= time((time_t*)NULL);
		}
		currenttime= time((time_t*)NULL);
	}
	dcs_log(0,0,"发送报文结束end");
	exit(1);

}











