#define	XASM6802
;------------------------------------------,
#ifdef XASM6802			           ;
 .TITLE Test: MC6800/6802 Instruction set
 .PAGELENGTH(84)		           ;
 .PAGEWIDTH(130)		           ;
 .SYMBOLS			           ;
#endif ;XASM6802	   	           ;
;------------------------------------------'
.LISTMAC

;------------------------------------------------------------------------------
;
; Example: MC6802 Minimum System Layout
;
; Working storage definitions (Direct Page $0..$FF)
;
RAMSTART	EQU	$0000	;User scratchpad RAM
RAM_SIZE	SET	128

STACK		EQU	RAMSTART+(RAM_SIZE-STACK_SIZE)
STACK_SIZE	EQU	32	;Stack size (64 bytes)

ROMSTART	EQU	$1000	;ROM Start
ROM_SIZE	EQU	1024	;1K 

RESET		EQU	ROMSTART+ROM_SIZE-2

.MACRO miniSystem
	ORG RAMSTART
	RMB	RAM_SIZE - STACK_SIZE
;      	. . . . . . . . . . . . . . . . . . . . . . . . . .
	ORG STACK	    ; Program STACK ($00C0 .. 00FF)
	RMB	STACK_SIZE  ; 64 bytes user stack area
;	. . . . . . . . . . . . . . . . . . . . . . . . . . 

RAMEND	EQU  	*   ; $007F last location of on-chip-RAM
;
	ORG ROMSTART
.ENDM

;------------------------------------------------------------------------------
;
	MINISYSTEM
;
;			Info String
info	FCB	"MC6800/6802 Instruction Set"
INFO_LEN EQU * - info
		 
_234567890123456789012345678901234567890: ; 40 chars max:	     

.EJECT
;------------------------------------------------------------------------------
;
;		 MC6800 Instruction Set
;
MC6800_instructions:
	ABA			;= 1B

 	ADCA 	#$25		;= 89
 	ADCA 	$25		;= 99
 	ADCA 	$10,X		;= A9  
 	ADCA 	STACK_SIZE,X	;= A9  
 	ADCA 	,X		;= A9  
 	ADCA 	X		;= A9  
 	ADCA 	ROMSTART	;= B9

 	ADCB 	#$25	;= C9
 	ADCB 	$25	;= D9
 	ADCB 	$10,X	;= E9
 	ADCB 	$1000	;= F9

 	ADDA 	#$25	;= 8B
 	ADDA 	$25	;= 9B
 	ADDA 	$10,X	;= AB  
 	ADDA 	$1000	;= BB

 	ADDB 	#$25	;= CB
 	ADDB 	$25	;= DB
 	ADDB 	$10,X	;= EB
 	ADDB 	$1000	;= FB

 	ANDA 	#$25	;= 84
 	ANDA 	$25	;= 94
 	ANDA 	$10,X	;= A4  
 	ANDA 	$1000	;= B4
 	ANDB 	#$25	;= C4
 	ANDB 	$25	;= D4
 	ANDB 	$10,X	;= E4
 	ANDB 	$1000	;= F4

	ASLA		;= 48
	ASLB		;= 58
	ASL 	$10,X  	;= 68
	ASL 	$1000  	;= 78

	ASRA		;= 47
	ASRB		;= 57
	ASR 	$10,X  	;= 67
	ASR 	$1000  	;= 77

 	BCC	*+2	;= 24
 	BCS	*+2	;= 25
 	BCC	MC6800_instructions	;= 24
 	BCS	MC6800_instructions	;= 25
 	BEQ	*	;= 27
 	BGE	*+2	;= 2C
	BGT	*	;= 2E
 	BHI	*+2	;= 22
 	BHI	_l1	;= 22
 	
 	BITA 	#$25	;= 85
 	BITA 	$25	;= 95
	BITA 	$10,X	;= A5  
 	BITA 	$1000	;= B5
 	BITB 	#$25	;= C5
 	BITB 	$25	;= D5
	BITB 	$10,X	;= E5
 	BITB 	$1000	;= F5
 	
 	BLE	*+2	;= 2F
 	BLS	*+2	;= 23
	BLT	*+2	;= 2D
 	BMI	*+2	;= 2B
 	BNE	*+2	;= 26
 	BPL	*+15	;= 2A
	BRA	*+15	;= 20
 	BSR	*+15	;= 8D
 	BVC	*+15	;= 28
 	BVS	*+15	;= 29
 	
.EJECT
_l1:	CBA		;= 11
	CLC		;= 0C
	CLI		;= 0E

 	CLRA		;= 4F
 	CLRB		;= 5F
 	CLR 	$10,X	;= 6F
 	CLR 	$1000	;= 7F
	
 	CLV		;= 0A
 	
 	CMPA 	#$25	;= 81
 	CMPA 	$25	;= 91
	CMPA 	$10,X	;= A1  
 	CMPA 	$1000	;= B1
 	CMPB 	#$25	;= C1
 	CMPB 	$25	;= D1
	CMPB 	$10,X	;= E1
 	CMPB 	$1000	;= F1
 	
 	COMA		;= 43
 	COMB		;= 53
	COM 	$10,X	;= 63
 	COM 	$1000	;= 73
 	
 	CPX 	#$25	;= 8C
 	CPX 	$25	;= 9C
	CPX 	$10,X	;= AC  
 	CPX 	$1000	;= BC
 	
 	DAA		;= 19
 	
	DECA		;= 4A
	DECB		;= 5A
	DEC 	$10,X	;= 6A
	DEC 	$1000	;= 7A

 	DES		;= 34
 	DEX		;= 09
 	
