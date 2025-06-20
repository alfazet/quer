#include <stdint.h>
#include <string.h>
#include <time.h>

#include "bitset.h"

#define ERR_AND_DIE(...) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), fprintf(stderr, __VA_ARGS__), exit(EXIT_FAILURE))

typedef struct rgb_color_t {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_color_t;

void save_as_ppm(bitset_t* code, rgb_color_t* color, int module_size, char* filename) {
    FILE* file = fopen(filename, "w");
    if (file == NULL)
        ERR_AND_DIE("fopen");

    int width = code->width * module_size;
    int height = code->height * module_size;
    if (fprintf(file, "P6 %d %d 255\n", width, height) < 0)
        ERR_AND_DIE("fprintf");

    uint8_t buf[3 * width * height];
    memset(buf, 255, 3 * width * height * sizeof(uint8_t));
    buf[0] = 128;
    buf[1] = 128;
    buf[2] = 128;
    for (int y = 0; y < height; y += module_size) {
        for (int x = 0; x < width; x += module_size) {
            if (bitset_get(code, y / module_size, x / module_size) > 0) {
                for (int i = 0; i < module_size; i++) {
                    for (int j = 0; j < 3 * module_size; j += 3) {
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
    if (fclose(file))
        ERR_AND_DIE("fclose");
}

int main(void) {
    srand(time(NULL));

    int w = 29, h = 29;
    bitset_t data;
    bitset_init(&data, w, h);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (rand() % 2 == 0)
                bitset_set(&data, y, x);
        }
    }
    rgb_color_t color = {.r = 0, .g = 0, .b = 255};
    save_as_ppm(&data, &color, 20, "colored.ppm");
    bitset_free(&data);
    return 0;
}
