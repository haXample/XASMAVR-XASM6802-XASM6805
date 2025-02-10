----------------------------
### XASM8042 Quick Reference
----------------------------
To assemble an 8042 / UPI-C42 source file, run `XASM8042 mykbcproj.asm`.  
You will end up with a listing at `mykbcproj.LST` of the source file,  
and with `mykbcproj.hex` in Intel HEX format. Optionally `mykbcproj.bin` can be generated.    

#### List File Formatting Directives
.TITLE text.............................    Title in page header  
.SUBTTL text.........................       Subtitle in page header  
.PAGELENGTH(number).......  		    Lines per page   
.PAGEWIDTH(number) .........   		    Colums per page  
.EJECT...................................   New listing page ejected ('FormFeed')  
.SYMBOLS............................        Symbol map appended in listing  
.NOSYMBOLS ......................           Listing without symbol map (default)  
.NOLIST.................................    Suppress source lines in listing  
.LIST ..................................... Show source lines in listing (default)  

#### Commandline options:

``` hlp
UPI-41/C42 Cross-Assembler, Version 2.1  
XASM8042 [/options] srcfile.asm | srcfile.s  
  /Bp0xFF    Generate a .BIN-File with padding=0xFF for unused gaps  
  /Bp0x00    Generate a .BIN-File with padding=0x00 for unused gaps  
  /AmOFF     Suppress activity monitor display  
  /D<symbol> Define text symbol    
  Note: [/options] are case-sensitive.  
```

#### Code alignment  
 .EVEN                  Code alignment on an even address boundary.  

#### Other directives
[symbol:]...    DB...... expr,[[expr],["string"],['c'],...]...    ;Define Byte (Allocates bytes)  
[symbol:]...    DW.....  expr,[[expr],...]....................... ;Define Word 32bit Constant (truncated if >32bit)  
[symbol:]...    DS...... expr,[[expr],...]....................... ;Define storage space  

................. ORG...    expr... ;Sets the location counter to expr.  
constant....      EQU...    expr... ;Assigns expr constant (silently truncated to 16bit).  
symbol......      SET....   expr... ;Assign a redefinable symbol equal to an 16bit expression (truncated to 16bit)  
................. END.............. ;Marks the end of a program.

Directives must be given at start of the source line.  
Symbols (Labels) end with colon ':'.  
No colon is allowed with constant/symbols defined by the EQU/SET directives.  

#### Comments
Example Listing

``` com
 LOC   OBJ               LINE   SOURCE
 
 0000  00                   1           nop
                            2   ; standard assembler comment
                            5   /* Multiline text block.
                            6      The enclosed text is a comment
                            7   */ Note: Block marker must be first in text line.
 0001  00                   8           nop 
```

####  Line continuation
 Source lines with DB DW directives can be continued by backslash '\\' as the last character.  
Example Listing

