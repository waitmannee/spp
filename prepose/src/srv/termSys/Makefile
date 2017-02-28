FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib
BINSPATH = $(HOME)/prepose/src/temp/bin
MYSQLINC = /usr/include/mysql
MEMINC = /usr/local/libmemcached/include
MYSQLLIB =  -L/usr/lib64/mysql -lmysqlclient  
# libxml2的头文件路径
XML2INC =  -Wall -g `xml2-config --cflags`

LIBS = -lpthread -lm -ldl -lstdc++ -lrt `xml2-config --libs`

ALLINC=  -I$(FILESINCS)  -I$(MYSQLINC)   -I$(XML2INC) -I$(MEMINC)

LIB= -L$(LIBSPATH)  -ldcs  -ldcsutil -lfold -liso8583 -ltimer -lsecu3  -lcurl -lmem -luuid -ltimeout
CFLAGS=-O $(ALLINC)

#测试编译条件
#CDEPEND=-D __TEST__ -D __LOG__ -D __MPOSKEY__

#生产编译条件
CDEPEND=-D __MPOSKEY__

EXE = TransRcv   HTServer MoniPosp SMSRcv PinEncrySrv XPETransRcv TransferRcv
#moniChongZheng moniChexiao

.c.o:
	$(CC) $(CDEPEND) -c $(CFLAGS) $*.c

all: $(EXE)  clean

TransRcv: TranRcv.o public.o  public_ISO8583.o dssdclfn_mysql.o  ewpHT_mysql.o  mpos.o ewpProc.o mposProc.o mposProc_yindian.o glptProc.o  transfer_mysql.o transferProc.o dealPay.o appProcEpos.o ahttpclient.o posProc.o custProc.o mysql_link_pool.o appMposProc.o  xmlUtils.o
		$(CC) $(CDEPEND) -g -o $@ $(CFLAGS) $^ $(LIB) $(MYSQLLIB) $(LIBS)
		strip $@ 2>/dev/null
		mv $@ $(BINSPATH)/$@
		
HTServer: HTServer.o h3cHTProc.o ewpHT_mysql.o dssdclfn_mysql.o ewpHTProc.o  mpos.o ewpProc.o mposProc.o mposProc_yindian.o ahttpclient.o posProc.o public.o  public_ISO8583.o mysql_link_pool.o xmlUtils.o
		$(CC) $(CDEPEND) -g -o $@ $(CFLAGS) $^ $(LIB) $(MYSQLLIB) $(LIBS)
		strip $@ 2>/dev/null
		mv $@ $(BINSPATH)/$@
	
MoniPosp: MoniPosp.o dssdclfn_mysql.o public.o public_ISO8583.o custProc.o mpos.o ewpProc.o mposProc.o mposProc_yindian.o transfer_mysql.o  transferProc.o dealPay.o appProcEpos.o ahttpclient.o posProc.o moni_posp.o moni_cups.o mysql_link_pool.o xmlUtils.o
	$(CC) $(CDEPEND) -g -o $@  $(CFLAGS) $^  $(LIB) $(MYSQLLIB) $(LIBS)
	strip $@ 2>/dev/null
	mv $@ $(BINSPATH)/$@ 

SMSRcv: SMSRcv.o smsdb_mysql.o
	$(CC)  $(CDEPEND) -g -o $@  $(CFLAGS) $^ $(LIB) $(MYSQLLIB) $(LIBS)
	strip $@ 2>/dev/null
	mv $@ $(BINSPATH)/$@

PinEncrySrv: PinEncryProc.o public.o public_ISO8583.o dssdclfn_mysql.o ahttpclient.o posProc.o ewpProc.o mpos.o mysql_link_pool.o xmlUtils.o
	$(CC)  $(CDEPEND) -g -o $@  $(CFLAGS) $^ $(LIB) $(MYSQLLIB) $(LIBS)
	strip $@ 2>/dev/null
	mv $@ $(BINSPATH)/$@
	
XPETransRcv: XPETransRcv.c public.o public_ISO8583.o dssdclfn_mysql.c  ewpHT_mysql.o  mpos.o ewpProc.o mposProc.o mposProc_yindian.o glptProc.o transfer_mysql.o transferProc.o dealPay.o appProcEpos.o ahttpclient.o posProc.o  mysql_link_pool.o xmlUtils.o
		$(CC) $(CDEPEND) -g -o $@ $(CFLAGS) $^ $(LIB) $(MYSQLLIB) $(LIBS)
		strip $@ 2>/dev/null
		mv $@ $(BINSPATH)/$@

TransferRcv: transferRcv.o transferProc.o public.o  public_ISO8583.o dssdclfn_mysql.o  transfer_mysql.o  posProc.o  mpos.o ewpProc.o mposProc.o ahttpclient.o appProcEpos.o  dealPay.o dealAsynReturnProc.o  nextDayPayProc.o mysql_link_pool.o
		$(CC) $(CDEPEND) -g -o $@ $(CFLAGS) $^ $(LIB) $(MYSQLLIB) $(LIBS)
		strip $@ 2>/dev/null
		mv $@ $(BINSPATH)/$@

#moniChongZheng:moniChongzheng.c
#		$(CC) $(CDEPEND) -g -o moniChongZheng $(CFLAGS) $^ $(LIB) $(MYSQLLIB) $(LIBS)
#		strip $@ 2>/dev/null
#		mv $@ $(BINSPATH)/$@

#moniChexiao:moniChexiao.c
#		$(CC) $(CDEPEND) -g -o moniChexiao $(CFLAGS) $^ $(LIB) $(MYSQLLIB) $(LIBS)
#		strip $@ 2>/dev/null
#		mv $@ $(BINSPATH)/$@
clean:
	rm -f *.o

	
