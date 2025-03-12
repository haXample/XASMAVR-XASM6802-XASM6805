;#define XASMAVR via commandline option '/DXASMAVR'
;#define XASM ;TEST
;----------------------------------,
#ifdef XASMAVR                     ;
 .TITLE Test: Operator Precedence  ;
 .PAGELENGTH(84)                   ;
 .PAGEWIDTH(130)                   ;
 .SYMBOLS                          ;
 .MODEL BYTE                       ;
; .MODEL WORD                      ;
 .DEVICE ATxmega384C3              ;
#else                              ;
 .DEVICE ATmega2560                ;
 .OVERLAP                          ;
#endif ;XASMAVR                    ;
;----------------------------------'
.SET    _MSET_MASK      =       ~ ((1 << 0) | (1 << 1))             ; =$FC
        .DB     _MSET_MASK
         
.set _p0 =  ((((~((((   ((((0001 )))<<((( 0000))))  ))))|((((  ((((0001 )))<<((( 0001))))   ))))  )))) ;= $FE
        .DB     _p0

.set _p0 = (((((~((((( (((((0001 )))<<((( 0000))))) ))))|(((( (((((0001 )))<<((( 0001))))) ))))) ))))) ;= $FC
        .DB     _p0

.SET    _MSET_MASK      =       ~ ((1 << 0) | (1 << 1) | (1 << 2))  ; =$F8
        .DB     _MSET_MASK
         
.SET    _MSET_MASK      =       ( ~ (1 << 0) | (1 << 1) | (1 << 2)) ; =$FE
        .DB     _MSET_MASK

.SET    _MSET_MASK      =          ((1 << 0) | (1 << 1) | (1 << 2)) ; =$07
        .DB     _MSET_MASK

.SET    _MSET_MASK      =       ~ (1 << 5) & (1 << 4)               ; =$0010
        .DW     _MSET_MASK

.SET    _MSET_MASK      =       ~ ((1 << 5) & (1 << 4))             ; =$FFFF
        .DW     _MSET_MASK

.SET    _MSET_MASK      =         (1 << 5) & (1 << 4)               ; =$0000
        .DW     _MSET_MASK

#ifdef XASMAVR
.EQU    _TCR7_          =       7
.EQU    _TCR6_          =       6
.EQU    _TCR5_          =       5
.EQU    _TCR4_          =       4
.EQU    _TCR3_          =       3
.EQU    _TCR2_          =       2
.EQU    _TCR1_          =       1
.EQU    _TCR0_          =       0

.EQU    TDISABLE_IRQ    =       1 SHL _TCR6_                                                         ;= $40
.EQU    TMODE2          =       NOT (1 SHL _TCR5_) AND (1 SHL _TCR4_)                                ;= $10
.EQU    TSCALE          =       (1 SHL _TCR3_) OR (1 SHL _TCR2_) OR (1 SHL _TCR1_) OR (1 SHL _TCR0_) ;= $0F
.EQU    TIMER_INIT      =       TMODE2 OR TSCALE OR TDISABLE_IRQ                                     ;= $5F


;--------------------------------------------------------------------------
;  7       6        5         4         3         2        1        0
;  ...     ...     _NUM_LED_ _SCR_LED_ _CAP_LED_ _KYBDAT_ _SYSCLK_ _KYBCLK_
;--------------------------------------------------------------------------
.EQU    _KYBCLK_        =       0               ;Keyboard clock  (output)
.EQU    _SYSCLK_        =       1               ;system clock    (input)
.EQU    _KYBDAT_        =       2               ;Keyboard data   (output)
.EQU    _CAP_LED_       =       3               ;Caps-lock LED   (output)
.EQU    _SCR_LED_       =       4               ;Scroll-lock LED (output)
.EQU    _NUM_LED_       =       5               ;Num-lock LED    (output)
;.EQU   NC              =       6               ;NC - not connected
;.EQU   NC              =       7               ;NC - not connected

.EQU    KYBCLK          =       1 SHL _KYBCLK_  ;_KYBCLK_ byte value
.EQU    SYSCLK          =       1 SHL _SYSCLK_  ;_SYSCLK_ byte value
.EQU    KYBDAT          =       1 SHL _KYBDAT_  ;_SYSCLK_ byte value
.EQU    CAP_LED         =       1 SHL _CAP_LED_ ;_CAP_LED_ byte value
.EQU    SCR_LED         =       1 SHL _SCR_LED_ ;_SCR_LED_ byte value
.EQU    NUM_LED         =       1 SHL _NUM_LED_ ;_NUM_LED_ byte value

