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
extern void apply_filter_simd(unsigned char* src, unsigned char* dest, int width, int height);

// Standard C implementation for Speedup comparison
void apply_filter_c(unsigned char* src, unsigned char* dest, int width, int height) {
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int sum = (src[(y-1)*width + x] + src[(y+1)*width + x] + 
                       src[y*width + (x-1)] + src[y*width + (x+1)]) - 4 * src[y*width + x];
            if (sum < 0) sum = -sum;
            dest[y*width + x] = (sum > 31) ? 255 : 0;
        }
    }
}
// Geometric Analysis for Digit Recognition (Improved with Template Matching)
int recognize_digit_advanced(unsigned char* tile, int stride) {
    int tile_size = 28;
    int min_x = 28, max_x = 0, min_y = 28, max_y = 0;
    int total_pixels = 0;

    // 1. استخراج محدوده (Bounding Box)
    for (int y = 0; y < tile_size; y++) {
        for (int x = 0; x < tile_size; x++) {
            if (tile[y * stride + x] == 255) {
                if (x < min_x) min_x = x; 
                if (x > max_x) max_x = x;
                if (y < min_y) min_y = y; 
                if (y > max_y) max_y = y;
                total_pixels++;
            }
        }
    }
    
    if (total_pixels < 10) return -1; // نویز یا تصویر خالی

    int width = max_x - min_x + 1;
    int height = max_y - min_y + 1;
    float aspect_ratio = (float)width / (float)height;

    // فیلتر سریع برای عدد 1 به دلیل ساختار بسیار باریک آن
    if (aspect_ratio < 0.40f) return 1;

    // 2. محاسبه تراکم نرمال‌شده در شبکه 3x3
    float s[9] = {0}; // تبدیل به آرایه یک بعدی برای پردازش راحت‌تر
    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            if (tile[y * stride + x] == 255) {
                int r = ((y - min_y) * 3) / (height + 1);
                int c = ((x - min_x) * 3) / (width + 1);
                if (r >= 0 && r < 3 && c >= 0 && c < 3) {
                    s[r * 3 + c]++;
                }
            }
        }
    }
    
    // نرمال‌سازی: مجموع مقادیر این ۹ خانه برابر با 1.0 خواهد بود
    for (int i = 0; i < 9; i++) {
        s[i] /= (float)total_pixels;
    }

    // 3. موتور تصمیم‌گیری بر اساس تطابق الگو (Mean Absolute Error)
    // هر ردیف نشان‌دهنده توزیع تراکم ایده‌آل یک عدد در 9 خانه (3x3) است.
    // این مقادیر تقریبی هستند و بر اساس ریخت‌شناسی اعداد تنظیم شده‌اند.
    const float templates[10][9] = {
        {0.15, 0.15, 0.15,  0.15, 0.00, 0.15,  0.15, 0.15, 0.15}, // 0: مرکز خالی
        {0.05, 0.25, 0.05,  0.05, 0.30, 0.05,  0.05, 0.25, 0.05}, // 1: ستون وسط پر
        {0.15, 0.15, 0.15,  0.05, 0.15, 0.00,  0.20, 0.15, 0.00}, // 2: بالا پر، وسط مورب، پایین چپ
        {0.10, 0.15, 0.15,  0.05, 0.15, 0.15,  0.10, 0.15, 0.15}, // 3: سمت راست پر، چپ خالی
        {0.15, 0.05, 0.10,  0.15, 0.15, 0.15,  0.00, 0.05, 0.15}, // 4: بالا چپ، وسط پر، پایین راست
        {0.15, 0.15, 0.10,  0.15, 0.15, 0.00,  0.05, 0.15, 0.15}, // 5: بالا چپ، وسط، پایین راست
        {0.10, 0.10, 0.05,  0.15, 0.15, 0.15,  0.15, 0.15, 0.15}, // 6: بالا چپ، حلقه پایین
        {0.20, 0.20, 0.20,  0.00, 0.05, 0.15,  0.00, 0.10, 0.00}, // 7: بالا پر، خط مورب به راست
        {0.10, 0.15, 0.10,  0.15, 0.10, 0.15,  0.10, 0.15, 0.10}, // 8: متعادل، مرکز کمی خالی‌تر
        {0.15, 0.15, 0.15,  0.15, 0.15, 0.15,  0.00, 0.05, 0.10}  // 9: حلقه بالا، خط پایین راست
    };

    int best_match = -1;
    float min_error = 999.0f; // یک عدد بزرگ برای شروع

    for (int digit = 0; digit < 10; digit++) {
        float error = 0.0f;
        for (int i = 0; i < 9; i++) {
            float diff = s[i] - templates[digit][i];
            error += (diff > 0) ? diff : -diff; // قدر مطلق (Absolute Difference)
        }

        if (error < min_error) {
            min_error = error;
            best_match = digit;
        }
    }

    // Fallback: رفع ابهام بین صفر و هشت در صورت توخالی بودن شدید
    if ((best_match == 0 || best_match == 8) && s[4] > 0.12f) {
        return 8; // اگر مرکز بیش از حد پر بود، هشت است نه صفر
    }

    return best_match;
}

