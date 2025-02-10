#define XASM6805
;------------------------------------------,
#ifdef XASM6805                            ;
 .TITLE Test: MC68HC05 Instruction set
 .PAGELENGTH(84)                           ;
 .PAGEWIDTH(130)                           ;
 .SYMBOLS                                  ;
#endif ;XASM6805                           ;
;------------------------------------------'
.LISTMAC

;------------------------------------------------------------------------------
;
; Example:      MC6805U3 Hardware Equates 
;
; RAM Layout and working storage definitions (Direct Page $0..$FF)
;
; $0000F Memory mapped I/O area
IOAREA          EQU     $0000           ;Memory mapped I/O - 16 bytes
IOAREA_SIZE     EQU     16

; $0020..$003F = Reserved user scratchpad RAM (48 bytes)
RAMUSER         EQU     $0020           ;Reserved user RAM 48 bytes
RAMUSER_SIZE    EQU     48

; $0040..$007F = RAM (64 bytes)
RAMSTART        EQU     $0040           ;RAM (including stack) 64 bytes
RAM_SIZE        EQU     64

; $0078..$007F Stack area begin (within RAM)
STACK           EQU     RAMSTART+(RAM_SIZE-STACK_SIZE) ;Stack 8 bytes
STACK_SIZE      EQU     8

; $0080 User ROM (User ROM Page-Zero)
ROMPAGE0        EQU     $0080           ;User ROM (Page Zero) - 128 bytes
ROMPAGE0_SIZE   EQU     128             ;Access with short instructions (8bit addressing)

; $0080 Main User ROM
ROMSTART        EQU     $0100           ;User ROM (Page Zero) - 128 bytes
ROM_SIZE        EQU     4096-ROMSTART   ;Access with short instructions (8bit addressing)

; $0EFA Manufacturing Tests
TEST_VECTORS    EQU     $0EFA           ;Addresses to RAM/ROM test procedures
MANUF_MODE      EQU     $0F00           ;Manufacturing burn-in modes

; $0F38 Sel Check ROM
ROMCHECKSTART   EQU     $0F38           ;ROM checksum byte placed here
ROMCHECK_SIZE   EQU     192

; $0FF8 Timer Interrupt Vector
; $0FFA External Interrupt Vector
; $0FFC SWI
; $0FFE RESET
VECTORS         EQU     $0FF8           ;Interrupt Vector Area (end-of-ROM)
VECTORS_SIZE    EQU     4*2             ;4 * 16bit addresses

;--------------------------------------------------------------------------
;  7       6        5         4         3         2        1        0
;  ...     ...     _NUM_LED_ _SCR_LED_ _CAP_LED_ _KYBDAT_ _SYSCLK_ _KYBCLK_
;--------------------------------------------------------------------------
_KYBCLK_        EQU     0               ;Keyboard clock  (output)
_SYSCLK_        EQU     1               ;system clock    (input)
_KYBDAT_        EQU     2               ;Keyboard data   (output)
_CAP_LED_       EQU     3               ;Caps-lock LED   (output)
_SCR_LED_       EQU     4               ;Scroll-lock LED (output)
_NUM_LED_       EQU     5               ;Num-lock LED    (output)
;NC             EQU     6               ;NC - not connected
;NC             EQU     7               ;NC - not connected

KYBCLK          EQU     1 SHL _KYBCLK_  ;_KYBCLK_ byte value
SYSCLK          EQU     1 SHL _SYSCLK_  ;_SYSCLK_ byte value
KYBDAT          EQU     1 SHL _KYBDAT_  ;_SYSCLK_ byte value
CAP_LED         EQU     1 SHL _CAP_LED_ ;_CAP_LED_ byte value
SCR_LED         EQU     1 SHL _SCR_LED_ ;_SCR_LED_ byte value
NUM_LED         EQU     1 SHL _NUM_LED_ ;_NUM_LED_ byte value

;------------------------------------------------------------------------------
;
;       I/O Ports and Registers ($0000 .. $000F)
;
                ORG IOAREA

