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
char* signon = "M68HC05 Macro-Assembler, Version 2.1\n";

int predefCount = 0;                     // Not used with 68HC05

// Valid operand string(s)
char* operstr_A     = "A";
char* operstr_X     = "X";

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
// (refer to Motorola "MCHC05P4A Technical Data Rev.7 5/2002" Page 26 Fig. 2-1)                              
// (also see Freescale Semiconductor, Inc. "M68HC05 Family Rev.2.0" P61 Fig. 14)
//
#define IO_MIN          0x0000 // 32byte IO min address
#define IO_MAX          0x001F // IO max address
#define IO_SIZE         32     // 32 bytes IO

#define USER_ROM_START  0x0020 // USER ROM Page Zero start address
#define USER_ROM_END    0x004F // USER ROM Page Zero end address
#define USER_ROM_SIZE   48     // 48 bytes USER ROM

#define RAM_START       0x0050 // RAM start address
#define RAM_END         0x00FF // RAM limit address (recommended)
#define RAM_SIZE        176    // 176 (0x00B0) bytes RAM (shared with STACK!) 

#define STACK_END       0x00C0 // STACK RAM limit lo address (recommended)
#define STACK_START     0x0100 // STACK RAM start hi address
#define STACK_SIZE      64     // 64 (0x0040) bytes STACK (recommended, shared with RAM!)

#define ROM_START       0x0100  // ROM start address
#define ROM_END         0x10FF  // ROM end address
//ha//#define ROM_SIZE        4*1024 // 4K (0x1000) ROM
#define ROM_SIZE        64*1024 // 64K (0x10000) ROM (allow any HW design)

#define UNUSED_START    0x1100 // UNUSED start address
#define UNUSED_END      0x1EFF // UNUSED end address
#define UNUSED_SIZE     3584   // 3.5K  (0x0E00) UNUSED range 

#define SELFTEST_ROM_START     0x1F00 // SELFTEST ROM start address
#define SELFTEST_ROM__END      0x1FDF // SELFTEST ROM end address
#define SELFTEST_ROM_SIZE      240    // 240 bytes (0x00F0) SELFTEST ROM

#define SELFTEST_VECTORS_START 0x1FE0 // SELFTEST VECTORS start address 
#define SELFTEST_VECTORS_END   0x1FEF // SELFTEST VECTORS end address
#define SELFTEST_VECTORS_SIZE  16     // 16 bytes (0x00F0) SELFTEST VECTORS

#define USER_VECTORS_START     0x1FF0 // USER_VECTORS start address
#define USER_VECTORS_END       0x1FFF // Power on 16bit reset address vector

#define ADDRESS_RANGE          16*1024// 16K (0x4000) M68HC05BD5 max address range

//------------------------------------------------------------------------------
//
//                          InitMemorySize
//
void InitMemorySize()
  {
  RomSize  = ADDRESS_RANGE;
  SRamSize = RAM_SIZE + STACK_SIZE;
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
  
  nopAlign = 0x9D;            // NOP instruction value for .ALIGN / .EVEN directives
  MotorolaFlag = TRUE;        // Set Motorola format convention flag
  } // oper_preval

