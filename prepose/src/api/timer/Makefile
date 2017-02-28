FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib

LIB= -L$(LIBSPATH)  -ldcs  -ldcsutil -lfold

ALLINC=  -I$(FILESINCS)  
CFLAGS= -O $(ALLINC)  
 

# all is the default rule
all: libtimer.a clean

.c.o:
	gcc -c $< $(ALLINC)
	
# remove the old tapestry library and remake the new one
libtimer.a:	tm_func.o 
		rm -f $@
		ar cq $@  $^
		mv $@ $(LIBSPATH)/$@

clean:
	rm -f *.o

