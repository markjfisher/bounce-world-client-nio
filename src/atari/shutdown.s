        .export         _cleanup_client
        .export         dev_name

        .import         _pause
        .import         _restore_vbi
        .include        "atari.inc"

.proc _cleanup_client
        ; restore key click
        lda     #$00
        sta     NOCLIK
        ; turn off DLI
        lda     #$40
        sta     NMIEN

        ; restore the VBI
        jsr     _restore_vbi

        rts
.endproc

.data
dev_name:        .byte "E:", $9b
