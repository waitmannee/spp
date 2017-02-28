FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib

LIB= -L $(LIBSPATH)  -ldcs -ldcsutil -lfold -liso8583
CFLAGS=-O -I$(FILESINCS)

LIBOBJ=  secuLib3.o  sha.o Sha256Calc.o md5.o

#测试编译条件
#CDEPEND=-D __TEST__ 

#生产编译条件
CDEPEND=

.c.o:
	$(CC) $(CDEPEND) -c $(CFLAGS) $*.c

EXE = libsecu3.a 

all:  $(EXE)  cleanup
		
libsecu3.a: secuLib3.o  sha.o Sha256Calc.o md5.o
	ar cq $@ $^
	mv $@ $(LIBSPATH)/$@
cleanup:
		rm -f *.o
