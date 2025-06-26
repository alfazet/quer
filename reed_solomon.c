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
            // add temp
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
    uint8_t* res = malloc(block_len * sizeof(uint8_t));
    memcpy(res, msg_bytes + block_start, block_len * sizeof(uint8_t));
    for (int i = 0; i < block_len; i++) {
        int coeff_exp = log_2[res[0]];
        for (int j = 0; j < n_corr_codewords + 1; j++) {
            res[j] ^= pow_2[(log_2[gen_poly[j]] + coeff_exp) % MAX_N];
        }
        for (int j = 0; j < block_len - 1; j++)
            res[j] = res[j + 1];
        if (i >= block_len - n_corr_codewords - 1)
            res[n_corr_codewords] = 0;
    }
    for (int i = 0; i < n_corr_codewords; i++) {
        corr_codewords[i] = res[i];
    }
    free(res);
}

/*
void test() {
    // int deg = 5;
    // int poly[MAX_DEGREE];
    // compute_generator_poly(deg, poly);
    // for (int i = 0; i <= deg; i++) {
    //     printf("%d\n", log_2[poly[i]]);
    // }

    int deg = 10;
    int poly[MAX_DEGREE];
    compute_generator_poly(deg, poly);
    uint8_t msg[16] = {32, 91, 11, 120, 209, 114, 220, 77, 67, 64, 236, 17, 236, 17, 236, 17};
    uint8_t corr_codewords[10];
    compute_corr_codewords(poly, msg, 0, 16, 10, corr_codewords);
    printf("***\n");
    for (int i = 0; i < 10; i++)
        printf("%d\n", corr_codewords[i]);
}
*/
