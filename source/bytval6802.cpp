// haXASM - Cross-Assembler for Motorola 8bit processors
// bytval68HC05.cpp - C++ Developer source file.
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
char* signon = "6800/6802 Macro-Assembler, Version 2.1\n";

int predefCount = 0;              // Not used with MC6802

// Valid operand strings
char* operstr_X     = "X";
char* operstr_A     = "A";

// Extern variables and functions
extern void DebugStop(int, char*, char*); // Usage: DebugStop(testNr, "Function()", __FILE__);
extern void DebugPrintBuffer(char*, int); // Usage: DebugPrintBuffer(buffer, count);

extern int error;

extern char szCmdlineSymbol[];

extern char* skip_leading_spaces(char*);
extern UINT expr(char *);
extern void edpri();
extern void clepc();
extern void errorp2(int, char*);

// Forward declaration of functions included in this code module:
int oper_x(char* _oper_X);
void ins6();

//------------------------------------------------------------------------------
//
// BASIC HARDWARE MEMORY-MAPPED ADDRESS LAYOUT
// (refer to "Motorola M6800 Programming Reference Manual 1976", Chapter 2.1)                              
//
#define RAM_START 0x0000 // 128byte RAM storage start address
#define RAM_END   0x007F // RAM storage limit at address 0x007F

// Address lines [A9..A0] ROM = 64K
#define ROM_SIZE 64*1024

// Address lines [A6..A0] RAM = 128byte 
#define RAM_SIZE 128

// Address lines [A2..A0] I/O Ports = PA7..PA0
#define PIA_MIN    0     
#define PIA_MAX    7
                             
#define ROM_START 0xFC00 // 1K ROM start address at 63K
#define ROM_RESET 0xFFFF // Power on 16bit reset address vector

//------------------------------------------------------------------------------
//
//                          InitMemorySize
//
void InitMemorySize()
  {
  RomSize  = ROM_SIZE;
  SRamSize = RAM_SIZE;
  } // InitMemorySize

//------------------------------------------------------------------------------
//
//                          dq_endian                      
//
// Dummy: little endian
//
void dq_endian()
  {                                   // Intel: little endian
  ilen+=8;                            // Count byte elements
  return;
  } // dq_endian


//------------------------------------------------------------------------------
//
//                          dd_endian
//
// Motorola: big endian
//
void dd_endian()
  {                                   // Intel: little endian
  ilen++;                             // Count byte elements
  insv[ilen] = (lvalue >> 24) & 0xFF; // 4th hi byte
  ilen++;                             // Count byte elements
  insv[ilen] = (lvalue >> 16) & 0xFF; // 3rd byte
  ilen++;                             // Count byte elements
  insv[ilen] = (lvalue >> 8) & 0xFF;  // 2nd byte
  ilen++;                             // Count byte elements
  insv[ilen] = lvalue & 0xFF;         // 1st lo byte
  return;
  } // dd_endian


//------------------------------------------------------------------------------
//
//                          dw_endian
//
// Motorola: big endian
//
void dw_endian()
  {
  int _bigEndianValue = 0;
                                    // Motorola: big endian
  ilen++;                           // Count byte elements
  insv[ilen] = (value >> 8) & 0xFF; // 1st hi byte
  ilen++;                           // Count byte elements
  insv[ilen] = value & 0xFF;        // 2nd lo byte
  return;
  } // DW_endian

//-----------------------------------------------------------------------------
//
//                      oper_preval
//
// Prepare and format headline in listing
// MC68HC05 Cross-Assembler, Version 2.1                     09/11/2001 PAGE 0000
//
void oper_preval()
  {
  InitMemorySize();           // Init memory size (RAM / ROM)

  datpos_ptr = &lhbuf[59];    // Init position for 10 chars of date string
  pagpos_ptr = datpos_ptr+16; // Init position for 4-digits page number

  for (_i=0; _i<LHBUFLEN; _i++) lhbuf[_i] = SPACE; // Space out lhbuf area

  lhbuf[0] = FF;
  for (_i=0; _i<(strlen(signon)-1); _i++) lhbuf[_i+1] = signon[_i];
  for (_i=0; _i<strlen(lh_date); _i++) datpos_ptr[_i] = lh_date[_i];
  sprintf(&datpos_ptr[strlen(lh_date)+1], " PAGE 0000\n");

  // Symbol from cmdline
  if (szCmdlineSymbol[0] > SPACE)
    {
    StrCpy(symboltab_ptr->symString, szCmdlineSymbol);        
    symboltab_ptr->symAddress = 0x01;   // Value = 0x01
    symboltab_ptr->symType = _ABSSET;   // Type = redefinable absolute constant
    symboltab_ptr++;
    }
  
  nopAlign = 0x01;            // NOP instruction value for .ALIGN / .EVEN directives
  MotorolaFlag = TRUE;        // Set Motorola format convention flag
  } // oper_preval

