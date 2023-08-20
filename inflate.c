#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include <winsock.h>
#include <stdlib.h>

#include "huffman.h"
#include "deflate.h"
#include "inflate.h"
#include "LZ77.h"


#define BLOCK_ZERO_MAX 65535

/**
 * Constructs the tree to decode a prefix code bit by bit
 * @param struct DecodeTree* LL_tree is the decode tree to be made
 * @param struct CodeLength* CL_table is the encode table to construct the tree from
 * @param Fint tree_len is the number of symbols in the tree
*/ 
void make_decode_tree(struct DecodeTree* root, struct CodeLength* CL_table, int tree_len) {\
    int path;
    struct DecodeTree* curTree = root;
    for (int i = 0; i < tree_len; i++) {
        for (int j = CL_table[i].Len; j > 0; j--) {
            path = CL_table[i].Code & (1 << (j - 1)); //get the bits of the code left to right

            //make the next branch node
            struct DecodeTree* child = malloc(sizeof(struct DecodeTree));
            child->val = -1;
            child->zero = NULL;
            child->one = NULL;

            if (path) {
                if (curTree->one == NULL) {
                    curTree->one = child;
                    curTree = child;
                } else {
                    curTree = curTree->one;
                }
            } else {
                if (curTree->zero == NULL) {
                    curTree->zero = child;
                    curTree = child;
                } else {
                    curTree = curTree->zero;
                }
            }
        }
        curTree->val = i; //curTree is now the leaf that will hold the symbol
        curTree = root; //go back to the root
    }
}

/**
 * Constructs the tree to decode the LL prefix codes for BT 1
 * @param struct DecodeTree* LL_tree is the decode tree to be made
*/ 
void make_BT_ONE_LL_decode_tree(struct DecodeTree* LL_tree) {
    struct CodeLength LL_table[288] = {{0}};
    make_BT_ONE_LL_code(LL_table);
    make_decode_tree(LL_tree, LL_table, 288);
}

/**
 * Constructs the tree to decode the distance prefix codes for BT 1
 * @param struct DecodeTree* distance_tree is the decode tree to be made
*/ 
void make_BT_ONE_distance_decode_tree(struct DecodeTree* distance_tree) {
    struct CodeLength distance_table[32] = {{0}};
    make_BT_ONE_distance_code(distance_table);
    make_decode_tree(distance_tree, distance_table, 32);
}

/**
 * Decodes the next element from the tree in the bitstream.  ASSUMES NEXT SYMBOL IS IN TH GIVEN TREE.  NO CHECKING IF IT IS NOT
 * @param char* data is the compressed bitstream
 * @param int* cur_byte is the current byte to start reading (data + cur_byte)
 * @param uint8_t* byte_offset is the bit within the current byte to start reading
 * @param struct DecodeTree* root is the decode tree
 * @result is the first symbol that is read
*/ 
uint16_t decode_from_tree(char* data, int* cur_byte, uint8_t* byte_offset, struct DecodeTree* root) {
    uint16_t sym = 0;
    uint8_t loop_done = 0; //flag to break loop
    uint8_t bit = 0;
    struct DecodeTree* cur_path = root;
    while (!loop_done) {
        bit = data[*cur_byte] & (1 << (7 - *byte_offset));

        if (bit) {
            if (cur_path->one == NULL){
                sym = cur_path->val;
                loop_done = 1;
            } else {
                cur_path = cur_path->one;
            }
        } else {
            if (cur_path->zero == NULL){
                sym = cur_path->val;
                loop_done = 1;
            } else {
                cur_path = cur_path->zero;
            }
        }
        //byte offset incrementor
        *byte_offset = (*byte_offset + 1) % 8; 
        if (!*byte_offset) *cur_byte = *cur_byte + 1; //increment the byte when we use up all the bits in the current byte
    }

    return sym;
}

/**
 * Reads a single block using the LL and distance trees.  Start the pointer after the 3 bit header
 * @param char* data is the compressed bitstream Starting with the data to block header
 * @param int* len is the length of the block read in BITS
 * @param FILE* fp is the pointer to the file that will hold the final uncompressed data
 * @param struct DecodeTree* LL_tree is the decode tree for the LL table
 * @param struct DecodeTree* distance_tree is the decode tree for the distance table
*/ 
void read_block_from_trees(char* data, int* len, FILE* fp, struct DecodeTree* LL_tree, struct DecodeTree* distance_tree){
    int cur_byte = 0;
    uint8_t byte_offset = 3;

    uint16_t last_sym = 0;
    while (1) {
        last_sym = decode_from_tree(data, &cur_byte, &byte_offset, LL_tree);

        if (last_sym == 256) break; //end of block reached

        if (last_sym < 256) { //literal
            char sym = last_sym & 0xff; // get last byte since first byte is not needed here
            if ( fwrite(&sym, 1, 1, fp) != 1){
                fprintf(stderr, "Failed to write a literal while decoding");
                exit(-1);
            }
            *len += 1;
        } else { //length
            read_from_LZ77(data, &cur_byte, &byte_offset, last_sym, len, fp, LL_tree, distance_tree);
        }
    }
}

