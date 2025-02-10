;---------------------------------------,
.TITLE Test: UPI-C42 Instruction set
.PAGELENGTH(84)                         ;
.PAGEWIDTH(130)                         ;
.SYMBOLS                                ;
;---------------------------------------'

;------------------------------------------------------------------------------
;
;                UPI-C42 2K User ROM/OTP  (000h .. 7FFh)
;

; DATA MEMORY
;
ROMSIZE         EQU     4096    ; Size of on-chip ROM (bytes)
PAGESIZE        EQU     100h    ; Size of data page in ROM (bytes)

RAMSIZE         EQU     128     ; Size of on-chip RAM (bytes)
STACK           EQU     08h     ; System stack starts at RAM address 08h
STACKSIZE       EQU     8*2     ; Stack size = 8 words (16 bit) entries

MCYCLE          EQU     1250    ;((1/12MHz)/3)*5 = 1.250us per machine cycle 
TCTIME          EQU     40      ;(T/C increments every 32 machine cycles).
                                ;;  e.g.   32 * 1.875us = 60us (8MHz)

                ORG     0       ; RAM Segment starts at address 0
RBANK0:         DS      8       ; * Register Bank 0 *
;               - - - - - - - - - - - - - - - - - - - - - - - -
                ORG     STACK           ; STACK starts at 08h
                DS      STACKSIZE       ; * Stack (STACKSIZE) *
;               - - - - - - - - - - - - - - - - - - - - - - - -
RBANK1:         DS      8       ; * Register Bank 1 *
;
; Start of Working Storage
;
                ORG     20h             ;IBM COMPATIBLE LOCATION
                DS      1               ;we use a register as cmd byte instead
                                        ;-----------------------
                ORG     2Bh             ;IBM COMPATIBLE LOCATION
rwoffs:         DS      1               ;offset for commands 00..1F, 40..5F

                ORG     33h             ;IBM COMPATIBLE LOCATION
cmdA6_ack:      DS      1               ;non-zero values are sent to system
                                        ; to acknowledge the "0A6h" cmd

                ORG     34h             ;IBM COMPATIBLE LOCATION
pwmatch_ack:    DS      1               ;non-zero values are sent to system
                                        ; to acknowledge a password match

                ORG     35h             ;reserved
;                                       ;-----------------------
                ORG     36h             ;IBM COMPATIBLE LOCATION
pwskip:         DS      2               

                ORG     38h             ;reserved

                ORG     50h             ;UPI PS/2 (EISA) PASSWORD MODE AREA
                DS      5               ;51h..54h  unused
                
                                        ;PASSWORD SECURITY FEATURE AREA
sysPWinst:      DS      1               ;55h: 
sysPW:          DS      2               ;56/57h: 
                DS      2               ;58/59h: 
verPW:          DS      2               ;5A/5Bh: 

actPWfld:                               ;----------------------------
PWactive:       DS      1               ;5Ch:
PWinst:         DS      1               ;5Dh:
actPW:          DS      2               ;5E/5Fh:
L_ACTPWFLD      EQU     $-actPWfld      ;length of field

RAMAVAIL        EQU     RAMSIZE-$       ;unused RAM available   

;------------------------------------------------------------------------------
; ROMPAGE 0
;       
        ORG     0               ; RomPage 0

HWreset:
        JMP     UPI_C42_instructions

; Info String
info    DB      "Intel UPI-C42 Instruction Set"
INFO_LEN EQU $ - info
                 