//-----------------------------------------------------------------------------
//
//                      p1eval
//
void p1eval()
  {
  ins_group &= ~(0x40|0x80);                  // Turn off special flags

  if (ins_group == ERR) pcc += 3;             // group 255: illegal, give 3 bytes
  else if (ins_group == 1) return;            // group 1: Pseudo instructions

  // Addressing Mode: IMPLIED
  else if (ins_group == 2) pcc += 1;          // group 2: 1 byte, definitely!
  // Addressing Mode: RELATIVE
  else if (ins_group == 3) pcc += 2;          // group 3: 2 byte, definitely!

  // Addressing Modes: IMMEDIATE, DIRECT, INDEXED, EXTENDED
  else if (ins_group == 4 || ins_group == 5)  // group 4,5: 2 or 3 bytes
    {
    if (oper_x(oper1) == TRUE || oper_x(oper2) == TRUE)
      pcc+=2;                                 // INC  ,X  INC X,offset8 - 2 bytes

    else if (oper2[0] != 0) pcc+=3;           // Two operands - 3 bytes

    else if (oper1[0] == '#' && ins_group == 5)            
      pcc+=3;                                 // One operand 16bit - 3 bytes
  
    else if (oper1[0] == '#' && ins_group == 4)            
      pcc+=2;                                 // One operand 8bit - 2 bytes
    
    else if (oper2[0] == 0)                   // One operand - 2or3 bytes
      {                                      
      value = expr(oper1);
      if (_exprInfo == ERR_UNDFSYM) pcc += 3; // not yet defined, assume 3bytes
      else if ((value & 0xFF00) == 0) pcc+=2; // 2 bytes (label 8bit)
      else pcc+=3;                            // 3 bytes (label 16bit)
      }                                       // Label may be defined later in source
    } // end else if (group 4 and group 5)
  
  // Addressing Modes: INDEXED, EXTENDED
  else if (ins_group == 6 || ins_group == 7)  // groups 6,7: 2 or 3 bytes
    {
    value = expr(oper1);
    if (_exprInfo == ERR_UNDFSYM) pcc += 3;   // not yet defined, assume 3bytes
    else if (oper_x(oper1) == TRUE || oper_x(oper2) == TRUE)
      pcc+=2;                                 // INC  ,X  INC X,offset8
    else pcc += 3;                            // LDA offset16 or LDA offset16,X
    } // end else if (group 6 and group 7)
  } // p1eval

//------------------------------------------------------------------------------
//
//                           oper_x
//
int oper_x(char* _oper_X)
  {                                                               
  if (toupper(_oper_X[0]) == 'X' && _oper_X[1] <= SPACE) return(TRUE);
  else return(FALSE);
  } // oper_x

//------------------------------------------------------------------------------
//
//                          p2emp  P2 EMPTY
//
// Check if the unused operand field is empty. Otherwise give error
//
void p2emp(char _operN[])
  {
  if (_operN[0] != 0) errorp2(ERR_TOOLONG, _operN);
  } // p2emp

//-----------------------------------------------------------------------------
//
//                             p2eval
//
// PASS2 INSTRUCTION EVALUATION
//
void p2eval(int _num)
  {
  if (SegType != _CODE)
    {
    errorp2(ERR_DSEG, NULL);
    clepc();
    edpri();
    return;
    }

  switch (_num)
    {
    case 10:          // One byte, no operands
      p2emp(oper1);   // Unused operands empty?
      p2emp(oper2);
      p2emp(oper3);
      ilen = 1;
      break;
    case 11:          // One byte, one operand
      p2emp(oper2);
      p2emp(oper3);
      ilen = 1;
      break;
    case 12:          // One byte, two operands
      ilen = 1;
      p2emp(oper3);
      break;
    case 21:          // Two bytes, one operand
      p2emp(oper2);
      p2emp(oper3);
      ilen = 2;
      break;
    case 22:          // Two bytes, two operands
      ilen = 2;
      p2emp(oper3);
     break;
    case 31:          // Three bytes, one operand
      p2emp(oper2);
      p2emp(oper3);
      ilen = 3;
      break;
    case 32:          // Three bytes, two operands
      ilen = 3;
      p2emp(oper3);
      break;
    case 33:          // Three bytes, three operands
      ilen = 3;
      break;
    default:
      ilen = 0;       // Default none
      break;
    } // end switch (_num)

  edpri();
  } // p2eval         // return to Pass 2

//-----------------------------------------------------------------------------
//
//                             ins_error11
//
void ins_error11()
  {
  if (erno == 0) errorp2(ERR_BADREG, NULL);
  p2eval(11);        // Give 1 byte on error
  } // ins_error11
                      

