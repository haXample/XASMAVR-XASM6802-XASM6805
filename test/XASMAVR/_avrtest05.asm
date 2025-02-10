;#define XASMAVR via commandline option '/DXASMAVR'
;----------------------------------,
#ifdef XASMAVR                     ;
 .TITLE Test: .DB - Assembler Directive and Line Continuation
 .PAGELENGTH(90)                   ;
 .PAGEWIDTH(300)                   ;
 .SYMBOLS                          ;
 .MODEL BYTE                       ;
 .DEVICE ATxmega384C3              ;
#else                              ;
 .DEVICE ATmega2560                ;
#endif ; XASMAVR                   ;
;----------------------------------'

.EQU REV='2'
.EQU REV0='1'
.EQU REV00='0'
.SET _10 = 10
.SET _13 = 13
 
#ifdef XASMAVR                     ;
_db00   .DB     "XASMAVR Macro Assembler ", \
                'V',REV OR 30h,'.',REV0 OR 30h
        .EQU _DB00_LENGTH = STRLEN()

        .DB     STRLEN("XASMAVR Macro Assembler V2.1"), _DB00_LENGTH 
phase_test_db00:

_db01a:;
_db01b:
_db01c  .DB     _DB01C_LENGTH, 1,13,'012"3":; ,"', '", "', 'A', '"', ''', '', "";, 22, 25, 4 \
        .EQU _DB01C_LENGTH = STRLEN()
phase_test_db01c:

_db02   .DB     _DB02_LENGTH, 1,13,"012'3':; ,'", "', '", 'A', '"', ''', '', "", 22, 25, 4 
        .EQU _DB02_LENGTH = STRLEN()
phase_test_db02:
#endif ;XASMAVR

        .DB     _13,_10,'Z'+1, 0b10101, 0b1,  \
                0b10, "1234567890abcd ", 'A', \
                0xFF, "1234567890abcde ";,
        .DB STRLEN()

        .DB     _13,_10,'Z'+1, 0b10101, 0b1,  \
                0b10, "1234567890abcd ", 'A', \
                0xFF, "1234567890abcde ",1;,
        .DB STRLEN()

; -------------------------------------------
#ifdef XASMAVR
.SUBTTL More .DB-Testing 
.EJECT
#endif ; XASMAVR
; -------------------------------------------
_db03:  .DB 255, 0b101, 0x0C/2, 87%8 ,__TIME__, ''',"",'',"','",'\','h','a','X','A','S','M','A','V','R','|',';','ä','ö','ü',"<<",'%',$FF 
phase_test_db03:

_db04:  .DB     24, 15,  6, 19, 20, 28, 20, 28, 11, 27, 16,  0, \
                16,  0, 14, 22, 25,  4, 25,  4, 17, 30,  9,  1, \
                 9,  1,  7, 23, 13, 31, 13, 31, 26,  2,  8, 18, \
                16,  0, 14, 22, 25,  4, 25,  4, 17, 30,  9,  1, \
                 9,  1,  7, 23, 13, 31, 13, 31, 26,  2,  8, 18, \
                 8, 18, 12, 29,  5, 21,  5, 21, 10, '\', "\\"

_db05:  .DB     0x28,$11, 0b00100111, $16, 0, $16, 0, 14, (' '+1)*2, $25,$4,$25, \
                'Z'+1,$FF,$FF,"1234567890abcd " ,"1234567890abcde ",$FF,$FF;, \\ 

_db06:  .DB     "1234567890abrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrcdef 12345"
phase_test_db06:

; -------------------------------------------
#ifdef XASMAVR
.EJECT
#endif ; XASMAVR
; -------------------------------------------
_db07:  .DB     14, \                   
                "xx12345 ,", 'A'

_db08:  .DB     'A', \                  
                "xx12345 ,", 14, 15

#ifdef XASMAVR
_db09   .DB     __DATE__, __TIME__, __CENTURY__
                
_db10   .DB     __DATE__, __TIME__, '__CENTURY__', \
                0x00

.db "__YEAR____MONTH____DAY____HOUR____MINUTE__" , \
    "__YEAR____MONTH____DAY____HOUR____MINUTE__"

.db "__YEAR__","__MONTH__","__DAY__","__HOUR__","__MINUTE__" , \
    "__YEAR__","__MONTH__","__DAY__","__HOUR__","__MINUTE__"

.db "__YEAR__",'/',"__MONTH__",'/',"__DAY__",' ',"__HOUR__",':',"__MINUTE__" , \
    "__YEAR__",'/',"__MONTH__",'/',"__DAY__",' ',"__HOUR__",':',"__MINUTE__"
#endif ; XASMAVR

.EXIT
; -------------------------------------------
