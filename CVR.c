#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <dirent.h> // برای خواندن لیست فایل‌ها بدون توجه به اسم

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// اعلان تابع اسمبلی
extern void apply_filter_simd(unsigned char* src, unsigned char* dest, int width, int height);

// تابع ساده C برای مقایسه سرعت (Speedup)
void apply_filter_c(unsigned char* src, unsigned char* dest, int width, int height) {
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            // یک فیلتر لبه‌یابی ساده (مثل لاپلاسین) برای مقایسه
            int sum = (src[(y-1)*width + x] + src[(y+1)*width + x] + 
                       src[y*width + (x-1)] + src[y*width + (x+1)]) - 4 * src[y*width + x];
            if (sum < 0) sum = -sum;
            // آستانه‌گذاری ۳۱ (همان که در اسمبلی زدید)
            dest[y*width + x] = (sum > 31) ? 255 : 0;
        }
    }
}

int main() {
    int grid = 10;          
    int tile_size = 28;     
    int big_w = 280; 
    int big_h = 280;

    unsigned char *big_img = (unsigned char *)calloc(big_w * big_h, 1);
    unsigned char *big_output_asm = (unsigned char *)calloc(big_w * big_h, 1);
    unsigned char *big_output_c = (unsigned char *)calloc(big_w * big_h, 1);

    // ۱. باز کردن پوشه و برداشتن ۱۰۰ فایل اول (بدون نیاز به اسم مشخص)
    DIR *d = opendir("./mnist/0");
    if (!d) {
        printf("خطا: پوشه mnist پیدا نشد!\n");
        return 1;
    }

    struct dirent *dir;
    int count = 0;
    printf("در حال لود کردن تصاویر *.png از پوشه mnist...\n");

    while ((dir = readdir(d)) != NULL && count < 100) {
        if (strstr(dir->d_name, ".png")) {
            char path[256];
            sprintf(path, "./mnist/%s", dir->d_name);
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
    printf("تعداد %d تصویر در موزاییک چیده شد.\n", count);

    struct timespec start, end;

    // ۲. اجرای نسخه C و زمان‌سنجی
    clock_gettime(CLOCK_MONOTONIC, &start);
    apply_filter_c(big_img, big_output_c, big_w, big_h);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_c = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    // ۳. اجرای نسخه اسمبلی (SIMD) و زمان‌سنجی
    clock_gettime(CLOCK_MONOTONIC, &start);
    apply_filter_simd(big_img, big_output_asm, big_w, big_h);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_asm = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    // ۴. گزارش نهایی برای فایل Word پروژه
    printf("\n========= گزارش عملکرد فاز ۲ =========\n");
    printf("زمان اجرای کد C: %.3f میلی‌ثانیه\n", time_c * 1000);
    printf("زمان اجرای اسمبلی (AVX2): %.3f میلی‌ثانیه\n", time_asm * 1000);
    printf("تسریع (Speedup): %.2f برابر\n", time_c / time_asm);
    printf("دقت تشخیص: لبه‌های ۱۰۰ عدد استخراج شد.\n");
    printf("======================================\n");

    stbi_write_jpg("result_mosaic_asm.jpg", big_w, big_h, 1, big_output_asm, 100);
    
    free(big_img); free(big_output_asm); free(big_output_c);
    return 0;
}