//-----------------------------------------------------------------------------
//
//                      p1eval
//
// PASS1 INSTRUCTION EVALUATION
//
void p1eval()
  {
  if (ins_group == ERR) pcc += 3;             // group 255: illegal, give 3 bytes
  else if (ins_group == 1) return;            // group 1: Pseudo instructions

  else if (ins_group == 2) pcc += 1;          // group 2: 1 byte, definitely!
  else if (ins_group == 3) pcc += 2;          // group 3: 2 byte, definitely!

  else if (ins_group == 4)                    // group 4: 1 or 2 byte
    {
    if (oper_x(oper1) == TRUE) pcc++;         // INC  ,X
    else pcc += 2;                            // INC offset8 or INC offset8,X
    }

  else if (ins_group == 5 || ins_group == 6)  // groups 5,6: 1, 2 or 3 bytes
    {
    if (oper_x(oper1) == TRUE) pcc++;         // LDA ,X
    else if (oper1[0] == '#') pcc += 2;       // LDA #offset8
    else
      {
      value = expr(oper1);                    // returns _exprInfo

      // Undefined or forward reference in User ROM - Page Zero ($0020 .. 004F)
      // Special prediction, assumes RAM access only - refer to M68HC05 memory layout.
      if (_exprInfo == ERR_UNDFSYM && pcc >= 0x20 && pcc < 0x50)  
        pcc += 2;

      else if (_exprInfo == ERR_UNDFSYM) pcc += 3; // not yet defined, assume 3bytes
      else if ((value & 0xFF00) == 0) pcc += 2;    // LDA offset8 or  LDA offset8,X
      else pcc += 3;                               // LDA offset16 or LDA offset16,X
      }
    } // end else if

  else if (ins_group == 7) pcc += 2;          // group 7: 2 bytes, definitely!
  else if (ins_group == 8) pcc += 3;          // group 8: 3 bytes, definitely!
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
    errorp2(ERR_DSEG, NULL); // Bad Segment
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
  errorp2(ERR_BADREG,NULL);
  p2eval(11);        // Give 1 byte on error
  } // ins_error11
                      

//-----------------------------------------------------------------------------
//
//                             ins_error22
//
void ins_error22()
  {
  errorp2(ERR_BADREG,NULL);
  p2eval(22);        // Give 2 bytes on error
  } // ins_error22


//-----------------------------------------------------------------------------
//
//                             ins_error33
//
void ins_error33()
  {
  errorp2(ERR_BADREG,NULL);
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
  errorp2(ERR_ILLOPCD,NULL);
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

  if ((op1val-(pcc+2)) >= 0 && ((op1val-(pcc+2)) & 0xFF00) == 0)
    insv[2] = (op1val-(pcc+2)) & 0xFF;

  else if ( ((op1val-(pcc+2)) & 0xFF00) && ((op1val-(pcc+2) & 0x80) == 0x80))
    insv[2] = (op1val-(pcc+2)) & 0xFF;

  else errorp2(ERR_JMPADDR, NULL);

  p2eval(21);           // 2 byte instr., 2nd operand must be empty
  } // ins3

//-----------------------------------------------------------------------------
//
//                            ins4
//
// INSTRUCTION GROUP 4
// direct addressing mode: e.g LDA offset8
//
void ins4() 
  {
  if (oper_x(oper1) == TRUE)
    {
    insv[1] |= 0x50;    // indexed: e.g. LDA ,X
    p2eval(11);
    }
  
  else if (oper_x(oper2) == TRUE)
    {
    value = expr(oper1);
    insv[2] = value;

    if ((value & 0xFF00) != 0)
      {
      errorp2(ERR_ILLEXPR, NULL);
      p2eval(22);        // Give 2 bytes on error
      return;
      } 
    else 
      {
      insv[1] |= 0x40;   // indexed: e.g. LDA ,X
      p2eval(22);        // Give 2 bytes, 2 operands
      }
    }

  else if (oper1 != 0)
    {
    value = expr(oper1);
    insv[2] = value;

    if ((value & 0xFF00) != 0)
      {
      errorp2(ERR_ILLEXPR, NULL);
      p2eval(22);        // Give 2 bytes on error
      return;
      } 

    else 
      {
      insv[1] |= 0x10;   // indexed: e.g. LDA ,X
      p2eval(21);        // Give 2 bytes
      }
    }
  } // ins4

