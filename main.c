#include <getopt.h>
#include <limits.h>
#include <png.h>

#include "bitset.h"
#include "bitstream.h"
#include "reed_solomon.h"

#define ERR_AND_DIE(...)                                                                         \
    (fprintf(stderr, "fatal error: %s:%d - ", __FILE__, __LINE__), fprintf(stderr, __VA_ARGS__), \
     fprintf(stderr, "\n"), exit(EXIT_FAILURE))
#define USAGE_STR                                                                                                     \
    "quer [-i input_file (default: stdin)] [-o output_file (default: stdout)] [-[l]ow/-[m]edium/-[q]uartile/-[h]igh " \
    "(error correction level, "                                                                                       \
    "default: "                                                                                                       \
    "-l)] [-p pixels_per_module (default: 20)]"

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
static const int MAX_CAPACITY = 2953;

// total number of data codewords for given error correction level and version
static const int TOTAL_DATA_CODEWORDS[4][41] = {
    {0,    19,   34,   55,   80,   108,  136,  156,  194,  232,  274,  324,  370,  428,
     461,  523,  589,  647,  721,  795,  861,  932,  1006, 1094, 1174, 1276, 1370, 1468,
     1531, 1631, 1735, 1843, 1955, 2071, 2191, 2306, 2434, 2566, 2702, 2812, 2956},
    {0,    16,   28,   44,   64,   86,   108,  124,  154,  182,  216,  254,  290,  334,
     365,  415,  453,  507,  563,  627,  669,  714,  782,  860,  914,  1000, 1062, 1128,
     1193, 1267, 1373, 1455, 1541, 1631, 1725, 1812, 1914, 1992, 2102, 2216, 2334},
    {0,   13,  22,  34,  48,  62,  76,  88,  110, 132, 154,  180,  206,  244,  261,  295,  325,  367,  397,  445, 485,
     512, 568, 614, 664, 718, 754, 808, 871, 911, 985, 1033, 1115, 1171, 1231, 1286, 1354, 1426, 1502, 1582, 1666},
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

// lookup table for encoded version information
static const int VERSION_INFO[41] = {0,       0,       0,       0,       0,       0,       0,       0x07C94, 0x085BC,
                                     0x09A99, 0x0A4D3, 0x0BBF6, 0x0C762, 0x0D847, 0x0E60D, 0x0F928, 0x10B78, 0x1145D,
                                     0x12A17, 0x13532, 0x149A6, 0x15683, 0x168C9, 0x177EC, 0x18EC4, 0x191E1, 0x1AFAB,
                                     0x1B08E, 0x1CC1A, 0x1D33F, 0x1ED75, 0x1F250, 0x209D5, 0x216F0, 0x228BA, 0x2379F,
                                     0x24B0B, 0x2542E, 0x26A64, 0x27541, 0x28C69};

int mask0(int i, int j) { return ((i + j) % 2) == 0; }
int mask1(int i, int j) {
    (void)j;
    return (i % 2) == 0;
}
int mask2(int i, int j) {
    (void)i;
    return (j % 3) == 0;
}
int mask3(int i, int j) { return ((i + j) % 3) == 0; }
int mask4(int i, int j) { return ((i / 2 + j / 3) % 2) == 0; }
int mask5(int i, int j) { return ((i * j) % 2 + (i * j) % 3) == 0; }
int mask6(int i, int j) { return (((i * j) % 2 + (i * j) % 3) % 2) == 0; }
int mask7(int i, int j) { return (((i + j) % 2 + (i * j) % 3) % 2) == 0; }
static int (*masks[8])(int, int) = {mask0, mask1, mask2, mask3, mask4, mask5, mask6, mask7};

enum corr_level_t {
    CORR_L,
    CORR_M,
    CORR_Q,
    CORR_H,
};

int save_as_png(bitset_t* code, int ppm, int padding, FILE* file) {
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL)
        return -1;
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        png_destroy_write_struct(&png_ptr, NULL);
        return -1;
    }
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return -1;
    }

    int width = (code->width + 2 * padding) * ppm;
    int height = (code->height + 2 * padding) * ppm;
    png_init_io(png_ptr, file);
    png_set_IHDR(png_ptr, info_ptr, width, height, 1, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png_ptr, info_ptr);
    png_set_packing(png_ptr);
    png_set_invert_mono(png_ptr);

    unsigned char image_row[width];
    memset(image_row, 0, width * sizeof(unsigned char));
    for (int i = 0; i < padding * ppm; i++) {
        png_write_row(png_ptr, image_row);
    }
    for (int y = 0; y < code->height * ppm; y++) {
        int x = 0;
        for (int i = padding * ppm; i < width - padding * ppm; i++) {
            image_row[i] = bitset_get(code, y / ppm, x / ppm);
            x++;
        }
        png_write_row(png_ptr, image_row);
    }
    memset(image_row, 0, width * sizeof(unsigned char));
    for (int i = 0; i < padding * ppm; i++) {
        png_write_row(png_ptr, image_row);
    }

    png_write_end(png_ptr, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return 0;
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
    int big_block_len = small_block_len + 1;
    init_lut();

    int gen_poly[MAX_DEGREE];
    uint8_t corr_codewords[n_corr_codewords_per_block];
    compute_generator_poly(n_corr_codewords_per_block, gen_poly);

    int block_start = 0;
    for (int i = 0; i < n_blocks; i++) {
        int block_len = (i < n_small_blocks ? small_block_len : big_block_len);
        compute_corr_codewords(gen_poly, bitstream->values, block_start, block_len, n_corr_codewords_per_block,
                               corr_codewords);
        int idx = i;
        for (int j = 0; j < block_len; j++) {
            // so that we don't leave empty spaces
            if (j == big_block_len - 1)
                idx -= n_small_blocks;
            res[idx] = bitstream->values[block_start + j];
            idx += n_blocks;
        }
        for (int j = 0; j < n_corr_codewords_per_block; j++) {
            res[corr_offset + i + n_blocks * j] = corr_codewords[j];
        }
        block_start += block_len;
    }
}

