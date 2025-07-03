#include "bitstream.h"

void add_bits_to_stream(bitstream_t* bitstream, int value, int n_bits) {
    int tail = bitstream->len_bits % 8;
    int b = n_bits - 1;
    if (tail != 0) {
        for (int i = 0; i < 8 - tail; i++) {
            if (b < 0)
                return;
            bitstream->values[bitstream->len_bytes - 1] *= 2;
            if ((value & (1 << b)) > 0)
                bitstream->values[bitstream->len_bytes - 1]++;
            bitstream->len_bits++;
            b--;
        }
    }
    while (b >= 0) {
        if (bitstream->len_bits % 8 == 0)
            bitstream->len_bytes++;
        bitstream->values[bitstream->len_bytes - 1] *= 2;
        if ((value & (1 << b)) > 0)
            bitstream->values[bitstream->len_bytes - 1]++;
        bitstream->len_bits++;
        b--;
    }
}
