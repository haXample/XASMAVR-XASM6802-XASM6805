// haXASM - Cross-Assembler for Atmel/Microchip 8bit processors
// opcodeAVR.cpp - C++ Developer source file.
// (c)2024 by helmut altmann
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; see the file COPYING.  If not, write to
// the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.

#include <io.h>        // File open, close, access, etc.
#include <conio.h>     // For _putch(), _getch() ..
#include <string>      // printf, etc.

#include <shlwapi.h>   // StrStrI, StrCmpI, StrCmpNI

#include <sys/stat.h>  // For filesize
#include <iostream>    // I/O control
#include <fstream>     // File control

#include <windows.h>   // For console specific functions

#include "equate.h"
#include "extern.h"    // Variables published in workst.cpp

using namespace std;

// Global variables
char* MODEL_str = ".MODEL";       // .MODEL BYTE/WORD/SYNTAX/NOINFO
char* TITLE_str = ".TITLE";
char* SUBTL_str = ".SUBTTL";
char* MSG_str   = ".MESSAGE";
char* ERROR_str = ".ERROR";
char* WARN_str  = ".WARNING";
char* PL_str    = ".PAGELENGTH(";                        
char* PW_str    = ".PAGEWIDTH(";                 
char* EJECT_str = ".EJECT";            
char* NOLST_str = ".NOLIST";
char* NOSYM_str = ".NOSYMBOLS";
char* SYM_str   = ".SYMBOLS";
char* LST_str   = ".LIST";
char* ICL_str   = ".INCLUDE";
char* DEVICE_str= ".DEVICE";
                                       // Preprocessor (C-style)
char* _icl_str        = "#include";    // same as .INCLUDE or <include>
char* _msg_str        = "#message";    // supported as .MESSAGE alias
char* _error_str      = "#error";      // supported as .ERROR alias
char* _warn_str       = "#warning";    // supported as .MESSAGE alias
char* _if_str         = "#if";         // supported as .IF alias    
char* _else_str       = "#else";       // supported as .ELSE alias  
char* _elif_str       = "#elif";       // supported as .ELIF alias  
char* _elseifdef_str  = "#elseifdef";  // supported as .ELSEIF alias  
char* _elseifndef_str = "#elseifndef"; // supported as .ELSEIF alias  
char* _elseif_str     = "#elseif";     // supported as .ELSEIF alias  
char* _ifdef_str      = "#ifdef";      // supported as .IFDEF alias 
char* _ifndef_str     = "#ifndef";     // supported as .IFNDEF alias
char* _endif_str      = "#endif";      // supported as .ENDIF alias
char* _defin_str      = "#define";     // supported as .EQU alias
char* _undef_str      = "#undef";
char* _pragm_str      = "#pragma";     // ,,see Atmel/Microchip specification

char* _if_def_str   = "#if_defined";   // alias for #ifdef  (see dcomp.cpp)
char* _if_ndef_str  = "#if_!defined";  // alias for #ifndef
char* _elif_def_str = "#elif_defined"; // alias for #elseifdef
char* _elif_ndef_str= "#elif_!defined";// alias for #elseifndef

char* IF_str       = ".IF";            // Assembler directives
char* ELSE_str     = ".ELSE";     
char* ELIF_str     = ".ELIF";     
char* ELSEIF_str   = ".ELSEIF";   
char* IFDEF_str    = ".IFDEF";      
char* IFNDEF_str   = ".IFNDEF";   
char* ENDIF_str    = ".ENDIF";      
char* DEFIN_str    = ".DEFINE";         
char* UNDEF_str    = ".UNDEF";         
char* LSTMAC_str   = ".LISTMAC";    
char* LSTMACR_str  = ".LISTMACRO";  
char* NOLSTMACR_str= ".NOLISTMACRO";  
char* NOLSTMAC_str = ".NOLISTMAC";  
char* MACRO_str    = ".MACRO";      
char* ENDM_str     = ".ENDM";     
char* ENDMACR_str  = ".ENDMACRO";  
char* CSEGSIZ_str  = ".CSEGSIZE";
char* OVLAP_str    = ".OVERLAP";    
char* NOOVLAP_str  = ".NOOVERLAP"; 
char* EVEN_str     = ".EVEN"; 

