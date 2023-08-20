#include <stdint.h>
#include <stdio.h>

#include "inflate.h"

/**
 * Reads the next num_bits many bits as a number and returns it
 * @param char* data is the compressed bitstream
 * @param int* cur_byte is the current byte to start reading (data + cur_byte)
 * @param uint8_t* byte_offset is the bit within the current byte to start reading
 * @param int num_bits is the number of bits to read the number as
 * @result the number stored in the next num_bits many bits
*/ 
int read_offset_bits(char* data, int* cur_byte, uint8_t* byte_offset, int num_bits) {
    int output = 0;
    for (int i = 0; i < num_bits; i++) {
        output = output | (data[*cur_byte] & (1 << (7 - *byte_offset)));

        //byte offset incrementor
        *byte_offset = (*byte_offset + 1) % 8; 
        if (!*byte_offset) *cur_byte = *cur_byte + 1; //increment the byte when we use up all the bits in the current byte
    }
    return output;
}

/**
 * Return the base length of the length sym
 * @param int length_sym is the code for the length returned from a LL_decode
 * @result the base length of the length sym
*/ 
int decode_length_sym(int length_sym) {
    int length_table[] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
    return length_table[length_sym - 257];
}

/**
 * Return the base distance of the distance sym
 * @param int distance_sym is the code for the distance returned from a LL_decode
 * @result the base distance of the distance sym
*/ 
int decode_distance_sym(int distance_sym) {
    int distance_table[] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
    return distance_table[distance_sym];
}

/**
 * Convert a length_sym to the final numerical length stored
 * @param int length_sym is the code for the length returned from a LL_decode
 * @result the final numerical length stored.  the length in <length, distance>
*/ 
int len_sym_to_len(char* data, int* cur_byte, uint8_t byte_offset, int length_sym) {
    int length = decode_length_sym(length_sym);
    if (264 < length_sym < 269) { // one offset bit
        length += read_offset_bits(data, cur_byte, byte_offset, 1);
    } else if (269 <= length_sym < 273) { // two offset bit
        length += read_offset_bits(data, cur_byte, byte_offset, 2);
    } else if (273 <= length_sym < 277) { // three offset bit
        length += read_offset_bits(data, cur_byte, byte_offset, 3);
    } else if (277 <= length_sym < 281) { // four offset bit
        length += read_offset_bits(data, cur_byte, byte_offset, 4);
    } else if (281 <= length_sym < 284) { // five offset bit
        length += read_offset_bits(data, cur_byte, byte_offset, 5);
    }
}

/**
 * Convert a distance_sym to the final numerical distance stored
 * @param int distance_sym is the code for the distance returned from a LL_decode
 * @result the final numerical distance stored.  the distance in <length, distance>
*/ 
int dist_sym_to_dist(char* data, int* cur_byte, uint8_t byte_offset, int distance_sym) {
    int distance = decode_distance_sym(distance_sym);
    if (4 <= distance_sym < 6) { //one offset bit
        distance += read_offset_bits(data, cur_byte, byte_offset, 1);
    } else if (6 <= distance_sym < 8) { //two offset bit
        distance += read_offset_bits(data, cur_byte, byte_offset, 2);
    } else if (8 <= distance_sym < 10) { //three offset bit
        distance += read_offset_bits(data, cur_byte, byte_offset, 3);
    } else if (10 <= distance_sym < 12) { //four offset bit
        distance += read_offset_bits(data, cur_byte, byte_offset, 4);
    } else if (12 <= distance_sym < 14) { //five offset bit
        distance += read_offset_bits(data, cur_byte, byte_offset, 5);
    } else if (14 <= distance_sym < 16) { //six offset bit
        distance += read_offset_bits(data, cur_byte, byte_offset, 6);
    } else if (16 <= distance_sym < 18) { //seven offset bit
        distance += read_offset_bits(data, cur_byte, byte_offset, 7);
    } else if (18 <= distance_sym < 20) { //eight offset bit
        distance += read_offset_bits(data, cur_byte, byte_offset, 8);
    } else if (20 <= distance_sym < 22) { //nine offset bit
        distance += read_offset_bits(data, cur_byte, byte_offset, 9);
    } else if (22 <= distance_sym < 24) { //ten offset bit
        distance += read_offset_bits(data, cur_byte, byte_offset, 10);
    } else if (24 <= distance_sym < 26) { //eleven offset bit
        distance += read_offset_bits(data, cur_byte, byte_offset, 11);
    } else if (26 <= distance_sym < 28) { //twelve offset bit
        distance += read_offset_bits(data, cur_byte, byte_offset, 12);
    } else if (28 <= distance_sym < 30) { //thirteen offset bit
        distance += read_offset_bits(data, cur_byte, byte_offset, 13);
    }
}

/**
 * Reads the value stored by <length, distance>
 * @param FILE* fp is the file to write to
 * @param int length is the length
 * @param int distance is the distance
*/ 
void uncompress_dl_pair(FILE* fp, int length, int distance) {
    for (int i = 0; i < length; i++) {
        fp = fseek(fp, -1 * (distance - i), SEEK_CUR); 
        char* cpy;
        if (fread(cpy, 1, 1, fp) != 1) {
            fprintf(stderr, "Failed to read while decoding length,distance pair");
            exit(-1);
        }
        fp = fseek(fp, 0, SEEK_END);
        if (fwrite(cpy, 1, 1, fp) != 1) {
            fprintf(stderr, "Failed to write while decoding length,distance pair");
            exit(-1);
        }
    }
}


/**
 * Reads a length distance pair taking in only the length symbol once it is reached.  Also updates write_len
 * @param char* data is the compressed bitstream
 * @param int* cur_byte is the current byte to start reading (data + cur_byte)
 * @param uint8_t* byte_offset is the bit within the current byte to start reading
 * @param int length_sym is the length symbol read from the LL decoding
 * @param int* write_len is the number of bytes read so far
 * @param FILE* fp is the file to write to 
 * @param struct DecodeTree* LL_tree is the LL decode tree
 * @param struct DecodeTree* distance_tree is the distance decode tree
*/ 
void read_from_LZ77(char* data, int* cur_byte, uint8_t* byte_offset, int length_sym, int* write_len, FILE* fp, struct DecodeTree* LL_tree, struct DecodeTree* distance_tree) {

    int length = len_sym_to_len(data, cur_byte, byte_offset, length_sym);
    int distance_sym = decode_from_tree(data, cur_byte, byte_offset, distance_tree);
    int distance = dist_sym_to_dist(data, cur_byte, byte_offset, distance_sym);

    uncompress_dl_pair(fp, length, distance);

    *write_len += length;
}