;ensure a byte value
PORTA_MASK EQU NOT (SCR_LED OR CAP_LED OR NUM_LED OR KYBCLK OR KYBDAT) AND $FF 

; $0000..0003 Ports A..D
PORTA           RMB     1       ;PortA i/o data register                        
PORTB           RMB     1       ;PortB i/o data register
PORTC           RMB     1       ;PortC i/o data register
PORTD           RMB     1       ;PortD input only

; $0004..0006 Port Data Direction A..C (write only)
DDRA            RMB     1       ;PortA data direction (bit0..bit7)
DDRB            RMB     1       ;PortB data direction (bit0..bit7)
DDRC            RMB     1       ;PortC 0ata direction (bit0..bit7)

;-------------------------------------------------------------------                                                       
;
;               User Data RAM Area (Example)
;
set3Modtbl      RMB     32      ;$22..$41
SET3MODTBL_L EQU * - set3Modtbl

                RMB     4       ;$42

mtxFld          RMB     16      ;$46 
MTXFLD_L EQU * - mtxFld         ;=16 bytes

tmpcnt          RMB     1       ;$56 temporary multi purpose location
cmdparm         RMB     1       ;$57 keyboard command parameter
respbyt         RMB     1       ;$58

mtxC            RMB     1       ;$59 port C DDR
mtxB            RMB     1       ;$5a port B DDR

specialkey      RMB     1       ;$5b
typmatic        RMB     1       ;$5c
altprint        RMB     1       ;$5d
delay_count     RMB     1       ;$5e

tmpaux1         RMB     1       ;$5f temporary auxiliary #1

lastchr         RMB     1       ;$60
mtrxSenseIndex  RMB     1       ;$61
mtrxSenseNew    RMB     1       ;$62
mtrxSenseOld    RMB     1       ;$63

tmpaux2         RMB     1       ;$64 temporary auxiliary #2
tmp_65          RMB     1       ;$65 
tmpaux3         RMB     1       ;$66 temporary auxiliary #3
                                
outbuf          RMB     16+1    ;$67..$77
OUTBUF_L EQU * - outbuf                         

;               . . . . . . . . . . . . . . . . . . . . . . . . . .
                ORG STACK               ; $78 Start of program STACK
                RMB    STACK_SIZE       ; 8 bytes user stack area
;               . . . . . . . . . . . . . . . . . . . . . . . . . . 
RAMEND          EQU     *               ; top_of_stack = end-of-RAM

.EJECT
;------------------------------------------------------------------------------
;
;                MC6805U3 4K User ROM  ($0100 .. 0FFF)
;
;                       Info String
;
        ORG     ROMPAGE0        
info    FCB     "MC68HC05 Instruction Set"
INFO_LEN EQU * - info

_234567890123456789012345678901234567890: ; 40 chars max:            
                 
;------------------------------------------------------------------------------
;
;                MC68HC05 Instruction Set
;
        ORG ROMSTART
MC68HC05_instructions:

        ; Add with Carry 
        ADC #$12                ;IMM A9 ii    2~
        ADC $12                 ;DIR B9 dd    3~
        ADC $1234               ;EXT C9 hh ll 4~
        ADC ,X                  ;IX  F9       3~
        ADC $12,X               ;IX1 E9 ff    4~
        ADC $1234,X             ;IX2 D9       5~
        
        ; Add without Carry
        ADD #$12                ;IMM AB ii    2~
        ADD tmpaux1             ;DIR BB dd    3~
        ADD $1234               ;EXT CB hh ll 4~
        ADD ,X                  ;IX  FB       3~
        ADD $12,X               ;IX1 EB ff    4~
        ADD $1234,X             ;IX2 DB       5~
        
        ; Logical AND
        AND #$12                ;IMM A4 ii    2~
        AND $12                 ;DIR B4 dd    3~
        AND $1234               ;EXT C4 hh ll 4~
        AND ,X                  ;IX  F4       3~
        AND $12,X               ;IX1 E4 ff    4~
        AND $1234,X             ;IX2 D4 ee ff 5~
        
        ; Arithmetic Shift Left
        ASLA                    ;INH (A) 48    3~
        ASLX                    ;INH (X) 58    3~
        ASL $12                 ;DIR     38 dd 5~
        ASL ,X                  ;IX      78    5~
        ASL $12,X               ;IX1     68 ff 6~
        
        ; Arithmetic Shift Right
        ASRA                    ;INH (A) 47    3~
        ASRX                    ;INH (X) 57    3~
        ASR $12                 ;DIR     37 dd 5~
        ASR ,X                  ;IX      77    5~
        ASR $12,X               ;IX1     67 ff 6~
        
