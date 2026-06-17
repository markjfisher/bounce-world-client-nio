        .export _wait_vsync, _pause

        .importzp tmp1

        .include "oslib/os.inc"

.code

; void wait_vsync(void);
; Use the BBC MOS primitive directly: OSBYTE 19 waits for one vertical sync.
_wait_vsync:
        lda     #$13
        jsr     OSBYTE
        rts

; void __fastcall__ pause(uint8_t count);
; count arrives in A. Store it in a private byte because OSBYTE clobbers X/Y.
_pause:
        sta     tmp1

@loop:  lda     tmp1
        beq     @done
        dec     tmp1
        jsr     _wait_vsync
        ; A is preserved across OSBYTE &13 call, but is Z?
        bne     @loop

@done:  rts
