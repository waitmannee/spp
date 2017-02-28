/*

本函数功能说明：

1、能够移动支付终端的解密磁道和pin转加密函数
2、pos终端mac计算、校验和pin转加密

何庆其
2012-3-29
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "secuSrv.h"
#include "dccdcs.h"
#include "folder.h"
#include "ibdcs.h"
static int shmid = -1;
static int sg_qid=-1;
static struct SECSRVCFG * g_pCfg=NULL;
struct secLinkInfo * PEP_GetFreeLink( );
int XPE_SendFailMessage ( int id);

char PIN_KEY[] = "2F786830A25A5E4E128BF54D3C15415B";
char MAC_KEY[] = "DA4BBCB91ABC0AFB";
char MAIN_KEY[] = "3016745AB289EFCDBADCFE0325476981";
char POS_MAK[] = "1111111111111111";

#define byte unsigned char

int CountOne(byte input);
int GetDukptKey(unsigned char *ksn, unsigned char *key, unsigned char *dukptkey);
int GetIPEK(unsigned char *ksn, int bdk, int bdkxor, unsigned char *ipek);
int getKey(unsigned char *dukptkey, int type, unsigned char *workey);
int NonRevKeyGen(unsigned char *cReg1, unsigned char *key);
void procCounter(unsigned char *ksn, unsigned char *counter, unsigned char *counterTemp);
int SearchOne(unsigned char *counter, unsigned char *rtn);
byte SearchOneCore(byte input);



int PEP_DESInit()
{ 
	if (shmid <0)
	{
		dcs_log(0,0,"shmid<0, todo shm_connect ...");
		
		if ( (g_pCfg=shm_connect("SECSRV2", &shmid))==NULL)
		{
			 dcs_log(0,0,"shm_connect is null");
			 shmid= -1;
			 return 0;
		}
	}
	return 1;
}

/**
    返回NULL为失败，成功返回链路结构
**/
struct secLinkInfo * PEP_GetFreeLink()
{

	 int i;
	
	 if( g_pCfg == NULL) 
	 {
	 	dcs_log(0,0,"test g_pCfg=NULL");
	 	return NULL;
	 }
	 
	 if ( g_pCfg->srvStatus == 1)
	 {
	 	sem_lock(g_pCfg->secSemId ,1 ,1);
	 	for ( i=0; i< g_pCfg->maxLink ; i++)
	 	{
	 			
	 		if ( !(g_pCfg->secLink[i].lFlags & DCS_FLAG_USED) )
	 			 continue;
	 			 
 			if ( g_pCfg->secLink[i].iWorkFlag == 1)
	 			 continue;
	 			 
	 		if ( g_pCfg->secLink[i].iStatus == DCS_STAT_CONNECTED )
	 			 break;
	 			
	 	}
	 	if ( i == g_pCfg->maxLink )
	 	{
	 		sem_unlock(g_pCfg->secSemId,1,1);
	 		return NULL;
	 	}
	 	else
	 	{
	 		g_pCfg->secLink[i].iWorkFlag=1;
	 		sem_unlock(g_pCfg->secSemId ,1 ,1);
	 		return &g_pCfg->secLink[i];
	 	}
	 }
	 else
	 	return NULL;
}

int PEP_FreeLink( struct secLinkInfo *  pLink)
{
			int i=0;
/*			struct SECSRVCFG * pCfg;
		 pCfg = (struct SECSRVCFG *)shm_attach(shmid);
		 if( pCfg == NULL) 
		 {
		 			dcs_log(0,0,"can not attach SHM  shm_id=%d",shmid);
		 			return 0;
		 }
	 		for ( i=0; i< pCfg->maxLink ; i++)
	 		{
	 			
	 			 if ( !(pCfg->secLink[i].lFlags & DCS_FLAG_USED) )
	 			 		continue;
	 			 if ( pCfg->secLink[i].iNum ==  pLink->iNum )
	 			 {
	 			 		pCfg->secLink[i].iWorkFlag=0;
	 			 		break;
	 			 }
	 			 
	 		}
	 		if ( i == pCfg->maxLink )
	 		{
	 						dcs_log(0,0," Not found specify link");
	 			      return 0;
	 		}
	*/
	    pLink->iWorkFlag=0;
	 		return 1;
	
}

int XPE_SendFailMessage ( int id)
{
	int fid;
	char caBuf[20];
/*  if (fold_initsys() < 0)
  {
          dcs_log(0,0," cannot attach to FOLDER !");

          exit(0);
  }
*/
	fid = fold_locate_folder("SECSRV2");
	if  ( fid <0)
	{
		dcs_log(0,0,"can not locate folder [SECSRV2]");
		return 0;
	}
			
	sprintf(caBuf,"STAR%02d",id);		
 	fold_write(fid ,-1,caBuf,6);		
 
 	return 1;
	
}


int MPOS_KeyGen(int keyIndex, char *psam, char *rtnData)
{
	 int i;
	 struct secLinkInfo * pLink = NULL;
	
	 if (!PEP_DESInit())
	 {
	 	 dcs_log(0,0,"MPOS_KeyGen init des system fail");
	 	 return -1;
	 }

	 pLink = PEP_GetFreeLink();
	 if ( pLink == NULL )
	 {
	 	  dcs_log(0,0,"MPOS_KeyGen get free link fail");
	 	  return -2;
	 }

	 if ( memcmp(pLink->cEncType,"01",2)==0)
	 {
	 	  i = sjl05_mpos_keygen(pLink->iSocketId, keyIndex, psam, rtnData);
	 	  if ( i == -1 )
	 	  {
	 			PEP_FreeLink( pLink);
	 			dcs_log(0,0,"MPOS_KeyGen fail!");
	 			return -3;
	 	  }
	 	  else if ( i == -2)
	 	  {
	 			pLink->iStatus= DCS_STAT_DISCONNECTED;
	 			XPE_SendFailMessage( pLink->iNum);
	 			PEP_FreeLink( pLink);
	 			dcs_log(0,0,"MPOS_KeyGen Comm fail!");
	 			return -4;
	 	  }
	 }
	 else
	 {
	 	 PEP_FreeLink(pLink);
	 	 dcs_log(0,0,"不支持的加密机类型");
	 	 return -5;
	 }
	 PEP_FreeLink( pLink);
	 return 0;
}

int MPOS_KeyEnc(int zmkIndex, char *toEncryptKey, char *rtnData)
{
	 int i;
	 struct secLinkInfo * pLink = NULL;
	
	 if (!PEP_DESInit())
	 {
	 	 dcs_log(0,0,"MPOS_KeyEnc init des system fail");
	 	 return -1;
	 }

	 pLink = PEP_GetFreeLink();
	 if ( pLink == NULL )
	 {
	 	  dcs_log(0,0,"MPOS_KeyEnc get free link fail");
	 	  return -2;
	 }

	 if ( memcmp(pLink->cEncType,"01",2)==0)
	 {
		 // i= sjl05_mpos_keygen_encrypt(int iSockId, int zmkIndex, char *toEncryptKey, char *rtnData)
	 	 // i = sjl05_mpos_keyMPOS_KeyEncgen(pLink->iSocketId, zmkIndex, toEncryptKey, rtnData);
	 	i = sjl05_mpos_keygen_encrypt(pLink->iSocketId, zmkIndex, toEncryptKey, rtnData);
	 	if ( i == -1 )
	 	  {
	 			PEP_FreeLink( pLink);
	 			dcs_log(0,0,"MPOS_KeyEnc fail!");
	 			return -3;
	 	  }
	 	  else if ( i == -2)
	 	  {
	 			pLink->iStatus= DCS_STAT_DISCONNECTED;
	 			XPE_SendFailMessage( pLink->iNum);
	 			PEP_FreeLink( pLink);
	 			dcs_log(0,0,"MPOS_KeyEnc Comm fail!");
	 			return -4;
	 	  }
	 }
	 else
	 {
	 	 PEP_FreeLink(pLink);
	 	 dcs_log(0,0,"不支持的加密机类型");
	 	 return -5;
	 }
	 PEP_FreeLink( pLink);
	 return 0;
}

/*
	POS报文解密
	@param tdk(ascii, 32字节)
	@param data
	@param dataLen
	@param rtnData
	
	@author he.qingqi
	@date 2012-12-11 16:47:00
*/
int POS_PackageDecrypt(char *tdk, char *data, int dataLen, char *rtnData)
{
	 int i;
	 struct secLinkInfo * pLink = NULL;
	
	 if (!PEP_DESInit())
	 {
	 	 dcs_log(0,0," init des system fail");
	 	 return -1;
	 }
	 	
	 pLink = PEP_GetFreeLink();
	 if ( pLink == NULL ) 
	 {
	 	  dcs_log(0,0,"get free link fail");
	 	  return -2;
	 }
	 
	 if ( memcmp(pLink->cEncType,"01",2)==0)
	 {
	 	  i = sjl05_pos_packageDecrypt(pLink->iSocketId, tdk, data, dataLen, rtnData);
	 	  if ( i == -1 )
	 	  {		
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"Decrypt Track fail!");
	 			return -3;
	 	  }
	 	  else if ( i == -2)
	 	  {
	 			pLink->iStatus= DCS_STAT_DISCONNECTED;
	 			XPE_SendFailMessage( pLink->iNum);
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"Comm fail!");
	 			return -4;
	 	  }
	 }
	 else
	 {
	 	 PEP_FreeLink( pLink);	
	 	 dcs_log(0,0,"不支持的加密机类型");
	 	 return -5;
	 }
	 PEP_FreeLink( pLink);
	 return 0;
}

/*
	POS报文加密
	@param tdk(ascii, 32字节)
	@param data
	@param dataLen
	@param rtnData
	
	@author he.qingqi
	@date 2012-12-11 16:47:00
*/
int POS_PackageEncrypt(char *tdk, char *data, int dataLen, char *rtnData)
{
	 int i;
	 struct secLinkInfo * pLink = NULL;
	
	 if (!PEP_DESInit())
	 {
	 	 dcs_log(0,0," init des system fail");
	 	 return -1;
	 }
	 	
	 pLink = PEP_GetFreeLink();
	 if ( pLink == NULL ) 
	 {
	 	  dcs_log(0,0,"get free link fail");
	 	  return -2;
	 }
	 
	 if ( memcmp(pLink->cEncType,"01",2)==0)
	 {
	 	  i = sjl05_pos_packageEncrypt(pLink->iSocketId, tdk, data, dataLen, rtnData);
	 	  if ( i == -1 )
	 	  {		
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"Decrypt Track fail!");
	 			return -3;
	 	  }
	 	  else if ( i == -2)
	 	  {
	 			pLink->iStatus= DCS_STAT_DISCONNECTED;
	 			XPE_SendFailMessage( pLink->iNum);
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"Comm fail!");
	 			return -4;
	 	  }
	 }
	 else
	 {
	 	 PEP_FreeLink( pLink);	
	 	 dcs_log(0,0,"不支持的加密机类型");
	 	 return -5;
	 }
	 PEP_FreeLink( pLink);
	 return 0;
}



/*
	磁道解密
	@param tdk(ascii, 32字节)
	@param data(ascii, 16字节)
	@param rtnData(bcd,8字节)
	
	@author he.qingqi
	@date 2012-09-18 11:25:00
*/
int POS_TrackDecrypt(char *tdk, char *data, char *rtnData)
{
	 int i;
	 struct secLinkInfo * pLink = NULL;
	
	 if (!PEP_DESInit())
	 {
	 	 dcs_log(0,0," init des system fail");
	 	 return -1;
	 }
	 	
	 pLink = PEP_GetFreeLink();
	 if ( pLink == NULL ) 
	 {
	 	  dcs_log(0,0,"get free link fail");
	 	  return -2;
	 }
	 
	 if ( memcmp(pLink->cEncType,"01",2)==0)
	 {
	 	  i = sjl05_pos_trackdecrypt(pLink->iSocketId, tdk, data, rtnData);
	 	  if ( i == -1 )
	 	  {		
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"Decrypt Track fail!");
	 			return -3;
	 	  }
	 	  else if ( i == -2)
	 	  {
	 			pLink->iStatus= DCS_STAT_DISCONNECTED;
	 			XPE_SendFailMessage( pLink->iNum);
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"Comm fail!");
	 			return -4;
	 	  }
	 }
	 else
	 {
	 	 PEP_FreeLink( pLink);	
	 	 dcs_log(0,0,"不支持的加密机类型");
	 	 return -5;
	 }
	 PEP_FreeLink( pLink);
	 return 0;
}

int POS_PinConvert(char *pik1, int pinFormatter1, char *pik2, int pinFormatter2, char *card, char *pinBlock, char *rtnData)
{
	 int i;
	 struct secLinkInfo * pLink = NULL;
	
	 if (!PEP_DESInit())
	 {
	 	 dcs_log(0,0," init des system fail");
	 	 return -1;
	 }
	 	
	 pLink = PEP_GetFreeLink();
	 if ( pLink == NULL ) 
	 {
	 	  dcs_log(0,0,"get free link fail");
	 	  return -2;
	 }
	 
	 if ( memcmp(pLink->cEncType,"01",2)==0)
	 {
	 	  i = sjl05_pos_pinconvert(pLink->iSocketId, pik1, pinFormatter1, pik2, pinFormatter2, card, pinBlock, rtnData);
	 	  if ( i == -1 )
	 	  {		
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"Decrypt Pin fail!");
	 			return -3;
	 	  }
	 	  else if ( i == -2)
	 	  {
	 			pLink->iStatus= DCS_STAT_DISCONNECTED;
	 			XPE_SendFailMessage( pLink->iNum);
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"Comm fail!");
	 			return -4;
	 	  }
	 }
	 else
	 {
	 	 PEP_FreeLink( pLink);	
	 	 dcs_log(0,0,"不支持的加密机类型");
	 	 return -5;
	 }
	 PEP_FreeLink( pLink);
	 return 0;
}

int POS_WorkKeyGen(int keyLen, char *keyType, char *kek, char *rtnkey)
{
	 int i;
	 struct secLinkInfo * pLink = NULL;
	
	 if (!PEP_DESInit())
	 {
	 	 dcs_log(0,0," init des system fail");
	 	 return -1;
	 }
	 	
	 pLink = PEP_GetFreeLink();
	 if ( pLink == NULL ) 
	 {
	 	  dcs_log(0,0,"get free link fail");
	 	  return -2;
	 }
	 
	 if ( memcmp(pLink->cEncType,"01",2)==0)
	 {
	 	  i = sjl05_pos_workeygen(pLink->iSocketId, keyLen, keyType, kek, rtnkey);
	 	  if ( i == -1 )
	 	  {		
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"Decrypt Track fail!");
	 			return -3;
	 	  }
	 	  else if ( i == -2)
	 	  {
	 			pLink->iStatus= DCS_STAT_DISCONNECTED;
	 			XPE_SendFailMessage( pLink->iNum);
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"Comm fail!");
	 			return -4;
	 	  }
	 }
	 else
	 {
	 	 PEP_FreeLink( pLink);	
	 	 dcs_log(0,0,"不支持的加密机类型");
	 	 return -5;
	 }
	 PEP_FreeLink( pLink);
	 return 0;
}

int POS_MacCal(char *mak, char *data, char *rtnData)
{
	 int i;
	 struct secLinkInfo * pLink = NULL;
	
	 if (!PEP_DESInit())
	 {
	 	 dcs_log(0,0," init des system fail");
	 	 return -1;
	 }
	 	
	 pLink = PEP_GetFreeLink();
	 if ( pLink == NULL ) 
	 {
	 	  dcs_log(0,0,"get free link fail");
	 	  return -2;
	 }
	 
	 if ( memcmp(pLink->cEncType,"01",2)==0)
	 {
	 	  i = sjl05_pos_maccal(pLink->iSocketId, mak, data, rtnData);
	 	  if ( i == -1 )
	 	  {		
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"Decrypt Track fail!");
	 			return -3;
	 	  }
	 	  else if ( i == -2)
	 	  {
	 			pLink->iStatus= DCS_STAT_DISCONNECTED;
	 			XPE_SendFailMessage( pLink->iNum);
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"Comm fail!");
	 			return -4;
	 	  }
	 }
	 else
	 {
	 	 PEP_FreeLink( pLink);	
	 	 dcs_log(0,0,"不支持的加密机类型");
	 	 return -5;
	 }
	 PEP_FreeLink( pLink);
	 return 0;
}

/*
	pos终端mac计算，POS终端采用ＥＣＢ的加密方式
*/
int GetPospMAC( char *source, int len, char *mak, char *dest)
{
	int i, j, k, s;
	unsigned char tmp[9], tmpkey[9],workey[9];
	unsigned char buff[18], bcd[9];

	if ( source == NULL || mak == NULL )
		return -1;
		
	if ( len % 8 )
	{
		i = len / 8 + 1;
		memset( (void*) (source + len), 0x0, 8 );		//后补0x0
	}
	else
		i = len / 8;
		
	memset( (void*) bcd, 0, sizeof(bcd) );
	
	for ( j = 0; j < i; j++ )
	{
		memcpy( (void*) tmp, (void*)(source + 8*j), 8 );
		for ( k = 0; k < 8; k++ )
		{
			bcd[k] = tmp[k] ^ bcd[ k];
		}
	}
	
	memset( buff, 0, sizeof(buff));
	bcd_to_asc( (unsigned char*)buff, (unsigned char *)bcd, 16, 0 );
	
	memset( tmp, 0, sizeof(tmp));
	memset( tmpkey, 0, sizeof(tmpkey));
	memcpy(tmp, buff, 8 );
	
	s = POS_MacCal(mak, tmp, tmpkey);
	
	if(s <0)
		return -5;
		
	memset( tmp, 0, sizeof(tmp));
	memcpy(tmp, buff + 8, 8 );
	for ( k = 0; k < 8; k++ )
	{
		tmp[k] = tmp[k] ^ tmpkey[k];
	}
	memset( tmpkey, 0, sizeof(tmpkey));

	s = POS_MacCal(mak, tmp, tmpkey);
	if(s <0)
		return -5;
		
	memset( buff, 0, sizeof(buff));
	bcd_to_asc((unsigned char*)buff, (unsigned char *)tmpkey, 16, 0);
	memcpy(dest, buff, 8);	
	dest[8] = 0;
	return 0;
}

/*
	函数功能：产生密钥

	输入参数：keyIndex 		psam发卡密钥的第几组编号
			psam 			psam卡号，16字节
			random 	磁道密钥随机数,16字节
			
	输出参数：data_buf		密钥明文
*/
int EPOS_KeyGen(int keyIndex, char *psam, char *random, char *data_buf)
{
	 int i;
	 struct secLinkInfo * pLink = NULL;
	 char dest[16+1],source[16+1],tmp[32+1];
	 char _mainkey[16+1], _data_buf[16+1];
	
	 if (!PEP_DESInit())
	 {
	 	 dcs_log(0,0," init des system fail");
	 	 return -1;
	 }
	 	
	 pLink = PEP_GetFreeLink();
	 if ( pLink == NULL ) 
	 {
	 	  dcs_log(0,0,"get free link fail");
	 	  return -2;
	 }
	 
	 //if ( pLink == NULL ) 
	 //{
	 //	  dcs_log(0,0,"get free link fail");
	 //	  dcs_log(0,0,"soft encrypt start ...");
	 //	  
	 //	  memset(dest, 0 ,sizeof(dest));
	 //	  memset(tmp, 0 ,sizeof(tmp));
	 //	  memset(source, 0 ,sizeof(source));
	 //	  memcpy(tmp, psam, 16);
	 //	  memcpy(tmp+16, random, 16);
	 //	  
	 //   asc_to_bcd(source, tmp, 32, 0);
	 //   dcs_debug(source, 16, "source :");
	//
	//	  memset(tmp, 0 ,sizeof(tmp));
	//	  asc_to_bcd(tmp, "EAAE7CE19F766F9AABBA90FCC70118B8", 32, 0);
	//	  
	// 	  i = keyDispersion(source, 16, tmp, dest);
	// 	  if(i < 0)
	// 	  	return -2;
	// 	  
	// 	  memcpy(data_buf, dest, 16);
	//	  
	// 	  dcs_debug(data_buf, 16, "获取pik :");
	// 	  
	// 	  return 0; 
	// }
	 
	 if ( memcmp(pLink->cEncType,"01",2)==0)
	 {
	 	  memset(_data_buf, 0, sizeof(_data_buf));
	 	  i = sjl05_epos_keygen(pLink->iSocketId, keyIndex, psam, random, _data_buf);
	 	  if ( i == -1 )
	 	  {		
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"Decrypt Track fail!");
	 			return -3;
	 	  }
	 	  else if ( i == -2)
	 	  {
	 			pLink->iStatus= DCS_STAT_DISCONNECTED;
	 			XPE_SendFailMessage( pLink->iNum);
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"Comm fail!");
	 			return -4;
	 	  }
	 }
	 else
	 {
	 	 PEP_FreeLink( pLink);	
	 	 dcs_log(0,0,"不支持的加密机类型");
	 	 return -5;
	 }
	 PEP_FreeLink( pLink);
	 
	 asc_to_bcd( (unsigned char *) _mainkey, (unsigned char*)"E250B6F207CA14B182C4D0FE3C7920CA", 32, 0);
	 D3des( (unsigned char *) _data_buf, (unsigned char*) data_buf, (unsigned char *) _mainkey, 1 );
	 
	 return 0;
}

