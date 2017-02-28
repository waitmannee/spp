#include "ibdcs.h"
#include "trans.h"
#include "cnew.h"
#include "sms.h"
#include "pub_function.h"
#include "syn_http.h"


#define NON_NUM '0'

extern struct ISO_8583 *iso8583;
extern struct ISO_8583 iso8583_conf[128];
extern struct ISO_8583 iso8583conf[128];
extern struct ISO_8583 iso8583posconf[128];
extern char g_key[20+1];
int gsem_id = -1;

/*
	功能：解密磁道和pin转加密
	输入参数：trackRandom,track,secuinfo
	
	输出参数：termbuf,siso
*/
int DecryptTrack(char *trackRandom, char *track, TERMMSGSTRUCT *termbuf, ISO_data *siso, SECUINFO secuinfo,char *outcard)
{
	int	keyIndex,s,n,j,outlen;
	char psam[16+1],tmpbuf[256],data_buf[256],cdno[20],bcdtrack[80+1],tmp_track[150];
	
	memset(psam, 0, sizeof(psam));
	memset(cdno, 0, sizeof(cdno));
	memset(data_buf, 0, sizeof(data_buf));
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(bcdtrack, 0, sizeof(bcdtrack));
	
	memcpy(psam, secuinfo.sam_card, 16);
	
	/*
		音频刷卡设备
	*/
	if( memcmp(secuinfo.sam_model,"1",1) == 0)
	{
		keyIndex = secuinfo.sam_tdkey_idx;
		outlen = getstrlen(track);
		asc_to_bcd( (unsigned char*) bcdtrack, (unsigned char*) track, getstrlen(track), 0 );
		s = EPOS_DecryptTrack(keyIndex, psam, trackRandom, bcdtrack, outlen/2, tmpbuf);
		if(s < 0)
		{
			dcs_log(0,0, "加密机错误,s=%d",s);
			return -1;
		}
		bcd_to_asc((unsigned char *) data_buf, (unsigned char*) tmpbuf, outlen, 0);
	}
	/*
		一体机
	*/
	else if( memcmp(secuinfo.sam_model, "2", 1) == 0)
	{
		keyIndex = secuinfo.sam_tdkey_idx;
		outlen = getstrlen(track);
		asc_to_bcd( (unsigned char*) bcdtrack, (unsigned char*) track, getstrlen(track), 0 );
		s = EPOS_DecryptTrack(keyIndex, psam, trackRandom, bcdtrack, outlen/2, tmpbuf);
		if(s < 0)
		{
			dcs_log(0,0, "加密机错误,s=%d",s);
			return -1;
		}
		bcd_to_asc((unsigned char *) data_buf, (unsigned char*) tmpbuf, outlen, 0);
	}
	/*
		车载USB POS
	*/
	//else if( memcmp(secuinfo.sam_model, "3", 1) == 0)
	//{
	//	keyIndex = secuinfo.sam_tdkey_idx;
	//	outlen = getstrlen(track);
	//	asc_to_bcd( (unsigned char*) bcdtrack, (unsigned char*) track, getstrlen(track), 0 );
	//	s = EPOS_DecryptTrack(keyIndex, psam, trackRandom, bcdtrack, outlen/2, tmpbuf);
	//	if(s < 0)
	//	{
	//		dcs_log(0,0, "加密机错误,s=%d",s);
	//		return -1;
	//	}
	//	bcd_to_asc((unsigned char *) data_buf, (unsigned char*) tmpbuf, outlen, 0);
	//}
	else
	{
		dcs_log(0,0, "设备属性错误，返回-3.");
		return -3;
	}
	
	/*
	dcs_debug(tmpbuf, outlen/2, "加密机返回磁道：%d",outlen/2);
	
	dcs_log(0, 0, "加密机返回磁道：%s", data_buf);
	*/
	
	for(n=0;n<outlen;n++)
	{
		if(data_buf[n]=='D')
		{
			data_buf[n]='=';
			if( n<30 )
				s = n;
		}
		//if( s == 0 )
	    //	tmpbuf[n] = data_buf[n];
	}
	
	//dcs_debug(data_buf, outlen, "磁道后：%d",outlen);
	
	/*取主账号错误*/
	if (s<10 || s>19)
		return -2;
   	
	memcpy(cdno, data_buf, s);
        	
//	dcs_log(0,0, "转出卡=%s", cdno);
	
	memcpy(outcard, data_buf, s);
	setbit(siso,2,(unsigned char *)cdno,s);
	
	/*
		第二磁道最长48个字节
	*/
	for(j=0;j<48;j++)
	{
		if( data_buf[j] == 'F' || j > 37)
			break;
	}
	
	if(j > 37){
		dcs_log(0,0, "解析磁道不对，不存在结束符F");
		return -4;
	}
		
	memset(tmp_track, 0, sizeof(tmp_track));
    memcpy(tmp_track, data_buf, j);
    #ifdef __TEST__
    	dcs_log(0,0, "set track2=%s,j=%d", tmp_track, j);
   	#endif
	setbit(siso, 35, (unsigned char *)tmp_track, j);
	
	if(outlen > 48)
	{
		for(j=0; j<outlen-48; j++)
		{
			if(data_buf[48+j]=='F')
				break;
		}
		

//		dcs_debug(data_buf+48, j, "磁道3,j=%d",j);
		/*
		if(j==0)
		{
			dcs_log(0,0, "没有三磁信息");
		}
		else if( j > 104){
			dcs_log(0,0, "解析第三磁道错误");
			return -4;
		}	
		else 
		{
			memset(tmp_track, 0, sizeof(tmp_track));
    		memcpy(tmp_track, data_buf+48, j);
    		#ifdef __TEST__
    			dcs_log(0,0, "set track3=%s,j=%d", tmp_track, j);
    		#endif
			setbit(siso,36,(unsigned char *)tmp_track,j);
		}*/
		if(j==0 || j >104)
		{
			dcs_log(0,0, "解析第三磁道错误");
			return -4;
		}
		memset(tmp_track, 0, sizeof(tmp_track));
    	memcpy(tmp_track, data_buf+48, j);
    	#ifdef __TEST__
    		dcs_log(0,0, "set track3=%s,j=%d", tmp_track, j);
    	#endif
		setbit(siso,36,(unsigned char *)tmp_track,j);
	}
	
	#ifdef __TEST__
		memset(tmpbuf, 0, sizeof(tmpbuf));
		s = getbit(siso,35,(unsigned char *)tmpbuf);
		dcs_log(0,0, "track2=[%s],s=%d", tmpbuf,s);
		
		memset(tmpbuf, 0, sizeof(tmpbuf));
		s = getbit(siso,36,(unsigned char *)tmpbuf);
		dcs_log(0,0, "track3=[%s],s=%d", tmpbuf,s);
	#endif
	
	return 0;
}

/*
	功能：解密磁道和pin转加密
	输入参数：pikRandom,enc_pin,secuinfo
	
	输出参数：termbuf,siso
*/
int DecryptPin(char *pikRandom, char *enc_pin, char *pan, TERMMSGSTRUCT *termbuf, char *outpin, SECUINFO secuinfo)
{
	int	keyIndex,s;
	char psam[16+1],bcd_pin[8+1];
	//char out_pin[16+1];
	
	memset(psam, 0, sizeof(psam));
	//memset(out_pin, 0, sizeof(out_pin));
	memset(bcd_pin, 0, sizeof(bcd_pin));
	
	memcpy(psam, secuinfo.sam_card, 16);
	
	#ifdef __LOG__
		dcs_log(0,0, "pikRandom=%s",pikRandom);
//		dcs_log(0,0, "pan=%s",pan);
		dcs_log(0,0, "secuinfo.pinkey_idx=%d",secuinfo.pinkey_idx);
		dcs_log(0,0, "secuinfo.pinkey=%s",secuinfo.pinkey);
		dcs_log(0,0, "secuinfo.pin_convflag=%d",secuinfo.pin_convflag);
	#endif
	
	#ifdef __TEST__
		dcs_log(0,0, "enc_pin=%s",enc_pin);
	#endif
	
	/*
		音频刷卡设备
	*/
	if( memcmp(secuinfo.sam_model,"1",1) == 0)
	{
		keyIndex = secuinfo.sam_pinkey_idx;
		asc_to_bcd( (unsigned char*) bcd_pin, (unsigned char*) enc_pin, getstrlen(enc_pin), 0 );
		s = EPOS_PinConvert(keyIndex, psam, pikRandom, bcd_pin, pan, secuinfo.pinkey_idx, secuinfo.pinkey, secuinfo.pin_convflag, bcd_pin);
		if(s < 0)
		{
			dcs_log(0,0, "加密机错误,s=%d",s);
			return -1;
		}
		
		//bcd_to_asc((unsigned char *) out_pin, (unsigned char*) bcd_pin, 16, 0);
	}
	else if( memcmp(secuinfo.sam_model, "2", 1) == 0)
	{
		keyIndex = secuinfo.sam_pinkey_idx;
		asc_to_bcd( (unsigned char*) bcd_pin, (unsigned char*) enc_pin, getstrlen(enc_pin), 0 );
		s = EPOS_PinConvert(keyIndex, psam, pikRandom, bcd_pin, pan, secuinfo.pinkey_idx, secuinfo.pinkey, secuinfo.pin_convflag, bcd_pin);
		if(s < 0)
		{
			dcs_log(0,0, "加密机错误,s=%d",s);
			return -1;
		}
		
		//bcd_to_asc((unsigned char *) out_pin, (unsigned char*) bcd_pin, 16, 0);
	}
	/*车载USB POS*/
	//else if( memcmp(secuinfo.sam_model, "3", 1) == 0)
	//{
	//	keyIndex = secuinfo.sam_pinkey_idx;
	//	asc_to_bcd( (unsigned char*) bcd_pin, (unsigned char*) enc_pin, getstrlen(enc_pin), 0 );
	//	s = EPOS_PinConvert(keyIndex, psam, pikRandom, bcd_pin, pan, secuinfo.pinkey_idx, secuinfo.pinkey, secuinfo.pin_convflag, bcd_pin);
	//	if(s < 0)
	//	{
	//		dcs_log(0,0, "加密机错误,s=%d",s);
	//		return -1;
	//	}
	//	
	//	bcd_to_asc((unsigned char *) out_pin, (unsigned char*) bcd_pin, 16, 0);
	//}
	else
	{
		dcs_log(0,0, "设备属性错误，返回-3.");
		return -2;
	}
	#ifdef __TEST__
	 	dcs_debug(bcd_pin , 8, "Psam卡终端，Pin转加密：");
	#endif
	
	memcpy(outpin, bcd_pin, 8);
	
	return 0;
}

