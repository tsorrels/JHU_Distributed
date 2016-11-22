CC=gcc
LD=gcc
CFLAGS=-g -Wall 
#CPPFLAGS=-I. -I/home/cs437/exercises/ex3/include
CPPFLAGS=-I. -I./include
#SP_LIBRARY_DIR=/home/cs437/exercises/ex3
SP_LIBRARY_DIR=.

all: server client

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<


server:  $(SP_LIBRARY_DIR)/libspread-core.a server.o recover.o
	$(LD) -o $@ server.o  recover.o $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a


client:  $(SP_LIBRARY_DIR)/libspread-core.a client.o
	$(LD) -o $@ client.o  $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a


clean:
	rm -f *.o server client

