        .text
        nop
        mov     #0x1234, w0
        add     w0, w1, w2
        repeat  #3
        nop
        repeat  w2
        nop
        mac     w4*w5, a
        mac     w6*w7, b
        return