/*
	功能介绍：通过foldId得到folder的名称

	输入参数：fid
	
	输出参数:channel
	
*/
int TermChannels(int fid, char *channel)
{
	char folderName[64];
	int s = 0;
	
	memset(folderName,0,sizeof(folderName));
	s = fold_get_name(fid,folderName,64);
	if(s < 0)
	{
		dcs_log(0,0,"fold get error,use default!");
		memcpy(folderName, "TM01", 4);
	}
	
	memcpy(channel, folderName, getstrlen(folderName));
	
	return 0;
}

/*发送数据到核心系统*/
int WriteXpe( ISO_data iso)
{
	int gs_comfid = -1, s;
	char buffer1[1024], buffer2[1024];
	char tmpbuf[100];
	struct Timer_struct timerCtrl_str;

    memset(tmpbuf, 0, sizeof(tmpbuf));
    
    gs_comfid = fold_locate_folder("PEX_COM");

    if( gs_comfid < 0)
    {
        dcs_log(0,0,"locate folder pecomm failed.");
        return -1;
    }

    if(fold_get_maxmsg(gs_comfid) < 2)
		fold_set_maxmsg(gs_comfid, 20);
		
	memset(buffer1, 0, sizeof(buffer1));
	memset(buffer2, 0, sizeof(buffer2));
	memset(tmpbuf, 0, sizeof(tmpbuf));
	
	//if(IsoLoad8583config(&iso8583_conf[0], "iso8583.conf") < 0)
	{
		//dcs_log(0,0,"load iso8583_pos failed:%s", strerror(errno));
		//return -1;
	}
	memcpy(iso8583_conf,iso8583conf,sizeof(iso8583conf));
	iso8583 = &iso8583_conf[0];
  	SetIsoHeardFlag(0);
    SetIsoFieldLengthFlag(1);
	
	
	s = iso_to_str((unsigned char *)buffer1, &iso, 1);
	
	bcd_to_asc((unsigned char *)tmpbuf, (unsigned char *)buffer1, 20 ,0);
	
	memcpy(buffer2, tmpbuf, 20);
	memcpy(buffer2+20, buffer1+10, s-10);
	
	s = s+10;
	#ifdef __TEST__
		dcs_debug(buffer2, s, "发送数据到核心系统， Folder id =%d, len =%d", gs_comfid, s);
	#else
		dcs_log(0, 0, "发送数据到核心系统");
	#endif
	fold_write(gs_comfid, -1, buffer2, s);

    //测试超时控制
    if(strlen(g_key)!=0)
    {
        char service[2+1];
        memset(service, 0, sizeof(service));
        if (getbit(&iso, 25, (unsigned char *)service)<0)
        {
            dcs_log(0, 0, "can not get bit_25!");
            return -1;
        }
        /*脱机消费交易上送核心*/
        if(memcmp(service, "50", 2)== 0)
        {
            //添加超时控制
            memset(buffer1, 0, sizeof(buffer1));
            memset(&timerCtrl_str, 0, sizeof(timerCtrl_str));
            memcpy(timerCtrl_str.data,buffer2,s);
            timerCtrl_str.length = s;
            timerCtrl_str.foldid = (char )gs_comfid;
            timerCtrl_str.sendnum =3;
            timerCtrl_str.timeout = 80;
            sprintf(timerCtrl_str.key, g_key,sizeof(g_key));

            tm_insert_entry(timerCtrl_str);
            return 0;
        }
    }
	return 0;
}

/*
	产生二次分散的工作密钥，用于EWP向前置获取交易的PIK
*/
int PsamPikGen(char *psam, char *workey)
{
	SECUINFO sec_info;
	int s = 0;
	char tmpkey[16+1], tmp[50], random[16+1], tmp2[16+1];
	
	memset(&sec_info, 0, sizeof(SECUINFO));
	memset(tmpkey, 0, sizeof(tmpkey));
	memset(tmp, 0, sizeof(tmp));
	memset(random, 0, sizeof(random));
	
	s = GetSecuInfo(psam, &sec_info);
	if(s == -1)
	{
		dcs_log(0,0,"获取psam卡号信息失败");
		memcpy(workey, "0810", 4);
		memcpy(workey+4, psam, 16);
		memcpy(workey+20, "99", 2);
		memcpy(workey+22, "000000000000000000000000000000000000000000000000", 48);
	}
	else
	{
		GenrateKey(random);
		s = EPOS_KeyGen(sec_info.sam_pinkey_idx, psam, random, tmpkey);
		if(s != 0)
		{
			dcs_log(0,0,"产生工作密钥失败");
			memcpy(workey, "0810", 4);
			memcpy(workey+4, psam, 16);
			memcpy(workey+20, "99", 2);
			memcpy(workey+22, "000000000000000000000000000000000000000000000000", 48);
		}
		else
		{
			memset(tmp2, 0, sizeof(tmp2));
			asc_to_bcd((unsigned char *)tmp2, (unsigned char *)psam, 16, 0);
			asc_to_bcd((unsigned char *)tmp2+8, (unsigned char *)random, 16, 0);
			
			#ifdef __TEST__
				dcs_debug(tmpkey, 16, "tmpkey 异或之前的结果：");
			#endif
			
			for(s = 0; s<16; s++)
			{
				tmpkey[s] = tmpkey[s]^tmp2[s];
			}
			
			#ifdef __TEST__
				dcs_debug(tmpkey, 16, "tmpkey 异或之后的结果：");
			#endif
			
			bcd_to_asc((unsigned char *)tmp, (unsigned char *)tmpkey, 32 ,0);
			memcpy(workey, "0810", 4);
			memcpy(workey+4, psam, 16);
			memcpy(workey+20, "00", 2);
			memcpy(workey+22, random, 16);
			memcpy(workey+38, tmp, 32);
		}
	}
	
	#ifdef __LOG__
		dcs_log(0,0,"PsamPikGen, workey=%s", workey);
	#endif
	
	return 70;
}