UPI_C42_instructions:
; ACCUMULATOR
        ADD     A, R0           ; Add register0 to A 
        ADD     A, R1           ; Add register1 to A 
        ADD     A, R2           ; Add register2 to A 
        ADD     A, R3           ; Add register3 to A 
        ADD     A, R4           ; Add register4 to A 
        ADD     A, R5           ; Add register5 to A 
        ADD     A, R6           ; Add register6 to A 
        ADD     A, R7           ; Add register7 to A 
        ADD     A, @R0          ; Add data memory to A 
        ADD     A, @R1          ; Add data memory to A 
        ADD     A, #INFO_LEN    ; data Add immediate to A 
        ADDC    A, R0           ; Add register0 to A with carry 
        ADDC    A, R1           ; Add register1 to A with carry 
        ADDC    A, R2           ; Add register2 to A with carry 
        ADDC    A, R3           ; Add register3 to A with carry 
        ADDC    A, R4           ; Add register4 to A with carry 
        ADDC    A, R5           ; Add register5 to A with carry 
        ADDC    A, R6           ; Add register6 to A with carry 
        ADDC    A, R7           ; Add register7 to A with carry 
        ADDC    A, @R0          ; Add data memory to A with carry 
        ADDC    A, @R1          ; Add data memory to A with carry 
        ADDC    A, #12h         ; Add immediate to A data with carry 
        ANL     A, R0           ; And register0 to A 
        ANL     A, R1           ; And register1 to A 
        ANL     A, R2           ; And register2 to A 
        ANL     A, R3           ; And register3 to A 
        ANL     A, R4           ; And register4 to A 
        ANL     A, R5           ; And register5 to A 
        ANL     A, R6           ; And register6 to A 
        ANL     A, R7           ; And register7 to A 
        ANL     A, @R0          ; And data memory to A 
        ANL     A, @R1          ; And data memory to A 
        ANL     A, #12h         ; data And immediate to A 
        ORL     A, R0           ; Or register0 to A 
        ORL     A, R1           ; Or register1 to A 
        ORL     A, R2           ; Or register2 to A 
        ORL     A, R3           ; Or register3 to A 
        ORL     A, R4           ; Or register4 to A 
        ORL     A, R5           ; Or register5 to A 
        ORL     A, R6           ; Or register6 to A 
        ORL     A, R7           ; Or register7 to A 
        ORL     A, @R0          ; Or data memory to A 
        ORL     A, @R1          ; Or data memory to A 
        ORL     A, #12h         ; data Or immediate to A 
        XRL     A, R0           ; Exclusive Or register0 to A 
        XRL     A, R1           ; Exclusive Or register1 to A 
        XRL     A, R2           ; Exclusive Or register2 to A 
        XRL     A, R3           ; Exclusive Or register3 to A 
        XRL     A, R4           ; Exclusive Or register4 to A 
        XRL     A, R5           ; Exclusive Or register5 to A 
        XRL     A, R6           ; Exclusive Or register6 to A 
        XRL     A, R7           ; Exclusive Or register7 to A 
        XRL     A, @R0          ; Exclusive Or data memory to A 
        XRL     A, @R1          ; Exclusive Or data memory to A 
        XRL     A, #12h         ; data Exclusive Or immediate to A 
        DEC     A               ; Decrement A 
        INC     A               ; Increment A 
        CLR     A               ; Clear A 
        CPL     A               ; Complement A 
        SWAP    A               ; Swap nibbles of A 
        DA      A               ; Decimal Adjust A 
        RRC     A               ; Rotate A right through carry 
        RR      A               ; Rotate A right 
        RL      A               ; Rotate A left 
        RLC     A               ; Rotate A left through carry 

.EJECT
; INPUT/OUTPUT
        IN      A, P1           ; Input port1, to A 
        IN      A, P2           ; Input port2, to A 
        OUTL    P1, A           ; Output A to port1 
        OUTL    P2, A           ; Output A to port2 
        ANL     P1, #12h        ;data And immediate to port1 
        ANL     P2, #12h        ;data And immediate to port2 
        ORL     P1, #12h        ;data Or immediate to port1 
        ORL     P2, #12h        ;data Or immediate to port2 
        IN      A, DBB          ; Input DDB to A, clear IBF 
        OUT     DBB, A          ; Output A to DBB, Set OBF 
        MOV     STS,A           ; A4-A7 to bits 4-7 of status 

        MOVD    A, P4           ;= 0Ch Input Expander port4, to A
        MOVD    A, P5           ;= 0Dh Input Expander port5, to A
        MOVD    A, P6           ;= 0Eh Input Expander port6, to A
        MOVD    A, P7           ;= 0Fh Input Expander port7, to A
        MOVD    P4, A           ;= 3Ch Output A to Expander port4
        MOVD    P5, A           ;= 3Dh Output A to Expander port5
        MOVD    P6, A           ;= 3Eh Output A to Expander port6
        MOVD    P7, A           ;= 3Fh Output A to Expander port7
        ANLD    P4, A           ;= 9Ch And A to Expander port4 
        ANLD    P5, A           ;= 9Dh And A to Expander port5 
        ANLD    P6, A           ;= 9Eh And A to Expander port6 
        ANLD    P7, A           ;= 9Fh And A to Expander port7 
        ORLD    P4, A           ;= 8Ch Or A to Expander port4 
        ORLD    P5, A           ;= 8Dh Or A to Expander port5 
        ORLD    P6, A           ;= 8Eh Or A to Expander port6 
        ORLD    P7, A           ;= 8Fh Or A to Expander port7 

