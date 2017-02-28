#ifndef DCSCPGDEF_H
#define DCSCPGDEF_H

#define	  DCS	       		3    //DCS subsystem
#define   DCMHSTSRV_C  		0    //dcmhstsrv.c
#define   DCMTCP_C     		1    //dcmtcp.c

// 定义通讯服务器函数代号
// (DCS=3,DCMHSTSRV_C=0)
#define P_HstSrv		(DCS * 10000 + DCMHSTSRV_C * 100 + 1)	// 30001
#define P_HstSvrExit		(DCS * 10000 + DCMHSTSRV_C * 100 + 2)	// 30002
#define P_LoadBankCounter	(DCS * 10000 + DCMHSTSRV_C * 100 + 3)	// 30003
#define P_LoadCommLnk		(DCS * 10000 + DCMHSTSRV_C * 100 + 4)	// 30004
#define P_StartAllComm		(DCS * 10000 + DCMHSTSRV_C * 100 + 5)	// 30005
#define P_DoService		(DCS * 10000 + DCMHSTSRV_C * 100 + 6)	// 30006
#define P_HstSrvOpenLog		(DCS * 10000 + DCMHSTSRV_C * 100 + 7)	// 30007
#define P_StartComm		(DCS * 10000 + DCMHSTSRV_C * 100 + 8)	// 30008
#define P_AnswerRequest		(DCS * 10000 + DCMHSTSRV_C * 100 + 9)	// 30009
#define P_HandleCommDie		(DCS * 10000 + DCMHSTSRV_C * 100 + 10)	// 30010
#define P_UpdateNetLink		(DCS * 10000 + DCMHSTSRV_C * 100 + 11)	// 30011
#define P_HandleLinkEstab	(DCS * 10000 + DCMHSTSRV_C * 100 + 12)	// 30012
#define P_HandleCommandLine	(DCS * 10000 + DCMHSTSRV_C * 100 + 13)	// 30013
#define P_HandleAppTimeout	(DCS * 10000 + DCMHSTSRV_C * 100 + 14)	// 30014

// (DCS=3,DCMTCP_C=1)
#define P_Tcpip			(DCS * 10000 + DCMTCP_C * 100 + 1)	// 30101
#define P_TcpipExit		(DCS * 10000 + DCMTCP_C * 100 + 2)	// 30102
#define P_TcpipOpenLog		(DCS * 10000 + DCMTCP_C * 100 + 3)	// 30103
#define P_GetLinkInfo 		(DCS * 10000 + DCMTCP_C * 100 + 4)	// 30104
#define P_MakeConnection	(DCS * 10000 + DCMTCP_C * 100 + 5)	// 30105
#define P_ReadFromNet		(DCS * 10000 + DCMTCP_C * 100 + 6)	// 30106
#define P_ReadFromIse		(DCS * 10000 + DCMTCP_C * 100 + 7)	// 30107
#define P_NotifyHstSvr		(DCS * 10000 + DCMTCP_C * 100 + 8)	// 30108
#define P_TcpipUpdateBCDANet	(DCS * 10000 + DCMTCP_C * 100 + 9)	// 30109

#endif
