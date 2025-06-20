#include "bitset.h"

int bitset_init(bitset_t* bset, int width, int height) {
    bset->width = width;
    bset->height = height;
    bset->arr_w = (width + CELL_SIZE - 1) / CELL_SIZE;
    bset->arr_h = (height + CELL_SIZE - 1) / CELL_SIZE;
    bset->arr = calloc(bset->arr_h, sizeof(uint16_t*));
    if (bset->arr == NULL)
        return -1;
    for (int i = 0; i < bset->arr_h; i++) {
        bset->arr[i] = calloc(bset->arr_w, sizeof(uint16_t));
        if (bset->arr[i] == NULL)
            return -1;
    }
    return 0;
}

int bitset_get(bitset_t* bset, int r, int c) {
    int arr_r = r / CELL_SIZE;
    int arr_c = c / CELL_SIZE;
    int cell_r = r % CELL_SIZE;
    int cell_c = c % CELL_SIZE;
    int bit = CELL_SIZE * cell_r + cell_c;
    return bset->arr[arr_r][arr_c] & (1 << bit);
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

void bitset_free(bitset_t* bset) {
    for (int i = 0; i < bset->arr_h; i++)
        free(bset->arr[i]);
    free(bset->arr);
}

void bitset_print(bitset_t* bset) {
    for (int r = 0; r < bset->height; r++) {
        for (int c = 0; c < bset->width; c++) {
            printf("%d ", bitset_get(bset, r, c) == 0 ? 0 : 1);
        }
        printf("\n");
    }
}