.EJECT
; DATA MOVES
        MOV     A, R0           ; Move register0 to A 
        MOV     A, R1           ; Move register1 to A 
        MOV     A, R2           ; Move register2 to A 
        MOV     A, R3           ; Move register3 to A 
        MOV     A, R4           ; Move register4 to A 
        MOV     A, R5           ; Move register5 to A 
        MOV     A, R6           ; Move register6 to A 
        MOV     A, R7           ; Move register7 to A 
        MOV     A, @R0          ; Move data memory to A 
        MOV     A, @R1          ; Move data memory to A 
        MOV     A, #12h         ;data Move immediate to A 
        MOV     R0, A           ; Move A to register0 
        MOV     R1, A           ; Move A to register1 
        MOV     R2, A           ; Move A to register2 
        MOV     R3, A           ; Move A to register3 
        MOV     R4, A           ; Move A to register4 
        MOV     R5, A           ; Move A to register5 
        MOV     R6, A           ; Move A to register6 
        MOV     R7, A           ; Move A to register7 
        MOV     @R0, A          ; Move A to data memory 
        MOV     @R1, A          ; Move A to data memory 
        MOV     R0, #5+30*2     ;data Move immediate to register0 
        MOV     R1, #(25/5)+30*2;data Move immediate to register1 
        MOV     R2, #5+(-30*-2) ;data Move immediate to register2 
        MOV     R3, #12h        ;data Move immediate to register3 
        MOV     R4, #12h        ;data Move immediate to register4 
        MOV     R5, #12h        ;data Move immediate to register5 
        MOV     R6, #12h        ;data Move immediate to register6 
        MOV     R7, #15/(3+18/9);data Move immediate to register7 
        MOV     @R0, #15/3+18/9 ; Move immediate to Ãdata data memory 
        MOV     @R1, #12        ; Move immediate to Ãdata data memory 
        MOV     A, PSW          ; Move PSW to A 
        MOV     PSW, A          ; Move A to PSW 
        XCH     A, R0           ; Exchange A and registers0 
        XCH     A, R1           ; Exchange A and registers1 
        XCH     A, R2           ; Exchange A and registers2 
        XCH     A, R3           ; Exchange A and registers3 
        XCH     A, R4           ; Exchange A and registers4 
        XCH     A, R5           ; Exchange A and registers5 
        XCH     A, R6           ; Exchange A and registers6 
        XCH     A, R7           ; Exchange A and registers7 
        XCH     A, @R0          ; Exchange A and data memory 
        XCH     A, @R1          ; Exchange A and data memory 
        XCHD    A, @R0          ; Exchange digit (bit[3:0]) of A and register0 
        XCHD    A, @R1          ; Exchange digit (bit[3:0]) of A and register1 
        
        MOVP    A, @A           ; Move to A from current page 
        MOVP3   A, @A           ; Move to A from page 3 
        
.EJECT
; FLAGS
        CLR     C               ; Clear Carry 
        CPL     C               ; Complement Carry 
        CLR     F0              ; Clear Flag 0 
        CPL     F0              ; Complement Flag 0 
        CLR     F1              ; Clear F1 Flag 
        CPL     F1              ; Complement F1 Flag 