char* ORG_str = ".ORG";           // used in dcomp.cpp
char* DQ_str  = ".DQ";            // .DQ not supported, ignored
char* DD_str  = ".DD";
char* DW_str  = ".DW";
char* DB_str  = ".DB";   
char* DS_str  = ".BYTE";
char* END_str = ".EXIT";
char* EQU_str = ".EQU";
char* DEF_str = ".DEF";           // supported as .SET alias
char* SET_str = ".SET";

// Extern variables and functions
extern void DebugStop(int, char*, char*); // Usage: DebugStop(testNr, "Function()", __FILE__);
extern void DebugPrintBuffer(char*, int); // Usage: DebugPrintBuffer(buffer, count);

extern void AVRInstructionCheck();

//------------------------------------------------------------------------------
//
//                               HexFileFormat
//
int HexFileFormat()
  {
  _PCsymbol = _BUCK;
  return(INTEL);
  } // HexFileFormat

//------------------------------------------------------------------------------
//
// Atmel/Microchip AVR(R) Instruction Set
//
// Instruction Set Summary
//  Several updates of the AVR CPU during its lifetime has resulted in different
//  flavors of the instruction set, especially for the timing of the instructions. 
//  Machine code level of compatibility is intact for all CPU versions with very few
//  exceptions related to the Reduced Core (AVRrc), though not all instructions are 
//  included in the instruction set for all devices. The table below contains 
//  the major versions of the AVR 8-bit CPUs. In addition to the different versions,
//  there are differences depending on the size of the device memory map. 
//  Typically these differences are handled by a C/EC++ compiler, but users that are 
//  porting code should be aware that the code execution can vary slightly in the
//  number of clock cycles.
//  
//  Versions of AVR® 8-bit CPU
//  Name   Description
//  AVR    Original instruction set from 1995
//  
//  AVRe   AVR instruction set extended with the Move Word (MOVW) instruction, 
//         and the Load Program Memory (LPM) instruction has been enhanced. 
//         Same timing as AVR.
//  
//  AVRe+  AVRe instruction set extended with the Multiply (xMULxx) instruction. 
//         Same timing as AVR and AVRe.
//  
//  AVRxm  AVRe+ instruction set extended with the Read Modify Write (RMW) and 
//         Data Encryption Standard (DES) instructions. 
//         SPM extended to include SPM Z+2. Significantly different timing 
//         compared to AVR, AVRe, AVRe+.
//  
//  AVRxt  A combination of AVRe+ and AVRxm. Available instructions are the 
//         same as AVRe+, but the timing has been improved compared 
//         to AVR, AVRe, AVRe+ and AVRxm.
//  
//  AVRrc  AVRrc has only 16 registers in its register file (R31-R16), 
//         and the instruction set is reduced. The timing is significantly 
//         different compared to the AVR, AVRe, AVRe+, AVRxm and AVRxt. 
//         Refer to the instruction set summary for further details.
//
// © 2021 Microchip Technology Inc. Manual DS40002198B-page 149++
// Core Descriptions
//  The table lists all instructions that vary between the different cores
//  and marks if it is included in the core. If the instruction is not a
//  part of the table, then it is included in all cores.
//
//  Instructions   AVR   AVRe  AVRe+ AVRxm AVRxt AVRrc
//  --------------------------------------------------
//  ADIW         | x   | x   | x   | x   | x   |     |
//  BREAK        |     | x   | x   | x   | x   | x   |
//  CALL         |     | x   | x   | x   | x   |     |
//  DES          |     |     |     | x   |     |     |
//  EICALL       |     |     | x   | x   | x   |     |
//  EIJMP        |     |     | x   | x   | x   |     |
//  ELPM         |     |     | x   | x   | x   |     |
//  FMUL         |     |     | x   | x   | x   |     |
//  FMULS        |     |     | x   | x   | x   |     |
//  FMULSU       |     |     | x   | x   | x   |     |
//  JMP          |     | x   | x   | x   | x   |     |
//  LAC          |     |     |     | x   |     |     |
//  LAS          |     |     |     | x   |     |     |
//  LAT          |     |     |     | x   |     |     |
//  LDD          | x   | x   | x   | x   | x   |     |
//  LPM          | x   | x   | x   | x   | x   |     |
//  LPM Rd, Z    |     | x   | x   | x   | x   |     |
//  LPM Rd, Z+   |     | x   | x   | x   | x   |     |
//  MOVW         |     | x   | x   | x   | x   |     |
//  MUL          |     |     | x   | x   | x   |     |
//  MULS         |     |     | x   | x   | x   |     |
//  MULSU        |     |     | x   | x   | x   |     |
//  SBIW         | x   | x   | x   | x   | x   |     |
//  SPM          |     | x   | x   | x   | x   |     |
//  SPM Z+       |     |     |     | x   | x   |     |
//  STD          | x   | x   | x   | x   | x   |     |
//  XCH          |     |     |     | x   |     |     |
//  --------------------------------------------------

