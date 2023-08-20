#include <stdint.h>
#include <stdio.h>

void read_from_LZ77(char* data, int* cur_byte, uint8_t* byte_offset, int length_sym, int* write_len, FILE* fp, struct DecodeTree* LL_tree, struct DecodeTree* distance_tree);