; TIMER/COUNTER
        MOV     A, T            ; Read Timer/Counter 
        MOV     T, A            ; Load Timer/Counter 
        STRT    T               ; Start Timer 
        STRT    CNT             ; Start Counter 
        STOP    TCNT            ; Stop Timer/Counter 
        EN      TCNTI           ; Enable Timer/Counter 
        DIS     TCNTI           ; Disable Timer/Counter 

; Interrupt CONTROL
;       -----------             ;UPI-C42 (80C42) only, enable A20 mechanism 
        EN      A20             ;NOTE: if the new Intel UPI-C42 is used and
;       -----------             ; the instruction "EN A20" has been executed
                                ; then all "A20-gate" performance problems
                                ; are resolved for the time being
                                ; (see Intel UPI-C42 O-Nr: 290414-003 Dec1995 page 6,7)
                                
        EN      FLAGS           ; Enable Master Interrupts 
        EN      DMA             ; Enable DMA Handshake Lines 
        EN      I               ; Enable IBF interrupt 
        DIS     I               ; Disable IBF interrupt 
        SEL     RB0             ; Select register bank 0 
        SEL     RB1             ; Select register bank 1
        SEL     PMB0            ;= 63h Select program memory bank 0 (addr > 2K)
        SEL     PMB1            ;= 73h Select program memory bank 1 (addr > 2K)
        NOP                     ; No Operation 

; REGISTERS
        INC     R0              ; Increment register0 
        INC     R1              ; Increment register1 
        INC     R2              ; Increment register2 
        INC     R3              ; Increment register3 
        INC     R4              ; Increment register4 
        INC     R5              ; Increment register5 
        INC     R6              ; Increment register6 
        INC     R7              ; Increment register7 
        INC     @R0             ; Increment data memory 
        INC     @R1             ; Increment data memory 
        DEC     R0              ; Decrement register0 
        DEC     R1              ; Decrement register1 
        DEC     R2              ; Decrement register2 
        DEC     R3              ; Decrement register3 
        DEC     R4              ; Decrement register4 
        DEC     R5              ; Decrement register5 
        DEC     R6              ; Decrement register6 
        DEC     R7              ; Decrement register7 
;
amovp0:         MOV     A,R0            
                MOVP    A,@A    ;routine is called during SELFTEST
                RET

.EJECT
; Example - ROMPAGE 1   
        ORG     100h            ;RomPage 1

;------------------------------------------------------------------------------
;
;                       KYBIN: Input from keyboard.
;
; "KYBIN" - entered via call from 'SNDKYB' and ISA/EISA main polling loop
;
; PROCESS: the keyboard data is read either in PS/2 or AT mode
;
; INPUT: PS/2 (EISA) KybDat P1.0 = low (start bit) 
;        ISA (AT) KybDat T1 = low (start bit)
;        EISA/ISA KybClk T0 = low (start bit clk)
; OUTPUT: (A) = key scancode, STATUS
;         CY=0 if no errors, CY=1 if receive errors were detected 
; MODIFICATIONS: A,R1,ONECNT,BITCNT,STATUS
; STACK USAGE: 1 byte 
;
; DESCRIPTION:  This routine needs 17~ MCYCLEs (8MHz=31.8us, 12MHz=21.2us) to
;               verify that the start bit clk (T0) is low.
;               Refer to IBM keyboard and auxiliary timing specification:
;                Time from DATA transition to falling edge of CLK := 5..25us
;                Duration of CLK low or CLK high := 30..50us            
;                Time to inhibit keyboard/mouse after clock 11 := <50us 
;
;                MAIN POLLING LOOP & SNDMOU, trigger T0&P1.0 = low (start bit):
;                 When called the minimum total MCYCLEs needed to get to the
;                 KYBCLK routine are: 19~+9~=28~ (8MHz=52.5us, 12MHz=35.0us).
;      
;                 1) Keyboards may pull start-bit-clock low after > 25us!
;                    (This allows keyboards to be out of spec. at the
;                     start-bit phase).
;
;                      ___________     ___      
;               KYBCLK            \___/   \___/  ... 
;                                 |       |      
;                      ____   start bit  _|_ 1st data bit ...
;               KYBDAT     \______|_____/ |      ...
;                          |      | min.  |
;        [us(12MHz)] ->    | un-  | 35.0  |     
;                          | cri- | 43.7  |     (1 extra NOVELL polling (7~))
;        [us(8MHz)] ->     | ti-  | 52.5  |
;                          | cal  | 65.6  |     (1 extra NOVELL polling (7~))
;      
;                 2) The total period of the start-bit-clock (low+high) must be
;                    at least 65.6us. Typically keyboards have clk periods
;                    greater than (35+35 = 70)us.
;
                                ;BANK 0