.EJECT
 	EORA 	#$25	;= 88
	EORA 	$25	;= 98
 	EORA 	$10,X	;= A8  
 	EORA 	$1000	;= B8
 	EORB 	#$25	;= C8
	EORB 	$25	;= D8
 	EORB 	$10,X	;= E8
 	EORB 	$1000	;= F8
 		
 	INCA		;= 4C
	INCB		;= 5C
 	INC 	$10,X	;= 6C
 	INC 	$1000	;= 7C
 	
 	INS		;= 31
	INX		;= 08
 	
 	JMP	$10,X	;= 6E
 	JMP	$1000	;= 7E
 	JMP	procRts	;= 7E
 	
	JSR	$10,X 	;= AD
 	JSR	$1000	;= BD
 	JSR	procRts	;= BD
 	
 	LDAA 	#$25	;= 86
 	LDAA 	$25	;= 96
	LDAA 	$10,X	;= A6  
	LDAA 	$1000	;= B6  
 	LDAB 	#$25	;= C6
 	LDAB 	$25	;= D6
 	LDAB 	$10,X	;= E6
 	LDAB 	$1000	;= F6

 	LDS 	#$1000	;= 8E
 	LDS 	$25	;= 9E
 	LDS 	$10,X	;= AE 
 	LDS 	$1000	;= BE

 	LDX 	#$1000	;= CE
 	LDX 	$25	;= DE
 	LDX 	$10,X	;= EE
 	LDX 	$1000	;= FE

 	LSRA		;= 44
 	LSRB		;= 54
 	LSR 	$10,X	;= 64
 	LSR 	$1000	;= 74
	
 	NEGA		;= 40
 	NEGB		;= 50
 	NEG 	$10,X	;= 60
 	NEG 	$1000	;= 70

 	NOP		;= 01
 	
 	ORAA 	#$25	;= 8A
 	ORAA 	$25	;= 9A
	ORAA 	$10,X	;= AA 
	ORAA 	$1000	;= BA
 	ORAB 	#$25	;= CA
 	ORAB 	$25	;= DA
 	ORAB 	$10,X	;= EA
 	ORAB 	$1000	;= FA

 	PSHA		;= 36
 	PSHB		;= 37
 	PULA		;= 32
 	PULB		;= 33

 	ROLA		;= 49
 	ROLB		;= 59
 	ROL 	$10,X	;= 69
 	ROL 	$1000	;= 79

 	RORA		;= 46
 	RORB		;= 56
 	ROR 	$10,X	;= 66
 	ROR 	$1000	;= 76

.EJECT
 	RTI		;= 3B

procRts EQU *
 	RTS		;= 39

 	SBA		;= 10
 	
	SBCA 	#$25	;= 82
 	SBCA 	$25	;= 92
 	SBCA 	$10,X	;= A2  
 	SBCA 	$1000	;= B2
 	
	SBCB 	#$25	;= C2
	SBCB 	$25	;= D2
	SBCB 	$10,X	;= E2
	SBCB 	$1000	;= F2

 	SEC		;= 0D
 	SEI		;= 0F
 	SEV		;= 0B
 	
	STAA 	$10,X	;= 97
 	STAA 	$25	;= A7
 	STAA 	$1000	;= B7
 	
 	STAB 	$10,X	;= D7
	STAB 	$25	;= E7
 	STAB 	$1000	;= F7
 	
 	STS 	$10,X	;= 9F
 	STS 	$25	;= AF
	STS 	$1000	;= BF
 	
 	STX 	$10,X	;= DF
 	STX 	$25	;= EF
 	STX 	$1000	;= FF

 	SUBA 	#$25	;= 80
 	SUBA 	$25	;= 90
 	SUBA 	$10,X	;= A0   
 	SUBA 	$1000	;= B0

 	SUBB 	#$25	;= C0
 	SUBB 	$25	;= D0
 	SUBB 	$10,X	;= E0
 	SUBB 	$1000	;= F0
	
	SWI		;= 3F
	TAB		;= 16
	TAP		;= 06
	TBA		;= 17
	TPA		;= 07
		
	TSTA		;= 4D
	TSTB		;= 5D
	TST 	$10,X	;= 6D
	TST 	$1000	;= 7D

	TSX		;= 30
	TXS		;= 35
	WAI		;= 3E

INSTRUCTIONS_END EQU *  ; last location of used ROM

