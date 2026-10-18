; Mean of an array
; Locals: [0]=n, [1]=i, [2]=sum

            adj         -3          ; reserve 3 locals
            ldc         count
            ldnl        0
            stl         0           ; n = count
            ldc         0
            stl         1           ; i = 0
            stl         2           ; sum = 0

loop:       ldl         0
            ldl         1
            sub                     ; n - i
            brz         calc        ; if i == n, calculate mean
            brlz        calc
            ldc         arr
            ldl         1
            add
            ldnl        0           ; A = arr[i]
            ldl         2
            add                     ; A = sum + arr[i]
            stl         2           ; sum = updated sum
            ldl         1
            adc         1
            stl         1           ; i++
            br          loop

calc:       ldl         2           ; A = sum
            ldl         0           ; B = sum, A = n
            div                     ; sum / n
            ldc         result
            stnl        0           ; store mean
            adj         3           ; free locals
            HALT

count:      data        5
arr:        data        10
            data        25
            data        35
            data        40
            data        15
result:   data        0           ; Expected: 25