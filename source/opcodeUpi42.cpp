// haXASM - Cross-Assembler for INtel 8bit processors
// opcodeUpi42.cpp - C++ Developer source file.
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

//#pragma pack           // Needed for INSTR structure 'sizeof(..)'

using namespace std;

// Global variables
char* TITLEstr = "$TITLE";       // "$..." Compatibility w/ old SIEMENS KBC source           
char* SUBTLstr = "$SUBTTL";                
char* PWstr    = "$PAGEWIDTH(";                
char* PLstr    = "$PAGELENGTH(";                 
char* NOLSTstr = "$NOLIST";                
char* LSTstr   = "$LIST";
char* NOSYMstr = "$NOSYMBOLS";                 
char* SYMstr   = "SYMBOLS";
char* ICLstr   = "$INCLUDE";                 
char* EJECTstr = "$EJECT";                 

char* TITLE_str = ".TITLE";      // introduced in XASM version 2.1           
char* SUBTL_str = ".SUBTTL";                 
char* PW_str    = ".PAGEWIDTH(";                 
char* PL_str    = ".PAGELENGTH(";                
char* NOLST_str = ".NOLIST";                 
char* LST_str   = ".LIST";
char* NOSYM_str = ".NOSYMBOLS";                
char* SYM_str   = ".SYMBOLS";
char* ICL_str   = ".INCLUDE";                
char* EJECT_str = ".EJECT";                
                                                 
char* ORG_str = "ORG";  // used in dcomp.cpp
char* DW_str  = "DW";                            
char* DB_str  = "DB";                            
char* DS_str  = "DS";                            
char* END_str = "END";                           
char* EQU_str = "EQU";                           
char* SET_str = "SET";
                                                 
char* WARN_str  = TITLE_str; // Dummy
char* ERR_str   = TITLE_str; // Dummy
char* MSG_str   = TITLE_str; // Dummy
char* DD_str    = DW_str;    // Dummy
char* DQ_str    = DW_str;    // Dummy
char* DEF_str   = EQU_str;   // Dummy
char* _defin_str= EQU_str;   // Dummy
char* DEFIN_str = EQU_str;   // Dummy
char* MODEL_str = ORG_str;   // Dummy
char* DEVICE_str= ORG_str;   // Dummy

char* CSEGSIZ_str = EQU_str; // Dummy

char* _if_def_str     = NULL; // Dummy
char* _if_ndef_str    = NULL; // Dummy
char* _elif_str       = NULL; // Dummy
char* _elif_def_str   = NULL; // Dummy
char* _elif_ndef_str  = NULL; // Dummy
char* _elseifdef_str  = NULL; // Dummy
char* _elseifndef_str = NULL; // Dummy
char* _elseif_str     = NULL; // Dummy

