        .export   _dli
        .export   _enable_dli
        .export   current_section

        .import   _is_flashing_screen
        .import   _is_showing_info
        .import   _debug
        .import   _txt_c1
        .import   _txt_c2

        .import   _wait_vsync

        .include  "atari.inc"

; This DLI makes the background BLACK for the graphics area at the top of the
; screen, and BLUE for the text area in the last 4 lines.
; The DLI instructions are added in dlist.c

.proc _dli
        pha                     ; store A while we do our routine
        lda     _is_showing_info
        beq     exit            ; exit if not showing the info bar

        lda     current_section ; which part of the screen are we in?
        sta     WSYNC           ; ensure we're at start of scan line for colour change
        bne     section_1

section_0:
        lda     _txt_c1         ; first colour change
        bne     set_and_inc

section_1:
        lda     _txt_c2         ; second colour change

set_and_inc:
        sta     COLPF2
        inc     current_section

done:   lda     #$0F            ; bright white
        sta     COLPF1          ; text colour

exit:
        pla                     ; restore A
        rti                     ; end DLI

.endproc

.proc _enable_dli
        ; has to be done at the beginning of the screen to ensure the DLI at
        ; the top fires first.
        jsr     _wait_vsync
        lda     #$C0
        sta     NMIEN
        rts
.endproc

.data
current_section:        .byte 0