/*
	函数功能：磁道解密

	输入参数：keyIndex 		psam发卡密钥的第几组编号
			psam 			psam卡号，16字节
			trackRandom 	磁道密钥随机数,16字节
			track			磁道数据,bcd
			trackLen		磁道数据长度
			
	输出参数：data_buf		解密之后的磁道
*/
int EPOS_DecryptTrack(int keyIndex, char *psam, char *trackRandom, char *track, int trackLen, char *data_buf)
{
	 int i;
	 struct secLinkInfo * pLink = NULL;
	 char dest[16+1],source[16+1],tmp[32+1];
	
	 if (!PEP_DESInit())
	 {
	 	 dcs_log(0,0," init des system fail");
	 	 return -1;
	 }
	 	
	 pLink = PEP_GetFreeLink();
	 //软加密
	 //pLink = NULL;
	 if ( pLink == NULL ) 
	 {
	 	  dcs_log(0,0,"get free link fail");
		  /*
	 	  dcs_log(0,0,"soft encrypt start ...");
	 	  
	 	  memset(dest, 0 ,sizeof(dest));
	 	  memset(tmp, 0 ,sizeof(tmp));
	 	  memset(source, 0 ,sizeof(source));
	 	  memcpy(tmp, psam, 16);
	 	  memcpy(tmp+16, trackRandom, 16);
	 	  
	 	  asc_to_bcd(source, tmp, 32, 0);
	 	  
	 	  //dcs_debug(source, 16, "source :");
	
		  memset(tmp, 0 ,sizeof(tmp));
		  asc_to_bcd(tmp, "EAAE7CE19F766F9AABBA90FCC70118B8", 32, 0);
		  
	 	  i = keyDispersion(source, 16, tmp, dest);
	 	  if(i < 0)
	 	  	return -2;
	 	  
	 	  //dcs_debug(dest, 16, "磁道工作密钥 :");
	 	  
	 	  i = eposTrackUnpack(track, trackLen, data_buf, dest);
	 	  if(i < 0)
	 	  	return -2;
	 	  
	 	  //dcs_debug(data_buf, trackLen, "磁道明文 :");
	 	  //dcs_log(0,0,"soft encrypt end.");
	 	  
	 	  return 0;*/
		  return -2;
	 	  
	 }
	 
	 if ( memcmp(pLink->cEncType,"01",2)==0)
	 {
	 	  i = sjl05_decrypt_epos_track(pLink->iSocketId, keyIndex, psam, trackRandom, track, trackLen, data_buf);
	 	  if ( i == -1 )
	 	  {		
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"Decrypt Track fail!");
	 			return -3;
	 	  }
	 	  else if ( i == -2)
	 	  {
	 			pLink->iStatus= DCS_STAT_DISCONNECTED;
	 			XPE_SendFailMessage( pLink->iNum);
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"Comm fail!");
	 			return -4;
	 	  }
	 }
	 else
	 {
	 	 PEP_FreeLink( pLink);	
	 	 dcs_log(0,0,"不支持的加密机类型");
	 	 return -5;
	 }
	 PEP_FreeLink( pLink);
	 return 0;
}


int epos_hbmac(int makIndex, char *psam, char *data, int dataLen, char *rtnData)
{
	 int i;
	 struct secLinkInfo * pLink = NULL;
	
	 if (!PEP_DESInit())
	 {
	 	 dcs_log(0,0," init des system fail");
	 	 return -1;
	 }
	 	
	 pLink = PEP_GetFreeLink();
	 if ( pLink == NULL ) 
	 {
	 	  dcs_log(0,0,"get free link fail");
	 	  return -2;
	 }
	 
	 if ( memcmp(pLink->cEncType,"01",2)==0)
	 {
	 	   i = sjl05_ewphb_mac(pLink->iSocketId, makIndex, psam, data, dataLen, rtnData);
	 	  if ( i == -1 )
	 	  {		
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"epos_hbmac fail!");
	 			return -3;
	 	  }
	 	  else if ( i == -2)
	 	  {
	 			pLink->iStatus= DCS_STAT_DISCONNECTED;
	 			XPE_SendFailMessage( pLink->iNum);
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"Comm fail!");
	 			return -4;
	 	  }
	 }
	 else
	 {
	 	 PEP_FreeLink( pLink);	
	 	 dcs_log(0,0,"不支持的加密机类型");
	 	 return -5;
	 }
	 PEP_FreeLink( pLink);
	 return 0;
}


/*
	函数功能：pin转加密

	输入参数：keyIndex			发卡密钥的第几组编号
			psam 			psam卡号,16字节ascii
			pinRandom 		pik密钥随机数，16字节ascii
			enc_pin			加密的pin, 8字节bcd
			pan				主账号
			pinkeyIndex		转加密主密钥索引
			pinkey			pik，16字节ascii
			pin_convflag	主账号是否异或标志
			outpin			转加密之后的pin
			
	输出参数：outpin			转解密之后的密码
	
*/
int EPOS_PinConvert(int keyIndex, char *psam, char *pinRandom, char *enc_pin, char *pan, int pinkeyIndex, char *pinkey, int pin_convflag, char *outpin)
{
	 int i;
	 struct secLinkInfo * pLink = NULL;
	 char dest[16+1],source[16+1],tmp[32+1],innerPik[8+1];
	
	 if (!PEP_DESInit())
	 {
	 	 dcs_log(0,0," init des system fail");
	 	 return -1;
	 }
	 	
	 pLink = PEP_GetFreeLink();
	 //软加密测试
	 //pLink = NULL;
	 if ( pLink == NULL ) 
	 {
	 	  dcs_log(0,0,"get free link fail");
		  /*
	 	  dcs_log(0,0,"soft encrypt start ...");
	 	  
	 	  memset(dest, 0 ,sizeof(dest));
	 	  memset(tmp, 0 ,sizeof(tmp));
	 	  memset(source, 0 ,sizeof(source));
	 	  memset(innerPik, 0 ,sizeof(innerPik));
	 	  
	 	  memcpy(tmp, psam, 16);
	 	  memcpy(tmp+16, pinRandom, 16);
	 	  
	 	  asc_to_bcd(source, tmp, 32, 0);
	 	  
	 	  //dcs_debug(source, 16, "source :");
	 	  
	 	  memset(tmp, 0 ,sizeof(tmp));
		  asc_to_bcd(tmp, "2CDEB74919E3B1B1CB69E4FFDC273A3C", 32, 0);
		  
	 	  i = keyDispersion(source, 16, tmp, dest);
	 	  if(i < 0)
	 	  	return -2;
	 	  
	 	  //dcs_debug(dest, 16, "终端pik工作密钥明文 :");
	 	  
	 	  asc_to_bcd(innerPik, "D0DF49B070027591", 16, 0);
	 	  i = eposPinConvert(enc_pin, 8, dest, pin_convflag, innerPik, 0, pan, outpin);
	 	  if(i < 0)
	 	  	return -2;
	 	  
	 	  //dcs_debug(outpin, 8, "convert pin:"); 
	 	  //dcs_log(0,0,"soft encrypt end.");
	 	  
	 	  return 0;*/
		  return -2;
	 	  
	 }
	 
	 if ( memcmp(pLink->cEncType,"01",2)==0)
	 {
	 	  i = sjl05_pin_convert( pLink->iSocketId, keyIndex, psam, pinRandom, enc_pin, pan, pinkeyIndex, pinkey, pin_convflag, outpin);
	 	  if ( i == -1 )
	 	  {		
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"SJL05_PinConvert fail!");
	 			return -3;
	 	  }
	 	  else if ( i == -2)
	 	  {
	 			pLink->iStatus= DCS_STAT_DISCONNECTED;
	 			XPE_SendFailMessage( pLink->iNum);
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"Comm fail!");
	 			return -4;
	 	  }
	 }
	 else
	 {
	 	 PEP_FreeLink( pLink);	
	 	 dcs_log(0,0,"不支持的加密机类型");
	 	 return -5;
	 }
	 PEP_FreeLink( pLink);	
	 return 0;
}

/*
	用指定的次主密钥分散数据，并返回校验码和分散结果
*/
int sjl05_epos_keygen(int iSockId ,int keyIndex ,char *psam, char *random, char *data_buf)
{
	int rtn, i,iPid;
	int offset = 0, sndlen = 0, rcvlen, tmplen;
	char retcode[3];
	char sndbuf[1024], rcvbuf[1024],caBuf[2048];
	char tmpbuf[128];
	int qid1=-1;
	
	dcs_log(0,0, "sjl05_epos_keygen,keyIndex=%d", keyIndex);
	dcs_log(0,0, "sjl05_epos_keygen psam=%s", psam);
	dcs_log(0,0, "sjl05_epos_keygen random =%s", random);

	//if (strlen(psam) != 16 || strlen(random) != 16)
	//{
	//	dcs_log(0,0,"sjl05_epos_keygen error or data orrer!!!");
	//	return(-1);
	//}

	memset(sndbuf,0,sizeof(sndbuf));
	
	/* 指令类型 */
	memcpy(sndbuf, "B061", 4);
	offset += 4;
	
	/*用户保留字 */
	memcpy(sndbuf+offset,"0000000000000000",16);
	offset += 16;
	
	/*次主密钥索引*/
	sprintf(tmpbuf, "%04x", keyIndex);
	memcpy(sndbuf + offset, tmpbuf, 4);
	offset += 4;
	
	/*分散次数*/
	memcpy(sndbuf + offset, "02", 2);
	offset += 2;
	
	/* SAM卡序列号 */
	memcpy(sndbuf + offset, psam, 16);
	offset += 16;
	
	/*随机数*/
	memcpy(sndbuf + offset, random, 16);
	offset += 16;
	
	/*压缩数据*/
	memset(rcvbuf,0,sizeof(rcvbuf));
	asc_to_bcd(rcvbuf, sndbuf, offset, 0);
	offset = offset/2;
	memcpy(sndbuf, rcvbuf, offset);
	
	dcs_debug(sndbuf, offset, "sjl05_epos_keygen:");

    memset(caBuf,0,sizeof(caBuf));
  	iPid = getpid();
  	memcpy(caBuf,&iPid,sizeof(long));
  	sprintf(caBuf+sizeof(long),"%04d",iSockId);

  	memcpy(caBuf+sizeof(long)+5,sndbuf,offset);
  	
	if( sg_qid <0) 
  		sg_qid = queue_connect("SECURVR");
  	if ( sg_qid <0)
  	{
  		dcs_log(0,0,"connect msg queue fail!");
  		return -1;
  	}
  	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%d",iPid);
  	qid1 = queue_connect(tmpbuf);
	if(qid1 <0)
		qid1 = queue_create(tmpbuf);
  	if ( qid1 <0)
  	{
  		dcs_log(0,0,"connect msg queue fail!");
  		return -1;
  	}
  	memcpy(sndbuf,caBuf,sizeof(long));
  	queue_recv(qid1,sndbuf,sizeof(sndbuf),1);
  	if (0 >queue_send( sg_qid ,caBuf,offset+sizeof(long)+5,0))
  	{
  		sg_qid=-1;
  		dcs_log(0,0,"send msg to queue fail!");
  		return -1;
  	}

  	memset(caBuf,0,sizeof(caBuf));
  	memcpy(caBuf,&iPid,sizeof(long));
  	if ( 0 >(rcvlen=queue_recv(qid1,caBuf,sizeof(caBuf),0)))
  	{
  		qid1=-1;
  		dcs_log(0,0,"recv msg from msg queue fail!");
  		return -1;
  	}
  	
  	if ( memcmp(caBuf+sizeof(long),"FF",2)==0)
  	{
  		dcs_log(0,0," function processing fail!");
  		return -2;
  	}
	
	memcpy(rcvbuf,caBuf+sizeof(long)+2,rcvlen-sizeof(long)-2);
	
	offset = 1;
	
	if (rcvbuf[0] == 'A' )
	{
		memcpy(data_buf, rcvbuf + 9, 16);
		return(0);
	}
	else if (rcvbuf[0] == 'E')
	{
		memcpy(retcode, rcvbuf + offset + 8, 1);
		dcs_debug(rcvbuf,rcvlen,"gen key error!!");
		return(-1);
	}
	else
	{
		return(-1);
	}
}

/*
	函数功能：磁道解密

	输入参数：keyIndex 		psam发卡密钥的第几组编号
			psam 			psam卡号
			trackRandom 	磁道密钥随机数
			track			磁道数据
			trackLen		磁道数据长度

	输出参数：data_buf		解密之后的磁道

*/
int sjl05_decrypt_epos_track(int iSockId ,int keyIndex ,char *psam, char *trackRandom, char *track, int trackLen, char *data_buf)
{
	int rtn, i,iPid;
	int offset = 0, sndlen = 0, rcvlen, tmplen;
	char retcode[3];
	char sndbuf[1024], rcvbuf[1024],caBuf[2048];
	char tmpbuf[128];
	int qid1=-1;
	/*
	dcs_log(0,0, "sjl05_decrypt_epos_track,keyIndex=%d", keyIndex);
	dcs_log(0,0, "sjl05_decrypt_epos_track psam=%s", psam);
	dcs_log(0,0, "sjl05_decrypt_epos_track trackRandom =%s", trackRandom);
	dcs_log(0,0, "sjl05_decrypt_epos_track track =%s", track);
	dcs_log(0,0, "sjl05_decrypt_epos_track trackLen=%d", trackLen);
	
	dcs_debug(inbuf,inlen, "track unpack,before unpck:");
	*/
	
	if (trackLen%8 || trackLen < 8)
	{
		dcs_log(0,0,"sjl05_decrypt_epos_track error or data orrer!!!");
		return(-1);
	}
	memset(sndbuf,0,sizeof(sndbuf));
	
	/* 指令类型 */
	memcpy(sndbuf, "B092", 4);
	offset += 4;
	
	/*用户保留字 */
	memcpy(sndbuf+offset,"0000000000000000",16);
	offset += 16;	
	/*次主密钥索引*/
	sprintf(tmpbuf, "%04x", keyIndex);
	memcpy(sndbuf + offset, tmpbuf, 4);
	offset += 4;
	
	/*分散次数*/
	sprintf(tmpbuf, "%02x", 2);
	memcpy(sndbuf + offset, tmpbuf, 2);
	offset += 2;
	
	/* SAM卡序列号 */
	memcpy(sndbuf + offset, psam, 16);
	offset += 16;
	
	/*磁道密钥随机数*/
	memcpy(sndbuf + offset, trackRandom, 16);
	offset += 16;
	/*数据长度必须为8的倍数*/
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%04x",trackLen);
	memcpy(sndbuf + offset, tmpbuf, 4);
	offset += 4;
	
	/*压缩数据*/
	memset(rcvbuf,0,sizeof(rcvbuf));
	asc_to_bcd(rcvbuf, sndbuf, offset,0);
	offset = offset/2;
	memcpy(sndbuf,rcvbuf,offset);
	/*待解密数据*/
	memcpy(sndbuf + offset, track, trackLen);
	offset += trackLen;
	//dcs_debug(sndbuf, offset, "track unpck ,send to encrypt:sndbuf=%s,offset=%d",sndbuf,offset);

    memset(caBuf,0,sizeof(caBuf));
  	iPid = getpid();
  	memcpy(caBuf,&iPid,sizeof(long));
  	sprintf(caBuf+sizeof(long),"%04d",iSockId);
  	
  	//dcs_debug(0,0,"send sock id =%d",iSockId); 
  	memcpy(caBuf+sizeof(long)+5,sndbuf,offset);
	if( sg_qid <0) 
  		sg_qid = queue_connect("SECURVR");
  	if ( sg_qid <0)
  	{
  		dcs_log(0,0,"connect msg queue fail!");
  		return -1;
  	}
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%d",iPid);
  	qid1 = queue_connect(tmpbuf);
	if(qid1 <0)
		qid1 = queue_create(tmpbuf);
  	if ( qid1 <0)
  	{
  		dcs_log(0,0,"connect msg queue fail!");
  		return -1;
  	}
  	memcpy(sndbuf,caBuf,sizeof(long));
  	queue_recv(qid1,sndbuf,sizeof(sndbuf),1);
  	if (0 >queue_send( sg_qid ,caBuf,offset+sizeof(long)+5,0))
  	{
  		sg_qid=-1;
  		dcs_log(0,0,"send msg to queue fail!");
  		return -1;
  	}
  	//dcs_debug(caBuf,offset+sizeof(long)+5,"send msg to queue sendlen=%d!",offset+sizeof(long)+5);

  	//dcs_log(0,0,"send data to msg succ!");
  	memset(caBuf,0,sizeof(caBuf));
  	memcpy(caBuf,&iPid,sizeof(long));
  	if ( 0 >(rcvlen=queue_recv(qid1,caBuf,sizeof(caBuf),0)))
  	{
  		qid1=-1;
  		dcs_log(0,0,"recv msg from msg queue fail!");
  		return -1;
  	}
  	//dcs_debug(caBuf,rcvlen,"recv msg from queue succ rcvlen=%d!",rcvlen);

  	if ( memcmp(caBuf+sizeof(long),"FF",2)==0)
  	{
  		dcs_log(0,0," function processing fail!");
  		return -2;
  	}
	memcpy(rcvbuf,caBuf+sizeof(long)+2,rcvlen-sizeof(long)-2);
	
	//dcs_debug(rcvbuf, rcvlen-sizeof(long)-2, "len=%d",rcvlen-sizeof(long)-2);
	//dcs_debug(caBuf, rcvlen, "rcvlen=%d",rcvlen);
	
	offset = 1;
	//dcs_log(0,0, "rcvbuf[0]=%2x", rcvbuf[0]);
	
	if (rcvbuf[0] == 'A' )
	{
		//dcs_log(0,0, "返回A");
		/*磁道数据长度送加密机前后一样  why??*/
		/*输出域 应答码 1H用户保留字8H数据长度2H数据明文LH*/
		memset(tmpbuf,0,sizeof(tmpbuf));
		memcpy(tmpbuf,rcvbuf+1+8,2);
		//dcs_debug(tmpbuf,2, "tmpbuf");
		
		memset(caBuf,0,sizeof(caBuf));
		bcd_to_asc(caBuf, tmpbuf, 4,1);
		//dcs_debug(caBuf,4, "caBuf");
		
		sscanf(caBuf,"%x",&tmplen);
		//dcs_log(0,0, "tmplen1=%d",tmplen);
		memset(tmpbuf,0,sizeof(tmpbuf));
		sprintf(tmpbuf,"%d",tmplen);
		sscanf(tmpbuf,"%d",&tmplen);
		//dcs_log(0,0, "tmplen2=%d",tmplen);
	
		offset=1+8+2;
		//memset(data_buf,0,sizeof(data_buf));
		memcpy(data_buf, rcvbuf + offset, tmplen);

		//dcs_debug(data_buf,tmplen, "磁道明文=%s", data_buf);
		return(0);
	}
	else if (rcvbuf[0] == 'E')
	{
		memcpy(retcode, rcvbuf + offset, 1);
		dcs_debug(rcvbuf,rcvlen,"track unpck error!!");
		return(-1);
	}
	else
	{
		return(-1);
	}
}


/*
	函数功能：pin转加密

	输入参数：keyIndex			发卡密钥的第几组编号
			psam 			psam卡号
			pinRandom 		pik密钥随机数
			enc_pin			加密的pin
			pan				主账号
			pinkeyIndex		转加密主密钥索引
			pinkey			pik
			pin_convflag	主账号是否异或标志
			outpin			转加密之后的pin
			
	输出参数：outpin			转解密之后的密码
	
*/
int sjl05_pin_convert(int iSockId ,int keyIndex ,char *psam, char *pinRandom, char *enc_pin, char *pan, int pinkeyIndex, char *pinkey, int pin_convflag, char *outpin)
{
	int rtn, i,iPid;
	int offset = 0, sndlen = 0, rcvlen, tmplen;
	char retcode[3];
	char sndbuf[1024], rcvbuf[1024],caBuf[2048];
	char tmpbuf[128];
	
	/*
	dcs_log(0,0, "sjl05_pin_convert,keyIndex=%d", keyIndex);
	dcs_log(0,0, "sjl05_pin_convert psam=%s", psam);
	dcs_log(0,0, "sjl05_pin_convert pinRandom =%s", pinRandom);
	dcs_log(0,0, "sjl05_pin_convert enc_pin =%s", enc_pin);
	dcs_log(0,0, "sjl05_pin_convert pinkeyIndex=%d", pinkeyIndex);
	dcs_log(0,0, "sjl05_pin_convert pinkey =%s", pinkey);
	dcs_log(0,0, "sjl05_pin_convert pin_convflag=%d", pin_convflag);
	dcs_log(0,0, "sjl05_pin_convert pan=%s", pan);
	*/
	
	//if (strlen(psam) != 16)
	//{
	//	dcs_log(0,0,"sjl05_pin_convert error or data orrer!!!");
	//	return(-1);
	//}	
	
	memset(sndbuf,0,sizeof(sndbuf));
	
	/* 指令类型 */
	memcpy(sndbuf, "B252", 4);
	offset += 4;
	
	/*用户保留字 */
	memcpy(sndbuf+offset,"0000000000000000",16);
	offset += 16;
	
	/*次主密钥索引,网络字节序*/
	sprintf(tmpbuf, "%04x", keyIndex);
	memcpy(sndbuf + offset, tmpbuf, 4);
	offset += 4;
	/*区域主密钥索引*/
	sprintf(tmpbuf, "%04x", pinkeyIndex);
	memcpy(sndbuf + offset, tmpbuf, 4);
	offset += 4;
	
	/*分散次数*/
	sprintf(tmpbuf, "%02x", 2);
	memcpy(sndbuf + offset, tmpbuf, 2);
	offset += 2;
	
	/* SAM卡序列号 */
	memcpy(sndbuf + offset, psam, 16);
	offset += 16;
	
	/*随机数*/
	memcpy(sndbuf + offset, pinRandom, 16);
	offset += 16;
	
	/*临时密钥计算方法*/
	sprintf(tmpbuf, "%02x", 3);
	memcpy(sndbuf + offset, tmpbuf, 2);
	offset += 2;
	
	/*临时密钥计算数据*/
	//memcpy(sndbuf + offset,"0000000000000000",16);
	//offset += 16;
	
	memset(rcvbuf,0,sizeof(rcvbuf));
	asc_to_bcd(rcvbuf, sndbuf, offset,1);
	offset = offset/2;
	memcpy(sndbuf,rcvbuf,offset);
	
	
	/*转换前密文PIN*/
	memcpy(sndbuf + offset,enc_pin,8);
	offset += 8;
	
	/*PIK长度*/
	sprintf(tmpbuf, "%02x", 8);
	/*密文PIK*/
	sprintf(tmpbuf, "%s%s",tmpbuf,pinkey);	
	/*转换前PINBLOCK格式*/
	sprintf(tmpbuf, "%s%02x",tmpbuf, 1);	
	/*转换后PINBLOCK格式*/
	sprintf(tmpbuf, "%s%02x",tmpbuf, 6);
	
	memset(rcvbuf,0,sizeof(rcvbuf));
	asc_to_bcd(rcvbuf, tmpbuf, 22,1);
	memcpy(sndbuf+offset,rcvbuf,11);
	offset +=11;
	
	/*转换前主张号pan*/
	memcpy(sndbuf + offset, pan, strlen(pan));
	offset += strlen(pan);
	
	/*主账号结束符*/
	memcpy(sndbuf + offset, ";", 1);
	offset += 1;
	
	//dcs_debug(sndbuf, offset, "pin convert send to encrypt:");

    memset(caBuf,0,sizeof(caBuf));
  	iPid = getpid();
  	memcpy(caBuf,&iPid,sizeof(long));
  	sprintf(caBuf+sizeof(long),"%04d",iSockId);
  	
  	//dcs_debug(0,0,"send sock id =%d",iSockId); 

  	memcpy(caBuf+sizeof(long)+5,sndbuf,offset);
  	
	if( sg_qid <0)
  		sg_qid = queue_connect("SECURVR");
  	if ( sg_qid <0)
  	{
  		dcs_log(0,0,"connect msg queue fail!");
  		return -1;
  	}
  	int qid1=-1;
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%d",iPid);
		qid1 = queue_connect(tmpbuf);
	if(qid1 <0)
	qid1 = queue_create(tmpbuf);
	if ( qid1 <0)
	{
		dcs_log(0,0,"connect msg queue fail!");
		return -1;
	}
  	memcpy(sndbuf,caBuf,sizeof(long));
  	queue_recv(qid1,sndbuf,sizeof(sndbuf),1);
  	if (0 >queue_send( sg_qid ,caBuf,offset+sizeof(long)+5,0))
  	{
  		sg_qid=-1;
  		dcs_log(0,0,"send msg to queue fail!");
  		return -1;
  	}
  	
  	//dcs_log(0,0,"send data to msg succ!");

  	memset(caBuf,0,sizeof(caBuf));
  	memcpy(caBuf,&iPid,sizeof(long));
  	if ( 0 >(rcvlen=queue_recv(qid1,caBuf,sizeof(caBuf),0)))
  	{
  		qid1=-1;
  		dcs_log(0,0,"recv msg from msg queue fail!");
  		return -1;
  	}
  	
  	//dcs_debug(caBuf,rcvlen,"recv msg from queue succ!");

  	if ( memcmp(caBuf+sizeof(long),"FF",2)==0)
  	{
  		dcs_log(0,0," function processing fail!");
  		return -2;
  	}
	
	memcpy(rcvbuf,caBuf+sizeof(long)+2,rcvlen-sizeof(long)+2);
	
	//dcs_debug(rcvbuf, rcvlen-sizeof(long)+2, "rcvbuf");
	//dcs_debug(caBuf, rcvlen, "caBuf");
	
	offset = 1;
	//dcs_log(0,0, "rcvbuf[0]=%2x", rcvbuf[0]);
	
	if (rcvbuf[0] == 'A' )
	{
		//dcs_log(0,0, "转加密pin=%s", rcvbuf);
		offset+=8;/*越过8个长度的用户保留字*/
		memcpy(outpin, rcvbuf + offset, 8);
		return(0);
	}
	else if (rcvbuf[0] == 'E')
	{
		memcpy(retcode, rcvbuf + 9, 1);
		dcs_debug(rcvbuf,rcvlen,"pin unpck error!!");
		return(-1);
	}
	else
	{
		return(-1);
	}
}