//-----------------------------------------------------------------------------
//
//                             ins_error21
//
void ins_error21()
  {                                          // Error previously?
  if (erno == 0) errorp2(ERR_JMPADDR, NULL); // Set error "branch out of range"
  p2eval(21);                                // 2 bytes, 2nd operand must be empty
  } // ins_error21

//-----------------------------------------------------------------------------
//
//                             ins_error22
//
void ins_error22()
  {
  if (erno == 0) errorp2(ERR_BADREG, NULL);
  p2eval(22);        // Give 2 bytes on error
  } // ins_error22


//-----------------------------------------------------------------------------
//
//                             ins_error33
//
void ins_error33()
  {
  if (erno == 0) errorp2(ERR_BADREG, NULL);
  p2eval(33);        // Give 3 bytes on error
  } // ins_error33


//-----------------------------------------------------------------------------
//
//                             ins0
//
// INSTRUCTION GROUP 0: ILLEGAL INSTRUCTION
//  2 bytes nul
//
void ins0()
  {
  errorp2(ERR_ILLOPCD, NULL);
  p2eval(32);          // 68HC05: illegal opcode, 3 bytes NUL
  } // ins0

//-----------------------------------------------------------------------------
//
//                            ins2
//
// INSTRUCTION GROUP 2
// single byte, no operands
//
void ins2()
  {
  p2eval(10);  // 1 byte, 0 operands
  } // ins2

//-----------------------------------------------------------------------------
//
//                            ins3
//
// INSTRUCTION GROUP 3
// Bxx rr (un)conditional jumps (branch relative)
//  rr    Relative program counter offset byte (-128 to +127 Bytes)
//
void ins3()
  {
  int op1val;

  op1val = expr(oper1);
  insv[2] = (op1val-(pcc+2)) & 0xFF;

  // check range -128..+127
  // 1) negative range -1..-128, higher must be 0ffh
  if (((op1val-(pcc+2)) & 0xFF00) == 0xFF00 && ((op1val-(pcc+2)) & 0x80) == 0x80)
    p2eval(21);           // 2 byte instr., 2nd operand must be empty

  // 2) positive range 0..+127, higher must be 0
  else if (((op1val-(pcc+2)) & 0xFF00) == 0 && ((op1val-(pcc+2)) & 0x80) == 0)
    p2eval(21);           // 2 byte instr., 2nd operand must be empty

  else ins_error21();
  } // ins3

//-----------------------------------------------------------------------------
//
//                            ins4_ins5
//
// INSTRUCTION GROUPs 4 and 5
// 
//
void ins4_ins5()
  {
  int op1val;

  // indexed: LDAA X
  if (oper_x(oper2) == TRUE && oper3[0] == 0)
    {
    insv[1] |= 0x20;    // build opcode
    op1val = expr(oper1);
    insv[2] = op1val & 0xFF;
    p2eval(22);         // 2 bytes, 2 operands
    return;
    }
  if (oper_x(oper1) == TRUE && oper2[0] == 0)
    {
    insv[1] |= 0x20;    // build opcode
    insv[2] = 0;
    p2eval(21);         // 2 bytes, 1 operand
    return;
    }
  
  // immediate ii: No immediate mode (STA #!)
  if (oper1[0] == '#' && (ins_group & 0x80) == 0x80)   
    {
    errorp2(ERR_ILLEXPR, NULL);
    p2eval(22);          // give 2 bytes on error
    return;
    }
  
  // immediate ii: ;LDAA #, LDX #  (16bit)
  else if (oper1[0] == '#' && ins_group == 5)
    {
    op1val = expr(&oper1[1]);
    insv[2] = (op1val >> 8) & 0xFF;
    insv[3] = op1val & 0xFF;
    p2eval(33);           // Give 3 bytes, 1 operand
    return;
    }

  // immediate ii: ;LDAA #, LDX #  (8bit)
  else if (oper1[0] == '#' && ins_group == 4)
    {
    op1val = expr(&oper1[1]);
    if (op1val > 255)
      {
      errorp2(ERR_ILLBYTE, oper1);
      p2eval(21);          // give 3 bytes on error
      return;
      }
    insv[2] = op1val & 0xFF;
    p2eval(21);           // Give 2 bytes, 1 operand
    return;
    }

  op1val = expr(oper1);
  if (op1val > 0xFFFF)
    {
    errorp2(ERR_SEGADDR, oper1);
    p2eval(21);          // give 2 bytes on error
    return;
    }

  if (oper1[0] != 0 && oper_x(oper2) == FALSE)
    {
    // Extended hh ll: STA address16
    if ((op1val & 0xFF00) != 0)
      {
      insv[1] |= 0x30;
      insv[2] = (op1val >> 8) & 0xFF;
      insv[3] = op1val & 0xFF;
      p2eval(31);           // Give 3 bytes, 1 operand
      }

    // Direct dd: STA address8
    else if ((op1val &  0xFF00) == 0)
      {
      insv[1] |= 0x10;
      insv[2] = op1val & 0xFF;
      p2eval(21);           // Give 2 bytes, 1 operand
      }
    } // end if

  else if (oper1 != 0 && oper_x(oper2) == TRUE)
    {
    // Indexed IX2 16-bit offset ee ff: CMP address16, X
    if ((op1val & 0xFF00) != 0)
      {
      insv[1] |= 0x50;
      insv[2] = (op1val >> 8) & 0xFF;
      insv[3] = op1val & 0xFF;
      p2eval(32);           // Give 3 bytes, 2 operands
      }

    // Indexed IX1 8-bit offset ee ff: CMP address8, X
    else 
      {
      insv[1] |= 0x60;
      insv[2] = op1val & 0xFF;
      p2eval(22);           // Give 2 bytes, 2 operands
      }
    }
  } // ins4_ins5

