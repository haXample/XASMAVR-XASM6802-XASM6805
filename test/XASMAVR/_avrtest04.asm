;#define XASMAVR via commandline option '/DXASMAVR'
;----------------------------------,
#ifdef XASMAVR			   ;
 .TITLE Test: Assembler supported inbuild functions
 .PAGELENGTH(84)		   ;
 .PAGEWIDTH(130)		   ;
 .SYMBOLS			   ;
 .MODEL BYTE 			   ;
 .DEVICE ATxmega384C3		   ;
#else				   ;
 .DEVICE ATmega2560		   ;
#endif ;XASMAVR		   	   ;
;----------------------------------'

        .DB LOW  (0x89ABCDEF), 0x89ABCDEF & 0xEF         ; Bits [ 7: 0]
        .DB HIGH (0x89ABCDEF), (0x89ABCDEF >>  8) & 0xFF ; Bits [15: 8]
        .DB BYTE2(0x89ABCDEF), (0x89ABCDEF >>  8) & 0xFF ; Bits [15: 8]
        .DB BYTE3(0x89ABCDEF), (0x89ABCDEF >> 16) & 0xFF ; Bits [23:16]
        .DB BYTE4(0x89ABCDEF), (0x89ABCDEF >> 24) & 0xff ; Bits [31:24]
        .DW LWRD (0x89ABCDEF), 0x89ABCDEF & 0xFFFF       ; Bits [15: 0]	    
        .DW HWRD (0x89ABCDEF), 0x89ABCDEF >> 16	         ; Bits [31:16]	         

#ifdef XASMAVR
 ; See Microchip AVR Assembler Manual 2017 Chapt 7.1:
 ; "PAGE(expression) returns bits 16-21 of an expression"
  	.DW PAGE(0x89ABCDEF), (0x89ABCDEF & 0x003F0000) >> 16 ; Bits [21:16]        
  	.DW PAGE(0x12345678), (0x12345678 & 0x003F0000) >> 16 ; Bits [21:16]             
#else
#endif ;XASMAVR
										     
#ifdef XASMAVR
	.DB EXP2(0),  1		  
	.DB EXP2(7),  128	  
	.DW EXP2(8),  256	  
	.DW EXP2(15), 32768                                                             
	.DD EXP2(16), 65536                                                            
	.DD EXP2(23), 8388608	  
	.DD EXP2(24), 16777216 
	.DD EXP2(31), 2147483648  
	.DQ EXP2(32), 4294967296  
	.DQ EXP2(39), 549755813888  
	.DQ EXP2(47), 140737488355328  
	.DQ EXP2(55), 36028797018963968  
	.DQ EXP2(63), 9223372036854775808  
	.DQ EXP2(64), 18446744073709551616 ; Out of range
#else
#endif ;XASMAVR

#ifdef XASMAVR
        .DB LOG2(0),                    64 ; Illegal, out of range
        .DB LOG2(1),                     0
        .DB LOG2(128),                   7
        .DB LOG2(32768),     		15
        .DB LOG2(65536),     		16
        .DB LOG2(8388608),     		23
        .DB LOG2(16777216),     	24
        .DB LOG2(2147483648),     	31
        .DB LOG2(4294967296),     	32
        .DB LOG2(549755813888),     	39
        .DB LOG2(140737488355328),      47
        .DB LOG2(36028797018963968),    55
        .DB LOG2(9223372036854775808),  63
        .DB LOG2(18446744073709551616),	64
#else                         			
#endif ;XASMAVR

; -------------------------------------------
#ifdef XASMAVR ;
.SUBTTL Floating point 1.7 and absolute value
.EJECT
; -------------------------------------------
	.DW     INT(1.780029)
        .DW     INT(-1.780029)

        .DD     FRAC(1.780000)          ;; = 780000 
	.DW     FRAC(1.780000) & 0xFFFF ;; =  59104(!)

        .DB     Q7(0.575), Q7(1.575), Q7(-0.575), Q7(-1.575)
        .DW     Q15(0.575)     
        .DB     Q7 (0.390625),  Q7(1.390625)  
        .DW     Q15(1.390625)  
        .DB     Q7 (-0.390625), Q7(-1.390625) 
        .DW     Q15(-0.390625) 
        .DB     Q7 (0.609375),  Q7(1.609375)  
        .DW     Q15(0.609375)  
        .DB     Q7 (0.85), Q7(1.85)   ;; = $6C (->0.6C = 1.110 1100)
        .DW     Q15(1.85)             ;; = $EC (->1.6C = 1.110 1100)


        .DB     Q7(0.625), Q7(-0.625) ;; = $50,  (NOT $50) +1
        .DW     Q7(1.375)             

        .DW     Q7(1.8125)            ;; = $E8
        .DB     Q7(1.0), Q7(-1.0)     ;; = $80, $80
        .DW     Q15(-1.0), Q15(1.0)   ;; = $8000, $8000

        .DB     Q7 (0.78), Q7(1.78)
        .DW     Q15(1.780029)   ;; = (nearest representation of 1.78 in memory)
        .DW     Q15(0.780029)
        .DW     Q15(1.86478)
	.DD     Q15(0.321117799673)   ;; = $291A (=10522)

	.DB     Q7(0.9921875), Q7(1.9921875), Q7(-0.9921875), Q7(-1.9921875)
	.DW     Q15(0.999969482421875)

 ; Absolute value of a constant expression
        .DW     ABS(123)        ;; = |$7B|
        .DW     ABS(-123)       ;; = |$7B|

 ; String length in bytes and in words
	.DB	STRLEN_IN_BYTES, STRLEN_IN_WORDS
.EQU STRLEN_IN_BYTES = STRLEN("123456789012345678901234567890123")
.EQU STRLEN_IN_WORDS = STRLEN_IN_BYTES / 2 + STRLEN_IN_BYTES % 2
      	.DB STRLEN("a"), STRLEN("")

#else                         			
#endif ;XASMAVR

; -------------------------------------------
