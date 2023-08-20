#include <stdint.h>

struct DecodeTree {
    int val;    //val of -1 means its just a branch and holds no prefix code
    struct DecodeTree* zero;
    struct DecodeTree* one;
};

uint16_t decode_from_tree(char* data, int* cur_byte, uint8_t* byte_offset, struct DecodeTree* root);