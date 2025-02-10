// haXASM - Cross-Assembler for Atmel/Microchip processors
// bytvalAVR.cpp - C++ Developer source file.
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
char* signon = "AVR Macro-Assembler, Version 2.1       \n";

char _syntaxMOVW[] = "Operand (Rd+1:Rd,Rr+1:Rr) expected";
char _syntaxIW[]   = "Operand (Rd+1:Rd,K) expected";

char _syntaxBuf[20];
  
int predefCount;       // Used with Atmel AVR

// Extern variables and functions
extern void DebugStop(int, char*, char*); // Usage: DebugStop(testNr, "Function()", __FILE__);
extern void DebugPrintBuffer(char*, int); // Usage: DebugPrintBuffer(buffer, count);

extern char szCmdlineSymbol[];

extern void remove_white_spaces(char*);
extern UINT expr(char*);
extern void edpri();
extern void clepc();
extern void errorp2(int, char*);

//------------------------------------------------------------------------------
//
// BASIC HARDWARE LAYOUT
//  (refer to the corresponding Atmel/Microchip User's Manual(s)
//   and use the specific part definition file (partdef.inc), 
//   provided by Atmel for each of their various CPU designs)
//

//------------------------------------------------------------------------------
//
//                          InitMemorySize
//
void InitMemorySize()
  {
  RomSize   =  256*1024*1024; // Program Flash ROM: Board design specific (default=256M)
  SRamStart =          0x000; // Internal SRAM start address: Board design specific (default=0x000)
  SRamSize  =    8*1024*1024; // Internal SRAM: Board design specific (default=8M)
  EEPromSize =       64*1024; // EEPROM: Board design specific (default = 64K)
  } // InitMemorySize

//------------------------------------------------------------------------------
//
//                          dq_endian                      
//
// Atmel: little endian
//
void dq_endian()
  {                                   // Intel: little endian
  ilen++;                             // Count byte elements
  insv[ilen] = qvalue & 0xFF;         // 1st lo byte
  ilen++;                             // Count byte elements
  insv[ilen] = (qvalue >> 8) & 0xFF;  // 2nd byte
  ilen++;                             // Count byte elements
  insv[ilen] = (qvalue >> 16) & 0xFF; // 3rd byte
  ilen++;                             // Count byte elements
  insv[ilen] = (qvalue >> 24) & 0xFF; // 4th byte
  ilen++;                             // Count byte elements
  insv[ilen] = (qvalue >> 32) & 0xFF; // 5th byte
  ilen++;                             // Count byte elements
  insv[ilen] = (qvalue >> 40) & 0xFF; // 6th byte
  ilen++;                             // Count byte elements
  insv[ilen] = (qvalue >> 48) & 0xFF; // 7th byte
  ilen++;                             // Count byte elements
  insv[ilen] = (qvalue >> 56) & 0xFF; // 8th hi byte
  return;
  } // dq_endian


//------------------------------------------------------------------------------
//
//                          dd_endian                      
//
// Atmel: little endian
//
void dd_endian()
  {                                   // Intel: little endian
  ilen++;                             // Count byte elements
  insv[ilen] = lvalue & 0xFF;         // 1st lo byte
  ilen++;                             // Count byte elements
  insv[ilen] = (lvalue >> 8) & 0xFF;  // 2nd byte
  ilen++;                             // Count byte elements
  insv[ilen] = (lvalue >> 16) & 0xFF; // 3rd byte
  ilen++;                             // Count byte elements
  insv[ilen] = (lvalue >> 24) & 0xFF; // 4th hi byte
  return;
  } // dd_endian


//------------------------------------------------------------------------------
//
//                          dw_endian
//
// Atmel: little endian
//
void dw_endian()
  {                                 // Intel: little endian
  ilen++;                           // Count byte elements
  insv[ilen] = value & 0xFF;        // 1st lo byte
  ilen++;                           // Count byte elements
  insv[ilen] = (value >> 8) & 0xFF; // 2nd hi byte
  return;
  } // DW_endian

//-----------------------------------------------------------------------------
//
//                      oper_preval
//
// Prepare and format headline in listing
// Atmel AVR Macro-Assembler, Version 2.1                    09/11/2001 PAGE 0000
//
void oper_preval()
  {
  InitMemorySize();           // Init memory size (RAM / ROM)

  lpLOC  = LOC32;  // start column of 16bit program counter
  lpOBJ  = OBJ32;  // start column of object code bytes
  lpSRC  = SRC32;  // start column (in TABs 9..17..) of source statement

  lpLINE = LINE32; // start column of line count
  lpMARK = MARK32; // special marker, e.g. 'C' for include file line
  lpMACR = MACR32; // special marker, e.g. '+' for macro expansion
  lpXSEG = XSEG32; // start column of segment type indicator

  lhtxt_ptr = lhtxt32;        // Default pcc = 16bit

  datpos_ptr = &lhbuf[59];    // Init position for 10 chars of date string
  pagpos_ptr = datpos_ptr+16; // Init position for 4-digits page number

  for (_i=0; _i<LHBUFLEN; _i++) lhbuf[_i] = SPACE; // Space out lhbuf area

  lhbuf[0] = FF;
  for (_i=0; _i<(strlen(signon)-1); _i++) lhbuf[_i+1] = signon[_i];
  for (_i=0; _i<strlen(lh_date); _i++) datpos_ptr[_i] = lh_date[_i];
  sprintf(&datpos_ptr[strlen(lh_date)+1], " PAGE 0000\n");

  // Hidden pre-definition of important registers R0..R31 needed for EQU aliases
  // Insert predefined symbols at start of symboltab array
  //
  //  typedef struct tagSYMBOLBUF {  // Symbol structure
  //    char  symString[SYMLEN + 1]; // Symbol/Label name string
  //    ULONG symAddress;            // Symbol value or pointer to operand string
  //    UCHAR symType;               // Symbol type (ABS, C-SEG, UNDEFINED)
  //  } SYMBOLBUF, *LP_SYMBOLBUF;
  //
  symboltab_ptr = symboltab;
  predefCount = 31+1;                  // 32 symbols: "R0".."R31"
  for (_i=0; _i<predefCount; _i++)      
    {
    symboltab_ptr->symString[0] = 'R'; // Symbol string "Rd" "Rr"     
    // Convert to ASCII
    if (_i<=9) symboltab_ptr->symString[1] = _i | '0';
    else
      {
      _k = ((_i / 10) << 4) | (_i % 10);
      symboltab_ptr->symString[2] = (_k & 0x0F) | '0';        
      symboltab_ptr->symString[1] = ((_k & 0xF0) >> 4) | '0'; 
      }
    symboltab_ptr->symAddress = _i;    // Value: 0..31
    symboltab_ptr->symType = _ABSEQU;  // Type = ABS (absolute constant)
    symboltab_ptr++;
    } // end for

  // Symbols: "X","Y","Z" may be defined in Atmels *def.INC files.,
  // Using .SET to avoid "Doubly defined symbols".
  symboltab_ptr->symString[0] = 'X';   // Symbol string "X"
  symboltab_ptr->symAddress = 26;      // Value: X = 26
  symboltab_ptr->symType = _ABSSET;    // Type = ABSSET (redefinable absolute constant)
  symboltab_ptr++;

  symboltab_ptr->symString[0] = 'Y';   // Symbol string "Y"
  symboltab_ptr->symAddress = 28;      // Value: Y = 28
  symboltab_ptr->symType = _ABSSET;    // Type = ABSSET (redefinable absolute constant)
  symboltab_ptr++;

  symboltab_ptr->symString[0] = 'Z';   // Symbol string "Z"     
  symboltab_ptr->symAddress = 30;      // Value: Z = 30
  symboltab_ptr->symType = _ABSSET;    // Type = ABSSET (redefinable absolute constant)
  symboltab_ptr++;

  predefCount += 3; //  3 Symbols: "X","Y","Z"

  // Symbols: "YH","YL","YH","YL","ZH","ZL" 
  // may also be defined in Atmels *def.INC files.
  // In general programmers use X,Y,Z for shorthand, 
  // so it's superfluous to predefine these symbols.
  // Note: Unfortunately there is no strict syntax. 
  // Example (all means the same):
  //   movw ZH:ZL,YH,YL     - movw ZL,YL   - movw Z,Y 
  //   movw R31:R30,R29:R28 - movw R30,R28 
  // 
  symboltab_ptr->symString[0] = 'X';   // Symbol string "XH"
  symboltab_ptr->symString[1] = 'H';                                                   
  symboltab_ptr->symAddress = 27;      // Value: YH = 27                               
  symboltab_ptr->symType = _ABSSET;    // Type = redefinable absolute constant               
  symboltab_ptr++;                                                                     
                                                                                       
  symboltab_ptr->symString[0] = 'X';   // Symbol string "XL"                           
  symboltab_ptr->symString[1] = 'L';
  symboltab_ptr->symAddress = 26;      // Value = 26
  symboltab_ptr->symType = _ABSSET;    // Type = redefinable absolute constant
  symboltab_ptr++;

  symboltab_ptr->symString[0] = 'Y';   // Symbol string "YH"
  symboltab_ptr->symString[1] = 'H';
  symboltab_ptr->symAddress = 29;      // Value: YH = 29
  symboltab_ptr->symType = _ABSSET;    // Type = redefinable absolute constant
  symboltab_ptr++;

  symboltab_ptr->symString[0] = 'Y';   // Symbol string "YL"     
  symboltab_ptr->symString[1] = 'L';
  symboltab_ptr->symAddress = 28;      // Value = 28
  symboltab_ptr->symType = _ABSSET;    // Type = redefinable absolute constant
  symboltab_ptr++;

  symboltab_ptr->symString[0] = 'Z';   // Symbol string "ZH"     
  symboltab_ptr->symString[1] = 'H';
  symboltab_ptr->symAddress = 31;      // Value = 31
  symboltab_ptr->symType = _ABSSET;    // Type = redefinable absolute constant
  symboltab_ptr++;

  symboltab_ptr->symString[0] = 'Z';   // Symbol string "ZL"     
  symboltab_ptr->symString[1] = 'L';
  symboltab_ptr->symAddress = 30;      // Value = 30
  symboltab_ptr->symType = _ABSSET;    // Type = redefinable absolute constant
  symboltab_ptr++;

  predefCount += 6; //  6 Symbols: "XH","XL","YH","YL","ZH","ZL"
  
  // Symbol from cmdline
  if (szCmdlineSymbol[0] > SPACE)
    {
    StrCpy(symboltab_ptr->symString, szCmdlineSymbol);        
    symboltab_ptr->symAddress = 0x01;   // Value = 0x01
    symboltab_ptr->symType = _ABSSET;   // Type = redefinable absolute constant
    symboltab_ptr++;
    }

  nopAlign = 0x00;    // NOP instruction value for .ALIGN / .EVEN directives
  AtmelFlag = TRUE;   // Set Atmel syntax
  } // oper_preval