int main() {
    int grid = 10;
    int tile_size = 28;
    int big_w = 280;
    int big_h = 280;
    char input;

    scanf(" %c", &input);

    unsigned char *big_img = (unsigned char *)calloc(big_w * big_h, 1);
    unsigned char *big_output_asm = (unsigned char *)calloc(big_w * big_h, 1);
    unsigned char *big_output_c = (unsigned char *)calloc(big_w * big_h, 1);


    char folder[] = "./mnist_png/ "; // Testing folder
    folder[12] = input;
    const char* target_folder = folder;
    DIR *d = opendir(target_folder);
    if (!d) { printf("Error: Folder %s not found!\n", target_folder); return 1; }

    struct dirent *dir;
    int count = 0;
    while ((dir = readdir(d)) != NULL && count < 100) {
        if (strstr(dir->d_name, ".png")) {
            char path[256];
            sprintf(path, "%s/%s", target_folder, dir->d_name);
            int w, h, c;
            unsigned char *small = stbi_load(path, &w, &h, &c, 1);
            if (small) {
                int row = count / grid, col = count % grid;
                for (int y = 0; y < tile_size; y++)
                    memcpy(&big_img[(row * tile_size + y) * big_w + (col * tile_size)], &small[y * tile_size], tile_size);
                stbi_image_free(small);
                count++;
            }
        }
    }
    closedir(d);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    apply_filter_c(big_img, big_output_c, big_w, big_h);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_c = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    clock_gettime(CLOCK_MONOTONIC, &start);
    apply_filter_simd(big_img, big_output_asm, big_w, big_h);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_asm = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    // Print Results Matrix
    printf("\n<<<  DETECTION MATRIX (10x10)  >>>\n");
    int results[100];
    for (int i = 0; i < 100; i++) {
        results[i] = recognize_digit_advanced(&big_output_asm[(i/10*28*280) + (i%10*28)], 280);
    }
    for (int r = 0; r < 10; r++) {
        printf("Row %d |", r);
        for (int c = 0; c < 10; c++) {
            int v = results[r*10+c];
            if(v == -1) printf(" [?] "); else printf(" [%d] ", v);
        }
        printf("\n");
    }

    printf("\n========= PERFORMANCE REPORT =========\n");
    printf("C Time: %.3f ms | ASM Time: %.3f ms\n", time_c*1000, time_asm*1000);
    printf("Speedup: %.2f x\n", time_c / time_asm);
    
    char filename[] = "mnist_detected_mosaic_ .jpg";
    filename[22] = input;
    stbi_write_jpg(filename, big_w, big_h, 1, big_output_asm, 100);
    free(big_img); free(big_output_asm); free(big_output_c);
    return 0;
}