/**
 * Reads a single block of block type 0
 * @param char* data is the compressed bitstream Starting with the data to block header
 * @param int* len is the length of the block in BITS
 * @param FILE* fp is the pointer to the file that will hold the final uncompressed data
*/ 
void read_BT_ZERO(char* data, int* len, FILE* fp){
    //LEN is number of data bytes in the block 
    int16_t LEN = data[1];
    int16_t NLEN = data[3];

    if (LEN != ~NLEN) {
        fprintf(stderr, "NLEN is not LEN's one's complement in Block Type '00'");
        exit(-1);
    }

    //write
    if ( fwrite(data + 5, 1, LEN, fp) != LEN ) {
        fprintf(stderr, "Could not write full block of data for Block Type '00'");
        exit(-1);
    }
    *len = LEN;
}

/**
 * Reads a single block of block type 1
 * @param char* data is the compressed bitstream Starting with the data to block header
 * @param int* len is the length of the block in BITS
 * @param FILE* fp is the pointer to the file that will hold the final uncompressed data
*/ 
void read_BT_ONE(char* data, int* len, FILE* fp){
    int LL_size = 288;
    int distance_size = 32;

    struct DecodeTree distance_tree; 
    distance_tree.val = -1;
    distance_tree.zero = NULL;
    distance_tree.one = NULL;

    struct DecodeTree LL_tree;
    LL_tree.val = -1;
    LL_tree.zero = NULL;
    LL_tree.one = NULL;

    make_BT_ONE_distance_decode_tree(&distance_tree);
    make_BT_ONE_LL_decode_tree(&LL_tree);

    read_block_from_trees(data, len, fp, &LL_tree, &distance_tree);
}

/**
 * Reads a single block of block type 2
 * @param char* data is the compressed bitstream Starting with the data to block header
 * @param int* len is the length of the block in BITS
 * @param FILE* fp is the pointer to the file that will hold the final uncompressed data
*/ 
void read_BT_TWO(char* data, int* len, FILE* fp){
    
}

/**
 * Reads a single block
 * @param char* data is the compressed bitstream starting with the BFINAL header.  
 * This may be more than the size of the block as we don't check ahead for the end of block marker
 * @param char* BFINAL is the BFINAL flag at the start of the header.  This is updated so the read_data method knows when to end the loop
 * @param int* len is the length of the block in BITS
 * @param FILE* fp is the pointer to the file that will hold the final uncompressed data
*/  
void read_block(char* data, char* BFINAL, int* len, FILE* fp) {
    //TODO someway to back reference across blocks
    //header read
    *BFINAL = data[0] & 1;
    char BTYPE = ((data[0] & 1) << 1) | ((data[0] & 2) >> 1); //flip the bytes (rule 1 for numbers) to make BTYPE literally match the read value
    int byte = 0;   //the byte we are currently reading from
    char byte_offset = 3;
    
    //output init
    int write_size = 0;
    char* output = malloc(0);

    if (!BTYPE) { //block 0 (3.2.4)
        read_BT_ZERO(data, &write_size, fp);
    } 
    else if (BTYPE == 1) { //block 1

    } else if (BTYPE == 2) { //block 2
        
    } else if (BTYPE == 3) { //reserved -> throw error
        fprintf(stderr, "INVALID BLOCK TYPE '11' ENCOUNTERED");
        exit(-1);
    }

}

/**
 * Reads the deflate data block by block
 * @param char* data is the compressed bitstream starting with the first bit of the first block
 * @param FILE* fp the file to write the uncompressed data into
*/  
void read_data(char* data, FILE* fp) {
    char BFINAL = 0; 
    int read_len = 0;
    
    while (!BFINAL) {
        read_block(data, &BFINAL, &read_len, fp); //TODO update the len to read new blocks
    }
}


int main () {
    struct DecodeTree LL_tree;
    LL_tree.val = -1;
    LL_tree.zero = NULL;
    LL_tree.one = NULL;

    make_BT_ONE_LL_decode_tree(&LL_tree);

    char data[2] = {1, 255};
    int cur_byte = 0;
    uint8_t byte_offset = 7;

    uint16_t sym = decode_from_tree(data, &cur_byte, &byte_offset, &LL_tree);
    printf("%d", sym);
}
