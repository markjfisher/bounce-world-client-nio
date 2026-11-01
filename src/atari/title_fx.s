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
        ; Slow luminance waves through the two field interiors. The DLI
        ; changes colours at WSYNC so every band begins on the same line.
        lda RTCLOK+2
        lsr a
        lsr a
        lsr a
        lsr a
        and #7
        tax
        lda blue_wave,x
        sta colours+2
        lda violet_wave,x
        sta colours+6
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
        cpx #12
        bcc :+
        ldx #0
:       stx phase
        pla
        tax
        pla
        rti
.endproc
.data
; Following the bitmap and text rows 9-16, 19, 21 and 23.
colours: .byte $02, $82, $84, $82, $02, $42, $44, $42, $02, $62, $02, $02
.rodata
blue_wave:   .byte $82, $84, $86, $88, $86, $84, $82, $80
violet_wave: .byte $42, $44, $46, $48, $46, $44, $42, $40