.EJECT
        _l1:
        ; Branch if Carry Clear
        BCC _l1                 ;REL 24 rr 3~
        
        ; Clear Bit in Memory
        BCLR 0,$12              ;DIR (bit 0) 11 dd 5~
        BCLR 1,$12              ;DIR (bit 1) 13 dd 5~
        BCLR 2,$12              ;DIR (bit 2) 15 dd 5~
        BCLR 3,$12              ;DIR (bit 3) 17 dd 5~
        BCLR 4,$12              ;DIR (bit 4) 19 dd 5~
        BCLR 5,$12              ;DIR (bit 5) 1B dd 5~
        BCLR 6,$12              ;DIR (bit 6) 1D dd 5~
        BCLR 7,$12              ;DIR (bit 7) 1F dd 5~
        
        ; Branch if Carry Set
        BCS _l1                 ;REL 25 rr 3~
        ; Branch if Equal
        BEQ _l1                 ;REL 27 rr 3~
        ; Branch if Half Carry Clear
        BHCC *                  ;REL 28 rr 3~
        ; Branch if Half Carry Set
        BHCS * + 2              ;REL 29 rr 3~
        ; Branch if Higher
        BHI * + 2               ;REL 22 rr 3~
        ; Branch if Higher or Same  / Branch if Carry Clear
        BHS * + 2               ;REL 24 rr 3~
        BCC * + 2               ;REL 24 rr 3~
        ; Branch if Interrupt Pin is High
        BIH * + 2               ;REL 2F rr 3~
        ; Branch if Interrupt Pin is Low
        BIL * + 2               ;REL 2E rr 3~
        
        ; Bit Test Memory with Accumulator
        BIT #$12                ;IMM A5 ii    2~
        BIT $12                 ;DIR B5 dd    3~
        BIT $1234               ;EXT C5 hh ll 4~
        BIT ,X                  ;IX  F5       3~
        BIT $12,X               ;IX1 E5 ff    4~
        BIT $1234,X             ;IX2 D5 ee ff 5~
        
        ; Branch if Lower / Branch if Carry Set
        BLO * + 2               ;REL 25 rr 3~
        BCS * + 2               ;REL 25 rr 3~
        ; Branch if Lower or Same
        BLS * + 2               ;REL 23 rr 3~
        ; Branch if Interrupt Mask is Clear
        BMC * + 2               ;REL 2C rr 3~
        ; Branch if Minus
        BMI * + 2               ;REL 2B rr 3~
        ; Branch if Interrupt Mask is Set
        BMS * + 2               ;REL 2D rr 3~
        ; Branch if Not Equal
        BNE * + 2               ;REL 26 rr 3~
        ; Branch if Plus
        BPL * + 2               ;REL 2A rr 3~
        ; Branch Always
        BRA * + 2               ;REL 20 rr 3~
        
        ; Branch if Bit n is Clear
        BRCLR 0,$12,* + 2       ;DIR (bit 0) 01 dd rr 5~
        BRCLR 1,$12,* + 2       ;DIR (bit 1) 03 dd rr 5~
        BRCLR 2,$12,* + 2       ;DIR (bit 2) 05 dd rr 5~
        BRCLR 3,$12,* + 2       ;DIR (bit 3) 07 dd rr 5~
        BRCLR 4,$12,* + 2       ;DIR (bit 4) 09 dd rr 5~
        BRCLR 5,$12,* + 2       ;DIR (bit 5) OB dd rr 5~
        BRCLR 6,$12,* + 2       ;DIR (bit 6) OD dd rr 5~
        BRCLR 7,$12,* + 2       ;DIR (bit 7) OF dd rr 5~
        
        ; Branch Never
        BRN * + 2               ;REL 21 rr 3~
        
        ; Branch if Bit n is Set
        BRSET 0,$12,* + 2       ;DIR (bit 0) 00 dd rr 5~
        BRSET 1,$12,* + 2       ;DIR (bit 1) 02 dd rr 5~
        BRSET 2,$12,* + 2       ;DIR (bit 2) 04 dd rr 5~
        BRSET 3,$12,* + 2       ;DIR (bit 3) 06 dd rr 5~
        BRSET 4,$12,* + 2       ;DIR (bit 4) 08 dd rr 5~
        BRSET 5,$12,* + 2       ;DIR (bit 5) 0A dd rr 5~
        BRSET 6,$12,* + 2       ;DIR (bit 6) 0C dd rr 5~
        BRSET 7,$12,* + 2       ;DIR (bit 7) 0E dd rr 5~
        
        ; Set Bit in Memory
        BSET 0,$12              ;DIR (bit 0) 10 dd 5~
        BSET 1,$12              ;DIR (bit 1) 12 dd 5~
        BSET 2,$12              ;DIR (bit 2) 14 dd 5~
        BSET 3,$12              ;DIR (bit 3) 16 dd 5~
        BSET 4,$12              ;DIR (bit 4) 18 dd 5~
        BSET 5,$12              ;DIR (bit 5) 1A dd 5~
        BSET 6,$12              ;DIR (bit 6) 1C dd 5~
        BSET 7,$12              ;DIR (bit 7) 1E dd 5~
        
        ; Branch to Subroutine
        BSR * + 2               ;REL AD rr 6~
        
        ; Clear Carry Bit
        CLC                     ;INH 98 2~
        ; Clear Interrupt Mask Bit
        CLI                     ;INH 9A 2~
        
        ; Clear
        CLRA                    ;INH (A) 4F    3~
        CLRX                    ;INH (X) 5F    3~
        CLR $12                 ;DIR     3F dd 5~
        CLR ,X                  ;IX      7F    5~
        CLR $12,X               ;IX1     6F ff 6~
        
        ; Compare Accumulator with Memory
        CMP #$12                ;IMM A1 ii    2~
        CMP $12                 ;DIR B1 dd    3~
        CMP $1234               ;EXT C1 hh ll 4~
        CMP ,X                  ;IX  F1       3~
        CMP $12,X               ;IX1 E1 ff    4~
        CMP $1234,X             ;IX2 D1 ee ff 5~
        
        ; Complement
        COMA                    ;INH (A) 43    3~
        COMX                    ;INH (X) 53    3~
        COM $12                 ;DIR     33 dd 5~
        COM ,X                  ;IX      73    5~
        COM $12,X               ;IX1     63 ff 6~
        
        ; Compare Index Register with Memory
        CPX #$12                ;IMM A3 ii    2~
        CPX $12                 ;DIR B3 dd    3~
        CPX $1234               ;EXT C3 hh ll 4~
        CPX ,X                  ;IX  F3       3~
        CPX $12,X               ;IX1 E3 ff    4~
        CPX $1234,X             ;IX2 D3 ee ff 5~
        
        ; Decrement
        DECA                    ;INH (A) 4A    3~
        DECX                    ;INH (X) 5A    3~
        DEC $12                 ;DIR     3A dd 5~
        DEC ,X                  ;IX      7A    5~
        DEC $12,X               ;IX1     6A ff 6~
        
        ; Exclusive-OR Memory with Accumulator
        EOR #$12                ;IMM A8 ii    2~
        EOR $12                 ;DIR B8 dd    3~
        EOR $1234               ;EXT C8 hh ll 4~
        EOR ,X                  ;IX  F8       3~
        EOR $12,X               ;IX1 E8 ff    4~
        EOR $1234,X             ;IX2 D8 ee ff 5~
        
        ; Increment
        INCA                    ;INH (A) 4C    3~
        INCX                    ;INH (X) 5C    3~
        INC $12                 ;DIR     3C dd 5~
        INC ,X                  ;IX      7C    5~
        INC $12,X               ;IX1     6C ff 6~
        
