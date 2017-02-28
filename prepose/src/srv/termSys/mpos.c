#include "ibdcs.h"

#ifdef __MPOSKEY__
/*
 * 多媒体终端远程密钥下载功能
 *
 * @param pikIndex:pik主密钥索引
 * @param makIndex:mak主密钥索引
 * @param tdkIndex:tdk主密钥索引
 * @param psam:psam卡号，ascii码，16位长
 *
 * @return rtnmposKey,格式：pik主密钥(32字节长度)+pik主密钥校验值（8字节长度）+mak主密钥(32字节长度)+mak主密钥校验值（8字节长度）+tdk主密钥(32字节长度)+tdk主密钥校验值（8字节长度）
 * @return int, 成功返回1，失败返回-1
 * */
int mposKeyDownload(int pikIndex, int makIndex, int tdkIndex, int zmkindex, char *psam, char *rtnmposKey)
{
	char _mposKey[16+1],_mposKeyCheckValue[4+1], _mposkeyAll[20+1],_encmposKey[16+1];
	char mposKey[32+1],mposKeyCheckValue[8+1];
	int result,t=0,key,tmp=0;

	dcs_log(0,0, "mposKeyDownload psam=%s", psam);
	for(;t<3;t++)
	{
		if(t==0)
			key = pikIndex;
		else if(t == 1)
			key = makIndex;
		else
			key = tdkIndex;

		memset(_mposKey, 0, sizeof(_mposKey));
		memset(_mposKeyCheckValue, 0, sizeof(_mposKeyCheckValue));
		memset(_mposkeyAll, 0, sizeof(_mposkeyAll));
		dcs_log(0,0, "MPOS_KeyGen t= %d key=%d, psam=%s", t, key, psam);
		result = MPOS_KeyGen(key, psam, _mposkeyAll);
		/*if(result >= 0 && strlen(_mposkeyAll) == 20)*/
		if(result >= 0)
		{
			memcpy(_mposKey, _mposkeyAll, 16);
			memcpy(_mposKeyCheckValue, _mposkeyAll+16, 4);

			memset(mposKey, 0, sizeof(mposKey));
			memset(mposKeyCheckValue, 0, sizeof(mposKeyCheckValue));
			bcd_to_asc((unsigned char *)mposKey, (unsigned char *)_mposKey, 32 ,0);
			bcd_to_asc((unsigned char *)mposKeyCheckValue, (unsigned char *)_mposKeyCheckValue, 8 ,0);

			dcs_log(0,0,"%d mposKey=%s",t, mposKey);
			dcs_log(0,0,"%d mposKeyCheckValue=%s",t,mposKeyCheckValue);

			memset(_encmposKey, 0, sizeof(_encmposKey));
			
			dcs_log(0, 0, "MPOS_KeyEnc want to");
			//result = MPOS_KeyEnc(802, _mposKey, _encmposKey);/*zmkindex*/
			result = MPOS_KeyEnc(zmkindex, _mposKey, _encmposKey);
			dcs_log(0, 0, "MPOS_KeyEnc end result =%d", result);
			dcs_log(0, 0, "strlen(_encmposKey) =%d", strlen(_encmposKey));

			#ifdef __LOG__
				dcs_debug(_encmposKey, 16, "_encmposKey: ");
			#endif
			/*if(result >= 0 && strlen(_encmposKey) == 16)*/
			if(result >= 0)
			{
				memset(mposKey, 0, sizeof(mposKey));
				bcd_to_asc((unsigned char *)mposKey, (unsigned char *)_encmposKey, 32 ,0);

				dcs_log(0,0,"%d enmposKey=%s",t,mposKey);
				memcpy(rtnmposKey+tmp, mposKey, 32);
				memcpy(rtnmposKey+tmp+32, mposKeyCheckValue, 8);
				tmp += 40;
			}else
				return -1;
		}else
			return -1;
	}

	dcs_log(0,0,"rtnmposKey=%s", rtnmposKey);
	return 1;
}
#endif
