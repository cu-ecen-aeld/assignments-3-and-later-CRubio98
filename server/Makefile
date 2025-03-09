CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -g -Wall -Werror
LDFLAGS ?= -lpthread -lrt

all: aesdsocket

default: aesdsocket

aesdsocket:  thread_list.o main.o aesdsocket.o socketserver.o socketclient.o thread_list.o
	$(CC) ${CFLAGS} $^ -o $@ ${LDFLAGS}

main.o: main.c
	$(CC) -c $<
asedsocket.o: aesdsocket.c aesdsocket.h
	$(CC) -c $<
socketserver.o: socketserver.c socketserver.h
	$(CC) -c $<
socketclient.o: socketclient.c socketclient.h
	$(CC) -c $<
thread_list.o: thread_list.c thread_list.h
	$(CC) -c $<

.PHONY: clean
clean:
	rm -f *.o aesdsocket