.EJECT
        ; Jump
        JMP info                ;DIR BC dd    2~
        JMP procRts             ;EXT CC hh ll 3~
        JMP ,X                  ;IX  FC       2~
        JMP $12,X               ;IX1 EC ff    3~
        JMP $1234,X             ;IX2 DC ee ff 4~
        
        ; Jump to Subroutine
        JSR info                ;DIR BD dd    5~
        JSR procRts             ;EXT CD hh ll 6~
        JSR ,X                  ;IX  FD       5~
        JSR $12,X               ;IX1 ED ff    6~
        JSR $1234,X             ;IX2 DD ee ff 7~
        
        ; Load Accumulator from Memory
        LDA #$12                ;IMM A6 ii    2~
        LDA tmpaux1             ;DIR B6 dd    3~
        LDA $1234               ;EXT C6 hh ll 4~
        LDA ,X                  ;IX  F6       3~
        LDA $12,X               ;IX1 E6 ff    4~
        LDA $1234,X             ;IX2 D6 ee ff 5~
        
        ; Load Index Register from Memory
        LDX #$12                ;IMM AE ii    2~
        LDX tmpaux1             ;DIR BE dd    3~
        LDX $1234               ;EXT CE hh ll 4~
        LDX ,X                  ;IX  FE       3~
        LDX $12,X               ;IX1 EE ff    4~
        LDX $1234,X             ;IX2 DE ee ff 5~
        
        ; Logical Shift Left
        LSLA                    ;INH (A) 48    3~
        LSLX                    ;INH (X) 58    3~
        LSL $12                 ;DIR     38 dd 5~
        LSL ,X                  ;IX      78    5~
        LSL $12,X               ;IX1     68 ff 6~
        
        ; Logical Shift Right
        LSRA                    ;INH (A) 44    3~
        LSRX                    ;INH (X) 54    3~
        LSR $12                 ;DIR     34 dd 5~
        LSR ,X                  ;IX      74    5~
        LSR $12,X               ;IX1     64 ff 6~
        
        ; Multiply Unsigned
        MUL                     ;INH 42 11~
        
        ; Negate
        NEGA                    ;INH (A) 40    3~
        NEGX                    ;INH (X) 50    3~
        NEG $12                 ;DIR     30 dd 5~
        NEG ,X                  ;IX      70    5~
        NEG $12,X               ;IX1     60 ff 6~
        
        ; No Operation
        NOP                     ;INH 9D 2~
        
        ; Inclusive-OR
        ORA #$12                ;IMM AA ii    2~
        ORA $12                 ;DIR BA dd    3~
        ORA $1234               ;EXT CA hh ll 4~
        ORA ,X                  ;IX  FA       3~
        ORA $12,X               ;IX1 EA ff    4~
        ORA $1234,X             ;IX2 DA ee ff 5~
        
        ; Rotate Left thru Carry
        ROLA                    ;INH (A) 49    3~
        ROLX                    ;INH (X) 59    3~
        ROL $12                 ;DIR     39 dd 5~
        ROL ,X                  ;IX      79    5~
        ROL $12,X               ;IX1     69 ff 6~
        
        ; Rotate Right thru Carry
        RORA                    ;INH (A) 46    3~
        RORX                    ;INH (X) 56    3~
        ROR $12                 ;DIR     36 dd 5~
        ROR ,X                  ;IX      76    5~
        ROR $12,X               ;IX1     66 ff 6~
        
