TRANSINCS =  $(HOME)/prepose/src/srv/termSys
FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib
BINSPATH = $(HOME)/prepose/src/temp/bin
MEMINC = /usr/local/libmemcached/include
MYSQLINC = /usr/include/mysql

MYSQLLIB =  -L/usr/lib64/mysql -lmysqlclient  

LIB= -L$(LIBSPATH) -ldcs  -ldcsutil -lfold -liso8583 -lcurl -lmem 
LIBS = -lpthread -lm -ldl -lstdc++ -lrt 

ALLINC=  -I$(FILESINCS) -I$(MEMINC)  -I$(TRANSINCS) -I$(MYSQLINC)
CFLAGS=-O $(ALLINC)  

#依赖文件的搜索路径
VPATH= *.c  $(TRANSINCS)

#测试编译条件
#CDEPEND=-D __TEST__ -D __LOG__

# all is the default rule
all:timeOutRcv clean

.c.o:*.c $(TRANSINCS)/mysql_link_pool.c  $(TRANSINCS)/public_ISO8583.c
	gcc  $(CDEPEND) -c $< $(ALLINC)
	
# remove the old tapestry library and remake the new one

timeOutRcv:	timeOutRcv.o  timeOut_mysql.o  mysql_link_pool.o public_ISO8583.o
		$(CC)  $(CDEPEND) -g -o $@ $(CFLAGS) $^  $(LIB) $(MYSQLLIB)  $(LIBS)
		mv $@ $(BINSPATH)/$@

clean:
	rm -f *.o

