// based on ISO-18004
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bitset.h"
#include "reed_solomon.h"

#define ERR_AND_DIE(...) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), fprintf(stderr, __VA_ARGS__), exit(EXIT_FAILURE))
static const char* USAGE_MSG = "Usage: ./qr <data> [error correction level]";

// data capacity (in bytes) for given error correction level and version
static const int CAPACITY[4][41] = {
    {0,    17,   32,   53,   78,   106,  134,  154,  190,  226,  262,  321,  367,  419,
     461,  523,  589,  647,  714,  792,  858,  929,  1003, 1091, 1171, 1273, 1367, 1465,
     1528, 1628, 1732, 1840, 1952, 2068, 2188, 2303, 2431, 2563, 2699, 2809, 2953},
    {0,    14,   26,   42,   62,   84,   106,  122,  152,  180,  213,  251,  287,  331,
     362,  412,  450,  504,  560,  624,  666,  711,  779,  857,  911,  997,  1059, 1125,
     1190, 1264, 1370, 1452, 1538, 1628, 1722, 1809, 1911, 1989, 2099, 2213, 2331},
    {0,   11,  20,  32,  46,  60,  74,  86,  108, 130, 151,  177,  203,  241,  258,  292,  322,  364,  394,  442, 482,
     509, 565, 611, 661, 715, 751, 805, 868, 908, 982, 1030, 1112, 1168, 1228, 1283, 1351, 1423, 1499, 1579, 1663},
    {0,   7,   14,  24,  34,  44,  58,  64,  84,  98,  119, 137, 155, 177, 194, 220,  250,  280,  310,  338, 382,
     403, 439, 461, 511, 535, 593, 625, 658, 698, 742, 790, 842, 898, 958, 983, 1051, 1093, 1139, 1219, 1273}};

// total number of data codewords for given error correction level and version
static const int TOTAL_DATA_CODEWORDS[4][41] = {
    {0,    19,   34,   55,   80,   108,  136,  156,  194,  232,  274,  324,  370,  428,
     461,  523,  589,  647,  721,  795,  861,  932,  1006, 1094, 1174, 1276, 1370, 1468,
     1531, 1631, 1735, 1843, 1955, 2071, 2191, 2306, 2434, 2566, 2702, 2812, 2956},
    {0,    16,   28,   44,   64,   86,   108,  124,  154,  182,  216,  254,  290,  334,
     365,  415,  453,  507,  563,  627,  669,  714,  782,  860,  914,  1000, 1062, 1128,
     1193, 1267, 1373, 1455, 1541, 1631, 1725, 1812, 1914, 1992, 2102, 2216, 2334},
    {0,    13,   22,   34,   48,   62,   76,   88,   110,  132,  154,  178,  204,  224,
     279,  335,  395,  468,  535,  619,  667,  714,  782,  860,  914,  1000, 1062, 1128,
     1193, 1267, 1373, 1455, 1541, 1631, 1725, 1812, 1914, 1992, 2102, 2216, 2334},
    {0,   9,   16,  26,  36,  46,  60,  66,  86,  100, 122, 140, 158, 180, 197, 223,  253,  283,  313,  341, 385,
     406, 442, 464, 514, 538, 596, 628, 661, 701, 745, 793, 845, 901, 961, 986, 1054, 1096, 1142, 1222, 1276}};

// number of modules (bits) available in the entire code (excluding function patterns) for given version
static const int TOTAL_AVAILABLE_MODULES[41] = {
    0,     208,   359,   567,   807,   1079,  1383,  1568,  1936,  2336,  2768,  3232,  3728,  4256,
    4651,  5243,  5867,  6523,  7211,  7931,  8683,  9252,  10068, 10916, 11796, 12708, 13652, 14628,
    15371, 16411, 17483, 18587, 19723, 20891, 22091, 23008, 24272, 25568, 26896, 28256, 29648};