/*
	两次分散函数
	source：分散数据
	len：分散数据长度
	inkey：密钥，明文，16字节bcd压缩
	dest：分散结果

	操作步骤：
	1、判断len是否为16，否则返回错误-1
	2、用inkey对source做2次密钥分散操作等到dest
*/
int keyDispersion(char *source, int len, char *inkey, char *dest)
{	
	char tmp1[8+1],tmp2[8+1],key[16+1];
	char out_tmp1[8+1],out_tmp2[8+1];
	int t = 0;
	
	memset(tmp1, 0 ,sizeof(tmp1));
	memset(tmp2, 0 ,sizeof(tmp2));
	memset(out_tmp1, 0 ,sizeof(out_tmp1));
	memset(out_tmp2, 0 ,sizeof(out_tmp2));
	memset(key, 0 ,sizeof(key));
	
	memcpy(tmp1, source, 8);
	memcpy(key, inkey, 16);
	
	//dcs_debug(key, 16, "发卡母卡密钥：");
	
	for(t = 0; t< 8; t++)
		tmp2[t] = ~tmp1[t];
	
	//dcs_debug(tmp1, 8, "tmp1：");
	//dcs_debug(tmp2, 8, "tmp2：");
	
	//dcs_debug(key, 16, "key1：");
	D3des( (unsigned char *) tmp1, (unsigned char*) out_tmp1, (unsigned char *) key, 1 );
	
	//dcs_debug(out_tmp1, 8, "tmp1 加密结果：");
	
	//dcs_debug(key, 16, "key1：");
	//dcs_debug(inkey, 16, "inkey：");
	D3des( (unsigned char *) tmp2, (unsigned char*) out_tmp2, (unsigned char *) inkey, 1 );
	
	//dcs_debug(out_tmp2, 8, "tmp2 加密结果：");
	
	memset(key, 0 ,sizeof(key));
	memcpy(key, out_tmp1, 8);
	memcpy(key+8, out_tmp2, 8);
	
	//dcs_debug(key, 16, "新密钥：");
	
	memset(tmp1, 0 ,sizeof(tmp1));
	memset(tmp2, 0 ,sizeof(tmp2));
	memset(out_tmp1, 0 ,sizeof(out_tmp1));
	memset(out_tmp2, 0 ,sizeof(out_tmp2));
	
	memcpy(tmp1, source+8, 8);
	
	//dcs_debug(tmp1, 8, "新tmp1：");
	for(t = 0; t< 8; t++)
		tmp2[t] = ~tmp1[t];
	//dcs_debug(tmp2, 8, "新tmp2：");
	
	//dcs_debug(key, 16, "key2：");
	D3des( (unsigned char *) tmp1, (unsigned char*) out_tmp1, (unsigned char *) key, 1 );
	
	//dcs_debug(out_tmp1, 8, "新tmp1 加密结果：");
	//dcs_debug(key, 16, "key2：");
	D3des( (unsigned char *) tmp2, (unsigned char*) out_tmp2, (unsigned char *) key, 1 );
	
	//dcs_debug(out_tmp2, 8, "新tmp2 加密结果：");
	
	memcpy(dest, out_tmp1, 8);
	memcpy(dest+8, out_tmp2, 8);
	dest[16] = 0;
	
	//dcs_debug(dest, 16, "工作密钥：");
	
	return 0;
}

/*
	移动刷卡终端磁道解密
	
	source：磁道密文
	len：磁道密文长度
	dest：磁道明文
	inkey：磁道密钥，明文，16字节bcd压缩
	
	3des解密，ecb方式
	
	处理流程：
	1、如果len不为8的倍数，返回错误
	2、以8字节为单位切分source
	3、用inkey对每8字节解密
	4、把所有解密数据组合成返回数据
*/
int eposTrackUnpack(char *source, int len, char *dest, char *inkey)
{
	char ascbuf[8+1],tmpbuf[8+1];
	int i=0;
	if(len%8!=0)
		return -1;
		
	//dcs_debug(inkey, 16, "磁道解密工作密钥：");
	//dcs_debug(source, len, "磁道：");
	
	for(i=0;i<len;i+=8)
	{
		memset(tmpbuf,0,sizeof(tmpbuf));
		memset(ascbuf,0,sizeof(ascbuf));
		
		memcpy(tmpbuf,source+i,8);
		
		//dcs_debug(inkey, 16, "磁道解密工作密钥：");
		//dcs_debug(tmpbuf, 8, "解密数据：");
		
		D3des( (unsigned char *) tmpbuf, (unsigned char*) ascbuf, (unsigned char *) inkey, 0 );
		//dcs_debug(ascbuf, 8, "解密结果：");
		memcpy(dest+i, ascbuf, 8);		
	}
	return 0;
}

/*
	移动刷卡终端Pin转解密

	source：pin密文
	len：pin密文长度
	inkey：解密pik，明文，16字节bcd压缩，3des解密
	in_cardflag：加密时主账号异或情况，0：不异或，1：异或
	outkey：加密pik，明文，8字节bcd压缩，des加密
	out_cardflag:转加密pin时主账号异或情况，0：不异或，1：异或
	pan：主账号
	dest：转加密之后的pin
	
	处理流程：
	1、如果len不为8的倍数，返回错误
	2、用inkey对source 3des解密 ，得到tmp明文
	3、判断in_cardflag和out_cardflag，只要其中之一为1，那么对pan做处理，从pan的倒数第2位开始向前取12个字节，然后bcd压缩成6个字节，再在最前面添加两个字节0x00，组成8个字节的pan
	4、判断in_cardflag，如果是1，那么tmp与 做异或，否则不异或，得到tmp2
	5、判断out_cardflag，如果是1，那么tmp2与pan做异或，否则不异或，得到tmp3
	6、用outkey des加密tmp3得到dest
*/
int eposPinConvert(char *source, int len, char *inkey, int in_cardflag, char *outkey, int out_cardflag, char *pan, char *dest)
{	
	char tmp_pin[8+1],bcdbuf[16+1],tmp_pan[8+1];
	int i=0,s;
	
	if(len%8!=0)
		return -1;

	if(in_cardflag==1||out_cardflag==1)
	{
		memset( bcdbuf, 0x0, sizeof(bcdbuf) );
		memset( tmp_pan, 0x0, sizeof(tmp_pan) );
		
		sprintf( bcdbuf, "0000000000000000" );
		i = getstrlen( pan );
		if ( i > 1 )
		{
			s = i;
			for ( i = 1; i <= 12; i++ )
			{
				if ( (s - i) <= 0 )
					break;
				bcdbuf[ 16 - i] = pan[s - i - 1];
			}
		}
		memset( tmp_pan, 0x0, sizeof(tmp_pan) );
		asc_to_bcd( (unsigned char*) tmp_pan, (unsigned char*) bcdbuf, 16, 0 );
	}
	
	//dcs_debug( tmp_pan, 8, "PAN:");
	//dcs_debug( source, 8, "转加密之前的PIN密文:");
	
	memset( tmp_pin, 0x0, sizeof(tmp_pin) );
	D3des( (unsigned char *) source, (unsigned char*) tmp_pin, (unsigned char *) inkey, 0 );
	
	//dcs_debug( tmp_pin, 8, "PIN 明文:");
	
	if(in_cardflag == 1)
	{
		for(i=0;i<8;i++)
			tmp_pin[i] = tmp_pan[i] ^ tmp_pin[i];
			
		//dcs_debug( tmp_pin, 8, "PIN xor1 tmp_pin:");
	}
	
	if(out_cardflag == 1)
	{
		for(i=0;i<8;i++)
			tmp_pin[i] = tmp_pan[i] ^ tmp_pin[i];
		
		//dcs_debug( tmp_pin, 8, "PIN xor2 tmp_pin:");
	}
	
	//dcs_debug( outkey, 16, "转加密Pik明文:");
	//D3des( (unsigned char *) tmp_pin, (unsigned char*) dest, (unsigned char *) outkey, 1 );
	des( (unsigned char *) tmp_pin, (unsigned char*) dest, (unsigned char *) outkey, 1 );
	dest[8] = 0;
	
	//dcs_debug( dest, 8, "PIN dest:");
	return 0;
}

int GetPin( char * PinBlk, int len, char * cardno, char * pin )
{
	char	buffer[1024], newbuf[1024];
	char    tmpbuf[200], bcdbuf[200], card[20];
	int	    i, s;
	unsigned char inkey[33], mainkey[33];

	memset( inkey, 0x0, sizeof(inkey) );
	memset( bcdbuf, 0x0, sizeof(bcdbuf) );
	memset( mainkey, 0x0, sizeof(mainkey) );
	if ( strlen( PIN_KEY ) > 16 )
	{
		asc_to_bcd( (unsigned char *) bcdbuf, (unsigned char*) PIN_KEY, 32, 0);
		asc_to_bcd( (unsigned char *) mainkey, (unsigned char*) MAIN_KEY, 32, 0);
		D3des( (unsigned char *) bcdbuf, (unsigned char*) inkey, (unsigned char *) mainkey, 0 );
	} else
	{
		asc_to_bcd( (unsigned char *) bcdbuf, (unsigned char*) PIN_KEY, 16, 0);
		asc_to_bcd( (unsigned char *) mainkey, (unsigned char*) MAIN_KEY, 16, 0);
		des( (unsigned char *) bcdbuf, (unsigned char*) inkey, (unsigned char *) mainkey, 0 );
	}
	dcs_debug( inkey, 16, "PIN PIN_KEY=%s.", PIN_KEY );

	memset( buffer, 0x0, sizeof(buffer) );
	sprintf( buffer, "%02d%sFFFFFFFFFFFFFFFFFFFFFFFF", len, PinBlk );
	dcs_log( 0, 0, "PinBlk=%s, len=%d", buffer, strlen(buffer) );
	memset( tmpbuf, 0x0, sizeof(tmpbuf) );
	asc_to_bcd( (unsigned char*) tmpbuf, (unsigned char*) buffer, 16, 0 );
	dcs_debug( tmpbuf, 9, "PinBlk" );

	memset( bcdbuf, 0x0, sizeof(bcdbuf) );
	sprintf( bcdbuf, "0000000000000000" );
	i = getstrlen( cardno );
	if ( i > 1 )
	{
		s = i;
		for ( i = 1; i <= 12; i++ )
		{
			if ( (s - i) <= 0 )
				break;
			bcdbuf[ 16 - i] = cardno[s - i - 1];
		}
	}
	dcs_debug( bcdbuf, 16, "bcdbuf" );
	memset( card, 0x0, sizeof(card) );
	asc_to_bcd( (unsigned char*) card, (unsigned char*) bcdbuf, 16, 0 );
	dcs_debug( card, 9, "card" );
	for ( i = 0; i < 8; i++ )
	{
		tmpbuf[i] = tmpbuf[i] ^ card[i];
	}
	dcs_debug( tmpbuf, 9, "PinBlk1" );
	memset( bcdbuf, 0x0, sizeof(bcdbuf) );
	if ( strlen( PIN_KEY ) > 16 )
	{
		D3des( (unsigned char*)tmpbuf, (unsigned char*)bcdbuf, (unsigned char*)inkey, 1 );
	} else
	{
		des( (unsigned char*)tmpbuf, (unsigned char*)bcdbuf, (unsigned char*)inkey, 1 );
	}
	dcs_debug( bcdbuf, 8, "Get Pin" );
	memcpy( pin, bcdbuf, 8 );
	return 8;
}


/*
	POS终端校验计算mac
	source：计算mac的数据
	len：数据长度
	inkey：mak明文，8字节压缩
	dest：8字节mac数据，bcd表示
*/
int PosMAC(  unsigned char *source, int len, unsigned char * dest,unsigned char * inkey )
{
	int i, j, k;
	unsigned char tmp[8], bcd[8];
	unsigned char buff[18];

	if ( source == NULL || dest == NULL || inkey == NULL )
		return -1;
	if ( len % 8 )
	{
		i = len / 8 + 1;
		memset( (void*) (source + len), 0x0, 8 );		// 后补0x0
	}
	else
		i = len / 8;
	memset( (void*) bcd, 0x0, sizeof(bcd) );
	
	for ( j = 0; j < i; j++ )
	{
		memcpy( (void*) tmp, (void*)(source + 8*j), 8 );
		
		for ( k = 0; k < 8; k++ )
		{
			bcd[k] = tmp[k] ^ bcd[ k];
		}
	}

	memset( buff, 0x0, sizeof( buff ) );
	bcd_to_asc( (unsigned char*) buff, bcd, 16, 0 );
	
	//tmp后8字节数据
	memcpy( tmp, buff + 8, 8 );

	//前8字节des加密
	if ( des( buff, bcd, inkey, 1 ) )
		return -1;
		
	//前8字节的des加密结果与后8字节数据异或
	for ( k = 0; k < 8; k++ )
	{
		tmp[k] = tmp[k] ^ bcd[ k];
	}
	
	//异或结果再次des加密
	if ( des( tmp, bcd, inkey, 1 ) )
		return -1;

	memcpy( (void*) dest, bcd, 8 );
	dest[8] = 0;
	
	return 0;
}

/*
	单倍长密钥计算
	source：计算mac的数据
	len：数据长度
	inkey：mak明文，8字节压缩
	dest：8字节mac数据，bcd表示
	
	mac计算采用CBC方式，ANSI X9.9
	
*/
int MAC( unsigned char *source, int len, unsigned char *dest, unsigned char *inkey )
{
	int i, j, k;
	unsigned char tmp[8], bcd[8];
	unsigned char buff[18];

	if ( source == NULL || dest == NULL || inkey == NULL )
		return -1;
	if ( len % 8 )
	{
		i = len / 8 + 1;
		memset( (void*) (source + len), 0x0, 8 );		// 后补0x0
	}
	else
		i = len / 8;
	memset( (void*) bcd, 0x0, sizeof(bcd) );
	for ( j = 0; j < i; j++ )
	{
		memcpy( (void*) tmp, (void*)(source + 8*j), 8 );
		for ( k = 0; k < 8; k++ )
		{
			tmp[k] = tmp[k] ^ bcd[ k];
		}
		if ( des( tmp, bcd, inkey, 1 ))
			return -1;
	}
	memcpy( (void*) dest, bcd, 8 );
	dest[8] = 0;
	return 0;
}


