#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define CELL_SIZE 4

typedef struct bitset_t {
    int width;
    int height;
    int arr_w;
    int arr_h;
    uint16_t** arr;
} bitset_t;

int bitset_init(bitset_t* bset, int width, int height);
int bitset_get(bitset_t* bset, int r, int c);
void bitset_set(bitset_t* bset, int r, int c);
void bitset_unset(bitset_t* bset, int r, int c);
void bitset_negate(bitset_t* bset, int r, int c);
void bitset_free(bitset_t* bset);
void bitset_print(bitset_t* bset);
