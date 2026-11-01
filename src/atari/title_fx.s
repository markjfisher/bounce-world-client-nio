; Title-only colour bands. Paused across every SIO transaction.
.export _title_fx_start, _title_fx_stop, _title_fx_pause, _title_fx_resume
.include "atari.inc"
.bss
old_dli: .res 2
old_vbi: .res 2
phase: .res 1
.code
.proc _title_fx_start
        lda #$40
        sta NMIEN
        lda VDSLST
        sta old_dli
        lda VDSLST+1
        sta old_dli+1
        lda VVBLKI
        sta old_vbi
        lda VVBLKI+1
        sta old_vbi+1
        lda #<title_dli
        sta VDSLST
        lda #>title_dli
        sta VDSLST+1
        ldy #<title_vbi
        ldx #>title_vbi
        lda #6
        jsr SETVBV
        jmp _title_fx_resume
.endproc
.proc _title_fx_pause
        lda #$40
        sta NMIEN
        rts
.endproc
.proc _title_fx_resume
        lda #0
        sta phase
        lda #$c0
        sta NMIEN
        rts
.endproc
.proc _title_fx_stop
        jsr _title_fx_pause
        lda old_dli
        sta VDSLST
        lda old_dli+1
        sta VDSLST+1
        ldy old_vbi
        ldx old_vbi+1
        lda #6
        jmp SETVBV
.endproc
.proc title_vbi
        lda #0
        sta phase
        lda #$2a
        sta COLPF2
        jmp SYSVBV
.endproc
.proc title_dli
        pha
        txa
        pha
        ldx phase
        lda colours,x
        sta WSYNC
        sta COLPF2
        inx
        cpx #6
        bcc :+
        ldx #0
:       stx phase
        pla
        tax
        pla
        rti
.endproc
.rodata
; After the bitmap, then text rows 11, 14, 17, 20 and 23.
colours: .byte $02, $b4, $94, $02, $42, $02
