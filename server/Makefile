CC=gcc -Wall -Werror
ARCH=$(CROSS_COMPILE)$(CC)

all: aesdsocket

default: aesdsocket

aesdsocket: main.o aesdsocket.o socketserver.o
	$(ARCH) $^ -o $@
main.o: main.c
	$(ARCH) -c $<

asedsocket.o: aesdsocket.c aesdsocket.h
	$(ARCH) -c $<
socketserver.o: socketserver.c socketserver.h
	$(ARCH) -c $<

.PHONY: clean
clean:
	rm -f *.o socketserver