void draw_separator(bitset_t* code, int sx, int sy, bitset_t* blocked) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            bitset_unset(code, sy + y, sx + x);
            bitset_set(blocked, sy + y, sx + x);
        }
    }
}

void draw_finder_pattern(bitset_t* code, int sx, int sy, bitset_t* blocked) {
    for (int y = 0; y < 7; y++) {
        for (int x = 0; x < 7; x++) {
            bitset_set(code, sy + y, sx + x);
            bitset_set(blocked, sy + y, sx + x);
        }
    }
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            bitset_unset(code, sy + y + 1, sx + x + 1);
            bitset_set(blocked, sy + y + 1, sx + x + 1);
        }
    }
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            bitset_set(code, sy + y + 2, sx + x + 2);
            bitset_set(blocked, sy + y + 2, sx + x + 2);
        }
    }
}

void draw_timing_patterns(bitset_t* code, int sx, int sy, bitset_t* blocked) {
    int x = sx + 7 + 1;
    int y = sy + 7 - 1;
    int flip = 1;
    for (; x < code->width - 7 - 1; x++) {
        if (flip == 1)
            bitset_set(code, y, x);
        else
            bitset_unset(code, y, x);
        bitset_set(blocked, y, x);
        flip ^= 1;
    }
    x = sx + 7 - 1;
    y = sy + 7 + 1;
    flip = 1;
    for (; y < code->height - 7 - 1; y++) {
        if (flip == 1)
            bitset_set(code, y, x);
        else
            bitset_unset(code, y, x);
        bitset_set(blocked, y, x);
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

void draw_alignment_patterns(bitset_t* code, int version, bitset_t* blocked) {
    int pos[7];
    int count = get_alignment_pattern_positions(version, pos);
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < count; j++) {
            // these ones would overlap with the finder patterns
            if ((i == 0 && j == 0) || (i == 0 && j == count - 1) || (i == count - 1 && j == 0))
                continue;

            for (int y = pos[i] - 2; y <= pos[i] + 2; y++) {
                for (int x = pos[j] - 2; x <= pos[j] + 2; x++) {
                    bitset_set(code, y, x);
                    bitset_set(blocked, y, x);
                }
            }
            for (int y = pos[i] - 1; y <= pos[i] + 1; y++) {
                for (int x = pos[j] - 1; x <= pos[j] + 1; x++) {
                    bitset_unset(code, y, x);
                    bitset_set(blocked, y, x);
                }
            }
            bitset_set(code, pos[i], pos[j]);
            bitset_set(blocked, pos[i], pos[j]);
        }
    }
}

