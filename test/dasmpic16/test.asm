        processor 16f84a
        org 0

        addwf   0x0c,w
        decf    0x4c,f
        movf    0x7f,w
        btfsc   0x22,5
        call    0x156
        goto    0x2ab
        addlw   0x11
        sublw   0x22
        retfie
        return

        end
