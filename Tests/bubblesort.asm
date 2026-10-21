; Bubble Sort
; Locals: [0]=pass, [1]=j, [2]=&arr[j](adress of element), [3]=arr[j], [4]=arr[j+1]

            adj         -5          ; reserve 5 locals using default SP
            ldc         count
            ldnl        0
            adc         -1
            stl         0           ; pass = count - 1

outer:      ldl         0
            brz         done        ; if pass == 0, sorting finished
            brlz        done
            ldc         0
            stl         1           ; j = 0

inner:      ldl         0
            ldl         1
            sub                     ; pass - j
            brz         nextpass    ; if pass-j <=0, go to next pass
            brlz        nextpass

            ldc         arr
            ldl         1
            add
            stl         2           ; save &arr[j]

            ldl         2
            ldnl        0
            stl         3           ; save arr[j]

            ldl         2
            ldnl        1
            stl         4           ; save arr[j+1]

            ldl         3
            ldl         4
            sub                     ; arr[j] - arr[j+1]
            brlz        noswap      ; skip swap if already in order
            brz         noswap

            ldl         4
            ldl         2
            stnl        0           ; arr[j] = arr[j+1]

            ldl         3
            ldl         2
            stnl        1           ; arr[j+1] = old arr[j]

noswap:     ldl         1
            adc         1
            stl         1           ; j++
            br          inner

nextpass:   ldl         0
            adc         -1
            stl         0           ; pass--
            br          outer

done:       adj         5           ; free
            HALT

count:      data        6
arr:        data        42
            data        15
            data        -8
            data        99
            data        23
            data        4           ; Expected: -8, 4, 15, 23, 42, 99


;int count = 6;
;int arr[6] = {42, 15, -8, 99, 23, 4};
;int pass = count - 1;                  // local 0
;while (pass > 0) {
;    int j = 0;                         // local 1
;    while (j < pass) {
;        if (arr[j] > arr[j+1]) swap(arr[j], arr[j+1]);
;        j++;
;    }
;    pass--;
;}
