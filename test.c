#include <stdio.h>
#include <stdlib.h>

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
    apply_filter_simd(img, output_img, width, height);

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