----------------------------
### XASM6805 Quick Reference
----------------------------
To assemble an MC68HC05 source file, run `XASM6805 myproj.asm`.  
You will end up with a listing at `myproj.LST` of the source file,  
and with `myproj.s19` in Motorola S-Record format. Optionally `myproj.bin` can be generated.    

#### List File Formatting Directives
.MODEL NOINFO..................     		Suppress some info displayed on console  
.TITLE text................................    	Title in page header  
.SUBTTL text............................       	Subtitle in page header  
.PAGELENGTH(number)......  			Lines per page   
.PAGEWIDTH(number) .......   			Colums per page  
.EJECT........................................      	New listing page ejected ('FormFeed')  
.SYMBOLS................................           	Symbol map appended in listing  
.NOSYMBOLS ........................          	Listing without symbol map (default)  
.NOLISTMACRO ...................         	Disable listing of macro expansion (default)  
.NOLISTMAC...........................         	Disable listing of macro expansion (default)  
.LISTMACRO............................          	Enable listing of macro expansion  
.LISTMAC..................................         	Enable listing of macro expansion  
.NOLIST.....................................	Suppress source lines in listing  
.LIST ...........................................	Show source lines in listing (default)  

#### Commandline options:

``` hlp
M68HC05 Macro-Assembler, Version 2.1  
XASM6805 [/options] srcfile.asm | srcfile.s  
  /Bp0xFF    Generate a .BIN-File with padding=0xFF for unused gaps  
  /Bp0x00    Generate a .BIN-File with padding=0x00 for unused gaps  
  /AmOFF     Suppress activity monitor display  
  /D<symbol> Define text symbol    
  Note: [/options] are case-sensitive.  
```

#### Code alignment  
 .EVEN                  Code alignment on an even address boundary.

#### Other directives
[symbol:]...    FCB...... expr,[[expr],["string"],['c'],...]...  ;Form Constant Byte (Allocates bytes)  
[symbol:]...    FDB...... expr,[[expr],...]..............................  ;Form Double Byte Constant (Allocates Words)  
[symbol:]...    FDW.....  expr,[[expr],...]..............................  ;Form Double Word Constant (Allocates Dwords)  
[symbol:]...    RMB.....  expr,[[expr],...]..............................  ;Reserve Memory Block (Defines storage space)  

...................... ORG...    expr... ;Sets the location counter to expr.  
constant....      EQU...    expr... ;Assigns expr constant (silently truncated to 16bit).  
symbol.......      SET....   expr.... ;Assign a redefinable symbol equal to an 16bit expression (truncated to 16bit)  
...................... END................ ;Marks the end of a program.

Directives must be given at start of the source line.
Symbols (labels) end with colon ':'.  
No colon is allowed with constant/symbols defined by the EQU/SET directives.

#### Comments
Example Listing

``` com
 LOC   OBJ               LINE   SOURCE

 0000  9D                   1           nop
                                    2   ; standard assembler comment
                                    5   /* Multiline text block.
                                    6      The enclosed text is a comment
                                    7   */ Note: Block marker must be first in text line.
 0001  9D                   8           nop 
```

####  Line continuation
 Source lines with FCB FDB FDW directives can be continued by backslash '\\' as the last character.  
Example Listing

``` lin
 LOC   OBJ               LINE   SOURCE

 = 00000011                 9   _11 SET $11
 = 00000012                10   _12 SET $12
 = 00000013                11   _13 SET $13
 0030                      12   _fcb3:  FCB     _11,_12,_13, 'Z'+1, \
                                                %10101, %1, %101,   \
                                                "1234567890abcd ",  \
                                                'A',_fcb3 AND $FF,  \
                                                "1234567890abcde"   \
                                                $FF,$FF
 0030  11 12 13 5B 15     
 0035  01 05 31 32 33  
 003A  34 35 36 37 38  
 003F  39 30 61 62 63  
 0044  64 20 41 18 31  
 0049  32 33 34 35 36  
 004E  37 38 39 30 61  
 0053  62 63 64 65 FF  
 0058  FF              
```

#### Macros (Atmel/Microchip style)
.MACRO  
.ENDM  
.ENDMACRO    
Example Listing
    
``` mac
 LOC   OBJ               LINE   SOURCE

                          629   .MACRO Addition 
                          630           lda     #@0     ; param0                
                          631           add     #@1     ; param1
                          632   .ENDM
                          633   
 0322                  +  634           ADDITION $10,20
 0322  A6 10           +  635           lda     #$10 
 0324  AB 14           +  636           add     #20 
 0326  C7 01 00           637           STA     $100 
 0329                  +  638           ADDITION $12,15
 0329  A6 12           +  639           lda     #$12 
 032B  AB 0F           +  640           add     #15 
 032D  C7 01 01           641           STA     $101
 0330  83                 642           SWI
```

