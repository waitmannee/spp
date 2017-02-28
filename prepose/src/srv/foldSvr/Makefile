FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib
BINSPATH = $(HOME)/prepose/src/temp/bin

LIB= -L $(LIBSPATH)  -lfold   -ldcsutil

ALLINC=  -I $(FILESINCS)
CFLAGS=$(ALLINC)
.c.o:*.c
	$(CC) -c $(CFLAGS) $*.c -o $@

EXE = foldSvr foldUtil 

all:  $(EXE) $(LIBOBJ) clean

foldSvr:	fold_svr.o
		$(CC) -g -o $@ $(CFLAGS) $^ $(LIB)
		mv $@ $(BINSPATH)/$@   
        
foldUtil:	fold_cmd.o
		$(CC) -g -o $@ $(CFLAGS) $^ $(LIB)
		mv $@ $(BINSPATH)/$@
clean:
		rm -f *.o
