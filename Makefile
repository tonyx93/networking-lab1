CC=gcc
CFLAGS=-c -Wall

all: webserver

webserver: webserver.o
	$(CC) webserver.o -o webserver

webserver.o: webserver.c
	$(CC) $(CFLAGS) webserver.c

clean:
	rm *~ *.o webserver