CONVERT         EQU     R2      ; UPI AT-mode flag
BITCNT          EQU     R3      ; bit counter for transmit/receive
ONECNT          EQU     R4      ; "1" counter for parity checking

kybinjmp:       DB      PS2kbin AND 0FFh ;jump table
                DB      ATkbin  AND 0FFh  

KybIn:                                  ;GENERAL INITIALIZATION
        MOV     A,#(-(2000/TCTIME))     ;2ms
        MOV     T,A                     ;set timer
        JTF     $+2                     ;reset timer overflow flag
        MOV     ONECNT,#0               ;reset "1" counter
        MOV     BITCNT,#8               ;receive 8 bits

        MOV     A,CONVERT       ;jump to either PS/2 or AT design
        JMPP    @A              ;go to routine via table at start of page seg

PS2kbin: ; PS/2 (EISA): Input from keyboard. RECEIVE 8 BITS: PS/2 interface

ATkbin:  ; AT (ISA): Input from keyboard. RECEIVE 8 BITS: AT (ISA) interface

        RET                     ; ... Code not shown


.EJECT
;------------------------------------------------------------------------------
;
        CALL    addr            ; Jump to checksum subroutine (..code not shown)
addr:   RET                     ; Return 
        RETR                    ; Return and restore status 

; BRANCH
        JMP     addr            ; Jump unconditional 
        JMPP    @A              ; Jump indirect 
        DJNZ    R0, addr        ; Decrement register0 addr and jump on non-zero 
        DJNZ    R1, addr        ; Decrement register1 addr and jump on non-zero 
        DJNZ    R2, addr        ; Decrement register2 addr and jump on non-zero 
        DJNZ    R3, addr        ; Decrement register3 addr and jump on non-zero 
        DJNZ    R4, addr        ; Decrement register4 addr and jump on non-zero 
        DJNZ    R5, addr        ; Decrement register5 addr and jump on non-zero 
        DJNZ    R6, addr        ; Decrement register6 addr and jump on non-zero 
        DJNZ    R7, addr        ; Decrement register7 addr and jump on non-zero 
        JC      addr            ; Jump on Carry e1 
        JNC     addr            ; Jump on Carry e0 
        JZ      addr            ; Jump on A zero 
        JNZ     addr            ; Jump on A not zero 
        JT0     addr            ; Jump on T0 e1 
        JNT0    addr            ; Jump on T0 e0 
        JT1     addr            ; Jump on T1 e1 
        JNT1    addr            ; Jump on T1 e0 
        JF0     addr            ; Jump on F0 Flag e1 
        JF1     addr            ; Jump on F1 Flag e1 
        JTF     addr            ; Jump on Timer Flag e1 
        JNIBF   addr            ; Jump on IBF Flag e0 
        JOBF    addr            ; Jump on OBF Flag e1 
        JB0     addr            ; Jump on Accumulator Bit0 
        JB1     addr            ; Jump on Accumulator Bit1 
        JB2     addr            ; Jump on Accumulator Bit2 
        JB3     addr            ; Jump on Accumulator Bit3 
        JB4     addr            ; Jump on Accumulator Bit4 
        JB5     addr            ; Jump on Accumulator Bit5 
        JB6     addr            ; Jump on Accumulator Bit6 
        JB7     addr            ; Jump on Accumulator Bit7 

amovp1:         MOV     A,R0            
                MOVP    A,@A    ;routine is called during SELFTEST
                RET

;  Example - ROMPAGE 2..7       
        ORG     200h            ;RomPage 2