/* ================================================================ 
des() Description: DES algorithm,do 加密 (1) or 解密 (0).
================================================================ */ 
int des(unsigned char *source,unsigned char * dest,unsigned char * inkey, int flg) 
{ 
	unsigned char bufout[64], 
	kwork[56], worka[48], kn[48], buffer[64], key[64], 
	nbrofshift, temp1, temp2; 
	int valindex; 
	int i, j, k, iter; 

	if ( source == NULL || dest == NULL || inkey == NULL )
		return -1;
	/* INITIALIZE THE TABLES */ 
	/* Table - s1 */ 
	static unsigned char s1[4][16] = { 
					14, 4, 13, 1, 2, 15, 11, 8, 3, 10, 6, 12, 5, 9, 0, 7, 
					0, 15, 7, 4, 14, 2, 13, 1, 10, 6, 12, 11, 9, 5, 3, 8, 
					4, 1, 14, 8, 13, 6, 2, 11, 15, 12, 9, 7, 3, 10, 5, 0, 
					15, 12, 8, 2, 4, 9, 1, 7, 5, 11, 3, 14, 10, 0, 6, 13 }; 
	/* Table - s2 */ 
	static unsigned char s2[4][16] = { 
					15, 1, 8, 14, 6, 11, 3, 4, 9, 7, 2, 13, 12, 0, 5, 10, 
					3, 13, 4, 7, 15, 2, 8, 14, 12, 0, 1, 10, 6, 9, 11, 5, 
					0, 14, 7, 11, 10, 4, 13, 1, 5, 8, 12, 6, 9, 3, 2, 15, 
					13, 8, 10, 1, 3, 15, 4, 2, 11, 6, 7, 12, 0, 5, 14, 9 }; 
	/* Table - s3 */ 
	static unsigned char s3[4][16] = { 
					10, 0, 9, 14, 6, 3, 15, 5, 1, 13, 12, 7, 11, 4, 2, 8, 
					13, 7, 0, 9, 3, 4, 6, 10, 2, 8, 5, 14, 12, 11, 15, 1, 
					13, 6, 4, 9, 8, 15, 3, 0, 11, 1, 2, 12, 5, 10, 14, 7, 
					1, 10, 13, 0, 6, 9, 8, 7, 4, 15, 14, 3, 11, 5, 2, 12 }; 
	/* Table - s4 */ 
	static unsigned char s4[4][16] = { 
					7, 13, 14, 3, 0, 6, 9, 10, 1, 2, 8, 5, 11, 12, 4, 15, 
					13, 8, 11, 5, 6, 15, 0, 3, 4, 7, 2, 12, 1, 10, 14, 9, 
					10, 6, 9, 0, 12, 11, 7, 13, 15, 1, 3, 14, 5, 2, 8, 4, 
					3, 15, 0, 6, 10, 1, 13, 8, 9, 4, 5, 11, 12, 7, 2, 14 }; 
	/* Table - s5 */ 
	static unsigned char s5[4][16] = { 
					2, 12, 4, 1, 7, 10, 11, 6, 8, 5, 3, 15, 13, 0, 14, 9, 
					14, 11, 2, 12, 4, 7, 13, 1, 5, 0, 15, 10, 3, 9, 8, 6, 
					4, 2, 1, 11, 10, 13, 7, 8, 15, 9, 12, 5, 6, 3, 0, 14, 
					11, 8, 12, 7, 1, 14, 2, 13, 6, 15, 0, 9, 10, 4, 5, 3 }; 
	/* Table - s6 */ 
	static unsigned char s6[4][16] = { 
					12, 1, 10, 15, 9, 2, 6, 8, 0, 13, 3, 4, 14, 7, 5, 11, 
					10, 15, 4, 2, 7, 12, 9, 5, 6, 1, 13, 14, 0, 11, 3, 8, 
					9, 14, 15, 5, 2, 8, 12, 3, 7, 0, 4, 10, 1, 13, 11, 6, 
					4, 3, 2, 12, 9, 5, 15, 10, 11, 14, 1, 7, 6, 0, 8, 13 }; 
	/* Table - s7 */ 
	static unsigned char s7[4][16] = { 
					4, 11, 2, 14, 15, 0, 8, 13, 3, 12, 9, 7, 5, 10, 6, 1, 
					13, 0, 11, 7, 4, 9, 1, 10, 14, 3, 5, 12, 2, 15, 8, 6, 
					1, 4, 11, 13, 12, 3, 7, 14, 10, 15, 6, 8, 0, 5, 9, 2, 
					6, 11, 13, 8, 1, 4, 10, 7, 9, 5, 0, 15, 14, 2, 3, 12 }; 
	/* Table - s8 */ 
	static unsigned char s8[4][16] = { 
					13, 2, 8, 4, 6, 15, 11, 1, 10, 9, 3, 14, 5, 0, 12, 7, 
					1, 15, 13, 8, 10, 3, 7, 4, 12, 5, 6, 11, 0, 14, 9, 2, 
					7, 11, 4, 1, 9, 12, 14, 2, 0, 6, 10, 13, 15, 3, 5, 8, 
					2, 1, 14, 7, 4, 10, 8, 13, 15, 12, 9, 0, 3, 5, 6, 11 }; 
	
	/* Table - Shift */ 
	static unsigned char shift[16] = { 
	1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1 }; 
	
	/* Table - Binary */ 
	static unsigned char binary[64] = { 
					0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 1, 
					0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 
					1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 
					1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1 }; 
	/* MAIN PROCESS */ 
	/* Convert from 64-bit key into 64-byte key */ 
	for (i = 0; i < 8; i++) { 
		key[8*i] = ((j = *(inkey + i)) / 128) % 2; 
		key[8*i+1] = (j / 64) % 2; 
		key[8*i+2] = (j / 32) % 2; 
		key[8*i+3] = (j / 16) % 2; 
		key[8*i+4] = (j / 8) % 2; 
		key[8*i+5] = (j / 4) % 2; 
		key[8*i+6] = (j / 2) % 2; 
		key[8*i+7] = j % 2; 
	} 
	/* Convert from 64-bit data into 64-byte data */ 
	for (i = 0; i < 8; i++) { 
		buffer[8*i] = ((j = *(source + i)) / 128) % 2; 
		buffer[8*i+1] = (j / 64) % 2; 
		buffer[8*i+2] = (j / 32) % 2; 
		buffer[8*i+3] = (j / 16) % 2; 
		buffer[8*i+4] = (j / 8) % 2; 
		buffer[8*i+5] = (j / 4) % 2; 
		buffer[8*i+6] = (j / 2) % 2; 
		buffer[8*i+7] = j % 2; 
	} 
	/* Initial Permutation of Data */ 
	bufout[ 0] = buffer[57]; 
	bufout[ 1] = buffer[49]; 
	bufout[ 2] = buffer[41]; 
	bufout[ 3] = buffer[33]; 
	bufout[ 4] = buffer[25]; 
	bufout[ 5] = buffer[17]; 
	bufout[ 6] = buffer[ 9]; 
	bufout[ 7] = buffer[ 1]; 
	bufout[ 8] = buffer[59]; 
	bufout[ 9] = buffer[51]; 
	bufout[10] = buffer[43]; 
	bufout[11] = buffer[35]; 
	bufout[12] = buffer[27]; 
	bufout[13] = buffer[19]; 
	bufout[14] = buffer[11]; 
	bufout[15] = buffer[ 3]; 
	bufout[16] = buffer[61]; 
	bufout[17] = buffer[53]; 
	bufout[18] = buffer[45]; 
	bufout[19] = buffer[37]; 
	bufout[20] = buffer[29]; 
	bufout[21] = buffer[21]; 
	bufout[22] = buffer[13]; 
	bufout[23] = buffer[ 5]; 
	bufout[24] = buffer[63]; 
	bufout[25] = buffer[55]; 
	bufout[26] = buffer[47]; 
	bufout[27] = buffer[39]; 
	bufout[28] = buffer[31]; 
	bufout[29] = buffer[23]; 
	bufout[30] = buffer[15]; 
	bufout[31] = buffer[ 7]; 
	bufout[32] = buffer[56]; 
	bufout[33] = buffer[48]; 
	bufout[34] = buffer[40]; 
	bufout[35] = buffer[32]; 
	bufout[36] = buffer[24]; 
	bufout[37] = buffer[16]; 
	bufout[38] = buffer[ 8]; 
	bufout[39] = buffer[ 0]; 
	bufout[40] = buffer[58]; 
	bufout[41] = buffer[50]; 
	bufout[42] = buffer[42]; 
	bufout[43] = buffer[34]; 
	bufout[44] = buffer[26]; 
	bufout[45] = buffer[18]; 
	bufout[46] = buffer[10]; 
	bufout[47] = buffer[ 2]; 
	bufout[48] = buffer[60]; 
	bufout[49] = buffer[52]; 
	bufout[50] = buffer[44]; 
	bufout[51] = buffer[36]; 
	bufout[52] = buffer[28]; 
	bufout[53] = buffer[20]; 
	bufout[54] = buffer[12]; 
	bufout[55] = buffer[ 4]; 
	bufout[56] = buffer[62]; 
	bufout[57] = buffer[54]; 
	bufout[58] = buffer[46]; 
	bufout[59] = buffer[38]; 
	bufout[60] = buffer[30]; 
	bufout[61] = buffer[22]; 
	bufout[62] = buffer[14]; 
	bufout[63] = buffer[ 6]; 
	/* Initial Permutation of Key */ 
	kwork[ 0] = key[56]; 
	kwork[ 1] = key[48]; 
	kwork[ 2] = key[40]; 
	kwork[ 3] = key[32]; 
	kwork[ 4] = key[24]; 
	kwork[ 5] = key[16]; 
	kwork[ 6] = key[ 8]; 
	kwork[ 7] = key[ 0]; 
	kwork[ 8] = key[57]; 
	kwork[ 9] = key[49]; 
	kwork[10] = key[41]; 
	kwork[11] = key[33]; 
	kwork[12] = key[25]; 
	kwork[13] = key[17]; 
	kwork[14] = key[ 9]; 
	kwork[15] = key[ 1]; 
	kwork[16] = key[58]; 
	kwork[17] = key[50]; 
	kwork[18] = key[42]; 
	kwork[19] = key[34]; 
	kwork[20] = key[26]; 
	kwork[21] = key[18]; 
	kwork[22] = key[10]; 
	kwork[23] = key[ 2]; 
	kwork[24] = key[59]; 
	kwork[25] = key[51]; 
	kwork[26] = key[43]; 
	kwork[27] = key[35]; 
	kwork[28] = key[62]; 
	kwork[29] = key[54]; 
	kwork[30] = key[46]; 
	kwork[31] = key[38]; 
	kwork[32] = key[30]; 
	kwork[33] = key[22]; 
	kwork[34] = key[14]; 
	kwork[35] = key[ 6]; 
	kwork[36] = key[61]; 
	kwork[37] = key[53]; 
	kwork[38] = key[45]; 
	kwork[39] = key[37]; 
	kwork[40] = key[29]; 
	kwork[41] = key[21]; 
	kwork[42] = key[13]; 
	kwork[43] = key[ 5]; 
	kwork[44] = key[60]; 
	kwork[45] = key[52]; 
	kwork[46] = key[44]; 
	kwork[47] = key[36]; 
	kwork[48] = key[28]; 
	kwork[49] = key[20]; 
	kwork[50] = key[12]; 
	kwork[51] = key[ 4]; 
	kwork[52] = key[27]; 
	kwork[53] = key[19]; 
	kwork[54] = key[11]; 
	kwork[55] = key[ 3]; 
	/* 16 Iterations */ 
	for (iter = 1; iter < 17; iter++) { 
		for (i = 0; i < 32; i++) 
		{
			buffer[i] = bufout[32+i]; 
		}
		/* Calculation of F(R, K) */ 
		/* Permute - E */ 
		worka[ 0] = buffer[31]; 
		worka[ 1] = buffer[ 0]; 
		worka[ 2] = buffer[ 1]; 
		worka[ 3] = buffer[ 2]; 
		worka[ 4] = buffer[ 3]; 
		worka[ 5] = buffer[ 4]; 
		worka[ 6] = buffer[ 3]; 
		worka[ 7] = buffer[ 4]; 
		worka[ 8] = buffer[ 5]; 
		worka[ 9] = buffer[ 6]; 
		worka[10] = buffer[ 7]; 
		worka[11] = buffer[ 8]; 
		worka[12] = buffer[ 7]; 
		worka[13] = buffer[ 8]; 
		worka[14] = buffer[ 9]; 
		worka[15] = buffer[10]; 
		worka[16] = buffer[11]; 
		worka[17] = buffer[12]; 
		worka[18] = buffer[11]; 
		worka[19] = buffer[12]; 
		worka[20] = buffer[13]; 
		worka[21] = buffer[14]; 
		worka[22] = buffer[15]; 
		worka[23] = buffer[16]; 
		worka[24] = buffer[15]; 
		worka[25] = buffer[16]; 
		worka[26] = buffer[17]; 
		worka[27] = buffer[18]; 
		worka[28] = buffer[19]; 
		worka[29] = buffer[20]; 
		worka[30] = buffer[19]; 
		worka[31] = buffer[20]; 
		worka[32] = buffer[21]; 
		worka[33] = buffer[22]; 
		worka[34] = buffer[23]; 
		worka[35] = buffer[24]; 
		worka[36] = buffer[23]; 
		worka[37] = buffer[24]; 
		worka[38] = buffer[25]; 
		worka[39] = buffer[26]; 
		worka[40] = buffer[27]; 
		worka[41] = buffer[28]; 
		worka[42] = buffer[27]; 
		worka[43] = buffer[28]; 
		worka[44] = buffer[29]; 
		worka[45] = buffer[30]; 
		worka[46] = buffer[31]; 
		worka[47] = buffer[ 0]; 
		/* KS Function Begin */ 
		if (flg) { 
			 nbrofshift = shift[iter-1]; 
			 for (i = 0; i < (int) nbrofshift; i++) { 
				  temp1 = kwork[0]; 
				  temp2 = kwork[28]; 
				  for (j = 0; j < 27; j++) { 
					   kwork[j] = kwork[j+1]; 
					   kwork[j+28] = kwork[j+29]; 
				  } 
				  kwork[27] = temp1; 
				  kwork[55] = temp2; 
			 } 
		} else if (iter > 1) { 
			 nbrofshift = shift[17-iter]; 
			 for (i = 0; i < (int) nbrofshift; i++) { 
				  temp1 = kwork[27]; 
				  temp2 = kwork[55]; 
				  for (j = 27; j > 0; j--) { 
					   kwork[j] = kwork[j-1]; 
					   kwork[j+28] = kwork[j+27]; 
				  } 
				  kwork[0] = temp1; 
				  kwork[28] = temp2; 
				 } 
		} 
		/* Permute kwork - PC2 */ 
		kn[ 0] = kwork[13]; 
		kn[ 1] = kwork[16]; 
		kn[ 2] = kwork[10]; 
		kn[ 3] = kwork[23]; 
		kn[ 4] = kwork[ 0]; 
		kn[ 5] = kwork[ 4]; 
		kn[ 6] = kwork[ 2]; 
		kn[ 7] = kwork[27]; 
		kn[ 8] = kwork[14]; 
		kn[ 9] = kwork[ 5]; 
		kn[10] = kwork[20]; 
		kn[11] = kwork[ 9]; 
		kn[12] = kwork[22]; 
		kn[13] = kwork[18]; 
		kn[14] = kwork[11]; 
		kn[15] = kwork[ 3]; 
		kn[16] = kwork[25]; 
		kn[17] = kwork[ 7]; 
		kn[18] = kwork[15]; 
		kn[19] = kwork[ 6]; 
		kn[20] = kwork[26]; 
		kn[21] = kwork[19]; 
		kn[22] = kwork[12]; 
		kn[23] = kwork[ 1]; 
		kn[24] = kwork[40]; 
		kn[25] = kwork[51]; 
		kn[26] = kwork[30]; 
		kn[27] = kwork[36]; 
		kn[28] = kwork[46]; 
		kn[29] = kwork[54]; 
		kn[30] = kwork[29]; 
		kn[31] = kwork[39]; 
		kn[32] = kwork[50]; 
		kn[33] = kwork[44]; 
		kn[34] = kwork[32]; 
		kn[35] = kwork[47]; 
		kn[36] = kwork[43]; 
		kn[37] = kwork[48]; 
		kn[38] = kwork[38]; 
		kn[39] = kwork[55]; 
		kn[40] = kwork[33]; 
		kn[41] = kwork[52]; 
		kn[42] = kwork[45]; 
		kn[43] = kwork[41]; 
		kn[44] = kwork[49]; 
		kn[45] = kwork[35]; 
		kn[46] = kwork[28]; 
		kn[47] = kwork[31]; 
		/* KS Function End */ 
		/* worka XOR kn */ 
		for (i = 0; i < 48; i++) 
			worka[i] = worka[i] ^ kn[i]; 
		/* 8 s-functions */ 
		valindex = s1[2*worka[ 0]+worka[ 5]] 
		[2*(2*(2*worka[ 1]+worka[ 2])+ 
		worka[ 3])+worka[ 4]]; 
		valindex = valindex * 4; 
		kn[ 0] = binary[0+valindex]; 
		kn[ 1] = binary[1+valindex]; 
		kn[ 2] = binary[2+valindex]; 
		kn[ 3] = binary[3+valindex]; 
		valindex = s2[2*worka[ 6]+worka[11]] 
		[2*(2*(2*worka[ 7]+worka[ 8])+ 
		worka[ 9])+worka[10]]; 
		valindex = valindex * 4; 
		kn[ 4] = binary[0+valindex]; 
		kn[ 5] = binary[1+valindex]; 
		kn[ 6] = binary[2+valindex]; 
		kn[ 7] = binary[3+valindex]; 
		valindex = s3[2*worka[12]+worka[17]] 
		[2*(2*(2*worka[13]+worka[14])+ 
		worka[15])+worka[16]]; 
		valindex = valindex * 4; 
		kn[ 8] = binary[0+valindex]; 
		kn[ 9] = binary[1+valindex]; 
		kn[10] = binary[2+valindex]; 
		kn[11] = binary[3+valindex]; 
		valindex = s4[2*worka[18]+worka[23]] 
		[2*(2*(2*worka[19]+worka[20])+ 
		worka[21])+worka[22]]; 
		valindex = valindex * 4; 
		kn[12] = binary[0+valindex]; 
		kn[13] = binary[1+valindex]; 
		kn[14] = binary[2+valindex]; 
		kn[15] = binary[3+valindex]; 
		valindex = s5[2*worka[24]+worka[29]] 
		[2*(2*(2*worka[25]+worka[26])+ 
		worka[27])+worka[28]]; 
		valindex = valindex * 4; 
		kn[16] = binary[0+valindex]; 
		kn[17] = binary[1+valindex]; 
		kn[18] = binary[2+valindex]; 
		kn[19] = binary[3+valindex]; 
		valindex = s6[2*worka[30]+worka[35]] 
		[2*(2*(2*worka[31]+worka[32])+ 
		worka[33])+worka[34]]; 
		valindex = valindex * 4; 
		kn[20] = binary[0+valindex]; 
		kn[21] = binary[1+valindex]; 
		kn[22] = binary[2+valindex]; 
		kn[23] = binary[3+valindex]; 
		valindex = s7[2*worka[36]+worka[41]] 
		[2*(2*(2*worka[37]+worka[38])+ 
		worka[39])+worka[40]]; 
		valindex = valindex * 4; 
		kn[24] = binary[0+valindex]; 
		kn[25] = binary[1+valindex]; 
		kn[26] = binary[2+valindex]; 
		kn[27] = binary[3+valindex]; 
		valindex = s8[2*worka[42]+worka[47]] 
		[2*(2*(2*worka[43]+worka[44])+ 
		worka[45])+worka[46]]; 
		valindex = valindex * 4; 
		kn[28] = binary[0+valindex]; 
		kn[29] = binary[1+valindex]; 
		kn[30] = binary[2+valindex]; 
		kn[31] = binary[3+valindex]; 
		/* Permute - P */ 
		worka[ 0] = kn[15]; 
		worka[ 1] = kn[ 6]; 
		worka[ 2] = kn[19]; 
		worka[ 3] = kn[20]; 
		worka[ 4] = kn[28]; 
		worka[ 5] = kn[11]; 
		worka[ 6] = kn[27]; 
		worka[ 7] = kn[16]; 
		worka[ 8] = kn[ 0]; 
		worka[ 9] = kn[14]; 
		worka[10] = kn[22]; 
		worka[11] = kn[25]; 
		worka[12] = kn[ 4]; 
		worka[13] = kn[17]; 
		worka[14] = kn[30]; 
		worka[15] = kn[ 9]; 
		worka[16] = kn[ 1]; 
		worka[17] = kn[ 7]; 
		worka[18] = kn[23]; 
		worka[19] = kn[13]; 
		worka[20] = kn[31]; 
		worka[21] = kn[26]; 
		worka[22] = kn[ 2]; 
		worka[23] = kn[ 8]; 
		worka[24] = kn[18]; 
		worka[25] = kn[12]; 
		worka[26] = kn[29]; 
		worka[27] = kn[ 5]; 
		worka[28] = kn[21]; 
		worka[29] = kn[10]; 
		worka[30] = kn[ 3]; 
		worka[31] = kn[24]; 
		/* bufout XOR worka */ 
		for (i = 0; i < 32; i++) { 
		bufout[i+32] = bufout[i] ^ worka[i]; 
		bufout[i] = buffer[i]; 
		} 
	} /* End of for Iter */ 
	/* Prepare Output */ 
	for (i = 0; i < 32; i++) { 
		j = bufout[i]; 
		bufout[i] = bufout[32+i]; 
		bufout[32+i] = j; 
	} 
	/* Inverse Initial Permutation */ 
	buffer[ 0] = bufout[39]; 
	buffer[ 1] = bufout[ 7]; 
	buffer[ 2] = bufout[47]; 
	buffer[ 3] = bufout[15]; 
	buffer[ 4] = bufout[55]; 
	buffer[ 5] = bufout[23]; 
	buffer[ 6] = bufout[63]; 
	buffer[ 7] = bufout[31]; 
	buffer[ 8] = bufout[38]; 
	buffer[ 9] = bufout[ 6]; 
	buffer[10] = bufout[46]; 
	buffer[11] = bufout[14]; 
	buffer[12] = bufout[54]; 
	buffer[13] = bufout[22]; 
	buffer[14] = bufout[62]; 
	buffer[15] = bufout[30]; 
	buffer[16] = bufout[37]; 
	buffer[17] = bufout[ 5]; 
	buffer[18] = bufout[45]; 
	buffer[19] = bufout[13]; 
	buffer[20] = bufout[53]; 
	buffer[21] = bufout[21]; 
	buffer[22] = bufout[61]; 
	buffer[23] = bufout[29]; 
	buffer[24] = bufout[36]; 
	buffer[25] = bufout[ 4]; 
	buffer[26] = bufout[44]; 
	buffer[27] = bufout[12]; 
	buffer[28] = bufout[52]; 
	buffer[29] = bufout[20]; 
	buffer[30] = bufout[60]; 
	buffer[31] = bufout[28]; 
	buffer[32] = bufout[35]; 
	buffer[33] = bufout[ 3]; 
	buffer[34] = bufout[43]; 
	buffer[35] = bufout[11]; 
	buffer[36] = bufout[51]; 
	buffer[37] = bufout[19]; 
	buffer[38] = bufout[59]; 
	buffer[39] = bufout[27]; 
	buffer[40] = bufout[34]; 
	buffer[41] = bufout[ 2]; 
	buffer[42] = bufout[42]; 
	buffer[43] = bufout[10]; 
	buffer[44] = bufout[50]; 
	buffer[45] = bufout[18]; 
	buffer[46] = bufout[58]; 
	buffer[47] = bufout[26]; 
	buffer[48] = bufout[33]; 
	buffer[49] = bufout[ 1]; 
	buffer[50] = bufout[41]; 
	buffer[51] = bufout[ 9]; 
	buffer[52] = bufout[49]; 
	buffer[53] = bufout[17]; 
	buffer[54] = bufout[57]; 
	buffer[55] = bufout[25]; 
	buffer[56] = bufout[32]; 
	buffer[57] = bufout[ 0]; 
	buffer[58] = bufout[40]; 
	buffer[59] = bufout[ 8]; 
	buffer[60] = bufout[48]; 
	buffer[61] = bufout[16]; 
	buffer[62] = bufout[56]; 
	buffer[63] = bufout[24]; 
	j = 0; 
	for (i = 0; i < 8; i++) { 
		*(dest + i) = 0x00; 
		for (k = 0; k < 7; k++) 
			*(dest + i) = ((*(dest + i)) + buffer[j+k]) * 2; 
		*(dest + i) = *(dest + i) + buffer[j+7]; 
		j += 8; 
	} 
	return 0;
} 

/* ================================================================ 
D3des() 
Description: 3DES algorithm,do 加密 (1) or 解密 (0).
Parameters: source, dest, inkey are 16 bytes.
================================================================ */ 
int D3des(unsigned char *source,unsigned char * dest,unsigned char * inkey, int flg)
{
	unsigned char buffer[64], key1[9], key2[9], tmp1[9], tmp2[9]; 
	int ret; 

	if ( source == NULL || dest == NULL || inkey == NULL )
		return -1;

	memset( key1, 0x0, sizeof( key1 ) );
	memset( key2, 0x0, sizeof( key2 ) );
	memcpy( key1, inkey, 8 );
	memcpy( key2, inkey + 8, 8 );
	if ( flg == 1 )	/*加密*/
	{
		if( getstrlen(source) == 8 )
		{
			memset( tmp1, 0x0, sizeof( tmp1 ) );
			ret = des( source, tmp1, key1, 1 );
			if ( ret  < 0 )
				return -1;
			memset( tmp2, 0x0, sizeof( tmp2 ) );
			ret = des( tmp1, tmp2, key2, 0 );
			if ( ret  < 0 )
				return -1;
			ret = des( tmp2, dest, key1, 1 );
			if ( ret  < 0 )
				return -1;
		}else
		{
			memset( tmp1, 0x0, sizeof( tmp1 ) );
			ret = des( source, tmp1, key1, 1 );
			if ( ret  < 0 )
				return -1;
			memset( tmp2, 0x0, sizeof( tmp2 ) );
			ret = des( tmp1, tmp2, key2, 0 );
			if ( ret  < 0 )
				return -1;
			ret = des( tmp2, dest, key1, 1 );
			if ( ret  < 0 )
				return -1;
			memset( tmp1, 0x0, sizeof( tmp1 ) );
			ret = des( source + 8, tmp1, key1, 1 );
			if ( ret  < 0 )
				return -1;
			memset( tmp2, 0x0, sizeof( tmp2 ) );
			ret = des( tmp1, tmp2, key2, 0 );
			if ( ret  < 0 )
				return -1;
			ret = des( tmp2, dest + 8, key1, 1 );
			if ( ret  < 0 )
				return -1;
		}
	} else 
	{
		if( getstrlen(source) == 8 )
		{
			memset( tmp1, 0x0, sizeof( tmp1 ) );
			ret = des( source, tmp1, key1, 0 );
			if ( ret  < 0 )
				return -1;
			memset( tmp2, 0x0, sizeof( tmp2 ) );
			ret = des( tmp1, tmp2, key2, 1 );
			if ( ret  < 0 )
				return -1;
			ret = des( tmp2, dest, key1, 0 );
			if ( ret  < 0 )
				return -1;
		}else
		{
			memset( tmp1, 0x0, sizeof( tmp1 ) );
			ret = des( source, tmp1, key1, 0 );
			if ( ret  < 0 )
				return -1;
			memset( tmp2, 0x0, sizeof( tmp2 ) );
			ret = des( tmp1, tmp2, key2, 1 );
			if ( ret  < 0 )
				return -1;
			ret = des( tmp2, dest, key1, 0 );
			if ( ret  < 0 )
				return -1;
			memset( tmp1, 0x0, sizeof( tmp1 ) );
			ret = des( source + 8, tmp1, key1, 0 );
			if ( ret  < 0 )
				return -1;
			memset( tmp2, 0x0, sizeof( tmp2 ) );
			ret = des( tmp1, tmp2, key2, 1 );
			if ( ret  < 0 )
				return -1;
			ret = des( tmp2, dest + 8, key1, 0 );
			if ( ret  < 0 )
				return -1;
		}
	}
	return 0;
}

