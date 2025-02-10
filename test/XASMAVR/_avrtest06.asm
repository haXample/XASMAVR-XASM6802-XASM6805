;#define XASMAVR via commandline option '/DXASMAVR'
;----------------------------------,
#ifdef XASMAVR                     ;
 .TITLE Test: Conditional assembly
 .PAGELENGTH(84)                   ;
 .PAGEWIDTH(120)                   ;
 .SYMBOLS                          ;
 .MODEL BYTE                       ;
 .DEVICE ATxmega384C3              ;
#else                              ;
 .DEVICE ATmega2560                ;
#endif ;XASMAVR                    ;
;----------------------------------'
;;ha;;#define SUPPRESS ;; Suppress all
#ifndef SUPPRESS

.ifndef FOO1                            ;;ha;;                .ifndef FOO1
  nop         ; some code here          ;;ha;;  000000 0000     nop ; some code here
  .DW label1                            ;;ha;;  000001 0005     .dw label1
  .DW FOO1                              ;;ha;;  000002 0111     .dw FOO1
.endif                                  ;;ha;;                .endif
  .DW label1                            ;;ha;;  000003 0005     .dw label1
  rjmp label1 ; more code               ;;ha;;  000004 c000     rjmp label1 ; more code here
  .EQU FOO1 = $100                      ;;ha;;  = 00000100      .equ FOO1 = $100
  label1: nop                           ;;ha;;  000005 0000     label1: nop
;------------------------------         ;;ha;;                ;------------------------------

.ifdef FOO2                             ;;ha;;                .ifdef FOO2
  nop         ; some code here          ;;ha;;
  .dw label2                            ;;ha;;
  .dw FOO2                              ;;ha;;                
.endif                                  ;;ha;;                .endif
  .dw label2                            ;;ha;;  000006 0008     .dw label2
  rjmp label2 ; more code here          ;;ha;;  000007 c000     rjmp label2 ; more code here
  .EQU FOO2 = $200                      ;;ha;;  = 00000200      .equ FOO2 = $200
  label2: nop                           ;;ha;;  000008 0000     label2: nop
;------------------------------         ;;ha;;                ;------------------------------

.ifdef FOO3                             ;;ha;;                .ifdef FOO3
  nop         ; some code here          ;;ha;;
  .DW label3                            ;;ha;;
  .DW FOO3                              ;;ha;;
  rjmp label3 ; more code here          ;;ha;;
  .EQU FOO3 = $300                      ;;ha;;
  label3: nop                           ;;ha;;
.endif                                  ;;ha;;                .endif                            
;------------------------------         ;;ha;;                ;------------------------------

.ifndef FOO4                            ;;ha;;                .ifndef FOO4
  nop         ; some code here          ;;ha;;  000009 0000    nop         ; some code here
  .DW LABEL4                            ;;ha;;  00000a 000d    .DW LABEL4
  .DW FOO4                              ;;ha;;  00000b 0400    .DW FOO4
  rjmp label4 ; more code here          ;;ha;;  00000c c000    rjmp label4 ; more code here
  .EQU FOO4 = $400                      ;;ha;;  = 00000400     .EQU FOO4 = $400
  label4: nop                           ;;ha;;  00000d 0000    label4: nop
.endif                                  ;;ha;;                .endif
;------------------------------         ;;ha;;                ;------------------------------

; .IF.IFDEF.IFNDEF.ELIF.ELSE.ENDIF
.IF 1                                   ;;ha;;           .IF 1
  .DB     $11,$AA,$BB,255               ;;ha;;  000000 11AABBFF .DB $11,$AA,$BB,255
  .IF 0                                 ;;ha;;             .IF 0                 
    .DB     $22,$AA                     ;;ha;;             .ELIF 1                 ;=.ELSEIF a
  .ELIF 1               ;=.ELSEIF a     ;;ha;;  000002 0155     .DB $01,$55
    .DB     $01,$55                     ;;ha;;  
  .ELIF 1               ;=.ELSEIF b     ;;ha;;             .ELIF 1                 ;=.ELSEIF b
    .DB     $02,$56                     ;;ha;;             .ENDIF ;.IF 0
  .ELIF 1               ;=.ELSEIF c     ;;ha;;          .ENDIF ;.IF 1
    .DB     $03,$57                     ;;ha;;          ;------------------------------------
  .ELSE                 ;=.ELSE
    .DB     $04,$66                                      
    .IF 3                                                
      .DB     $33,$A1                                    
      .IF 4                                              
        .DB     $44,$A2                                  
        .IF 5                                            
          .DB     $55,$A3                                
          .IF 6                                          
            .DB     $66,$A4                              
          .ENDIF                                         
        .ENDIF                                           
      .ENDIF                                             
    .ENDIF
  .ENDIF
.ENDIF
;---------------------------------------

.IF 2                                   ;;ha;;           .IF 2                           
  .DB     $11,$AA,15,255                ;;ha;;  000003 11AA0FFF .DB $11,$AA,15,255
  .IF 0                                 ;;ha;;  
    .DB     $22,$AA,15,255              ;;ha;;             .IF 0                         
  .ELIF 1                  ;=.ELSEIF a  ;;ha;;             .ELIF 1                 ;=.ELSEIF a
    .DB     $01,$55,15,255              ;;ha;;  000005 01550FFF .DB $01,$55,15,255
  .ELIF 0                  ;=.ELSEIF b  ;;ha;;  
    .DB     $02,$56,15,255              ;;ha;;             .ELIF 0                 ;=.ELSEIF b
  .ELIF 0                  ;=.ELSEIF c  ;;ha;;             .ENDIF ;.IF 0
    .DB     $03,$57,15,255              ;;ha;;           .ENDIF ;.IF 2
  .ELSE                    ;=.ELSE      ;;ha;;           ;-----------------------------------
    .DB     $04,$66,15,255
    .IF 3
      .DB     $33,$A1,15,255
      .IF 4
        .DB     $44,$A2,15,255
        .IF 5
          .DB     $55,$A3,15,255
          .IF 6
            .DB     $66,$A4,15,255
          .ENDIF
        .ENDIF
      .ENDIF
    .ENDIF
  .ENDIF