void draw_version_pattern(bitset_t* code, int version, int dim, bitset_t* blocked) {
    int version_info = VERSION_INFO[version];
    for (int y = 0; y < 6; y++) {
        for (int x = 0; x < 3; x++) {
            int b = 3 * y + x;
            if ((version_info & (1 << b)) == 0) {
                bitset_unset(code, dim - 11 + x, y);
                bitset_unset(code, y, dim - 11 + x);
            } else {
                bitset_set(code, dim - 11 + x, y);
                bitset_set(code, y, dim - 11 + x);
            }
            bitset_set(blocked, dim - 11 + x, y);
            bitset_set(blocked, y, dim - 11 + x);
        }
    }
}

void block_format_info(int dim, bitset_t* blocked) {
    for (int x = 0; x < 9; x++)
        bitset_set(blocked, 8, x);
    for (int y = 0; y < 9; y++)
        bitset_set(blocked, y, 8);
    for (int x = 0; x < 8; x++)
        bitset_set(blocked, 8, dim - 1 - x);
    for (int y = 0; y < 7; y++)
        bitset_set(blocked, dim - 1 - y, 8);
}

void draw_functional_patterns(bitset_t* code, int version, int dim, bitset_t* blocked) {
    int separator_coords_x[3] = {0, dim - 8, 0};
    int separator_coords_y[3] = {0, 0, dim - 8};
    for (int i = 0; i < 3; i++)
        draw_separator(code, separator_coords_x[i], separator_coords_y[i], blocked);
    int finder_coords_x[3] = {0, dim - 7, 0};
    int finder_coords_y[3] = {0, 0, dim - 7};
    for (int i = 0; i < 3; i++)
        draw_finder_pattern(code, finder_coords_x[i], finder_coords_y[i], blocked);
    draw_timing_patterns(code, finder_coords_x[0], finder_coords_y[0], blocked);
    draw_alignment_patterns(code, version, blocked);
    if (version >= 7)
        draw_version_pattern(code, version, dim, blocked);
    // format info (just block, will be filled in later)
    block_format_info(dim, blocked);
    // that single black module in the lower left corner
    bitset_set(code, dim - 8, 8);
    bitset_set(blocked, dim - 8, 8);
}

void draw_data(bitset_t* code, uint8_t* data, int data_len, int dim, bitset_t* blocked) {
    int bit = 7, byte = 0;
    for (int col = dim - 1; col >= 1; col -= 2) {
        // the "parity" changes after column 6 (a column fully devoted to function patterns)
        if (col == 6)
            col = 5;
        for (int row = 0; row < dim; row++) {
            for (int side = 0; side <= 1; side++) {
                int x = col - side;
                // 0 = down, 1 = up
                int dir = ((col + 1) % 4 == 0 || (col + 1) % 4 == 1) ? 1 : 0;
                int y = (dir == 0) ? row : dim - 1 - row;
                if (bitset_get(blocked, y, x))
                    continue;
                if ((data[byte] & (1 << bit)) > 0)
                    bitset_set(code, y, x);
                bit--;
                if (bit < 0) {
                    bit = 7;
                    byte++;
                }
                if (byte == data_len)
                    return;
            }
        }
    }
    // we ignore remainder bits because they've already been zeroed by default
}

void apply_mask(bitset_t* code, int dim, bitset_t* blocked, int mask_i) {
    for (int y = 0; y < dim; y++) {
        for (int x = 0; x < dim; x++) {
            if (!bitset_get(blocked, y, x)) {
                if (masks[mask_i](y, x) > 0) {
                    bitset_negate(code, y, x);
                }
            }
        }
    }
}

