; -----------------------------------------------------------------------------
; Function: apply_filter_simd
; Processes an image using AVX2 (32 pixels at a time).
; Handles edge cases using a scalar fallback loop.
;
; Registers:
; RDI: src_ptr, RSI: dest_ptr, RDX: width, RCX: height
; -----------------------------------------------------------------------------

global apply_filter_simd

section .text
apply_filter_simd:
    push rbp
    mov rbp, rsp

    ; Start from row 1 to avoid top boundary overflow -> edge case
    mov r8, 1                       ; r8 = current_row (y)

.row_loop:
    ; Calculate row offset: RAX = current_row * width
    mov rax, r8
    imul rax, rdx

    ; --- THE FIX: Calculate Base Row Addresses ONCE per row ---
    lea r9, [rdi + rax]             ; r9 = base pointer to current input row
    lea r12, [rsi + rax]            ; r12 = base pointer to current output row
    
    ; Start from column 1 to avoid left boundary overflow
    mov r11, 1                      ; r11 = current_col (x)

.col_loop:
    ; Calculate remaining pixels in the current row
    mov r10, rdx
    sub r10, r11
    
    ; Check if we have at least 33 pixels (32 for YMM + 1 for right neighbor)
    cmp r10, 33             
    jl .scalar_fallback             ; If not enough, switch to pixel-by-pixel

    ; --- SIMD BLOCK (32-way Parallel Processing) ---
    ; Now using exactly 2 registers for memory addressing (Base + Index)
    vmovdqu ymm1, [r9 + r11 + 1]    ; Load 32 right neighbors
    vmovdqu ymm2, [r9 + r11 - 1]    ; Load 32 left neighbors
    
    ; result = (right - left) and the opposite clipped at zero
    vpsubusb ymm3, ymm1, ymm2
    vpsubusb ymm4, ymm2, ymm1

    vpor ymm3, ymm4, ymm3           ; logical 'or' for absolute value of the subtraction

    vpsllw ymm3, ymm3, 2            ; Each edge pixel is now 4x brighter
    
    vmovdqu [r12 + r11], ymm3       ; Store 32 results back to memory
    
    add r11, 32                     ; Advance column index by 32
    jmp .col_loop

.scalar_fallback:                   ; edge considerations
    ; --- SCALAR BLOCK (Handle remaining pixels one by one) ---
    cmp r11, rdx                    ; Check if we reached the end of the width
    jge .next_row                   ; If done with row, move to next
    
    ; Process single pixel
    mov al, [r9 + r11 + 1]          ; Load right neighbor
    mov bl, [r9 + r11 - 1]          ; Load left neighbor
    
    shr al, 1
    shr bl, 1
    add al, bl                      ; Add left and right
    jnc .no_overflow                ; If result >= 0 and no carry, skip capping
    mov al, 255                     ; If it overflowed, saturate to 255 (White)
.no_overflow:
    
    mov [r12 + r11], al             ; Store single result
    inc r11                         ; Advance column index by 1
    jmp .scalar_fallback

.next_row:
    inc r8                          ; Move to the next row
    mov r10, rcx
    dec r10                         ; Boundary check: row < height - 1
    cmp r8, r10
    jl .row_loop

    vzeroupper                      ; Clean up YMM state (Crucial for AVX)
    leave
    ret

; Secure stack note for modern linkers to prevent execution in stack
section .note.GNU-stack noalloc noexec nowrite progbits