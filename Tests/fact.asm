; Iterative Factorial (N!)
; Locals: [0]=n, [1]=result

            adj         -2          ; reserve 2 locals
            ldc         num
            ldnl        0
            stl         0           ; n = num
            ldc         1
            stl         1           ; result = 1

factloop:  ldl         0
            brz         done        ; if n == 0, finished
            brlz        done        
            ldl         1           ; B = result
            ldl         0           ; A = n
            mul                     ; A = result * n
            stl         1           ; result = result * n
            ldl         0
            adc         -1
            stl         0           ; n = n - 1
            br          factloop

done:       ldl         1
            ldc         factres
            stnl        0           ; store factorial result
            adj         2           ; free locals
            HALT

num:        data        5           
factres:   data        0           ; Expected: 120