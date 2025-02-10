// haXASM - Cross-Assembler for Motorola 8bit processors
// opcode6802.cpp - C++ Developer source file.
// (c)2023 by helmut altmann
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
char* TITLE_str = ".TITLE";
char* SUBTL_str = ".SUBTTL";
char* MSG_str   = ".MESSAGE";
char* ERROR_str = ".ERROR";
char* WARN_str  = ".WARNING";
char* PW_str    = ".PAGEWIDTH(";
char* PL_str    = ".PAGELENGTH(";
char* NOLST_str = ".NOLIST";
char* LST_str   = ".LIST";
char* NOSYM_str = ".NOSYMBOLS";
char* SYM_str   = ".SYMBOLS";
char* ICL_str   = ".INCLUDE";
char* EJECT_str = ".EJECT";

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
char* LSTMAC_str   = ".LISTMAC";    
char* LSTMACR_str  = ".LISTMACRO";  
char* NOLSTMACR_str= ".NOLISTMACRO";  
char* NOLSTMAC_str = ".NOLISTMAC";  
char* MACRO_str    = ".MACRO";      
char* ENDM_str     = ".ENDM";     
char* ENDMACR_str  = ".ENDMACRO";  
char* DEFIN_str    = ".DEFINE";       // supported as .EQU alias
char* UNDEF_str    = ".UNDEF";         
char* EVEN_str     = ".EVEN";
                                                                         
char* ORG_str = "ORG";  // used in dcomp.cpp
char* DD_str  = "FDW";  // Form Double Word Constant
char* DW_str  = "FDB";  // Form Double Byte Constant
char* DB_str  = "FCB";  // Form Constant Byte
char* DS_str  = "RMB";  // Reserve Memory Block
char* END_str = "END";
char* EQU_str = "EQU";
char* SET_str = "SET";

char* DQ_str    = TITLE_str; // Dummy
char* DEF_str   = EQU_str;   // Dummy
char* MODEL_str = ORG_str;   // Dummy
char* DEVICE_str= ORG_str;   // Dummy

char* CSEGSIZ_str = EQU_str; // Dummy

// Extern variables and functions
extern void DebugStop(int, char*, char*); // Usage: DebugStop(testNr, "Function()", __FILE__);
extern void DebugPrintBuffer(char*, int); // Usage: DebugPrintBuffer(buffer, count);

//------------------------------------------------------------------------------
//
//                               HexFileFormat
//
int HexFileFormat()
  {
  _PCsymbol = _STAR;
  return(MOTOROLA);
  } // HexFileFormat

