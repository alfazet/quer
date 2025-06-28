#include "reed_solomon.h"

int pow_2[MAX_N + 1];
int log_2[MAX_N + 1];

void init_lut() {
    pow_2[0] = 1;
    for (int i = 1; i <= MAX_N; i++) {
        pow_2[i] = pow_2[i - 1] * 2;
        if (pow_2[i] >= 256)
            pow_2[i] ^= MOD;
    }
    for (int i = 1; i <= MAX_N; i++) {
        for (int j = 0; j <= MAX_N; j++) {
            if (pow_2[j] == i) {
                log_2[i] = j;
                break;
            }
        }
    }
}

void compute_generator_poly(int deg, int poly[MAX_DEGREE]) {
    memset(poly, 0, MAX_DEGREE * sizeof(int));
    poly[0] = 1;
    // (x - 2^0)(x - 2^1) ... (x - 2^(deg - 1))
    int temp[MAX_DEGREE];
    for (int i = 0; i < deg; i++) {
        memset(temp, 0, MAX_DEGREE * sizeof(int));
        for (int j = 1; j <= i + 1; j++) {
            // multiply by x
            temp[j] = poly[j - 1];
        }
        for (int j = 0; j <= i + 1; j++) {
            // multiply by -2^i
            if (poly[j] != 0)
                poly[j] = pow_2[(log_2[poly[j]] + i) % MAX_N];
            poly[j] ^= temp[j];
        }
    }
    for (int i = 0; i <= deg / 2; i++) {
        int tmp = poly[i];
        poly[i] = poly[deg - i];
        poly[deg - i] = tmp;
    }
}

void compute_corr_codewords(int* gen_poly, uint8_t* msg_bytes, int block_start, int block_len, int n_corr_codewords,
                            uint8_t* corr_codewords) {
    // the generator poly is implicitly multiplied by x^(block_len - 1), and the message poly by x^n_corr_codewords
    // every iteration the degree of the generator poly decreases by 1
    int degree = block_len > n_corr_codewords + 1 ? block_len : n_corr_codewords + 1;
    uint8_t res[degree];
    for (int i = 0; i < block_len; i++)
        res[i] = msg_bytes[block_start + i];
    // if there are more correction codewords than the length of the block
    for (int i = block_len; i < degree; i++)
        res[i] = 0;
    for (int i = 0; i < block_len; i++) {
        int coeff_exp = log_2[res[0]];
        for (int j = 0; j < n_corr_codewords + 1; j++) {
            res[j] ^= pow_2[(log_2[gen_poly[j]] + coeff_exp) % MAX_N];
        }
        for (int j = 0; j < degree - 1; j++)
            res[j] = res[j + 1];
        if (i >= degree - n_corr_codewords - 1)
            res[n_corr_codewords] = 0;
    }
    for (int i = 0; i < n_corr_codewords; i++)
        corr_codewords[i] = res[i];
}
