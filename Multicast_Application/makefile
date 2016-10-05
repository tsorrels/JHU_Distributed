CC=gcc

CFLAGS = -g -c -Wall -pedantic
#CFLAGS = -ansi -c -Wall -pedantic

all: sendto_dbg.o mcast start_mcast  
		

mcast: mcast.o sendto_dbg.o
	    $(CC) -o mcast mcast.o sendto_dbg.o

start_mcast: start_mcast.o 
	    $(CC) -o start_mcast start_mcast.o

clean:
	rm *.o
	rm mcast
	rm start_mcast

%.o:    %.c
	$(CC) $(CFLAGS) $*.c


