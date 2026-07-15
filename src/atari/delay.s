        .export _pause
        .export _wait_vsync
        .export _network_retry_pause

        .include "atari.inc"

.proc _wait_vsync
:       lda     VCOUNT
        bne     :-

; you have to wait for 1 now, else pause will skip immediately as VCOUNT is still 0
:       lda     VCOUNT
        beq     :-
        rts
.endproc

; void pause(uint8_t jiffies)
;
; each vblank is 1 jiffy, and we wait for that many to occur.
; NTSC: 59.9227 Hz (6 = 0.1s, 20 = 0.33s)
;  PAL: 49.8607 Hz (5 = 0.1s, 20 = 0.40s)

.proc _pause
        tax

:       jsr     _wait_vsync
        dex
        bne     :-
        rts
.endproc

; void network_retry_pause(void)
; Keep network polling backoff close to the existing 3-frame retry delay.
.proc _network_retry_pause
        lda     #$03
        jmp     _pause
.endproc
