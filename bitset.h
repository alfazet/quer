#ifndef BITSET_H
#define BITSET_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define CELL_SIZE 4
#define ARENA_SIZE (1 << 16)

typedef struct arena_t {
    size_t size;
    size_t offset;
    void* start;
} arena_t;

typedef struct bitset_t {
    int width;
    int height;
    int arr_w;
    int arr_h;
    uint16_t** arr;
    arena_t arena;
} bitset_t;

int bitset_init(bitset_t* bset, int width, int height);
int bitset_get(bitset_t* bset, int r, int c);
void bitset_set(bitset_t* bset, int r, int c);
void bitset_unset(bitset_t* bset, int r, int c);
void bitset_negate(bitset_t* bset, int r, int c);
void bitset_free(bitset_t* bset);
void bitset_print(bitset_t* bset);

#endif