//------------------------------------------------------------------------------
//
// The AVR® Enhanced RISC microcontroller supports powerful and efficient
// addressing modes for access to the program memory (Flash) and
// Data memory (SRAM, Register file, I/O Memory, and Extended I/O Memory).
// The addressing modes define the manner in which the CPU finds the
// data required to execute an instruction. The addressing modes are:
//
//  Register Direct, Single Register Rd
//  Register Direct - Two Registers, Rd and Rr
//  I/O Direct
//  Data Direct
//  Data Indirect
//  Data Indirect with Pre-decrement
//  Data Indirect with Post-increment
//  Data Indirect with Displacement
//  Program Memory Constant Addressing using the LPM, ELPM, and SPM Instructions
//  Program Memory with Post-increment using the LPM Z+ and ELPM Z+ Instruction
//  Store Program Memory Post-increment
//  Direct Program Addressing, JMP and CALL
//  Indirect Program Addressing, IJMP and ICALL
//  Extended Indirect Program Addressing, EIJMP and EICALL
//  Relative Program Addressing, RJMP and RCALL
//
// each entry is organized as described below:
//   1: mnemonic string (nul-terminated),
//   2: group,
//   3: basic value (16bit)
//
// typedef struct tagINSTRAVR {  // CPU instruction structure
//   char*  mneStr;              // Instruction Mnemonic
//   UCHAR  mneGrp;              // Instruction group
//   UINT   mneVal16;            // Instruction+Operand default value (16bit)
// } INSTRAVR, *LPINSTRAVR;
//
// ----------------------------
// Instruction Set Nomenclature
// ----------------------------
//  Status Register (SREG)
//   SREG Status Register
//   C Carry Flag
//   Z Zero Flag
//   N Negative Flag
//   V Two’s Complement Overflow Flag
//   S Sign Flag
//   H Half Carry Flag
//   T Transfer Bit
//   I Global Interrupt Enable Bit
//
//  Registers and Operands
//   Rd: Destination (and source) register in the Register File
//   Rr: Source register in the Register File
//   R: Result after instruction is executed
//   K: Constant data
//   k: Constant address
//   b: Bit position (0..7) in the Register File or I/O Register
//   s: Bit position (0..7) in the Status Register
//   X,Y,Z: Indirect Address Register
//    X=R27:R26
//    Y=R29:R28
//    Z=R31:R30
//  if the memory is larger than 64 KB: 
//    X=RAMPX:R27:R26
//    Y=RAMPY:R29:R28
//    Z=RAMPZ:R31:R30 
//   A: I/O memory address
//   q: Displacement for direct addressing
//   UU Unsigned × Unsigned operands
//   SS Signed × Signed operands
//   SU Signed × Unsigned operands                       
//
//   Operator
//   × Arithmetic multiplication
//
//   Note:
//   * This instruction is not available in all devices.
//     Refer to the device specific instruction set summary.
// 
INSTR16 instring[] = {
  // Group 1: Pseudo Instructions and Directives
  {ORG_str,   1,0x0202},  // 02
  {MODEL_str, 1,0x0303},  // 03 .MODEL BYTE/WORD/SYNTAX (for Atmel AVR)
  {".CSEG",   1,0x0404},  // 04 Code Segment  
  {".DSEG",   1,0x0606},  // 06 Data Segment 
  {".ESEG",   1,0x0808},  // 08 EEPROM Segment
  {DW_str,    1,0x0A0A},  // 10  
  {DB_str,    1,0x0C0C},  // 12                               
  {DS_str,    1,0x0E0E},  // 14  
  {END_str,   1,0x1010},  // 16
  {EQU_str,   1,0x1212},  // 18
  {_defin_str,1,0x1212},  // 18 Alias: #define = .EQU
  {DEFIN_str, 1,0x1212},  // 18 Alias: .DEFINE = .EQU
  {"EXTRN",   1,0x1414},  // 20 No relocatable object format yet
  {"PUBLC",   1,0x1616},  // 22 No relocatable object format yet
  {TITLE_str, 1,0x1818},  // 24
  {SUBTL_str, 1,0x1A1A},  // 26
  {MSG_str,   1,0x1B1B},  // 27
  {_msg_str,  1,0x1B1B},  // 27 Also handled in .MESSAGE
  {ERROR_str, 1,0x1B1B},  // 27 Also handled in .MESSAGE
  {_error_str,1,0x1B1B},  // 27 Also handled in .MESSAGE
  {WARN_str,  1,0x1B1B},  // 27 Also handled in .MESSAGE
  {_warn_str, 1,0x1B1B},  // 27 Also handled in .MESSAGE
  {PW_str,    1,0x1C1C},  // 28
  {PL_str,    1,0x1E1E},  // 30
  {ICL_str,   1,0x2020},  // 32
  {_icl_str,  1,0x2020},  // 32 Alias: #include = .INCLUDE = <include>
  {NOLST_str, 1,0x2222},  // 34
  {LST_str,   1,0x2424},  // 36
  {NOSYM_str, 1,0x2626},  // 38
  {SYM_str,   1,0x2626},  // 38 Also handled in .NOSYMBOLS
  {EJECT_str, 1,0x2828},  // 40
  {DEVICE_str,1,0x2929},  // 41 Not Implemented (always Full Atmel instruction set)
  {DD_str,    1,0x2A2A},  // 42
  {SET_str,   1,0x2B2B},  // 43
  {DEF_str,   1,0x2B2B},  // 43 Alias: .DEF = .SET
  
  //{ ,  1,0x2C2C},       // 44  Reserved
  
  {UNDEF_str,  1,0x2D2D}, // 45 Undefine .DEF'ed regs (silently ignoerd, not needed)
  {_undef_str, 1,0x2E2E}, // 46 undefine #define'd symbols
  {DQ_str,     1,0x2F2F}, // 47
  {CSEGSIZ_str,1,0x3030}, // 48
  {EVEN_str,   1,0x3131}, // 49                               //ha//
  
  //{ ,  1,0x3232},       // 50  Reserved

  {LSTMAC_str,     1,0x3333}, // 51
  {LSTMACR_str,    1,0x3333}, // 51
  {NOLSTMACR_str,  1,0x3333}, // 51 Also handled in .LISTMAC
  {NOLSTMAC_str,   1,0x3333}, // 51 Also handled in .LISTMAC
  {MACRO_str,      1,0x3434}, // 52
  {ENDM_str,       1,0x3535}, // 53
  {ENDMACR_str,    1,0x3535}, // 53
  {_pragm_str,     1,0x3636}, // 54
                              
  {OVLAP_str,      1,0x3737}, // 55
  {NOOVLAP_str,    1,0x3838}, // 56
            
  {IF_str,         1,0x3939}, // 57 Assembler directives      //ha//
  {ELSE_str,       1,0x3A3A}, // 58 (same as #if/#ifdef..)    //ha//
  {ELIF_str,       1,0x3B3B}, // 59                           //ha//
  {ELSEIF_str,     1,0x3B3B}, // 59                           //ha//
  {IFDEF_str,      1,0x3C3C}, // 60                           //ha//
  {IFNDEF_str,     1,0x3D3D}, // 61                           //ha//
  {ENDIF_str,      1,0x3E3E}, // 62                           //ha//

  {_if_str,        1,0x3939}, // 57 Pre-processor (C-style)
  {_else_str,      1,0x3A3A}, // 58
  {_elif_str,      1,0x3B3B}, // 59 
  {_elseif_str,    1,0x3B3B}, // 59 // Alias =elif
  {_elseifdef_str, 1,0x3B3B}, // 59 // Alias
  {_elseifndef_str,1,0x3B3B}, // 59 // Alias
  {_ifdef_str,     1,0x3C3C}, // 60
  {_ifndef_str,    1,0x3D3D}, // 61
  {_endif_str,     1,0x3E3E}, // 62
  
  {_if_def_str,    1,0x3C3C}, // 60 Alias = "#if defined"   = #ifdef  (see dcomp.cpp)
  {_if_ndef_str,   1,0x3D3D}, // 61 Alias = "#if !defined"  = #ifndef
  {_elif_def_str,  1,0x3B3B}, // 61 Alias = "#elif defined" = #elseifdef
  {_elif_ndef_str, 1,0x3B3B}, // 61 Alias = "#elif !defined"= #elseifndef

  //{ ,  1,0x4545},           // 63  Reserved
  //{ ,  1,0x4646},           // 64  Reserved
  //{ ,  1,0x4747},           // 65  Reserved

  // Group 2:
  // no operands
  {"NOP",   2,0x0000},   //00000000 00000000  // nopAlign=0x00 for .ALIGN / .EVEN directives
  {"SEC",   2,0x9408},   //10010100 00001000
  {"IJMP",  2,0x9409},   //10010100 00001001*
  {"SEZ",   2,0x9418},   //10010100 00011000
  {"EIJMP", 2,0x9419},   //10010100 00011001*
  {"SEN",   2,0x9428},   //10010100 00101000
  {"SEV",   2,0x9438},   //10010100 00111000
  {"SES",   2,0x9448},   //10010100 01001000                      
  {"SEH",   2,0x9458},   //10010100 01011000
  {"SET",   2,0x9468},   //10010100 01101000
  {"SEI",   2,0x9478},   //10010100 01111000
  {"CLC",   2,0x9488},   //10010100 10001000
  {"CLZ",   2,0x9498},   //10010100 10101000
  {"CLN",   2,0x94A8},   //10010100 10101000
  {"CLV",   2,0x94B8},   //10010100 10111000
  {"CLS",   2,0x94C8},   //10010100 11001000
  {"CLH",   2,0x94D8},   //10010100 11011000
  {"CLT",   2,0x94E8},   //10010100 11101000
  {"CLI",   2,0x94F8},   //10010100 11111000
  {"RET",   2,0x9508},   //10010101 00001000
  {"ICALL", 2,0x9509},   //10010101 00001001*  (*see note)
  {"RETI",  2,0x9518},   //10010101 00011000
  {"EICALL",2,0x9519},   //10010101 00011001*
  {"SLEEP", 2,0x9588},   //10010101 10001000
  {"BREAK", 2,0x9598},   //10010101 10011000*
  {"WDR",   2,0x95A8},   //10010101 10101000
  {"SPM",   2,0x95E8},   //10010101 11101000*  (i) SPM - Store Program Memory
                         //10010101 11111000   (ii)

  // Group 3:
  {"BRCS",3,0xF000},   //111100kk kkkkk000 
  {"BRLO",3,0xF000},   //111100kk kkkkk000  =BRCS / BRBS 0,k
  {"BREQ",3,0xF001},   //111100kk kkkkk001 
  {"BRMI",3,0xF002},   //111100kk kkkkk010 
  {"BRVS",3,0xF003},   //111100kk kkkkk011 
  {"BRLT",3,0xF004},   //111100kk kkkkk100 
  {"BRHS",3,0xF005},   //111100kk kkkkk101 
  {"BRTS",3,0xF006},   //111100kk kkkkk110 
  {"BRIE",3,0xF007},   //111100kk kkkkk111 
  {"BRCC",3,0xF400},   //111101kk kkkkk000  
  {"BRSH",3,0xF400},   //111101kk kkkkk000  =BRCC / BRBC 0,k
  {"BRNE",3,0xF401},   //111101kk kkkkk001 
  {"BRPL",3,0xF402},   //111101kk kkkkk010 
  {"BRVC",3,0xF403},   //111101kk kkkkk011 
  {"BRGE",3,0xF404},   //111101kk kkkkk100 
  {"BRHC",3,0xF405},   //111101kk kkkkk101 
  {"BRTC",3,0xF406},   //111101kk kkkkk110 
  {"BRID",3,0xF407},   //111101kk kkkkk111 

  // Group 4:
  {"CPC", 4,0x0400},   //000001rd ddddrrrr
  {"SBC", 4,0x0800},   //000010rd ddddrrrr
  {"ADD", 4,0x0C00},   //000011rd ddddrrrr
  {"CPSE",4,0x1000},   //000100rd ddddrrrr
  {"CP",  4,0x1400},   //000101rd ddddrrrr
  {"SUB", 4,0x1800},   //000110rd ddddrrrr
  {"ADC", 4,0x1C00},   //000111rd ddddrrrr
  {"AND", 4,0x2000},   //001000rd ddddrrrr
  {"EOR", 4,0x2400},   //001001rd ddddrrrr
  {"OR",  4,0x2800},   //001010rd ddddrrrr
  {"MOV", 4,0x2C00},   //001011rd ddddrrrr
  {"MUL", 4,0x9C00},   //100111rd ddddrrrr*

  // Group 5:
  {"CPI", 5,0x3000},   //0011KKKK ddddKKKK                     
  {"SBCI",5,0x4000},   //0100KKKK ddddKKKK
  {"SUBI",5,0x5000},   //0101KKKK ddddKKKK
  {"ORI", 5,0x6000},   //0110KKKK ddddKKKK
  {"SBR", 5,0x6000},   //0110KKKK ddddKKKK  =ORI
  {"ANDI",5,0x7000},   //0111KKKK ddddKKKK
  {"CBR", 5,0x7000},   //0111KKKK ddddKKKK  =ANDI Rd,0xFF-K (-->see ANDI ~K)
  {"LDI", 5,0xE000},   //1110KKKK ddddKKKK 
   
  // Group 6:
  {"COM", 6,0x9400},   //1001010d dddd0000
  {"NEG", 6,0x9401},   //1001010d dddd0001
  {"SWAP",6,0x9402},   //1001010d dddd0010
  {"INC", 6,0x9403},   //1001010d dddd0011
  {"ASR", 6,0x9405},   //1001010d dddd0101
  {"LSR", 6,0x9406},   //1001010d dddd0110
  {"ROR", 6,0x9407},   //1001010d dddd0111
  {"DEC", 6,0x940A},   //1001010d dddd1010

  // Group 7:
  {"CBI", 7,0x9800},   //10011000 AAAAAbbb
  {"SBIC",7,0x9900},   //10011001 AAAAAbbb
  {"SBI", 7,0x9A00},   //10011010 AAAAAbbb
  {"SBIS",7,0x9B00},   //10011011 AAAAAbbb

  // Group 8:
  {"MULSU", 8,0x0300}, //00000011 0ddd0rrr/
  {"FMUL",  8,0x0308}, //00000011 0ddd1rrr*
  {"FMULS", 8,0x0380}, //00000011 1ddd0rrr*
  {"FMULSU",8,0x0388}, //00000011 1ddd1rrr*

  // Group 9:
  {"LSL",9,0x0C00},    //000011dd dddddddd
  {"ROL",9,0x1C00},    //000111dd dddddddd
  {"TST",9,0x2000},    //001000dd dddddddd
  {"CLR",9,0x2400},    //001001dd dddddddd

  // Group 10:
  {"XCH", 10,0x9204},  //1001001r rrrr0100
  {"LAS", 10,0x9205},  //1001001r rrrr0101
  {"LAC", 10,0x9206},  //1001001r rrrr0110
  {"LAT", 10,0x9207},  //1001001r rrrr0111

  // Group 11:
  {"POP", 11,0x900F},  //1001000d dddd1111*
  {"PUSH",11,0x920F},  //1001001r rrrr1111*

  // Group 12:
  {"ADIW",12,0x9600},  //10010110 KKddKKKK*  
  {"SBIW",12,0x9600},  //10010111 KKddKKKK*

  // Group 13:
  {"LDS",13,0x9000},   //1001000d dddd0000 kkkkkkkk kkkkkkkk*

  // Group 14:
  {"RJMP", 14,0xC000}, //1100kkkk kkkkkkkk
  {"RCALL",14,0xD000}, //1101kkkk kkkkkkkk

  // Group 15:
  {"BRBS",15,0xF000},  //111100kk kkkkksss
  {"BRBC",15,0xF400},  //111101kk kkkkksss

  // Group 16:
  {"BLD", 16,0xF800},  //1111100d dddd0bbb
  {"BST", 16,0xFA00},  //1111101d dddd0bbb
  {"SBRC",16,0xFC00},  //1111110r rrrr0bbb
  {"SBRS",16,0xFE00},  //1111111r rrrr0bbb

  // Group 17:

  // Group 18:
  {"BSET",18,0x9408},  //10010100 0sss1000
  {"BCLR",18,0x9488},  //10010100 1sss1000

  // Group 19:
  {"SER", 19,0xEF0F},  //11101111 dddd1111

  // Group 20:
  {"MULS",20,0x0200},  //00000010 ddddrrrr*

  // Group 21:
  {"DES",21,0x940B},   //10010100 KKKK1011

  // Group 22:
  {"CALL",22,0x940E},  //1001010k kkkk111k kkkkkkkk kkkkkkkk*
  {"JMP", 22,0x940C},  //1001010k kkkk110k kkkkkkkk kkkkkkkk*

  // Group 23:         //reserved

  // Group 24:
  {"MOVW",24,0x0100},  //00000001 ddddrrrr*

  // Group 25
  {"STS",25,0x9200},   //1001001r rrrr0000 kkkkkkkk kkkkkkkk*

  // Group 26:
  {"IN", 26,0xB000},   //10110AAd ddddAAAA

  // Group 27:
  {"OUT",27,0xB800},   //10111AAr rrrrAAAA

  // Group 28:
  {"LD", 28,0x8000},   //1001000d dddd1100   (i)     (LD Rd,X)
                       //1001000d dddd1101*  (ii)    (LD Rd,X+)
                       //1001000d dddd1110*  (iii)   (LD Rd,-X)

                       //1000000d dddd1000   (i)     (LD Rd,Y)
                       //1001000d dddd1001*  (ii)    (LD Rd,Y+)
                       //1001000d dddd1010*  (iii)   (LD Rd,-Y)

                       //1000000d dddd0000   (i)     (LD Rd,Z)
                       //1001000d dddd0001*  (ii)    (LD Rd,Z+)
                       //1001000d dddd0010*  (iii)   (LD Rd,-Z)
  
  // Group 29:
  {"LDD",29,0x8000},   //10q0qq0d dddd1qqq*  (iv)    (LDD Rd+q,Y)
                       //10q0qq0d dddd0qqq*  (iv)    (LDD Rd+q,Z)

  // Group 30:
  {"ST",30,0x8200},    //1001001r rrrr1100   (i)     (ST X,Rr)
                       //1001001r rrrr1101*  (ii)    (ST X+,Rr)
                       //1001001r rrrr1110*  (iii)   (ST -X,Rr)

                       //1000001r rrrr1000   (i)     (ST Y,Rr)
                       //1001001r rrrr1001*  (ii)    (ST Y+,Rr)
                       //1001001r rrrr1010*  (iii)   (ST -Y,Rr)

                       //1000001r rrrr0000   (i)     (ST Z,Rr)
                       //1001001r rrrr0001*  (ii)    (ST Z+,Rr)
                       //1001001r rrrr0010*  (iii)   (ST -Z,Rr)

  // Group 31:
  {"STD",31,0x8200},   //10q0qq1r rrrr1qqq*  (iv)    (STD Y+q,Rr)
                       //10q0qq1r rrrr0qqq*  (iv)    (STD Z+q,Rr)

  // Group 32:
  // LPM -  Load Program Memory
  // ELPM - Extended Load Program Memory (*see note)
  {"LPM",32,0x9000},   //10010101 11001000*  (i)     (no operands)
                       //1001000d dddd0100*  (ii)    (LPM Rd,Z)
                       //1001000d dddd0101*  (iii)   (LPM Rd,Z+)
  {"ELPM",32,0x9000},  //10010101 11011000*  (i)     (no operands)
                       //1001000d dddd0110*  (ii)    (ELPM Rd,Z)
                       //1001000d dddd0111*  (iii)   (ELPM Rd,Z+)

  // Group END:
  {"0",99,0x0000}
  };                   // end of instruction table