//-----------------------------------------------------------------------------
//
//                            ins4
//
// INSTRUCTION GROUP 4
// direct addressing mode: e.g LDA offset8
//
void ins4() 
  {
  ins4_ins5();
  } // ins4

//-----------------------------------------------------------------------------
//
//                            ins5
//
// INSTRUCTION GROUP 5
// direct:  e.g. STA offset8
// indexed: e.g. STAA ,X
//
void ins5() 
  {
  ins4_ins5();
  } // ins5

//-----------------------------------------------------------------------------
//
//                            ins6_ins7
//
// INSTRUCTION GROUP 6 and 7
// Evaluate OBJ-byte of instruction code according to the various
// addressing modes of the Motorola 6800/6802
//
// RAA-byte group: 1xaa [code]  RY-byte group: 011y [code]
//   aa|addr-mode     y|addr-mode
//   ------------     -----------
//   00|#             0|,X  
//   01|direct        1|extended 
//   10|,X                       
//   11|extended                 
//
void ins6_ins7() 
  {
  int op1val;

  // indexed: JMP X
  if (oper_x(oper1) == TRUE)
    {
    p2eval(21);           // Give 2 bytes, 1 operand
    }

  // immediate ii: No immediate mode (JMP #!)
  else if (oper1[0] == '#' && (ins_group & 0x80) == 0x80)  
    {
    errorp2(ERR_ILLEXPR, NULL);
    p2eval(22);          // give 2 bytes on error
    }

  // indexed: JMP offset8,X
  else if (oper_x(oper2) == TRUE)
    {
    op1val = expr(oper1);
    insv[2] = op1val;
    p2eval(22);
    }
 
  // indexed: JMP offset16
  else if (oper1[0] != 0 && oper2[0] == 0)
    {
    op1val = expr(oper1);
    insv[1] |= 0x10;
    // Motorola expects high/low in RAM
    insv[2] = (op1val >> 8) & 0xFF;
    insv[3] = op1val & 0xFF;
    p2eval(31);           // Give 1 bytes, 1 operand
    }
  } // ins6_ins7


//-----------------------------------------------------------------------------
//
//                            ins6
//
// INSTRUCTION GROUP 6
//
void ins6() 
  {
  ins6_ins7();
  } // ins6

//-----------------------------------------------------------------------------
//
//                            ins7
//
// INSTRUCTION GROUP 7
//
void ins7() 
  {
  ins6_ins7();
  } // ins7


//-----------------------------------------------------------------------------
//
//                       ins8..33  Reserved
void ins8()  {}
void ins9()  {}
void ins10() {}
void ins11() {}
void ins12() {}
void ins13() {}
void ins14() {}
void ins15() {}
void ins16() {}
void ins17() {}
void ins18() {}
void ins19() {}
void ins20() {}
void ins21() {}
void ins22() {}
void ins23() {}
void ins24() {}
void ins25() {}
void ins26() {}
void ins27() {}
void ins28() {}
void ins29() {}
void ins30() {}
void ins31() {}
void ins32() {}
void ins33() {}

//-----------------------------------------------------------------------------

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//if (_debugSwPass == 2)
//ha//{
//ha//printf("oper1=%s  op1val=%04X", oper1, op1val);
//ha//DebugStop(1, "ins6_ins7()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("pcc=%04X_labAdr=%04X  _pcAdr=%04X  insv[2]=%02X erno=%d\n(_labAdr-_pcAdr) & 0xFF00=%04X\n",
//ha//        pcc,     _labAdr,     _pcAdr,      insv[2],     erno,    (_labAdr-_pcAdr) & 0xFF00);
//ha//printf("oper1 ");
//ha//DebugPrintBuffer(oper1, OPERLEN);
//ha//DebugStop(1, "p2eval()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//--------------------------end-of-c++-module-----------------------------------