#### Conditional assembly
.DEFINE  
.UNDEF  
.IFDEF  
.IF DEFINED  
.IFNDEF  
.IF !DEFINED  
.IF  
.ELIF  
.ELSEIF  
.ELIF DEFINED  
.ELIF !DEFINED  
.ELSE  
.ENDIF  

#define  
#undef  
#ifdef  
#if defined  
#ifndef  
#if !defined  
#if  
#elif  
#elseif  
#elif defined  
#elseifdef  
#elif !defined  
#elseifndef  
#else  
#endif  

#### Include files  

``` inc  
.INCLUDE "123def.inc"
#include <123def.inc>
```
#### Messages
.MESSAGE, .WARNING, .ERROR - Display information on console during assembly  
Example
  
``` msg
.MESSAGE "Info text"
.WARNING "Warning info text"
.ERROR   "Error: Assembly aborted text"

#message "Info text"
#warning "Warning info text"
#error   "Error: Assembly aborted text"
```

#### Functions Provided by the assembler.
 LOW(expression)..........returns the low byte of an expression  
 HIGH(expression).........returns the second byte of an expression  
 BYTE2(expression)......  is the same function as HIGH  
 BYTE3(expression)......  returns the third byte of an expression  
 BYTE4(expression)......  returns the fourth byte of an expression  
 LWRD(expression)......  returns bits 0-15 of an expression  
 HWRD(expression).....  returns bits 16-31 of an expression  
 PAGE(expression)........ returns bits 16-21 of an expression  

 EXP2(expression)..........returns 2 to the power of expression  
 LOG2(expression).........returns the integer part of log2(expression)  

 DEFINED(symbol). Returns true if symbol is previously defined using .EQU /.SET /.DEF directives.  
  Normally used in conjunction with .IF directives (.IF DEFINED(xyz)), but may be used in any context.  
  It only makes sense to use a single symbol as argument.    
Example Listing

``` def
 LOC   OBJ               LINE   SOURCE

                          202   ;--------------------------------------------
 = 00000001               203   #define _flag1
 = 00000001               204   .DEFINE _flag2
 009E  01 00              205           FCB DEFINED(_flag1), !DEFINED(_flag1)
 00A0  01 00              206           FCB DEFINED(_flag2), !DEFINED(_flag2)
                          207   
                          208   #if DEFINED(_flag1)
 00A2  9D                 209           nop             ; flag1
                          210   #elif DEFINED(_flag2)
                          211   #endif
                          212   
                          213   #if !DEFINED(_flag1)    
                          214   #elif DEFINED(_flag2)
 00A3  9D                 215           nop             ; flag2 
                          216   #endif
```

#### Operands - The following operands can be used:
 User defined labels, which are given the value of the location counter at the place they appear.  
 User re-definable symbols with the SET directive  
 User defined constants with the EQU directive  
 Constants can be given in several formats:
 - Decimal (default):                   10, 255, 65536  
 - Hexadecimal (three notations):       0x10, $10, 10h, 0xFF, $FF, 0FFh  
 - Binary (three notations):            0b00001010, %11111111, 11111111b  
 - Octal: not supported.                        -             
 - Current Program memory location counter (two notations): '*', '$'  

#### Operators - The Assembler supports a number of operators

``` op
    !      Logical not
    ~ NOT  Bitwise Not     (XASM Feature: 'NOT' is the same as '~')
    -      Unary Minus
    *      Multiplication
    /      Division
    %  MOD Modulo          (XASM Feature: 'MOD' is the same as '%')
    +      Addition
    -      Subtraction
    << SHL Shift left      (XASM Feature: 'SHL' is the same as '<<')
    >> SHR Shift right     (XASM Feature: 'SHR' is the same as '>>')
    <      Less than (LT)          
    <=     Less than or equal (LE)
    >      Greater than (GT)
    >=     Greater than or equal (GE)
    ==     Equal (EQ)
    !=     Not equal (NE)
    & AND  Bitwise And     (XASM Feature: 'AND' is the same as '&')
    ^ XOR  Bitwise Xor     (XASM Feature: 'XOR' is the same as '^')
    | OR   Bitwise Or      (XASM Feature: 'OR'  is the same as '|')
    &&     Logical And
    ||     Logical Or
```

#### Operator precedence
 To yield the expected result of a complex expression,  
 especially when arithmetic operators are mixed with logical operators,  
 **the usage of parentheses is strongly recommended**.  
 Assemblers/Compilers may sometimes slightly differ about the operator precedence rules    
 in such expressions, generating a result that may not always be expected.  
Example Listing