.EQU    PORTA_MASK      =       NOT (SCR_LED OR CAP_LED OR NUM_LED OR KYBCLK OR KYBDAT) AND $00FF    ;= $C2

Portinittbl:    .DB     PORTA_MASK, $00, $00, $FF ;PORTA .. PORTD                                    ;= C2, ..
                .DB     NOT PORTA_MASK, $00, $01  ;DDRA .. DDRC                                      ;= 3D, ..

;----------------------------------------------------------------------
;  7      6         5         4         3        2       1       0
; _XTAT_ _NUMSTAT_ _PSHIFTL_ _PSHIFTR_ _PNUMLK_ _MSET3_ _MSET2_ _MSET1_
;----------------------------------------------------------------------
.EQU    _MSET1_         =       0               ;Set1
.EQU    _MSET2_         =       1               ;Set2
.EQU    _MSET3_         =       2               ;Set3
.EQU    _PNUMLK_        =       3               ;Pseudo-NUM-lock
.EQU    _PSHIFTR_       =       4               ;Pseudo-Shift-rechts
.EQU    _PSHIFTL_       =       5               ;Pseudo-Shift-links
.EQU    _NUMSTAT_       =       6               ;Internal NUM-state
.EQU    _XTAT_          =       7               ;XT-Mode(1) / AT-Mode(0)

.SET    _MSET_MASK      =       NOT ((1 SHL _MSET1_) | (1 SHL _MSET2_) | (1 SHL _MSET3_)) ; =$F8
                .DB     _MSET_MASK

.SET    _MSET_MASK      =        ((1 SHL _MSET1_) | (1 SHL _MSET2_) | (1 SHL _MSET3_))    ; =$07
                .DB     _MSET_MASK

MF2_F0cmd3:     .DB     NOT ((1 SHL _MSET1_) | (1 SHL _MSET2_) | (1 SHL _MSET3_))         ; =$F8

;----------------------------------------------------------------------                                                    
;  7        6        5        4      3        2        1        0
; _PSHFTL_ _PSHFTR_ _NUMLOK_ _SET3_ _PUTSET_ _POWRON_ _SET2_3_ _ATMODE_
;----------------------------------------------------------------------                                                    
.EQU    _ATMODE_        =       0       ;1 = AT-type protocol, 0 = XT-type
.EQU    _SET2_3_        =       1       ;scan code set: 0 = set1, 1 = set2 or set3
.EQU    _POWRON_        =       2       ;Power-On reset
.EQU    _PUTSET_        =       3       ;F0cmd Opt=00, must send current set in use next 
.EQU    _SET3_          =       4       ;if _SET2_3_ = 1: 1 = scan code set3, 0 = set2
.EQU    _NUMLOK_        =       5       ;NumLock is active
.EQU    _PSHFTR_        =       6       ;system believes ShiftR-key active
.EQU    _PSHFTL_        =       7       ;system believes ShiftL-key active

.EQU    ATMODE          =       1 SHL _ATMODE_  ;_ATMODE_ byte value
.EQU    SET2_3          =       1 SHL _SET2_3_  ;_SET2_3_ byte value
.EQU    POWRON          =       1 SHL _POWRON_  ;_POWRON_ byte value
.EQU    SET3            =       1 SHL _SET3_    ;_SET3_ byte value

reset2:         .DB     POWRON OR ATMODE OR SET2_3 AND NOT SET3   ;= $07

.SET _VAR = 0xA600*256+0x75A2>>8  ;; =0000A675 assumed (result XASMAVR=00A60075)          
        .DW LWRD(_VAR)            ;; =    A675 XASMAVR NOT EXPECTED?!
        .DW HWRD(_VAR)            ;; =    0000 XASMAVR NOT EXPECTED?!

.SET _VAR = 0x1234<<16|0x5678     ;; =12345678
        .DW LWRD(_VAR)            ;; =    5678
        .DW HWRD(_VAR)            ;; =    1234

.DSEG
Variable:       .BYTE 4
VarWord1:       .BYTE 2
VarWord2:       .BYTE 2
Result:         .BYTE 8
.EQU Address1   =VarWord1
.EQU Address2   =VarWord2
.EQU Address    =Variable
.SET Constant   =0x4321

