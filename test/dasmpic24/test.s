        .text
        nop
        mov     #0x1234, w0
        add     w0, w1, w2
        sub     w3, w4, w5
        and     w3, w4, w5
        ior     w3, w4, w5
        xor     w3, w4, w5
        cp      w1, w2
        clr     w0
        clr.b   w1
        com     w2, w3
        neg     w4, w5
        inc     w6, w7
        dec     w8, w9
        sl      w1, w2
        lsr     w3, w4
        asr     w5, w6
        mov     w1, w2
        mov     [w1], w2
        mov     w1, [w2]
        bset    w2, #3
        bclr    w3, #4
        btg     w4, #5
        btst    w5, #6
        repeat  #3
        nop
        .pword  0x370001
        .pword  0x07fffe
        return
        reset
        break
