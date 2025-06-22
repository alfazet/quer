#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bitset.h"

#define ERR_AND_DIE(...) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), fprintf(stderr, __VA_ARGS__), exit(EXIT_FAILURE))

static const char* USAGE_MSG = "Usage: ./qr <data> [error correction level]";
static const int CAPS_CORR_L[41] = {0, 17,   32,   53,   78,   106,  134,  154,  192,  230,  271,  321,  367,  425,  458,
                                    520,  586,  644,  718,  792,  858,  929,  1003, 1091, 1171, 1273, 1367, 1465, 1528,
                                    1628, 1732, 1840, 1952, 2068, 2188, 2303, 2431, 2563, 2699, 2809, 2953};
static const int CAPS_CORR_M[41] = {0, 14,   26,   42,   62,   84,   106,  122,  152,  180,  213,  251,  287,  331,  362,
                                    412,  450,  504,  560,  624,  666,  711,  779,  857,  911,  997,  1059, 1125, 1190,
                                    1264, 1370, 1452, 1538, 1628, 1722, 1809, 1911, 1989, 2099, 2213, 2331};
static const int TOTAL_CODEWORDS_L[41] = {0, 19,   34,   55,   80,   108,  136,  156,  194,  232,  274,
                                          324,  370,  428,  461,  523,  589,  647,  721,  795,  861,
                                          932,  1006, 1094, 1174, 1276, 1370, 1468, 1531, 1631, 1735,
                                          1843, 1955, 2071, 2191, 2306, 2434, 2566, 2702, 2812, 2956};

enum corr_level_t {
    CORR_L,
    CORR_M,
    CORR_Q,
    CORR_H,
};

typedef struct rgb_color_t {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_color_t;

void save_as_ppm(bitset_t* code, rgb_color_t* color, int mod_sz, int padding, char* filename) {
    FILE* file = fopen(filename, "w");
    if (file == NULL)
        ERR_AND_DIE("fopen");

    int width = (code->width + 2 * padding) * mod_sz;
    int height = (code->height + 2 * padding) * mod_sz;
    if (fprintf(file, "P6 %d %d 255\n", width, height) < 0)
        ERR_AND_DIE("fprintf");

    uint8_t* buf = malloc(3 * width * height * sizeof(uint8_t));
    memset(buf, 255, 3 * width * height * sizeof(uint8_t));
    for (int y = padding * mod_sz; y < height - padding * mod_sz; y += mod_sz) {
        for (int x = padding * mod_sz; x < width - padding * mod_sz; x += mod_sz) {
            if (bitset_get(code, (y - padding * mod_sz) / mod_sz, (x - padding * mod_sz) / mod_sz) > 0) {
                for (int i = 0; i < mod_sz; i++) {
                    for (int j = 0; j < 3 * mod_sz; j += 3) {
                        int offset = 3 * width * (y + i) + 3 * x + j;
                        buf[offset + 0] = color->r;
                        buf[offset + 1] = color->g;
                        buf[offset + 2] = color->b;
                    }
                }
            }
        }
    }
    for (int i = 0; i < 3 * width * height; i++) {
        if (fwrite(&buf[i], sizeof(buf[i]), 1, file) == 0)
            ERR_AND_DIE("fwrite");
    }
    free(buf);
    if (fclose(file))
        ERR_AND_DIE("fclose");
}

void draw_finder_pattern(bitset_t* code, int sx, int sy) {
    for (int y = 0; y < 7; y++) {
        for (int x = 0; x < 7; x++) {
            bitset_set(code, sx + x, sy + y);
        }
    }
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            bitset_unset(code, sx + x + 1, sy + y + 1);
        }
    }
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            bitset_set(code, sx + x + 2, sy + y + 2);
        }
    }
}

void draw_timing_patterns(bitset_t* code, int sx, int sy) {
    int x = sx + 7 + 1;
    int y = sy + 7 - 1;
    int flip = 1;
    for (; x < code->width - 7 - 1; x++) {
        if (flip == 1)
            bitset_set(code, x, y);
        else
            bitset_unset(code, x, y);
        flip ^= 1;
    }

    x = sx + 7 - 1;
    y = sy + 7 + 1;
    flip = 1;
    for (; y < code->height - 7 - 1; y++) {
        if (flip == 1)
            bitset_set(code, x, y);
        else
            bitset_unset(code, x, y);
        flip ^= 1;
    }
}

int get_alignment_pattern_positions(int version, int positions[7]) {
    if (version == 1)
        return 0;
    int count = version / 7 + 2;
    int delta = (version * 8 + count * 3 + 5) / (count * 4 - 4) * 2;
    int pos = version * 4 + 10;
    for (int i = count - 1; i >= 1; i--) {
        positions[i] = pos;
        pos -= delta;
    }
    positions[0] = 6;
    return count;
}

