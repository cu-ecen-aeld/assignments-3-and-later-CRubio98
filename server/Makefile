CC=gcc -Wall -Werror
ARCH=$(CROSS_COMPILE)$(CC)

all: aesdsocket

default: aesdsocket

aesdsocket: main.o aesdsocket.o
	$(ARCH) $^ -o $@
main.o: main.c
	$(ARCH) -c $<
aesdsocket.o: aesdsocket.c aesdsocket.h
	$(ARCH) -c $<

.PHONY: clean
clean:
	rm -f *.o aesdsocket