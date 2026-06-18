; MODE 7 screen_buf drawing: row offset table, put_cell, gfx_show_shape.
;
; cc65 fastcall (default): rightmost parameter in A (8-bit) or A/X (16-bit).
; Remaining parameters sit on the C stack at c_sp+0 upward, from right-to-left
; (second-from-right at +0, then +1, +2 …).  Callee restores c_sp with addyspN
; where N is the total stack-byte count only (register args are not on stack).

        .export         _screen_put_cell
        .export         _gfx_show_shape
        .export         row_offsets
        .export         set_row_ptr_y

        .import         addysp
        .import         _screen_buf
        .import         _gfx_shapes
        .import         _gfx_shape_count
        .importzp       c_sp
        .importzp       ptr1, ptr2
        .importzp       tmp1, tmp2, tmp3, tmp4

ROW_BYTES       = 40
COL_OFFSET      = 1
SCREEN_WIDTH    = 40
SCREEN_HEIGHT   = 24

; screen_put_cell(uint8_t x, uint8_t y, uint8_t ch)
;   A=ch  c_sp+0=y  c_sp+1=x  →  addysp 2
PUTCELL_STACK_BYTES     = 2
PUTCELL_Y               = 0
PUTCELL_X               = 1

; gfx_show_shape(uint8_t shape_id, int8_t cx, int8_t cy, uint8_t max_row)
;   A=max_row  c_sp+0=cy  c_sp+1=cx  c_sp+2=shape_id  →  addysp 3
GFX_STACK_BYTES         = 3
GFX_CY                  = 0
GFX_CX                  = 1
GFX_SHAPE_ID            = 2

row_offsets:
        .word   0
        .word   40
        .word   80
        .word   120
        .word   160
        .word   200
        .word   240
        .word   280
        .word   320
        .word   360
        .word   400
        .word   440
        .word   480
        .word   520
        .word   560
        .word   600
        .word   640
        .word   680
        .word   720
        .word   760
        .word   800
        .word   840
        .word   880
        .word   920

max_row_save:   .byte 0
shape_row:      .byte 0
shape_id_save:  .byte 0

; ptr1 = screen_buf + row_offsets[Y]
set_row_ptr_y:
        tya
        asl
        tay
        lda     row_offsets,y
        clc
        adc     #<_screen_buf
        sta     ptr1
        lda     row_offsets+1,y
        adc     #>_screen_buf
        sta     ptr1+1
        rts

fix_stack_putcell:
        ldy     #PUTCELL_STACK_BYTES
        jmp     addysp

fix_stack_gfx:
        ldy     #GFX_STACK_BYTES
        jmp     addysp

; void screen_put_cell(uint8_t x, uint8_t y, uint8_t ch)
_screen_put_cell:
        sta     tmp2                    ; ch (fastcall, rightmost)

        ldy     #PUTCELL_X
        lda     (c_sp),y
        cmp     #SCREEN_WIDTH
        bcs     fix_stack_putcell
        sta     tmp1                    ; x

        ldy     #PUTCELL_Y
        lda     (c_sp),y
        cmp     #SCREEN_HEIGHT
        bcs     fix_stack_putcell
        tay
        jsr     set_row_ptr_y

        ldy     tmp1
        lda     tmp2
        sta     (ptr1),y
        jmp     fix_stack_putcell

; void gfx_show_shape(uint8_t shape_id, int8_t cx, int8_t cy, uint8_t max_row)
; tmp1=width tmp2=height tmp3=start_x tmp4=start_y ptr2=cells row ptr
_gfx_show_shape:
        sta     max_row_save            ; max_row (fastcall rightmost, in A)

        ldy     #GFX_SHAPE_ID
        lda     (c_sp),y
        sta     shape_id_save

        lda     shape_id_save
        cmp     _gfx_shape_count
        bcc     @shape_ok
        jmp     fix_stack_gfx

@shape_ok:
        lda     shape_id_save
        asl
        asl
        tay
        lda     _gfx_shapes,y
        sta     tmp1
        lda     _gfx_shapes+1,y
        sta     tmp2
        lda     _gfx_shapes+2,y
        sta     ptr2
        lda     _gfx_shapes+3,y
        sta     ptr2+1

        ; wd2 = width >> 1
        lda     tmp1
        lsr
        sta     tmp4

        ; start_x = cx - wd2 - 1; even width -> start_x++
        ldy     #GFX_CX
        lda     (c_sp),y
        sec
        sbc     tmp4
        sbc     #1
        sta     tmp3

        lda     tmp1
        and     #1
        bne     @start_y

        inc     tmp3

@start_y:
        ; start_y = cy - wd2 - 1; even width -> start_y++
        ldy     #GFX_CY
        lda     (c_sp),y
        sec
        sbc     tmp4
        sbc     #1
        sta     tmp4

        lda     tmp1
        and     #1
        bne     @row_loop

        inc     tmp4

@row_loop:
        ldy     #0

@row_each:
        cpy     tmp2
        bcc     @row_body
        jmp     fix_stack_gfx

@row_body:
        sty     shape_row

        tya
        clc
        adc     tmp4
        tax
        bmi     @next_row
        cpx     #SCREEN_HEIGHT
        bcs     @next_row
        cpx     max_row_save
        bcs     @next_row

        txa
        tay
        jsr     set_row_ptr_y

        ldx     #0

@col_loop:
        cpx     tmp1
        bcs     @next_row

        txa
        tay
        lda     (ptr2),y
        beq     @next_col

        pha
        txa
        clc
        adc     tmp3
        adc     #COL_OFFSET
        bmi     @col_pop
        cmp     #SCREEN_WIDTH
        bcs     @col_pop
        cmp     #COL_OFFSET
        bcc     @col_pop
        tay
        pla
        sta     (ptr1),y
        jmp     @next_col

@col_pop:
        pla

@next_col:
        inx
        jmp     @col_loop

@next_row:
        lda     ptr2
        clc
        adc     tmp1
        sta     ptr2
        bcc     @ptr_ok
        inc     ptr2+1
@ptr_ok:
        ldy     shape_row
        iny
        jmp     @row_each
