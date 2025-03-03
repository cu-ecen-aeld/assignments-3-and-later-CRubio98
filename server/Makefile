CC=gcc -Wall -Werror -pthread
ARCH=$(CROSS_COMPILE)$(CC)

all: aesdsocket

default: aesdsocket

aesdsocket:  thread_list.o main.o aesdsocket.o socketserver.o socketclient.o thread_list.o
	$(ARCH) $^ -o $@

main.o: main.c
	$(ARCH) -c $<
asedsocket.o: aesdsocket.c aesdsocket.h
	$(ARCH) -c $<
socketserver.o: socketserver.c socketserver.h
	$(ARCH) -c $<
socketclient.o: socketclient.c socketclient.h
	$(ARCH) -c $<
thread_list.o: thread_list.c thread_list.h
	$(ARCH) -c $<

.PHONY: clean
clean:
	rm -f *.o aesdsocket