/*
	POS终端发起的签到交易，签到成功返回1，否则返回-1（签到类型错误）或者-2（数据库操作失败）
*/
int PosGenKey(MER_INFO_T *mer_info, MER_SIGNIN_INFO *mer_signin_info)
{
	int s;
	char mak[40+1], pik[40+1], tdk[40+1], tmp1[100], tmp2[100], tmp3[100];
	
	memset(mak, 0, sizeof(mak));
	memset(pik, 0, sizeof(pik));
	memset(tdk, 0, sizeof(tdk));
	
	//pik,mak都是单倍长
	if(memcmp(mer_info->mgr_des, "001", 3) == 0)
	{
		s = POS_WorkKeyGen(1, "pik", mer_info->mgr_kek, pik);
		if(s < 0 )
		{
			dcs_log(0, 0, "获取单倍长PIK失败");
			return -1;
		}
		s = POS_WorkKeyGen(1, "mak", mer_info->mgr_kek, mak);
		if(s < 0 )
		{
			dcs_log(0, 0, "获取单倍长MAK失败");
			return -1;
		}
		memset(tmp1, 0, sizeof(tmp1));
		memset(tmp2, 0, sizeof(tmp2));
		bcd_to_asc((unsigned char *)tmp1, (unsigned char *)pik, 48 ,0);
		bcd_to_asc((unsigned char *)tmp2, (unsigned char *)mak, 48 ,0);
		#ifdef __LOG__
			dcs_log(0,0,"tmp1=%s", tmp1);
			dcs_log(0,0,"tmp2=%s", tmp2);
		#endif
		memset(mer_info->mgr_pik, 0, sizeof(mer_info->mgr_pik));
		memcpy(mer_info->mgr_pik, tmp1, 16);
		memcpy(mer_signin_info->mgr_des, "001", 3);
		memcpy(mer_signin_info->mgr_pik, tmp1+16, 16);
		memcpy(mer_signin_info->mgr_pik_checkvalue, tmp1+32, 8);

		memset(mer_info->mgr_mak, 0, sizeof(mer_info->mgr_mak));
		memset(mer_info->mgr_tdk, 0, sizeof(mer_info->mgr_tdk));
		memcpy(mer_info->mgr_mak, tmp2, 16);
		memcpy(mer_signin_info->mgr_mak, tmp2+16, 16);
		memcpy(mer_signin_info->mgr_mak_checkvalue, tmp2+32, 8);
	}
	//pik双倍长,mak单倍长,用0补全双倍长
	else if(memcmp(mer_info->mgr_des, "003", 3) == 0)
	{
		s = POS_WorkKeyGen(2, "pik", mer_info->mgr_kek, pik);
		if(s < 0 )
		{
			dcs_log(0, 0, "获取双倍长PIK失败");
			return -1;
		}
		s = POS_WorkKeyGen(1, "mak", mer_info->mgr_kek, mak);
		if(s < 0 )
		{
			dcs_log(0, 0, "获取单倍长MAK失败");
			return -1;
		}
		memset(tmp1, 0, sizeof(tmp1));
		memset(tmp2, 0, sizeof(tmp2));
		bcd_to_asc((unsigned char *)tmp1, (unsigned char *)pik, 80 ,0);
		bcd_to_asc((unsigned char *)tmp2, (unsigned char *)mak, 48 ,0);
		#ifdef __LOG__
			dcs_log(0,0,"tmp1=%s", tmp1);
			dcs_log(0,0,"tmp2=%s", tmp2);
		#endif
		memset(mer_info->mgr_pik, 0, sizeof(mer_info->mgr_pik));
		memcpy(mer_info->mgr_pik, tmp1, 32);
		memcpy(mer_signin_info->mgr_des, "001", 3);
		memcpy(mer_signin_info->mgr_pik, tmp1+32, 32);
		memcpy(mer_signin_info->mgr_pik_checkvalue, tmp1+64, 8);
		
		memset(mer_info->mgr_mak, 0, sizeof(mer_info->mgr_mak));
		memset(mer_info->mgr_tdk, 0, sizeof(mer_info->mgr_tdk));
		memcpy(mer_info->mgr_mak, tmp2, 16);
		memcpy(mer_signin_info->mgr_mak, tmp2+16, 16);
		memcpy(mer_signin_info->mgr_mak+16, "0000000000000000", 16);
		memcpy(mer_signin_info->mgr_mak_checkvalue, tmp2+32, 8);
	}
	//pik双倍长,mak单倍长,tdk双倍长
	else if(memcmp(mer_info->mgr_des, "004", 3) == 0)
	{
		s = POS_WorkKeyGen(2, "pik", mer_info->mgr_kek, pik);
		if(s < 0 )
		{
			dcs_log(0, 0, "获取双倍长PIK2失败");
			return -1;
		}
		s = POS_WorkKeyGen(1, "mak", mer_info->mgr_kek, mak);
		if(s < 0 )
		{
			dcs_log(0, 0, "获取单倍长MAK2失败");
			return -1;
		}
		s = POS_WorkKeyGen(2, "tdk", mer_info->mgr_kek, tdk);
		if(s < 0 )
		{
			dcs_log(0, 0, "获取双倍长tdk失败");
			return -1;
		}
		memset(tmp1, 0, sizeof(tmp1));
		memset(tmp2, 0, sizeof(tmp2));
		memset(tmp3, 0, sizeof(tmp3));
		bcd_to_asc((unsigned char *)tmp1, (unsigned char *)pik, 80 ,0);
		bcd_to_asc((unsigned char *)tmp2, (unsigned char *)mak, 48 ,0);
		bcd_to_asc((unsigned char *)tmp3, (unsigned char *)tdk, 80 ,0);
		#ifdef __LOG__
			dcs_log(0,0,"tmp1=%s", tmp1);
			dcs_log(0,0,"tmp2=%s", tmp2);
			dcs_log(0,0,"tmp3=%s", tmp3);
		#endif
		memset(mer_info->mgr_pik, 0, sizeof(mer_info->mgr_pik));
		memcpy(mer_info->mgr_pik, tmp1, 32);
		memcpy(mer_signin_info->mgr_des, "001", 3);
		memcpy(mer_signin_info->mgr_pik, tmp1+32, 32);
		memcpy(mer_signin_info->mgr_pik_checkvalue, tmp1+64, 8);

		memset(mer_info->mgr_mak, 0, sizeof(mer_info->mgr_mak));
		memcpy(mer_info->mgr_mak, tmp2, 16);
		memcpy(mer_signin_info->mgr_mak, tmp2+16, 16);
		memcpy(mer_signin_info->mgr_mak+16, "0000000000000000", 16);
		memcpy(mer_signin_info->mgr_mak_checkvalue, tmp2+32, 8);

		memset(mer_info->mgr_tdk, 0, sizeof(mer_info->mgr_tdk));
		memcpy(mer_info->mgr_tdk, tmp3, 32);
		memcpy(mer_signin_info->mgr_tdk, tmp3+32, 32);
		memcpy(mer_signin_info->mgr_tdk_checkvalue, tmp3+64, 8);
		
	}else{
		return -1;
	}
	
	s = updateMerWorkey(mer_info);
	if(s < 0)
		return -2;
	else
	{
		#ifdef __LOG__
			dcs_log(0,0,"pos pik=%s,checkvalue=%s", mer_signin_info->mgr_pik,mer_signin_info->mgr_pik_checkvalue);
			dcs_log(0,0,"pos mak=%s,checkvalue=%s", mer_signin_info->mgr_mak,mer_signin_info->mgr_mak_checkvalue);
			dcs_log(0,0,"pos tdk=%s,checkvalue=%s", mer_signin_info->mgr_tdk,mer_signin_info->mgr_tdk_checkvalue);
			
			dcs_log(0,0,"xpep pik=%s", mer_info->mgr_pik);
			dcs_log(0,0,"xpep mak=%s", mer_info->mgr_mak);
			dcs_log(0,0,"xpep tdk=%s", mer_info->mgr_tdk);
		#endif
		return 1;
	}
}

/*
	POS pin转加密

	成功返回1，否则返回-1
	pik1：终端密钥mgr_pik
	pik2：前置系统密钥
	card：转出卡
	pinBlock：pin密文，16字节
	
	outPin：返回加密之后的pin，8字节
*/
int PosPinConvert(char *pik1, char *pik2, char *card, char *pinBlock, char *outPin)
{
	int s;
	
	s = POS_PinConvert(pik1, 1, pik2, 6, card, pinBlock, outPin);
	if(s == 0)
	{
		#ifdef __TEST__
			dcs_debug(outPin, 8 ,"加密机返回Pin密文：");
		#endif
		return 1;
	}else{
		dcs_log(0,0,"pin转加密异常,s=%d", s);
		return -1;
	}
}


int PosTrackDecrypt(MER_INFO_T mer_info, ISO_data *iso)
{
	char track2[38],track3[105],tmp[17],rtndata[9];
	int s,len;
	
	memset(track2, 0, sizeof(track2));
	memset(track3, 0, sizeof(track3));
	memset(tmp, 0, sizeof(tmp));
	memset(rtndata, 0, sizeof(rtndata));
	
#ifdef __LOG__
	dcs_log(0, 0, "磁道解密函数");
#endif
	len = getbit(iso, 35, (unsigned char *)track2);
	if(len <= 0){
		dcs_log(0, 0, "二磁数据不存在，返回-2.");
		return -2;
	}
	else{
		if(len % 2 == 0)
			memcpy(tmp, track2+len-18, 16);
		else
			memcpy(tmp, track2+len-17, 16);
		#ifdef __TEST__	
		 	dcs_log(0,0,"终端上送二磁 track2 :%s", track2);
		 	dcs_log(0,0,"终端上送二磁密文部分 data:%s", tmp);
		 #endif
	}
	
	if(strlen(mer_info.mgr_tdk) != 32){
		dcs_log(0, 0, "平台端磁道密钥长度不是32字节，实际长度=%d, 返回-3.", strlen(mer_info.mgr_tdk));
		return -3;
	}
		
	s = POS_TrackDecrypt(mer_info.mgr_tdk, tmp, rtndata);
	if(s == 0)
	{
		memset(tmp, 0, sizeof(tmp));
		bcd_to_asc((unsigned char *)tmp, (unsigned char *)rtndata, 16 ,0);
		#ifdef __TEST__
			dcs_log(0,0,"二磁密文部分解密之后的明文数据 data:%s", tmp);
		#endif
		for(s = 0;s<17;s++){
			if(tmp[s] == 'D')
				tmp[s] = '=';
		}
			
		if(len % 2 == 0)
			memcpy(track2+len-18, tmp, 16);
		else
			memcpy(track2+len-17, tmp, 16);
		#ifdef __TEST__	
			dcs_log(0,0,"最终明文二磁数据 track2 :%s, len:%d", track2, len);
		#endif
		
		setbit(iso, 35, (unsigned char *)track2, len);
		
	}else{
		dcs_log(0,0,"二磁道解密异常,s=%d", s);
		return -1;
	}
	
	len = getbit(iso, 36, (unsigned char *)track3);
	if(len <= 0){
#ifdef __LOG__
		dcs_log(0, 0, "不存在三磁");
#endif
		return 0;
	}
	else{
		memset(tmp, 0, sizeof(tmp));
		
		if(len % 2 == 0)
			memcpy(tmp, track3+len-18, 16);
		else
			memcpy(tmp, track3+len-17, 16);
		
		#ifdef __TEST__		
			dcs_log(0,0,"终端上送三磁 track3 :%s", track3);
			dcs_log(0,0,"终端上送三磁密文部分 data:%s", tmp);
		#endif
	}
	
	memset(rtndata, 0, sizeof(rtndata));
	s = POS_TrackDecrypt(mer_info.mgr_tdk, tmp, rtndata);
	if(s == 0)
	{
		memset(tmp, 0, sizeof(tmp));
		bcd_to_asc((unsigned char *)tmp, (unsigned char *)rtndata, 16 ,0);
		#ifdef __TEST__
			dcs_log(0,0,"三磁密文部分解密之后的明文数据 data:%s", tmp);
		#endif
		
		for(s = 0;s<17;s++){
			if(tmp[s] == 'D')
				tmp[s] = '=';
		}
		
		if(len % 2 == 0)
			memcpy(track3+len-18, tmp, 16);
		else
			memcpy(track3+len-17, tmp, 16);
		
		#ifdef __TEST__	
			dcs_log(0,0,"最终明文三磁数据 track3 :%s, len:%d", track3, len);
		#endif
		setbit(iso, 36, (unsigned char *)track3, len);
		
	}else{
		dcs_log(0,0,"三磁道解密异常, s=%d", s);
		return -1;
	}
	return 0;
}

