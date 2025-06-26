#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DEGREE 31
#define MAX_N 255
#define MOD 285

// init lookup tables of powers of 2 and logs base 2 in GF(256)
void init_lut();
// returns the polynomial a_nx^n + a_{n - 1}x^{n - 1} + ... as {a_n, a_{n - 1}, ...}
void compute_generator_poly(int deg, int poly[MAX_DEGREE]);
void compute_corr_codewords(int* gen_poly, uint8_t* msg_bytes, int block_start, int block_len, int n_corr_codewords,
                            uint8_t* corr_codewords);

// void test();
