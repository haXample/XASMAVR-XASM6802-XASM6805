;#define XASMAVR via commandline option '/DXASMAVR'
;----------------------------------,
#ifdef XASMAVR			   ;
 .TITLE Test: Operands, Operators and Expressions
 .PAGELENGTH(84)		   ;
 .PAGEWIDTH(145)		   ;
 .SYMBOLS			   ;
 .MODEL BYTE 			   ;
 .DEVICE ATxmega384C3		   ;
#else				   ;
 .DEVICE ATmega2560		   ;
#endif ;XASMAVR		   	   ;
;----------------------------------'

        .DB  $18  | $B,          24  | 11 ;= 1B 1B
        .DB  0b11000  | 0b1011,  24  | 11 ;= 1B 1B

        .DB 0x18 ,  0x0B ,24,11, 1, \
             $18 ,   $0B ,24,11,    \
         0b11000 ,0b1011 ,24,11

        .DB 0x18 ,  0x0B ,24,11, \
             $18 ,   $0B ,24,11, \
         0b11000 ,0b1011 ,24,11

	.DW -0b1
	.DW -0b0

#ifdef XASMAVR
;; Test: Intel/MASM syntax binary/hex suffix 'b' 'h' special handling
;; 	 Motorola syntax binary prefix '%' '$'
;;       Unary '+'
	.DB +0b1, -0b1
	.DB +0b0, -0b0

        .DB 0x0B, $0B, 0Bh, 0B0h, 0b

        .DB 0x18 ,  0x0B ,24,11, \
             $18 ,   $0B ,24,11, \
              18h,    0Bh,24,11, \ 
         0b11000 ,0b1011 ,24,11, \
          %11000 , %1011 ,24,11, \
           11000b,  1011b,24,11


	.DB 0x18, 0x0B, 24, 11		  ;= 18 0B 1B 2B
	.DB 18h, 0Bh, 1Bh, 2Bh		  ;= 18 0B 1B 2B
	.DB 18h, 0Bh, 1Bh, 2Bh		  ;= 18 0B 1B 2B

	.DB 18h, 0Ah, 1Ah, 2Ah		  ;= 18 0A 1A 2A

        .DB 0x18     OR 0x0B,   24 OR 11  ;= 1B 1B
        .DB  $18     OR  $0B,   24 OR 11  ;= 1B 1B
        .DB  18h     OR  0Bh,   24 OR 11  ;= 1B 1B
        .DB 0b11000  OR 0b1011, 24 OR 11  ;= 1B 1B
        .DB %11000   OR %1011,  24 OR 11  ;= 1B 1B
        .DB 11000b   OR 1011b,  24 OR 11  ;= 1B 1B

.SET Test1=+1b
.DW TEST1
.SET Test1=-1b
.DW TEST1
.SET Test1=1b
.DW TEST1
.SET Test1=01b

.SET Test1=+11b
.DW TEST1
.SET Test1=-11b
.DW TEST1
.SET Test1=11b
.DW TEST1
.SET Test1=011b

.SET Test1=0b+11b
.DW TEST1
.SET Test1=0b-11b

.SET Test1=101b-11b
.DW TEST1

.SET Test1=1b*3
.DW TEST1
.SET Test1=11b *3
.DW TEST1
.SET Test1=1b+3
.DW TEST1
.SET Test1=11b+3

.SET Test1=%0+%11
.DW TEST1
.SET Test1=%0-%11
.DW TEST1
.SET Test1=%1+3
.DW TEST1
.SET Test1=%11+3
.DW TEST1
.SET Test1=%1*3
.DW TEST1
.SET Test1=%11*3
.DW TEST1

.SET  TEST1=%101*%11+%10 
.DW TEST1
.SET  TEST1=101b*11b+10b 
.DW TEST1
	.DB  %1*3,   %1+3,   %101*3,   %101+3,   %101-3,   +%11 
	.DB  %1*%11, %1+%11, %101*%11, %101+%11, %101-%11, -%11
	.DB  1b*3,   1b+3,   101b*3,   101b+3,   101b-3,   +11b
	.DB  1b*11b, 1b+11b, 101b*11b, 101b+11b, 101b-11b, -11b
#endif ;XASMAVR

; Decimal Constants
	.DW  20000,  65500,  -1234		    

; Hexadecimal Constants (three notations)		    
#ifdef XASMAVR
	.DW  0x4E20, $4E20,  4E20h, \
	     0xFFDC, $FFDC, 0FFDCh, \
	     -0x4d2, -$4d2,  -4d2h
