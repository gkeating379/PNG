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

int freq_comp(const void* a, const void* b);

int sym_comp(const void* a, const void* b);

int* huffman_length(struct SFD sfds[], int len);

int max_number(int arr[], int len);

void set_bl_count(int bit_lengths[], int bl_len, int* bl_count);

void set_next_code(int bl_count[], int max_bits, int next_code[]);

void assign_prefix_codes(int* next_code, int code_count, struct CodeLength tree[]);

void set_tree(int bit_lengths[], int bl_len, struct CodeLength tree[]);

void generate_codes_from_bl(int* bit_lengths, int len, struct CodeLength tree[]);

void generate_codes_from_SFD(struct SFD sfds[], int len, struct CodeLength tree[286]);

