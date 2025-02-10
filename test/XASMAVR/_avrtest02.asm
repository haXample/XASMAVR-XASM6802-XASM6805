;#define XASMAVR via commandline option '/DXASMAVR'
;----------------------------------,
#ifdef XASMAVR                     ;
 .TITLE Test: FILE/DATE/TIME - Assembler supported inbuild Macros
 .PAGELENGTH(84)                   ;
 .PAGEWIDTH(120)                   ;
 .SYMBOLS                          ;
 .MODEL BYTE                       ;
 .DEVICE ATxmega384C3              ;
#else                              ;
 .DEVICE ATmega2560                ;
#endif ;XASMAVR                    ;
;----------------------------------'

#ifdef XASMAVR
        .DB __FILE__
                 
        .DB __DATE__

        .DB __TIME__

        .DB __DATE__, __TIME__, \
            0x00

        .DB "__MONTH__", '/', "__DAY__", '/', "__YEAR__"

        .DB "__HOUR__", ':', "__MINUTE__", ':', "__SECOND__"

        .DB __DAY__
        .DB "__DAY__"

        .DB __MONTH__
        .DB "__MONTH__"

        .DW __CENTURY__
        .DB "__CENTURY__"

        .DW __YEAR__                                           
        .DB "__YEAR__"

        .DB __HOUR__ 
        .DB "__HOUR__"

        .DB __MINUTE__
        .DB "__MINUTE__"

        .DB __SECOND__
        .DB "__SECOND__"

        ;.DB __LINE__   ; Integer, not implemted
#else
;;ha;;  .DB  %FILE%
;;ha;;  .DB  %DATE%
        .DB  "%MONTH%", '/', "%DAY%", '/', "%YEAR%"
;;ha;;  .DB  %TIME%
        .DB  "%HOUR%", ':', "%MINUTE%" ;;ha;;, ':', "%SECOND%"
        .DB  %DAY%
        .DB  "%DAY%"
        .DB  %MONTH%
        .DB  "%MONTH%"
;;ha;;  .DW  %CENTURY%
;;ha;;  .DB  "%CENTURY%"
        .DW  %YEAR%                                           
        .DB  "%YEAR%"
        .DB  %HOUR% 
        .DB  "%HOUR%"
        .DB  %MINUTE%
        .DB  "%MINUTE%"
;;ha;;  .DB  %SECOND%
;;ha;;  .DB  "%SECOND%"
#endif ;XASMAVR

#ifdef XASMAVR
; -------------------------------------------
.SUBTTL Hex-file "_avrtest02.HEX" (example)
.EJECT
; -------------------------------------------
/*                                           |
 :020000020000FC                             |
 :100000005F6176727465737430322E41534D3235B0 | 
 :100010002F31322F3230323431343A35303A3039B0 | 
 :1000200032352F31322F3230323431343A35303AA2 | 
 :100030003039000031322F32352F32303234313303 | 
 :100040003A35303A3039190032350C00313215006A | 
 :100050003231E807323032340D003133320035307E |
 :04006000090030392A                         |
 :00000001FF                                 |
*/                                           |
; -------------------------------------------

; --------------------------------------------------------------------------
.SUBTTL Bin-file "_avrtest02.BIN", ascii-dump (ATxmega384C3 384K example)
.EJECT
; --------------------------------------------------------------------------
/*                                                                          |
00000000  5F 61 76 72 74 65 73 74-30 32 2E 41 53 4D 32 35  _avrtest02.ASM25 |
00000010  2F 31 32 2F 32 30 32 34-31 34 3A 35 30 3A 30 39  /12/202414:50:09 |
00000020  32 35 2F 31 32 2F 32 30-32 34 31 34 3A 35 30 3A  25/12/202414:50: |
00000030  30 39 00 00 31 32 2F 32-35 2F 32 30 32 34 31 33  09..12/25/202413 |
00000040  3A 35 30 3A 30 39 19 00-32 35 0C 00 31 32 15 00  :50:09..25..12.. |
00000050  32 31 E8 07 32 30 32 34-0D 00 31 33 32 00 35 30  21..2024..132.50 |
00000060  09 00 30 39 FF FF FF FF-FF FF FF FF FF FF FF FF  ..09............ |
00000070  FF FF FF FF FF FF FF FF-FF FF FF FF FF FF FF FF  ................ |
  ....                                                                      |
                                                                            |
0005FFE0  FF FF FF FF FF FF FF FF-FF FF FF FF FF FF FF FF  ................ |
0005FFF0  FF FF FF FF FF FF FF FF-FF FF FF FF FF FF FF FF  ................ |
*/                                                                          |
; --------------------------------------------------------------------------
#endif ;XASMAVR
.EXIT

