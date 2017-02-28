FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib
BINSPATH = $(HOME)/prepose/src/temp/bin
MEMINC = /usr/local/libmemcached/include
ALLINC= -I$(FILESINCS) -I$(MEMINC)
LIB= -L $(LIBSPATH)  -ldcs -ldcsutil -lfold  -lpthread   -lnsl -lsasl2  -lstdc++ -lmemcached
CFLAGS=$(ALLINC)

#测试编译条件
#CDEPEND=-D __TEST__ 

#生产编译条件
CDEPEND=

.c.o:*.c
	$(CC) -c $(CFLAGS) $*.c -o $@
	
EXE = memSrv 

all:  $(EXE)cleanup

memSrv: memSrv.o
		$(CC) -g -o memSrv $(CFLAGS) $^  $(LIB)
		mv memSrv $(BINSPATH)/memSrv
		
cleanup:
		rm -f *.o
