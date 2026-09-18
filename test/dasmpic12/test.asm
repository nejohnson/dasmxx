        processor 12c508a
        org 0

        addwf   0x0c,w
        andwf   0x0d,f
        bcf     0x06,3
        btfss   0x06,2
        call    0x55
        goto    0x123
        movlw   0xaa
        retlw   0x5a
        tris    6
        option
        sleep

        end