void draw_format_info(bitset_t* code, int dim, int mask_i, enum corr_level_t corr_level) {
    int corr_level_i;
    switch (corr_level) {
        case CORR_L:
            corr_level_i = 0b01;
            break;
        case CORR_M:
            corr_level_i = 0b00;
            break;
        case CORR_Q:
            corr_level_i = 0b11;
            break;
        default:
            corr_level_i = 0b10;
    }
    int info_code = (corr_level_i << 3) | mask_i;
    int rem = info_code, gen_poly = 0b0000010100110111;
    for (int i = 0; i < 10; i++) {
        rem = (rem << 1) ^ ((rem >> 9) * gen_poly);
    }
    info_code = (info_code << 10 | rem) ^ 0b0101010000010010;

    for (int y = 0; y < 6; y++) {
        if ((info_code & (1 << y)) > 0)
            bitset_set(code, y, 8);
        else
            bitset_unset(code, y, 8);
    }

    if ((info_code & (1 << 6)) > 0)
        bitset_set(code, 7, 8);
    else
        bitset_unset(code, 7, 8);
    if ((info_code & (1 << 7)) > 0)
        bitset_set(code, 8, 8);
    else
        bitset_unset(code, 8, 8);
    if ((info_code & (1 << 8)) > 0)
        bitset_set(code, 8, 7);
    else
        bitset_unset(code, 8, 7);

    for (int x = 9; x < 15; x++) {
        if ((info_code & (1 << x)) > 0)
            bitset_set(code, 8, 14 - x);
        else
            bitset_unset(code, 8, 14 - x);
    }
    for (int x = 0; x < 8; x++) {
        if ((info_code & (1 << x)) > 0)
            bitset_set(code, 8, dim - 1 - x);
        else
            bitset_unset(code, 8, dim - 1 - x);
    }
    for (int y = 0; y < 7; y++) {
        if ((info_code & (1 << (14 - y))) > 0)
            bitset_set(code, dim - 1 - y, 8);
        else
            bitset_unset(code, dim - 1 - y, 8);
    }
}

int check_finder_pattern_ver(bitset_t* code, int y, int x, int r, int dir) {
    int pos = y;
    for (int i = 0; i < r; i++) {
        if (!bitset_get(code, pos, x))
            return 0;
        pos += dir;
    }
    for (int i = 0; i < r; i++) {
        if (bitset_get(code, pos, x))
            return 0;
        pos += dir;
    }
    for (int i = 0; i < 3 * r; i++) {
        if (!bitset_get(code, pos, x))
            return 0;
        pos += dir;
    }
    for (int i = 0; i < r; i++) {
        if (bitset_get(code, pos, x))
            return 0;
        pos += dir;
    }
    for (int i = 0; i < r; i++) {
        if (!bitset_get(code, pos, x))
            return 0;
        pos += dir;
    }
    return 1;
}

int check_finder_pattern_hor(bitset_t* code, int y, int x, int r, int dir) {
    int pos = x;
    for (int i = 0; i < r; i++) {
        if (!bitset_get(code, y, pos))
            return 0;
        pos += dir;
    }
    for (int i = 0; i < r; i++) {
        if (bitset_get(code, y, pos))
            return 0;
        pos += dir;
    }
    for (int i = 0; i < 3 * r; i++) {
        if (!bitset_get(code, y, pos))
            return 0;
        pos += dir;
    }
    for (int i = 0; i < r; i++) {
        if (bitset_get(code, y, pos))
            return 0;
        pos += dir;
    }
    for (int i = 0; i < r; i++) {
        if (!bitset_get(code, y, pos))
            return 0;
        pos += dir;
    }
    return 1;
}

