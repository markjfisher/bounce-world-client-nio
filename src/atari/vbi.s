        .export    _init_vbi
        .export    _restore_vbi

        .export    vbi_main

        .import    _debug
        .import    _txt_c3

        ;; DLI values
        .import    current_section

        ; set in C for the iteration of the flash pulse
        .import    _current_flash_time
        ; boolean: true when flash has been requested
        .import    _is_flashing_screen

        ; collision sound
        .import    _is_playing_collision
        .import    _current_volume_index
        .import    _sc0
        .import    _sc1
        .import    _sc2

        .include   "atari.inc"

.proc _init_vbi
        ; save the old VBI
        lda     VVBLKI
        sta     old_vbi
        lda     VVBLKI+1
        sta     old_vbi+1

        ; set the new VBI
        ldy     #<vbi_main
        ldx     #>vbi_main
        lda     #$06
        jmp     SETVBV
.endproc

.proc _restore_vbi
        ldy     old_vbi
        ldx     old_vbi+1
        lda     #$06
        jmp     SETVBV
.endproc


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; VBI routines

; Set PF2 (background) to BLACK at the top of the screen.
; A DLI will change the colour below.

.proc vbi_main
        lda     #$00
        sta     COLPF2
        sta     current_section
        sta     ATRACT
        lda     _txt_c3
        sta     COLBK
        sta     COLOR4

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ;; BG SCREEN FLASH FOR COLLISION

        lda     _is_flashing_screen
        beq     done_flash

        ldy     _current_flash_time
        cpy     #16
        bcs     end_flash

        lda     flash_data, y
        sta     COLBK
        sta     COLOR4
        inc     _current_flash_time
        bne     done_flash

end_flash:
        lda     #$00
        sta     _is_flashing_screen
        sta     _current_flash_time
        lda     _txt_c3
        sta     COLBK
        sta     COLOR4

done_flash:
        jmp     SYSVBV
.endproc


.data
old_vbi:        .word 0
flash_data:     .byte 15, 13, 11, 9, 8, 7, 6, 5, 4, 3, 3, 2, 2, 1, 1, 0
