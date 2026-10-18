; Test division by zero exception
    ldc 10      ; A = 10
    ldc 0       ; B = 10 (previous A), A = 0
    div         ; Attempts 10 / 0
    HALT