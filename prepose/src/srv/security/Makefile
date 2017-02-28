FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib
BINSPATH = $(HOME)/prepose/src/temp/bin

ALLINC= -I$(FILESINCS) 
LIB= -L $(LIBSPATH)  -ldcs -ldcsutil -lfold -lpthread   -lnsl
CFLAGS=$(ALLINC)

#测试编译条件
#CDEPEND=-D __TEST__ 

#生产编译条件
CDEPEND=

.c.o:*.c
	$(CC) -c $(CFLAGS) $*.c -o $@

EXE = secuSrv3 secuCmd3 

all:  $(EXE) cleanup

secuSrv3: secuSrv.o
		$(CC) -g -o $@ $(CFLAGS) $^  $(LIB)
		mv $@ $(BINSPATH)/$@
		
secuCmd3:	secuCmd.o  
		$(CC) -g -o $@ $(CFLAGS) $^ $(LIB)
		mv $@ $(BINSPATH)/$@
		
cleanup:
		rm -f *.o