//-----------------------------------------------------------------------------
//
//                      opcod
//
// Check opcode against table.
// Return:  opcode-group,  INSV[1], INSV[2]= 16bit instruction value
// If opcode is not in the table, return zero.
// If opcode field is empty, return ins_group.
//
int opcod()
  {
  ins_group = ERR;     // Assume illegal opcode (set general error)

  if (fopcd[0] == 0)   // Empty?
    {
    ins_group = 0;     // Clear group
    insv[1] = 0;       // Instring VALUE = 0
    insv[2] = 0;       // Instring VALUE = 0
    } // end if
                                                     
  else
    {
    for (_i=0; _i<sizeof(instring)/sizeof(INSTR16); _i++)
      {
      if (StrCmpI(fopcd, instring[_i].mneStr) == 0 ||
          (strstr(fopcd, "(") != 0 && 
           StrCmpNI(fopcd, instring[_i].mneStr, strlen(instring[_i].mneStr)) == 0)
         )
        {
        // Basic intruction value
        insv[2] = (UCHAR)(instring[_i].mneVal16 >> 8);       
        insv[1] = (UCHAR)(instring[_i].mneVal16);       
        // Instruction group
        ins_group = instring[_i].mneGrp;   
        break;                 // Opcode found
        } // end if
      } // end for(_i)
    } // end else

  if (swpass == _PASS2) AVRInstructionCheck();
  return(ins_group);
  } // opcod

