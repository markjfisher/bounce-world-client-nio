; Some PBI/SIO combinations lose keyboard IRQs even while POKEY reports a
; held key. Preserve the OS handler and only supply CH when no IRQ arrived
; during the first two VBI samples of a press.
        .export _key_fallback_init, _key_fallback_done, key_fallback_poll
        .include "atari.inc"

        .bss
key_previous:  .res 2
key_irq_count: .res 1
key_baseline:  .res 1
key_frames:    .res 1
key_handled:   .res 1

        .code
.proc _key_fallback_init
        php
        sei
        lda VKEYBD
        sta key_previous
        lda VKEYBD+1
        sta key_previous+1
        lda #0
        sta key_irq_count
        sta key_baseline
        sta key_frames
        sta key_handled
        lda #<key_irq
        sta VKEYBD
        lda #>key_irq
        sta VKEYBD+1
        plp
        rts
.endproc

.proc _key_fallback_done
        php
        sei
        lda key_previous
        sta VKEYBD
        lda key_previous+1
        sta VKEYBD+1
        plp
        rts
.endproc

; The OS dispatcher has saved A; leave the old handler's entry state intact.
.proc key_irq
        php
        inc key_irq_count
        plp
        jmp (key_previous)
.endproc

; Called from the immediate VBI; only A and flags are changed.
.proc key_fallback_poll
        lda $d20f              ; SKSTAT bit 2 is low while a key is down
        and #4
        beq down
        lda #0
        sta key_frames
        sta key_handled
        lda key_irq_count
        sta key_baseline
        rts
 down:  lda key_handled
        bne done
        inc key_frames
        lda key_frames
        cmp #2
        bcc done
        lda #1
        sta key_handled
        lda key_irq_count
        cmp key_baseline
        bne done               ; OS already received this key
        lda CH
        cmp #$ff
        bne done               ; never overwrite an unconsumed key
        lda $d209              ; KBCODE, including modifiers
        cmp #$c0
        bcs done               ; OS keyboard handler ignores Ctrl+Shift
        pha
        and #$3f
        cmp #$11               ; HELP uses HELPFG, not the CH latch
        beq skip
        pla
        sta CH
 done:  rts
 skip:  pla
        rts
.endproc
