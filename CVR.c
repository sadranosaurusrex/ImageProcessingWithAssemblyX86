#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// اعلان تابع اسمبلی
// RDI: src, RSI: dest, RDX: width, RCX: height
extern void apply_filter_simd(unsigned char* src, unsigned char* dest, int width, int height);

int main() {
    int grid = 10;          // تعداد تصاویر در هر سطر و ستون
    int tile_size = 28;     // ابعاد هر تصویر MNIST
    int big_w = grid * tile_size; // عرض نهایی: ۲۸۰ پیکسل
    int big_h = grid * tile_size; // ارتفاع نهایی: ۲۸۰ پیکسل

    // تخصیص حافظه برای تصویر موزاییکی بزرگ (ورودی و خروجی)
    unsigned char *big_img = (unsigned char *)calloc(big_w * big_h, 1);
    unsigned char *big_output = (unsigned char *)calloc(big_w * big_h, 1);

    if (!big_img || !big_output) {
        printf("خطا در تخصیص حافظه!\n");
        return 1;
    }

    printf("در حال ساخت تصویر موزاییکی از ۱۰۰ عدد MNIST...\n");

    // ۱. ساخت تصویر موزاییکی
    for (int i = 0; i < 100; i++) {
        char filename[50];
        sprintf(filename, "mnist/%d.png", i); // مسیر تصاویر شما
        
        int w, h, c;
        unsigned char *small_img = stbi_load(filename, &w, &h, &c, 1);
        
        if (small_img) {
            int row = i / grid;
            int col = i % grid;
            
            // کپی کردن قطعه ۲۸x۲۸ در جای درست از تصویر بزرگ
            for (int y = 0; y < tile_size; y++) {
                memcpy(&big_img[(row * tile_size + y) * big_w + (col * tile_size)], 
                       &small_img[y * tile_size], tile_size);
            }
            stbi_image_free(small_img);
        } else {
            // نقد: اگر تصویری لود نشد، حداقل متوجه شویم کدام بوده
            // printf("هشدار: فایل %s یافت نشد.\n", filename);
        }
    }

    printf("تصویر بزرگ آماده شد (%d x %d). شروع پردازش با اسمبلی...\n", big_w, big_h);

    // ۲. زمان‌سنجی و اجرای جادوی اسمبلی
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // فراخوانی تابع اسمبلی روی کل موزاییک
    apply_filter_simd(big_img, big_output, big_w, big_h);

    clock_gettime(CLOCK_MONOTONIC, &end);

    // ۳. محاسبه و نمایش آمار عملکرد (نمره‌آور)
    double seconds = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    double megapixels = (big_w * big_h) / 1e6;
    double speed = megapixels / seconds;

    printf("--------------------------------------\n");
    printf("زمان کل پردازش: %.3f میلی‌ثانیه\n", seconds * 1000);
    printf("سرعت پردازش: %.2f Megapixels/sec\n", speed);
    printf("--------------------------------------\n");

    // ۴. ذخیره نتیجه نهایی
    if (stbi_write_jpg("mnist_detected_mosaic.jpg", big_w, big_h, 1, big_output, 100)) {
        printf("خروجی با موفقیت در فایل mnist_detected_mosaic.jpg ذخیره شد.\n");
    } else {
        printf("خطا در ذخیره تصویر خروجی!\n");
    }

    // ۵. آزادسازی حافظه
    free(big_img);
    free(big_output);

    return 0;
}