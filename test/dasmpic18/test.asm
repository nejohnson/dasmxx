        processor 18f8722
        org 0

start   nop
        addwf   0x12,w,a
        movwf   0x34,b
        negf    0x56,a
        subwf   0x78,f,b
        movff   0x123,0x456
        lfsr    1,0xabc
        bcf     0x22,3,a
        bra     target
        nop
target  nop
        rcall   start
        call    0x12344,0
        goto    0x23456
        return  1

        end
