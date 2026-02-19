; -----------------------------------------------------------------------------
; This is a SIMD Copy - The "Hello World" of Image Processing
; -----------------------------------------------------------------------------
global fast_copy

section .text
fast_copy:
    ; RDI = Source Address (Input Image)
    ; RSI = Destination Address (Output Image)

    ; Load 32 bytes (256 bits) at once from memory into YMM0
    vmovdqu ymm0, [rdi]
    
    ; Store those 32 bytes from YMM0 into the destination memory
    vmovdqu [rsi], ymm0

    vzeroupper    ; Clean up state (Good practice for AVX)
    ret