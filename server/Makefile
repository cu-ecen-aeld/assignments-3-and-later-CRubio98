CC=gcc -Wall -Werror
ARCH=$(CROSS_COMPILE)$(CC)

all: aesdsocket

default: aesdsocket

aesdsocket: aesdsocket.o
	$(ARCH) $^ -o $@
aesdsocket.o: aesdsocket.c aesdsocket.h 
	$(ARCH) -c $<

.PHONY: clean
clean:
	rm -f *.o aesdsocket