/* ================================================================ 
D3desbylen() 
Description: 3DES algorithm,do 加密 (1) or 解密 (0).
Parameters: source, dest, inkey are 16 bytes.
flg：表示加密或是解密
sourcelen： 表示source数据的长度
inkey：加解密密钥
dest：输出参数
================================================================ */
int D3desbylen(unsigned char *source,int sourcelen,unsigned char *dest,unsigned char *inkey, int flg)
{
	unsigned char key1[9], key2[9], tmp1[9], tmp2[9]; 
	int ret = 0; 

	if ( source == NULL || dest == NULL || inkey == NULL )
		return -1;

	if ( sourcelen != 8 && sourcelen != 16)
		return -1;

	memset( key1, 0x0, sizeof( key1 ) );
	memset( key2, 0x0, sizeof( key2 ) );
	memcpy( key1, inkey, 8 );
	memcpy( key2, inkey + 8, 8 );
	if ( flg == 1 )	/*加密*/
	{
		if( sourcelen == 8 )
		{
			memset( tmp1, 0x0, sizeof( tmp1 ) );
			ret = des( source, tmp1, key1, 1 );
			if ( ret  < 0 )
				return -1;
			memset( tmp2, 0x0, sizeof( tmp2 ) );
			ret = des( tmp1, tmp2, key2, 0 );
			if ( ret  < 0 )
				return -1;
			ret = des( tmp2, dest, key1, 1 );
			if ( ret  < 0 )
				return -1;
		}else
		{
			memset( tmp1, 0x0, sizeof( tmp1 ) );
			ret = des( source, tmp1, key1, 1 );
			if ( ret  < 0 )
				return -1;
			memset( tmp2, 0x0, sizeof( tmp2 ) );
			ret = des( tmp1, tmp2, key2, 0 );
			if ( ret  < 0 )
				return -1;
			ret = des( tmp2, dest, key1, 1 );
			if ( ret  < 0 )
				return -1;
			memset( tmp1, 0x0, sizeof( tmp1 ) );
			ret = des( source + 8, tmp1, key1, 1 );
			if ( ret  < 0 )
				return -1;
			memset( tmp2, 0x0, sizeof( tmp2 ) );
			ret = des( tmp1, tmp2, key2, 0 );
			if ( ret  < 0 )
				return -1;
			ret = des( tmp2, dest + 8, key1, 1 );
			if ( ret  < 0 )
				return -1;
		}
	} 
	else 
	{
		if( sourcelen == 8 )
		{
			memset( tmp1, 0x0, sizeof( tmp1 ) );
			ret = des( source, tmp1, key1, 0 );
			if ( ret  < 0 )
				return -1;
			memset( tmp2, 0x0, sizeof( tmp2 ) );
			ret = des( tmp1, tmp2, key2, 1 );
			if ( ret  < 0 )
				return -1;
			ret = des( tmp2, dest, key1, 0 );
			if ( ret  < 0 )
				return -1;
		}else
		{
			memset( tmp1, 0x0, sizeof( tmp1 ) );
			ret = des( source, tmp1, key1, 0 );
			if ( ret  < 0 )
				return -1;
			memset( tmp2, 0x0, sizeof( tmp2 ) );
			ret = des( tmp1, tmp2, key2, 1 );
			if ( ret  < 0 )
				return -1;
			ret = des( tmp2, dest, key1, 0 );
			if ( ret  < 0 )
				return -1;
			memset( tmp1, 0x0, sizeof( tmp1 ) );
			ret = des( source + 8, tmp1, key1, 0 );
			if ( ret  < 0 )
				return -1;
			memset( tmp2, 0x0, sizeof( tmp2 ) );
			ret = des( tmp1, tmp2, key2, 1 );
			if ( ret  < 0 )
				return -1;
			ret = des( tmp2, dest + 8, key1, 0 );
			if ( ret  < 0 )
				return -1;
		}
	}
	return 0;
}

int sjl05_pos_workeygen(int iSockId, int keyLen, char *keyType, char *kek, char *rtnkey)
{
	int rtn, i,iPid;
	int offset = 0, sndlen = 0, rcvlen, tmplen;
	char retcode[3];
	char sndbuf[1024], rcvbuf[1024],caBuf[2048];
	char tmpbuf[128];
	
	dcs_log(0,0, "sjl05_pos_workeygen,keyLen=%d", keyLen);
	dcs_log(0,0, "sjl05_pos_workeygen keyType=%s", keyType);
	dcs_log(0,0, "sjl05_pos_workeygen kek =%s", kek);

	
	//if (strlen(kek) != 32)
	//{
	//	dcs_log(0,0,"strlen(kek)=%d", strlen(kek));
	//	dcs_log(0,0,"sjl05_pos_workeygen error or data orrer!");
	//	return(-1);
	//}	
	
	memset(sndbuf, 0, sizeof(sndbuf));
	
	/* 指令类型 */
	memcpy(sndbuf, "D107", 4);
	offset += 4;
	
	if(keyLen == 1){
		sprintf(tmpbuf, "%02x", 8);
		memcpy(sndbuf + offset, tmpbuf, 2);
		offset += 2;
	}else{
		sprintf(tmpbuf, "%02x", 16);
		memcpy(sndbuf + offset, tmpbuf, 2);
		offset += 2;
	}
	
	if(memcmp(keyType, "pik", 3) == 0){
		sprintf(tmpbuf, "%02x", 17);
		memcpy(sndbuf + offset, tmpbuf, 2);
		offset += 2;
	}else if(memcmp(keyType, "mak", 3) == 0){
		sprintf(tmpbuf, "%02x", 18);
		memcpy(sndbuf + offset, tmpbuf, 2);
		offset += 2;
	}else{
		sprintf(tmpbuf, "%02x", 19);
		memcpy(sndbuf + offset, tmpbuf, 2);
		offset += 2;
	}

	memcpy(sndbuf + offset, "ffff", 4);
	offset += 4;
	
	sprintf(tmpbuf, "%02x", 16);
	memcpy(sndbuf + offset, tmpbuf, 2);
	offset += 2;
		
	/*kek*/
	memcpy(sndbuf + offset, kek, 32);
	offset += 32;
	
	memset(rcvbuf,0,sizeof(rcvbuf));
	asc_to_bcd(rcvbuf, sndbuf, offset,1);
	offset = offset/2;
	memcpy(sndbuf,rcvbuf,offset);
	
	dcs_debug(sndbuf, offset, "pos workey gen, send to encrypt:");

    memset(caBuf,0,sizeof(caBuf));
  	iPid = getpid();
  	memcpy(caBuf,&iPid,sizeof(long));
  	sprintf(caBuf+sizeof(long),"%04d",iSockId);

  	memcpy(caBuf+sizeof(long)+5,sndbuf,offset);
  	
	if( sg_qid <0)
  		sg_qid = queue_connect("SECURVR");
  	if ( sg_qid <0)
  	{
  		dcs_log(0,0,"connect msg queue fail!");
  		return -1;
  	}
  	int qid1=-1;
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%d",iPid);
		qid1 = queue_connect(tmpbuf);
	if(qid1 <0)
	qid1 = queue_create(tmpbuf);
	if ( qid1 <0)
	{
		dcs_log(0,0,"connect msg queue fail!");
		return -1;
	}
  	memcpy(sndbuf,caBuf,sizeof(long));
  	queue_recv(qid1,sndbuf,sizeof(sndbuf),1);
  	if (0 >queue_send( sg_qid ,caBuf,offset+sizeof(long)+5,0))
  	{
  		sg_qid=-1;
  		dcs_log(0,0,"send msg to queue fail!");
  		return -1;
  	}

  	memset(caBuf,0,sizeof(caBuf));
  	memcpy(caBuf,&iPid,sizeof(long));
  	if ( 0 >(rcvlen=queue_recv(qid1,caBuf,sizeof(caBuf),0)))
  	{
  		qid1=-1;
  		dcs_log(0,0,"recv msg from msg queue fail!");
  		return -1;
  	}

  	if ( memcmp(caBuf+sizeof(long),"FF",2)==0)
  	{
  		dcs_log(0,0," function processing fail!");
  		return -2;
  	}
	
	memcpy(rcvbuf,caBuf+sizeof(long)+2,rcvlen-sizeof(long)+2);
	
	offset = 1;
	
	if (rcvbuf[0] == 'A' )
	{
		offset = rcvbuf[1];
		memcpy(rtnkey, rcvbuf + 2, offset*2+8);
		return(0);
	}
	else if (rcvbuf[0] == 'E')
	{
		memcpy(retcode, rcvbuf + 1, 1);
		dcs_debug(retcode, 1, "pos workey gen error!");
		return(-1);
	}
	else
	{
		return(-1);
	}
}

int sjl05_pos_pinconvert(int iSockId, char *pik1, int pinFormatter1, char *pik2, int pinFormatter2, char *card, char *pinBlock, char *outPin)
{
	int rtn, i,iPid;
	int offset = 0, sndlen = 0, rcvlen, tmplen;
	char retcode[3];
	char sndbuf[1024], rcvbuf[1024],caBuf[2048];
	char tmpbuf[128];
	
	/*
	dcs_log(0,0, "sjl05_pos_pinconvert,pik1=%s", pik1);
	dcs_log(0,0, "sjl05_pos_pinconvert pinFormatter1=%d", pinFormatter1);
	dcs_log(0,0, "sjl05_pos_pinconvert pik2 =%s", pik2);
	dcs_log(0,0, "sjl05_pos_pinconvert pinFormatter2=%d", pinFormatter2);
	dcs_log(0,0, "sjl05_pos_pinconvert card=%s", card);
	dcs_debug(pinBlock, 8, "sjl05_pos_pinconvert pinBlock");
	*/
	
	//if (strlen(pik1) != 32 || strlen(pinBlock) != 8)
	//{
	//	dcs_log(0,0,"strlen(pik1) =%d, strlen(pinBlock)=%d", strlen(pik1), strlen(pinBlock));
	//	dcs_log(0,0,"sjl05_pos_pinconvert error or data orrer!");
	//	return(-1);
	//}
	
	memset(sndbuf, 0, sizeof(sndbuf));
	
	/* 指令类型 */
	memcpy(sndbuf, "D124", 4);
	offset += 4;
	
	sprintf(tmpbuf, "%02x", 16);
	memcpy(sndbuf + offset, tmpbuf, 2);
	offset += 2;
	
	memcpy(sndbuf + offset, pik1, 32);
	offset += 32;

	sprintf(tmpbuf, "%02x", 8);
	memcpy(sndbuf + offset, tmpbuf, 2);
	offset += 2;
	
	memcpy(sndbuf + offset, pik2, 16);
	offset += 16;
		
	sprintf(tmpbuf, "%02x", 1);
	memcpy(sndbuf + offset, tmpbuf, 2);
	offset += 2;
	
	sprintf(tmpbuf, "%02x", 6);
	memcpy(sndbuf + offset, tmpbuf, 2);
	offset += 2;
	
	memset(rcvbuf,0,sizeof(rcvbuf));
	asc_to_bcd(rcvbuf, sndbuf, offset,1);
	offset = offset/2;
	memcpy(sndbuf, rcvbuf, offset);
	
	memcpy(sndbuf + offset, pinBlock, 8);
	offset += 8;
	
	memcpy(sndbuf + offset, card, strlen(card));
	offset += strlen(card);
	
	memcpy(sndbuf + offset, ";", 1);
	offset += 1;
	
	memcpy(sndbuf + offset, card, strlen(card));
	offset += strlen(card);
	
	memcpy(sndbuf + offset, ";", 1);
	offset += 1;
	
//	dcs_debug(sndbuf, offset, "sjl05_pos_pinconvert , send to encrypt:");

    memset(caBuf,0,sizeof(caBuf));
  	iPid = getpid();
  	memcpy(caBuf,&iPid,sizeof(long));
  	sprintf(caBuf+sizeof(long),"%04d",iSockId);

  	memcpy(caBuf+sizeof(long)+5,sndbuf,offset);
  	
	if( sg_qid <0)
  		sg_qid = queue_connect("SECURVR");
  	if ( sg_qid <0)
  	{
  		dcs_log(0,0,"connect msg queue fail!");
  		return -1;
  	}
  	int qid1=-1;
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%d",iPid);
	qid1 = queue_connect(tmpbuf);
	if(qid1 <0)
		qid1 = queue_create(tmpbuf);
	if ( qid1 <0)
	{
		dcs_log(0,0,"connect msg queue fail!");
		return -1;
	}
	  memcpy(sndbuf,caBuf,sizeof(long));
  	queue_recv(qid1,sndbuf,sizeof(sndbuf),1);
  	if (0 >queue_send( sg_qid ,caBuf,offset+sizeof(long)+5,0))
  	{
  		sg_qid=-1;
  		dcs_log(0,0,"send msg to queue fail!");
  		return -1;
  	}

  	memset(caBuf,0,sizeof(caBuf));
  	memcpy(caBuf,&iPid,sizeof(long));
  	if ( 0 >(rcvlen=queue_recv(qid1,caBuf,sizeof(caBuf),0)))
  	{
  		qid1=-1;
  		dcs_log(0,0,"recv msg from msg queue fail!");
  		return -1;
  	}

  	if ( memcmp(caBuf+sizeof(long),"FF",2)==0)
  	{
  		dcs_log(0,0," function processing fail!");
  		return -2;
  	}
	
	memcpy(rcvbuf,caBuf+sizeof(long)+2,rcvlen-sizeof(long)+2);
	
	offset = 1;
	
	if (rcvbuf[0] == 'A' )
	{
		memcpy(outPin, rcvbuf + 1, 8);
		return(0);
	}
	else if (rcvbuf[0] == 'E')
	{
		memcpy(retcode, rcvbuf + 1, 1);
		dcs_debug(retcode, 1, "pos pin convert error!");
		return(-1);
	}
	else
	{
		return(-1);
	}
}

int sjl05_pos_maccal(int iSockId, char *mak, char *data, char *rtnData)
{
	int rtn, i,iPid;
	int offset = 0, sndlen = 0, rcvlen, tmplen;
	char retcode[3];
	char sndbuf[1024], rcvbuf[1024],caBuf[2048];
	char tmpbuf[128];
	
	//dcs_log(0,0, "sjl05_pos_maccal,mak=%s", mak);
	//dcs_log(0,0, "sjl05_pos_maccal data=%s", data);
	
	//if (strlen(mak) != 16)
	//{
	//	dcs_log(0,0,"sjl05_pos_maccal error or data orrer!");
	//	return(-1);
	//}	
	
	memset(sndbuf, 0, sizeof(sndbuf));
	
	/* 指令类型 */
	memcpy(sndbuf, "D132", 4);
	offset += 4;
	
	sprintf(tmpbuf, "%02x", 2);
	memcpy(sndbuf + offset, tmpbuf, 2);
	offset += 2;

	sprintf(tmpbuf, "%02x", 8);
	memcpy(sndbuf + offset, tmpbuf, 2);
	offset += 2;
	
	memcpy(sndbuf + offset, mak, 16);
	offset += 16;
		
	memcpy(sndbuf + offset, "0000000000000000", 16);
	offset += 16;
	
	sprintf(tmpbuf, "%04x", 8);
	memcpy(sndbuf + offset, tmpbuf, 4);
	offset += 4;
	
	memset(rcvbuf,0,sizeof(rcvbuf));
	asc_to_bcd(rcvbuf, sndbuf, offset,1);
	offset = offset/2;
	memcpy(sndbuf, rcvbuf, offset);
	
	memcpy(sndbuf + offset, data, 8);
	offset += 8;
	
	//dcs_debug(sndbuf, offset, "sjl05_pos_maccal , send to encrypt:");

    memset(caBuf,0,sizeof(caBuf));
  	iPid = getpid();
  	memcpy(caBuf,&iPid,sizeof(long));
  	sprintf(caBuf+sizeof(long),"%04d",iSockId);

  	memcpy(caBuf+sizeof(long)+5,sndbuf,offset);
  	
	if( sg_qid <0)
  		sg_qid = queue_connect("SECURVR");
  	if ( sg_qid <0)
  	{
  		dcs_log(0,0,"connect msg queue fail!");
  		return -1;
  	}
  	int qid1=-1;
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%d",iPid);
	qid1 = queue_connect(tmpbuf);
	if(qid1 <0)
		qid1 = queue_create(tmpbuf);
	if ( qid1 <0)
	{
		dcs_log(0,0,"connect msg queue fail!");
		return -1;
	}
  	memcpy(sndbuf,caBuf,sizeof(long));
  	long tid= -1;//存放当前线程的tid,如果是进程，tid等于pid
  	tid = syscall(SYS_gettid);
  	if (0 >queue_send_by_msgtype( sg_qid ,caBuf,offset+sizeof(long)+5,tid,0))
  	{
  		sg_qid=-1;
  		dcs_log(0,0,"send msg to queue fail!");
  		return -1;
  	}

  	memset(caBuf,0,sizeof(caBuf));
  	if ( 0 >(rcvlen=queue_recv_by_msgtype(qid1,caBuf,sizeof(caBuf), &tid, 0)))
  	{
  		qid1=-1;
  		dcs_log(0,0,"recv msg from msg queue fail!");
  		return -1;
  	}
  	if ( memcmp(caBuf+sizeof(long),"FF",2)==0)
  	{
  		dcs_log(0,0," function processing fail!");
  		return -2;
  	}
	
	memcpy(rcvbuf,caBuf+sizeof(long)+2,rcvlen-sizeof(long)+2);
	
	offset = 1;
	
	if (rcvbuf[0] == 'A' )
	{
		memcpy(rtnData, rcvbuf + 1, 8);
		return(0);
	}
	else if (rcvbuf[0] == 'E')
	{
		memcpy(retcode, rcvbuf + 1, 1);
		dcs_debug(retcode, 1, "pos pin convert error!");
		return(-1);
	}
	else
	{
		return(-1);
	}
}

/*
	@param tdk(ascii, 32字节)
	@param data(ascii, 16字节)
	@param rtnData(bcd,8字节)
*/
int sjl05_pos_trackdecrypt(int iSockId, char *tdk, char *data, char *rtnData)
{
	int rtn, i,iPid;
	int offset = 0, sndlen = 0, rcvlen, tmplen;
	char retcode[3];
	char sndbuf[2048], rcvbuf[2048],caBuf[2048];
	char tmpbuf[128];
	
	//dcs_log(0,0, "sjl05_pos_trackdecrypt, tdk=%s", tdk);
	//dcs_debug(data, 16, "sjl05_pos_trackdecrypt");
	
	//if (strlen(tdk) != 32)
	//{
	//	dcs_log(0,0,"sjl05_pos_trackdecrypt tdk length error!");
	//	return(-1);
	//}	
	
	memset(sndbuf, 0, sizeof(sndbuf));
	
	/* 指令类型 */
	memcpy(sndbuf, "D114", 4);
	offset += 4;
	
	/* 算法标志 */
	memcpy(sndbuf+offset, "12", 2);
	offset += 2;
	
	/* 密钥 */
	memcpy(sndbuf + offset, tdk, 32);
	offset += 32;
	
	/* 数据长度 */
	sprintf(tmpbuf, "%04x", 8);
	memcpy(sndbuf + offset, tmpbuf, 4);
	offset += 4;
	
	/* 数据 */
	memcpy(sndbuf + offset, data, 16);
	offset += 16;
	
	memset(rcvbuf, 0, sizeof(rcvbuf));
	asc_to_bcd(rcvbuf, sndbuf, offset, 1);
	offset = offset/2;
	memcpy(sndbuf, rcvbuf, offset);
	
	//dcs_debug(sndbuf, offset, "sjl05_pos_trackdecrypt , send to encrypt:");

    memset(caBuf,0,sizeof(caBuf));
  	iPid = getpid();
  	memcpy(caBuf,&iPid,sizeof(long));
  	sprintf(caBuf+sizeof(long),"%04d",iSockId);

  	memcpy(caBuf+sizeof(long)+5,sndbuf,offset);
  	
	if( sg_qid <0)
  		sg_qid = queue_connect("SECURVR");
  	if ( sg_qid <0)
  	{
  		dcs_log(0,0,"connect msg queue fail!");
  		return -1;
  	}
  	int qid1=-1;
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%d",iPid);
	qid1 = queue_connect(tmpbuf);
	if(qid1 <0)
		qid1 = queue_create(tmpbuf);
	if ( qid1 <0)
	{
		dcs_log(0,0,"connect msg queue fail!");
		return -1;
	}
  	memcpy(sndbuf,caBuf,sizeof(long));
  	queue_recv(qid1,sndbuf,sizeof(sndbuf),1);
  	if (0 >queue_send( sg_qid ,caBuf,offset+sizeof(long)+5,0))
  	{
  		sg_qid=-1;
  		dcs_log(0,0,"send msg to queue fail!");
  		return -1;
  	}

  	memset(caBuf,0,sizeof(caBuf));
  	memcpy(caBuf,&iPid,sizeof(long));
  	if ( 0 >(rcvlen=queue_recv(qid1,caBuf,sizeof(caBuf),0)))
  	{
  		qid1=-1;
  		dcs_log(0,0,"recv msg from msg queue fail!");
  		return -1;
  	}

  	if ( memcmp(caBuf+sizeof(long),"FF",2)==0)
  	{
  		dcs_log(0,0," function processing fail!");
  		return -2;
  	}
	
	memcpy(rcvbuf,caBuf+sizeof(long)+2,rcvlen-sizeof(long)+2);
	
	offset = 1;
	
	if (rcvbuf[0] == 'A' )
	{
		memcpy(rtnData, rcvbuf + 3, 8);
		return(0);
	}
	else if (rcvbuf[0] == 'E')
	{
		memcpy(retcode, rcvbuf + 1, 1);
		dcs_debug(retcode, 1, "sjl05_pos_trackdecrypt decrypt error!");
		return(-1);
	}
	else
	{
		return(-1);
	}
}

