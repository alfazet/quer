#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DEGREE 31
#define MAX_N 255
#define MOD 285

// init lookup tables of powers of 2 and logs base 2 in GF(256)
void init_lut();
// returns the polynomial a_0 + a_1x + ... as {a_0, a_1, ...}
void compute_generator_poly(int deg, int poly[MAX_DEGREE]);