/*
	SMSBody to Byte[],
	return length
	
	@author he.qingqi@chinaebi.com
*/
int SmsBody2Byte(SMSBody smsbody, char *smsdata){
	int tt=0;
	
	if(strlen(smsbody.trancde) == 0)
		return -1;
	if(strlen(smsbody.userphone) == 0)
		return -2;
		
    memcpy(smsdata+tt, smsbody.trancde, strlen(smsbody.trancde));
    tt += strlen(smsbody.trancde);
    memcpy(smsdata+tt, "|", 1);
    tt += 1;

#ifdef __LOG__
    dcs_log(0,0,"trancde:%s%d",smsbody.trancde,tt);
#endif
    
    if(strlen(smsbody.outcard) == 0)
    {
    	memcpy(smsdata+tt, " ", 1);
    	tt += 1;
    }
    else
    {
    	memcpy(smsdata+tt, smsbody.outcard, strlen(smsbody.outcard));
    	tt += strlen(smsbody.outcard);
    }
    memcpy(smsdata+tt, "|", 1);
    tt += 1;
#ifdef __TEST__
    dcs_log(0,0,"outcard:%s%d",smsbody.outcard,tt);
#endif
    
    if(strlen(smsbody.incard) == 0)
    {
    	memcpy(smsdata+tt, " ", 1);
    	tt += 1;
    }
    else
    {
	    memcpy(smsdata+tt, smsbody.incard, strlen(smsbody.incard));
	    tt += strlen(smsbody.incard);
    }
    memcpy(smsdata+tt, "|", 1);
    tt += 1;
#ifdef __TEST__
    dcs_log(0,0,"incard:%s%d",smsbody.incard,tt);
#endif
    
    if(strlen(smsbody.tradeamt) == 0)
    {
    	memcpy(smsdata+tt, " ", 1);
    	tt += 1;
    }
    else
    {
	    memcpy(smsdata+tt, smsbody.tradeamt, strlen(smsbody.tradeamt));
	    tt += strlen(smsbody.tradeamt);
    }
    memcpy(smsdata+tt, "|", 1);
    tt += 1;
#ifdef __LOG__
    dcs_log(0,0,"tradeamt:%s%d",smsbody.tradeamt,tt);
#endif
    
	if(strlen(smsbody.tradefee) == 0)
    {
    	memcpy(smsdata+tt, " ", 1);
    	tt += 1;
    }
    else
    {
		 memcpy(smsdata+tt, smsbody.tradefee, strlen(smsbody.tradefee));
   		 tt += strlen(smsbody.tradefee);
   	}
    memcpy(smsdata+tt, "|", 1);
    tt += 1;
#ifdef __LOG__
    dcs_log(0,0,"tradefee:%s%d",smsbody.tradefee,tt);
#endif
    
    if(strlen(smsbody.tradedate) == 0)
    {
    	memcpy(smsdata+tt, " ", 1);
    	tt += 1;
    }
    else
    {
    	memcpy(smsdata+tt, smsbody.tradedate, strlen(smsbody.tradedate));
   		tt += strlen(smsbody.tradedate);
    }
    memcpy(smsdata+tt, "|", 1);
    tt += 1;

#ifdef __LOG__
    dcs_log(0,0,"tradedate:%s%d",smsbody.tradedate,tt);
#endif
    
    if(strlen(smsbody.traderesp) == 0)
    {
    	memcpy(smsdata+tt, " ", 1);
    	tt += 1;
    }
    else
    {
    	memcpy(smsdata+tt, smsbody.traderesp, strlen(smsbody.traderesp));
    	tt += strlen(smsbody.traderesp);
    }
    memcpy(smsdata+tt, "|", 1);
    tt += 1;

#ifdef __LOG__
    dcs_log(0,0,"traderesp:%s%d",smsbody.traderesp,tt);
#endif
    
    if(strlen(smsbody.tradetrace) == 0)
    {
    	memcpy(smsdata+tt, " ", 1);
    	tt += 1;
    }
    else
    {
    	memcpy(smsdata+tt, smsbody.tradetrace, strlen(smsbody.tradetrace));
    	tt += strlen(smsbody.tradetrace);
    }
    memcpy(smsdata+tt, "|", 1);
    tt += 1;

#ifdef __LOG__
    dcs_log(0,0,"tradetrace:%s%d",smsbody.tradetrace,tt);
#endif
    
    if(strlen(smsbody.traderefer) == 0)
    {
    	memcpy(smsdata+tt, " ", 1);
    	tt += 1;
    }
    else
    {
    	memcpy(smsdata+tt, smsbody.traderefer, strlen(smsbody.traderefer));
    	tt += strlen(smsbody.traderefer);
    }
    memcpy(smsdata+tt, "|", 1);
    tt += 1;

#ifdef __LOG__
    dcs_log(0,0,"traderefer:%s%d",smsbody.traderefer,tt);
#endif
    
    if(strlen(smsbody.tradebillmsg) == 0)
    {
    	memcpy(smsdata+tt, " ", 1);
    	tt += 1;
    }
    else
    {
   	 	memcpy(smsdata+tt, smsbody.tradebillmsg, strlen(smsbody.tradebillmsg));
    	tt += strlen(smsbody.tradebillmsg);
    }
    memcpy(smsdata+tt, "|", 1);
    tt += 1;

#ifdef __LOG__
    dcs_log(0,0,"tradebillmsg:%s%d",smsbody.tradebillmsg,tt);
#endif
    memcpy(smsdata+tt, smsbody.userphone, strlen(smsbody.userphone));
    tt += strlen(smsbody.userphone);
    
#ifdef __LOG__
    dcs_log(0,0,"userphone:%s%d",smsbody.userphone,tt);
#endif
    return tt;
}

/*
	pos报文解密,返回数据放入rtnData中,返回数据长度dataLen
*/
int POSPackageDecrypt(MER_INFO_T mer_info, char *data, int dataLen, char *rtnData)
{
	int s;
	
	if(strlen(mer_info.mgr_tdk) != 32){
		dcs_log(0, 0, "平台端磁道密钥长度不是32字节，实际长度=%d, 返回-1.", strlen(mer_info.mgr_tdk));
		return -1;
	}
	
	s = POS_PackageDecrypt(mer_info.mgr_tdk, data, dataLen, rtnData);
	if(s != 0)
	{
		dcs_log(0, 0, "加密机解密pos报文错误。");
		return -2;
	}
	
	return 1;
}

/*
	pos报文加密，返回补全8的倍数之后的实际报文长度
	
*/
int POSPackageEncrypt(MER_INFO_T mer_info, char *data, int dataLen, char *rtnData)
{
	int s;
	
	if(strlen(mer_info.mgr_tdk) != 32){
		dcs_log(0, 0, "平台端磁道密钥长度不是32字节，实际长度=%d, 返回-1.", strlen(mer_info.mgr_tdk));
		return -1;
	}
	
	s = POS_PackageEncrypt(mer_info.mgr_tdk, data, dataLen, rtnData);
	if(s != 0)
	{
		dcs_log(0, 0, "加密机加密pos报文错误。");
		return -2;
	}
	
	s = dataLen % 8; 
	if(s == 0)
		return dataLen;
	else
		return dataLen + s;
}

int ewphbmac_chack(SECUINFO secuinfo, EWP_INFO ewp_info)
{
	int result,len=0;
	char data[512], rtnData[5],tmp[10];
	
	memset(rtnData, 0, sizeof(rtnData));
	memset(data, 0, sizeof(data));
	//Psam卡号，机具编号，用户名，订单号，交易金额，磁道，磁道密钥随机数，密码，密码密钥随机数
	memcpy(data+len, ewp_info.consumer_psamno, 16);
	len += 16;
	
	memcpy(data+len, ewp_info.consumer_phonesnno, strlen(ewp_info.consumer_phonesnno));
	len += strlen(ewp_info.consumer_phonesnno);
	
	memcpy(data+len, ewp_info.consumer_username, strlen(ewp_info.consumer_username));
	len += strlen(ewp_info.consumer_username);
	
	memcpy(data+len, ewp_info.consumer_orderno, strlen(ewp_info.consumer_orderno));
	len += strlen(ewp_info.consumer_orderno);
	
	if(memcmp(ewp_info.consumer_money, "000000000000", 12) != 0)
	{
		memcpy(data+len, ewp_info.consumer_money, strlen(ewp_info.consumer_money));
		len += strlen(ewp_info.consumer_money);
	}
	
	memcpy(data+len, ewp_info.consumer_trackno, strlen(ewp_info.consumer_trackno));
	len += strlen(ewp_info.consumer_trackno);
	
	memcpy(data+len, ewp_info.consumer_track_randnum, strlen(ewp_info.consumer_track_randnum));
	len += strlen(ewp_info.consumer_track_randnum);
	
	memcpy(data+len, ewp_info.consumer_password, strlen(ewp_info.consumer_password));
	len += strlen(ewp_info.consumer_password);
	
	memcpy(data+len, ewp_info.consumer_pin_randnum, strlen(ewp_info.consumer_pin_randnum));
	len += strlen(ewp_info.consumer_pin_randnum);

#ifdef __LOG__
	dcs_log(0, 0, "hb_mac keyIndex=%d, psam=%s, data=[%s], len=%d", secuinfo.sam_mackey_idx, secuinfo.sam_card, data, len);
#endif
	result = epos_hbmac(secuinfo.sam_mackey_idx, secuinfo.sam_card, data, len, rtnData);
	
	if(result == 0)
	{
		memset(tmp, 0, sizeof(tmp));
		bcd_to_asc((unsigned char *)tmp, (unsigned char *)rtnData, 8 ,0);
#ifdef __LOG__
		dcs_log(0, 0, "pos_hb_mac:%s, posp_hb_mac:%s", ewp_info.macinfo, tmp);
#endif
		
		if( memcmp(ewp_info.macinfo, tmp, 8)==0)
			return 0;
		else
			return -1;
	}else
		return -1;

}