//-----------------------------------------------------------------------------
//
//                            ins5_ins6
//
// INSTRUCTION GROUPs 5 and 6
// 
//
void ins5_ins6()
  {
  int op1val;

  // indexed: STA ,X
  if (oper_x(oper1) == TRUE && oper2[0] == 0 && oper3[0] == 0)
    {
    insv[1] |= 0x70;    // build opcode
    p2eval(11);         // 1 byte, 1 operand
    return;
    }
  
  op1val = expr(oper1);

  if (oper1 != 0 && oper_x(oper2) == FALSE)
    {
    // Extended hh ll: STA address16
    if ((op1val & 0xFF00) != 0)
      {
      insv[1] |= 0x40;
      insv[2] = (op1val >> 8) & 0xFF;
      insv[3] = op1val & 0xFF;
      p2eval(31);           // Give 3 bytes, 1 operand
      }

    // Direct dd: STA address8
    else 
      {
      insv[1] |= 0x30;
      insv[2] = op1val & 0xFF;
      p2eval(21);           // Give 2 bytes, 1 operand
      }
    }

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
   
  } // ins5_ins6

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
  // immediate ii: No immediate mode (STA #!)
  if (oper1[0] == '#')   
    {
    errorp2(ERR_ILLEXPR, NULL);
    p2eval(22);          // give 2 bytes on error
    }
   
  else ins5_ins6();
  } // ins5


//-----------------------------------------------------------------------------
//
//                            ins6
//
// INSTRUCTION GROUP 6
// immediate: e.g. CMP #12
// direct:  e.g. CMP offset8
// indexed: e.g. CMP ,X
//
void ins6() 
  {
  // immediate ii: CMP #12
  if (oper1[0] == '#')
    {
    insv[1] |= 0x20;
    value = expr(&oper1[1]);
    insv[2] = value;
    p2eval(22);           // Give 2 bytes, 2 operands
    }

  else ins5_ins6();
  } // ins6


//-----------------------------------------------------------------------------
//
//                            ins7
//
// INSTRUCTION GROUP 7
//
// direct:  e.g. BSET 7,offset8
//
void ins7() 
  {
  int op1val, op2val;

  op1val = expr(oper1);
  if (op1val > 7)
    {
    ins_error22();
    return;
    }
  insv[1] += op1val << 1;  // Build opcode 

  op2val = expr(oper2);
  if ((op2val & 0xFF00) != 0) ins_error22();
  else
    {
    insv[2] = op2val;
    p2eval(22);           // Give 2 bytes, 2 operands
    }
  } // ins7

//-----------------------------------------------------------------------------
//
//                            ins8
// INSTRUCTION GROUP 8
// 
// direct:  e.g. BRSET 7,offset8,rel
//
void ins8() 
  {
  int op1val, op2val, op3val;

  op1val = expr(oper1);
  if (op1val > 7)
    {
    ins_error22();
    return;
    }
  insv[1] += op1val << 1;  // Build opcode
   
  op2val = expr(oper2);
  op3val = expr(oper3);

  insv[2] = op2val;
  insv[3] = op3val - (pcc+3);

  if ((op2val & 0xFF00) != 0)
    {
    errorp2(ERR_JMPADDR, NULL);
    p2eval(33);         // Give 3 bytes on error
    }
  
  else if (((op3val-(pcc+3)) & 0xFF00) == 0 && ((op3val-(pcc+3)) & 0x80) == 0)
    p2eval(33);            // Give 3 bytes, 3 operands

  else if (((op3val-(pcc+3)) & 0xFF00) == 0xFF00 && ((op3val-(pcc+3)) & 0x80) == 0x80)
    p2eval(33);            // Give 3 bytes, 3 operands

  else
    {
    errorp2(ERR_JMPADDR, NULL);
    p2eval(33);         // Give 3 bytes on error
    }
  } // ins8


//-----------------------------------------------------------------------------
//
//                       ins9..33  Reserved
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
//ha//if (_debugSwPass == _PASS2)
//ha//{
//ha//printf("pcc=%04X_labAdr=%04X  _pcAdr=%04X  insv[2]=%02X erno=%d\n(_labAdr-_pcAdr) & 0xFF00=%04X\n",
//ha//        pcc,     _labAdr,     _pcAdr,      insv[2],     erno,    (_labAdr-_pcAdr) & 0xFF00);
//ha//printf("oper1 ");
//ha//DebugPrintBuffer(oper1, OPERLEN);
//ha//DebugStop(1, "p2eval()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//--------------------------end-of-c++-module-----------------------------------

