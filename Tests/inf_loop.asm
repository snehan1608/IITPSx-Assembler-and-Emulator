; Test unconditional backward jump
start:
    ldc 5       ; A = 5
    adc 1       ; A = 6
    br start    ; Jump back to start
    br -1       ; Infinite loop (jump to self)
    HALT