/*
	产生二次分散的工作密钥，被过程密钥加密
*/
int CustKeyGen(char *randomData, char *xpepRandom, char *workey)
{
	int s = 0;
	char tmpkey[16+1], tmp[50], _xpepRandom[8+1];
	
	memset(tmpkey, 0, sizeof(tmpkey));
	memset(tmp, 0, sizeof(tmp));
	memset(_xpepRandom, 0, sizeof(_xpepRandom));
	
	
	dcs_log(0,0,"CustKeyGen randomData=%s", randomData);
	
	//产生前置的随机数
	GenrateKey8(_xpepRandom);
	
	bcd_to_asc((unsigned char *)xpepRandom, (unsigned char *)_xpepRandom, 16 ,0);
#ifdef __LOG__
	dcs_debug(_xpepRandom, 8 ,"CustKeyGen _xpepRandom ：");
#endif
	dcs_log(0, 0, "CustKeyGen xpepRandom=%s", xpepRandom);
	
	s = EPOS_KeyGen(50, randomData, xpepRandom, tmpkey);
	if(s != 0)
	{
		return -1;
	}
	else
	{
		bcd_to_asc((unsigned char *)workey, (unsigned char *)tmpkey, 32 ,0);
	}
	
	#ifdef __LOG__
		dcs_log(0,0,"Cust workey=%s", workey);
	#endif
	
	return 32;
}

int pub_cust_mac(char *sendCde, char *data, int dataLen, char *macData)
{
	int s;
	char sendRandom[16+1], xpeprandom[16+1];
	char _macData[8], tmp[17];
	
	memset(sendRandom, 0, sizeof(sendRandom));
	memset(xpeprandom, 0, sizeof(xpeprandom));
	memset(_macData, 0, sizeof(_macData));
	memset(tmp, 0, sizeof(tmp));
	
	s = getCustKeyInfo(sendCde, sendRandom, xpeprandom);
	if(s == -1)
	{
		dcs_log(0,0,"获取数据库管理平台密钥信息错误");
		return -1;
	}else
	{
		s = cust_mac(50, sendRandom, xpeprandom, data, dataLen, _macData);
		if(s != 0)
		{
			dcs_log(0,0,"加密机计算mac错误");
			return -2;
		}
		else
		{
			bcd_to_asc((unsigned char *)tmp, (unsigned char *)_macData, 16 ,0);
			memcpy(macData, tmp, 8);
			dcs_log(0,0,"pub_cust_mac =[%s]", macData);
			return 0;
		}
	}
	
}

char GetDetailFlag(MER_INFO_T  *mer_info_t)
{
	char value_flag ='0';
#ifdef __LOG__
	dcs_log(0, 0, "download_para_flag =[%s]", mer_info_t->download_para_flag);
	dcs_log(0, 0, "download_pubkey_flag =[%s]", mer_info_t->download_pubkey_flag);
#endif

	if(mer_info_t->pos_update_flag ==1 && mer_info_t->pos_update_mode=='9')
		value_flag = mer_info_t->pos_update_mode;
	else if(mer_info_t->para_update_flag=='1'&& mer_info_t->para_update_mode =='6')
		value_flag = mer_info_t->para_update_mode;
	else if(mer_info_t->pos_update_flag ==1)
		value_flag = mer_info_t->pos_update_mode;
	else if(mer_info_t->para_update_flag == '1')
		value_flag = mer_info_t->para_update_mode;
	else if(memcmp(mer_info_t->download_pubkey_flag, "0", 1)==0)
		value_flag = 'E';
	else if(memcmp(mer_info_t->download_para_flag, "0", 1)==0)
		value_flag = 'F';
	else	
		value_flag ='0';
		
	return value_flag;
}

/*
	srcbuf:用来存放源字符串
	len:源字符串的长度
	desbuf:用来存放目的字符串
	flag：右边补空格还是左边补0 的标记  1：为右边补空格 0：为左边补0
	去掉字符串右边的空格或是金额左边的0  add by lp 3.9
*/
int pub_rtrim_lp(char *srcbuf, int len,char* desbuf,int flag)
{
	int i=0;
	int cout=0;
	char *ptr=srcbuf;
	if(flag==1)/*左边对齐，右边补空格*/
	{	
		for(i=0;i<len;i++)
		{
			if(srcbuf[i]==' ')
				ptr++;
			else
				break;
		}
		srcbuf=ptr;
		len=strlen(srcbuf);
		for(i=len;i> 0;i--)
		{
			if(srcbuf[i-1]==' ')
			{
				cout++;
			}
			else
				break;
		}
		strncpy(desbuf,srcbuf,len-cout);
		desbuf[len-cout]='\0';
		//printf( " %s%d\n ",desbuf,strlen(desbuf));
	}
	else if(flag==0)/*右边对齐 左边补0*/
	{
		for(i=0;i<len;i++)
		{
			if(srcbuf[i]=='0')
				ptr++;
			else
				break;	
		}
		strcpy(desbuf,ptr);
		//printf(" money=%s,len=%d",desbuf,strlen(desbuf));
	}
	return 0;
}

/*
	pszString:用来存放字符串
	nLen:补充字符串后的长度
	ch:填充的字符
	nOption：1：为右边填充字符ch   0：为左边填充字符ch
*/
int PubAddSymbolToStr(char *pszString, int nLen, char ch, int nOption)
{
	int i=0,len;
	char szBuf[1024+1];
	memset(szBuf,0x00,sizeof(szBuf));
	len =strlen(pszString);
	memcpy(szBuf,pszString,len);

	if(nOption == 1)
	{
		for(i=0;i<nLen -len;i++)
			pszString[len+i] = ch;
		pszString[len+i]='\0';
	}
	else if(nOption == 0)
	{
		for(i=0;i<nLen -len;i++)
			pszString[i] = ch;
		strcpy(pszString+i,szBuf);
	}
	else
	{
		return -1;
	}
	return 0;
}

/*
  55域解析
*/
int analyse55(char *szTmp, int iLen, fd55 *stFd55, int Len)
{
	long lRet = -1;

	char szCountLen[10];
	char szTag[10];
	char tmp_buf[2048];
	int iMark = 0;
	int i = 0;
	int s = 0;
	long len=0;
	int flag =0;
	
	char *szInData = NULL;
	memset(tmp_buf, 0, sizeof(tmp_buf));
	bcd_to_asc((unsigned char *)tmp_buf, (unsigned char *)szTmp, 2*iLen, 0);
	
	/*装载到数组缓存中*/
	szInData = &tmp_buf[0];
	/*解码*/
	#ifdef __TEST__
		dcs_log(0, 0, "55域tmp_buf = [%s]\n", tmp_buf);
	#endif
	while(*szInData != 0)
	{
		memset(szCountLen, 0, sizeof(szCountLen));
		memcpy(szCountLen, szInData, 2);
		iMark = strtol(szCountLen, NULL, 16);
		if((iMark&0x1f) == 0x1f)
		{
			memset(szTag,0,sizeof(szTag));
			memcpy(szTag, szInData, 4);
			szInData += 4; 
			flag = 1;
			#ifdef __TEST__
				dcs_log(0, 0, "4位TAG%s\n", szTag);
			#endif
		}
		else
		{
			memset(szTag,0,sizeof(szTag));
			memcpy(szTag, szInData, 2);
			szInData += 2;
			flag = 2;
			#ifdef __TEST__
				dcs_log(0,0,"2位TAG %s\n",szTag);
			#endif
		}
		for(i=0; i<Len; i++)
		{
			if((flag == 1  && memcmp(stFd55[i].szTag, szTag,4)==0) || (flag == 2  && memcmp(stFd55[i].szTag, szTag,2)==0))
			{
				memset(szCountLen, 0, sizeof(szCountLen));
				memcpy(szCountLen, szInData, 2);
				memcpy(stFd55[i].szLen, szInData, 2);
				szInData += 2;
				iLen = strtol(szCountLen, NULL, 16);
				memcpy(stFd55[i].szValue, szInData, iLen*2);
				#ifdef __TEST__	
					dcs_log(0,0,"%s = %s, iLen = %s\n", stFd55[i].szTag, stFd55[i].szValue, stFd55[i].szLen);
				#endif
				stFd55[i].iStatus = 1;
				szInData = szInData + iLen*2;
				lRet = 1;
				break;
			}
		}
		if(!lRet)
		{
			dcs_log(0,0,"55域解析错误\n");
			return -1;
		}
		lRet = 0;
	}

	return lRet; 
}

