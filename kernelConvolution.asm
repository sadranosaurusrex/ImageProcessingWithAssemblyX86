; -----------------------------------------------------------------------------
; Function: apply_filter_simd (Full 3x3 Convolution)
; -----------------------------------------------------------------------------

section .data
    kernel_top    dw  0, -1,  0
    kernel_mid    dw -1,  4, -1
    kernel_bottom dw  0, -1,  0

    threshold_val: dw 31

global apply_filter_simd

section .text
apply_filter_simd:
    push rbp
    mov rbp, rsp

    ; Load kernel coefficients once
    vpbroadcastw ymm8,  [kernel_top]      
    vpbroadcastw ymm9,  [kernel_top + 2]  
    vpbroadcastw ymm10, [kernel_top + 4]  
    vpbroadcastw ymm11, [kernel_bottom]   
    vpbroadcastw ymm12, [kernel_bottom + 2]
    vpbroadcastw ymm13, [kernel_bottom + 4]
    vpbroadcastw ymm14, [kernel_mid]        
    vpbroadcastw ymm15, [kernel_mid + 2]
    vpbroadcastw ymm7,  [kernel_mid + 4] 

    mov r8, 1                             ; r8 = current_row (y)

.row_loop:
    mov rax, r8
    imul rax, rdx                         ; offset = y * width

    lea r9,  [rdi + rax]                  ; Current input row
    lea r12, [rsi + rax]                  ; Output row
    
    ; FIX: Calculate Above/Below pointers correctly
    mov r13, r9
    sub r13, rdx                          ; r13 = Row ABOVE
    mov r14, r9
    add r14, rdx                          ; r14 = Row BELOW
    
    mov r11, 1                            ; r11 = current_col (x)

.col_loop:
    mov r10, rdx
    sub r10, r11
    cmp r10, 18                           
    jl .scalar_fallback 

    ; --- SIMD BLOCK (16 pixels at once) ---
    ; Row ABOVE
    vmovdqu xmm0, [r13 + r11 - 1]         
    vpmovzxbw ymm0, xmm0                  
    vpmullw ymm0, ymm0, ymm8              ; Top-Left

    vmovdqu xmm1, [r13 + r11]             
    vpmovzxbw ymm1, xmm1
    vpmullw ymm1, ymm1, ymm9              ; Top-Center
    vpaddw ymm0, ymm0, ymm1

    vmovdqu xmm2, [r13 + r11 + 1]         
    vpmovzxbw ymm2, xmm2
    vpmullw ymm2, ymm2, ymm10             ; Top-Right
    vpaddw ymm0, ymm0, ymm2; Mid-Left

    ; Row MIDDLE
    vmovdqu xmm3, [r9 + r11 - 1]
    vpmovzxbw ymm3, xmm3
    vpmullw ymm3, ymm3, ymm14             ; Mid-Left
    vpaddw ymm0, ymm0, ymm3

    vmovdqu xmm4, [r9 + r11]
    vpmovzxbw ymm4, xmm4
    vpmullw ymm4, ymm4, ymm15             ; Mid-Center
    vpaddw ymm0, ymm0, ymm4

    vmovdqu xmm4, [r9 + r11 + 1]
    vpmovzxbw ymm4, xmm4
    vpmullw ymm4, ymm4, ymm7              ; Mid-Right
    vpaddw ymm0, ymm0, ymm4

    ; Row BELOW
    vmovdqu xmm3, [r14 + r11 - 1]         
    vpmovzxbw ymm3, xmm3
    vpmullw ymm3, ymm3, ymm11             ; Bottom-Left
    vpaddw ymm0, ymm0, ymm3

    vmovdqu xmm4, [r14 + r11]             
    vpmovzxbw ymm4, xmm4
    vpmullw ymm4, ymm4, ymm12             ; Bottom-Center
    vpaddw ymm0, ymm0, ymm4

    vmovdqu xmm5, [r14 + r11 + 1]         
    vpmovzxbw ymm5, xmm5
    vpmullw ymm5, ymm5, ymm13             ; Bottom-Right
    vpaddw ymm0, ymm0, ymm5

    ; Absolute value
    vpabsw ymm0, ymm0
    vpackuswb ymm0, ymm0, ymm0            
    vpermq ymm0, ymm0, 0xD8               ; 00 10 01 11 Means 10 is before 01 

    ; فرض می‌کنیم آستانه ما 127 است (هر چه بیشتر از آن بود سفید شود)
    vpbroadcastb xmm6, [rel threshold_val]; مقدار آستانه را در یک رجیستر قرار دهید
    vpcmpgtb xmm0, xmm0, xmm6             ; مقایسه: اگر پیکسل > آستانه بود، تمام بیت‌ها 1 می‌شوند (255)
                                          ; و اگر نبود تمام بیت‌ها 0 می‌شوند (0)
    
    vmovdqu [r12 + r11], xmm0             ; ذخیره مستقیم نتیجه سیاه و سفید
    
    add r11, 16                           ; Next 16 pixels
    jmp .col_loop