static const int CORR_CODEWORDS_PER_BLOCK[4][41] = {
    {0,  7,  10, 15, 20, 26, 18, 20, 24, 30, 18, 20, 24, 26, 30, 22, 24, 28, 30, 28, 28,
     28, 28, 30, 30, 26, 28, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30},
    {0,  10, 16, 26, 18, 24, 16, 18, 22, 22, 26, 30, 22, 22, 24, 24, 28, 28, 26, 26, 26,
     26, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28},
    {0,  13, 22, 18, 26, 18, 24, 18, 22, 20, 24, 28, 26, 24, 20, 30, 24, 28, 28, 26, 30,
     28, 30, 30, 30, 30, 28, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30},
    {0,  17, 28, 22, 16, 22, 28, 26, 26, 24, 28, 24, 28, 22, 24, 24, 30, 28, 28, 26, 28,
     30, 24, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30},
};

static const int TOTAL_BLOCKS[4][41] = {
    {0, 1, 1, 1,  1,  1,  2,  2,  2,  2,  4,  4,  4,  4,  4,  6,  6,  6,  6,  7, 8,
     8, 9, 9, 10, 12, 12, 12, 13, 14, 15, 16, 17, 18, 19, 19, 20, 21, 22, 24, 25},
    {0,  1,  1,  1,  2,  2,  4,  4,  4,  5,  5,  5,  8,  9,  9,  10, 10, 11, 13, 14, 16,
     17, 17, 18, 20, 21, 23, 25, 26, 28, 29, 31, 33, 35, 37, 38, 40, 43, 45, 47, 49},
    {0,  1,  1,  2,  2,  4,  4,  6,  6,  8,  8,  8,  10, 12, 16, 12, 17, 16, 18, 21, 20,
     23, 23, 25, 27, 29, 34, 34, 35, 38, 40, 43, 45, 48, 51, 53, 56, 59, 62, 65, 68},
    {0,  1,  1,  2,  4,  4,  4,  5,  6,  8,  8,  11, 11, 16, 16, 18, 16, 19, 21, 25, 25,
     25, 34, 30, 32, 35, 37, 40, 42, 45, 48, 51, 54, 57, 60, 63, 66, 70, 74, 77, 81},
};

static const int VERSION_INFO[41] = {0,       0,       0,       0,       0,       0,       0,       0x07C94, 0x085BC,
                                     0x09A99, 0x0A4D3, 0x0BBF6, 0x0C762, 0x0D847, 0x0E60D, 0x0F928, 0x10B78, 0x1145D,
                                     0x12A17, 0x13532, 0x149A6, 0x15683, 0x168C9, 0x177EC, 0x18EC4, 0x191E1, 0x1AFAB,
                                     0x1B08E, 0x1CC1A, 0x1D33F, 0x1ED75, 0x1F250, 0x209D5, 0x216F0, 0x228BA, 0x2379F,
                                     0x24B0B, 0x2542E, 0x26A64, 0x27541, 0x28C69};

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

typedef struct bitstream_t {
    uint8_t* values;
    int len_bytes;
    int len_bits;
} bitstream_t;

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

// add lower n_bits of value to bitstream
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

void fill_data(bitstream_t* bitstream, char* data, int data_len, enum corr_level_t corr_level, int version) {
    add_bits_to_stream(bitstream, 0b0100, 4);
    add_bits_to_stream(bitstream, data_len, (version <= 9 ? 8 : 16));
    for (int i = 0; i < data_len; i++) {
        add_bits_to_stream(bitstream, data[i], 8);
    }
    int total_bits = TOTAL_DATA_CODEWORDS[(int)corr_level][version] * 8;
    int terminator_bits = (total_bits - bitstream->len_bits >= 4 ? 4 : total_bits - bitstream->len_bits);
    add_bits_to_stream(bitstream, 0, terminator_bits);
    if (bitstream->len_bits % 8 > 0)
        add_bits_to_stream(bitstream, 0, 8 - (bitstream->len_bits % 8));
    int pad_byte = 0b11101100;
    while (bitstream->len_bits < total_bits) {
        add_bits_to_stream(bitstream, pad_byte, 8);
        pad_byte ^= (0b11101100 ^ 0b00010001);
    }
}

