FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib
TRANSINCS = $(HOME)/prepose/src/srv/termSys
MEMINC = /usr/local/libmemcached/include
MYSQLINC = /usr/include/mysql

MYSQLLIB =  -L/usr/lib64/mysql -lmysqlclient  

LIB= -L$(LIBSPATH)  -ldcs  -ldcsutil -lfold -liso8583 -lcurl -lmem 
LIBS = -lpthread -lm -ldl -lstdc++ -lrt 

ALLINC=  -I$(FILESINCS) -I$(MEMINC) -I$(MYSQLINC)
CFLAGS=-O $(ALLINC)  

#依赖文件的搜索路径
VPATH= *.c  $(TRANSINCS)

#测试编译条件
#CDEPEND=-D __TEST__ -D __LOG__

# all is the default rule
all: libtimeout.a  clean

.c.o:*.c $(TRANSINCS)/mysql_link_pool.c  $(TRANSINCS)/public_ISO8583.c
	gcc  $(CDEPEND) -c $< $(ALLINC)
	
# remove the old tapestry library and remake the new one
libtimeout.a:	timeOutLib.o 
		rm -f $@
		ar cq $@  $^
		mv $@ $(LIBSPATH)/$@
clean:
	rm -f *.o