//-----------------------------------------------------------------------------
//
//                      p1eval
//
// PASS1 INSTRUCTION EVALUATION
//
void p1eval()
  {
  // Illegal Pseudo instructions (directives) - prevent a phase error !!
  if (ins_group == ERR && fopcd[0] == '.') return;
     
  // Legal Pseudo instructions (.DB .DW ...)
  if (ins_group == 1 || SegType != _CODE) return;
           
  // Groups 13, 22:  4 bytes, definitely!
  else if (ins_group == 13 || ins_group == 22 || ins_group == 25) pcc += 4;                         
  
  // Other groups (including illegal group): 2 bytes
  // At this point:
  // Inconsistency between Pass1(p1eval) and Pass2(p2eval) cause phase errors!
  else pcc += 2;
  pccw = pcc/2;                      
  } // p1eval


//------------------------------------------------------------------------------
//
//                          p2emp
//
// Check if the unused operand field is empty. Otherwise give error
//
void p2emp(char _operX[])
  {
  if (_operX[0] != 0 || lastCharAX == COMMA)
    {
    lastCharAX = 0;
    errorp2(ERR_TOOLONG, _operX);
    }
  } // p2emp

//-----------------------------------------------------------------------------
//
//                             p2eval
//
// PASS2 instruction evaluation
//
void p2eval(int _num)
  {
  if (SegType == _DATA)
    {
    errorp2(ERR_DSEG, NULL); // DSEG: Bad Segment, ESEG: Allowed ???
    clepc();
    edpri();
    return;
    }

  switch (_num)
    {
    case 10:          // One byte, no operands
      p2emp(oper1);   // Unused operands empty?
      p2emp(oper2);
      ilen = 1;
      break;
    case 20:          // Two bytes, no operands
      p2emp(oper1);   // Unused operands empty?
      p2emp(oper2);
      ilen = 2;
      break;
    case 11:          // One byte, one operand
      p2emp(oper2);
      ilen = 1;
      break;
    case 12:          // One byte, two operands
      ilen = 1;
      break;
    case 21:          // Two bytes, one operand
      p2emp(oper2);
      ilen = 2;
      break;
    case 22:          // Two bytes, two operands
      ilen = 2;
      break;
    case 31:          // Three bytes, one operand
      p2emp(oper2);
      ilen = 3;
      break;
    case 32:          // Three bytes, two operands
      ilen = 3;
      break;
    case 41:          // four bytes, one operand
      p2emp(oper2);
      ilen = 4;
      break;
    case 42:          // four bytes, two operands
      ilen = 4;
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
  errorp2(ERR_BADREG, NULL);  // Bad register
  p2eval(11);                 // Give 1 byte on error
  } // ins_error11
                      

//-----------------------------------------------------------------------------
//
//                             ins_error22
//
void ins_error22()
  {
  errorp2(ERR_BADREG, NULL);  // Bad register
  p2eval(22);                 // Give 2 bytes on error
  } // ins_error22


//-----------------------------------------------------------------------------
//
//                             ins0
//
// INSTRUCTION GROUP 0: ILLEGAL INSTRUCTION
//  2 bytes nul
//
void ins0()
  {
  errorp2(ERR_ILLOPCD, fopcd);
  p2eval(22);          // AVR illegal opcode, 2 bytes NUL
  } // ins0

//-----------------------------------------------------------------------------
//
//                            ins2
//
// INSTRUCTION GROUP 2
// two bytes (=1 word), no operands
//
void ins2()
  {
  if (StrCmpI(fopcd, "SPM") == 0 && StrCmpI(oper1, "Z+") == 0)
    p2eval(21);     // Two bytes, 1 operand (see Atmel's syntax)
  else p2eval(20);  // Two bytes, no operands
  } // ins2

//-----------------------------------------------------------------------------
//
//                            ins3
//
// INSTRUCTION GROUP 3
// (un)conditional jumps (branch relative)
//
//  {5,"BREQ",3,0xF001},   //111100kk kkkkk001  -64<=k<=+63
//  BEQ k                                       PC +=k+1
//                                              PC +=2  if condition is false
void ins3()
  {
  int op1val;

  op1val = (int)expr(oper1) - (pccw+1); // op1val: viewed as 16bit WORDs
  if (erno != 0) { errorp2(erno, oper1); p2eval(22); return; }

  else if (op1val > 63 || op1val < -64)
    errorp2(ERR_JMPADDR, oper1);

  else
    {
    insv[1] |= (op1val & 0x001F) << 3; // kkkkk°°°      
    insv[2] |= (op1val & 0x0060) >> 5; // 111100kk      
    }
  p2eval(21); // 2-byte instruction, 1 operand (2nd operand must be empty)
  } // ins3

//-----------------------------------------------------------------------------
//
//                           ins4
// INSTRUCTION GROUP 4
//                                 RR RRRRRRRR
//  {4,"MOV", 4,0x2C00},   //001011rd ddddrrrr
//  MOV Rd,Rr
//
void ins4()
  {
  UINT op1val, op2val;

  op1val = expr(oper1);
  if (erno != 0) { errorp2(erno, oper1); p2eval(22); return; }

  op2val = expr(oper2);
  if (erno != 0) { errorp2(erno, oper2); p2eval(22); return; }

  else if (op1val > 31) errorp2(ERR_BADREG, oper1);
  else if (op2val > 31) errorp2(ERR_BADREG, oper2);
     
  else
    {
    insv[1] |= (op1val & 0x0F) << 4; // dddd°°°°
    insv[1] |=  op2val & 0x0F;       // °°°°rrrr
    insv[2] |= (op1val & 0x10) >> 4; // 001011°d
    insv[2] |= (op2val & 0x10) >> 3; // 001011r°
    }
  p2eval(22);           // 2-byte instruction, 2 operands
  } // ins4

//-----------------------------------------------------------------------------
//
//                               ins5
// INSTRUCTION GROUP 5
//                                    RRRR        0<= d<= F, 0<=K<= FF
//  {4,"LDI", 5,0xE000},   //1110KKKK ddddKKKK   16<=Rd<=31, 0<=K<=255
//  LDI Rd,K
//
// Examples                  insv[2]  insv[1]
//  0AE0  ldi   r16, 10    //11100000 00001010   0x16->d=0000 0x0A=K
//  10E0  ldi   r17, 0     //11100000 00010000   0x17->d=0001 0x00=K
//  21E0  ldi   r18, 1     //11100000 00100001   0x18->d=0010 0x01=K
//  1233  cpi   r17, 50    //11100011 00010010   0x17->d=0001 0x32=K
//
//  60 e1 ldi   r22, 0x10  //11100001 01100000   0x16->d=0110 0x10=K   
//  70 e0 ldi   r23, 0x00  //11100000 01110000   0x17->d=0111 0x00=K   
//  80 e0 ldi   r24, 0x00  //11100000 10000000   0x18->d=1000 0x00=K   
//  94 e0 ldi   r25, 0x04  //11100000 10010100   0x19->d=1001 0x04=K   
//
void ins5()
  {
  UINT op1val;  
  int  op2val;  // int!! (0<=K<=255) 

  op2val = expr(oper2);
  if (erno != 0) { errorp2(erno, oper2); p2eval(22); return; }

  op1val = expr(oper1);
  if (erno != 0) { errorp2(erno, oper1); p2eval(22); return; }

  else if (op1val > 31 || op1val < 16) errorp2(ERR_BADREG, oper1);

  // Truncate <= 255 (i.e. give no error but warn if > 255)                
  // This is Atmel assembler compatible,                           
  //  but it is not nice, since it alows sloppy program code.      
  // Example:                                                      
  // LDI R16,$C34F8 --> generates no error and loads $F8 instead   
  //                                                               
  else if (op2val > 255) // && (op2val & 0xFF00) != 0xFF00) // !! (UINT)(-value)  
    {
    warno = WARN_MSKVALUE;
    warnSymbol_ptr = oper2;
    op2val &= 0x00FF;
    }

  insv[1] |= (op1val & 0x0F) << 4; // dddd°°°°
  
  // "CBR" is special
  if (StrCmpI(fopcd, "CBR") == 0) op2val ^= 0xFFFF;  
  
  insv[1] |=  op2val & 0x0F;       // °°°°KKKK
  insv[2] |= (op2val & 0xF0) >> 4; // 1110KKKK
  p2eval(22);           // 2-byte instruction, 2 operands
  } // ins5

//-----------------------------------------------------------------------------
//
//                              ins6
// INSTRUCTION GROUP 6
//                                  R RRRR
//  {4,"DEC", 6,0x940A},   //1001010d dddd1010   0<=Rd<=31
//  DEC Rd
//
void ins6()
  {
  UINT op1val;

  op1val = expr(oper1);
  if (erno != 0 || op1val > 31) errorp2(ERR_BADREG, NULL);  
  else
    {
    insv[1] |= (op1val & 0x0F) << 4; // dddd°°°°
    insv[2] |= (op1val & 0x10) >> 4; // 1001010d
    }
  p2eval(21); // 2-byte instruction, 1 operand (2nd operand must be empty)
  } // ins6

//-----------------------------------------------------------------------------
//
//                              ins7
// INSTRUCTION GROUP 7
//
//  {4,"SBI", 7,0x9A00},   //10011010 AAAAAbbb
//  CBI A,b
//
void ins7()
  {
  UINT op1val, op2val;

  op2val = expr(oper2);
  if (erno != 0) { errorp2(erno, oper2); p2eval(22); return; }

  op1val = expr(oper1);   // ..must stay here (expr() erno check below) 
  if (erno != 0) { errorp2(erno, oper1); p2eval(22); return; }

  // -------------------
  // I/O error detection
  // -------------------

  // Check predefined registers
  else if ((toupper(oper1[0]) == 'R'  ||
            toupper(oper1[0]) == 'X'  ||
            toupper(oper1[0]) == 'Y'  ||
            toupper(oper1[0]) == 'Z') &&
           op1val <=31)
    {
    errorp2(ERR_ILLOP, oper1);
    }

  // Check I/O address
  else if (op1val > 31) errorp2(ERR_SEGADDR, oper1);

  // Check Bit value
  else if (op2val > 7) errorp2(ERR_LONGCONST, oper2);

  // Syntax is correct
  else
    {
    insv[1] |= (op1val & 0x1F) << 3; // AAAAA°°°
    insv[1] |=  op2val & 0x07;       // °°°°°bbb
    }
  p2eval(22);           // 2-byte instruction, 2 operands
  } // ins7

//-----------------------------------------------------------------------------
//
//                                ins8
// INSTRUCTION GROUP 8
//                                     RRR RRR
//  {5,"FMUL",  8,0x0308}, //00000011 0ddd1rrr  16<=d<=23, 16<=r<=23
//   FMUL Rd,Rr
//
void ins8()
  {
  UINT op1val, op2val;

  op1val = expr(oper1);
  if (erno != 0) { errorp2(erno, oper1); p2eval(22); return; }

  op2val = expr(oper2);
  if (erno != 0) { errorp2(erno, oper2); p2eval(22); return; }

  else if (op1val > 23 || op1val < 16)
    errorp2(ERR_BADREG, oper1); 
  else if (op2val > 23 || op2val < 16)
    errorp2(ERR_BADREG, oper2); 

  else
    {
    insv[1] |= (op1val & 0x07) << 4; // 0ddd1°°°
    insv[1] |=  op2val & 0x07;       // 0°°°1rrr
    }
  p2eval(22);           // 2-byte instruction, 2 operands
  } // ins8

//-----------------------------------------------------------------------------
//
//                                ins9
// INSTRUCTION GROUP 9
//
// Test for Zero or Minus
//                                 RR RRRRRRRR
//  {4,"TST",9,0x2000},    //001000dd dddddddd  0<=d<=31  (see AND Rd,Rd)
//  TST Rd                           
//
void ins9()
  {
  UINT op1val;

  op1val = expr(oper1);
  if (erno != 0) { errorp2(erno, oper1); p2eval(21); return; }

  else if (op1val > 31) errorp2(ERR_BADREG, oper1); 

  else
    {
    insv[1] |= (op1val & 0x0F) << 4; // dddd°°°°
    insv[1] |=  op1val & 0x0F;       // °°°°dddd
    insv[2] |= (op1val & 0x10) >> 4; // 001000°d
    insv[2] |= (op1val & 0x10) >> 3; // 001000d°
    }
  p2eval(21); // 2-byte instruction, 1 operand (2nd operand must be empty)
  } // ins9

//-----------------------------------------------------------------------------
//
//                                ins10
// INSTRUCTION GROUP 10
//                                  R RRRR
//  {4,"XCH",10,0x9204},   //1001001d dddd0100  0<=d<=31
//  XCH Z,Rd
//
void ins10()
  {
  char _syntaxXCH[] = "Operand (Z,Rd) expected";
  UINT op2val;        

  op2val = expr(oper2);
  if (erno != 0) { errorp2(erno, oper2); p2eval(22); return; }

  else if (op2val > 31) errorp2(ERR_BADREG, oper2);

  // Demand specified syntax
  else if (StrCmpI(oper1,"Z") != 0) errorp2(ERR_SYNTAX, _syntaxXCH);

  else
    {
    insv[1] |= (op2val & 0x0F) << 4; // rrrr°°°°
    insv[2] |= (op2val & 0x10) >> 4; // 1001001°r
    }
  p2eval(22);           // 2-byte instruction, 2 operands
  } // ins10

//-----------------------------------------------------------------------------
//
//                                ins11
// INSTRUCTION GROUP 11
//                                  R RRRR
//  {5,"PUSH",11,0x920F},  //1001001r rrrr1111 0<=r<=31
//  PUSH Rr
//  POP Rr
//
void ins11()
  {
  UINT op1val;        

  op1val = expr(oper1);
  if (erno != 0) { errorp2(erno, oper1); p2eval(21); return; }

  else if (op1val > 31) errorp2(ERR_BADREG, oper1);

  else
    {
    insv[1] |= (op1val & 0x0F) << 4; // rrrr°°°°
    insv[2] |= (op1val & 0x10) >> 4; // 1001001°r
    }
  p2eval(21); // 2-byte instruction, 1 operand (2nd operand must be empty)
  } // ins11

//-----------------------------------------------------------------------------
//
//                                ins12
// INSTRUCTION GROUP 12 - Add Immediate to Word
//                      - Subtract Immediate from Word
//                         
// This instruction is not available in all devices.
//
//                                      RR        18,1A,1C,1E
//  {5,"ADIW",12,0x9600},  //10010110 KKddKKKK  d{24,26,28,30}, 0<=K<=63
//  ADIW Rd+1:Rd,K         
//
//  Add K to the Z-pointer(R31:R30)
//  ADIW ZH:ZL,K
//                                      RR
//  {5,"SBIW",12,0x9700},  //10010111 KKddKKKK  d{24,26,28,30}, 0<=K<=63
//  SBIW Rd+1:Rd,K         
//
// Examples:
//  adiw r25:24,1 ; Add 1 to r25:r24
//  adiw ZH:ZL,63 ; Add 63 to the Z-pointer(r31:r30)
//  sbiw r25:24,1 ; Subtract 1 from r25:r24
//  sbiw YH:YL,63 ; Subtract 63 from the Y-pointer(r29:r28)
//
//  e.g. Wrong Syntax: sbwi r24,1  // Other assemblers would allow this, why?
//       (ambiguity)   adiw x,1    // We should demand the specified syntax!
//
void ins12()
  {
  char _syntaxIWXYZ[] = "Operand (rH:rL,k) expected"; // [] = to be patched
  char* _colonPtr1 = NULL;

  UINT op1val, op2val, op3val;

  // Evaluate operands
  op1val = expr(oper1);
  if (erno != 0) { errorp2(erno, oper1); p2eval(22); return; }
  op2val = expr(oper2);
  if (erno != 0) { errorp2(erno, oper2); p2eval(22); return; }

  if (oper3[0] == 0) op3val = 0;  // oper3 may be NULL
  else
    {
    op3val = expr(oper3);
    if (erno != 0) { errorp2(erno, oper3); p2eval(22); return; }
    }

  // Extensive syntax check ':'
  _colonPtr1 = strstr(inbuf, fopcd);
  _colonPtr1 = strstr(_colonPtr1, ":");

  // 1) syntax check: adiw x,y        = wrong syntax
  //                  adiw xh:xl, k   = expected
  //                  adiw r24,r25,63 = wrong syntax
  //                  adiw r24,63     = wrong syntax
  //                  adiw r24:r25,k  = expected
  if ((swmodel & _SYNTAX) && (oper3[0] == 0 || _colonPtr1 == NULL))
    {
    switch(toupper(oper1[0]))
      {
      case 'X':
      case 'Y':
      case 'Z':
        _syntaxIWXYZ[9]   = toupper(oper1[0]);
        _syntaxIWXYZ[9+3] = toupper(oper1[0]);
        errSymbol_ptr = _syntaxIWXYZ;
        break;
      default:
        errSymbol_ptr = _syntaxIW;
        break;
      } // end switch
    errorp2(ERR_SYNTAX, errSymbol_ptr);
    p2eval(22);         // 2-byte instruction, 2 special operands
    return;             // Demand the specified syntax
    } // end if

  // Allow wrong (simplified) syntax: adiw r28,63
  //                                  adiw x,y
  else if (!(swmodel & _SYNTAX) && oper3[0] == 0 && _colonPtr1 == NULL)
    {
    if (op1val >30 || op1val <24 || (op1val %2) != 0) errorp2(ERR_SYNTAX, oper1);

    else if (op2val >63) errorp2(ERR_LONGCONST, oper2);

    else
      {
      if (toupper(fopcd[0]) == 'A')      // "ADIW"
        {
        insv[1] |= (op1val & 0x06) << 3;       
        insv[1] |= op2val & 0x0F;
        insv[1] |= (op2val & 0x30) << 2;       
        }
      else if (toupper(fopcd[0]) == 'S')  // "SBIW"                
        {                                                  
        insv[1] |= (op1val & 0x06) << 3;       
        insv[1] |= op2val & 0x0F;
        insv[1] |= (op2val & 0x30) << 2;       
        insv[2] |= 0x01;                                   
        }
      } // end else
    } // end if

  // Check correct syntax: adiw r28:R29,63
  //                       adiw XH:XL,k
  else if (oper3[0] != 0 && _colonPtr1 != NULL)
    {
    if (op1val >31 || op1val <25 || (op1val %2) != 1) errorp2(ERR_SYNTAX, _syntaxIW);

    else if (op2val >30 || op2val <24 || (op2val %2) != 0 || (op1val-1) != op2val)
      errorp2(ERR_SYNTAX, _syntaxIW);

    else if (op3val >63) errorp2(ERR_LONGCONST, oper3);

    else
      {
      if (toupper(fopcd[0]) == 'A')       // "ADIW"
        {
        insv[1] |= (op2val & 0x06) << 3;       
        insv[1] |= op3val & 0x0F;
        insv[1] |= (op3val & 0x30) << 2;       
        }
      else if (toupper(fopcd[0]) == 'S')  // "SBIW"                
        {                                                  
        insv[1] |= (op2val & 0x06) << 3;       
        insv[1] |= op3val & 0x0F;
        insv[1] |= (op3val & 0x30) << 2;       
        insv[2] |= 0x01;                                   
        }
      } // end else
    } // end else if
  
  else errorp2(ERR_SYNTAX, NULL);        // no info

  p2eval(22);             // 2-byte instruction, 2 (special) operands
  } // ins12

//-----------------------------------------------------------------------------
//
//                                ins13
// INSTRUCTION GROUP 13
//                                R RRRR
//  {4,"LDS",13,0x9000}, //1001000d dddd0000 kkkkkkkk kkkkkkkk  0<=d<=31, 0<=k<=65535
//  LDS Rd,k
//
void ins13()
  {
  UINT op1val, op2val;
                                            
  op1val = expr(oper1);
  if (erno != 0) { errorp2(erno, oper1); p2eval(42); return; }

  op2val = expr(oper2); 
  if (erno != 0) { errorp2(erno, NULL); p2eval(42); return; }

  // Note: lvalue holds 32 bit result of expr in oper2
  else if (lvalue > 0xFFFF) // lvalue = op2val promoted to 32bit   
    {
    sprintf(_syntaxBuf, "0x%08X", lvalue);
    errorp2(ERR_SEGADDR, _syntaxBuf);
    }

  // Check if 2nd operand is a register, instead of a symbol/label
  else if ((strlen(oper2) <= 3) && ((toupper(oper2[0]) == 'R' && expr(&oper2[1]) <= 31) ||
           (strlen(oper2) == 1) && 
           (toupper(oper2[0]) == 'X'  ||
            toupper(oper2[0]) == 'Y'  ||
            toupper(oper2[0]) == 'Z'))
          )
    {
    errorp2(ERR_ILLOP, oper2);
    }

  else if (op1val > 31) errorp2(ERR_BADREG, oper1);                            

  else
    {
    insv[1] |= (op1val & 0x0F) << 4;   // dddd°°°°
    insv[2] |= (op1val & 0x10) >> 4;   // 1001000d
    insv[3]  =  op2val & 0x00FF;       // kkkkkkkk
    insv[4]  = (op2val & 0xFF00) >> 8; // kkkkkkkk
    }
  p2eval(42); // 4-byte instruction, 2 operands
  } // ins13

//-----------------------------------------------------------------------------
//
//                                ins14
//
// INSTRUCTION GROUP 14 - Relative Jump
//
//  {5,"RJMP", 14,0xC000}, //1100kkkk kkkkkkkk  -2K<=k<2K  (PC += k+2)
//  RJMP k
//
void ins14()
  {
  int op1val;

  op1val = (int)expr(oper1) - (pccw+1); // op1val: viewed as 16bit WORDs
  if (erno != 0) { errorp2(erno, oper1); p2eval(21); return; }

  else if (op1val > 2047 || op1val < -2048) errorp2(ERR_JMPADDR, NULL);
  else
    {
    insv[1] =   op1val & 0x00FF;        // kkkkkkkk
    insv[2] |= (op1val & 0x0F00) >> 8;  // °°°°kkkk
    }
  p2eval(21); // 2-byte instruction, 1 operand (2nd operand must be empty)
  } // ins14


//-----------------------------------------------------------------------------
//
//                                ins15
//
// INSTRUCTION GROUP 15 - Branch if Bit in SREG is Set
//
//                           insv[2]  insv[1]
//  {5,"BRBS",15,0xF000},  //111100kk kkkkksss 0<=s<=7, -64<=k<=+63
//  BRBS s,k                                   PC +=k+1
//                                             PC +=2 if condition is false
void ins15()
  {
  UINT op1val;
  int op2val;
                                          
  op1val = expr(oper1);
  if (erno != 0) { errorp2(erno, oper1); p2eval(22); return; }

  op2val = (int)expr(oper2) - (pccw+1); // op2val: viewed as 16bit WORDs
  if (erno != 0) { errorp2(erno, oper2); p2eval(22); return; }

  else if (op1val > 7)  errorp2(ERR_ILLEXPR, oper1);

  else if (op2val > 63 || op2val < -64) errorp2(ERR_JMPADDR, oper2);

  else
    {
    insv[1] |=  op1val & 0x0007;       // °°°°°sss
    insv[1] |= (op2val & 0x001F) << 3; // kkkkk°°°     
    insv[2] |= (op2val & 0x0060) >> 5; // 111100kk     
    }
  p2eval(22);           // 2-byte instruction, 2 operands
  } // ins15

//-----------------------------------------------------------------------------
//
//                                ins16
// INSTRUCTION GROUP 16
//                                  R RRRR
//  {4,"BLD",16,0xF800},   //1111100d dddd0bbb 0<=d<=31, 0<=b<=7
//  BLD Rd,b
//
void ins16()
  {
  UINT op1val, op2val;

  op1val = expr(oper1);
  if (erno != 0) { errorp2(erno, oper1); p2eval(22); return; }

  op2val = expr(oper2);
  if (erno != 0) { errorp2(erno, oper2); p2eval(22); return; }

  else if (op1val > 31) errorp2(ERR_BADREG, oper1);

  else if (op2val > 7) errorp2(ERR_ILLOP, oper2);

  else
    {
    insv[1] |= (op2val & 0x0007);      // °°°°°bbb
    insv[1] |= (op1val & 0x000F) << 4; // dddd°°°°
    insv[2] |= (op1val & 0x0010) >> 4; // °°°°°°°d
    }
  p2eval(22);           // 2-byte instruction, 2 operands
  } // ins16

//-----------------------------------------------------------------------------
//
//                                ins17
// INSTRUCTION GROUP 17
//
void ins17()
  {
  } // ins17

//-----------------------------------------------------------------------------
//
//                                ins18
// INSTRUCTION GROUP 18
//
//  {5,"BSET",18,0x9408},  //10010100 0sss1000 0<=s<=7
//  BSET s
//
void ins18()
  {
  UINT op1val;

  op1val = expr(oper1);
  if (erno != 0) { errorp2(erno, oper1); p2eval(21); return; }

  else if (op1val > 7) errorp2(ERR_ILLOP, oper1);

  else insv[1] |= (op1val & 0x0007) << 4; // °sss°°°°

  p2eval(21); // 2-byte instruction, 1 operand (2nd operand must be empty)
  } // ins18

//-----------------------------------------------------------------------------
//
//                                ins19
// INSTRUCTION GROUP 19
//                                    RRRR
//  {4,"SER", 19,0xEF0F},  //11101111 dddd1111 16<=d<=31
//  SER rd
//
void ins19()
  {
  UINT op1val;

  op1val = expr(oper1);
  if (erno != 0) { errorp2(erno, oper1); p2eval(21); return; }

  else if (op1val < 16 || op1val > 31) errorp2(ERR_BADREG, oper1);
 
  else insv[1] |= (op1val & 0x000F) << 4; // dddd°°°°

  p2eval(21); // 2-byte instruction, 1 operand (2nd operand must be empty)
  } // ins19

//-----------------------------------------------------------------------------
//
//                                ins20
// INSTRUCTION GROUP 20
//                                    RRRRRRRR
//  {5,"MULS",20,0x0200},  //00000010 ddddrrrr 16<=d<=31, 16<=r<=31
//  MULS Rd,Rr
//
void ins20()
  {
  UINT op1val, op2val;

  op1val = expr(oper1);
  if (erno != 0) { errorp2(erno, oper1); p2eval(22); return; }

  op2val = expr(oper2);
  if (erno != 0) { errorp2(erno, oper2); p2eval(22); return; }

  else if (op1val <16 || op1val > 31) errorp2(ERR_BADREG, oper1);
  else if (op2val <16 || op2val > 31) errorp2(ERR_BADREG, oper2);

  else
    {
    insv[1] |= (op1val & 0x000F) << 4; // dddd°°°°
    insv[1] |=  op2val & 0x000F;       // rrrr°°°°
    }
  p2eval(22);           // 2-byte instruction, 2 operands
  } // ins20

//-----------------------------------------------------------------------------
//
//                                ins21
// INSTRUCTION GROUP 21
//
// An instruction set extension to the AVR CPU, performing DES iterations.
// The 64-bit data block (plaintext or ciphertext) is placed in the CPU register
// file, registers R0-R7, where LSB of data is placed in LSB of R0
// and MSB of data is placed in MSB of R7.
// The full 64-bit key (including parity bits) is placed in registers R8-R15,
// organized in the register file with LSB of key in LSB of R8
// and MSB of key in MSB of R15. Executing one DES instruction performs
// one round in the DES algorithm. Sixteen rounds must be executed in increasing
// order to form the correct DES ciphertext or plaintext.
// Intermediate results are stored in the register file (R0-R15)
// after each DES instruction.
// The instruction's operand (K) determines which round is executed,
// and the half carry flag (H) determines whether encryption or decryption
// is performed. The DES algorithm is described in
// "Specifications for the Data Encryption Standard"
// (Federal Information Processing Standards Publication 46).
// Intermediate results in this implementation differ from the standard,
// because the initial permutation and the inverse initial permutation
// are performed each iteration. This does not affect the result
// in the final ciphertext or plaintext, but reduces execution time.
//
// Operation:
//  If H = 0 then Encrypt round (R7-R0, R15-R8, K)
//  If H = 1 then Decrypt round (R7-R0, R15-R8, K)
//
//  {4,"DES",21,0x940B},   //10010100 KKKK1011  0<=K<=15
//  DES K
//
void ins21()
  {
  UINT op1val;

  op1val = expr(oper1);
  if (erno != 0) { errorp2(erno, oper1); p2eval(21); return; }

  else if (op1val > 15) errorp2(ERR_LONGCONST, oper1);

  else insv[1] |= (op1val & 0x000F) << 4; // KKKK°°°°

  p2eval(21); // 2-byte instruction, 1 operand (2nd operand must be empty)
  } // ins21

//-----------------------------------------------------------------------------
//
//                                ins22
// INSTRUCTION GROUP 22
//
// Jump to an address within the entire 4M (words) program memory.
//
//  {5,"CALL",22,0x940E},  //1001010k kkkk111k kkkkkkkk kkkkkkkk 0<=k<=4M
//  CALL k                 //kkkkkkkk kkkkkkkk kkkk111k 1001010k
//  JMP  k                 //kkkkkkkk kkkkkkkk kkkk110k 1001010k
//
void ins22()
  {
  ULONG op1val;
  UINT op2val;
                                            
  op1val = expr(oper1);
  op2val = (op1val & 0x00FF0000) >>16;
  if (erno != 0) { errorp2(erno, oper1); p2eval(41); return; }
  
  // Note: 4M (words) = 8Mbyte  (0x400000) wraps to 0
  else if (op1val >= (4*1024*1024)) 
    {
    // Show upper limit in words
    if (swmodel & _WORD) sprintf(_syntaxBuf, "0x%08X", op1val);
    // Show upper limit in bytes
    else if (swmodel & _BYTE) sprintf(_syntaxBuf, "0x%08X", op1val*2);
    errorp2(ERR_JMPADDR, _syntaxBuf);
    }

  else
    {
    insv[3]  =  op1val & 0x000000FF;       // kkkkkkkk
    insv[4]  = (op1val & 0x0000FF00) >> 8; // kkkkkkkk

    insv[1] |=  op2val & 0x0001;           // °°°°°°°k
    insv[1] |= (op2val & 0x001E) << 3;     // kkkk°°°°
    insv[2] |= (op2val & 0x0020) >> 5;     // °°°°°°°k
    }                                          
  p2eval(41);  // 4-byte instruction, 1 operand (2nd operand must be empty)
  } // ins22

//-----------------------------------------------------------------------------
//
//                                ins23
// INSTRUCTION GROUP 23 
//
void ins23()
  {             // (reserved)
  } // ins23

//-----------------------------------------------------------------------------
//
//                                ins24
// INSTRUCTION GROUP 24
// Copy Register Word 
//                                    RRRRRRRR
//  {5,"MOVW",24,0x0100},  //00000001 ddddrrrr  d={0,2,..,30}. r={0,2,..,30}
//  MOVW Rd+1:Rd, Rr+1:Rr
//
//  Example:   
//  MOVW R17:R16, R1:R0   ; Copy r1:r0 to r17:r16
//  MOVW XH:XL, YH:YL
//  e.g. Wrong Syntax: movw r0,r16  // Other assemblers would allow this, why?
//                     movw x,y     // We should demand the specified syntax!
//
// To force the correct syntax place the directive '.MODEL SYNTAX' at the 
// head of the source file
// 
void ins24()
  {
  char _syntaxMOVWXYZ[] = "Operand (rH:rL,rH:rL) expected";

  UINT op1val, op2val, op3val, op4val;
  char* _colonPtr1 = NULL;
  char* _colonPtr2 = NULL;

  op1val = expr(oper1);
  if (erno != 0) { errorp2(erno, oper1); p2eval(22); return; }
  op2val = expr(oper2);
  if (erno != 0) { errorp2(erno, oper2); p2eval(22); return; }

  if (oper3[0] == 0) op3val = 0; // oper3 may be zero
  else
    {
    op3val = expr(oper3);
    if (erno != 0) { errorp2(erno, oper3); p2eval(22); return; }
    }
  if (oper4[0] == 0) op4val = 0; // oper4 may be zero
  else
    {
    op4val = expr(oper4);
    if (erno != 0) { errorp2(erno, oper4); p2eval(22); return; }
    }

  // Extensive syntax check ':'
  _colonPtr1 = strstr(inbuf, ":");
  if (_colonPtr1 != NULL) _colonPtr2 = strstr(++_colonPtr1, ":");

  // 1) syntax check: movw x,y          = wrong syntax
  //                  movw xh:xl, yh,yl = expected
  if ((swmodel & _SYNTAX) && oper3[0] == 0 && oper4[0] == 0)
    {
    switch(toupper(oper1[0]))
      {
      case 'X':
      case 'Y':
      case 'Z':
        _syntaxMOVWXYZ[9]    = toupper(oper1[0]);
        _syntaxMOVWXYZ[9+3]  = toupper(oper1[0]);
        errSymbol_ptr = _syntaxMOVWXYZ;
        break;
      default:
        errSymbol_ptr = _syntaxMOVW;
        break;
      } // end switch

    switch(toupper(oper2[0]))
      {
      case 'X':
      case 'Y':
      case 'Z':
        _syntaxMOVWXYZ[15]   = toupper(oper2[0]);
        _syntaxMOVWXYZ[15+3] = toupper(oper2[0]);
        errSymbol_ptr = _syntaxMOVWXYZ;
        break;
      default:
        if (errSymbol_ptr != _syntaxMOVWXYZ)
        errSymbol_ptr = _syntaxMOVW;
        break;
      } // end switch

    // Demand specified syntax
    errorp2(ERR_SYNTAX, errSymbol_ptr);
    p2eval(22);         // 2-byte instruction, 2 special operands
    return;             // Demand specified syntax
    } // end if

  // 2) syntax check: movw r2,r1,r4,r3 = wrong syntax (no ':')
  //                  movw r2:r1,r4,r3 = wrong syntax
  //                  movw r2,r1,r4:r3 = wrong syntax
  //                  movw r2:r1,r4:r3 = expected
  else if ((swmodel & _SYNTAX) && (_colonPtr1 == NULL || _colonPtr2 == NULL))
    {
    errorp2(ERR_SYNTAX, _syntaxMOVW);
    p2eval(22);         // 2-byte instruction, 2 special operands
    return;             // Demand specified syntax
    } // end else if
  
  // 3) Allow simplified syntax movw x,y
  //                            movw r2,r4
  if ((op1val %2) == 0 && op1val <= 30 &&
      (op2val %2) == 0 && op2val <= 30 &&
      oper3[0] == 0 && oper4[0] == 0)
    {
    insv[1] = ((op1val/2) << 4) | (op2val/2);
    } // end if
  
  // Check correct syntax
  else if ((op1val %2) != 1 || op1val > 31 ||
           (op2val %2) != 0 || op2val > 30 || 
           (op3val %2) != 1 || op3val > 31 ||
           (op4val %2) != 0 || op4val > 30 ||
           (op1val-1) != op2val || (op3val-1) != op4val ||
           _colonPtr1 == NULL || _colonPtr2 == NULL)
    {
    errorp2(ERR_SYNTAX, _syntaxMOVW);
    } // end else if

  else insv[1] = ((op2val/2) << 4) | (op4val/2);
  p2eval(22);           // 2-byte instruction, 2 special operands
  } // ins24

//-----------------------------------------------------------------------------
//
//                                ins25
// INSTRUCTION GROUP 25
//
//  {4,"STS",25,0x9200}, //1001001r rrrr0000 kkkkkkkk kkkkkkkk  0<=r<=31, 0<=k<=65535
//  STS k,Rr
//
void ins25()
  {
  UINT op1val, op2val;
                                            
  op2val = expr(oper2);
  if (erno != 0) { errorp2(erno, oper2); p2eval(42); return; }

  op1val = expr(oper1);
  if (erno != 0) { errorp2(erno, oper1); p2eval(42); return; }

  // Note: lvalue holds 32 bit result of expr in oper1
  else if (lvalue > 0xFFFF)  
    {
    sprintf(_syntaxBuf, "0x%08X", lvalue);
    errorp2(ERR_SEGADDR, _syntaxBuf);
    }

  else if (op2val > 31) errorp2(ERR_BADREG, oper2);
  else
    {
    insv[1] |= (op2val & 0x0F) << 4;   // rrrr°°°°
    insv[2] |= (op2val & 0x10) >> 4;   // 1001001r
    insv[3]  =  op1val & 0x00FF;       // kkkkkkkk
    insv[4]  = (op1val & 0xFF00) >> 8; // kkkkkkkk
    }
  p2eval(42); // 4-byte instruction, 2 operands
  } // ins25

//-----------------------------------------------------------------------------
//
//                                ins26
// INSTRUCTION GROUP 26
//                                  R RRRR
//  {3,"IN",26,0xB000},    //10110AAd ddddAAAA 0<=d<=31, 0<=A<=63
//  IN Rd,A
//
void ins26()
  {
  UINT op1val, op2val;

  op2val = expr(oper2);
  if (erno != 0) { errorp2(erno, oper2); p2eval(22); return; }

  op1val = expr(oper1);
  if (erno != 0) { errorp2(erno, oper1); p2eval(22); return; }

  else if (op1val > 31) errorp2(ERR_BADREG, oper1);
  else if (op2val > 63) errorp2(ERR_SEGADDR, oper2);
  else
    {
    insv[1] |=  op2val & 0x000F;       // °°°°AAAA
    insv[1] |= (op1val & 0x000F) << 4; // dddd°°°°
    insv[2] |= (op1val & 0x0010) >> 4; // °°°°°°°d
    insv[2] |= (op2val & 0x0030) >> 3; // °°°°°AA°
    }
  p2eval(22);           // 2-byte instruction, 2 operands
  } // ins26

//-----------------------------------------------------------------------------
//
//                                ins27
// INSTRUCTION GROUP 27
//                                  R RRRR
//  {4,"OUT",27,0xB800},   //10111AAr rrrrAAAA 0<=r<=31, 0<=A<=63
//  OUT A,Rr
//
void ins27()
  {
  UINT op1val, op2val;

  op1val = expr(oper1);
  if (erno != 0) { errorp2(erno, oper1); p2eval(22); return; }

  op2val = expr(oper2);
  if (erno != 0) { errorp2(erno, oper2); p2eval(22); return; }

  else if (op2val > 31) errorp2(ERR_BADREG, oper2);
  else if (op1val > 63) errorp2(ERR_SEGADDR, oper1);
  else
    {
    insv[1] |=  op1val & 0x000F;       // °°°°AAAA
    insv[1] |= (op2val & 0x000F) << 4; // dddd°°°°
    insv[2] |= (op2val & 0x0010) >> 4; // °°°°°°°d
    insv[2] |= (op1val & 0x0030) >> 3; // °°°°°AA°
    }
  p2eval(22);           // 2-byte instruction, 2 operands
  } // ins27

//-----------------------------------------------------------------------------
//
//                                ins28
// INSTRUCTION GROUP 28
//                         
//  {3,"LD",28,0x900C}
//
// The result of these combinations is undefined:
//  LD r26, X+
//  LD r27, X+
//  LD r26, -X
//  LD r27, -X
//
// The result of these combinations is undefined:
//  LD r28, Y+
//  LD r29, Y+
//  LD r28, -Y
//  LD r29, -Y
//
// The result of these combinations is undefined:
//  LD r30, Z+
//  LD r31, Z+
//  LD r30, -Z
//  LD r31, -Z
//                      R RRRR             
//  LD Rd, X   //1001000d dddd1100   (i)     (LD Rd,X)
//  LD Rd,X+   //1001000d dddd1101   (ii)    (LD Rd,X+)
//  LD Rd,-X   //1001000d dddd1110   (iii)   (LD Rd,-X)
//
//  LD Rd, Y   //1000000d dddd1000   (i)     (LD Rd,Y)
//  LD Rd,Y+   //1001000d dddd1001   (ii)    (LD Rd,Y+)
//  LD Rd,-Y   //1001000d dddd1010   (iii)   (LD Rd,-Y)
//
//  LD Rd, Z   //1000000d dddd0000   (i)     (LD Rd,Z)
//  LD Rd,Z+   //1001000d dddd0001   (ii)    (LD Rd,Z+)
//  LD Rd,-Z   //1001000d dddd0010   (iii)   (LD Rd,-Z)
//
void ins28()
  {
  UINT op1val;

  op1val = expr(oper1);
  if (erno != 0) { errorp2(erno, oper1); p2eval(22); return; }

  else if (op1val > 31) errorp2(ERR_BADREG, oper1);
  else
    {
    insv[1] |= (op1val & 0x000F) << 4;  // dddd°°°°
    insv[2] |= (op1val & 0x0010) >> 4;  // °°°°°°°d
    }

  if (StrCmpI(oper2, "X") == 0)
    {
    insv[1] |= 0x0C;                                     // °°°°1100  "X"
    insv[2] |= 0x10;                                     // °°°1°°°°  "X+"
    }

  else if (StrCmpI(oper2, "X+") == 0)
    {
    insv[1] |= 0x0D;                                     // °°°°1101  "X+"
    insv[2] |= 0x10;                                     // °°°1°°°°  "X+"
//ha//    if (op1val == 26 || op1val == 27) errorp2(ERR_BADREG, NULL);
    }
  else if (StrCmpI(oper2, "-X") == 0)
    {
    insv[1] |= 0x0E;                                     // °°°°1000  "-X"
    insv[2] |= 0x10;                                     // °°°1°°°°  "X+"
//ha//    if (op1val == 26 || op1val == 27) errorp2(ERR_BADREG, NULL);
    }

  else if (StrCmpI(oper2, "Y") == 0)
    insv[1] |= 0x08;                                     // °°°°1100  "Y"

  else if (StrCmpI(oper2, "Y+") == 0)
    {
    insv[1] |= 0x09;                                     // °°°°1001  "Y+"
    insv[2] |= 0x10;                                     // °°°1°°°°  "Y+"
//ha//    if (op1val == 28 || op1val == 29) errorp2(ERR_BADREG, NULL);
    }
  else if (StrCmpI(oper2, "-Y") == 0)
    {
    insv[1] |= 0x0A;                                     // °°°°1010  "-Y"
    insv[2] |= 0x10;                                     // °°°1°°°°  "-Y"
//ha//    if (op1val == 28 || op1val == 29) errorp2(ERR_BADREG, NULL);
    }

  else if (StrCmpI(oper2, "Z") == 0)
    insv[1] |= 0x00;                                     // °°°°0000  "Z"

  else if (StrCmpI(oper2, "Z+") == 0)
    {
    insv[1] |= 0x01;                                     // °°°°0001  "Z+"
    insv[2] |= 0x10;                                     // °°°1°°°°  "Z+"
//ha//    if (op1val == 30 || op1val == 31) errorp2(ERR_BADREG, NULL);
    }
  else if (StrCmpI(oper2, "-Z") == 0)
    {
    insv[1] |= 0x02;                                     // °°°°0010  "-Z"
    insv[2] |= 0x10;                                     // °°°1°°°°  "-Z"
//ha//    if (op1val == 30 || op1val == 31) errorp2(ERR_BADREG, NULL);
    }

  else errorp2(ERR_ILLOP, oper2);

  p2eval(22);               // 2-byte instruction, 2 operands
  } // ins28

//-----------------------------------------------------------------------------
//
//                                ins29
// INSTRUCTION GROUP 29
//
//  {4,"LDD",29,0x9000}
//                               R RRRR
//  LDD Rr,Y+q          //10q0qq0d dddd1qqq  (iv)
//  LDD Rr,Z+q          //10q0qq0d dddd0qqq  (iv)
//
void ins29()
  {
  UINT op1val, op2val;
  
  // Eliminate all white spaces in oper1
  // Allow something like "Z + N_TEST" (correct syntax is "Z+N_TEST, R..") 
  remove_white_spaces(oper2);

  op2val = expr(&oper2[2]);
  if (erno != 0) { errorp2(erno, &oper2[2]); p2eval(22); return; }

  op1val = expr(oper1);
  if (erno != 0) { errorp2(erno, oper1); p2eval(22); return; }
  
  else if (op1val >31) errorp2(ERR_BADREG, oper1);

  else if (toupper(oper2[0]) == 'Y' && toupper(oper2[1]) == '+')
    {
    if (op2val > 63)
      {
      sprintf(_syntaxBuf, "0x%04X", op2val);
      errorp2(ERR_LONGCONST, _syntaxBuf);
      }
    else insv[1] |= 0x08;  // °°°°1°°°  "Y"
    } // end if (Y)
                                         
  else if (toupper(oper2[0]) == 'Z' && toupper(oper2[1]) == '+')
    {
    if (op2val > 63)
      {
      sprintf(_syntaxBuf, "0x%04X", op2val);
      errorp2(ERR_LONGCONST, _syntaxBuf);
      }
    else insv[1] |= 0x00;  // °°°°0°°°  "Z"
    } // end if (Z)

  else errorp2(ERR_ILLOP, oper2);                                      
                                                         
  if (op1val <= 31)
    {
    insv[1] |= (op1val & 0x000F) << 4;  // dddd°°°°
    insv[2] |= (op1val & 0x0010) >> 4;  // °°°°°°°d
    insv[1] |=  op2val & 0x0007;        // °°°°°qqq
    insv[2] |= (op2val & 0x0018) >> 1;  // °°°°qq°°
    insv[2] |=  op2val & 0x0020;        // °°q°°°°°
    }

  else errorp2(ERR_BADREG, oper1);

  p2eval(22);           // 2-byte instruction, 2 operands
  } // ins29

//-----------------------------------------------------------------------------
//
//                                ins30
// INSTRUCTION GROUP 30
//
//  {3,"ST",30,0x92C0}
//
// The result of these combinations is undefined: No warning - Caveat programmer!
//  ST X+, r26
//  ST X+, r27
//  ST -X, r26
//  ST -X, r27
//
// The result of these combinations is undefined: No warning - Caveat programmer!
//  ST Y+, r28
//  ST Y+, r29
//  ST -Y, r28
//  ST -Y, r29
//
// The result of these combinations is undefined: No warning - Caveat programmer!
//  ST Z+, r30
//  ST Z+, r31
//  ST -Z, r30
//  ST -Z, r31
//                        R RRRR
//  ST X ,Rr     //1001001r rrrr1100   (i)     (ST X,Rr)   
//  ST X+,Rr     //1001001r rrrr1101   (ii)    (ST X+,Rr)
//  ST -X,Rr     //1001001r rrrr1110   (iii)   (ST -X,Rr)
//
//  ST Y ,Rr     //1000001r rrrr1000   (i)     (ST Y,Rr)   
//  ST Y+,Rr     //1001001r rrrr1001   (ii)    (ST Y+,Rr)
//  ST -Y,Rr     //1001001r rrrr1010   (iii)   (ST -Y,Rr)
//
//  ST Z ,Rr     //1000001r rrrr0000   (i)     (ST Z,Rr)   
//  ST Z+,Rr     //1001001r rrrr0001   (ii)    (ST Z+,Rr)
//  ST -Z,Rr     //1001001r rrrr0010   (iii)   (ST -Z,Rr)
//
void ins30()
  {
  UINT op2val;

  // Evaluate 2nd operand
  op2val = expr(oper2);
  if (erno != 0) { errorp2(erno, oper2); p2eval(22); return; }

  else if (op2val > 31) errorp2(ERR_BADREG, oper2);

  else
    {
    insv[1] |= (op2val & 0x000F) << 4;  // dddd°°°°
    insv[2] |= (op2val & 0x0010) >> 4;  // °°°°°°°d
    }

  // Evaluate 1st operand
  if (StrCmpI(oper1, "X") == 0)
    {
    insv[1] |= 0x0C;                                     // °°°°1100  "X"
    insv[2] |= 0x10;                                     // °°°1°°°°  "X"
    }

  else if (StrCmpI(oper1, "X+") == 0)
    {
    insv[1] |= 0x0D;                                     // °°°°1101  "X+"
    insv[2] |= 0x10;                                     // °°°1°°°°  "X+"
//ha//    if (op2val == 26 || op2val == 27) errorp2(ERR_BADREG, NULL);  // undefined
    }
  else if (StrCmpI(oper1, "-X") == 0)
    {
    insv[1] |= 0x0E;                                     // °°°°1110  "-X"
    insv[2] |= 0x10;                                     // °°°1°°°°  "-X"
//ha//    if (op2val == 26 || op2val == 27) errorp2(ERR_BADREG, NULL);  // undefined
    }

  else if (StrCmpI(oper1, "Y") == 0)
    insv[1] |= 0x08;                                     // °°°°1000  "Y"

  else if (StrCmpI(oper1, "Y+") == 0)
    {
    insv[1] |= 0x09;                                     // °°°°1001  "Y+"
    insv[2] |= 0x10;                                     // °°°1°°°°  "Y+"
//ha//    if (op2val == 28 || op2val == 29) errorp2(ERR_BADREG, NULL);  // undefined
    }
  else if (StrCmpI(oper1, "-Y") == 0)
    {
    insv[1] |= 0x0A;                                     // °°°°1010  "-Y"
    insv[2] |= 0x10;                                     // °°°1°°°°  "-Y"
//ha//    if (op2val == 28 || op2val == 29) errorp2(ERR_BADREG, NULL);  // undefined
    }

  else if (StrCmpI(oper1, "Z") == 0)
    insv[1] |= 0x00;                                     // °°°°0000  "Z"

  else if (StrCmpI(oper1, "Z+") == 0)
    {
    insv[1] |= 0x01;                                     // °°°°0001  "Z+"
    insv[2] |= 0x10;                                     // °°°1°°°°  "Z+"
//ha//    if (op2val == 30 || op2val == 31) errorp2(ERR_BADREG, NULL);
    }
  else if (StrCmpI(oper1, "-Z") == 0)
    {
    insv[1] |= 0x02;                                     // °°°°0010  "-Z"
    insv[2] |= 0x10;                                     // °°°1°°°°  "-Z"
//ha//    if (op2val == 30 || op2val == 31) errorp2(ERR_BADREG, NULL);
    }

  else errorp2(ERR_ILLOP, oper1);

  p2eval(22);               // 2-byte instruction, 2 operands
  } // ins30

//-----------------------------------------------------------------------------
//
//                                ins31
// INSTRUCTION GROUP 30
//
//  {4,"STD",31,0x9200}
//                               R RRRR
//  STD Y+q, Rr         //10q0qq1r rrrr1qqq  (iv)
//  STD Z+q, Rr         //10q0qq1r rrrr0qqq  (iv)
//
void ins31()
  {
  UINT op1val, op2val;

  // Eliminate all white spaces in oper1
  // Allow something like "Z + N_TEST" (correct syntax is "Z+N_TEST, R..") 
  remove_white_spaces(oper1);

  op1val = expr(&oper1[2]);
  if (erno != 0) { errorp2(erno, &oper1[2]); p2eval(22); return; }

  op2val = expr(oper2);
  if (erno != 0) { errorp2(erno, oper2); p2eval(22); return; }

  else if (op2val > 31) errorp2(ERR_BADREG, oper2);

  else if (toupper(oper1[0]) == 'Y' && toupper(oper1[1]) == '+')
    {
    if (op1val > 63)
      {
      sprintf(_syntaxBuf, "0x%04X", op1val);
      errorp2(ERR_LONGCONST, _syntaxBuf);
      }
    else insv[1] |= 0x08;  // °°°°1°°°  "Y"
    } // end if (Y)
                                         
  else if (toupper(oper1[0]) == 'Z' && toupper(oper1[1]) == '+')
    {
    if (op1val > 63)
      {
      sprintf(_syntaxBuf, "0x%04X", op1val);
      errorp2(ERR_LONGCONST, _syntaxBuf);
      }
    else insv[1] |= 0x00;  // °°°°0°°°  "Z"
    } // end if (Z)

  else errorp2(ERR_ILLOP, oper1);
                                      
  if (op2val <= 31)
    {
    insv[1] |= (op2val & 0x000F) << 4;  // rrrr°°°°
    insv[2] |= (op2val & 0x0010) >> 4;  // °°°°°°°r
    insv[1] |=  op1val & 0x0007;        // °°°°°qqq
    insv[2] |= (op1val & 0x0018) >> 1;  // °°°°qq°°
    insv[2] |=  op1val & 0x0020;        // °°q°°°°°
    }

  else errorp2(ERR_BADREG, oper2);

  p2eval(22);           // 2-byte instruction, 2 operands
  } // ins31

//-----------------------------------------------------------------------------
//
//                                ins32
// INSTRUCTION GROUP 32
//
// ELPM - Extended Load Program Memory
// LPM - Load Program Memory
//
//  {5,"ELPM",32,0x9000},  //10010101 11011000   (i)     (no operands)
//                         //1001000d dddd0110   (ii)    (ELPM Rd,Z)
//                         //1001000d dddd0111   (iii)   (ELPM Rd,Z+)
//
//  {4,"LPM",32,0x9000},   //10010101 11001000   (i)     (no operands)
//                         //1001000d dddd0100   (ii)    (LPM Rd,Z)
//                         //1001000d dddd0101   (iii)   (LPM Rd,Z+)
//
void ins32()
  {                                        
  UINT op1val;

  if (oper1[0] == 0)
    {
    insv[2] = 0x95;                               // 10010101
    if (toupper(fopcd[0]) == 'E') insv[1] = 0xD8; // 11011000 ELPM
    else insv[1] = 0xC8;                          // 11001000 LPM
    p2eval(21);           // Single byte, no operands
    return;               // Done - return
    }

  op1val = expr(oper1);
  if (erno != 0) { errorp2(erno, oper1); p2eval(22); return; }

  if (erno == 0 && op1val > 31) errorp2(ERR_BADREG, oper1);

  else if (erno == 0 && op1val <= 31)
    {
    insv[1] |= (op1val & 0x000F) << 4;     // dddd°°°°
    insv[2] |= (op1val & 0x0010) >> 4;     // °°°°°°°d
    }

  // ELPM
  if (toupper(fopcd[0]) == 'E' && toupper(oper2[0]) == 'Z')     // && StrCmpI(oper2, "Z") == 0)
    {
    if (oper2[1] == '+') insv[1] |= 0x07;  // °°°°°111  "Z+"
    else insv[1] |= 0x06;                  // °°°°°110  "Z"
    }

  // LPM
  else if (toupper(fopcd[0]) == 'L' && toupper(oper2[0]) == 'Z') // && StrCmpI(oper2, "Z") == 0)
    {
    if (oper2[1] == '+') insv[1] |= 0x05;  // °°°°°101  "Z+" 
    else insv[1] |= 0x04;                  // °°°°°100  "Z"
    }

  else errorp2(ERR_ILLOP, oper2);

  p2eval(22);               // 2-byte instruction, 2 operands
  } // ins32

//-----------------------------------------------------------------------------
//
//                       ins33  Reserved
void ins33() {}

//-----------------------------------------------------------------------------

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("szCmdlineSymbol=%s  symboltab_ptr->symString=%s",
//ha//        szCmdlineSymbol,    symboltab_ptr->symString);
//ha//DebugStop(1, "oper_preval()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//if (_debugSwPass == _PASS2)
//ha//{
//ha//printf("erno=%d lvalue=%08X  op1val=%04X  op2val=%04X\noper1=%s  oper2=%s",
//ha//        erno,   lvalue,      op1val,      op2val,     oper1,     oper2);
//ha//DebugStop(1, "ins13()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//--------------------------end-of-c++-module-----------------------------------

