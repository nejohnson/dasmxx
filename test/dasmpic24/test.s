        .text
        nop
        mov     #0x1234, w0
        add     w0, w1, w2
        sub     w3, w4, w5
        and     w3, w4, w5
        ior     w3, w4, w5
        xor     w3, w4, w5
        cp      w1, w2
        mov     w1, w2
        mov     [w1], w2
        mov     w1, [w2]
        repeat  #3
        nop
        .pword  0x370001
        .pword  0x07fffe
        return
        reset
        break