int get_penalty(bitset_t* code, int dim) {
    int penalty = 0;
    // monochromatic streaks of >= 5 modules
    for (int y = 0; y < dim; y++) {
        int cur_streak = 1, prev_color = bitset_get(code, y, 0);
        for (int x = 1; x < dim; x++) {
            int cur_color = bitset_get(code, y, x);
            if (cur_color == prev_color)
                cur_streak++;
            else {
                if (cur_streak >= 5) {
                    penalty += 3 + (cur_streak - 5);
                }
                cur_streak = 1;
            }
            prev_color = cur_color;
        }
        if (cur_streak >= 5) {
            penalty += 3 + (cur_streak - 5);
        }
    }
    for (int x = 0; x < dim; x++) {
        int cur_streak = 1, prev_color = bitset_get(code, 0, x);
        for (int y = 1; y < dim; y++) {
            int cur_color = bitset_get(code, y, x);
            if (cur_color == prev_color)
                cur_streak++;
            else {
                if (cur_streak >= 5) {
                    penalty += 3 + (cur_streak - 5);
                }
                cur_streak = 1;
            }
            prev_color = cur_color;
        }
        if (cur_streak >= 5) {
            penalty += 3 + (cur_streak - 5);
        }
    }

    // 2x2 monochromatic blocks
    for (int y = 0; y < dim - 1; y++) {
        for (int x = 0; x < dim - 1; x++) {
            if (bitset_get(code, y, x) == bitset_get(code, y + 1, x) &&
                bitset_get(code, y + 1, x) == bitset_get(code, y, x + 1) &&
                bitset_get(code, y, x + 1) == bitset_get(code, y + 1, x + 1))
                penalty += 3;
        }
    }

    // 1:1:3:1:1 pattern preceded/followed by 4 light modules
    for (int y = 0; y < dim; y++) {
        for (int x = 0; x < dim; x++) {
            if (y >= 4 && !(bitset_get(code, y, x) | bitset_get(code, y - 1, x) | bitset_get(code, y - 2, x) |
                            bitset_get(code, y - 3, x))) {
                int r = 1;
                while (y - 3 - 7 * r >= 0) {
                    if (check_finder_pattern_ver(code, y - 4, x, r, -1)) {
                        penalty += 40;
                        break;
                    }
                    r++;
                }
            }
            if (y + 4 < dim && !(bitset_get(code, y, x) | bitset_get(code, y + 1, x) | bitset_get(code, y + 2, x) |
                                 bitset_get(code, y + 3, x))) {
                int r = 1;
                while (y + 3 + 7 * r < dim) {
                    if (check_finder_pattern_ver(code, y + 4, x, r, 1)) {
                        penalty += 40;
                        break;
                    }
                    r++;
                }
            }
            if (x + 4 < dim && !(bitset_get(code, y, x) | bitset_get(code, y, x + 1) | bitset_get(code, y, x + 2) |
                                 bitset_get(code, y, x + 3))) {
                int r = 1;
                while (x + 3 + 7 * r < dim) {
                    if (check_finder_pattern_hor(code, y, x + 4, r, 1)) {
                        penalty += 40;
                        break;
                    }
                    r++;
                }
            }
            if (x >= 4 && !(bitset_get(code, y, x) | bitset_get(code, y, x - 1) | bitset_get(code, y, x - 2) |
                            bitset_get(code, y, x - 3))) {
                int r = 1;
                while (x - 3 - 7 * r >= 0) {
                    if (check_finder_pattern_hor(code, y, x - 4, r, -1)) {
                        penalty += 40;
                        break;
                    }
                    r++;
                }
            }
        }
    }

    // proportion of dark modules
    int dark_count = 0;
    for (int y = 0; y < dim; y++) {
        for (int x = 0; x < dim; x++)
            dark_count += bitset_get(code, y, x);
    }
    int proportion = (int)((double)dark_count / (dim * dim) * 100);
    penalty += 10 * abs(proportion - 50) / 5;

    return penalty;
}