void draw_alignment_patterns(bitset_t* code, int version) {
    int pos[7];
    int count = get_alignment_pattern_positions(version, pos);
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < count; j++) {
            // these ones would overlap with the finder patterns
            if ((i == 0 && j == 0) || (i == 0 && j == count - 1) || (i == count - 1 && j == 0))
                continue;

            for (int y = pos[i] - 2; y <= pos[i] + 2; y++) {
                for (int x = pos[j] - 2; x <= pos[j] + 2; x++) {
                    bitset_set(code, x, y);
                }
            }
            for (int y = pos[i] - 1; y <= pos[i] + 1; y++) {
                for (int x = pos[j] - 1; x <= pos[j] + 1; x++) {
                    bitset_unset(code, x, y);
                }
            }
            bitset_set(code, pos[j], pos[i]);
        }
    }
}

void draw_version_pattern(bitset_t* code, int version) {}

// add lower n_bits of value to bitstream
// fix this
void add_bits_to_stream(uint8_t* bitstream, int* bitstream_idx, int* bitstream_len, int value, int n_bits) {
    int rem = 8 * sizeof(uint8_t) - ((*bitstream_len) % 8);
    int b = n_bits - 1;
    for (int i = 0; i < rem; i++) {
        if (b < 0)
            return;
        bitstream[*bitstream_idx] *= 2;
        if ((value & (1 << b)) > 0)
            bitstream[*bitstream_idx]++;
        (*bitstream_len)++;
        b--;
    }
    if (rem == 8)
        (*bitstream_idx)++;
    if (b >= 0)
        (*bitstream_idx)++;
    while (b >= 0) {
        bitstream[*bitstream_idx] *= 2;
        if ((value & (1 << b)) > 0)
            bitstream[*bitstream_idx]++;
        (*bitstream_len)++;
        b--;
    }
}

// TODO: pack (bitstream, idx, len) into a single struct
void fill_data(uint8_t* bitstream, char* data, int data_len, const int* total_codewords_lut, int version) {
    int idx = 0;
    int len = 0;
    add_bits_to_stream(bitstream, &idx, &len, 0b0100, 4);
    add_bits_to_stream(bitstream, &idx, &len, data_len, (version <= 9 ? 8 : 16));
    for (int i = 0; i < data_len; i++) {
        add_bits_to_stream(bitstream, &idx, &len, data[i], 8);
    }
    int total_bits = total_codewords_lut[version] * 8;
    int terminator_bits = (total_bits - len >= 4 ? 4 : total_bits - len);
    add_bits_to_stream(bitstream, &idx, &len, 0, terminator_bits);
    if (len % 8 > 0)
        add_bits_to_stream(bitstream, &idx, &len, 0, 8 - (len % 8));
    int pad_byte = 0b11101100;
    while (len < total_bits) {
        add_bits_to_stream(bitstream, &idx, &len, pad_byte, 8);
        pad_byte ^= (0b11101100 ^ 0b00010001);
    }
    printf("Final bitstream:\n");
    for (int i = 0; i <= idx; i++) {
        printf("%d\n", bitstream[i]);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("%s\n", USAGE_MSG);
        return EXIT_FAILURE;
    }
    char* data = argv[1];
    int data_len = strlen(data);
    enum corr_level_t corr_level = CORR_L;
    const int* max_cap_lut = CAPS_CORR_L;
    const int* total_codewords_lut = TOTAL_CODEWORDS_L;
    if (argc >= 3) {
        char* flag = argv[2];
        if (strcmp(flag, "-l") == 0) {
            corr_level = CORR_L;
            max_cap_lut = CAPS_CORR_L;
            total_codewords_lut = TOTAL_CODEWORDS_L;
        }
        if (strcmp(flag, "-m") == 0) {
            corr_level = CORR_M;
            max_cap_lut = CAPS_CORR_M;
            // total_codewords = TOTAL_CODEWORDS_M;
        }
        if (strcmp(flag, "-q") == 0) {
            corr_level = CORR_Q;
            // max_cap = CAPS_CORR_Q;
            // total_codewords = TOTAL_CODEWORDS_Q;
        }
        if (strcmp(flag, "-h") == 0) {
            corr_level = CORR_H;
            // max_cap = CAPS_CORR_H;
            // total_codewords = TOTAL_CODEWORDS_H;
        }
    }

    int version = 1;
    while (max_cap_lut[version] < data_len)
        version++;
    printf("version: %d\n", version);
    int codewords_cap = max_cap_lut[version];

    int dim = 4 * version + 17;
    bitset_t code;
    bitset_init(&code, dim, dim);

    uint8_t bitstream[dim * dim];
    memset(bitstream, 0, dim * dim * sizeof(uint8_t));
    fill_data(bitstream, data, data_len, total_codewords_lut, version);

    // finder patterns
    int finder_coords_x[3] = {0, dim - 7, 0};
    int finder_coords_y[3] = {0, 0, dim - 7};
    for (int i = 0; i < 3; i++) {
        draw_finder_pattern(&code, finder_coords_x[i], finder_coords_y[i]);
    }
    // timing patterns
    draw_timing_patterns(&code, finder_coords_x[0], finder_coords_y[0]);
    // alignment patterns
    draw_alignment_patterns(&code, version);
    if (version >= 7)
        draw_version_pattern(&code, version);

    rgb_color_t color = {.r = 128, .g = 0, .b = 128};
    save_as_ppm(&code, &color, 20, 5, "code.ppm");
    bitset_free(&code);

    return EXIT_SUCCESS;
}
