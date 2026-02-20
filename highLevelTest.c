// #define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Define implementation before including (required by stb_image)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void apply_filter_c(unsigned char *src, unsigned char *dest, int width, int height) {
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int offset = y * width + x;
            
            int right = src[offset + 1];
            int left  = src[offset - 1];
            
            // همان منطق قدر مطلق که در اسمبلی زدیم
            int diff = abs(right - left);
            
            // ضربدر ۴ و اشباع روی ۲۵۵
            int res = diff * 4;
            if (res > 255) res = 255;
            
            dest[offset] = (unsigned char)res;
        }
    }
}

// Your assembly function declaration
// RDI: src, RSI: dest, RDX: width, RCX: height
extern void apply_filter_simd(unsigned char* src, unsigned char* dest, int width, int height);

int main() {
    int width, height, channels;
    int blurCount = 1000;
    
    // for (int i = 0; i < blurCount; i++ )
    // {
    // 1. Load the image (Forcing 1 channel/Grayscale for simplicity)
    unsigned char *img = stbi_load("output.jpeg", &width, &height, &channels, 1);
    if (img == NULL) {
        printf("Error: Could not load image.\n");
        return 1;
    }

    // 2. Allocate memory for output image
    unsigned char *output_img = (unsigned char *)malloc(width * height);

    printf("Image Loaded: %d x %d\n", width, height);

    // 3. Call your Assembly function
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // اجرای جادوی اسمبلی
    apply_filter_c(img, output_img, width, height);

    clock_gettime(CLOCK_MONOTONIC, &end);

    // محاسبه زمان با دقت بالا
    double seconds = (end.tv_sec - start.tv_sec);
    double pixels_per_second = (width * height) / (seconds + (end.tv_nsec - start.tv_nsec) / 1e9);

    printf("Time: %f ms\n", (seconds * 1000) + (end.tv_nsec - start.tv_nsec) / 1e6);
    printf("Speed: %.2f Megapixels/sec\n", pixels_per_second / 1e6);

    // 4. Save the result
    // if (!(i % (blurCount/50))){
    stbi_write_jpg("output.jpeg", width, height, 1, output_img, 100);
    // }

    // 5. Cleanup
    stbi_image_free(img);
    free(output_img);
    // }
    printf("Processing complete. Check output.jpg\n");
    return 0;
}