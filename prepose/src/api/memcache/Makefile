FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib

LIB= -L $(LIBSPATH) -ldcs -ldcsutil -lfold

CFLAGS=-O -I$(FILESINCS)
LIBOBJ=  memLib.o
#测试编译条件
#CDEPEND=-D __TEST__ 

#生产编译条件
CDEPEND=

.c.o:
	$(CC) $(CDEPEND) -c $(CFLAGS) $*.c

all: libmem.a cleanup
		
libmem.a: memLib.o
	ar cq $@ $^
	mv $@ $(LIBSPATH)/$@
cleanup:
		rm -f *.o
