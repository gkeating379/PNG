#include "huffman.h"
#include "deflate.h"


/**
 * @param struct CodeLength tree[288] The encode table for the fixed prefix codes for the LL table of Block Type '10'
*/  
void make_BT_ONE_LL_code(struct CodeLength tree[288]) {
    int len = 288;
    int bit_lengths[288]; 

    for (int i = 0; i < len; i++) {
        if (i < 144) {
            bit_lengths[i] = 8;
        } else if (i < 256) {
            bit_lengths[i] = 9;
        } else if (i < 280) {
            bit_lengths[i] = 7;
        } else {
            bit_lengths[i] = 8;
        }
    }

    generate_codes_from_bl(bit_lengths, len, tree);
}

/**
 * @param struct CodeLength tree[32] The encode table for the fixed prefix codes for the distance table of Block Type '10'
*/  
void make_BT_ONE_distance_code(struct CodeLength tree[32]) {
    int len = 32;
    int bit_lengths[32] = {0}; 
    for (int i = 0; i<32; i++) {
       bit_lengths[i] = 5;
    }
    generate_codes_from_bl(bit_lengths, len, tree);
}