.SUBTTL Code Examples
.EJECT
;----------------------------------------------------------------------------
;
        FCB LOW  ($89ABCDEF), $89ABCDEF & $EF         		; Bits [ 7: 0]
        FCB HIGH ($89ABCDEF), ($89ABCDEF >>  8) & $FF 		; Bits [15: 8]
        FCB BYTE2($89ABCDEF), ($89ABCDEF >>  8) & $FF 		; Bits [15: 8]
        FCB BYTE3($89ABCDEF), ($89ABCDEF >> 16) & $FF 		; Bits [23:16]
        FCB BYTE4($89ABCDEF), ($89ABCDEF >> 24) & $ff 		; Bits [31:24]
        FDB LWRD ($89ABCDEF), $89ABCDEF & $FFFF       		; Bits [15: 0]	    
        FDB HWRD ($89ABCDEF), $89ABCDEF >> 16	         	; Bits [31:16]	         
	FDB PAGE ($12345678), ($12345678 & $003F0000) >> 16	; Bits [21:16]             

_fcb01c	FCB  	_FCB01C_LENGTH, 1,13,'012"3":; ,"', '", "', 'A', '"', ''', '', "";, 22, 25, 4 \
_FCB01C_LENGTH EQU * - _fcb01c

_11 SET $11
_12 SET $12
_13 SET $13
_fcb3:   FCB	_11,_12, _13, 'Z'+1, \
		 %10101, %1, %101,   \
                 "1234567890abcd ",  \
                 'A',_fcb3 AND $FF, \
                 "1234567890abcde",  \
                 $FF,$FF

_fcb09	FCB	__DATE__, __TIME__, __CENTURY__

_fcb10	FCB	__DATE__, __TIME__, '__CENTURY__', \
		$00

_FDB1	FDB 1
_FDW12:	FDW 12
.EJECT
;----------------------------------------------------------------------------
;
	FDB 	STRLEN("XASMAVR Macro Assembler V2.1") 

	FCB EXP2(0),  1		  
	FCB EXP2(7),  128	  
	FDB EXP2(8),  256	  
	FDB EXP2(15), 32768                                                             
	FDW EXP2(16), 65536                                                            
	FDW EXP2(23), 8388608	  
	FDW EXP2(24), 16777216 
	FDW EXP2(31), 2147483648  
	
        FCB LOG2(0),                    64 ; Illegal, out of range
        FCB LOG2(1),                     0
        FCB LOG2(128),                   7
        FCB LOG2(32768),     		15
        FCB LOG2(65536),     		16
        FCB LOG2(8388608),     		23
        FCB LOG2(16777216),     	24
        FCB LOG2(2147483648),     	31
        FCB LOG2(4294967296),     	32
        FCB LOG2(549755813888),     	39
        FCB LOG2(140737488355328),      47
        FCB LOG2(36028797018963968),    55
        FCB LOG2(9223372036854775808),  63
        FCB LOG2(18446744073709551616),	64
	
	
#define _flag1
.DEFINE _flag2
	FCB DEFINED(_flag1), !DEFINED(_flag1)
	FCB DEFINED(_flag2), !DEFINED(_flag2)

#if DEFINED(_flag1)
	nop	      	; flag1
#elif DEFINED(_flag2)
#endif

#if !DEFINED(_flag1)	
#elif DEFINED(_flag2)
	nop		; flag2	
#endif
	
;-----------------------------------------------------------------,
; Warning: Use parenthesis in complex expressions!		  |
_VAR SET $a600*256+$75a2>>8	     ;; =00A60075 NOT EXPECTED ?! |
	FDB _VAR		     ;                            |
_VAR SET ($a600*256)+($75a2>>8)      ;; =00a60075 expected        |
	FDB LWRD(_VAR)               ;	                          |
	FDB HWRD(_VAR)               ;	                          |
_VAR SET ($a600*256+$75a2)>>8        ;; =0000a675 expected        |
	FDB _VAR                     ;	                          |
;-----------------------------------------------------------------'
.EJECT
;----------------------------------------------------------------------------
;
;	Decimal subtract subroutine for 16 decimal digit
;
SUBTRH	EQU	 0
MINUEND	EQU 	 8
RSLT	EQU	16

	ORG 	$1300

procDsub EQU *
 	ldx	#8
dsub1:	ldaa	#$99
	suba 	SUBTRH, X
.EVEN
	staa 	RSLT, X
	dex
	bne 	dsub1
	ldx	#8
	sec
dsub2: 	ldaa 	MINUEND, X
	adca 	RSLT, X
	daa
	staa 	RSLT, X
	dex
	bne	dsub2
	rts

;------------------------------------------------------------------------------
;
;	Macros
;
.MACRO Addition 
	ldaa 	@0	; param0		
	adda 	@1	; param1
.ENDM

	ADDITION $10, 20
	STAA 	$100 
	ADDITION $12, 15
	STAA 	$101
.EVEN
	SWI

;------------------------------------------------------------------------------

	ORG 	RESET
	FDB	MC6800_instructions

ROMEND	EQU  	*   	; $13FF last location of ROM

.IF ROMEND > (ROMSTART+ROM_SIZE)
  .WARNING "Rom size overflow"
.ENDIF
;------------------------------------------------------------------------------
.EJECT
	END