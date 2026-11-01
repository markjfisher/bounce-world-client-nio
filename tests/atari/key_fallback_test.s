        .export _test_func, _test_result
        .import _key_fallback_init, _key_fallback_done, key_fallback_poll
        .include "atari.inc"
        .bss
_test_result: .res 20
old_calls: .res 1
        .code
old_irq:
        inc old_calls
        pla
        rti
fire:
        lda #>@back
        pha
        lda #<@back
        pha
        php
        lda #$5a
        pha
        jmp (VKEYBD)
@back: rts
release:
        lda #4
        sta $d20f
        jmp key_fallback_poll
press:
        lda #0
        sta $d20f
        rts
_test_func:
        lda #<old_irq
        sta VKEYBD
        lda #>old_irq
        sta VKEYBD+1
        lda #$ff
        sta CH
        jsr _key_fallback_init
        jsr release
        lda #13
        sta $d209
        jsr press
        jsr key_fallback_poll
        lda CH
        sta _test_result       ; first frame: no injection
        jsr key_fallback_poll
        lda CH
        sta _test_result+1     ; second frame: I scan code
        lda #$ff
        sta CH
        jsr key_fallback_poll
        lda CH
        sta _test_result+2     ; held key does not repeat
        jsr release
        jsr fire
        jsr press
        jsr key_fallback_poll
        jsr key_fallback_poll
        lda CH
        sta _test_result+3     ; IRQ already seen: no fallback
        lda old_calls
        sta _test_result+4     ; OS handler chained
        jsr release
        lda #$24
        sta CH
        jsr press
        jsr key_fallback_poll
        jsr key_fallback_poll
        lda CH
        sta _test_result+5     ; pending key preserved
        lda #$ff
        sta CH
        jsr release
        lda #$11
        sta $d209
        jsr press
        jsr key_fallback_poll
        jsr key_fallback_poll
        lda CH
        sta _test_result+6     ; HELP not injected into CH
        jsr release
        lda #$cd
        sta $d209
        jsr press
        jsr key_fallback_poll
        jsr key_fallback_poll
        lda CH
        sta _test_result+7     ; Ctrl+Shift ignored
        jsr release
        lda #24
        sta $d209
        jsr press
        jsr key_fallback_poll
        jsr key_fallback_poll
        lda CH
        sta _test_result+8     ; new press is recognised
        jsr _key_fallback_done
        lda VKEYBD
        cmp #<old_irq
        bne fail
        lda VKEYBD+1
        cmp #>old_irq
        bne fail
        lda #1
        bne save
fail:   lda #0
save:   sta _test_result+9
        rts