int main(int argc, char** argv) {
    int c, parse_err = 0, ppm = 20;
    char* input_file = NULL;
    char* output_file = NULL;
    enum corr_level_t corr_level = CORR_L;
    while ((c = getopt(argc, argv, "i:o:p:lmqh")) != -1) {
        switch (c) {
            case 'i':
                input_file = optarg;
                break;
            case 'o':
                output_file = optarg;
                break;
            case 'p':
                ppm = atoi(optarg);
                break;
            case 'l':
                corr_level = CORR_L;
                break;
            case 'm':
                corr_level = CORR_M;
                break;
            case 'q':
                corr_level = CORR_Q;
                break;
            case 'h':
                corr_level = CORR_H;
                break;
            case ':':
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                parse_err = 1;
                break;
            case '?':
            default:
                parse_err = 1;
        }
    }
    if (parse_err) {
        fprintf(stderr, "%s\n", USAGE_STR);
        return EXIT_FAILURE;
    }
    if (ppm <= 0) {
        fprintf(stderr, "Pixels-per-module (ppm) must be a positive integer.\n");
        return EXIT_FAILURE;
    }

    char data[MAX_CAPACITY];
    memset(data, 0, MAX_CAPACITY * sizeof(char));
    FILE* in_stream = stdin;
    if (input_file != NULL) {
        in_stream = fopen(input_file, "r");
        if (in_stream == NULL) {
            fprintf(stderr, "Unable to open file `%s` for reading.\n", input_file);
            return EXIT_FAILURE;
        }
    }
    if (fread(data, sizeof(char), MAX_CAPACITY, in_stream) == 0) {
        fprintf(stderr, "No data provided for the QR code.\n");
        return EXIT_FAILURE;
    }
    if (fclose(in_stream))
        ERR_AND_DIE("fclose");

    int data_len = strlen(data);
    int version = 1;
    while (version <= 40 && CAPACITY[(int)corr_level][version] < data_len)
        version++;
    if (version > 40) {
        fprintf(stderr, "Input is too long to be stored in a QR code with the specified error correction level.\n");
        return EXIT_FAILURE;
    }

    int dim = 4 * version + 17;
    uint8_t values[TOTAL_AVAILABLE_MODULES[version] / 8 + 1];
    bitstream_t bitstream = {.len_bytes = 0, .len_bits = 0, .values = values};
    fill_data(&bitstream, data, data_len, corr_level, version);

    int n_codewords = TOTAL_DATA_CODEWORDS[(int)corr_level][version] +
                      TOTAL_BLOCKS[(int)corr_level][version] * CORR_CODEWORDS_PER_BLOCK[(int)corr_level][version];
    uint8_t final_codewords[n_codewords];
    add_error_correction_and_interleave(&bitstream, corr_level, version, final_codewords);
    bitset_t code, blocked;
    if (bitset_init(&code, dim, dim) == -1)
        ERR_AND_DIE("bitset_init");
    if (bitset_init(&blocked, dim, dim) == -1)
        ERR_AND_DIE("bitset_init");
    draw_functional_patterns(&code, version, dim, &blocked);
    draw_data(&code, final_codewords, n_codewords, dim, &blocked);

    int best_mask_i = 0, min_penalty = INT_MAX;
    for (int mask_i = 0; mask_i < 8; mask_i++) {
        apply_mask(&code, dim, &blocked, mask_i);
        draw_format_info(&code, dim, mask_i, corr_level);
        int penalty = get_penalty(&code, dim);
        apply_mask(&code, dim, &blocked, mask_i);
        if (penalty < min_penalty) {
            best_mask_i = mask_i;
            min_penalty = penalty;
        }
    }
    apply_mask(&code, dim, &blocked, best_mask_i);
    draw_format_info(&code, dim, best_mask_i, corr_level);

    FILE* out_stream = stdout;
    if (output_file != NULL) {
        out_stream = fopen(output_file, "w");
        if (out_stream == NULL) {
            fprintf(stderr, "Unable to open file `%s` for writing.\n", output_file);
            return EXIT_FAILURE;
        }
    }
    save_as_png(&code, ppm, dim / 5, out_stream);
    bitset_free(&code);
    bitset_free(&blocked);
    if (fclose(out_stream))
        ERR_AND_DIE("fclose");

    return EXIT_SUCCESS;
}
