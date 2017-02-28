FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib
BINSPATH = $(HOME)/prepose/src/temp/bin

LIB= -L $(LIBSPATH)  -ldcsutil -lfold -ltimer

ALLINC=  -I $(FILESINCS)
CFLAGS= $(ALLINC)
.c.o:*.c
	$(CC) -c $(CFLAGS) $*.c -o $@

EXE = dcxmonisv

all:  $(EXE) clean

dcxmonisv:	dcs_moni.o
		$(CC) -g -o $@  $(CFLAGS) $^ $(LIB)
		mv $@  $(BINSPATH)/$@        
clean:
		rm -f *.o