/*
	pos交易报文整包解密
	@param tdk(ascii, 32字节)
	@param data(ascii, 16字节)
	@param rtnData(bcd,8字节)
*/
int sjl05_pos_packageDecrypt(int iSockId, char *tdk, char *data, int dataLen, char *rtnData)
{
	int rtn, i,iPid;
	int offset = 0, sndlen = 0, rcvlen, tmplen;
	char retcode[3];
	char sndbuf[2048], rcvbuf[2048],caBuf[2048];
	char tmpbuf[10];
	
	memset(sndbuf, 0, sizeof(sndbuf));
	
	/* 指令类型 */
	memcpy(sndbuf, "D114", 4);
	offset += 4;
	
	/* 算法标志 */
	memcpy(sndbuf+offset, "12", 2);
	offset += 2;
	
	/* 密钥 */
	memcpy(sndbuf + offset, tdk, 32);
	offset += 32;
	
	/* 数据长度 */
	sprintf(tmpbuf, "%04x", dataLen);
	memcpy(sndbuf + offset, tmpbuf, 4);
	offset += 4;
	
	memset(rcvbuf, 0, sizeof(rcvbuf));
	asc_to_bcd(rcvbuf, sndbuf, offset, 1);
	offset = offset/2;
	
	memcpy(sndbuf, rcvbuf, offset);
	
	/* 数据 */
	memcpy(sndbuf + offset, data, dataLen);
	offset += dataLen;
	
	dcs_debug(sndbuf, offset, "sjl05_pos_packagedecrypt , send to encrypt:");

    memset(caBuf,0,sizeof(caBuf));
  	iPid = getpid();
  	memcpy(caBuf,&iPid,sizeof(long));
  	sprintf(caBuf+sizeof(long),"%04d",iSockId);

  	memcpy(caBuf+sizeof(long)+5,sndbuf,offset);
  	
	if( sg_qid <0)
  		sg_qid = queue_connect("SECURVR");
  	if ( sg_qid <0)
  	{
  		dcs_log(0,0,"connect msg queue fail!");
  		return -1;
  	}
  	int qid1=-1;
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%d",iPid);
	qid1 = queue_connect(tmpbuf);
	if(qid1 <0)
		qid1 = queue_create(tmpbuf);
	if ( qid1 <0)
	{
		dcs_log(0,0,"connect msg queue fail!");
		return -1;
	}
  	memcpy(sndbuf,caBuf,sizeof(long));
	long tid= -1;//存放当前线程的tid,如果是进程，tid等于pid
  	tid = syscall(SYS_gettid);
  	if (0 >queue_send_by_msgtype( sg_qid ,caBuf,offset+sizeof(long)+5,tid,0))
  	{
  		sg_qid=-1;
  		dcs_log(0,0,"send msg to queue fail!");
  		return -1;
  	}
  	memset(caBuf,0,sizeof(caBuf));
  	if ( 0 >(rcvlen=queue_recv_by_msgtype(qid1,caBuf,sizeof(caBuf), &tid, 0)))
  	{
  		qid1=-1;
  		dcs_log(0,0,"recv msg from msg queue fail!");
  		return -1;
  	}

  	if ( memcmp(caBuf+sizeof(long),"FF",2)==0)
  	{
  		dcs_log(0,0," function processing fail!");
  		return -2;
  	}
	
	memcpy(rcvbuf,caBuf+sizeof(long)+2,rcvlen-sizeof(long)+2);
	
	offset = 1;
	
	if (rcvbuf[0] == 'A' )
	{
		memcpy(rtnData, rcvbuf + 3, dataLen);
		return(0);
	}
	else if (rcvbuf[0] == 'E')
	{
		memcpy(retcode, rcvbuf + 1, 1);
		dcs_debug(retcode, 1, "sjl05_pos_trackdecrypt decrypt error!");
		return(-1);
	}
	else
	{
		return(-1);
	}
}

/*
	pos交易报文整包加密
	@param tdk(ascii, 32字节)
	@param data(ascii, 16字节)
	@param rtnData(bcd,8字节)
*/
int sjl05_pos_packageEncrypt(int iSockId, char *tdk, char *data, int dataLen, char *rtnData)
{
	int rtn, i,iPid;
	int offset = 0, sndlen = 0, rcvlen, tmplen;
	char retcode[3];
	char sndbuf[1024], rcvbuf[1024],caBuf[1024];
	char tmpbuf[10];
	
	memset(sndbuf, 0, sizeof(sndbuf));
	
	/* 指令类型 */
	memcpy(sndbuf, "D112", 4);
	offset += 4;
	
	/* 算法标志 */
	memcpy(sndbuf+offset, "12", 2);
	offset += 2;
	
	/* 密钥 */
	memcpy(sndbuf + offset, tdk, 32);
	offset += 32;
	
	/* 数据长度 */
	sprintf(tmpbuf, "%04x", dataLen);
	memcpy(sndbuf + offset, tmpbuf, 4);
	offset += 4;
	
	memset(rcvbuf, 0, sizeof(rcvbuf));
	asc_to_bcd(rcvbuf, sndbuf, offset, 1);
	offset = offset/2;
	
	memcpy(sndbuf, rcvbuf, offset);
	
	/* 数据 */
	memcpy(sndbuf + offset, data, dataLen);
	offset += dataLen;
	
	dcs_debug(sndbuf, offset, "sjl05_pos_packageEncrypt , send to encrypt:");

    memset(caBuf,0,sizeof(caBuf));
  	iPid = getpid();
  	memcpy(caBuf,&iPid,sizeof(long));
  	sprintf(caBuf+sizeof(long),"%04d",iSockId);

  	memcpy(caBuf+sizeof(long)+5,sndbuf,offset);
  	
	if( sg_qid <0)
  		sg_qid = queue_connect("SECURVR");
  	if ( sg_qid <0)
  	{
  		dcs_log(0,0,"connect msg queue fail!");
  		return -1;
  	}
	int qid1=-1;
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%d",iPid);
	qid1 = queue_connect(tmpbuf);
	if(qid1 <0)
		qid1 = queue_create(tmpbuf);
	if ( qid1 <0)
	{
		dcs_log(0,0,"connect msg queue fail!");
		return -1;
	}  	memcpy(sndbuf,caBuf,sizeof(long));
	long tid= -1;//存放当前线程的tid,如果是进程，tid等于pid
	tid = syscall(SYS_gettid);
	if (0 >queue_send_by_msgtype( sg_qid ,caBuf,offset+sizeof(long)+5,tid,0))
  	{
  		sg_qid=-1;
  		dcs_log(0,0,"send msg to queue fail!");
  		return -1;
  	}

  	memset(caBuf,0,sizeof(caBuf));
  	if ( 0 >(rcvlen=queue_recv_by_msgtype(qid1,caBuf,sizeof(caBuf), &tid, 0)))
  	{
  		qid1=-1;
  		dcs_log(0,0,"recv msg from msg queue fail!");
  		return -1;
  	}

  	if ( memcmp(caBuf+sizeof(long),"FF",2)==0)
  	{
  		dcs_log(0,0," function processing fail!");
  		return -2;
  	}
	
	memcpy(rcvbuf,caBuf+sizeof(long)+2,rcvlen-sizeof(long)+2);
	
	offset = 1;
	
	if (rcvbuf[0] == 'A' )
	{
		memcpy(rtnData, rcvbuf + 3, dataLen);
		return(0);
	}
	else if (rcvbuf[0] == 'E')
	{
		memcpy(retcode, rcvbuf + 1, 1);
		dcs_debug(retcode, 1, "sjl05_pos_trackdecrypt decrypt error!");
		return(-1);
	}
	else
	{
		return(-1);
	}
}


int sjl05_ewphb_mac(int iSockId, int makIndex, char *psam, char *data, int dataLen, char *rtnData)
{
	int rtn, i,iPid;
	int offset = 0, sndlen = 0, rcvlen, tmplen;
	char retcode[3];
	char sndbuf[1024], rcvbuf[1024],caBuf[1024];
	char tmpbuf[10];
	
	memset(sndbuf, 0, sizeof(sndbuf));
	
	/* 指令类型 */
	memcpy(sndbuf, "B251", 4);
	offset += 4;
	
	/* 用户保留字 */
	memcpy(sndbuf+offset, "1234567812345678", 16);
	offset += 16;
	
	/* MAC类型 */
	memcpy(sndbuf + offset, "00", 2);
	offset += 2;
	
	/* 次主密钥索引 */
	sprintf(tmpbuf, "%04x", makIndex);
	memcpy(sndbuf + offset, tmpbuf, 4);
	offset += 4;
	
	/* MAC算法类型  */
	memcpy(sndbuf + offset, "01", 2);
	offset += 2;
	
	/* 分散次数  */
	memcpy(sndbuf + offset, "01", 2);
	offset += 2;
	
	/* 分散数据  */
	memcpy(sndbuf + offset, psam, 16);
	offset += 16;
	
	/* MAC初始数据  */
	memcpy(sndbuf + offset, "0000000000000000", 16);
	offset += 16;
	
	/* MAC数据长度 */
	sprintf(tmpbuf, "%04x", dataLen);
	memcpy(sndbuf + offset, tmpbuf, 4);
	offset += 4;

	memset(rcvbuf, 0, sizeof(rcvbuf));
	asc_to_bcd(rcvbuf, sndbuf, offset, 1);
	offset = offset/2;
	memcpy(sndbuf, rcvbuf, offset);
	
	/* MAC数据  */
	memcpy(sndbuf + offset, data, dataLen);
	offset += dataLen;
	
	//dcs_debug(sndbuf, offset, "sjl05_ewphb_mac , send to encrypt:");

    	memset(caBuf,0,sizeof(caBuf));
  	iPid = getpid();
  	memcpy(caBuf,&iPid,sizeof(long));
  	sprintf(caBuf+sizeof(long),"%04d",iSockId);

  	memcpy(caBuf+sizeof(long)+5,sndbuf,offset);
  	
	if( sg_qid <0)
  		sg_qid = queue_connect("SECURVR");
  	if ( sg_qid <0)
  	{
  		dcs_log(0,0,"connect msg queue fail!");
  		return -1;
  	}
  	int qid1=-1;
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%d",iPid);
	qid1 = queue_connect(tmpbuf);
	if(qid1 <0)
		qid1 = queue_create(tmpbuf);
	if ( qid1 <0)
	{
		dcs_log(0,0,"connect msg queue fail!");
		return -1;
	}
  	memcpy(sndbuf,caBuf,sizeof(long));
  	queue_recv(qid1,sndbuf,sizeof(sndbuf),1);
  	if (0 >queue_send( sg_qid ,caBuf,offset+sizeof(long)+5,0))
  	{
  		sg_qid=-1;
  		dcs_log(0,0,"send msg to queue fail!");
  		return -1;
  	}

  	memset(caBuf,0,sizeof(caBuf));
  	memcpy(caBuf,&iPid,sizeof(long));
  	if ( 0 >(rcvlen=queue_recv(qid1,caBuf,sizeof(caBuf),0)))
  	{
  		qid1=-1;
  		dcs_log(0,0,"recv msg from msg queue fail!");
  		return -1;
  	}

  	if ( memcmp(caBuf+sizeof(long),"FF",2)==0)
  	{
  		dcs_log(0,0," function processing fail!");
  		return -2;
  	}
	
	memcpy(rcvbuf,caBuf+sizeof(long)+2,rcvlen-sizeof(long)+2);
	
	offset = 1;
	
	if (rcvbuf[0] == 'A' )
	{
		memcpy(rtnData, rcvbuf + 9, 8);
		return(0);
	}
	else if (rcvbuf[0] == 'E')
	{
		memcpy(retcode, rcvbuf + 9, 1);
		dcs_debug(retcode, 1, "sjl05_ewphb_mac decrypt error!");
		return(-1);
	}
	else
	{
		return(-1);
	}
}

int cust_mac(int makIndex, char *psam, char *xpeprandom, char *data, int dataLen, char *rtnData)
{
	 int i;
	 struct secLinkInfo * pLink = NULL;
	
	 if (!PEP_DESInit())
	 {
	 	 dcs_log(0,0," init des system fail");
	 	 return -1;
	 }
	 	
	 pLink = PEP_GetFreeLink();
	 if ( pLink == NULL ) 
	 {
	 	  dcs_log(0,0,"get free link fail");
	 	  return -2;
	 }
	 
	 if ( memcmp(pLink->cEncType,"01",2)==0)
	 {
	 	  i = sjl05_cust_mac(pLink->iSocketId, makIndex, psam, xpeprandom, data, dataLen, rtnData);
	 	  if ( i == -1 )
	 	  {		
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"cust_mac fail!");
	 			return -3;
	 	  }
	 	  else if ( i == -2)
	 	  {
	 			pLink->iStatus= DCS_STAT_DISCONNECTED;
	 			XPE_SendFailMessage( pLink->iNum);
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"Comm fail!");
	 			return -4;
	 	  }
	 }
	 else
	 {
	 	 PEP_FreeLink( pLink);	
	 	 dcs_log(0,0,"不支持的加密机类型");
	 	 return -5;
	 }
	 PEP_FreeLink( pLink);
	 return 0;
}

/*
	加密机实现
*/
int sjl05_cust_mac(int iSockId, int makIndex, char *psam, char *xpeprandom, char *data, int dataLen, char *rtnData)
{
	int rtn, i,iPid;
	int offset = 0, sndlen = 0, rcvlen, tmplen;
	char retcode[3];
	char sndbuf[1024], rcvbuf[1024],caBuf[1024];
	char tmpbuf[10];
	
	memset(sndbuf, 0, sizeof(sndbuf));
	
	/* 指令类型 */
	memcpy(sndbuf, "B251", 4);
	offset += 4;
	
	/* 用户保留字 */
	memcpy(sndbuf+offset, "1234567812345678", 16);
	offset += 16;
	
	/* MAC类型 */
	memcpy(sndbuf + offset, "00", 2);
	offset += 2;
	
	/* 次主密钥索引 */
	sprintf(tmpbuf, "%04x", makIndex);
	memcpy(sndbuf + offset, tmpbuf, 4);
	offset += 4;
	
	/* MAC算法类型  */
	memcpy(sndbuf + offset, "01", 2);
	offset += 2;
	
	/* 分散次数  */
	memcpy(sndbuf + offset, "02", 2);
	offset += 2;
	
	/* 分散数据1  */
	memcpy(sndbuf + offset, psam, 16);
	offset += 16;
	
	/* 分散数据2  */
	memcpy(sndbuf + offset, xpeprandom, 16);
	offset += 16;
	
	/* MAC初始数据  */
	memcpy(sndbuf + offset, "0000000000000000", 16);
	offset += 16;
	
	/* MAC数据长度 */
	sprintf(tmpbuf, "%04x", dataLen);
	memcpy(sndbuf + offset, tmpbuf, 4);
	offset += 4;

	memset(rcvbuf, 0, sizeof(rcvbuf));
	asc_to_bcd(rcvbuf, sndbuf, offset, 1);
	offset = offset/2;
	memcpy(sndbuf, rcvbuf, offset);
	
	/* MAC数据  */
	memcpy(sndbuf + offset, data, dataLen);
	offset += dataLen;
	
	dcs_debug(sndbuf, offset, "sjl05_cust_mac , send to encrypt:");

    memset(caBuf,0,sizeof(caBuf));
  	iPid = getpid();
  	memcpy(caBuf,&iPid,sizeof(long));
  	sprintf(caBuf+sizeof(long),"%04d",iSockId);

  	memcpy(caBuf+sizeof(long)+5,sndbuf,offset);
  	
	if( sg_qid <0)
  		sg_qid = queue_connect("SECURVR");
  	if ( sg_qid <0)
  	{
  		dcs_log(0,0,"connect msg queue fail!");
  		return -1;
  	}
	int qid1=-1;
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%d",iPid);
	qid1 = queue_connect(tmpbuf);
	if(qid1 <0)
		qid1 = queue_create(tmpbuf);
	if ( qid1 <0)
	{
		dcs_log(0,0,"connect msg queue fail!");
		return -1;
	}  	memcpy(sndbuf,caBuf,sizeof(long));
  	queue_recv(qid1,sndbuf,sizeof(sndbuf),1);
  	if (0 >queue_send( sg_qid ,caBuf,offset+sizeof(long)+5,0))
  	{
  		sg_qid=-1;
  		dcs_log(0,0,"send msg to queue fail!");
  		return -1;
  	}

  	memset(caBuf,0,sizeof(caBuf));
  	memcpy(caBuf,&iPid,sizeof(long));
  	if ( 0 >(rcvlen=queue_recv(qid1,caBuf,sizeof(caBuf),0)))
  	{
  		qid1=-1;
  		dcs_log(0,0,"recv msg from msg queue fail!");
  		return -1;
  	}

  	if ( memcmp(caBuf+sizeof(long),"FF",2)==0)
  	{
  		dcs_log(0,0," function processing fail!");
  		return -2;
  	}
	
	memcpy(rcvbuf,caBuf+sizeof(long)+2,rcvlen-sizeof(long)+2);
	
	offset = 1;
	
	if (rcvbuf[0] == 'A' )
	{
		memcpy(rtnData, rcvbuf + 9, 8);
		return(0);
	}
	else if (rcvbuf[0] == 'E')
	{
		memcpy(retcode, rcvbuf + 1, 1);
		dcs_debug(retcode, 1, "sjl05_cust_mac decrypt error!");
		return(-1);
	}
	else
	{
		return(-1);
	}

}

/*
 *
 *mpos 远程主密钥下载功能
 * */
int sjl05_mpos_keygen(int iSockId, int keyIndex, char *psam, char *data_buf)
{
		int rtn, i,iPid;
		int offset = 0, sndlen = 0, rcvlen, tmplen;
		char retcode[3];
		char sndbuf[1024], rcvbuf[1024],caBuf[2048];
		char tmpbuf[128];

		//dcs_log(0,0, "sjl05_mpos_keygen,keyIndex=%d", keyIndex);
		//dcs_log(0,0, "sjl05_mpos_keygen psam=%s", psam);

		if (strlen(psam) != 16 || keyIndex < 0)
		{
			dcs_log(0,0,"sjl05_mpos_keygen error or data orrer!!!");
			return(-1);
		}

		memset(sndbuf,0,sizeof(sndbuf));

		/* 指令类型 */
		memcpy(sndbuf, "B061", 4);
		offset += 4;

		/*用户保留字 */
		memcpy(sndbuf+offset,"0000000000000000",16);
		offset += 16;

		/*次主密钥索引*/
		sprintf(tmpbuf, "%04x", keyIndex);
		memcpy(sndbuf + offset, tmpbuf, 4);
		offset += 4;

		/*分散次数*/
		memcpy(sndbuf + offset, "01", 2);
		offset += 2;

		/* SAM卡序列号 */
		memcpy(sndbuf + offset, psam, 16);
		offset += 16;

		/*压缩数据*/
		memset(rcvbuf,0,sizeof(rcvbuf));
		asc_to_bcd(rcvbuf, sndbuf, offset, 0);
		offset = offset/2;
		memcpy(sndbuf, rcvbuf, offset);

		dcs_debug(sndbuf, offset, "sjl05_mpos_keygen:");

	    memset(caBuf,0,sizeof(caBuf));
	  	iPid = getpid();
	  	memcpy(caBuf,&iPid,sizeof(long));
	  	sprintf(caBuf+sizeof(long),"%04d",iSockId);

	  	memcpy(caBuf+sizeof(long)+5,sndbuf,offset);

		if( sg_qid <0)
	  		sg_qid = queue_connect("SECURVR");
	  	if ( sg_qid <0)
	  	{
	  		dcs_log(0,0,"connect msg queue fail!");
	  		return -1;
	  	}
	  	int qid1=-1;
		memset(tmpbuf,0,sizeof(tmpbuf));
		sprintf(tmpbuf,"%d",iPid);
		qid1 = queue_connect(tmpbuf);
		if(qid1 <0)
			qid1 = queue_create(tmpbuf);
	  	memcpy(sndbuf,caBuf,sizeof(long));
	  	queue_recv(qid1,sndbuf,sizeof(sndbuf),1);
	  	if (0 >queue_send( sg_qid ,caBuf,offset+sizeof(long)+5,0))
	  	{
	  		sg_qid=-1;
	  		dcs_log(0,0,"send msg to queue fail!");
	  		return -1;
	  	}

	  	memset(caBuf, 0, sizeof(caBuf));
	  	memcpy(caBuf, &iPid, sizeof(long));
	  	if ( 0 >(rcvlen=queue_recv(qid1, caBuf, sizeof(caBuf),0)))
	  	{
	  		qid1=-1;
	  		dcs_log(0,0,"recv msg from msg queue fail!");
	  		return -1;
	  	}

	  	if ( memcmp(caBuf+sizeof(long),"FF",2)==0)
	  	{
	  		dcs_log(0,0," function processing fail!");
	  		return -2;
	  	}

		memcpy(rcvbuf,caBuf+sizeof(long)+2,rcvlen-sizeof(long)-2);

		offset = 1;

		if (rcvbuf[0] == 'A' )
		{
			memcpy(data_buf, rcvbuf + 9, 20);
			return(0);
		}
		else if (rcvbuf[0] == 'E')
		{
			memcpy(retcode, rcvbuf + offset + 8, 1);
			dcs_debug(rcvbuf,rcvlen,"sjl05_mpos_keygen error!!");
			return(-1);
		}
		else
		{
			return(-1);
		}

}

