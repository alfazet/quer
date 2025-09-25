#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <stdint.h>

typedef struct bitstream_t {
    uint8_t* values;
    int len_bytes;
    int len_bits;
} bitstream_t;

// add lower n_bits of value to bitstream
void add_bits_to_stream(bitstream_t* bitstream, int value, int n_bits);

#endif  // BITSTREAM_H