.scalar_fallback:
    ; 1. Check if we have reached the end of the width
    cmp r11, rdx
    jge .next_row

    ; 2. Initialize accumulator in R10 (using R10 as a signed sum)
    xor r10, r10        ; Clear sum

    ; --- Process ROW ABOVE (R13) ---
    ; Top-Left
    movzx ax, byte [r13 + r11 - 1]
    movsx bx, [rel kernel_top]
    imul  ax, bx
    movsx eax, ax       ; Sign extend to 32-bit
    add   r10d, eax

    ; Top-Center
    movzx ax, byte [r13 + r11]
    movsx bx, [rel kernel_top + 2]
    imul  ax, bx
    movsx eax, ax
    add   r10d, eax

    ; Top-Right
    movzx ax, byte [r13 + r11 + 1]
    movsx bx, [rel kernel_top + 4]
    imul  ax, bx
    movsx eax, ax
    add   r10d, eax

    ; --- Process MIDDLE ROW (R9) ---
    ; Mid-Left
    movzx ax, byte [r9 + r11 - 1]
    movsx bx, [rel kernel_mid]
    imul  ax, bx
    movsx eax, ax
    add   r10d, eax

    ; Mid-Center
    movzx ax, byte [r9 + r11]
    movsx bx, [rel kernel_mid + 2]
    imul  ax, bx
    movsx eax, ax
    add   r10d, eax

    ; Mid-Right
    movzx ax, byte [r9 + r11 + 1]
    movsx bx, [rel kernel_mid + 4]
    imul  ax, bx
    movsx eax, ax
    add   r10d, eax

    ; --- Process ROW BELOW (R14) ---
    ; Bottom-Left
    movzx ax, byte [r14 + r11 - 1]
    movsx bx, [rel kernel_bottom]
    imul  ax, bx
    movsx eax, ax
    add   r10d, eax

    ; Bottom-Center
    movzx ax, byte [r14 + r11]
    movsx bx, [rel kernel_bottom + 2]
    imul  ax, bx
    movsx eax, ax
    add   r10d, eax

    ; Bottom-Right
    movzx ax, byte [r14 + r11 + 1]
    movsx bx, [rel kernel_bottom + 4]
    imul  ax, bx
    movsx eax, ax
    add   r10d, eax

    ; --- Post-Processing: Absolute Value and Clamping ---
    test r10d, r10d
    jns .is_positive
    neg r10d            ; قدر مطلق
.is_positive:
    ; آستانه‌گذاری ساده
    cmp r10d, threshold_val       ; مقایسه با آستانه 
    ja .make_white      ; اگر بزرگتر بود برو به بخش سفید کردن
    mov r10b, 0         ; سیاه
    jmp .no_clamp
.make_white:
    mov r10b, 255       ; سفید
.no_clamp:
    ; Store the result back to memory
    mov [r12 + r11], r10b

    ; Advance to next pixel
    inc r11
    jmp .scalar_fallback ; Don't let it slip to the next row

.next_row:
    inc r8
    mov r10, rcx
    dec r10
    cmp r8, r10
    jl .row_loop

    vzeroupper
    leave
    ret

section .note.GNU-stack noalloc noexec nowrite progbits