/*
 *
 *mpos 远程主密钥下载功能
 * */
int sjl05_mpos_keygen_encrypt(int iSockId, int zmkIndex, char *toEncryptKey, char *data_buf)
{
		int rtn, i,iPid;
		int offset = 0, sndlen = 0, rcvlen, tmplen;
		char retcode[3];
		char sndbuf[1024], rcvbuf[1024],caBuf[2048];
		char tmpbuf[128];

		//dcs_log(0,0, "sjl05_mpos_keygen_encrypt,zmkIndex=%d", zmkIndex);
		//dcs_log(0,0, "sjl05_mpos_keygen_encrypt toEncryptKey=%s", toEncryptKey);

		if (toEncryptKey == NULL || zmkIndex < 0 )
		{
			dcs_log(0,0,"sjl05_mpos_keygen error or data orrer!!!");
			return(-1);
		}

		memset(sndbuf,0,sizeof(sndbuf));

		/* 指令类型 */
		memcpy(sndbuf, "72", 2);
		offset += 2;

		/*区域主密钥索引*/
		sprintf(tmpbuf, "%04x", zmkIndex);
		memcpy(sndbuf + offset, tmpbuf, 4);
		offset += 4;

		/*初始向量 */
		memcpy(sndbuf+offset,"0000000000000000",16);
		offset += 16;

		/*加密标识*/
		memcpy(sndbuf + offset, "01", 2);
		offset += 2;

		/*算法标识 */
		memcpy(sndbuf + offset, "00", 2);
		offset += 2;

		/*处理数据的长度*/
		sprintf(tmpbuf, "%04x", 16);
		memcpy(sndbuf + offset, tmpbuf, 4);
		offset += 4;

		/*压缩数据*/
		memset(rcvbuf,0,sizeof(rcvbuf));
		asc_to_bcd(rcvbuf, sndbuf, offset, 0);
		offset = offset/2;
		memcpy(sndbuf, rcvbuf, offset);

		/*数据*/
		memcpy(sndbuf + offset, toEncryptKey, 16);
		offset += 16;

		dcs_debug(sndbuf, offset, "sjl05_mpos_keygen_encrypt:");

	    memset(caBuf,0,sizeof(caBuf));
	  	iPid = getpid();
	  	memcpy(caBuf,&iPid,sizeof(long));
	  	sprintf(caBuf+sizeof(long),"%04d",iSockId);

	  	memcpy(caBuf+sizeof(long)+5,sndbuf,offset);
			
		if( sg_qid <0)
	  		sg_qid = queue_connect("SECURVR");
	  	if ( sg_qid <0)
	  	{
	  		dcs_log(0,0,"connect msg queue fail!");
	  		return -1;
	  	}
		
	  	int qid1=-1;
		memset(tmpbuf,0,sizeof(tmpbuf));
		sprintf(tmpbuf,"%d",iPid);
		qid1 = queue_connect(tmpbuf);
		if(qid1 <0)
			qid1 = queue_create(tmpbuf);
	  	memcpy(sndbuf,caBuf,sizeof(long));
	  	queue_recv(qid1,sndbuf,sizeof(sndbuf),1);
	  	if (0 >queue_send( sg_qid ,caBuf,offset+sizeof(long)+5,0))
	  	{
	  		sg_qid=-1;
	  		dcs_log(0,0,"send msg to queue fail!");
	  		return -1;
	  	}

	  	memset(caBuf,0,sizeof(caBuf));
	  	memcpy(caBuf,&iPid,sizeof(long));
	  	if ( 0 >(rcvlen=queue_recv(qid1,caBuf,sizeof(caBuf),0)))
	  	{
	  		qid1=-1;
	  		dcs_log(0,0,"recv msg from msg queue fail!");
	  		return -1;
	  	}

	  	if ( memcmp(caBuf+sizeof(long),"FF",2)==0)
	  	{
	  		dcs_log(0,0," function processing fail!");
	  		return -2;
	  	}

		memcpy(rcvbuf,caBuf+sizeof(long)+2,rcvlen-sizeof(long)-2);

		offset = 1;

		if (rcvbuf[0] == 'A' )
		{
			memcpy(data_buf, rcvbuf + 1 + 2, 16);
			return(0);
		}
		else if (rcvbuf[0] == 'E')
		{
			memcpy(retcode, rcvbuf + offset, 1);
			dcs_debug(rcvbuf,rcvlen,"sjl05_mpos_keygen_encrypt error!!");
			return(-1);
		}
		else
		{
			return(-1);
		}

}

int D3des_dukpt(int iSockId, unsigned char *source, unsigned char * dest, int keyIndex, int flg, int dataLen)
{
	int offset = 0;
	int iPid = -1;
	int rcvlen = -1;
	
	char sndbuf[1024];
	char tmpbuf[128];
	char caBuf[2048];
	char rcvbuf[1024];
	char retcode[3];

	if ( source == NULL || dest == NULL )
		return -1;

	memset(sndbuf,0,sizeof(sndbuf));

	/* 指令类型 */
	memcpy(sndbuf, "72", 2);
	offset += 2;

	/*次主密钥索引,网络字节序*/
	sprintf(tmpbuf, "%04x", keyIndex);
	memcpy(sndbuf + offset, tmpbuf, 4);
	offset += 4;
	
	/*用户保留字 */
	memcpy(sndbuf+offset,"0000000000000000",16);
	offset += 16;

	/* 算法标识 */
	if ( flg == 1 )	/*加密*/
	{	
		sprintf(tmpbuf, "%02x", 1);
		memcpy(sndbuf + offset, tmpbuf, 2);
		offset += 2;
	}
	else 	/*解密*/
	{
		/* 算法标识 */
		sprintf(tmpbuf, "%02x", 0);
		memcpy(sndbuf + offset, tmpbuf, 2);
		offset += 2;
	}

	memcpy(sndbuf + offset, "00", 2);
	offset += 2;
	
	/* 数据长度 */
	sprintf(tmpbuf, "%04x", dataLen);
	memcpy(sndbuf + offset, tmpbuf, 4);
	offset += 4;

	/*用于计算的数据*/
	memset(rcvbuf,0,sizeof(rcvbuf));
	asc_to_bcd(rcvbuf, sndbuf, offset,1);
	offset = offset/2;

	memcpy(sndbuf, rcvbuf, offset);

	/* 数据 */
	memcpy(sndbuf + offset, source, dataLen);
	offset += dataLen;
	dcs_debug(sndbuf, offset, "dukptd3des , send to encrypt:");
	
	memset(caBuf,0,sizeof(caBuf));
  	iPid = getpid();

  	memcpy(caBuf,&iPid,sizeof(long));
  	sprintf(caBuf+sizeof(long),"%04d",iSockId);

  	memcpy(caBuf+sizeof(long)+5,sndbuf,offset);

	if( sg_qid <0)
  		sg_qid = queue_connect("SECURVR");

  	if ( sg_qid <0)
  	{
  		dcs_log(0,0,"connect msg queue fail!");
  		return -1;
  	}

  	int qid1=-1;
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%d",iPid);

	qid1 = queue_connect(tmpbuf);
	if(qid1 <0)
		qid1 = queue_create(tmpbuf);
	if ( qid1 <0)
	{
		dcs_log(0,0,"connect msg queue fail!");
		return -1;
	}

  	memcpy(sndbuf,caBuf,sizeof(long));
  	queue_recv(qid1,sndbuf,sizeof(sndbuf),1);
  	if (0 >queue_send( sg_qid ,caBuf,offset+sizeof(long)+5,0))
  	{
  		sg_qid=-1;
  		dcs_log(0,0,"send msg to queue fail!");
  		return -1;
  	}

  	memset(caBuf,0,sizeof(caBuf));
  	memcpy(caBuf,&iPid,sizeof(long));
  	if ( 0 >(rcvlen=queue_recv(qid1,caBuf,sizeof(caBuf),0)))
  	{
  		qid1=-1;
  		dcs_log(0,0,"recv msg from msg queue fail!");
  		return -1;
  	}

  	if ( memcmp(caBuf+sizeof(long),"FF",2)==0)
  	{
  		dcs_log(0,0," function processing fail!");
  		return -2;
  	}
  	
	memcpy(rcvbuf,caBuf+sizeof(long)+2,rcvlen-sizeof(long)-2);
		
	offset = 1;

	if (rcvbuf[0] == 'A' )
	{
		memcpy(dest, rcvbuf + 3, dataLen);
		return(0);
		
	}
	else if (rcvbuf[0] == 'E')
	{
		memcpy(retcode, rcvbuf + 1, 1);
		dcs_debug(retcode, 1, "D3des_dukpt error!");
		return(-1);
	}
	else
	{
		return(-1);
	}		
		
	return 0;
}

int des_dukpt(int iSockId, unsigned char *source,unsigned char * dest, int dataLen, unsigned char * inkey, int flg) 
{ 
	int offset = 0;
	int iPid = -1;
	int rcvlen = -1;
	
	char sndbuf[1024];
	char tmpbuf[128];
	char caBuf[2048];
	char rcvbuf[1024];
	char retcode[3];
	char inkeytmp[16+1];
	
	memset(retcode, 0x00, sizeof(retcode));
	memset(rcvbuf, 0x00, sizeof(rcvbuf));
	memset(caBuf, 0x00, sizeof(caBuf));
	memset(tmpbuf, 0x00, sizeof(tmpbuf));
	memset(sndbuf, 0x00, sizeof(sndbuf));

	if ( source == NULL || dest == NULL || inkey == NULL )
		return -1;
	
	memset(sndbuf,0,sizeof(sndbuf));
	
	/* 指令类型 */
	if(flg == 0)	/*加密*/
	{
		memcpy(sndbuf, "C008", 4);
		offset += 4;
	}
	else			/*解密*/
	{
		memcpy(sndbuf, "C009", 4);
		offset += 4;
	}

	/*用户保留字 */
	memcpy(sndbuf + offset, "0000000000000000", 16);
	offset += 16;

	/* DES密钥 */
	memset(rcvbuf,0,sizeof(rcvbuf));
	memset(inkeytmp, 0, sizeof(inkeytmp));
	bcd_to_asc(rcvbuf, inkey, 16,0);
	memcpy(inkeytmp, rcvbuf, 16);
	
	memcpy(sndbuf + offset, inkeytmp, 16);
	offset += 16;

	/* 初始向量 */
	memcpy(sndbuf + offset, "0000000000000000", 16);
	offset += 16;

	/* 报文数据 */
	memset(rcvbuf,0,sizeof(rcvbuf));
	asc_to_bcd(rcvbuf, sndbuf, offset,1);
	offset = offset/2;
	memcpy(sndbuf, rcvbuf, offset);

	/* 加密数据 */
	memcpy(sndbuf + offset, source, dataLen);
	offset += dataLen;

	memset(caBuf,0,sizeof(caBuf));
  	iPid = getpid();
  	memcpy(caBuf,&iPid,sizeof(long));
  	sprintf(caBuf+sizeof(long),"%04d",iSockId);
  
  	memcpy(caBuf+sizeof(long)+5,sndbuf,offset);
  	
	if( sg_qid <0)
  		sg_qid = queue_connect("SECURVR");
  	if ( sg_qid <0)
  	{
  		dcs_log(0,0,"connect msg queue fail!");
  		return -1;
  	}
  	int qid1=-1;
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%d",iPid);
	qid1 = queue_connect(tmpbuf);
	if(qid1 <0)
		qid1 = queue_create(tmpbuf);
	if ( qid1 <0)
	{
		dcs_log(0,0,"connect msg queue fail!");
		return -1;
	}	
  	memcpy(sndbuf,caBuf,sizeof(long));
  	queue_recv(qid1,sndbuf,sizeof(sndbuf),1);
  	if (0 >queue_send( sg_qid ,caBuf,offset+sizeof(long)+5,0))
  	{
  		sg_qid=-1;
  		dcs_log(0,0,"send msg to queue fail!");
  		return -1;
  	}
  	memset(caBuf,0,sizeof(caBuf));
  	memcpy(caBuf,&iPid,sizeof(long));
  	if ( 0 >(rcvlen=queue_recv(qid1,caBuf,sizeof(caBuf),0)))
  	{
  		qid1=-1;
  		dcs_log(0,0,"recv msg from msg queue fail!");
  		return -1;
  	}

  	if ( memcmp(caBuf+sizeof(long),"FF",2)==0)
  	{
  		dcs_log(0,0," function processing fail!");
  		return -2;
  	}
  		
	memcpy(rcvbuf,caBuf+sizeof(long)+2,rcvlen-sizeof(long)+2);

	offset = 1;
	
	if (rcvbuf[0] == 'A' )
	{
		memcpy(dest, rcvbuf + 9, dataLen);
		//dcs_debug(dest,dataLen,"C008加密机返回结果:");
		return(0);
	}
	else if (rcvbuf[0] == 'E')
	{
		memcpy(retcode, rcvbuf + 1, 1);
		dcs_debug(retcode, 1, "des_dukpt error!");
		return(-1);
	}
	else
	{
		return(-1);
	}		
		
	return 0;

}
int getKey(unsigned char *dukptkey,int type,unsigned char *workey)
{
		unsigned char dupkptKey[16+1];
		
		asc_to_bcd(dupkptKey, dukptkey, 32, 0);
		
		switch (type) {
			case SEC_KEY_TYPE_TPK:		
				dupkptKey[7] ^= 0xff;
				dupkptKey[15] ^= 0xff;
				break;
			case SEC_KEY_TYPE_TAK:		
				dupkptKey[6] ^= 0xff;
				dupkptKey[14] ^= 0xff;
				break;
			case SEC_KEY_TYPE_TDK:		
				dupkptKey[5] ^= 0xff;
				dupkptKey[13] ^= 0xff;
				break;
		}

		memcpy(workey, dupkptKey, sizeof(dupkptKey));
		
		return 1;
}

int GetDukptKey(unsigned char *ksn, unsigned char *key,unsigned char *dukptkey) 
{
		int i = 0, num = 0;
		int ret = 0;
		unsigned char KSNTemp[8+1], counter[3], counterTemp[3];

		memset(KSNTemp, 0, sizeof(KSNTemp));
		memset(counter, 0, sizeof(counter));
		memset(counterTemp, 0, sizeof(counterTemp));
#ifdef __TEST__
		dcs_debug(ksn, 16,"KSN:");
		dcs_debug(key, 16,"KEY:");
#endif
		for (i = 0; i < 8; i++)
			KSNTemp[i] = ksn[i + 2];
		KSNTemp[5] &= 0xE0;
		KSNTemp[6] = 0;
		KSNTemp[7] = 0;
		
		counter[0] = (byte) (ksn[7] & 0x1F);
		counter[1] = ksn[8];
		counter[2] = ksn[9];
		num = CountOne(counter[0]);
		num += CountOne(counter[1]);
		num += CountOne(counter[2]);

		SearchOne(counter, counterTemp);
		procCounter(KSNTemp, counter, counterTemp);
#ifdef __TEST__		
		dcs_log(0 ,0, "DUKPT 步骤二 计算次数 num[%d]", num);
		dcs_debug(KSNTemp, 8, "KSNTemp:");
		dcs_debug(counter, 3, "counter:");
		dcs_debug(counterTemp, 3, "counterTemp:");	
#endif		
		while (num > 0) {
			ret = NonRevKeyGen(KSNTemp, key);
			if(ret < 0)
			{
				return ret;
			}
			SearchOne(counter, counterTemp);
			procCounter(KSNTemp, counter, counterTemp);
			num--;
			if(num>0)
			{
			#ifdef __TEST__
					dcs_log(0 ,0, "DUKPT 步骤二 计算次数 num[%d]", num);
					dcs_debug(KSNTemp, 8, "KSNTemp:");
					dcs_debug(counter, 3, "counter:");
					dcs_debug(counterTemp, 3, "counterTemp:");	
			#endif
			}
			
		}

		memcpy(dukptkey, key, 16);
		return 1;
}
	
int GetIPEK(unsigned char *ksn, int bdk, int bdkxor, unsigned char *ipek) 
{
		unsigned char ksnTemp[8+1];
		unsigned char keyTemp[16+1];
		unsigned char result[16+1], resultTemp[16+1];
		int i;
	 	struct secLinkInfo * pLink = NULL;

		memset(ksnTemp, 0, sizeof(ksnTemp));
		memset(keyTemp, 0, sizeof(keyTemp));
		memset(result, 0, sizeof(result));
		memset(resultTemp, 0, sizeof(resultTemp));
	
		// mask KSN to get serial number part of KSN
		for (i = 0; i < 8; i++)
			ksnTemp[i] = ksn[i];
		ksnTemp[7] &= 0xE0;

		//得到K1
		if (!PEP_DESInit())
		{
			dcs_log(0,0," init des system fail");
			return -1;
		}
		 	
		pLink = PEP_GetFreeLink();
		if ( pLink == NULL ) 
		{
			dcs_log(0,0,"get free link fail");
			return -2;
		}
		
		if ( memcmp(pLink->cEncType,"01",2)==0)
		{
			i = D3des_dukpt(pLink->iSocketId, ksnTemp, resultTemp, bdk, 1, 8);
			if ( i == -1 )
		 	{		
				PEP_FreeLink( pLink);	
				dcs_log(0,0,"Decrypt Track fail!");
		 		return -3;
		 	}
		 	else if ( i == -2)
		 	{
		 		pLink->iStatus= DCS_STAT_DISCONNECTED;
		 		XPE_SendFailMessage( pLink->iNum);
		 		PEP_FreeLink( pLink);	
		 		dcs_log(0,0,"Comm fail!");
		 		return -4;
		 	}
		}
		else
		{
			PEP_FreeLink( pLink);	
			dcs_log(0,0,"不支持的加密机类型");
			return -5;
		}

		PEP_FreeLink( pLink);

		for (i = 0; i < 8; i++)
			result[i] = resultTemp[i];

		pLink = PEP_GetFreeLink();
		if ( pLink == NULL ) 
		{
			dcs_log(0,0,"get free link fail");
			return -2;
		}
		
		if ( memcmp(pLink->cEncType,"01",2)==0)
		{
			i = D3des_dukpt(pLink->iSocketId, ksnTemp, resultTemp, bdkxor, 1, 8);
			if ( i == -1 )
		 	{		
				PEP_FreeLink( pLink);	
				dcs_log(0,0,"Decrypt Track fail!");
		 		return -3;
		 	}
		 	else if ( i == -2)
		 	{
		 		pLink->iStatus= DCS_STAT_DISCONNECTED;
		 		XPE_SendFailMessage( pLink->iNum);
		 		PEP_FreeLink( pLink);	
		 		dcs_log(0,0,"Comm fail!");
		 		return -4;
		 	}
		}
		else
		{
			PEP_FreeLink( pLink);	
			dcs_log(0,0,"不支持的加密机类型");
			return -5;
		}
		PEP_FreeLink( pLink);
		
		for (i = 0; i < 8; i++)
			result[i + 8] = resultTemp[i];

		memcpy(ipek, result, sizeof(result));
		
dcs_debug(ipek, 16, "GetIPK initkey:");
}

int NonRevKeyGen(unsigned char *cReg1, unsigned char *key) 
{
		int i;
		unsigned char keyTemp1[8+1], keyTemp2[8+1], data[8+1];
		unsigned char result[16+1], resultTemp[16+1];
		
	 	struct secLinkInfo * pLink = NULL;
		
		memset(keyTemp1, 0, sizeof(keyTemp1));
		memset(keyTemp2, 0, sizeof(keyTemp2));
		memset(data, 0, sizeof(data));
		memset(result, 0, sizeof(result));
		memset(resultTemp, 0, sizeof(resultTemp));
		
		for (i = 0; i < 8; i++) {
			keyTemp1[i] = key[i];
			keyTemp2[i] = key[i + 8];
		}

		for (i = 0; i < 8; i++)
		{
			data[i] = (byte) (cReg1[i] ^ keyTemp2[i]);
		}
		
		
		if (!PEP_DESInit())
		{
			dcs_log(0,0," init des system fail");
			return -1;
		}
		 	
		pLink = PEP_GetFreeLink();
		if ( pLink == NULL ) 
		{
			dcs_log(0,0,"get free link fail");
			return -2;
		}
		
		if ( memcmp(pLink->cEncType,"01",2)==0)
		{
			i = des_dukpt(pLink->iSocketId, data, resultTemp, 8, keyTemp1, 0);
			if ( i == -1 )
		 	{		
				PEP_FreeLink( pLink);	
				dcs_log(0,0,"Decrypt Track fail!");
		 		return -3;
		 	}
		 	else if ( i == -2)
		 	{
		 		pLink->iStatus= DCS_STAT_DISCONNECTED;
		 		XPE_SendFailMessage( pLink->iNum);
		 		PEP_FreeLink( pLink);	
		 		dcs_log(0,0,"Comm fail!");
		 		return -4;
		 	}
		}
		else
		{
			PEP_FreeLink( pLink);	
			dcs_log(0,0,"不支持的加密机类型");
			return -5;
		}
		PEP_FreeLink( pLink);
	
		for (i = 0; i < 8; i++)
		{
			result[i + 8] = (byte) (resultTemp[i] ^ keyTemp2[i]);
		}
		
		keyTemp1[0] ^= 0xC0;
		keyTemp1[1] ^= 0xC0;
		keyTemp1[2] ^= 0xC0;
		keyTemp1[3] ^= 0xC0;
		keyTemp2[0] ^= 0xC0;
		keyTemp2[1] ^= 0xC0;
		keyTemp2[2] ^= 0xC0;
		keyTemp2[3] ^= 0xC0;

		memset(data, 0, sizeof(data));
		for (i = 0; i < 8; i++)
		{
			data[i] = (byte) (cReg1[i] ^ keyTemp2[i]);
		}
		
		if (!PEP_DESInit())
		{
			dcs_log(0,0," init des system fail");
			return -1;
		}
		 	
		pLink = PEP_GetFreeLink();
		if ( pLink == NULL ) 
		{
			dcs_log(0,0,"get free link fail");
			return -2;
		}
		
		if ( memcmp(pLink->cEncType,"01",2)==0)
		{
			i = des_dukpt(pLink->iSocketId, data, resultTemp, 8, keyTemp1, 0);
			if ( i == -1 )
		 	{		
				PEP_FreeLink( pLink);	
				dcs_log(0,0,"Decrypt Track fail!");
		 		return -3;
		 	}
		 	else if ( i == -2)
		 	{
		 		pLink->iStatus= DCS_STAT_DISCONNECTED;
		 		XPE_SendFailMessage( pLink->iNum);
		 		PEP_FreeLink( pLink);	
		 		dcs_log(0,0,"Comm fail!");
		 		return -4;
		 	}
		}
		else
		{
			PEP_FreeLink( pLink);	
			dcs_log(0,0,"不支持的加密机类型");
			return -5;
		}
		PEP_FreeLink( pLink);

		for (i = 0; i < 8; i++)
			result[i] = (byte) (resultTemp[i] ^ keyTemp2[i]);
	
		memcpy(key, result, sizeof(result));
		return 1;
}

