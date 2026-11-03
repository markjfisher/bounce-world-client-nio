; XEX INIT segment: paint the normal text screen while PicoBoot loads BWCN.
; The image lives in the OS screen RAM, above the application's load chunks.
.segment "CODE"
.export splash_start
SAVMSC = $58
COLOR1 = $02c5
COLOR2 = $02c6
COLOR4 = $02c8
ptr = $cb
splash_start:
        lda ptr
        pha
        lda ptr+1
        pha
        lda SAVMSC
        sta ptr
        lda SAVMSC+1
        sta ptr+1
        ldx #0
        ldy #0
copy_page:
        lda image,x
        sta (ptr),y
        inx
        iny
        bne copy_page
        inc ptr+1
        ldy #0
copy_page2:
        lda image+$100,y
        sta (ptr),y
        iny
        bne copy_page2
        inc ptr+1
        ldy #0
copy_page3:
        lda image+$200,y
        sta (ptr),y
        iny
        bne copy_page3
        inc ptr+1
        ldy #0
copy_tail:
        lda image+$300,y
        sta (ptr),y
        iny
        cpy #192
        bne copy_tail
        lda #$0e
        sta COLOR1
        lda #$92
        sta COLOR2
        lda #0
        sta COLOR4
        pla
        sta ptr+1
        pla
        sta ptr
        rts
image:
        .incbin "atari-splash-screen.bin"