//-------------------------------------------------------------------------------
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//if (devicePtr > NULL)
//ha//{
//ha//printf("devicePtr=%08X  fopcd=%s  devicePtr->missingInst=%04X  _instID=%04X",
//ha//        devicePtr,      fopcd,    devicePtr->missingInst,      _instID);
//ha////ha//printf("missInstrBuf ");
//ha////ha//DebugPrintBuffer(missInstrBuf, OPERLEN);
//ha//DebugStop(1, "StoreMissingInstr()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("fopcd=[%s]  strstr(fopcd, \x22(\x22)=%d\n instring[_i].mneStr=[%s]  ins_group=%d  insv[1]=%d [%02X]\n",
//ha//        fopcd,      strstr(fopcd, "("),           instring[_i].mneStr,      ins_group,    insv[1],    insv[1]);
//ha//printf("StrCmpI(fopcd, instring[_i].mneStr)=%d\n", 
//ha//        StrCmpI(fopcd, instring[_i].mneStr) == 0);
//ha//printf("(char)strstr(fopcd, \x22(\x22)=%d\n", 
//ha//        (char)strstr(fopcd, "(") != 0);
//ha//printf("StrCmpNI(fopcd, instring[_i].mneStr, strlen(instring[_i].mneStr)=%d\n", 
//ha//        StrCmpNI(fopcd, instring[_i].mneStr, strlen(instring[_i].mneStr)) == 0);
//ha//printf("result1=%d\n",
//ha//        ((char)strstr(fopcd, "(") == 0 & StrCmpNI(fopcd, instring[_i].mneStr, strlen(instring[_i].mneStr)) == 0));
//ha//printf("result=%d\n",
//ha//        StrCmpI(fopcd, instring[_i].mneStr) == 0 | ((char)strstr(fopcd, "(") == 0 & StrCmpNI(fopcd, instring[_i].mneStr, strlen(instring[_i].mneStr)) == 0));
//ha//DebugStop(1, "opcod()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("fopcd=%s\n instring[_i].mneStr=[%s]  ins_group=%d  insv[1]=%d [%02X]\n",
//ha//        fopcd,    instring[_i].mneStr,    ins_group,    insv[1],    insv[1]);
//ha//DebugStop(1, "opcod()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//--------------------------end-of-c++-module-----------------------------------

