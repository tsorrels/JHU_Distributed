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


server:  $(SP_LIBRARY_DIR)/libspread-core.a server.o writer.o recover.o vector.o
	$(LD) -o $@ server.o writer.o recover.o vector.o $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a


client:  $(SP_LIBRARY_DIR)/libspread-core.a client.o vector.o
	$(LD) -o $@ client.o  vector.o $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a


clean:
	rm -f *.o server client
	rm -rf ./recovery_files
