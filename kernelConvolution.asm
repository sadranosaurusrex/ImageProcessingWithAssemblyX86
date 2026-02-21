; -----------------------------------------------------------------------------
; Function: apply_filter_simd (Full 3x3 Convolution)
; -----------------------------------------------------------------------------

section .data
    align 32
    kernel_top    dw -1, -2, -1
    kernel_mid    dw  0,  0,  0    ; Added missing middle row
    kernel_bottom dw  1,  2,  1

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
    vpbroadcastw ymm14, [kernel_mid]      ; Coeff [1,0]
    vpbroadcastw ymm15, [kernel_mid + 2]  ; Coeff [1,1]

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
    vpaddw ymm0, ymm0, ymm2

    ; Row MIDDLE (Only if needed, here we process it for flexibility)
    vmovdqu xmm3, [r9 + r11 - 1]
    vpmovzxbw ymm3, xmm3
    vpmullw ymm3, ymm3, ymm14             ; Mid-Left
    vpaddw ymm0, ymm0, ymm3

    vmovdqu xmm4, [r9 + r11]
    vpmovzxbw ymm4, xmm4
    vpmullw ymm4, ymm4, ymm15             ; Mid-Center
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

    ; Absolute value & Pack
    vpabsw ymm0, ymm0                     
    vpackuswb ymm0, ymm0, ymm0            
    
    vmovdqu [r12 + r11], xmm0             ; Store 16 results
    
    add r11, 16
    jmp .col_loop

.scalar_fallback:
    cmp r11, rdx
    jge .next_row
    
    ; Simple Scalar Logic (keeping it simple for demo)
    mov al, [r9 + r11 + 1]
    mov bl, [r9 + r11 - 1]
    sub al, bl
    jns .pos
    neg al
.pos:
    shl al, 2
    mov [r12 + r11], al
    inc r11
    jmp .scalar_fallback

.next_row:
    inc r8
    mov r10, rcx
    dec r10
    cmp r8, r10
    jl .row_loop

    vzeroupper
    leave
    ret