// Extern variables and functions
extern void DebugStop(int, char*, char*); // Usage: DebugStop(testNr, "Function()", __FILE__);
extern void DebugPrintBuffer(char*, int); // Usage: DebugPrintBuffer(buffer, count);

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
//  UPI-41/42  INSTRUCTION SET 
//
// each entry is organized as described below:
//   1: mnemonic string (nul-terminated),
//   2: group,
//   3: basic value (8bit)
//
// typedef struct tagINSTR {
//   char*  mneStr;
//   UCHAR  mneGrp;
//   UCHAR  mneVal;
//} INSTR, *LPINSTR;
//
INSTR instring[] = {
  // Group 1: Pseudo Instructions                                                      
  {ORG_str,  1,0x02},  // 02
  {"CSEG",   1,0x04},  // 04 No relocatable object format yet
  {"DSEG",   1,0x06},  // 06 No relocatable object format yet
//{"",       1 0x08},  // 08 Reserved
  {DW_str,   1,0x0A},  // 10  
  {DB_str,   1,0x0C},  // 12  
  {DS_str,   1,0x0E},  // 14  
  {END_str,  1,0x10},  // 16
  {EQU_str,  1,0x12},  // 18
  {"EXTRN",  1,0x14},  // 20 No relocatable object format yet
  {"PUBLC",  1,0x16},  // 22 No relocatable object format yet
  {TITLE_str,1,0x18},  // 24
  {TITLEstr, 1,0x18},  // 24 "$..."
  {SUBTL_str,1,0x1A},  // 26
  {SUBTLstr, 1,0x1A},  // 26 "$..."
  {PW_str,   1,0x1C},  // 28
  {PWstr,    1,0x1C},  // 28 "$..."
  {PL_str,   1,0x1E},  // 30
  {PLstr,    1,0x1E},  // 30 "$..."
  {ICL_str,  1,0x20},  // 32
  {ICLstr,   1,0x20},  // 32 "$..."
  {NOLST_str,1,0x22},  // 34
  {NOLSTstr, 1,0x22},  // 34 "$..."
  {LST_str,  1,0x24},  // 36
  {LSTstr,   1,0x24},  // 36 "$..."
  {NOSYM_str,1,0x26},  // 38
  {NOSYMstr, 1,0x26},  // 38 "$..."
  {SYM_str,  1,0x26},  // 38  Also handled in .NOSYMBOLS
  {SYMstr,   1,0x26},  // 38 "$..."
  {EJECT_str,1,0x28},  // 40
  {EJECTstr, 1,0x28},  // 40 "$..."
  {SET_str,  1,0x2B},  // 43

  // Group 2: 1-Byte-Instructions, no operands                                       [9]
  {"NOP"     ,2,0x00},  // 0b00000000},  // nopAlign=0x00 for .ALIGN / .EVEN directives
  {"RET"     ,2,0x83},  // 0b10000011},
  {"RETR"    ,2,0x93},  // 0b10010011},
  {"STANDBY" ,2,0x01},  // 0b00000001},
  {"SUSPEND" ,2,0xE2},  // 0b11100010},

  // Group 3: 1-Byte-Instructions, 1 operand = {A//Rn}
  {"DEC"  ,3,0x00},  // 0b00000000},

  // Group 4: 1-Byte-Instructions, 1 operand = {A//Rn//@R0//@R1}
  {"INC"  ,4,0x00},  // 0b00000000},

  // Group 5: 1-Byte-Instructions, 1 operand = {A//C//F0//F1}
  {"CLR"  ,5,0x00},  // 0b00000000},
  {"CPL"  ,5,0x00},  // 0b00000000},

  // Group 6: 1-Byte-Instruction, 1 operand = {@A}                                   [18]
  {"JMPP" ,6,0xB3},  // 0b10110011},

  // Group 7: 1-Byte-Instructions, 1 operand = {A}
  {"DA"   ,7,0x57},   // 0b01010111},
  {"RL"   ,7,0xE7},   // 0b11100111},
  {"RR"   ,7,0x77},   // 0b01110111},
  {"RLC"  ,7,0xF7},   // 0b11110111},
  {"RRC"  ,7,0x67},   // 0b01100111},
  {"SWAP" ,7,0x47},   // 0b01000111},

  // Group 8: 1-Byte-Instructions, 1 operand = {I//TCNTI//FLAGS//DMA}
  {"EN"   ,8,0x05},   // 0b00000101},

  // Group 9: 1-Byte-Instructions, 1 operand = {T//CNT}
  {"STRT" ,9,0x45},   // 0b01000101},

  // Group 10: 1-Byte-Instruction, 1 operand = {TCNT}
  {"STOP" ,10,0x65},  // 0b01100101},

  // Group 11: 1-Byte-Instructions, 1 operand = {RB0//RB1}
  {"SEL"  ,11,0xC5},  // 0b11000101},

  // Group 12: 1-Byte-Instructions, 1 operand = {I//TCNTI}
  {"DIS"  ,12,0x25},  // 0b00100101},

  // Group 13: 1-Byte-Instructions, opnd1 = {P1//P2}, opnd2 = {A}                    [30]
  {"OUTL" ,13,0x38},  // 0b00111000},

  // Group 14: 1-Byte-Instruction, opn0bd1 = {DB}, opnd2 = {A}
  {"OUT"  ,14,0x02},  // 0b00000010},

  // Group 15: 1-Byte-Instructions, opnd1 = {A}, opnd2 = {DBB//P1//P2}
  {"IN"   ,15,0x00},  // 0b00000000},

  // Group 16: 1-Byte-Instructions, opnd1 = {A//P4//P5//P6//P7}, opnd2 = {A//P4//P5//P6//P7}
  {"MOVD" ,16,0x0C},  // 0b00001100},
  {"ORLD" ,16,0x8C},  // 0b10001100},
  {"ANLD" ,16,0x9C},  // 0b10011100},

  // Group 17: 1-Byte-Instructions, opnd1 = {A}, opnd2 = {@A}
  {"MOVP" ,17,0xA3}, // 0b10100011},
  {"MOVP3",17,0xE3}, // 0b11100011},

  // Group 18: 1-Byte-Instructions, op = {A}, op2 = {Rn//@R0//@R1}                   [38]
  {"XCH"  ,18         ,0x20},  // 0b00100000}, //{A}{Rn//@R0//@R1}},
  {"XCHD" ,(18 | 0x80),0x30},  // 0b00110000}, //{A}{@R0//@R1}},

  // Group 19: 1or2-Byte-Instr., op = {A//P1//P2}, op2 = {Rn//@R0//@R1//#dd}
  {"ADD"  ,(19 | 0x40),0x60},  // 0b01100000}, //{A}{Rn//@R0//@R1//#dd}},
  {"ADDC" ,(19 | 0x40),0x70},  // 0b01110000}, //{A}{Rn//@R0//@R1//#dd}},
  {"XRL"  ,19         ,0xD0},  // 0b11010000}, //{A}{Rn//@R0//@R1//#dd}},
  {"ANL"  ,(19 | 0x80),0x50},  // 0b01010000}, //{A//P1//P2}{Rn//@R0//@R1//#dd}},   [43]
  {"ORL"  ,(19 | 0x80),0x40},  // 0b01000000}, //{A//P1//P2}{Rn//@R0//@R1//#dd}},

  // Group 20: 1or2-Byte-Instr., op =    //{A//PSW//T//@R1//@R2//Rn}, op2 = {{op1}//#dd}
  {"MOV"  ,20,0x00},  // 0b00000000}, //{A//PSW//T//@R1//@R2//Rn}{{op1}//#dd}},   [45]

  // Group 21: 2-Byte-Instructions, opnd1 = {aa} <= 0FFh                             [46]
  {"JB0"  ,21,0x12},  // 0b00010010},
  {"JB1"  ,21,0x32},  // 0b00110010},
  {"JB2"  ,21,0x52},  // 0b01010010},
  {"JB3"  ,21,0x72},  // 0b01110010},
  {"JB4"  ,21,0x92},  // 0b10010010},
  {"JB5"  ,21,0xB2},  // 0b10110010},
  {"JB6"  ,21,0xD2},  // 0b11010010},
  {"JB7"  ,21,0xF2},  // 0b11110010},
  {"JC"   ,21,0xF6},  // 0b11110110},
  {"JF0"  ,21,0xB6},  // 0b10110110},
  {"JF1"  ,21,0x76},  // 0b01110110},
  {"JNC"  ,21,0xE6},  // 0b11100110},
  {"JNIBF",21,0xD6},  // 0b11010110},
  {"JNT0" ,21,0x26},  // 0b00100110},
  {"JNT1" ,21,0x46},  // 0b01000110},
  {"JNZ"  ,21,0x96},  // 0b10010110},
  {"JOBF" ,21,0x86},  // 0b10000110},
  {"JTF"  ,21,0x16},  // 0b00010110},
  {"JT0"  ,21,0x36},  // 0b00110110},
  {"JT1"  ,21,0x56},  // 0b01010110},
  {"JZ"   ,21,0xC6},  // 0b11000110},

  // Group 22: 2-Byte-Instructions, opnd1 = {aaaa} <= 7FFh
  {"CALL" ,22,0x14},  // 0b00010100},                                             [67]
  {"JMP"  ,22,0x04},  // 0b00000100},

  // Group 23: 2-Byte-Instruction, opnd1 = {Rn},  opnd2 = {aaaa} <= 0FFh           [69]
  {"DJNZ" ,23,0xE8},  // 0b11101000}

  // Group END:
  {"0"    ,99,0x00}
  };                     // end of instruction table

// Extern variables and functions
extern void DebugStop(char*, int);        // Usage: DebugStop("Text", i);
extern void DebugPrintBuffer(char*, int); // Usage: DebugPrintBuffer(buffer, count);

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

