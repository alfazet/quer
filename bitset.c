#include "bitset.h"

int arena_init(arena_t* arena, size_t size) {
    arena->size = size;
    arena->offset = 0;
    arena->start = malloc(size);
    if (arena->start == NULL)
        return -1;
    return 0;
}

void* arena_alloc(arena_t* arena, size_t n_bytes) {
    if (arena->offset + n_bytes > arena->size)
        return NULL;
    arena->offset += n_bytes;
    return (char*)(arena->start) + arena->offset - n_bytes;
}

void arena_free(arena_t* arena) { free(arena->start); }

int bitset_init(bitset_t* bset, int width, int height) {
    bset->width = width;
    bset->height = height;
    bset->arr_w = (width + CELL_SIZE - 1) / CELL_SIZE;
    bset->arr_h = (height + CELL_SIZE - 1) / CELL_SIZE;
    if (arena_init(&bset->arena, ARENA_SIZE) == -1)
        return -1;
    bset->arr = (uint16_t**)arena_alloc(&bset->arena, bset->arr_h * sizeof(uint16_t*));
    if (bset->arr == NULL) {
        arena_free(&bset->arena);
        return -1;
    }
    for (int i = 0; i < bset->arr_h; i++) {
        bset->arr[i] = (uint16_t*)arena_alloc(&bset->arena, bset->arr_w * sizeof(uint16_t));
        if (bset->arr[i] == NULL) {
            arena_free(&bset->arena);
            return -1;
        }
        for (int j = 0; j < bset->arr_w; j++)
            bset->arr[i][j] = 0;
    }
    return 0;
}

int bitset_get(bitset_t* bset, int r, int c) {
    int arr_r = r / CELL_SIZE;
    int arr_c = c / CELL_SIZE;
    int cell_r = r % CELL_SIZE;
    int cell_c = c % CELL_SIZE;
    int bit = CELL_SIZE * cell_r + cell_c;
    return (bset->arr[arr_r][arr_c] & (1 << bit)) > 0;
}

void bitset_set(bitset_t* bset, int r, int c) {
    int arr_r = r / CELL_SIZE;
    int arr_c = c / CELL_SIZE;
    int cell_r = r % CELL_SIZE;
    int cell_c = c % CELL_SIZE;
    int bit = CELL_SIZE * cell_r + cell_c;
    bset->arr[arr_r][arr_c] |= (1 << bit);
}

void bitset_unset(bitset_t* bset, int r, int c) {
    int arr_r = r / CELL_SIZE;
    int arr_c = c / CELL_SIZE;
    int cell_r = r % CELL_SIZE;
    int cell_c = c % CELL_SIZE;
    int bit = CELL_SIZE * cell_r + cell_c;
    bset->arr[arr_r][arr_c] &= (UINT16_MAX - (1 << bit));
}

void bitset_negate(bitset_t* bset, int r, int c) {
    int arr_r = r / CELL_SIZE;
    int arr_c = c / CELL_SIZE;
    int cell_r = r % CELL_SIZE;
    int cell_c = c % CELL_SIZE;
    int bit = CELL_SIZE * cell_r + cell_c;
    bset->arr[arr_r][arr_c] ^= (1 << bit);
}

void bitset_free(bitset_t* bset) { arena_free(&bset->arena); }

void bitset_print(bitset_t* bset) {
    for (int r = 0; r < bset->height; r++) {
        for (int c = 0; c < bset->width; c++) {
            printf("%d ", bitset_get(bset, r, c));
        }
        printf("\n");
    }
}
