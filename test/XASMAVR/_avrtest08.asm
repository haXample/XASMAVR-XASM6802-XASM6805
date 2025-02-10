;#define XASMAVR via commandline option '/DXASMAVR'
;----------------------------------,
#ifdef XASMAVR			   ;
 .TITLE Test: Macros		   ;
 .PAGELENGTH(84)		   ;
 .PAGEWIDTH(130)		   ;
 .SYMBOLS			   ;
 .MODEL BYTE 			   ;
; .MODEL WORD 			   ;
 .DEVICE ATxmega384C3		   ;
#else				   ;
 .DEVICE ATmega2560		   ;
 .OVERLAP			   ;
#endif ;XASMAVR		   	   ;
;----------------------------------'
.LISTMAC

.MACRO Align  	; align labels to specific boundaries.
alignfromhere:
         .ORG  (PC+(@0)) - ((PC+(@0)) % (@0))
.ENDMACRO

;----
.DSEG
;----
_sramStart:
	.BYTE 1
_sr1:
#ifdef XASMAVR
	.EVEN	; = if (PC % 2) silently skip over 1 byte (=0)'
#endif ;XASMAVR
_sr2:

;----
.CSEG
;----
_romStart:
	.DB $01,$23
_r1:
	ALIGN 16
	.DB $45,$67
_r2:
	ALIGN EXP2(4)
	.DB $89,$AB
_r3:
#ifdef XASMAVR
	.EVEN	; = if (PC % 2) silently insert a 'nop instruction (=0)'
#endif ;XASMAVR
	.DB $CD,$EF
_r4:

;----
.ESEG
;----
_eepromStart:
	.DB $01;,$23
#ifdef XASMAVR
	ALIGN 16
#endif ;XASMAVR
	.DB $45;,$67
_ee2:
#ifdef XASMAVR
	ALIGN EXP2(4)
#endif ;XASMAVR
	.DB $89;,$AB
_ee3:
#ifdef XASMAVR
	.EVEN	; = if (PC % 2)  silently insert a 'nop instruction (=0)'
#endif ;XASMAVR
_ee4:
	.DB $CD;,$EF
_ee5:
#ifdef XASMAVR
	.EVEN	; = if (PC % 2)  silently insert a 'nop instruction (=0)'
#endif ;XASMAVR
_ee6:

;----
.CSEG
;----
.MACRO newPage
  #ifdef XASMAVR
   .SUBTTL @0
   .EJECT
  #endif ;XASMAVR
.ENDM				       
.org $100

; Macro Test (Macro(s) within Macro and local Macro labels)
.MACRO wrInit 	;; Macro #1
#ifdef XASMAVR
mtst:	jmp	mtst    ;; local label test
#else							 
	jmp	mtst
#endif ;XASMAVR
	call	init							 
	call	cmd							 
       	call	_delay
       	jmp	delay
	.DW	LOW(cmd+init), HIGH(init) 
	.DW     cmd, cmd+Init+_delay 
	WRCMD (1<<_F)|(0<<_F_8B)                                       
 cmd:   WRCMD (1<<_F)|(0<<_F_8B)|(1<<_F_2L)                            
	WRCMD (1<<_CLR)                                                
_delay:	WRCMD (1<<_ENTRY_MODE)|(1<<_ENTRY_INC)                         
	WRCMD (1<<_ON)|(1<<_ON_DISPLAY)|(0<<_ON_CURSOR)|(0<<_ON_BLINK) 
 init:  WRCMD (1<<_HOME)						 
.ENDM

.MACRO wrCmd    ;;Macro #2
       	ldi	R17, @0
 wrc: 	call	cmd
        ROUT 10, R17, 9                                                      
.ENDM

.MACRO rOut	;;Macro #3					        
    	ldi 	@1, @2
 rou: 	out 	@0, @1
.ENDMACRO
;;----------------------------------------------------------------