int mposmac_check(EWP_INFO ewp_info, char *mack)
{
	int result, len=0;
	char data[512], rtnData[5], tmp[10];
	
	memset(rtnData, 0, sizeof(rtnData));
	memset(data, 0, sizeof(data));
	
	//TC值上送 和脚本上送通知 计算mac 域为KSN卡号，机具编号，系统参考号
	if(memcmp(ewp_info.consumer_transdealcode, "999997", 6) == 0 || memcmp(ewp_info.consumer_transtype, "0620", 4) == 0)
	{
		memcpy(data+len, ewp_info.consumer_ksn, 20);
		len += 20;
		
		memcpy(data+len, ewp_info.consumer_phonesnno, strlen(ewp_info.consumer_phonesnno));
		len += strlen(ewp_info.consumer_phonesnno);

		memcpy(data+len, ewp_info.consumer_sysreferno, strlen(ewp_info.consumer_sysreferno));
		len += strlen(ewp_info.consumer_sysreferno);
	}
	else
	{
		//KSN卡号，机具编号，用户名，订单号，交易金额，磁道，密码
		memcpy(data+len, ewp_info.consumer_ksn, 20);
		len += 20;
		
		memcpy(data+len, ewp_info.consumer_phonesnno, strlen(ewp_info.consumer_phonesnno));
		len += strlen(ewp_info.consumer_phonesnno);
		
		memcpy(data+len, ewp_info.consumer_username, strlen(ewp_info.consumer_username));
		len += strlen(ewp_info.consumer_username);
		
		memcpy(data+len, ewp_info.consumer_orderno, strlen(ewp_info.consumer_orderno));
		len += strlen(ewp_info.consumer_orderno);
		
		//if(memcmp(ewp_info.consumer_money, "000000000000", 12) != 0)
		{
			memcpy(data+len, ewp_info.consumer_money, strlen(ewp_info.consumer_money));
			len += strlen(ewp_info.consumer_money);
		}
		
		memcpy(data+len, ewp_info.trackno2, strlen(ewp_info.trackno2));
		len += strlen(ewp_info.trackno2);

		memcpy(data+len, ewp_info.trackno3, strlen(ewp_info.trackno3));
		len += strlen(ewp_info.trackno3);
		
		memcpy(data+len, ewp_info.consumer_password, strlen(ewp_info.consumer_password));
		len += strlen(ewp_info.consumer_password);
	}
		
	dcs_log(0, 0, "data=[%s], len=%d", data, len);

	result = mpos_mac(mack, ewp_info.macinfo, data, len, rtnData);
	if(result == 0)
	{
		return 0;
	}else
		return -1;
}

/*
	mpos磁道解密,返回数据放入rtnData中,返回数据长度dataLen
*/
int MPOSTrackDecrypt(char *ksntdk, char *data, int dataLen, char *rtnData)
{
	int s;
	char tdk[32+1];
	char trackdata[160+1];
	char tmpbuf[32+1];
	
	memset(tdk, 0x00, sizeof(tdk));
	memset(trackdata, 0x00, sizeof(trackdata));
	
	asc_to_bcd( (unsigned char *)trackdata, (unsigned char*)data, dataLen, 0 );
#ifdef __TEST__
	dcs_debug(ksntdk, 16, "磁道加密密钥:");
#endif
	bcd_to_asc(tdk, ksntdk, 32, 0);
	//dcs_log(0, 0, "ASC码磁道加密密钥:%s", tdk);
	
	if(strlen(tdk) != 32){
		dcs_log(0, 0, "平台端磁道密钥长度不是32字节，实际长度=%d, 返回-1.", strlen(tdk));
		return -1;
	}

	s = POS_PackageDecrypt(tdk, trackdata, dataLen/2, tmpbuf);
	if(s != 0)
	{
		dcs_log(0, 0, "加密机解密mpos磁道信息错误。");
		return -2;
	}
	
	bcd_to_asc(rtnData, tmpbuf, 16, 0);
	
	return 1;
}


int mposmac_check_yindian(EWP_INFO ewp_info, char *mack)
{
	int result,len=0;
	char data[512], rtnData[5],tmp[10];
	
	memset(rtnData, 0, sizeof(rtnData));
	memset(data, 0, sizeof(data));

	
	memcpy(data, ewp_info.macblk, strlen(ewp_info.macblk));
	len += strlen(ewp_info.macblk);
	
	
	//dcs_log(0, 0, "data=[%s], len=%d", data, len);
	
	result = mpos_mac(mack, ewp_info.macinfo, data, len, rtnData);
	
	if(result == 0)
	{
		memset(tmp, 0, sizeof(tmp));
		bcd_to_asc((unsigned char *)tmp, (unsigned char *)rtnData, 8 ,0);
		
		dcs_log(0, 0, "pos_hb_mac:%s, posp_hb_mac:%s", ewp_info.macinfo, tmp);
		if( strcasecmp(ewp_info.macinfo, tmp)==0)
			return 0;
		else
			return -1;
	}else
		return -1;

}
/*
* 根据上送的ksn，计算当前的dukpt的各个密钥
* 返回0表示成功，-1表示失败
*/
int MakeDukptKey(KSN_INFO *ksn_info)
{
	char ksn_bcd[10+1];
	char initkey[16+1];
	char dukptkey[16+1];
	int s =-1;
	
	//计算DUKPT    dukptkey (此处取值BCD码的initkey，不能使用asc码的ksn_info.initkey)
	memset(ksn_bcd, 0x00, sizeof(ksn_bcd));
	memset(initkey, 0x00, sizeof(initkey));
	memset(dukptkey, 0x00, sizeof(dukptkey));
	asc_to_bcd((unsigned char *)ksn_bcd, (unsigned char*)ksn_info->ksn, 20, 0);
	asc_to_bcd((unsigned char *)initkey, (unsigned char *)ksn_info->initkey, 32,0);

	s = GetDukptKey(ksn_bcd, initkey, dukptkey);
	if(s<0)
	{
		dcs_log(0 , 0, "GetDukptKey失败");
		return -1;
	}
	
	bcd_to_asc(ksn_info->dukptkey, dukptkey, 32, 0);
/*
	//保存 dukptkey 值到 ksn_info 数据表
	s = Savedukptkey(ksn_info);
	if(s<0)
	{
		dcs_log(0 , 0, "Savedukptkey失败");
		return;	
	}
*/	
#ifdef __TEST__
	dcs_log(0, 0, "ksn_info.dukptkey=[%s]", ksn_info->dukptkey);		
#endif
//获取PINK
	getKey(ksn_info->dukptkey,SEC_KEY_TYPE_TPK,ksn_info->tpk);
	
//获取MACK
	getKey(ksn_info->dukptkey,SEC_KEY_TYPE_TAK,ksn_info->tak);	

//获取TDK
	getKey(ksn_info->dukptkey,SEC_KEY_TYPE_TDK,ksn_info->tdk);	
#ifdef __TEST__
	dcs_debug(ksn_info->tpk, 16, "PIN计算密钥：");
	dcs_debug(ksn_info->tak, 16, "MAC计算密钥：");
	dcs_debug(ksn_info->tdk, 16, "磁道计算密钥：");
#endif
	return 0;
}