.ENDIF
; -------------------------------------------

.IFNDEF CPU_Clock                       
        .DW     LWRD(CPU_Clock)         
        .EQU    CPU_Clock = 25000000    
.ENDIF                                  
.IFDEF CPU_Clock                        
        .DW     LWRD(CPU_Clock)
.ENDIF                                  
.IFNDEF CPU_Clock                       
        .DW     LWRD(CPU_Clock)
.ENDIF                                  
        .SET    CPU_Clock1 = 100000             
        .DW     LWRD(CPU_Clock1)
.IFNDEF CPU_Clock1                      
        .DW     LWRD(CPU_Clock1)
        .EQU    CPU_Clock1 = 16000000   
.ENDIF                                  
        .DW     LWRD(CPU_Clock)
        .DW     LWRD(CPU_Clock1)
; -------------------------------------------

.SET freqHz   = 7372800                   
.SET cyclTime = 1000000000 / freqHz     

.SET cycles = ((1+cyclTime-1) / cyclTime-0) ;; +1 
#if (cycles > (255 * 3 + 2))
  .WARNING "cycles > 767"
#else                                       ;; #else - #if
  #if (cycles > 6)       
    .SET  loop_cycles = (cycles / 3)
        ldi   R16, cyclCount
        dec   R16
        brne  PC-1
    .SET  cycles = (cycles - (cyclCount * 3))
  #endif
  #if (cycles > 0)                          ;; #if - #if
    #if (cycles & 4)
        rjmp  PC+1
        rjmp  PC+1
    #endif
    #if (cycles & 2)
        rjmp  PC+1
    #endif
    #if (cycles & 1)
        nop
    #endif
  #endif
#endif

; -------------------------------------------

.SET cycles = ((1+cyclTime-1) / cyclTime-0) ;; +1 
.if (cycles > (255 * 3 + 2))
  .WARNING "cycles > 767"
.elif (cycles > 6)                          ;; #elif
  .SET  cyclCount = (cycles / 3)
        ldi   R16, cyclCount
        dec   R16
        brne  PC-1
  .SET  cycles = (cycles - (cyclCount * 3))
.elif (cycles > 0)                          ;; #elif - #if
  .if (cycles & 4)
        rjmp  PC+1
        rjmp  PC+1
  .endif
  .if (cycles & 2)
        rjmp  PC+1
  .endif
.elif (cycles & 1)                          ;; #elif
        nop
.endif
; -------------------------------------------

.SET cycles = ((1+cyclTime-1) / cyclTime-2)  ;; -2
        .DW cycles
        .DW (cycles > 0)
.if (cycles > (255 * 3 + 2))                                     
  .WARNING "cycles > 767"
.ELIF (cycles > 6)
  .SET cyclCount = (cycles / 3) 
        ldi   R16, cyclCount
        dec   R16
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

; -------------------------------------------
#ifdef XASMAVR ;
.SUBTTL Test: #undef - re-#define
.EJECT
#endif ;XASMAVR
; -------------------------------------------
  ;;----------------
  #define symbol123  ; #define
  #define symbol456
  #define symbol789
  ;;----------------
  #ifdef symbol123   ; before #undef
        nop
  #endif 
  #ifdef symbol456   ; before #undef
        nop
  #endif 
  #ifdef symbol789   ; before #undef
        nop
  #endif 

  ;----------------
  #ifndef symbol123  ; before #undef
  .WARNING "#define symbol123 was ignored."
        nop
  #endif 
  #ifndef symbol456  ; before #undef
  .WARNING "#define symbol456 was ignored."
        nop
  #endif 
  #ifndef symbol789  ; before #undef
  .WARNING "#define symbol789 was ignored."
        nop
  #endif 

;#ifdef XASMAVR
  ;;----------------
  #undef symbol123   ; #undef
  #undef symbol456
  #undef symbol789
  ;;----------------
  #ifdef symbol123   ; after #undef
  .WARNING "#undef symbol123 was ignored."
        nop
  #endif 
  #ifdef symbol456   ; after #undef
  .WARNING "#undef symbol456 was ignored."
        nop
  #endif 
  #ifdef symbol789   ; after #undef
  .WARNING "#undef symbol789 was ignored."
        nop
  #endif 

  ;----------------
  #ifndef symbol123  ; after #undef
        nop
  #endif 
  #ifndef symbol456  ; after #undef
        nop
  #endif 
  #ifndef symbol789  ; after #undef
        nop
  #endif 
;#endif ;XASMAVR

#ifdef XASMAVR
  ;; #undef'd symbols #define'd again
  ;;---------------
  #define symbol123
  #define symbol456
  #define symbol789
  ;;---------------
  #ifdef symbol123   ; after #define
        nop
  #endif 
  #ifdef symbol456   ; after #define
        nop
  #endif 
  #ifdef symbol789   ; after #define
        nop
  #endif 

  ;----------------
  #ifndef symbol123  ; after #define
  .WARNING "re-#define symbol123 was ignored."
        nop
  #endif 
  #ifndef symbol456  ; after #define
  .WARNING "re-#define symbol456 was ignored."
        nop
  #endif 
  #ifndef symbol789  ; after #define
  .WARNING "re-#define symbol789 was ignored."
        nop
  #endif 

#endif ;XASMAVR

#endif ;SUPPRESS
.EXIT



