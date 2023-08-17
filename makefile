CC=gcc
CFLAGS=-I.
DEPS = huffman.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

decode: huffman.o inflate.o 
	$(CC) -o decode huffman.o inflate.o 