.EJECT
        ; Reset Stack Pointer
        RSP                     ;INH 9C 2~
        ; Return from Interrupt
        RTI                     ;INH 80 2~
        ; Return from Subroutine
procRts EQU *
        RTS                     ;INH 81 6~
        
        ; Subtract with Carry
        SBC #$12                ;IMM A2 ii    2~
        SBC $12                 ;DIR B2 dd    3~
        SBC $1234               ;EXT C2 hh ll 4~
        SBC ,X                  ;IX  F2       3~
        SBC $12,X               ;IX1 E2 ff    4~
        SBC $1234,X             ;IX2 D2 ee ff 5~
        
        ; Set Carry Bit
        SEC                     ;INH 99 2~
        ; Set Interrupt Mask Bit
        SEI                     ;INH 9B 2~
        
        ; Store Accumulator in Memory
        STA $12                 ;DIR B7 dd    4~
        STA $1234               ;EXT C7 hh ll 5~
        STA ,X                  ;IX  F7       4~
        STA $12,X               ;IX1 E7 ff    5~
        STA $1234,X             ;IX2 D7 ee ff 6~
        
        ; Enable IRQ, Stop Oscillator
        STOP                    ;INH 8E 2
        
        ; Store Index Register X in Memory
        STX $12                 ;DIR BF ii    4~
        STX $1234               ;EXT CF hh ii 5~
        STX ,X                  ;IX  FF       4~
        STX $12,X               ;IX1 EF ff    5~
        STX $1234,X             ;IX2 DF ee ff 6~
        
        ; Subtract
        SUB #$12                ;IMM A0 ii    2~
        SUB $12                 ;DIR B0 dd    3~
        SUB $1234               ;EXT C0 hh ll 4~
        SUB ,X                  ;IX  F0       3~
        SUB $12,X               ;IX1 E0 ff    4~
        SUB $1234,X             ;IX2 D0 ee ff 5~
        
        ; Software Interrupt
        SWI                     ;INH 83 10~
        ; Transfer Accumulator to Index Register
        TAX                     ;INH 97 2~
        
        ; Test for Negative or Zero
        TSTA                    ;INH (A) 4D    3~
        TSTX                    ;INH (X) 5D    3~
        TST $12                 ;DIR     3D dd 4~
        TST ,X                  ;IX      7D    4~
        TST $12,X               ;IX1     6D ff 5~
        
        ; Transfer Index Register to Accumulator
        TXA                     ;INH 9F 2~
        ; Enable Interrupt, Stop Processor
        WAIT                    ;INH 8F 2~

