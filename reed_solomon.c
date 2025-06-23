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
}

// void test() {
//     int deg = 30;
//     int poly[MAX_DEGREE];
//     compute_generator_poly(deg, poly);
//     for (int i = 0; i <= deg; i++) {
//         printf("%d\n", log_2[poly[i]]);
//     }
// }
