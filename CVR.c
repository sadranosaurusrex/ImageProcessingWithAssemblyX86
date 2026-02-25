#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <dirent.h> 

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Assembly function declaration
// RDI: src, RSI: dest, RDX: width, RCX: height
extern void apply_filter_simd(unsigned char* src, unsigned char* dest, int width, int height);

// Standard C implementation for Speedup comparison
void apply_filter_c(unsigned char* src, unsigned char* dest, int width, int height) {
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            // Laplacian-like edge detection filter
            int sum = (src[(y-1)*width + x] + src[(y+1)*width + x] + 
                       src[y*width + (x-1)] + src[y*width + (x+1)]) - 4 * src[y*width + x];
            
            if (sum < 0) sum = -sum; // Absolute value
            
            // Thresholding at 31 to match the Assembly logic
            dest[y*width + x] = (sum > 31) ? 255 : 0;
        }
    }
}

int main() {
    int grid = 10;          // 10x10 mosaic
    int tile_size = 28;     // MNIST standard size
    int big_w = grid * tile_size; // 280px
    int big_h = grid * tile_size; // 280px

    // Allocate memory for the mosaic (Input and Output)
    unsigned char *big_img = (unsigned char *)calloc(big_w * big_h, 1);
    unsigned char *big_output_asm = (unsigned char *)calloc(big_w * big_h, 1);
    unsigned char *big_output_c = (unsigned char *)calloc(big_w * big_h, 1);

    if (!big_img || !big_output_asm || !big_output_c) {
        printf("Memory allocation failed!\n");
        return 1;
    }

    // 1. Scan directory and build the mosaic from first 100 PNGs
    DIR *d = opendir("./mnist_png/0");
    if (!d) {
        printf("Error: Directory ./mnist_png/0 not found!\n");
        return 1;
    }

    struct dirent *dir;
    int count = 0;
    printf("Loading .png files into mosaic buffer...\n");

    while ((dir = readdir(d)) != NULL && count < 100) {
        if (strstr(dir->d_name, ".png")) {
            char path[256];
            sprintf(path, "./mnist_png/0/%s", dir->d_name);
            
            int w, h, c;
            unsigned char *small = stbi_load(path, &w, &h, &c, 1);
            if (small) {
                int row = count / grid;
                int col = count % grid;
                for (int y = 0; y < tile_size; y++) {
                    memcpy(&big_img[(row * tile_size + y) * big_w + (col * tile_size)], 
                           &small[y * tile_size], tile_size);
                }
                stbi_image_free(small);
                count++;
            }
        }
    }
    closedir(d);
    printf("%d images successfully placed in mosaic.\n", count);

    struct timespec start, end;

    // 2. Performance Benchmark: C Implementation
    clock_gettime(CLOCK_MONOTONIC, &start);
    apply_filter_c(big_img, big_output_c, big_w, big_h);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_c = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    // 3. Performance Benchmark: Assembly (SIMD/AVX2) Implementation
    clock_gettime(CLOCK_MONOTONIC, &start);
    apply_filter_simd(big_img, big_output_asm, big_w, big_h);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_asm = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    // 4. Final Performance Report (For your documentation)
    printf("\n========= Phase 2 Performance Report =========\n");
    printf("C Execution Time:      %.3f ms\n", time_c * 1000);
    printf("Assembly (AVX2) Time:  %.3f ms\n", time_asm * 1000);
    printf("Calculated Speedup:    %.2f x faster\n", time_c / time_asm);
    printf("Detection Accuracy:    Features extracted for %d digits.\n", count);
    printf("==============================================\n");

    // 5. Save the final result to the Root Folder
    if (stbi_write_jpg("mnist_detected_mosaic.jpg", big_w, big_h, 1, big_output_asm, 100)) {
        printf("Result saved as 'mnist_detected_mosaic.jpg' in the root folder.\n");
    } else {
        printf("Error saving output image!\n");
    }

    // Cleanup
    free(big_img); 
    free(big_output_asm); 
    free(big_output_c);
    
    return 0;
}