INSTRUCTIONS_END EQU *  ; last location of used ROM

.SUBTTL Code Examples
.EJECT
;----------------------------------------------------------------------------
;
        FCB LOW  ($89ABCDEF), $89ABCDEF & $EF                   ; Bits [ 7: 0]
        FCB HIGH ($89ABCDEF), ($89ABCDEF >>  8) & $FF           ; Bits [15: 8]
        FCB BYTE2($89ABCDEF), ($89ABCDEF >>  8) & $FF           ; Bits [15: 8]
        FCB BYTE3($89ABCDEF), ($89ABCDEF >> 16) & $FF           ; Bits [23:16]
        FCB BYTE4($89ABCDEF), ($89ABCDEF >> 24) & $ff           ; Bits [31:24]
        FDB LWRD ($89ABCDEF), $89ABCDEF & $FFFF                 ; Bits [15: 0]      
        FDB HWRD ($89ABCDEF), $89ABCDEF >> 16                   ; Bits [31:16]           
        FDB PAGE ($12345678), ($12345678 & $003F0000) >> 16     ; Bits [21:16]             

_db01c  FCB     _DB01C_LENGTH, 1,13,'012"3":; ,"', '", "', 'A', '"', ''', '', "";, 22, 25, 4 \
_DB01C_LENGTH EQU * - _db01c

_11 SET $11
_12 SET $12
_13 SET $13
_db3:   FCB     _11,_12, _13, 'Z'+1, \
                 %10101, %1, %101,   \
                 "1234567890abcd ",  \
                 'A',_db3 AND $FF,  \
                 "1234567890abcde",  \
                 $FF,$FF

_db09   FCB     __DATE__, __TIME__, __CENTURY__

_db10   FCB     __DATE__, __TIME__, '__CENTURY__', \
                $00
_FDB1   FDB 1
.EJECT
;----------------------------------------------------------------------------
;
        FDB     STRLEN("XASMAVR Macro Assembler V2.1") 

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
        FCB LOG2(32768),                15
        FCB LOG2(65536),                16
        FCB LOG2(8388608),              23
        FCB LOG2(16777216),             24
        FCB LOG2(2147483648),           31
        FCB LOG2(4294967296),           32
        FCB LOG2(549755813888),         39
        FCB LOG2(140737488355328),      47
        FCB LOG2(36028797018963968),    55
        FCB LOG2(9223372036854775808),  63
        FCB LOG2(18446744073709551616), 64
        
#define _flag1
.DEFINE _flag2
        FCB DEFINED(_flag1), !DEFINED(_flag1)
        FCB DEFINED(_flag2), !DEFINED(_flag2)

#if DEFINED(_flag1)
        nop             ; flag1
#elif DEFINED(_flag2)
#endif

#if !DEFINED(_flag1)    
#elif DEFINED(_flag2)
        nop             ; flag2 
#endif
        
;-----------------------------------------------------------------,
; Warning: Use parenthesis in complex expressions!                |
_VAR SET $a600*256+$75a2>>8          ;; =00A60075 NOT EXPECTED ?! |
        FDB _VAR                     ;                            |
_VAR SET ($a600*256)+($75a2>>8)      ;; =00a60075 expected        |
        FDB LWRD(_VAR)               ;                            |
        FDB HWRD(_VAR)               ;                            |
_VAR SET ($a600*256+$75a2)>>8        ;; =0000a675 expected        |
        FDB _VAR                     ;                            |
;-----------------------------------------------------------------'
.EJECT
;----------------------------------------------------------------------------
;
;                       testROM
; ROM Test
;
testROM EQU *
        ldx     #INSTRUC_L-1    ;moved from end to begin        
tstrom1:
        lda     tstrom4, x      ;move into RAM for pointer addition
        sta     $22, x          ;= "_0022 FCB $DB,01,00, $CC,04,$F7"
        decx                    
        bpl     tstrom1         ;"branch if >= 0": 6 bytes

        clra                    ;Init checksum accu
tstrom2:
        clrx                    ;Init Pointer in RAM = 0

; Test the ROM using pointer in RAM
; by processing the two instructions moved from 'tstrom4'
tstrom3:
        JMP     $0022           ;direct addressing
;       ...............
;;ha;;  FCB $CC,$00,$22         ;;ha;; "JMP $0022" as 16bit address
;       ...............

;------------------------------
; The two instructions below          $22 $23 $24  $25 $26 $27 = RAM locations
; reside in RAM $22..$27 = "_0022 FCB $DB, 01, 00, $CC, 04,$F7"
;           RAM $23..$24 = $0100 (l6bit offset into ROM, allows easy increments)
tstrom4:
        add     ROMSTART, x     ;see Listing -> 04F1  DB 01 00 
        jmp     tstrom5         ;see Listing -> 04F4  CC 04 F7
INSTRUC_L EQU * - tstrom4       ;number of bytes these two instructions occupy  
;------------------------------

; Add up ROM bytes $0100..$0EFF
tstrom5:
        incx                    ;advance pointer residing in RAM
        bne     tstrom3         ;$0000..$00FF done?
                                ;count 256 chunks via ROMSTART ptr in RAM
        inc     $23             ;$23=HIGH(ROMSTART)-Operand of "add ROMSTART, x"
        LDX     $0023           ;direct addressing
;       ...............
;;ha;;  FCB $CE,$00,$23         ;;ha;; "LDX $0023" as 16bit address
;       ...............
        cpx     #$0F            ;X=$23=HIGH(ROMSTART)-Operand ROM $0100..$0EFF done?
        bne     tstrom2         ;not ready yet, loop for next
                                ;$0100..$0EFF done.

; Add up the remaining $0F00..$0F38
        ldx     #MANUF_MODE_L-1 ;add up the remaining $0F00..$0F38
tstrom6:
        add     MANUF_MODE, x
        decx 
        bpl     tstrom6         ;"branch if >= 0": $38 bytes

;add up the remaining $0FF8..$0FFF
        ldx     #VECTORS_L-1            
tstrom7:
        add     VECTORS, x
        decx 
        bpl     tstrom7         ;"branch if >= 0": 8 bytes
         
; The chunk $80..$FF still has to be done
        ldx     #ROMPAGE0_SIZE-1                
tstrom8:
        add     ROMPAGE0, x             
        decx                    
        bpl     tstrom8         ;"branch if >= 0": $80 bytes
        rts                     ;"#\%-SSSEUFZ!!!" Now all ROM is added up!

.EJECT
;----------------------------------------------------------------------------
;
;                       docmd
;
docmd EQU *
        sta     tmp_65          ;Save command $ED..$FF
        sub     #-(CMDTAB_L/3)  ;$ED...$FF? (SIZEOF(jmp) = 3bytes) 
        sta     tmpaux3         ;$ED..$FF --> 0..18 
        lsla                    ;*2
        add     tmpaux3         ;+1 (3 bytes per 'jmp' in 'docmd1' table)
        tax                     ;load index with table offset
        jmp     cmdtab, x       ;jump to routine
        
cmdtab: jmp     EDcmd           
        jmp     EEcmd           
        jmp     EFcmd           
        jmp     F0cmd           
        jmp     F1cmd           
        jmp     F2cmd           
        jmp     F3cmd           
        jmp     F4cmd           
        jmp     F5cmd           
        jmp     F6cmd           
        jmp     F7cmd           
        jmp     F8cmd           
        jmp     F9cmd           
        jmp     FAcmd           
        jmp     FBcmd           
        jmp     FCcmd           
        jmp     FDcmd           
        jmp     FEcmd           
        jmp     FFcmd           
CMDTAB_L EQU * - cmdtab
;       ---
EDcmd:
EEcmd:
EFcmd:
F0cmd:
F1cmd:
F2cmd:
F3cmd:
F4cmd:
F5cmd:
F6cmd:
F7cmd:
F8cmd:
F9cmd:
FAcmd:
FBcmd:
FCcmd:
FDcmd:
FEcmd:
FFcmd:  rts

;------------------------------------------------------------------------------
;
;       Macros
;
.MACRO Addition 
        lda     #@0     ; param0                
        add     #@1     ; param1
.ENDM

        ADDITION $10, 20
        STA     $100 
.EVEN
        ADDITION $12, 15
        STA     $101
        SWI

;----------------------------------------------------------------------------
;
;                       MNUFACTURING MODE2
;
; Reserved area (selfcheck ROM, see MC6805R_U2 (Jan1984) data sheet)
; Special burn-in mode2
;
        ORG MANUF_MODE

manufMod2 EQU *
        bset    _KYBCLK_, DDRA  ;PORTA bit0 = output

manufMod2_1:
        bset    _KYBCLK_, PORTA ;KYBCLK="L"
        clra                    ;2048us delay (=2048/MCYCLE)
        deca                    
        bne     *-1
        bclr    _KYBCLK_, PORTA ;KYBCLK="H"
        deca
        bne     *-1             ;2048us delay
        bra     manufMod2_1     ;Create a KYBCLK wave 
MANUF_MODE_L EQU * - MANUF_MODE ;Length of manufacturing mode procs

ROM_AVAIL EQU   ROMCHECKSTART-* ;Free ROM space

.EJECT
;----------------------------------------------------------------------------
;
; Checksum byte (to be inserted before PROM-programming)
;
;         ....##.. .#####..
;         ...##... ##...##.
;         ..##.... .....##.
;         .##..... ....##..
;         ##..##.. ...##...
;         ##..##.. ..##....
;         #######. .##.....
;         ....##.. ##......
;         ....##.. #######.
;
        ORG     ROMCHECKSTART
chksum  FCB     $42               ; ..why not use "$42" here ;-)

;----------------------------------------------------------------------------
;
;                       Vector Table (dummy)
;
        ORG     VECTORS

        FDB     info                    ; Timer INT
        FDB     info                    ; externat INT
        FDB     info                    ; SWI
        FDB     MC68HC05_instructions   ; Hardware Reset Vector

VECTORS_L EQU * - VECTORS               ; Size of vector area

;---------------------------------------------------------------------------
.IF INSTRUCTIONS_END >= ROM_SIZE
  .WARNING "Rom size overflow"
.ENDIF
        END