/*解密2,3磁道
*/
int MPOS_TrackDecrypt(EWP_INFO *ewp_info, KSN_INFO*ksn_info, ISO_data *iso)
{
	char tmp[16+1],rtndata[8+1],bcd_buf[8+1];
	int s = 0,len = 0;
	char tdkey[16+1];
	
	memset(tmp, 0, sizeof(tmp));
	memset(rtndata, 0, sizeof(rtndata));
	memset(tdkey, 0, sizeof(tdkey));
	memset(bcd_buf, 0, sizeof(bcd_buf));

	//tdk 对自己进行3des加密得到真正的tdkey
	 s = D3desbylen(ksn_info->tdk, 16,tdkey, ksn_info->tdk, 1);
	 if(s < 0)
	 {
	 	dcs_log(0,0,"磁道密钥计算失败");
		return -1;
	 }

	len = strlen(ewp_info->trackno2);
	if(len <=0)
	{
		dcs_log(0,0,"无二磁信息");
	}
	else if( len < 17)
	{
		dcs_log(0,0,"ewp 上送二磁 track2 太短:%s", ewp_info->trackno2);
		return -1;
	}
	else
	{
		if(len % 2 == 0)
			memcpy(tmp, ewp_info->trackno2+len-18, 16);
		else
			memcpy(tmp, ewp_info->trackno2+len-17, 16);
		
		#ifdef __TEST__	
		 	dcs_log(0,0,"ewp 上送二磁 track2 :%s", ewp_info->trackno2);
		 	dcs_log(0,0,"ewp 上送二磁密文部分 data:%s", tmp);
		 #endif
		
		 asc_to_bcd(bcd_buf, tmp, 16, 0);
		//tdkey对二磁道密文进行3des解密得到明文磁道内容rtndata
		 s = D3desbylen(bcd_buf,8, rtndata, tdkey, 0);
		 if(s < 0)
		 {
		 	dcs_log(0,0,"二磁道解密失败");
			return -1;
		 }

		memset(tmp, 0, sizeof(tmp));
		bcd_to_asc((unsigned char *)tmp, (unsigned char *)rtndata, 16 ,0);
		#ifdef __TEST__
			dcs_log(0,0,"二磁密文部分解密之后的明文数据 data:%s", tmp);
		#endif
		
		if(len % 2 == 0)
			memcpy(ewp_info->trackno2+len-18, tmp, 16);
		else
			memcpy(ewp_info->trackno2+len-17, tmp, 16);
		
		for(s = 0;s<len;s++){
			if(ewp_info->trackno2[s] == 'D' ||ewp_info->trackno2[s] == 'd' )
				ewp_info->trackno2[s] = '=';
		}
			
		#ifdef __TEST__	
			dcs_log(0,0,"最终明文二磁数据 track2 :%s, len:%d", ewp_info->trackno2, len);
		#endif
		
		setbit(iso, 35, (unsigned char *)ewp_info->trackno2, len);
	}
	
	//解密三磁道
	len = strlen(ewp_info->trackno3);
	if(len <= 0)
	{
		dcs_log(0,0,"无三磁信息");
	}
	else if(len < 17)
	{
#ifdef __LOG__
		dcs_log(0,0,"ewp 上送三磁 track3 太短:%s", ewp_info->trackno3);
#endif
		return -1;
	}
	else
	{
		memset(tmp, 0, sizeof(tmp));
	
		if(len % 2 == 0)
			memcpy(tmp, ewp_info->trackno3+len-18, 16);
		else
			memcpy(tmp, ewp_info->trackno3+len-17, 16);
		
		#ifdef __TEST__		
			dcs_log(0,0,"ewp上送三磁 track3 :%s", ewp_info->trackno3);
			dcs_log(0,0,"ewp上送三磁密文部分 data:%s", tmp);
		#endif
		
		memset(rtndata, 0, sizeof(rtndata));
		memset(bcd_buf, 0, sizeof(bcd_buf));
		 asc_to_bcd(bcd_buf, tmp, 16, 0);
		//tdkey对三磁道密文进行3des解密得到明文磁道内容rtndata
		 s = D3desbylen(bcd_buf, 8,rtndata, tdkey, 0);
		 if(s < 0)
		 {
		 	dcs_log(0,0,"三磁道解密失败");
			return -1;
		 }

		memset(tmp, 0, sizeof(tmp));
		bcd_to_asc((unsigned char *)tmp, (unsigned char *)rtndata, 16 ,0);
		#ifdef __TEST__
			dcs_log(0,0,"三磁密文部分解密之后的明文数据 data:%s", tmp);
		#endif

		if(len % 2 == 0)
			memcpy(ewp_info->trackno3+len-18, tmp, 16);
		else
			memcpy(ewp_info->trackno3+len-17, tmp, 16);
		
		for(s = 0;s<len;s++){
			if(ewp_info->trackno3[s] == 'D' ||ewp_info->trackno3[s] == 'd' )
				ewp_info->trackno3[s] = '=';
		}
		
		#ifdef __TEST__	
			dcs_log(0,0,"最终明文三磁数据 track3 :%s, len:%d", ewp_info->trackno3, len);
		#endif
		setbit(iso, 36, (unsigned char *)ewp_info->trackno3, len);
	}
	return 0;
}
/*
	MPOS转加密函数
	成功返回1，否则返回-1
	tpk : 密码加密密钥，16字节
	pinBlock：pin密文，16字节ASC码
	cardNo: 卡号
	outPin：返回加密之后的pin，8字节
*/
int MposPinConver(char * tpk,char *pinBlock,char *cardNo,char *outPin)
{
	char  resultTemp[16+1],buf[32+1];
	int rtn = 0;
	memset(resultTemp, 0, sizeof(resultTemp));
	memset(buf, 0, sizeof(buf));
	
	//LMK加密的pink
	rtn = getmposlmk(tpk, resultTemp, 16, 1);
	#ifdef  __TEST__
		dcs_debug(resultTemp, 16, "LMK加密pinkey:");
	#endif
	bcd_to_asc(buf, resultTemp, 32, 0);

	memset(resultTemp, 0, sizeof(resultTemp));
	asc_to_bcd(resultTemp, pinBlock, 16, 0);

	return PosPinConvert(buf, "a596ee13f9a93800", cardNo, resultTemp, outPin);
}
/*mpos  mac校验函数
 tak : mac 计算密钥
 返回0 表示成功,-1表示失败
*/
int MposMacCheck(char * tak,EWP_INFO ewp_info)
{
	char resultTemp[16+1], rcvbuf[32+1];
	int rtn = -1;
	memset(resultTemp, 0, sizeof(resultTemp));
	memset(rcvbuf, 0x00, sizeof(rcvbuf));
	
	rtn = getmposlmk(tak, resultTemp, 16, 2);
	if(rtn <0)
	{
		dcs_log(0, 0, "加密mac密钥失败");
		return -1;
	}
	#ifdef  __TEST__
		dcs_debug(resultTemp, 16, "LMK加密mackey:");
	#endif
	
	bcd_to_asc(rcvbuf, resultTemp, 32, 0);

	return mposmac_check(ewp_info, rcvbuf);	
}

/*获取信号量的id
*/
void sem_get(char * buf)
{
	gsem_id =sem_connect(buf, 2);
	if(gsem_id <0)
		gsem_id =sem_create(buf, 2);
}

/*报文哈希值校验
*  参数:
		buf: 要计算的报文内容,
		len: 参与计算的报文长度
		num: 表示报文允许出现最大次数
* 返回值:
		-1:表示操作出现错误,
		0:表示合法报文,
		1:表示报文出现次数已达最大值,为非法报文应丢弃
*/
int ShaCheck(char *buf, int len, int num)
{
	char sha_info[20+1];
	char sha_asc[40+1];
	int nRet =-1;
	memset(sha_info , 0, sizeof(sha_info));
	memset(sha_asc , 0, sizeof(sha_asc));
	sha1_get(buf, len, sha_info);
	bcd_to_asc(sha_asc, sha_info, 40, 0);
	dcs_log(0, 0, "gsem_id =[%d], sha 计算结果是[%s]", gsem_id, sha_asc);
	if(num > 1)
	{
		//排他使用数据库
		sem_lock(gsem_id, 1, 1);
		nRet=GetShaInfo(sha_asc, num);
		sem_unlock(gsem_id, 1, 1);
	}
	else
		nRet=GetShaInfo(sha_asc, num);
	
	return nRet;
}
		
/*
*	对传统POS交易进行防重放
*     参数:
		trancde ：  交易码
		messType：  交易类型
		srcBuf  ：  交易报文,去除TPDU头
		iLen    ：  报文长度
*    return : 
		0 ：表示该报文为合法报文
        1 ：表示为非法重放攻击报文
       -1 ：为其他错误
*/
int  posPreventReplay(char *trancde, char *messType, char *srcBuf, int iLen)
{
	int s = 0;
	/*IC卡参数、公钥下载、参数下载报文无需防重放*/
	/*主密钥下载也不需要防重放*/
   	if(memcmp(trancde, "901", 3) == 0 || memcmp(trancde, "902", 3) == 0 
	||memcmp(trancde, "903", 3) == 0 || memcmp(trancde, "007", 3) == 0 || memcmp(trancde, "008", 3) == 0)
    	{
    		return 0;
   	}
#ifdef __TEST__
	struct timeval start, end;
	gettimeofday(&start, NULL);
	dcs_log(0, 0, "pos start sec[%d] usec[%d]\n", start.tv_sec, start.tv_usec);
#endif
	//冲正类的交易、TC值上送和脱机上送交易最多可以发送4次
	if(memcmp(messType, "0400", 4) == 0 || memcmp(trancde, "046", 3) == 0 || memcmp(trancde, "050", 3) == 0)
	{
		s =ShaCheck(srcBuf, iLen, 4);
		if(s == 1)
		{
			dcs_log(0, 0, "重复报文丢弃");
		}
	}
	else  /*其他交易只允许收到一笔，其他认为是非法报文*/
	{
		
		s =ShaCheck(srcBuf, iLen, 1);
		if(s == 1)
		{
			dcs_log(0, 0, "重复报文丢弃");
		}
		
	}
#ifdef __TEST__
	gettimeofday(&end, NULL);
	dcs_log(0, 0, "pos end sec[%d]usec[%d]\n", end.tv_sec, end.tv_usec);
#endif

	return s;
}

/*
*	对ewp上送交易进行防重放
*   参数:
	messType:   交易类型
	srcBuf  :   交易报文,去除TPDU头
	iLen    :  报文长度
*   return : 
	0 :表示该报文为合法报文
    1 表示为非法重放攻击报文
    -1 为其他错误
*/
int ewpPreventReplay(char *messType, char *srcBuf, int iLen)
{
	int s = 0;
	//冲正类的交易、TC值上送交易、脱机消费最多可以发送4次
	if(memcmp(messType, "0400", 4) == 0 || memcmp(messType, "0320", 4) == 0 ||  memcmp(srcBuf+28, "211", 3) == 0 )
	{
		s =ShaCheck(srcBuf, iLen, 4);
		if(s == 1)
		{
			dcs_log(0, 0, "重复报文丢弃");
		}
	}
	else  /*其他交易只允许收到一笔，其他认为是非法报文*/
	{
		
		s =ShaCheck(srcBuf, iLen , 1);
		if(s == 1)
		{
			dcs_log(0, 0, "重复报文丢弃");
		}
		
	}
	return s;
}


