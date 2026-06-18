; BBC MODE 7 screen helpers: Master detection and MOS screen base.
;
; Never hard-code &7C00 — read the base from MOS workspace &0350/&0351.
; On Master 128, disable shadow RAM so writes to the MOS screen base are visible.

        .export         _screen_init
        .export         _screen_visible

        .include        "oslib/os.inc"

SCREEN_LO       = $350
SCREEN_HI       = $351

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