//------------------------------------------------------------------------------
//  6802  INSTRUCTION SET 
//
//  A       Accumulator
//  M       Memory location
//  op      Operand (one, two or three bytes)
//  X       Index register
//  PC      Program counter
//
// each entry is organized as described below:
//   1: mnemonic string (nul-terminated),
//   2: group,
//   3: basic value (8bit)
//
// typedef struct tag_6802INS {
//   char*  mneStr;
//   UCHAR  mneGrp;
//   UCHAR  mneVal;
//} _6802INS, *LP_6802INS;
//
INSTR instring[] = {
  // Group 1: Pseudo Instructions
  {ORG_str,   1,0x02},  // 02
  {"CSEG",    1,0x04},  // 04 No relocatable object format yet
  {"DSEG",    1,0x06},  // 06 No relocatable object format yet
//{"",        1 0x08},  // 08 Reserved
  {DW_str,    1,0x0A},  // 10  
  {DB_str,    1,0x0C},  // 12  
  {DS_str,    1,0x0E},  // 14  
  {END_str,   1,0x10},  // 16
  {EQU_str,   1,0x12},  // 18
  {_defin_str,1,0x12},  // 18 Alias: #define = .EQU
  {DEFIN_str, 1,0x12},  // 18 Alias: .DEFINE = .EQU
  {"EXTRN",   1,0x14},  // 20 No relocatable object format yet
  {"PUBLC",   1,0x16},  // 22 No relocatable object format yet
  {TITLE_str, 1,0x18},  // 24
  {SUBTL_str, 1,0x1A},  // 26
  {MSG_str,   1,0x1B},  // 27
  {_msg_str,  1,0x1B},  // 27 Also handled in .MESSAGE
  {ERROR_str, 1,0x1B},  // 27 Also handled in .MESSAGE
  {_error_str,1,0x1B},  // 27 Also handled in .MESSAGE
  {WARN_str,  1,0x1B},  // 27 Also handled in .MESSAGE
  {_warn_str, 1,0x1B},  // 27 Also handled in .MESSAGE
  {PW_str,    1,0x1C},  // 28
  {PL_str,    1,0x1E},  // 30
  {ICL_str,   1,0x20},  // 32
  {NOLST_str, 1,0x22},  // 34
  {LST_str,   1,0x24},  // 36
  {NOSYM_str, 1,0x26},  // 38
  {SYM_str,   1,0x26},  // 38 Also handled in .NOSYMBOLS
  {EJECT_str, 1,0x28},  // 40
  
  {"",        1,0x29},  // 44 Ignored

  {DD_str,    1,0x2A},  // 42
  {SET_str,   1,0x2B},  // 43
  
  {"",        1,0x2C},  // 44 Ignored
  
  {UNDEF_str, 1,0x2D},  // 45 Undefine .DEF'ed regs (silently ignoerd, not needed)
  {_undef_str,1,0x2E},  // 46 undefine #define'd symbols
  {DQ_str,    1,0x2F},  // 47

  {"",        1,0x30},  // 48 Ignored
  {EVEN_str,  1,0x31},  // 49                            //ha//

  //{ ,       1,0x32},  // 50  Reserved

  {LSTMAC_str,   1,0x33}, // 51
  {LSTMACR_str,  1,0x33}, // 51
  {NOLSTMACR_str,1,0x33}, // 51 Also handled in .LISTMAC
  {NOLSTMAC_str, 1,0x33}, // 51 Also handled in .LISTMAC
  {MACRO_str,    1,0x34}, // 52
  {ENDM_str,     1,0x35}, // 53
  {ENDMACR_str,  1,0x35}, // 53
                            
  {IF_str,       1,0x39}, // 57 Assembler directives      //ha//
  {ELSE_str,     1,0x3A}, // 58 (same as #if/#ifdef..)    //ha//
  {ELIF_str,     1,0x3B}, // 59                           //ha//
  {ELSEIF_str,   1,0x3B}, // 59                           //ha//
  {IFDEF_str,    1,0x3C}, // 60                           //ha//
  {IFNDEF_str,   1,0x3D}, // 61                           //ha//
  {ENDIF_str,    1,0x3E}, // 62                           //ha//
  {UNDEF_str,    1,0x2C}, // 63 Ignored

  {_if_str,        1,0x39}, // 57 Pre-processor (C-style)
  {_else_str,      1,0x3A}, // 58
  {_elif_str,      1,0x3B}, // 59 
  {_elseif_str,    1,0x3B}, // 59 // Alias =elif
  {_elseifdef_str, 1,0x3B}, // 59 // Alias
  {_elseifndef_str,1,0x3B}, // 59 // Alias
  {_ifdef_str,     1,0x3C}, // 60
  {_ifndef_str,    1,0x3D}, // 61
  {_endif_str,     1,0x3E}, // 62
  {_undef_str,     1,0x2C}, // 63 Ignored
  
  {_if_def_str,    1,0x3C}, // 60 Alias = "#if defined"   = #ifdef  (see dcomp.cpp)
  {_if_ndef_str,   1,0x3D}, // 61 Alias = "#if !defined"  = #ifndef
  {_elif_def_str,  1,0x3B}, // 61 Alias = "#elif defined" = #elseifdef
  {_elif_ndef_str, 1,0x3B}, // 61 Alias = "#elif !defined"= #elseifndef

//{"",  1,0x4545}, // 63  Reserved
//{"",  1,0x4646}, // 64  Reserved
//{"",  1,0x4747}, // 65  Reserved

  // Group 2: 1-Byte-Instructions, no operands
  // Addressing Mode: IMPLIED
  {"ABA",2,0x1B},
  {"CBA",2,0x11},
  {"CLC",2,0x0C},
  {"CLI",2,0x0E},
  {"CLV",2,0x0A},
  {"DAA",2,0x19},
  {"DES",2,0x34},
  {"DEX",2,0x09},
  {"INS",2,0x31},
  {"INX",2,0x08},
  {"NOP",2,0x01},    // nopAlign=0x01 for .ALIGN / .EVEN directives
  {"RTI",2,0x3B},
  {"RTS",2,0x39},
  {"SBA",2,0x10},
  {"SEC",2,0x0D},
  {"SEI",2,0x0F},
  {"SEV",2,0x0B},
  {"SWI",2,0x3F},
  {"TAB",2,0x16},
  {"TAP",2,0x06},
  {"TBA",2,0x17},
  {"TPA",2,0x07},
  {"TSX",2,0x30},
  {"TXS",2,0x35},
  {"WAI",2,0x3E},
  {"PSHA",2,0x36},
  {"PSHB",2,0x37},
  {"PULA",2,0x32},
  {"PULB",2,0x33},
  {"ASLA",2,0x48},
  {"ASLB",2,0x58},
  {"ASRA",2,0x47},
  {"ASRB",2,0x57},
  {"CLRA",2,0x4F},
  {"CLRB",2,0x5F},
  {"DECA",2,0x4A},
  {"DECB",2,0x5A},
  {"COMA",2,0x43},
  {"COMB",2,0x53},
  {"INCA",2,0x4C},
  {"INCB",2,0x5C},
  {"LSRA",2,0x44},
  {"LSRB",2,0x54},
  {"NEGA",2,0x40},
  {"NEGB",2,0x50},
  {"ROLA",2,0x49},
  {"ROLB",2,0x59},
  {"RORA",2,0x46},
  {"RORB",2,0x56},
  {"TSTA",2,0x4D},
  {"TSTB",2,0x5D},

  // Group 3: 2-Byte-Instructions, op1 = {aa} <= 0xFF,
  // Addressing Mode: RELATIVE
  {"BCC",3,0x24},
  {"BCS",3,0x25},
  {"BEQ",3,0x27},
  {"BGE",3,0x2C},
  {"BGT",3,0x2E},
  {"BHI",3,0x22},
  {"BLE",3,0x2F},
  {"BLS",3,0x23},
  {"BLT",3,0x2D},
  {"BMI",3,0x2B},
  {"BNE",3,0x26},
  {"BPL",3,0x2A},
  {"BRA",3,0x20},
  {"BSR",3,0x8D},
  {"BVC",3,0x28},
  {"BVS",3,0x29},

  // Group 4: 1, 2or3-Byte-Instr., op1 = { }, [op2 = { }]
  // Addressing Modes: IMMEDIATE, DIRECT, INDEXED, EXTENDED
  {"ADCA",4,0x89},
  {"ADDA",4,0x8B},
  {"ANDA",4,0x84},
  {"BITA",4,0x85},  
  {"CMPA",4,0x81},
  {"EORA",4,0x88},
  {"LDAA",4,0x86},
  {"ORAA",4,0x8A},
  {"SBCA",4,0x82},
  {"SUBA",4,0x80},
  {"ADCB",4,0xC9},
  {"ADDB",4,0xCB},
  {"ANDB",4,0xC4},
  {"BITB",4,0xC5},
  {"CMPB",4,0xC1},
  {"EORB",4,0xC8},
  {"LDAB",4,0xC6},
  {"ORAB",4,0xCA},
  {"SBCB",4,0xC2},
  {"SUBB",4,0xC0},
  {"STAB",(4 | 0x80),0xC7}, // no # immediate-mode
  {"STAA",(4 | 0x80),0x87}, // no # immediate-mode

  // Group 5: 1, 2or3-Byte-Instr., op1 = { }, [op2 = { }]
  // Addressing Modes: IMMEDIATE, DIRECT, INDEXED, EXTENDED
  {"CPX",5,0x8C},
  {"LDS",5,0x8E},
  {"LDX",5,0xCE},
  {"STS",(5 | 0x80),0x8F},  // no # immediate-mode
  {"STX",(5 | 0x80),0xCF},  // no # immediate-mode

  // Group 6: 2or3-Byte-Instr., op1 = { }, op2 = { }
  // Addressing Modes: INDEXED, EXTENDED
  {"ASL",6,0x68},
  {"ASR",6,0x67},
  {"CLR",6,0x6F},
  {"COM",6,0x63},
  {"DEC",6,0x6A},
  {"INC",6,0x6C},
  {"LSR",6,0x64},
  {"NEG",6,0x60},
  {"ROL",6,0x69},
  {"ROR",6,0x66},
  {"TST",6,0x6D},

  // Group 7: 2or3-Byte-Instr., op1 = { }, op2 = { }
  // Addressing Modes: INDEXED, EXTENDED
  {"JMP",7,0x6E},
  {"JSR",7,0xAD},

  // Group END:
  {"0", 99,0x00}
  };                     // end of instruction table

//-----------------------------------------------------------------------------
//
//                      opcod
//
// Check opcode against table.
// Return:  opcode-group,  INSV[1]= instruction value
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
    } // end if
                                                     
  else
    {
    for (_i=0; _i<sizeof(instring)/sizeof(INSTR); _i++)
      {
      if (StrCmpI(fopcd, instring[_i].mneStr) == 0 ||
          (strstr(fopcd, "(") != 0 && StrCmpNI(fopcd, instring[_i].mneStr, strlen(instring[_i].mneStr)) == 0))
        {
        // Basic intruction value
        insv[1] = instring[_i].mneVal;       
        // Instruction group
        ins_group = instring[_i].mneGrp;   
        break;         // Opcode found
        } // end if
      } // end for(_i)
    } // end else

  return(ins_group);
  } // opcod

//-------------------------------------------------------------------------------

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("fopcd=%s, instring[%d].mneStr=%s, insv[1]=%02X, ins_group=%02X\n",
//ha//        fopcd,   _i, instring[_i].mneStr, insv[1],      ins_group);
//ha//DebugStop(1, "opcod()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//--------------------------end-of-c++-module-----------------------------------

