#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>

struct CodeLength {
    int Code;
    int Len;
};

struct SFD {
    int symbol;
    int freq;
    int depth;
};

struct Basket {
    int total_freq;
    struct SFD* content;
    int content_len;
};

/**
 * Compares if basket a is more frequent than basket b 
 * @return true if a is more frequent than b
*/  
int freq_comp(const void* a, const void* b) {
    struct Basket* _a = (struct Basket*) a;
    struct Basket* _b = (struct Basket*) b;
    return _a->total_freq > _b->total_freq;
}

/**
 * Compares if a is symbolically larger than b 
 * @return true if a is symbolically larger than b
*/  
int sym_comp(const void* a, const void* b) {
    struct SFD* _a = (struct SFD*) a;
    struct SFD* _b = (struct SFD*) b;
    return _a->symbol > _b->symbol;
}


/**
 * Returns the bit_length array where index is the symbol of the bit_length.  Does NOT do the encoding required by Deflate.
 * @param struct SFD sfds[] is the array of sfds to be huffman encoded
 * @param int len is the len of the array
 * @return the MALLOCED bit_length array where index is the symbol of the bit_length
*/ 
int* huffman_length(struct SFD sfds[], int len) {
    // put each in a basket
    struct Basket* baskets = calloc(len, sizeof(struct Basket));
    struct Basket* free_baskets = baskets; //a copy of baskets to be used for freeing later
    int basket_count = len;
    for (int i = 0; i < len; i++) {
        baskets[i].total_freq = sfds[i].freq;
        baskets[i].content = malloc(sizeof(struct SFD));
        *baskets[i].content = sfds[i];
        baskets[i].content_len = 1;
    }

    //order by ascending freq 
    qsort((void *) baskets, len, sizeof(struct Basket), freq_comp);

    //make huffman code but only keep track of depth of each node
    while(basket_count > 1) {
        //add basket 0 to basket 1
        baskets[1].content = (struct SFD*) realloc(baskets[1].content, (baskets[1].content_len + baskets[0].content_len) * sizeof(struct SFD));
        for (int i = 0; i < baskets[0].content_len; i++) {
            baskets[1].content[i + baskets[1].content_len] = baskets[0].content[i];
        }
        baskets[1].content_len += baskets[0].content_len;
        free(baskets[0].content);
        baskets[0].content = NULL;

        baskets[1].total_freq += baskets[1].content_len;
        for (int i = 0; i < baskets[1].content_len; i++) {
            baskets[1].content[i].depth += 1;
        }

        // re-sort by freqs (also eliminates first basket now)
        basket_count--;
        qsort((void *) (baskets + 1), basket_count, sizeof(struct Basket), freq_comp);
        baskets += 1;
    }


    // sort by symbol now
    sfds = baskets[0].content;
    qsort((void *) sfds, len, sizeof(struct SFD), sym_comp);

    // create bit_length
    int* bit_length = calloc(len, sizeof(int));
    for (int i = 0; i < len; i++) {
        bit_length[i] = sfds[i].depth;
    }

    free(baskets[0].content);
    free(free_baskets);
    baskets[0].content = NULL;
    free_baskets = NULL;

    return bit_length;
}

/**
 * Returns the largest int in an int[]
 * @param int arr[] is the array to search 
 * @param int len is the len of the arry
 * @return largest value in the array
*/  
int max_number(int arr[], int len) {
    int max = arr[0];
    for (int i = 1; i < len; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    return max;
}

/**
 * Updates the given bl_count so as such: Let bl_count[N] be the number of codes of length N, N >= 1.
 * @param int bit_lengths[] is the array of bit lengths to be given codes
 * @param int bl_len is the number of bit lengths to be given codes
 * @param int* bl_count is the bl_count to be updated 
*/  
void set_bl_count(int bit_lengths[], int bl_len, int* bl_count) {
    for (int i = 0; i < bl_len; i++) {
        bl_count[bit_lengths[i]] += 1;
    }
}

/**
 * Updates the given next codes so to find the smallest code for each code length
 * @param int* bl_count is the bl_count list
 * @param int max_bits is the largest bit length
 * @param int* next_code is the list of smallest code lengths 
*/  
void set_next_code(int bl_count[], int max_bits, int next_code[]) {
    unsigned int code = 0;
    bl_count[0] = 0;
    for (int bits = 1; bits <= max_bits; bits++) {
        code = (code + bl_count[bits-1]) << 1;
        next_code[bits] = code;
    }
}

/**
 * Create a list of Code/Length pairs that is the final code for each length.  Given where the symbol is the index to the pair
 * @param int next_code is the list of smallest codes for each length
 * @param int code_count is the number of symbols to get codes
 * @param struct CodeLength tree[] is the unset CodeLength tree to be updated
*/ 
void assign_prefix_codes(int* next_code, int code_count, struct CodeLength tree[]) {
    int len;
    for (int n = 0;  n < code_count; n++) {
        len = tree[n].Len;
        if (len != 0) {
            tree[n].Code = next_code[len];
            next_code[len]++;
        }
    }
}

/**
 * Updates the tree to have each symbol set to its proper final bit length
 * @param int bit_lengths[] is the array of bit lengths to be given codes
 * @param int bl_len is the number of bit lengths to be given codes
 * @param struct CodeLength tree[] is the unset CodeLength tree to be updated
*/  
void set_tree(int bit_lengths[], int bl_len, struct CodeLength tree[]) {
    for (int i = 0; i < bl_len; i++) {
        tree[i].Len = bit_lengths[i];
    }
}


/**
 * Generates the Huffman Codes required by Deflate from a list of bit lengths
 * @param int* bit_lengths is the list of bit lengths where index is the symbol of that length
 * @param int len is the number of lengths
 * @param struct CodeLength tree[] is the unset CodeLength tree to be updated
*/  
void generate_codes_from_bl(int* bit_lengths, int len, struct CodeLength tree[]) {
    int bl_count[128] = {0}; //128 used to avoid mallocing because bit lengths SHOULD never be even close to this length
    int next_code[128] = {0};

    set_bl_count(bit_lengths, len, bl_count);
    int max_bits = max_number(bit_lengths, len);
    set_next_code(bl_count, max_bits, next_code);
    set_tree(bit_lengths, len, tree);
    assign_prefix_codes(next_code, len, tree);
}

/**
 * Generates the Huffman Codes required by Deflate from a list of SFDs
 * @param struct SFD sfds[] is the list of sfds for the symbols 
 * @param int len is the number of sfds
 * @param struct CodeLength tree[] is the unset CodeLength tree to be updated
*/  
void generate_codes_from_SFD(struct SFD sfds[], int len, struct CodeLength tree[286]) {
    int* bit_lengths = huffman_length(sfds, len);

    generate_codes_from_bl(bit_lengths, len, tree);

    free(bit_lengths);
    bit_lengths = NULL;
}

int main() {
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

    struct CodeLength tree[288] = {{0}}; //286 is the number of symbols in the LL table, extra initalizing for distance table shouldn't be a huge issue
    generate_codes_from_bl(bit_lengths, len, tree);
    
    for (int i = 0; i < 288; i++) {
        printf("%d: Len %d Code %d\n", i, tree[i].Len, tree[i].Code);
    }

}