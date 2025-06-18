#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ERR_AND_DIE(...) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), fprintf(stderr, __VA_ARGS__), exit(EXIT_FAILURE))

void save_as_ppm(uint16_t** image, int width, int height, char* filename) {
    FILE* file = fopen(filename, "w");
    if (file == NULL)
        ERR_AND_DIE("fopen");
    if (fprintf(file, "P6 %d %d 255\n", 4 * width, 4 * height) < 0)
        ERR_AND_DIE("fprintf");
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            for (int i = 0; i < 16; i++) {
                unsigned char rgb[3];
                memset(rgb, (image[y][x] & (1 << i)) > 0 ? 255 : 0, sizeof(rgb));
                if (fwrite(rgb, sizeof(rgb), 1, file) == 0)
                    ERR_AND_DIE("fwrite");
            }
        }
    }
    if (fclose(file))
        ERR_AND_DIE("fclose");
}

int main(int argc, char** argv) {
    srand(time(NULL));

    // has to be divisible by 4!
    int target_width = 1024, target_height = 1024;
    int width = target_width / 4, height = target_height / 4;
    uint16_t** image = malloc(height * sizeof(uint16_t*));
    if (image == NULL)
        ERR_AND_DIE("malloc");
    for (int y = 0; y < height; y++) {
        image[y] = malloc(width * sizeof(int));
        if (image[y] == NULL)
            ERR_AND_DIE("malloc");
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int r = rand();
            image[y][x] = (uint16_t)(r & ((1 << 16) - 1));
        }
    }
    save_as_ppm(image, width, height, "./ppm_test.ppm");
    for (int y = 0; y < height; y++)
        free(image[y]);
    free(image);

    return 0;
}
