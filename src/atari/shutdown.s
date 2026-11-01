        .export         _cleanup_client
        .export         dev_name

        .import         _pause
        .import         _restore_vbi
        .import         _screen_mem_orig
        .import         _show_other_screen
        .include        "atari.inc"

.proc _cleanup_client
        ; restore key click
        lda     #$00
        sta     NOCLIK
        ; turn off DLI
        lda     #$40
        sta     NMIEN

        ; The game flips SAVMSC and the display list between two buffers.
        ; Restore the OS screen before returning to the title.
        lda     _screen_mem_orig
        sta     SAVMSC
        lda     _screen_mem_orig+1
        sta     SAVMSC+1
        jsr     _show_other_screen

        ; restore the VBI
        jsr     _restore_vbi

        rts
.endproc

.data
dev_name:        .byte "E:", $9b
