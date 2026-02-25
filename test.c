#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Define implementation before including (required by stb_image)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Your assembly function declaration
// RDI: src, RSI: dest, RDX: width, RCX: height
extern void apply_filter_simd(unsigned char* src, unsigned char* dest, int width, int height);

int main() {
    int width, height, channels;
    int blurCount = 1000;
    
    // for (int i = 0; i < blurCount; i++ )
    // {int grid = 10; // ۱۰ در ۱۰
    int tile_size = 28;
    int big_w = grid * tile_size; // ۲۸۰
    int big_h = grid * tile_size; // ۲۸۰

    unsigned char *big_img = (unsigned char *)calloc(big_w * big_h, 1);
    unsigned char *big_output = (unsigned char *)calloc(big_w * big_h, 1);

    for (int i = 0; i < 100; i++) {
        char filename[30];
        sprintf(filename, "mnist/%d.png", i); // فرض بر این است که نام فایل‌ها ۱ تا ۱۰۰ است
        int w, h, c;
        unsigned char *small_img = stbi_load(filename, &w, &h, &c, 1);
        
        if (small_img) {
            int row = i / grid;
            int col = i % grid;
            
            for (int y = 0; y < tile_size; y++) {
                memcpy(&big_img[(row * tile_size + y) * big_w + (col * tile_size)], 
                    &small_img[y * tile_size], tile_size);
            }
            stbi_image_free(small_img);
        }
    }

    // 2. Allocate memory for output image
    unsigned char *output_img = (unsigned char *)malloc(width * height);

    printf("Image Loaded: %d x %d\n", width, height);

    // 3. Call your Assembly function
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // اجرای جادوی اسمبلی
    apply_filter_simd(img, output_img, width, height);

    clock_gettime(CLOCK_MONOTONIC, &end);

    // محاسبه زمان با دقت بالا
    double seconds = (end.tv_sec - start.tv_sec);
    double pixels_per_second = (width * height) / (seconds + (end.tv_nsec - start.tv_nsec) / 1e9);

    printf("Time: %f ms\n", (seconds * 1000) + (end.tv_nsec - start.tv_nsec) / 1e6);
    printf("Speed: %.2f Megapixels/sec\n", pixels_per_second / 1e6);

    // 4. Save the result
    // if (!(i % (blurCount/50))){
    stbi_write_jpg("download - Copy.jpeg", width, height, 1, output_img, 100);
    // }

    // 5. Cleanup
    stbi_image_free(img);
    free(output_img);
    // }
    printf("Processing complete. Check output.jpg\n");
    return 0;
}