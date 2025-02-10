;#define XASMAVR via commandline option '/DXASMAVR'
;----------------------------------,
#ifdef XASMAVR			   ;
 .TITLE Test: The AVR(R) Instruction Set
 .PAGELENGTH(84)		   ;
 .PAGEWIDTH(115)		   ;
 .SYMBOLS			   ;
 .MODEL WORD 			   ;
 .DEVICE ATxmega384C3		   ;
#else				   ;
 .DEVICE ATmega2560		   ;
#endif ; XASMAVR		   ;
;----------------------------------'

#ifndef XASMAVR
 .DEF X  = r26
 .DEF XL = R26
 .DEF XH = R27
 .DEF Y  = r28
 .DEF YL = R28
 .DEF YH = R29
 .DEF Z  = r30
#endif ; XASMAVR

_instruction_set:
        adc     R16, R16		      
        add     R16, R16
        adiw    R25:R24, 63
        adiw  	R24, 63
        and     R16, R16
        andi    R16, $FF
        asr     R18
        bclr    0
        bld     R18, 0
        brbc    0, PC+2
        brbs    0, PC+2
        brcc    PC+2
        brcs    PC+2
        brbc    0, PC-2
        brbs    0, PC-2
        brcc    PC-2
        brcs    PC-2
        break
        breq    PC-2
        brhc    PC-2
        brhs    PC-2
        brid    PC-2
        brie    PC-2
        brLo    PC-2
        brlt    PC-2
        brmi    PC-2
        brne    PC-2
        brpl    PC-2
        brsh    PC-2
        brtc    PC-2
        brts    PC-2
        brvc    PC-2
        brvs    PC-2
        bset    0
        bst     R0, 0
        call    10
        call    _instruction_set
        cbi     31, 0
        cbr     R16, 255
        clc
        clh
        cli
        cln
        clr     R18
        cls
        clt
        clv
        clz
        com     R18
        cp      R16, R16
        cpc     R16, R16
        cpi     R16, 255
        cpse    R16, R16
        dec     R18
        elpm
        elpm    R18, Z
        elpm    R18, Z+
        eor     R16, R16
        fmul    R16, R16
        fmuls   R16, R16
        fmulsu 	R16, R16
        icall
        ijmp
        in      R18, 63
        inc     R18
        jmp     10
        jmp     _instruction_set
        ld      R18,  X
        ld      R18,  X+
        ld      R18, -X
        ld      R18,  Y
        ld      R18,  Y+
        ld      R18, -Y
        ld      R18,  Z
        ld      R18,  Z+
        ld      R18, -Z
        ldd     R18,  Y+63
        ldd     R18,  Z+63
        ldi     R16, 255
        lds     R18, 65535
        lpm
        lpm     R18, Z
        lpm     R18, Z+
        lsl     R18
        lsr     R18
        mov     R16, R16
        movw  	XH:XL, YH:YL
        movw    X, Y             
        mul     R16, R16
        muls    R16, R16
        neg     R18
        nop
        or      R16, R16
        ori     R16, $FF
        out     63, R18
        pop     R18
        push    R18
;;ha;;  rcall 	-10     ;; -10-(PCw+1) won't make any sense
        rcall   PC-10
        ret
        reti
;;ha;;  rjmp    +10     ;; +10-(PCw+1) won't make any sense
        rjmp    PC+10
        rol     R18
        ror     R18
        sbc     R16, R16
        sbci    R16, 255
        sbi     31, 0
        sbic    31, 0        
        sbis    31, 0        
        sbiw    R25:R24, 63      
        sbiw    R24, 63
        sbr     R16, 255
        sbrc    R18, 0
        sbrs    R18, 0
        sec
        seh
        sei
        sen
        ser     R16
        ses
        set
        sev
        sez
        sleep
        spm
        st       X, R18
        st       X+,R18
        st      -X, R18
        st       Y, R18
        st       Y+,R18
        st      -Y, R18
        st       Z, R18
        st       Z+,R18
        st      -Z, R18
        std     Y+63, R18
        std     Z+63, R18
        sts     65535, R18
        sub     R16, R16
        subi    R16, 255
        swap    R18
        tst     R18
        wdr
; Advanced instructions supported in some ATxmega*
#ifdef XASMAVR
        des     5
        eicall
        eijmp
        lac     Z, R18
        las     Z, R18
        lat     Z, R18
        spm     Z+
        xch     Z, R18
_End_of_instruction_set:
#else                      
#endif ; XASMAVR                         

; -------------------------------------------