#endif ;XASWMAVR

	.DW  0x4E20, $4E20, \
	     0xFFDC, $FFDC, \
	     -0x4d2, -$4d2

;.SET  TEST1=4d2        ; Error, not a valid constant --> hex assumed
;.SET  TEST1=1bbb       ; Error, not a valid constant --> hex assumed
;;ha;;	.DW  4d2, -4d2 ; Error, not a valid constant --> hex assumed
;;ha;;	.DW  1bbb      ; Error, not a valid constant --> hex assumed

#ifdef XASMAVR
; Binary Constants (three notations)
	.DW   0b0100111000100000 , %0100111000100000 , 0100111000100000b , \
	      0b1111111111011100 , %1111111111011100 , 1111111111011100b , \
	         +(0b10011010010),    +(%10011010010),    +(10011010010b), \
	          +0b10011010010 ,     +%10011010010 ,     +10011010010b , \
	          -0b10011010010 ,     -%10011010010 ,     -10011010010b , \
	          ~0b10011010010 ,     ~%10011010010 ,     ~10011010010b , \
	          !0b10011010010 ,     !%10011010010 ,     !10011010010b , \
	       NOT(0b10011010010),  NOT(%10011010010),  NOT(10011010010b), \
	       NOT 0b10011010010 ,  NOT %10011010010 ,  NOT 10011010010b 
	 
	.DB  0b10, %10, 10b, -0b1, -%1, -1b

	.DB  0b1*3, 0b1+3, %1*3, %1+3

	.DB  1b*3, 1b+3, 01b*3, 1b, 01b 

; Current Program memory location counter (three notations)
	.DW  PC, *, $
#endif ;XASMAVR
    
; -------------------------------------------
#ifdef XASMAVR
.SUBTTL Floating point constants
.EJECT
; -------------------------------------------
; Floating Point Constants (1bit.7bits) and (1bit.15bits)
        .DB     Q7(0.62500000), Q7(-0.62500000) ;; = $50,  (NOT $50) + 1
        .DB     Q7(1.625), Q7(-1.625)           ;; = $D0,  (NOT $D0) + 1

