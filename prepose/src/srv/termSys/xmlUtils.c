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
	while (bitNode != NULL)
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


