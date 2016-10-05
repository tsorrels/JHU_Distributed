CC=gcc

CFLAGS = -g -c -Wall -pedantic
#CFLAGS = -ansi -c -Wall -pedantic

all: sendto_dbg.o rcv ncp t_rcv t_ncp  
		

rcv: rcv.o sendto_dbg.o
	    $(CC) -o rcv rcv.o sendto_dbg.o

ncp: ncp.o sendto_dbg.o
	    $(CC) -o ncp ncp.o sendto_dbg.o

t_rcv: t_rcv.o
	    $(CC) -o t_rcv t_rcv.o

t_ncp: t_ncp.o
	    $(CC) -o t_ncp t_ncp.o

clean:
	rm *.o
	rm rcv
	rm ncp
	rm t_rcv
	rm t_ncp

%.o:    %.c
	$(CC) $(CFLAGS) $*.c


