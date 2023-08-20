CC = gcc
CFLAGS = -I.
DEPS = huffman.h deflate.h inflate.h LZ77.h

%.o: %.c $(DEPS)
	$(CC) -g -c -o $@ $< $(CFLAGS)

decode: huffman.o inflate.o deflate.o LZ77.o
	$(CC) -g -o decode huffman.o inflate.o deflate.o LZ77.o