/*处理脱机消费交易结果变更通知其他系统
*/
int DealOfflineSaleResultNotice(PEP_JNLS pep_jnls)
{
	int	s = 0, rtn = 0;
	char tmpbuf[50];
	int i = 0, num = 0;
	int index[10] = {0};

	struct  tm *tm;
	time_t  t;

	char mess[150+1];
	char sha_info[20+1];
	char sha_asc[40+1];
	int queue_id = -1;
	long msgtype = 0;

	memset(tmpbuf, 0, sizeof(tmpbuf));

	time(&t);
	tm = localtime(&t);
	sprintf(tmpbuf,  "%04d%s", tm->tm_year+1900, pep_jnls.revdate);
	//连接共享内存SYN_HTTP_SHM_NAME,获取需要通知的系统链路信息
    struct http_Ast *pCtrl = (struct http_Ast*)shm_connect(SYN_HTTP_SHM_NAME, NULL);
  	if(pCtrl == NULL)
  	{
	    dcs_log(0, 0, "cannot connect share memory [%s]!", SYN_HTTP_SHM_NAME);
	    return -1;
  	}
	for(i=0; i < pCtrl->i_conn_num; i++)
	{
		if((memcmp(pCtrl->httpc[i].recv_queue, "TJTZ",4) == 0) && (pCtrl->httpc[i].iStatus == DCS_STAT_RUNNING ))/*recv_queue 对底层来说是接收队列*/
		{
			index[num] = i;//记录相关记录信息
			num++;
		}			
	}
	if( num == 0)
	{
		dcs_log(0, 0, "没有需要通知的系统链路信息");
		return 0;
	}
	
	/*需要通知相关系统*/
	memset(mess, 0, sizeof(mess));
	memset(sha_info, 0, sizeof(sha_info));
	memset(sha_asc, 0, sizeof(sha_asc));

	sprintf(mess, "%06d%s%s%s%s%s", pep_jnls.trace, pep_jnls.mercode, pep_jnls.termcde, pep_jnls.billmsg, pep_jnls.aprespn, tmpbuf);
	s = strlen(mess);
	sha1_get(mess, s, sha_info);
	bcd_to_asc(sha_asc, sha_info, 40, 0);
	memcpy(mess+s, sha_asc, 40);
	s += 40;	
	
	for(i=0; i<num; i++)/*循环向相关系统发送通知*/
	{
		//发送报文到队列
	  	queue_id = queue_connect(pCtrl->httpc[index[i]].recv_queue);
		if ( queue_id <0)
		{
			dcs_log(0, 0, "connect msg queue fail!");
			return -1;
		}
		msgtype = getpid();
		rtn=queue_send_by_msgtype(queue_id, mess, s, msgtype, 0);
		if(rtn< 0)
		{
			dcs_log(0, 0, "queue_send_by_msgtype err");
			return -1;
		}
		//接收队列
		queue_id = queue_connect(pCtrl->httpc[index[i]].send_queue);
		if ( queue_id <0)
		{
			dcs_log(0, 0, "connect msg queue fail!");
			return -1;
		}
	  	memset(tmpbuf, 0, sizeof(tmpbuf));
	  	rtn= queue_recv_by_msgtype(queue_id, tmpbuf, sizeof(tmpbuf), &msgtype, 0);
		if(rtn< 0)
		{
			dcs_log(0, 0, "queue_recv_by_msgtype err");
			return -1;
		}
		if(memcmp(tmpbuf, "00", 2) == 0)//返回成功
			dcs_log(0, 0, "发送队列[%s]返回应答成功[%s]", pCtrl->httpc[index[i]].recv_queue, tmpbuf+2);
		else
	  		dcs_log(0, 0, "发送队列[%s]返回应答[%s]", pCtrl->httpc[index[i]].recv_queue, tmpbuf);
	}
	return 0;
}

//根据节点取数据
int getnode(char *srvBuf, char *node, char *retBuf)
{
	char subbuf1[127], subbuf2[127];
	char *begin = NULL;
	char *end = NULL;
	char *teststr = NULL;
	int length = 0;
	
	memset(subbuf1, 0x00, sizeof(subbuf1));
	memset(subbuf2, 0x00, sizeof(subbuf2));
	
	sprintf(subbuf1, "%s%s%s", "<", node, ">");
	sprintf(subbuf2, "%s%s%s", "</", node, ">");
	begin = strstr(srvBuf, subbuf1);
	end = strstr(srvBuf, subbuf2);
	if(begin == NULL || end == NULL)
		return -1;
	length = end- begin;
	if(length < strlen(node)+2)
		return -1;
	teststr = (char *)malloc(1000 * sizeof(char));
	memset(teststr, 0x00, 1000);
	strncpy(teststr, begin, length);
	
	length = length-strlen(node)-2;
	memcpy(retBuf, teststr+strlen(node)+2, length);
	free(teststr);
	return length;
}

//生成唯一的UUID(32位)
char *getUUID(char *det)
{
    uuid_t uuid;
    char str[37];
    uuid_generate(uuid);
    uuid_unparse(uuid, str);
    //去除空格
    strncpy(det,str,8);
    strncpy(det+8,str+9,4);
    strncpy(det+12,str+14,4);
    strncpy(det+16,str+19,4);
    strncpy(det+20,str+24,12);
    return det;
}

//把小写字符串转化成大写
int strupr(char *str)
{
    for (; *str!='\0'; str++)
        *str = toupper(*str);
    return 0;
}

//生成签名值
int getUpMD5(char *md5Buf, int len, char *des)
{
	char MD5_asc[33];
	char MD5Value[17];
	int rtn = 0;
	
	memset(MD5_asc, 0x00, sizeof(MD5_asc));
	memset(MD5Value, 0x00, sizeof(MD5Value));
	
	rtn = GetMD5(md5Buf, len, MD5Value);
	if(rtn <0)
		return -1;
	bcd_to_asc(MD5_asc, MD5Value, 32, 0);
	strupr(MD5_asc);
	memcpy(des, MD5_asc, strlen(MD5_asc));
	return 0;
}


char Char2Num(char ch)
{
	if(ch>='0' && ch<='9')return (char)(ch-'0');
	if(ch>='a' && ch<='f')return (char)(ch-'a'+10);
	if(ch>='A' && ch<='F')return (char)(ch-'A'+10);
	return NON_NUM;
}

/************************************************
* 把字符串进行URL编码。
* 输入：
* str: 要编码的字符串
* strSize: 字符串的长度。这样str中可以是二进制数据
* result: 结果缓冲区的地址
* resultSize:结果地址的缓冲区大小(如果str所有字符都编码，该值为strSize*3)
* 返回值：
* >0: result中实际有效的字符长度，
* 0: 编码失败，原因是结果缓冲区result的长度太小
************************************************/
int URLEncode(const char* str, const int strSize, char* result, const int resultSize)
{
	int i;
	int j = 0; /* for result index */
	char ch;

	if ((str == NULL) || (result == NULL) || (strSize <= 0) || (resultSize <= 0))
	{
		return 0;
	}

	for (i=0; (i<strSize) && (j<resultSize); i++)
	{
		ch = str[i];
		if ((ch >= 'A') && (ch <= 'Z'))
		{
			result[j++] = ch;
		}
		else if ((ch >= 'a') && (ch <= 'z'))
		{
			result[j++] = ch;
		}
		else if ((ch >= '0') && (ch <= '9'))
		{
			result[j++] = ch;
		}
		else if(ch == ' ')
		{
			result[j++] = '+';
		}
		else
		{
			if (j + 3 < resultSize)
			{
				sprintf(result+j, "%%%02X", (unsigned char)ch);
				j += 3;
			}
			else
			{
				return 0;
			}
		}
	}

	result[j] = '\0';
	return j;
}


/************************************************
* 把字符串进行URL解码。
* 输入：
* str: 要解码的字符串
* strSize: 字符串的长度。
* result: 结果缓冲区的地址
* resultSize:结果地址的缓冲区大小，可以<=strSize
* 返回值：
* >0: result中实际有效的字符长度，
* 0: 解码失败，原因是结果缓冲区result的长度太小
************************************************/
int URLDecode(const char* str, const int strSize, char* result, const int resultSize)
{
	char ch, ch1, ch2;
	int i;
	int j = 0; /* for result index */

	if ((str == NULL) || (result == NULL) || (strSize <= 0) || (resultSize <= 0))
	{
		return 0;
	}

	for (i=0; (i<strSize) && (j<resultSize); i++)
	{
		ch = str[i];
		switch (ch)
		{
			case '+':
				result[j++] = ' ';
				break;

			case '%':
				if (i+2 < strSize)
				{
					ch1 = Char2Num(str[i+1]);
					ch2 = Char2Num(str[i+2]);
					if ((ch1 != NON_NUM) && (ch2 != NON_NUM))
					{
						result[j++] = (char)((ch1<<4) | ch2);
						i += 2;
						break;
					}
				}
				break;
			/* goto default */
			default:
				result[j++] = ch;
				break;
		}
	}

	result[j] = '\0';
	return j;
}

/*
	获取本机IP地址
*/
int getIP(char *local_ip)
{
	int inet_sock;
    struct ifreq ifr;
    inet_sock = socket(AF_INET, SOCK_DGRAM, 0);

    strcpy(ifr.ifr_name, "eth0");
    //SIOCGIFADDR标志代表获取接口地址
    if(ioctl(inet_sock, SIOCGIFADDR, &ifr) <  0)
	{
		dcs_log(0, 0, "获取IP失败");
        return -1;
	}
    strcpy(local_ip, inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr)); 
	close(inet_sock);
    return 0;
}