amovp2:         MOV     A,R0            
                MOVP    A,@A    ;routine is called during SELFTEST
                RET
        ORG     300h            ;RomPage 3
        ORG     400h            ;RomPage 4
amovp4:         MOV     A,R0            
                MOVP    A,@A    ;routine is called during SELFTEST
                RET
        ORG     500h            ;RomPage 5
amovp5:         MOV     A,R0            
                MOVP    A,@A    ;routine is called during SELFTEST
                RET
        ORG     600h            ;RomPage 6
amovp6:         MOV     A,R0            
                MOVP    A,@A    ;routine is called during SELFTEST
                RET
        ORG     700h            ;RomPage 7
amovp7:         MOV     A,R0            
                MOVP    A,@A    ;routine is called during SELFTEST
                RET

.SUBTTL Code Examples continued ...
.EJECT
;----------------------------------------------------------------------------
;
        DB LOW  (89ABCDEFh),  89ABCDEFh AND 0EFh              ; Bits [ 7: 0]
        DB HIGH (89ABCDEFh), (89ABCDEFh SHR  8) AND 0FFh      ; Bits [15: 8]
        DB BYTE2(89ABCDEFh), (89ABCDEFh SHR  8) AND 0FFh      ; Bits [15: 8]
        DB BYTE3(89ABCDEFh), (89ABCDEFh SHR 16) AND 0FFh      ; Bits [23:16]
        DB BYTE4(89ABCDEFh), (89ABCDEFh SHR 24) AND 0ffh      ; Bits [31:24]
        DW LWRD(89ABCDEFh),   89ABCDEFh AND 0FFFFh            ; Bits [15: 0]        
        DW HWRD(89ABCDEFh),   89ABCDEFh SHR 16                ; Bits [31:16]             
        DB PAGE(12345678h),  (12345678h AND 003F0000h) SHR 16 ; Bits [21:16]             