.CSEG
.DEF Temp       =R16
.DEF TempL      =R16
.DEF TempH      =R17
.DEF Flags      =R15
.EQU _Flag1     =1
.EQU _Flag2     =2
.EQU _Flag3     =3
.EQU _signResult=0

.SET _VAR = ((-0-1>0)*0b1111|0-0)<<1|high(Address)&1
        .DW LWRD(_VAR)            ;; =    0000
        .DW HWRD(_VAR)            ;; =    0000

        .DW 0-1<<1|high(Address)&1
;-----------------------------------------------------------------------------------------------------------
;-----------------------------------------------------------------------------------------------------------
;-----------------------------------------------------------------------------------------------------------
#else
.EQU    _TCR7_          =       7
.EQU    _TCR6_          =       6
.EQU    _TCR5_          =       5
.EQU    _TCR4_          =       4
.EQU    _TCR3_          =       3
.EQU    _TCR2_          =       2
.EQU    _TCR1_          =       1
.EQU    _TCR0_          =       0

.EQU    TDISABLE_IRQ    =       1 << _TCR6_                                                  ;= $40
.EQU    TMODE2          =       ~ (1 << _TCR5_) & (1 << _TCR4_)                              ;= $10
.EQU    TSCALE          =       (1 << _TCR3_) | (1 << _TCR2_) | (1 << _TCR1_) | (1 << _TCR0_);= $0F
.EQU    TIMER_INIT      =       TMODE2 | TSCALE | TDISABLE_IRQ                               ;= $5F
        .DB TIMER_INIT
                
;--------------------------------------------------------------------------
;  7       6        5         4         3         2        1        0
;  ...     ...     _NUM_LED_ _SCR_LED_ _CAP_LED_ _KYBDAT_ _SYSCLK_ _KYBCLK_
;--------------------------------------------------------------------------
.EQU    _KYBCLK_        =       0               ;Keyboard clock  (output)
.EQU    _SYSCLK_        =       1               ;system clock    (input)
.EQU    _KYBDAT_        =       2               ;Keyboard data   (output)
.EQU    _CAP_LED_       =       3               ;Caps-lock LED   (output)
.EQU    _SCR_LED_       =       4               ;Scroll-lock LED (output)
.EQU    _NUM_LED_       =       5               ;Num-lock LED    (output)
;.EQU   NC              =       6               ;NC - not connected
;.EQU   NC              =       7               ;NC - not connected

.EQU    KYBCLK          =       1 << _KYBCLK_   ;_KYBCLK_ byte value
.EQU    SYSCLK          =       1 << _SYSCLK_   ;_SYSCLK_ byte value
.EQU    KYBDAT          =       1 << _KYBDAT_   ;_SYSCLK_ byte value
.EQU    CAP_LED         =       1 << _CAP_LED_  ;_CAP_LED_ byte value
.EQU    SCR_LED         =       1 << _SCR_LED_  ;_SCR_LED_ byte value
.EQU    NUM_LED         =       1 << _NUM_LED_  ;_NUM_LED_ byte value

.SET    PORTA_MASK      =       ~ (SCR_LED | CAP_LED | NUM_LED | KYBCLK | KYBDAT) & $00FF    ;= $C2
.SET    PORTA_MASK      =       (SCR_LED | CAP_LED | NUM_LED | KYBCLK | KYBDAT) & $00FF      ;= $3D
.SET    PORTA_MASK      =       ~((SCR_LED | CAP_LED | NUM_LED | KYBCLK | KYBDAT) & $00FF)   ;= $C2
.SET    PORTA_MASK      =       ~((SCR_LED | CAP_LED | NUM_LED | KYBCLK | KYBDAT) & $00FF)   ;= $C2

Portinittbl:    .DB     PORTA_MASK, $00, $00, $FF ;PORTA .. PORTD                            ;= $C2, ..
                .DB     ~ PORTA_MASK, $00, $01  ;DDRA .. DDRC                                ;= $3D, ..


