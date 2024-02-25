CC=gcc
ARCH=$(CROSS_COMPILE)$(CC)

default: writer

writer: writer.o
	$(ARCH) $^ -o $@
writer.o: writer.c
	$(ARCH) -c $<

.PHONY: clean
clean:
	rm -f *.o writer