``` lin
 LOC   OBJ               LINE   SOURCE

 = 00000011                 9   _11 SET 11h
 = 00000012                10   _12 SET 12h
 = 00000013                11   _13 SET 13h
 0030                      12   _db3:   DB _11,_12,_13, 'Z'+1, \
                                           10101b, 1b, 101b,   \
                                           "1234567890abcd ",  \
                                           'A',_db3 AND 0FFh,  \
                                           "1234567890abcde"   \
                                           0FFh,0FFh
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

#### Include files

``` inc  
.INCLUDE(123def.inc)  
.INCLUDE "123def.inc"  
#include <123def.inc>  
```

#### Functions provided by the assembler.
 LOW(expression)..........returns the low byte of an expression  
 HIGH(expression).........returns the second byte of an expression in DB directive  
 BYTE2(expression)......  is the same function as HIGH (DB DW directive)   
 BYTE3(expression)......  returns the third byte of an expression  
 BYTE4(expression)......  returns the fourth byte of an expression  
 LWRD(expression)........ returns bits 0-15 of an expression  
 HWRD(expression).......  returns bits 16-31 of an expression  
 PAGE(expression)........ returns bits 16-21 of an expression  

 EXP2(expression).........returns 2 to the power of expression  
 LOG2(expression).........returns the integer part of log2(expression)  

#### Operands - The following operands can be used:
 User defined labels, which are given the value of the location counter at the place they appear  
 User re-definable symbols with the SET directive  
 User defined constants with the EQU directive  
 Constants can be given in several formats:  
 - Decimal (default):   10, 255, 65536  
 - Hexadecimal:         10h, 0FFh  
 - Binary:              11111111b, 01b  
 - Octal: not supported.             
 - Current program memory location counter: '$'

#### Operators - The Assembler supports a number of operators

``` op
     !      Logical not
     NOT    Bitwise Not     
     -      Unary Minus
     *      Multiplication
     /      Division
     MOD    Modulo          
     +      Addition
     -      Subtraction
     SHL    Shift left      
     SHR    Shift right     
     <      Less than (LT)          
     <=     Less than or equal (LE)
     >      Greater than (GT)
     >=     Greater than or equal (GE)
     ==     Equal (EQ)
     !=     Not equal (NE)
     AND    Bitwise And
     XOR    Bitwise Xor
     OR     Bitwise Or 
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
 
                          414   ;-----------------------------------------------------------------,
                          415   ; Warning: Use parenthesis in complex expressions!                |
 = A675                   416   _VAR SET 0a600h*256+75a2h SHR 8     ;; =00A60075 NOT EXPECTED ?!  |
 07A7  A675               417   	DW _VAR		     	    ;				  |
 = 0075                   418   _VAR SET (0a600h*256)+(75a2h SHR 8) ;; =00a60075 expected         |
 07A9  0075               419   	DW LOW(_VAR)                ;				  |
 07AB  A600               420   	DW _VAR SHR 8               ;				  |
 = A675                   421   _VAR SET (0a600h*256+75a2h) SHR 8   ;; =0000a675 expected         |
 07AD  A675               422   	DW _VAR                     ;				  |
                          423   ;-----------------------------------------------------------------'
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
 UPI-41/C42 Macro-Assembler, Version 2.1                    06/01/2025  PAGE   9
 Test: UPI-C42 Instruction set
 
  LOC   OBJ               LINE   SOURCE
 
                           464           END
 
 USER SYMBOLS
 
_11 . . . . . . . .  0011 A  _12 . . . . . . . .  0012 A  _13 . . . . . . . .  0013 A  _db01c. . . . . . .  0717 C    
_DB01C_LENGTH . . .  0014 A  _db09 . . . . . . .  0754 C  _db3. . . . . . . .  072B C  _DW1. . . . . . . .  077C C    
_testUPI. . . . . .  0001 A  _VAR. . . . . . . .  A675 A  actPW . . . . . . .  005E C  actPWfld. . . . . .  005C C    
addr. . . . . . . .  0102 C  amovp0. . . . . . .  00E7 C  amovp1. . . . . . .  0141 C  amovp2. . . . . . .  0200 C    
amovp4. . . . . . .  0400 C  amovp5. . . . . . .  0500 C  amovp6. . . . . . .  0600 C  amovp7. . . . . . .  0700 C    
chkrom. . . . . . .  07D5 C  cmdA2 . . . . . . .  07BA C  cmdA3 . . . . . . .  07BC C  cmdA5 . . . . . . .  07BE C    
cmdA6 . . . . . . .  07C0 C  cmdA6_ack . . . . .  0033 C  cmdA9 . . . . . . .  07C2 C  cmdAA . . . . . . .  07C4 C    
cmdAB . . . . . . .  07C6 C  cmdAX . . . . . . .  07AF C  cmdjmp_AX . . . . .  07B3 C  db10. . . . . . . .  0767 C    
HWreset . . . . . .  0000 C  info. . . . . . . .  0002 C  INFO_LEN. . . . . .  001D A  L_ACTPWFLD. . . . .  0004 A    
lshift. . . . . . .  07FB C  lshret. . . . . . .  0808 C  MCYCLE. . . . . . .  04E2 A  PAGESIZE. . . . . .  0100 A    
PWactive. . . . . .  005C C  PWinst. . . . . . .  005D C  pwmatch_ack . . . .  0034 C  pwskip. . . . . . .  0036 C    
RAMAVAIL. . . . . .  0020 A  RAMSIZE . . . . . .  0080 A  RBANK0. . . . . . .  0000 C  RBANK1. . . . . . .  0018 C    
ROMSIZE . . . . . .  1000 A  rwoffs. . . . . . .  002B C  STACK . . . . . . .  0008 A  STACKSIZE . . . . .  0010 A    
stdly . . . . . . .  07CD C  sterr . . . . . . .  07D1 C  sysPW . . . . . . .  0056 C  sysPWinst . . . . .  0055 C    
TCTIME. . . . . . .  0028 A  UPI_C42_instructions 001F C  verPW . . . . . . .  005A C  
 
 ASSEMBLY COMPLETE,   NO ERRORS
```



