_db01c  DB      _DB01C_LENGTH, 1,13,'12"3":; ,"', '", "', 'A', '"', ''', '', "";, 25, 4 \
_DB01C_LENGTH EQU $ - _db01c

_11 SET 11h
_12 SET 12h
_13 SET 13h
_db3:   DB      _11,_12, _13, 'Z'+1, \
                 10101b, 1b, 101b,   \
                 "1234567890abcd ",  \
                 'A',_db3 AND 0FFh,  \
                 "1234567890abcde",  \
                 0FFh,0FFh

_db09   DB      __DATE__, __TIME__, __CENTURY__

db10    DB      __DATE__, __TIME__, '__CENTURY__', \
        0
_DW1:   DW 1
.EJECT
;----------------------------------------------------------------------------
;
        DB STRLEN("XASMAVR Macro Assembler V2.1") 

        DB EXP2(0),  1            
        DB EXP2(7),  128          
        DW EXP2(8),  256          
        DW EXP2(15), 32768                                                             
        
        DB LOG2(0),                     64 ; Illegal, out of range
        DB LOG2(1),                      0
        DB LOG2(128),                    7
        DB LOG2(32768),                 15
        DB LOG2(65536),                 16
        DB LOG2(8388608),               23
        DB LOG2(16777216),              24
        DB LOG2(2147483648),            31
        DB LOG2(4294967296),            32
        DB LOG2(549755813888),          39
        DB LOG2(140737488355328),       47
        DB LOG2(36028797018963968),     55
        DB LOG2(9223372036854775808),   63
        DB LOG2(18446744073709551616),  64

;-----------------------------------------------------------------,
; Warning: Use parenthesis in complex expressions!                |
_VAR SET 0a600h*256+75a2h SHR 8     ;; =00A60075 NOT EXPECTED ?!  |
        DW _VAR                     ;                             |
_VAR SET (0a600h*256)+(75a2h SHR 8) ;; =00a60075 expected         |
        DW LOW(_VAR)                ;                             |
        DW _VAR SHR 8               ;                             |
_VAR SET (0a600h*256+75a2h) SHR 8   ;; =0000a675 expected         |
        DW _VAR                     ;                             |
;-----------------------------------------------------------------'

.EJECT
;------------------------------------------------------------------------------
;
;                       cmdAX
;
cmdAX:                          ;A0..AFh: UPI CONTROLLER COMMANDS.
        MOV     A,R1            ;get subfunction
        ADD     A,#cmdjmp_AX AND 0FFh
        JMPP    @A

cmdjmp_AX:
        DB      cmdA2 AND 0FFh  ;backup/restore sys password
        DB      cmdA3 AND 0FFh  ;load hashed password
        DB      cmdA5 AND 0FFh  ;load password from system
        DB      cmdA6 AND 0FFh  ;enable password
        DB      cmdA9 AND 0FFh  ;test auxiliary device interface
        DB      cmdAA AND 0FFh  ;UPI seftest
        DB      cmdAB AND 0FFh  ;keyboard interface test

cmdA2:  JMP     cmdA2           ;backup/restore sys password
cmdA3:  JMP     cmdA3           ;load hashed password
cmdA5:  JMP     cmdA5           ;load password
cmdA6:  JMP     cmdA6           ;enable password
cmdA9:  JMP     cmdA9           ;auxiliary interface test
cmdAA:  JMP     cmdAA           ;jmp to self test & init
cmdAB:  JMP     cmdAB           ;jmp to keyboard interface test

;------------------------------------------------------------------------------
;
;                       TIMER TEST
;
        MOV     A,#(-(240/TCTIME))      ;240us time out
        MOV     T,A                     ;set timer for a quick timeout
        JTF     $+2                     ;reset timer overflow flag
stdly:  DJNZ    R0,stdly        ;loop 256 times: 8MHz=959us, 12MHz=640us
        JTF     chkrom          ;timer overflow occurred (240us), go ahead
sterr:  JMP     $               ;failed self test, hang forever

;------------------------------------------------------------------------------
;
;                       ROM TEST
;
        MOV     R0,#0           ;Clear R0
chkrom:                         ;ROM CHECKSUM TEST (ptr R0 zero'ed previously)
        CALL    amovp0          ;ROM Page #0 checksum
        ADD     A,R1            ;cumulative sum R1 cleared previously
        MOV     R1,A            ;save cumulative checksum
        CALL    amovp1          ;ROM page #1
        ADD     A,R1
        MOV     R1,A
        CALL    amovp2          ;ROM page #2
        ADD     A,R1
        MOV     R1,A
        MOV     A,R0

        MOVP3   A,@A            ;ROM page #3
        ADD     A,R1
        MOV     R1,A

        CALL    amovp4          ;ROM page #4
        ADD     A,R1
        MOV     R1,A
        CALL    amovp5          ;ROM page #5
        ADD     A,R1
        MOV     R1,A
        CALL    amovp6          ;ROM page #6
        ADD     A,R1
        MOV     R1,A
        CALL    amovp7          ;ROM page #7
        ADD     A,R1
        MOV     R1,A

        DJNZ    R0,chkrom       ;each ROM page has 256 bytes
        JNZ     sterr           ;error if checksum isn't 0
        JMP     HWreset         ;JMP to mainlp (stack is all tested & cleared!)

.EJECT
;------------------------------------------------------------------------------
;
;                       LSHIFT
;
lshift:                         ;rol a word value in @R1(lsb) / @R1 (msb)
        MOV     A,@R1           ;get low byte
        CLR     C               ;at first shift the stuff logically left
        RLC     A               ;[C] << [msb] << [C] << [lsb] << [0]
        MOV     @R1,A           ;store low byte

        INC     R1              ;get hi byte address
        MOV     A,@R1           ;get hi byte
        RLC     A               ;shift carry from low byte into hi byte
        MOV     @R1,A           ;store hi byte
        DEC     R1              ;re-adjust pointer to low byte address
        JNC     lshret          ;done if no carry from hi byte
                                ;[lsb] << [C]
        MOV     A,@R1           ;get low byte
        INC     @R1             ;put carry from hi byte into lsb   
lshret:
        RET                     ;now a word has been ROL'ed

;------------------------------------------------------------------------------
        END     
        
        