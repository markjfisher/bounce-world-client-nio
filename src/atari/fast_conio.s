        .export         _cincx
        .export         _cputc_fast
        .export         _gotoxy_fast

        .import         _mul40
        .import         _revflag
        .import         popa

        .import         _debug
        .import         _pause

        .importzp       ptr4

        .include        "atari.inc"

; void cputc_fast(c)
;
; Minimal version of cputc that does not update the old cursor position
; or care about CR/LF chars. Used exclusively for object drawing.

_cputc_fast:
        ; quickly deal with space char by just incrementing the current column
        cmp     #' '
        beq     _cincx

        ; convert char c to screen code
        asl     a               ; shift out the inverse bit
        adc     #$c0            ; grab the inverse bit; convert ATASCII to screen code
        bpl     codeok          ; screen code ok?
        eor     #$40            ; needs correction
codeok: lsr     a               ; undo the shift
        bcc     cputdirect
        eor     #$80            ; restore the inverse bit

cputdirect:
        pha

        ;; precalculated row->offset table (48 byte table + 3 extra, 8 cycles)
        ldy     ROWCRS
        lda     mul40_lo, y
        ldx     mul40_hi, y
        adc     SAVMSC
        sta     ptr4
        txa
        adc     SAVMSC+1
        sta     ptr4+1

        pla
        ora     _revflag
        ldy     COLCRS
        sta     (ptr4), y

        inc     COLCRS
        rts

_cincx:
        inc     COLCRS
        rts

_gotoxy_fast:
        sta     ROWCRS          ; Set Y
        jsr     popa            ; Get X
        sta     COLCRS          ; Set X
        lda     #0
        sta     COLCRS+1
        rts

.data
.define mul40_table $0000, $0028, $0050, $0078, $00A0, $00C8, $00F0, $0118, $0140, $0168, $0190, $01B8, $01E0, $0208, $0230, $0258, $0280, $02A8, $02D0, $02F8, $0320, $0348, $0370, $0398

mul40_lo:       .lobytes mul40_table
mul40_hi:       .hibytes mul40_table