``` pre
 LOC   OBJ               LINE   SOURCE

                          398   ;-----------------------------------------------------------------,  
                          399   ; Warning: Use parenthesis in complex expressions!		  |  
 = 0000A675               400   _VAR SET $a600*256+$75a2>>8	     ;; =00A60075 NOT EXPECTED ?! |  
 1281  A675               401   	FDB _VAR		     ;                            |  
 = 00A60075               402   _VAR SET ($a600*256)+($75a2>>8)      ;; =00a60075 expected        |  
 1283  0075               403   	FDB LWRD(_VAR)               ;	                          |  
 1285  00A6               404   	FDB HWRD(_VAR)               ;	                          |  
 = 0000A675               405   _VAR SET ($a600*256+$75a2)>>8        ;; =0000a675 expected        |   
 1287  A675               406   	FDB _VAR                     ;	                          |  
                          407   ;-----------------------------------------------------------------'  
```

#### Pre-defined Macros
 Note: %DATE%  %TIME%.. etc, -  this format is not supported. Using __DATE__ __TIME__ instead  

\_\_FILE__  
\_\_DATE__  
\_\_TIME__  
\_\_CENTURY__  
\_\_YEAR__                                             
\_\_MONTH__  
\_\_DAY__  
\_\_HOUR__   
\_\_MINUTE__  
\_\_SECOND__  

#### Listing with formatted Symbol Map (values >16bit are shown truncated)

Example  

``` map
 MC68HC05 Macro-Assembler, Version 2.1                    06/01/2025  PAGE   9
 Test: MC68HC05 Instruction set

  LOC   OBJ               LINE   SOURCE

                       464           END

 USER SYMBOLS

_11 . . . . . . . .  0011 A  _12 . . . . . . . .  0012 A  _13 . . . . . . . .  0013 A  _2345678901234567890 0098 C    
_CAP_LED_ . . . . .  0003 A  _db01c. . . . . . .  02BA C  _DB01C_LENGTH . . .  0015 A  _db09 . . . . . . .  02F8 C    
_db10 . . . . . . .  030B C  _db3. . . . . . . .  02CF C  _FDB1 . . . . . . .  0320 C  _flag1. . . . . . .  0001 A    
_flag2. . . . . . .  0001 A  _KYBCLK_. . . . . .  0000 A  _KYBDAT_. . . . . .  0002 A  _l1 . . . . . . . .  0135 C    
_NUM_LED_ . . . . .  0005 A  _SCR_LED_ . . . . .  0004 A  _SYSCLK_. . . . . .  0001 A  _testUPI. . . . . .  0001 A    
_VAR. . . . . . . .  A675 A  altprint. . . . . .  0042 C  CAP_LED . . . . . .  0008 A  chksum. . . . . . .  0F38 C    
cmdparm . . . . . .  003C C  cmdtab. . . . . . .  03BE C  CMDTAB_L. . . . . .  0039 A  DDRA. . . . . . . .  0004 C    
DDRB. . . . . . . .  0005 C  DDRC. . . . . . . .  0006 C  delay_count . . . .  0043 C  docmd . . . . . . .  03B1 A    
EDcmd . . . . . . .  03F7 C  EEcmd . . . . . . .  03F7 C  EFcmd . . . . . . .  03F7 C  F0cmd . . . . . . .  03F7 C    
F1cmd . . . . . . .  03F7 C  F2cmd . . . . . . .  03F7 C  F3cmd . . . . . . .  03F7 C  F4cmd . . . . . . .  03F7 C    
F5cmd . . . . . . .  03F7 C  F6cmd . . . . . . .  03F7 C  F7cmd . . . . . . .  03F7 C  F8cmd . . . . . . .  03F7 C    
F9cmd . . . . . . .  03F7 C  FAcmd . . . . . . .  03F7 C  FBcmd . . . . . . .  03F7 C  FCcmd . . . . . . .  03F7 C    
FDcmd . . . . . . .  03F7 C  FEcmd . . . . . . .  03F7 C  FFcmd . . . . . . .  03F7 C  info. . . . . . . .  0080 C    
INFO_LEN. . . . . .  0018 A  INSTRUC_L . . . . .  0006 A  INSTRUCTIONS_END. .  02A4 A  IOAREA. . . . . . .  0000 A    
IOAREA_SIZE . . . .  0010 A  KYBCLK. . . . . . .  0001 A  KYBDAT. . . . . . .  0004 A  lastchr . . . . . .  0045 C    
MANUF_MODE. . . . .  0F00 A  MANUF_MODE_L. . . .  000F A  manufMod2 . . . . .  0F00 A  manufMod2_1 . . . .  0F02 C    
MC68HC05_instruction 0100 C  mtrxSenseIndex. . .  0046 C  mtrxSenseNew. . . .  0047 C  mtrxSenseOld. . . .  0048 C  
 
 ASSEMBLY COMPLETE,   NO ERRORS
```




