; 2.e+0	= 1
; 2.e-1 = 0,5
; 2.e-2 = 0,25
; 2.e-3 = 0,125
; 2.e-4 = 0,0625
; 2.e-5 = 0,03125
; 2.e-6 = 0,015625  Sum(2.e-1:2.e-6) = 0,9765625
; 2.e-7 = 0,0078125 Sum(2.e-1:2.e-7) = 0,9921875
;
	.DB	Q7(0.0), Q7(0.5), Q7(0.25), Q7(0.125),                \
	        Q7(0.0625), Q7(0.03125, Q7(0.015625), Q7(0.00078125), \
		Q7(0.00390625), Q7(0.001953125)

	.DB	Q7(0.9765625), Q7(0.001953125)

	.DB     Q7(0.9921875), Q7(1.9921875), Q7(-0.9921875), Q7(-1.9921875)
 
; 2.e-8	 = 0,00390625
; 2.e-9  = 0,001953125
; 2.e-10 = 0,0009765625
; 2.e-11 = 0,00048828125
; 2.e-12 = 0,000244140625
; 2.e-13 = 0,0001220703125
; 2.e-14 = 0,00006103515625
; 2.e-15 = 0,000030517578125  
; 2.e-16 = 0,0000152587890625  Sum(2.e-8:2.e-16) = 0,999847412109375
; 2.e-17 = 0,00000762939453125 Sum(2.e-8:2.e-17) = 0,999969482421875
;
	.DW     Q15(0.999847412109375), Q15(0.999969482421875)

	.DW     Q15(0.321117799673000)           ;; = $291A, 10522

	.DW     Q15 (0.9921875), Q15 (1.9921875)
	.DW    	Q15(-0.9921875), Q15(-1.9921875)
#endif ;XASMAVR

; -------------------------------------------
#ifdef XASMAVR
.SUBTTL Operators
.EJECT
#endif ;XASMAVR
; -------------------------------------------
; Operators			  ;WORD	BYTE
; -------------------------------------------
_tst01: .DB $18 ==  3 , 24 ==  3  ;0000	00 00
        .DB $18 == $18, 24 == 24  ;0101         
        .DB   3 == $18,  3 == 24  ;0000

        .DB $18 !=  3 , 24 != 3   ;0101
        .DB $18 != $18, 24 != 24  ;0000
        .DB   3 != $18,  3 != 24  ;0101

        .DB  -1 <= -2 , -2 <= -1  ;0100 00 01
        .DB  -1 <=  0 ,  0 <= -1  ;0001 01 00
        .DB $18 <=  3 , 24 <= 3   ;0000
        .DB $18 <= $18, 24 <= 24  ;0101
        .DB   3 <= $18,  3 <= 24  ;0101

        .DB  -1 >= -2 , -2 >= -1  ;0001 01 00
        .DB  -1 >=  0 ,  0 >= -1  ;0100 00 01
        .DB $18 >=  3 , 24 >= 3   ;0101
        .DB $18 >= $18, 24 >= 24  ;0101
        .DB   3 >= $18,  3 >= 24  ;0000

        .DB  -1 > -2 ,  -2 >  -1  ;0001 01 00
        .DB  -1 >  0 ,   0 >  -1  ;0100 00 01
        .DB $18 >  3 , -24 >  -3  ;0001	01 00
        .DB $18 > $18, -24 > -24  ;0000	00 00
        .DB   3 > $18,  -3 > -24  ;0100	00 01

        .DB  -1 < -2 ,  -2 <  -1  ;0100 00 01
        .DB  -1 <  0 ,   0 <  -1  ;0001 01 00
        .DB $18 <  3 , -24 <  -3  ;0100	00 01
        .DB $18 < $18, -24 < -24  ;0000	00 00
        .DB   3 < $18,  -3 < -24  ;0001	01 00

        .DB  1 && 1, 1 && 1       ;0101
        .DB  1 && 0, 1 && 0       ;0000
        .DB  0 && 1, 0 && 1       ;0000
        .DB  0 && 0, 0 && 0       ;0000

        .DB  1 || 1,  1 || 1      ;0101
        .DB  1 || 0,  1 || 0      ;0101
        .DB  0 || 1,  0 || 1      ;0101
        .DB  0 || 0,  0 || 0      ;0000

        .DB  $38 & $F,  56 & 15   ;0808
        .DB  $18 | $B,  24 | 11   ;1B1B
        .DB  $18 *  3,  24 *  3   ;4848
	.DW    5 *  0 		  ;0000
        .DB  $18 /  3,  24 /  3   ;0808
;;	.Dd    9 /  0	          ; Division by zero --> |ERROR|
        .DB  $18 %  5,  24 %  5   ;0404
;; 	.Dw    4 %  0	          ; Division by zero (MOD 0) --> |ERROR|
        .DB  $18 +  3,  24 +  3   ;1B1B
        .DB  $18 -  3,  24 -  3   ;1515
        .DB  $18 << 3,  24 << 3   ;C0C0 
        .DB  $18 >> 2,  24 >> 2   ;0606 
        .DB  $18 ^  9,  24 ^  9   ;1111

#ifdef XASMAVR
	.DB  NOT(0AAh), NOT 0C3h
        .DB  18h MOD  5h,  24 MOD(5)  ;0404
        .DB  38h AND 0Fh,  56 AND 15  ;0808
        .DB  18h OR  0Bh,  24 OR  11  ;1B1B
        .DB  18h XOR  9h,  24 XOR  9  ;1111
        .DB  18h SHL  3h,  24 SHL  3  ;C0C0 
        .DB  18h SHR  2h,  24 SHR  2  ;0606 
#endif ;XASMAVR

        ; Unary operators		     
        .DB     -1, -5, -0, -$FF	 ;FF FB 00 01
#ifdef XASMAVR
        .DB     +1, +5, +0, +$FF	 ;01 05 00 FF
#endif ;XASMAVR
					 
        .DB     !1, !5, !0, !$AA, !(2-2) ;00 00 01 00 01
        .DB     ~1, ~5, ~0, ~$AA, ~(2-2) ;FE FA FF 55 FF
    
; -------------------------------------------
#ifdef XASMAVR
.SUBTTL Expressions
.EJECT
#endif ;XASMAVR
; -------------------------------------------
; Expression Examples
.SET    Osc_Hz         = 7372800               
.SET    cycle_time_ns  = (1000000000 / Osc_Hz) 
.SET 	cycles = ((2 + cycle_time_ns - 1) / cycle_time_ns - 0)

        .DW 1 * 0b111   ; =07
        .DW 0b111 * 1   ; =07

	.SET _Exp2 = ((-1-1==-2)*0b111|1-0)<<1|high(0x1234)&1
     	.DW  _Exp2  ; MASM=FFF2 (= -0E): no unary precedence (-1-1==-2) = TRUE = +1

        .DW  -1-1==-2						       
        .DW  (((-1 ))-(( 1 ))==(( -2)))						       

        .DW  4+40*0     ; =0004
        .DW  4+(40*0)   ; =0004
        .DW  ((4+40)*0) ; =0000

        .DW  1<<3|0<<2	     ; Dependig on precedence
        .DW  (1<<3)|(0<<3)   ; Intended precedence         
        .DW  (((1<<3)|0)<<2) ; left-to-right

	.DB  (1<<7)
;;ha;;	.DB  (1<<8)	     ; .DB --> |ERROR|, not a |WARNING|
	.DW  (1<<15)
	.DW  (1<<16)	     ; .DW --> |WARNING| only 
#ifdef XASMAVR
	.DD  (1<<31)
	.DD  (1<<32)	; Compiler masks 32bit (shiftNR=shiftNR & 0x1F)
	.DQ  (1<<63)
	.DQ  (1<<64)	; Compiler sets 64bit result = 0x00
#endif ;XASMAVR

        ; Constant16 values relying on assembler precedence
        .DW     LOW(0x7654<<16|0x3210>>8)       ; =0032
        .DW     HIGH(0x7654<<16|0x3210>>8)      ; =0000
        .DW     LWRD((0x7654<<16|0x3210>>8))    ; =0032
        .DW     HWRD((0x7654<<16|0x3210>>8))    ; =7654
#ifdef XASMAVR
        .DD     0x7654<<16|0x3210>>8  		; =76540032
#endif ;XASMAVR

;; !! Warning: Use parenthesis in complex expressions !!
;; Especially when mixing arithmetic with logical operators.
;; Example:
.SET _VAR = 0xA600*256+0x75A2>>8  ;; =0000A675 assumed (result XASMAVR=00A60075)	  
	.DW LWRD(_VAR)		  ;; =    0075 XASMAVR NOT EXPECTED?!
	.DW HWRD(_VAR)		  ;; =    00A6 XASMAVR NOT EXPECTED?!
#ifdef XASMAVR
	.DD _VAR		  ;; =00A60075 NOT EXPECTED?!
.SET _VAR = (0xA600*256)+(0x75A2>>8)
	.DD _VAR                  ;; =00A60075 expected
.SET _VAR = (0xA600*256+0x75A2)>>8   
	.DD _VAR                  ;; =0000A675 expected	as assumed
;;#elif !DEFINED(XASMAVR)
#else
.SET _VAR = (0xA600*256+0x75A2)>>8;; =0000A675 expected	as assumed   
	.DW LWRD(_VAR)		  ;; =A675 expected
	.DW HWRD(_VAR)		  ;; =0000 expected
#endif ;XASMAVR

; -------------------------------------------
#ifdef XASMAVR
.EJECT
#endif ;XASMAVR
; -------------------------------------------
        .DW ((( ((( ((( -1 ))-(( 1 ))==(( -2))) )*( 0b111 ))|(( 1 ))-(( 0))) )<<( 1 ))|(( high(0x1234) )&( 1)))
	.DW 0x7654<<16|0x3210>>8  ;; =76540032

#ifdef XASMAVR
	.DQ 16*1024*1024*1024*1024*1024*1024-2, 18446744073709551615
.EQU _AbsConst4G=4294967296+1     ;; =4G+1
.EQU _AbsConst2G=4294967296/2-1	  ;; =2G-1
	.DQ 4294967296/2-1,_AbsConst2G	     
	.DQ 0x1234567890ABCDEF & 0xFFFFFFFFFFFFFF00,_AbsConst4G	     
	.DQ   1110001111111111111111111111111111111111111000111010101001010101b, 1b, 1b*3, \
	     %1110001111111111111111111111111111111111111000111010101001010101, %1, %1 *3, \
	    0b1110001111111111111111111111111111111111111000111010101001010101,  1b, 1b*3, \
                                        0b110011101110110111011111000010111011,  1b, 1b*3, \
                                0b10111100110011101110110111011111000010111011,  1b, 1b*3, \
                                    0b1100110011101110110111011111000010111011,  1b, 1b*3, \
            0b1110001111111111111111111111111111111111111000111010101001010101

	.DQ 18446744073709551615-1, 18446744073709551615, 18446744073709551615+1, 18446744073709551615/2 

#endif ;XASMAVR