.EQU    _CLR             = 0b00000000 ; 
.EQU    _HOME            = 0b00000001 ; 
.EQU    _ENTRY_INC       = 0b00000001 ; 
.EQU    _ENTRY_MODE      = 0b00000010 ; 

.EQU    _ON_BLINK        = 0b00000000 ; 
.EQU    _ON_CURSOR       = 0b00000001 ; 
.EQU    _ON_DISPLAY      = 0b00000010 ; 
.EQU    _ON              = 0b00000011 ; 

.EQU    _F_2L            = 0b00000011 ; 
.EQU    _F_8B            = 0b00000100 ; 
.EQU    _F               = 0b00000101 ; 

	call	init
	call	delay
	call	cmd

mtst:   WRINIT ;; Macro expansion #1
;	------

    	WRINIT ;; Macro expansion #2
;	------
#ifdef XASMAVR
.NOLISTMAC
#endif ;XASMAVR
    	WRINIT ;; Macro expansion #3
;	------
init:	ret                                                         
delay:	ret                    
cmd:	ret

; -------------------------------------------
	NEWPAGE	Macro Expansion Test 2
; -------------------------------------------
.SET freqHz   = 7372800                   
.SET cyclTime = 1000000000 / freqHz     
.SET cycles   = ((1 + cyclTime - 1) / cyclTime - 0)
	.DW cycles ;= 1
.SET cycles   = ((1 + cyclTime - 1) / cyclTime - 2)
	.DW cycles ;= -1

	.MACRO nsWaitA
      	.set cycles = ((@0 + cyclTime - 1) / cyclTime - @1)
    		.DW cycles
    		.DW (cycles > 0)
      	.if (cycles > (255 * 3 + 2))
          .WARNING "MACRO nsWait - too many cycles"
      	.else
          .if (cycles > 6)
            .SET cyclCount = (cycles / 3) 
          	ldi   @2, cyclCount
          	dec   @2
          	brne  PC-1
            .SET cycles = (cycles - (cyclCount * 3))
	    	.DW cycles
          .endif
          .if (cycles > 0)
    		.DW (cycles > 0)
            .if (cycles & 4)
            	rjmp  PC+1
            	rjmp  PC+1
            .elif (cycles > 0)
            .endif
            .if (cycles & 2)
            	rjmp  PC+1
            .endif
            .if (cycles & 1)
            	nop
            .endif
          .endif
        .endif
	.ENDMACRO

; -------------------------------------------
	NEWPAGE	Macro Expansion Test 3
; -------------------------------------------
	.MACRO nsWaitB
      	.SET cycles = ((@0 + cyclTime - 1) / cyclTime - @1)
    		.DW cycles
    		.DW (cycles > 0x00)
      	.if (cycles > (255 * 3 + 2))
          .WARNING "cycles > 767"
      	.ELIF (cycles > 6)
          .SET cyclCount = (cycles / 3) 
          	ldi   @2, cyclCount
          	dec   @2
          	brne  PC-1
          .SET cycles = (cycles - (cyclCount * 3))
	    	.DW cycles
        .ELIF (cycles > $00)
    		.DW (cycles > $00)
          .IF (cycles & 4)
            	rjmp  PC+1
            	rjmp  PC+1
          .ELIF (cycles > 0)
          .ENDIF
          .IF (cycles & 2)
            	rjmp  PC+1
          .ENDIF
          .IF (cycles & 1)
            	nop
          .ENDIF
        .ENDIF
	.ENDMACRO
; ----------------------------------------------------------

	NSWAITA 1, 0, R17 ;; Macro (A) expansion cycles = +1
;	-------

	NSWAITA 1, 2, R17 ;; Macro (A) expansion cycles = -1
;	-------
; ----------------------------------------------------------

	NSWAITB 1, 0, R17 ;; Macro (B) expansion cycles = +1
;	-------

 	NSWAITB 1, 2, R17 ;; Macro (B) expansion cycles = -1
;	-------
; ----------------------------------------------------------
.EXIT                    
