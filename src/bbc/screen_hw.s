; BBC MODE 7 screen helpers: Master detection, MOS screen base, fast blit.
;
; Never hard-code &7C00 — read the base from MOS workspace &0350/&0351.
; On Master 128, disable shadow RAM so writes to the MOS screen base are visible.

        .export         _screen_init
        .export         _screen_visible
        .export         _screen_blit_rows
        .export         _screen_playfield_clear

        .import         _screen_buf
        .import         _gfx_colour
        .import         set_row_ptr_y
        .importzp       ptr1, ptr2, tmp1, tmp2, tmp3, tmp4

        .include        "oslib/os.inc"

SCREEN_LO       = $350
SCREEN_HI       = $351
ROW_BYTES       = 40
PLAYFIELD_COLS  = 39
GFX_CHAR_EMPTY  = $20

; uint8_t *screen_visible;
_screen_visible:
        .addr   $7C00           ; default until screen_init runs (.addr == .word but clearer it's being used as address)

; void screen_init(void);
_screen_init:
        jsr     detect_master
        bcc     @not_master
        jsr     master_init

@not_master:
        lda     SCREEN_LO
        sta     _screen_visible
        lda     SCREEN_HI
        sta     _screen_visible + 1
        rts

detect_master:
        ldx     #$00
        ldy     #$FF            ; INKEY(-256), -256 = $FF00
        lda     #$81            ; OSBYTE 129
        jsr     OSBYTE
        cpx     #$FD            ; Master returns 253 in X
        beq     @is_master
        clc
        rts
@is_master:
        sec
        rts

master_init:
        ldx     #$00
        ldy     #$00
        lda     #$70            ; OSBYTE 112 (*FX112,0)
        jsr     OSBYTE
        ldx     #$00
        ldy     #$00
        lda     #$71            ; OSBYTE 113 (*FX113,0)
        jmp     OSBYTE

; ---------------------------------------------------------------------------
; void screen_playfield_clear(uint8_t max_row);
; Fastcall: A=max_row only — no stack args, no addysp.
; Clear max_row lines in screen_buf: col 0 = gfx colour, cols 1-39 = &20.
; ---------------------------------------------------------------------------
_screen_playfield_clear:
        sta     tmp1            ; max_row
        beq     @done

        ldy     #0

@row_loop:
        cpy     tmp1
        bcs     @done

        sty     tmp2            ; save row index (Y clobbered by fill)

        ldy     tmp2
        jsr     set_row_ptr_y

        ldy     #1
        lda     #GFX_CHAR_EMPTY
@fill:
        sta     (ptr1),y
        iny
        cpy     #ROW_BYTES
        bne     @fill

        lda     _gfx_colour
        ldy     #0
        sta     (ptr1),y

        ldy     tmp2
        iny
        jmp     @row_loop

@done:
        rts

; ---------------------------------------------------------------------------
; void screen_blit_rows(uint8_t num_rows);
; Fastcall: A=num_rows only — no stack args, no addysp.
; Copy num_rows * 40 bytes from screen_buf to screen_visible.
; Uses 256-byte page copies (ISS copy style) then a short remainder.
; ---------------------------------------------------------------------------
_screen_blit_rows:
        sta     tmp1            ; num_rows
        beq     @done

        lda     _screen_visible
        ora     _screen_visible+1
        beq     @done

        ; tmp2:tmp3 = num_rows * 40 (16-bit byte count, max 960)
        lda     #0
        sta     tmp2            ; high
        sta     tmp3            ; low
        ldx     tmp1
@acc:
        clc
        lda     tmp3
        adc     #ROW_BYTES
        sta     tmp3
        bcc     @acc_no_hi
        inc     tmp2
@acc_no_hi:
        dex
        bne     @acc

        lda     #<_screen_buf
        sta     ptr2
        lda     #>_screen_buf
        sta     ptr2+1

        lda     _screen_visible
        sta     ptr1
        lda     _screen_visible+1
        sta     ptr1+1

        ; Full 256-byte pages in tmp2
@page:
        lda     tmp2
        beq     @remainder
        ldy     #0
@page_inner:
        lda     (ptr2),y
        sta     (ptr1),y
        iny
        bne     @page_inner
        inc     ptr2+1
        inc     ptr1+1
        dec     tmp2
        jmp     @page

        ; Remaining bytes (0-255) in tmp3
@remainder:
        ldy     #0
@rem:
        cpy     tmp3
        beq     @done
        lda     (ptr2),y
        sta     (ptr1),y
        iny
        jmp     @rem

@done:
        rts
