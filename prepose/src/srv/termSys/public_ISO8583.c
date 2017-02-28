#include "ibdcs.h"
extern struct ISO_8583 *iso8583;
extern struct ISO_8583 iso8583_conf[128];
extern struct ISO_8583 iso8583conf[128];

extern pthread_key_t iso8583_conf_key;//线程私有变量
extern pthread_key_t iso8583_key;//线程私有变量

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


int str_to_iso( unsigned char * dstr, ISO_data * iso, int Len )
{
	int flaghead, flaglength, flagmap;
	int len, slen, i, s, j;
	char buffer[2048], tmpbuf[1024+1], bcdbuf[999],tmpAsc[999], utftmp[300];
	char dispinfo[8192];
	unsigned char bit[193];
	unsigned char tmp;

	struct ISO_8583 iso8583_conf_local[128];
	memset(iso8583_conf_local, 0, sizeof(iso8583_conf_local));

	if(GetIsoMultiThreadFlag() != 0)//多线程,从线程私有变量里取
	{
		extern pthread_key_t iso8583_conf_key;//线程私有变量
		memcpy(iso8583_conf_local,(struct ISO_8583 *)pthread_getspecific(iso8583_conf_key),sizeof(iso8583_conf_local));
	}
	else
	{
		memcpy(iso8583_conf_local,iso8583_conf,sizeof(iso8583_conf));
	}

	//iso8583=&iso8583_conf[0];
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
	sprintf( dispinfo, "%s[%3.3d] [%-24.24s] [%3.3d] [%s]\n", dispinfo, 0, iso8583name[0], 4, tmpbuf );
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
	sprintf( dispinfo, "%s[%3.3d] [%-24.24s] [%3.3d] [%s]\n", dispinfo, 1, iso8583name[1], (flagmap + 1 ) * 8, tmpbuf );
	slen += (flagmap + 1 ) * 8;
	if ( slen > Len )
		return -1;

	for ( i = 1; i < (flagmap + 1 ) * 8 * 8; i++ )
	{
		if ( bit[i] == 0 )
			continue;
		//dcs_log( dstr + slen, 3, "Set bit[%d] slen=%d. flag=%d, type=%d.", i + 1, slen, iso8583_conf[i].flag, iso8583_conf[i].type );
		if ( iso8583_conf_local[i].flag == 0 )	////固定长
		{
			len = iso8583_conf_local[i].len;
		} else if ( iso8583_conf_local[i].flag == 1 )	////LLVAR可变长
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
		if ( iso8583_conf_local[i].type == 0 )  ///不压缩
		{
			memcpy( tmpbuf, dstr + slen, len );
			slen += len;
		} 
		else	///压缩	暂不考虑左添加
		{
			memset( bcdbuf, 0x0, sizeof( bcdbuf ) );
			memset( tmpAsc, 0x0, sizeof( tmpAsc ) );
			memcpy( bcdbuf, dstr + slen, (len + 1 ) / 2 );
			bcd_to_asc( (unsigned char*) tmpAsc, (unsigned char*) bcdbuf, ((len + 1 ) / 2) * 2, 0 );  //暂时不考虑左边加0的情况
			slen += (len + 1 ) / 2;
			if( iso8583_conf_local[i].type == 2 )//左添加0
			{
				memcpy(tmpbuf,tmpAsc+1,slen);
			}
			else
			{
				memcpy(tmpbuf,tmpAsc,slen);//右添加0
			}
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
		//转账汇款交易的59域和二维码支付的63域含有gbk格式的中文
		if(i == 58 || (i==62 && memcmp(tmpbuf, "EWM", 3) == 0))
		{
			memset( utftmp, 0x0, sizeof( utftmp ) );
			//将上送的GBK格式的中文名转换为UTF8格式
			len = g2u(tmpbuf, len, utftmp, sizeof(utftmp));
			if(len < 0)
			{
				dcs_log( 0, 0, "g2u() fail");
				return -1;
			}
			memset( tmpbuf, 0x0, sizeof( tmpbuf ) );
			memcpy(tmpbuf, utftmp, len);
		}
			
		s = setbit( iso, i+1, (unsigned char *)tmpbuf, len );
		if ( slen > Len || s < 0 )
		{
			dcs_log( 0, 0, "Analyse Packet length error: bit[%d], len=%d. slen[%d] Len[%d] s[%d]", i + 1, len, slen, Len, s );
#ifdef __LOG__
			dcs_log( 0, 0, "%s", dispinfo );
#endif
			return -1;
		}
		if ( i == 51 || i == 54|| i == 63 || i == 127 || i == 191 )  
		{
			memset( bcdbuf, 0x0, sizeof( bcdbuf ) );
			bcd_to_asc( (unsigned char*) bcdbuf, (unsigned char*) tmpbuf, len  * 2, 0 );
			memset( tmpbuf, 0x0, sizeof( tmpbuf ) );
			sprintf( tmpbuf, bcdbuf );
		}
		#ifdef __TEST__
			sprintf( dispinfo, "%s[%3.3d] [%-24.24s] [%3.3d] [%s]\n", dispinfo, i + 1, iso8583name[i + 1], len, tmpbuf );
		#elif defined (__LOG__)
			//2域卡号,35域，36域磁道信息，,45,46域转出卡号，转入卡号（终端）52域密码域生产环境都要屏蔽显示
			if(i!=1 && i!=34 && i!=35 && i!=44 && i!=45 && i!=51)
				sprintf( dispinfo, "%s[%3.3d] [%-24.24s] [%3.3d] [%s]\n", dispinfo, i + 1, iso8583name[i + 1], len, tmpbuf );
		#else
			if(i==2 || i==3 || i==10 ||i==11 || i==12 || i==21 || i==36|| i==37 || i==38 ||  i==40 || i==41)
				sprintf( dispinfo, "%s[%3.3d] [%-24.24s] [%3.3d] [%s]\n", dispinfo, i + 1, iso8583name[i + 1], len, tmpbuf );
		#endif
	}
	//dcs_log( 0, 0, "bit_end" );
	sprintf( dispinfo, "%s-------------------------------------------------------------------", dispinfo );
//#ifdef __LOG__
	dcs_log( 0, 0, "%s", dispinfo );
//#endif
	return 0;
}


int iso_to_str( unsigned char * dstr , ISO_data * iso, int flag )
{
	int flaghead, flaglength, flagmap;
	int len, slen, i, s, num ;;
	char buffer[2048], tmpbuf[999], bcdbuf[999], newbuf[999],tmp[300];
	char map[25];
	char dispinfo[8192];
	struct ISO_8583 iso8583_conf_local[128];
	memset(iso8583_conf_local, 0, sizeof(iso8583_conf_local));

	if(GetIsoMultiThreadFlag() != 0)//多线程,从线程私有变量里取
	{
		extern pthread_key_t iso8583_conf_key;//线程私有变量
		memcpy(iso8583_conf_local,(struct ISO_8583 *)pthread_getspecific(iso8583_conf_key),sizeof(iso8583_conf_local));
	}
	else
	{
		memcpy(iso8583_conf_local,iso8583_conf,sizeof(iso8583_conf));
	}
	//iso8583=&iso8583_conf[0];注释掉
	flaghead = GetIsoHeardFlg();
	flaglength = GetFieldLeagthFlag();
	sprintf( dispinfo, "iso_to_str: flaghead=%d, flaglength=%d.\n", flaghead, flaglength );
	sprintf( dispinfo, "%s-------------------------------------------------------------------\n", dispinfo );
	flagmap = 0;
	memset( buffer, 0x0, sizeof(buffer) );
	memset( map, 0x0, sizeof( map ) );
	memset( tmpbuf, 0x0, sizeof( tmpbuf ) );
	s = getbit( iso, 0, (unsigned char*) tmpbuf );
	//dcs_debug( tmpbuf, s, "Get bit[0  ] len=%d.", s );
	sprintf( dispinfo, "%s[%3.3d] [%-24.24s] [%3.3d] [%s]\n", dispinfo, 0, iso8583name[0], s, tmpbuf );
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
		num = 0;
		memset( newbuf, 0x0, sizeof( newbuf ) );
		s = getbit( iso, i + 1, (unsigned char *)tmpbuf );
		if ( s > 0 )
		{
			tmpbuf[ s] = 0x0;
			//为了显示方便不乱码，特转换为ascii码显示
			if ( i == 51 || i == 54 || i == 63 || i == 127 || i == 191 )
			{
				memset( bcdbuf, 0x0, sizeof( bcdbuf ) );
				bcd_to_asc( (unsigned char*) bcdbuf, (unsigned char*) tmpbuf, s  * 2, 0 );
				sprintf( newbuf, bcdbuf );
			}
			else if(i == 61)
			{
				memset(tmp, 0, sizeof(tmp));
				getbit( iso,20, (unsigned char *)tmp );
				if(memcmp(tmp, "007", 3) == 0)
				{//参数下载时包含中文,为了显示不乱码,转换为UTF8显示
					memset(tmp, 0, sizeof(tmp));
					g2u(tmpbuf+2, 40, tmp, sizeof(tmp));
					sprintf(newbuf, "%s%-40s", "22",tmp);
					num = strlen(newbuf);
					memcpy( newbuf+num, tmpbuf+42, s -42 );
					num = num +s -42;
				}
				else if(memcmp(tmp, "003", 3) == 0)
				{//为了显示方便不乱码，特转换为ascii码显示
					memset( bcdbuf, 0x0, sizeof( bcdbuf ) );
					bcd_to_asc( (unsigned char*) bcdbuf, (unsigned char*) tmpbuf, s  * 2, 0 );
					sprintf( newbuf, bcdbuf );
				}
			}
			else
			{
				memcpy( newbuf, tmpbuf, s );
			}
			if(num == 0)
				num = s;
			#ifdef __TEST__
				sprintf( dispinfo, "%s[%3.3d] [%-24.24s] [%3.3d] [%1.1d] [%1.1d] [%s]\n", dispinfo, i + 1, iso8583name[i + 1], num, iso8583_conf_local[i].flag, iso8583_conf_local[i].type, newbuf );
			#elif defined (__LOG__)
				//2域卡号,20域转出卡号（核心）,35域，36域磁道信息，52域密码域生产环境都要屏蔽显示
				if(i!=1 && i!=19 && i!=34 && i!=35 && i!=51)
					sprintf( dispinfo, "%s[%3.3d] [%-24.24s] [%3.3d] [%1.1d] [%1.1d] [%s]\n", dispinfo, i + 1, iso8583name[i + 1], num, iso8583_conf_local[i].flag, iso8583_conf_local[i].type, newbuf );
			#else
				if(i==2 || i==3 || i==10 ||i==11 || i==12 || i==21 || i==36|| i==37 || i==38 ||  i==40 || i==41)
					sprintf( dispinfo, "%s[%3.3d] [%-24.24s] [%3.3d] [%1.1d] [%1.1d] [%s]\n", dispinfo, i + 1, iso8583name[i + 1], num, iso8583_conf_local[i].flag, iso8583_conf_local[i].type, newbuf );
			#endif
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
			if ( iso8583_conf_local[i].flag == 0 )	////固定长
			{
				if ( iso8583_conf_local[i].type == 0 )  ///不压缩
				{
					memcpy( buffer + slen, tmpbuf, s );
					slen += s;
					if ( iso8583_conf_local[i].len != s )	//长度不够(多或少）
					{
						slen += iso8583_conf_local[i].len - s;
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
				if ( iso8583_conf_local[i].flag == 1 ) //2位可变长
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
				if ( iso8583_conf_local[i].type == 0 )  ///不压缩
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
	sprintf( dispinfo, "%s[%3.3d] [%-24.24s] [%3.3d] [%s]\n", dispinfo, 1, iso8583name[1], (flagmap + 1 ) * 8, tmpbuf );
	//dcs_debug( map, (flagmap + 1) * 8, "Get bitmap len=%d.", (flagmap + 1) * 8 );
	memcpy( dstr + len, map, (flagmap + 1) * 8 );
	len += (flagmap + 1) * 8;
	memcpy( dstr + len, buffer, slen );
	len += slen;
isoend:
	sprintf( dispinfo, "%s-------------------------------------------------------------------", dispinfo );
	if ( flag )
	{
//#ifdef __LOG__
	dcs_log( 0, 0, "%s", dispinfo );
//#endif
	}

	return len;
}





//代码转换:从一种编码转为另一种编码
int code_convert(char* from_charset, char* to_charset, char* inbuf,
               size_t inlen, char* outbuf, size_t outlen)
{
	iconv_t cd;
	char** pin = &inbuf;  
	char** pout = &outbuf;
	int len = outlen;
	cd = iconv_open(to_charset,from_charset);  
	if(cd == 0)
	{
		return -1;
	}
	memset(outbuf,0,outlen);  
	int ret = 0;
	//ret = iconv(cd,pin,(size_t *)&inlen,pout,(size_t *)&outlen);
	ret = iconv(cd,pin,&inlen,pout,&outlen);
	if( ret<0)
	{
		dcs_log(0, 0, "iconv fail,retcode=%d\n",errno );
		iconv_close(cd);
		return -1;  
	}
	*pout='\0';
	iconv_close(cd);
	return len - outlen ;
}
//utf-8码转为GBK码
int u2g(char *inbuf, int inlen, char *outbuf, int outlen)
{
	return code_convert("utf-8", "GBK", inbuf, inlen, outbuf, outlen);
}
//GBK码转为utf-8码
int g2u(char *inbuf, size_t inlen, char *outbuf, size_t outlen)
{
	return code_convert("GBK", "utf-8", inbuf, inlen, outbuf, outlen);
}

