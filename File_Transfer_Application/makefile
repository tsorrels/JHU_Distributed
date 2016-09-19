CC=gcc

CFLAGS = -g -c -Wall -pedantic
#CFLAGS = -ansi -c -Wall -pedantic

all: sendto_dbg.o rcv ncp file_copy  
		

rcv: rcv.o sendto_dbg.o
	    $(CC) -o rcv rcv.o sendto_dbg.o

ncp: ncp.o sendto_dbg.o
	    $(CC) -o ncp ncp.o sendto_dbg.o

file_copy: file_copy.o
	    $(CC) -o file_copy file_copy.o

clean:
	rm *.o
	rm rcv
	rm ncp
	rm file_copy

%.o:    %.c
	$(CC) $(CFLAGS) $*.c