void add_error_correction_and_interleave(bitstream_t* bitstream, enum corr_level_t corr_level, int version,
                                         uint8_t* res) {
    int n_blocks = TOTAL_BLOCKS[(int)corr_level][version];
    int n_corr_codewords_per_block = CORR_CODEWORDS_PER_BLOCK[(int)corr_level][version];
    int n_all_codewords = TOTAL_AVAILABLE_MODULES[version] / 8;
    int corr_offset = TOTAL_DATA_CODEWORDS[(int)corr_level][version];
    int n_small_blocks = n_blocks - n_all_codewords % n_blocks;
    int small_block_len = n_all_codewords / n_blocks - n_corr_codewords_per_block;
    init_lut();

    int gen_poly[MAX_DEGREE];
    uint8_t* corr_codewords = malloc(n_corr_codewords_per_block * sizeof(uint8_t));
    compute_generator_poly(n_corr_codewords_per_block, gen_poly);

    int block_start = 0;
    for (int i = 0; i < n_blocks; i++) {
        int block_len = (i < n_small_blocks ? small_block_len : small_block_len + 1);
        compute_corr_codewords(gen_poly, bitstream->values, block_start, block_len, n_corr_codewords_per_block,
                               corr_codewords);
        // interleave
        for (int j = 0; j < block_len; j++)
            res[i + n_blocks * j] = bitstream->values[block_start + j];
        for (int j = 0; j < n_corr_codewords_per_block; j++)
            res[corr_offset + i + n_blocks * j] = corr_codewords[j];
        block_start += block_len;
    }
    free(corr_codewords);
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

void draw_version_pattern(bitset_t* code, int version, int dim) {
    int version_info = VERSION_INFO[version];
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 3; j++) {
            int b = 3 * i + j;
            if ((version_info & (1 << b)) == 0) {
                bitset_unset(code, dim - 11 + j, i);
                bitset_unset(code, i, dim - 11 + j);
            } else {
                bitset_set(code, dim - 11 + j, i);
                bitset_set(code, i, dim - 11 + j);
            }
        }
    }
}

void draw_functional_patterns(bitset_t* code, int version, int dim) {
    // finder patterns
    int finder_coords_x[3] = {0, dim - 7, 0};
    int finder_coords_y[3] = {0, 0, dim - 7};
    for (int i = 0; i < 3; i++) {
        draw_finder_pattern(code, finder_coords_x[i], finder_coords_y[i]);
    }
    // timing patterns
    draw_timing_patterns(code, finder_coords_x[0], finder_coords_y[0]);
    // alignment patterns
    draw_alignment_patterns(code, version);
    if (version >= 7)
        draw_version_pattern(code, version, dim);
}

int main(int argc, char** argv) {
    // init_lut();
    // test();
    // exit(1);
    if (argc < 2) {
        printf("%s\n", USAGE_MSG);
        return EXIT_FAILURE;
    }
    char* data = argv[1];
    int data_len = strlen(data);
    enum corr_level_t corr_level = CORR_L;
    if (argc >= 3) {
        char* flag = argv[2];
        if (strcmp(flag, "-l") == 0) {
            corr_level = CORR_L;
        }
        if (strcmp(flag, "-m") == 0) {
            corr_level = CORR_M;
        }
        if (strcmp(flag, "-q") == 0) {
            corr_level = CORR_Q;
        }
        if (strcmp(flag, "-h") == 0) {
            corr_level = CORR_H;
        }
    }

    int version = 1;
    while (version <= 40 && CAPACITY[(int)corr_level][version] < data_len)
        version++;
    if (version > 40) {
        printf("input too long to be stored in a QR code with the specified error correction capacity\n");
        return EXIT_FAILURE;
    }

    int dim = 4 * version + 17;
    bitstream_t bitstream = {.len_bytes = 0, .len_bits = 0};
    bitstream.values = calloc(TOTAL_AVAILABLE_MODULES[version] / 8 + 1, sizeof(uint8_t));
    fill_data(&bitstream, data, data_len, corr_level, version);

    int total_codewords = TOTAL_DATA_CODEWORDS[(int)corr_level][version] +
                          TOTAL_BLOCKS[(int)corr_level][version] * CORR_CODEWORDS_PER_BLOCK[(int)corr_level][version];
    uint8_t result[total_codewords];
    add_error_correction_and_interleave(&bitstream, corr_level, version, result);
    for (int i = 0; i < total_codewords; i++)
        printf("%d\n", result[i]);

    bitset_t code;
    bitset_init(&code, dim, dim);
    draw_functional_patterns(&code, version, dim);

    free(bitstream.values);

    rgb_color_t color = {.r = 192, .g = 0, .b = 0};
    save_as_ppm(&code, &color, 20, 5, "code.ppm");
    bitset_free(&code);

    return EXIT_SUCCESS;
}
