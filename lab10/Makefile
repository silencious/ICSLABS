TEAM = NOBODY
VERSION = 1

CC = gcc
CFLAGS = -Wall -g 
LDFLAGS = -pthread

OBJS = proxy.o csapp.o

all: proxy

proxy: $(OBJS)

csapp.o: csapp.c
	$(CC) $(LDFLAGS) $(CFLAGS) -c csapp.c

proxy.o: proxy.c
	$(CC) $(LDFLAGS) $(CFLAGS) -c proxy.c

clean:
	rm -f *~ *.o proxy core