int CountOne(byte input) 
{
		int temp = 0;
		if ((input & 0x80) != 0)
			temp++;
		if ((input & 0x40) != 0)
			temp++;
		if ((input & 0x20) != 0)
			temp++;
		if ((input & 0x10) != 0)
			temp++;
		if ((input & 0x08) != 0)
			temp++;
		if ((input & 0x04) != 0)
			temp++;
		if ((input & 0x02) != 0)
			temp++;
		if ((input & 0x01) != 0)
			temp++;
		return temp;
}

int SearchOne(unsigned char *counter, unsigned char *rtn) 
{
		unsigned char result[3];
		memset(result, 0, sizeof(result));
		if (counter[0] == 0) {
			if (counter[1] == 0)
				result[2] = SearchOneCore(counter[2]);
			else
				result[1] = SearchOneCore(counter[1]);
		} else
			result[0] = SearchOneCore(counter[0]);
			
		memcpy(rtn, result, sizeof(result));
		return 1;
}

byte SearchOneCore(byte input)
{
		if ((input & 0x80) != 0)
			return (byte) (0x80);
		if ((input & 0x40) != 0)
			return (byte) (0x40);
		if ((input & 0x20) != 0)
			return (byte) (0x20);
		if ((input & 0x10) != 0)
			return (byte) (0x10);
		if ((input & 0x08) != 0)
			return (byte) (0x08);
		if ((input & 0x04) != 0)
			return (byte) (0x04);
		if ((input & 0x02) != 0)
			return (byte) (0x02);
		if ((input & 0x01) != 0)
			return (byte) (0x01);
		return (byte)0;
}

void procCounter(unsigned char *ksn, unsigned char *counter, unsigned char *counterTemp) 
{
		ksn[5] |= counterTemp[0];
		ksn[6] |= counterTemp[1];
		ksn[7] |= counterTemp[2];
		counter[0] ^= counterTemp[0];
		counter[1] ^= counterTemp[1];
		counter[2] ^= counterTemp[2];
}

int MposGetLmk(int iSockId, unsigned char *source, unsigned char * dest, int dataLen, int flag)
{
	int offset = 0;
	int iPid = -1;
	int rcvlen = -1;
	
	char sndbuf[1024];
	char tmpbuf[128];
	char caBuf[2048];
	char rcvbuf[1024];
	char retcode[3];

	if ( source == NULL || dest == NULL )
		return -1;

	memset(sndbuf,0,sizeof(sndbuf));

	/* 指令类型 */
	memcpy(sndbuf, "D108", 4);
	offset += 4;
	
	/* 数据长度 */
	sprintf(tmpbuf, "%02x", dataLen);
	memcpy(sndbuf + offset, tmpbuf, 2);
	offset += 2;
	
	/*密钥类型*/
	if(flag == 1)
	{
		memcpy(sndbuf+offset,"11",2);/*tpk*/
	}
	else if(flag == 2)
	{
		memcpy(sndbuf+offset,"12",2);/*tak*/
	}
	else if(flag == 3)
	{
		memcpy(sndbuf+offset,"13",2);/*tdk*/
	}
	offset += 2;
	
	/*用于计算的数据*/
	memset(rcvbuf,0,sizeof(rcvbuf));
	asc_to_bcd(rcvbuf, sndbuf, offset,1);
	offset = offset/2;

	memcpy(sndbuf, rcvbuf, offset);

	/* 数据 */
	memcpy(sndbuf + offset, source, dataLen);
	offset += dataLen;

	//dcs_debug(sndbuf, offset, "dukptd3des , send to encrypt:");
	
	memset(caBuf,0,sizeof(caBuf));
  	iPid = getpid();

  	memcpy(caBuf,&iPid,sizeof(long));
  	sprintf(caBuf+sizeof(long),"%04d",iSockId);

  	memcpy(caBuf+sizeof(long)+5,sndbuf,offset);

	if( sg_qid <0)
  		sg_qid = queue_connect("SECURVR");

  	if ( sg_qid <0)
  	{
  		dcs_log(0,0,"connect msg queue fail!");
  		return -1;
  	}

  	int qid1=-1;
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%d",iPid);

	qid1 = queue_connect(tmpbuf);
	if(qid1 <0)
		qid1 = queue_create(tmpbuf);
	if ( qid1 <0)
	{
		dcs_log(0,0,"connect msg queue fail!");
		return -1;
	}

  	memcpy(sndbuf,caBuf,sizeof(long));
  	queue_recv(qid1,sndbuf,sizeof(sndbuf),1);
  	if (0 >queue_send( sg_qid ,caBuf,offset+sizeof(long)+5,0))
  	{
  		sg_qid=-1;
  		dcs_log(0,0,"send msg to queue fail!");
  		return -1;
  	}

  	memset(caBuf,0,sizeof(caBuf));
  	memcpy(caBuf,&iPid,sizeof(long));
  	if ( 0 >(rcvlen=queue_recv(qid1,caBuf,sizeof(caBuf),0)))
  	{
  		qid1=-1;
  		dcs_log(0,0,"recv msg from msg queue fail!");
  		return -1;
  	}

  	if ( memcmp(caBuf+sizeof(long),"FF",2)==0)
  	{
  		dcs_log(0,0," function processing fail!");
  		return -2;
  	}
  	
	memcpy(rcvbuf,caBuf+sizeof(long)+2,rcvlen-sizeof(long)-2);
	
	offset = 1;

	if (rcvbuf[0] == 'A' )
	{
		dataLen = rcvbuf[1];
		memcpy(dest, rcvbuf+2 , dataLen);
		return(0);
		
	}
	else if (rcvbuf[0] == 'E')
	{
		memcpy(retcode, rcvbuf + 1, 1);
		dcs_debug(retcode, 1, "sjl05_pos_trackdecrypt decrypt error!");
		return(-1);
	}
	else
	{
		return(-1);
	}		
		
	return 0;
}

int getmposlmk(char *workkey, char *resultTemp, int dataLen, int flag)
{
	
	struct secLinkInfo * pLink = NULL;
	
	int i = -1;
	
	if (!PEP_DESInit())
	{
		dcs_log(0,0," init des system fail");
		return -1;
	}
		 	
	pLink = PEP_GetFreeLink();
	if ( pLink == NULL ) 
	{
		dcs_log(0,0,"get free link fail");
		return -2;
	}
	
	if ( memcmp(pLink->cEncType,"01",2)==0)
	{
		i = MposGetLmk(pLink->iSocketId, workkey, resultTemp,dataLen, flag);
		if ( i == -1 )
	 	{		
			PEP_FreeLink( pLink);	
			dcs_log(0,0,"Decrypt Track fail!");
	 		return -3;
	 	}
	 	else if ( i == -2)
		{
		 	pLink->iStatus= DCS_STAT_DISCONNECTED;
		 	XPE_SendFailMessage( pLink->iNum);
		 	PEP_FreeLink( pLink);	
		 	dcs_log(0,0,"Comm fail!");
		 	return -4;
		}
	}
	else
	{
		PEP_FreeLink( pLink);	
		dcs_log(0,0,"不支持的加密机类型");
		return -5;
	}
	PEP_FreeLink( pLink);
	return 0;
}

int mpos_mac(char *mack, char *pre_mac, char *data, int dataLen, char *rtnData)
{
	 int i;
	 struct secLinkInfo * pLink = NULL;
	
	 if (!PEP_DESInit())
	 {
	 	 dcs_log(0,0," init des system fail");
	 	 return -1;
	 }
	 	
	 pLink = PEP_GetFreeLink();
	 if ( pLink == NULL ) 
	 {
	 	  dcs_log(0,0,"get free link fail");
	 	  return -2;
	 }
	 
	 if ( memcmp(pLink->cEncType,"01",2)==0)
	 {
	 	   i = sjl05_mpos_mac(pLink->iSocketId, mack, pre_mac, data, dataLen, rtnData);
	 	  if ( i == -1 )
	 	  {		
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"epos_hbmac fail!");
	 			return -3;
	 	  }
	 	  else if ( i == -2)
	 	  {
	 			pLink->iStatus= DCS_STAT_DISCONNECTED;
	 			XPE_SendFailMessage( pLink->iNum);
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"Comm fail!");
	 			return -4;
	 	  }
	 }
	 else
	 {
	 	 PEP_FreeLink( pLink);	
	 	 dcs_log(0,0,"不支持的加密机类型");
	 	 return -5;
	 }
	 PEP_FreeLink( pLink);
	 return 0;
}

int sjl05_mpos_mac(int iSockId, char  *mack_asc, char *pre_mac, char *data, int dataLen, char *rtnData)
{
	int rtn, i,iPid;
	int offset = 0, sndlen = 0, rcvlen, tmplen;
	char retcode[3];
	char sndbuf[1024], rcvbuf[1024],caBuf[1024];
	char tmpbuf[10];
	char ksntmp[16+1];
//	char mack_asc[32+1];
	
	memset(sndbuf, 0, sizeof(sndbuf));

#ifdef __TEST__
	dcs_log(0, 0, "mack_asc=[%s]", mack_asc);
	dcs_log(0, 0, "pre_mac=[%s]", pre_mac);
	dcs_log(0, 0, "data=[%s]", data);
	dcs_log(0, 0, "dataLen=[%d]", dataLen);
#endif
	/* 指令类型 */
	memcpy(sndbuf, "D134", 4);
	offset += 4;
	
	/* 算法标识 */
	sprintf(tmpbuf, "%02x", 3);
	memcpy(sndbuf + offset, tmpbuf, 2);
	offset += 2;
	
	/* MAK长度 */
	sprintf(tmpbuf, "%02x", 16);
	memcpy(sndbuf + offset, tmpbuf, 2);
	offset += 2;
	
	/* MAK */
	memcpy(sndbuf + offset, mack_asc, 32);
	offset += 32;
	
	/* 初始向量 */
	memcpy(sndbuf+offset, "0000000000000000", 16);
	offset += 16;
	
	/* 待校验MAC */
	memcpy(sndbuf + offset, pre_mac, 8);
	offset += 8;
	
	/* 数据长度 */
	sprintf(tmpbuf, "%04x", dataLen);
	memcpy(sndbuf + offset, tmpbuf, 4);
	offset += 4;

	memset(rcvbuf, 0, sizeof(rcvbuf));
	asc_to_bcd(rcvbuf, sndbuf, offset, 1);
	offset = offset/2;
	memcpy(sndbuf, rcvbuf, offset);
	
	/* 数据  */
	memcpy(sndbuf + offset, data, dataLen);
	offset += dataLen;
	
	//dcs_debug(sndbuf, offset, "sjl05_ewphb_mac , send to encrypt:");

    	memset(caBuf,0,sizeof(caBuf));
  	iPid = getpid();
  	memcpy(caBuf,&iPid,sizeof(long));
  	sprintf(caBuf+sizeof(long),"%04d",iSockId);

  	memcpy(caBuf+sizeof(long)+5,sndbuf,offset);
  	
	if( sg_qid <0)
  		sg_qid = queue_connect("SECURVR");
  	if ( sg_qid <0)
  	{
  		dcs_log(0,0,"connect msg queue fail!");
  		return -1;
  	}
  	int qid1=-1;
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%d",iPid);
	qid1 = queue_connect(tmpbuf);
	if(qid1 <0)
		qid1 = queue_create(tmpbuf);
	if ( qid1 <0)
	{
		dcs_log(0,0,"connect msg queue fail!");
		return -1;
	}
  	memcpy(sndbuf,caBuf,sizeof(long));
  	queue_recv(qid1,sndbuf,sizeof(sndbuf),1);
  	if (0 >queue_send( sg_qid ,caBuf,offset+sizeof(long)+5,0))
  	{
  		sg_qid=-1;
  		dcs_log(0,0,"send msg to queue fail!");
  		return -1;
  	}

  	memset(caBuf,0,sizeof(caBuf));
  	memcpy(caBuf,&iPid,sizeof(long));
  	if ( 0 >(rcvlen=queue_recv(qid1,caBuf,sizeof(caBuf),0)))
  	{
  		qid1=-1;
  		dcs_log(0,0,"recv msg from msg queue fail!");
  		return -1;
  	}

  	if ( memcmp(caBuf+sizeof(long),"FF",2)==0)
  	{
  		dcs_log(0,0," function processing fail!");
  		return -2;
  	}
	
	memcpy(rcvbuf,caBuf+sizeof(long)+2,rcvlen-sizeof(long)+2);
	
	offset = 1;
	
	if (rcvbuf[0] == 'A' )
	{
		memcpy(rtnData, rcvbuf + 1, 8);
		return(0);
	}
	else if (rcvbuf[0] == 'E')
	{
		memcpy(retcode, rcvbuf + 1, 1);
		dcs_debug(retcode, 1, "sjl05_ewphb_mac decrypt error!");
		return(-1);
	}
	else
	{
		return(-1);
	}
}

//传输密钥加密initkey
int getdeckey(unsigned char *initkey, int bdkindex, unsigned char *ipek) {
		unsigned char initkeyTemp[16+1];
		unsigned char result[16+1], resultTemp[16+1];
		int i;
	 	struct secLinkInfo * pLink = NULL;

		memset(initkeyTemp, 0, sizeof(initkeyTemp));
		memset(result, 0, sizeof(result));
		memset(resultTemp, 0, sizeof(resultTemp));
	
		// mask KSN to get serial number part of KSN
		for (i = 0; i < 16; i++)
			initkeyTemp[i] = initkey[i];

		//得到加密后初始密钥
		if (!PEP_DESInit())
		{
			dcs_log(0,0," init des system fail");
			return -1;
		}
		 	
		pLink = PEP_GetFreeLink();
		if ( pLink == NULL ) 
		{
			dcs_log(0,0,"get free link fail");
			return -2;
		}
		
		if ( memcmp(pLink->cEncType,"01",2)==0)
		{
			i = D3des_dukpt(pLink->iSocketId, initkeyTemp, resultTemp, bdkindex, 1, 16);
			if ( i == -1 )
		 	{		
				PEP_FreeLink( pLink);	
				dcs_log(0,0,"Decrypt Track fail!");
		 		return -3;
		 	}
		 	else if ( i == -2)
		 	{
		 		pLink->iStatus= DCS_STAT_DISCONNECTED;
		 		XPE_SendFailMessage( pLink->iNum);
		 		PEP_FreeLink( pLink);	
		 		dcs_log(0,0,"Comm fail!");
		 		return -4;
		 	}
		}
		else
		{
			PEP_FreeLink( pLink);	
			dcs_log(0,0,"不支持的加密机类型");
			return -5;
		}

		PEP_FreeLink( pLink);

		for (i = 0; i < 16; i++)
			result[i] = resultTemp[i];

		memcpy(ipek, result, sizeof(result));
	#ifdef __TEST__
   		dcs_debug(ipek, 16, "传输密钥加密后 initkey:");
	#endif
	return 0;
}

//initkey明文对8个0x00做加密 取校验值
int getcheckvalue(unsigned char *initkey, unsigned char *ipek) {
		unsigned char initkeyTemp[16+1];
		unsigned char result[16+1], resultTemp[16+1];
		int i;
	 	struct secLinkInfo * pLink = NULL;

		memset(initkeyTemp, 0, sizeof(initkeyTemp));
		memset(result, 0, sizeof(result));
		memset(resultTemp, 0, sizeof(resultTemp));
	
		// mask KSN to get serial number part of KSN
		for (i = 0; i < 16; i++)
			initkeyTemp[i] = initkey[i];


		//i = D3des((unsigned char *)"\x00\x00\x00\x00\x00\x00\x00\x00", (unsigned char *)resultTemp, (unsigned char *)initkey, 1);
		i = D3desbylen((unsigned char *)"\x00\x00\x00\x00\x00\x00\x00\x00", 8, (unsigned char *)resultTemp, (unsigned char *)initkey, 1);
		if ( i == -1 )
	 	{		
	 		return -1;
	 	}

		for (i = 0; i < 4; i++)
			result[i] = resultTemp[i];

		memcpy(ipek, result, sizeof(result));
	#ifdef __TEST__	
		dcs_debug(ipek, 8, "密文initkey校验值 checkvalue: ");
	#endif
	return 0;
}



/*just add for test*/
int sjl05_mpos_pincon(int iSockId, char *pik1, char *pik2, char *card, char *outPin)
{
	int rtn, i,iPid;
	int offset = 0, sndlen = 0, rcvlen, tmplen;
	char retcode[3];
	char sndbuf[1024], rcvbuf[1024],caBuf[2048];
	char tmpbuf[128];

	memset(sndbuf, 0, sizeof(sndbuf));
	
	/* 指令类型 */
	memcpy(sndbuf, "D122", 4);
	offset += 4;
	
	sprintf(tmpbuf, "%02x", 16);
	memcpy(sndbuf + offset, tmpbuf, 2);
	offset += 2;
	
	//lmk加密的pink
	memcpy(sndbuf + offset, pik1, 32);
	offset += 32;

	sprintf(tmpbuf, "%02x", 1);
	memcpy(sndbuf + offset, tmpbuf, 2);
	offset += 2;
	
	memcpy(sndbuf + offset, "06", 2);
	offset += 2;
	
	//密文pin
	memcpy(sndbuf + offset, pik2, 12);
	offset += 12;
	
	memset(rcvbuf,0,sizeof(rcvbuf));
	asc_to_bcd(rcvbuf, sndbuf, offset,1);
	offset = offset/2;
	memcpy(sndbuf, rcvbuf, offset);
	
	memcpy(sndbuf + offset, card, strlen(card));
	offset += strlen(card);
	
	//dcs_debug(sndbuf, offset, "sjl05_pos_pincon , send to encrypt:");

    	memset(caBuf,0,sizeof(caBuf));
  	iPid = getpid();
  	memcpy(caBuf,&iPid,sizeof(long));
  	sprintf(caBuf+sizeof(long),"%04d",iSockId);

  	memcpy(caBuf+sizeof(long)+5,sndbuf,offset);
  	
	if( sg_qid <0)
  		sg_qid = queue_connect("SECURVR");
  	if ( sg_qid <0)
  	{
  		dcs_log(0,0,"connect msg queue fail!");
  		return -1;
  	}
  	int qid1=-1;
	memset(tmpbuf,0,sizeof(tmpbuf));
	sprintf(tmpbuf,"%d",iPid);
	qid1 = queue_connect(tmpbuf);
	if(qid1 <0)
		qid1 = queue_create(tmpbuf);
	if ( qid1 <0)
	{
		dcs_log(0,0,"connect msg queue fail!");
		return -1;
	}
	  memcpy(sndbuf,caBuf,sizeof(long));
  	queue_recv(qid1,sndbuf,sizeof(sndbuf),1);
  	if (0 >queue_send( sg_qid ,caBuf,offset+sizeof(long)+5,0))
  	{
  		sg_qid=-1;
  		dcs_log(0,0,"send msg to queue fail!");
  		return -1;
  	}

  	memset(caBuf,0,sizeof(caBuf));
  	memcpy(caBuf,&iPid,sizeof(long));
  	if ( 0 >(rcvlen=queue_recv(qid1,caBuf,sizeof(caBuf),0)))
  	{
  		qid1=-1;
  		dcs_log(0,0,"recv msg from msg queue fail!");
  		return -1;
  	}

  	if ( memcmp(caBuf+sizeof(long),"FF",2)==0)
  	{
  		dcs_log(0,0," function processing fail!");
  		return -2;
  	}
	
	memcpy(rcvbuf,caBuf+sizeof(long)+2,rcvlen-sizeof(long)+2);
	
	offset = 1;
	
	if (rcvbuf[0] == 'A' )
	{
		memcpy(outPin, rcvbuf + 1, 8);
		return(0);
	}
	else if (rcvbuf[0] == 'E')
	{
		memcpy(retcode, rcvbuf + 1, 1);
		dcs_debug(retcode, 1, "pos pin convert error!");
		return(-1);
	}
	else
	{
		return(-1);
	}
}

//just add for test
int MPOS_PinCon(char *pik1, char *pik2, char *card, char *rtnData)
{
	 int i;
	 struct secLinkInfo * pLink = NULL;
	
	 if (!PEP_DESInit())
	 {
	 	 dcs_log(0,0," init des system fail");
	 	 return -1;
	 }
	 	
	 pLink = PEP_GetFreeLink();
	 if ( pLink == NULL ) 
	 {
	 	  dcs_log(0,0,"get free link fail");
	 	  return -2;
	 }
	 
	 if ( memcmp(pLink->cEncType,"01",2)==0)
	 {
	 	  i = sjl05_mpos_pincon(pLink->iSocketId, pik1, pik2, card, rtnData);
	 	  if ( i == -1 )
	 	  {		
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"Decrypt Pin fail!");
	 			return -3;
	 	  }
	 	  else if ( i == -2)
	 	  {
	 			pLink->iStatus= DCS_STAT_DISCONNECTED;
	 			XPE_SendFailMessage( pLink->iNum);
	 			PEP_FreeLink( pLink);	
	 			dcs_log(0,0,"Comm fail!");
	 			return -4;
	 	  }
	 }
	 else
	 {
	 	 PEP_FreeLink( pLink);	
	 	 dcs_log(0,0,"不支持的加密机类型");
	 	 return -5;
	 }
	 PEP_FreeLink( pLink);
	 return 0;
}
