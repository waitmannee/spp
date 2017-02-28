FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib
BINSPATH = $(HOME)/prepose/src/temp/bin

LIB= -L$(LIBSPATH) -ldcs  -ldcsutil -lfold -liso8583 -lpthread

ALLINC=  -I$(FILESINCS)  
CFLAGS=-O $(ALLINC)  
 

# all is the default rule
all:timerProc clean

.c.o:
	gcc -c $< $(ALLINC)
	
# remove the old tapestry library and remake the new one

timerProc:	timerProc.o
		$(CC) -g -o $@ $(CFLAGS) $^  $(LIB)
		mv $@ $(BINSPATH)/$@

clean:
	rm -f *.o