;----------------------------------------------------------------------
;  7      6         5         4         3        2       1       0
; _XTAT_ _NUMSTAT_ _PSHIFTL_ _PSHIFTR_ _PNUMLK_ _MSET3_ _MSET2_ _MSET1_
;----------------------------------------------------------------------
.EQU    _MSET1_         =       0               ;Set1
.EQU    _MSET2_         =       1               ;Set2
.EQU    _MSET3_         =       2               ;Set3
.EQU    _PNUMLK_        =       3               ;Pseudo-NUM-lock
.EQU    _PSHIFTR_       =       4               ;Pseudo-Shift-rechts
.EQU    _PSHIFTL_       =       5               ;Pseudo-Shift-links
.EQU    _NUMSTAT_       =       6               ;Internal NUM-state
.EQU    _XTAT_          =       7               ;XT-Mode(1) / AT-Mode(0)

.SET    _MSET_MASK      =       ~ ((1 << _MSET1_) | (1 << _MSET2_) | (1 << _MSET3_)) ; =$F8
                .DB     _MSET_MASK

.SET    _MSET_MASK      =        ((1 << _MSET1_) | (1 << _MSET2_) | (1 << _MSET3_))  ; =$07
                .DB     _MSET_MASK

MF2_F0cmd3:     .DB     ~ ((1 << _MSET1_) | (1 << _MSET2_) | (1 << _MSET3_))         ; =$F8


;----------------------------------------------------------------------                                                    
;  7        6        5        4      3        2        1        0
; _PSHFTL_ _PSHFTR_ _NUMLOK_ _SET3_ _PUTSET_ _POWRON_ _SET2_3_ _ATMODE_
;----------------------------------------------------------------------                                                    
.EQU    _ATMODE_        =       0       ;1 = AT-type protocol, 0 = XT-type
.EQU    _SET2_3_        =       1       ;scan code set: 0 = set1, 1 = set2 or set3
.EQU    _POWRON_        =       2       ;Power-On reset
.EQU    _PUTSET_        =       3       ;F0cmd Opt=00, must send current set in use next 
.EQU    _SET3_          =       4       ;if _SET2_3_ = 1: 1 = scan code set3, 0 = set2
.EQU    _NUMLOK_        =       5       ;NumLock is active
.EQU    _PSHFTR_        =       6       ;system believes ShiftR-key active
.EQU    _PSHFTL_        =       7       ;system believes ShiftL-key active

.EQU    ATMODE          =       1 << _ATMODE_   ;_ATMODE_ byte value
.EQU    SET2_3          =       1 << _SET2_3_   ;_SET2_3_ byte value
.EQU    POWRON          =       1 << _POWRON_   ;_POWRON_ byte value
.EQU    SET3            =       1 << _SET3_     ;_SET3_ byte value

reset2:         .DB     POWRON | ATMODE | SET2_3 & ~ SET3   ;= $07

.SET _VAR = 0xA600*256+0x75A2>>8  ;; =0000A675 assumed (result XASMAVR=00A60075)          
        .DW LWRD(_VAR)            ;; =    A675 XASMAVR NOT EXPECTED?!
        .DW HWRD(_VAR)            ;; =    0000 XASMAVR NOT EXPECTED?!

.SET _VAR = 0x1234<<16|0x5678
        .DW LWRD(_VAR)            ;; =    5678
        .DW HWRD(_VAR)            ;; =    1234

.DSEG
Variable:       .BYTE 4
VarWord1:       .BYTE 2
VarWord2:       .BYTE 2
Result:         .BYTE 8
.EQU Address1   =VarWord1
.EQU Address2   =VarWord2
.EQU Address    =Variable
.SET Constant   =0x4321

.CSEG
.DEF Temp       =R16
.DEF TempL      =R16
.DEF TempH      =R17
.DEF Flags      =R15
.EQU _Flag1     =1
.EQU _Flag2     =2
.EQU _Flag3     =3
.EQU _signResult=0

.SET _VAR = ((-0-1>0)*0b1111|0-0)<<1|high(Address)&1
        .DW LWRD(_VAR)                ;; = 0000
        .DW HWRD(_VAR)                ;; = 0000

        .DW 0-1<<3|high(Address)&1    ;= fffe  
        .DW (0-1<0)<<3|high(Address)&1;= fffe  

        .DW 0-1<<3|2&1                ;= fff8  
        .DW (0-1<<3)|(2&1)            ;= fff8  

        .DW 0-1<<3                    ;= fff8  

        .DW 0-1<<3|high(Address)^5    ;= ffff  
        .DW 0-1<<3|2^5                ;= ffff  

        .DW 0-1<<3|high(Address)|4    ;= fffe  
        .DW 0-1<<3|2|4                ;= fffe  

#endif
; ----------------------------------------------------------
.EXIT                    
