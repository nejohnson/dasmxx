.text
.global _start
_start:
    nop
    movw r24, r22
    muls r18, r19
    mulsu r18, r19
    fmul r18, r19
    fmuls r18, r19
    fmulsu r18, r19
    cpc r1, r2
    sbc r3, r4
    add r5, r6
    cpse r7, r8
    cp r9, r10
    sub r11, r12
    adc r13, r14
    and r15, r16
    eor r17, r18
    or r19, r20
    mov r21, r22
    cpi r16, 0x12
    sbci r17, 0x34
    subi r18, 0x56
    ori r19, 0x78
    andi r20, 0x9a
    ld r16, Z
    ldd r17, Z+5
    st Y, r18
    std Y+6, r19
    lpm r20, Z
    lpm r21, Z+
    elpm r22, Z
    elpm r23, Z+
    pop r24
    push r25
    ld r26, Z+
    ld r27, -Z
    ld r18, Y+
    ld r19, -Y
    ld r20, X
    ld r21, X+
    ld r22, -X
    st Z+, r1
    st -Z, r2
    st Y+, r3
    st -Y, r4
    st X, r5
    st X+, r6
    st -X, r7
    lds r8, 0x1234
    sts 0x1234, r9
    sev
    clv
    com r10
    neg r11
    swap r12
    inc r13
    asr r14
    lsr r15
    ror r16
    dec r17
    sec
    clc
    ijmp
    ret
    icall
    reti
    sleep
    break
    wdr
    jmp long_target
    call long_target
    lpm
    elpm
    spm
    adiw r24, 1
    sbiw r26, 2
    cbi 0x12, 3
    sbic 0x13, 4
    sbi 0x14, 5
    sbis 0x15, 6
    mul r18, r19
    in r20, 0x16
    out 0x17, r21
    rjmp near_target
    rcall near_target
    ldi r22, 0xaa
    brcs near_target
    brcc near_target
    breq near_target
    brne near_target
    bld r23, 1
    bst r24, 2
    sbrc r25, 3
    sbrs r26, 4
near_target:
    nop
long_target:
    nop
