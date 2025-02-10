// haXASM - Cross-Assembler for Intel 8bit processors
// bytvalUpi42.cpp - C++ Developer source file.
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
char* signon = "UPI-41/C42 Cross-Assembler, Version 2.1\n";

int predefCount;

// Valid operand strings
char* operstr_A     = "A";
char* operstr_aR    = "@R";     // operstr_@R
char* operstr_aA    = "@A";     // operstr_@A
char* operstr_C     = "C";
char* operstr_F0    = "F0";
char* operstr_F1    = "F1";
char* operstr_I     = "I";
char* operstr_FLAGS = "FLAGS";
char* operstr_DMA   = "DMA";
char* operstr_TCNT  = "TCNT";
char* operstr_CNT   = "CNT";
char* operstr_TCNTI = "TCNTI";
char* operstr_RB0   = "RB0";
char* operstr_RB1   = "RB1";
char* operstr_DBB   = "DBB";
char* operstr_P1    = "P1";
char* operstr_P2    = "P2";
char* operstr_T     = "T";
char* operstr_STS   = "STS";
char* operstr_PSW   = "PSW";
                                                              
char* operstr_PMB0  = "PMB0";
char* operstr_PMB1  = "PMB1";
char* operstr_A20   = "A20";
char* operstr_TX    = "TX";

// Extern variables and functions
extern void DebugStop(int, char*, char*); // Usage: DebugStop(testNr, "Function()", __FILE__);
extern void DebugPrintBuffer(char*, int); // Usage: DebugPrintBuffer(buffer, count);

extern char szCmdlineSymbol[];

extern char* skip_leading_spaces(char*);
extern UINT expr(char *);
extern void edpri();
extern void clepc();
extern void errorp2(int, char*);

//------------------------------------------------------------------------------
//
// BASIC HARDWARE MEMORY-MAPPED ADDRESS LAYOUT
// (refer to Intel "UPI-41A User's Manual" October 1993 Page 11,12 Fig.2-5,6)                              
// (also see Intel "UPI-C42 8-BIT SLAVE MICROCONTROLLER" 12/1995)
// (also see Intel "MCS-48 and UPI-41 Assembly Language Manual" (C)1976-78)
//
#define ROMSIZE_8042  0x0800 // 2K ROM
#define ROMSIZE_UPI42 0x1000 // 4K ROM
#define RAMSIZE_UPI42 128    // 128 bytes of on-chip RAM

//------------------------------------------------------------------------------
//
//                          InitMemorySize
//
void InitMemorySize()
  {
  //RomSize  = ROMSIZE_8042;
  RomSize  = ROMSIZE_UPI42;
  SRamSize = RAMSIZE_UPI42;
  } // InitMemorySize

//------------------------------------------------------------------------------
//
//                          dq_endian                      
//
// Intel: little endian
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
// Intel: little endian
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
// Intel: little endian
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
// UPI-41/C42 Cross-Assembler, Version 2.1                     09/11/2001 PAGE 0000
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
  
  // Hidden pre-definition of important registers R0..R7 needed for EQU aliases
  // Insert predefined symbols at start of symboltab array
  //
  //  typedef struct tagSYMBOLBUF {  // Symbol structure
  //    char  symString[SYMLEN + 1]; // Symbol/Label name string
  //    ULONG symAddress;            // Symbol value or pointer to operand string
  //    UCHAR symType;               // Symbol type (ABS, C-SEG, UNDEFINED)
  //  } SYMBOLBUF, *LP_SYMBOLBUF;
  //
  symboltab_ptr = symboltab;
  predefCount = 7+1;                         // Eight symbols: "R0".."R7"
  for (_i=0; _i<predefCount; _i++)
    {
    symboltab_ptr->symString[0] = 'R';       // Symbol string "Rn"
    symboltab_ptr->symString[1] = _i | '0';  // Convert to ASCII
    symboltab_ptr->symAddress = _i;          // Value: 0..7
    symboltab_ptr->symType = _ABSEQU;        // Type = ABS (absolute constant)
    symboltab_ptr++;
    }
  
  // Symbol from cmdline
  if (szCmdlineSymbol[0] > SPACE)
    {
    StrCpy(symboltab_ptr->symString, szCmdlineSymbol);        
    symboltab_ptr->symAddress = 0x01;   // Value = 0x01
    symboltab_ptr->symType = _ABSSET;   // Type = redefinable absolute constant
    symboltab_ptr++;
    }
  
  nopAlign = 0x00;    // NOP instruction value for .ALIGN / .EVEN directives
  IntelFlag = TRUE;   // Set Intel syntax
  } // oper_preval

//-----------------------------------------------------------------------------
//
//                      p1eval
//
// PASS1 INSTRUCTION EVALUATION
//
void p1eval()
  {
  ins_group &= ~(0x40|0x80);

  if (ins_group == ERR) pcc += 2;     // group 255: illegal, give 2 bytes
  else if (ins_group == 1)  return;
  else if (ins_group <= 18) pcc += 1; // groups 2..18:  1 byte, definitely!
  else if (ins_group >= 21) pcc += 2; // groups 21..23: 2 bytes, definitely!
  
  else if (ins_group >= 19 || ins_group <= 20) // groups 19,20: may be 1 or 2 bytes
    {
    if (oper2[0] == '#') pcc += 2;    // groups 19,20: 2 bytes
    else pcc += 1;                    // groups 19,20: 1 byte  '@R..'
    } 

  // PASS1: 3 bytes (not used w/ ASM42)
  } // p1eval


//------------------------------------------------------------------------------
//
//                           oper_Rn
//
int oper_Rn(char _operRn[])
  {
  if (toupper(_operRn[0]) == 'R')
    {
    lastCharAL = _operRn[1] & 0x07; // R0..R7
    if (_operRn[1] > '7' || _operRn[1] < '0' || _operRn[2] > SPACE) return(-1);
    else return(0);
    }
  else return(-1);
  } // oper_Rn

//------------------------------------------------------------------------------
//
//                           oper_aRr (oper_@Rr)
//
int oper_aRr(char _operaRr[])
  {
  if (_operaRr[0] == '@' && toupper(_operaRr[1]) == 'R')
    {
    lastCharAL = _operaRr[2] & 0x07; // @R0..@R7
    if (_operaRr[2] > '1' || _operaRr[2] < '0' || _operaRr[3] > SPACE) return(-1);
    else return(0);
    }
  else return(-1);
  } // oper_aRr

//------------------------------------------------------------------------------
//
//                          oper_Pp  (P1 / P2)
//
int oper_Pp(char _operP[])
  {
  if (toupper(_operP[0]) == 'P')
    {
    lastCharAL = _operP[1] & 0x03;  // P0..P1
    if (_operP[1] > '2' || _operP[1] < '0' || _operP[2] > SPACE) return(-1);
    }
  return(0);
  } // oper_Pp

//------------------------------------------------------------------------------
//
//                          oper_XPp  (P4 / P5 / P6 / P7)
//
int oper_XPp(char _operXP[])
  {
  if (toupper(_operXP[0]) == 'P')
    {
    lastCharAL = _operXP[1] & 0x03;  // P4..P7
    if (_operXP[1] > '7' || _operXP[1] < '4' || _operXP[2] > SPACE) return(-1);
    }
  return(0);
  } // oper_XPp

//------------------------------------------------------------------------------
//
//                          p2emp  P2 EMPTY
//
// Check if the unused operand field is empty. Otherwise give error
//
void p2emp(char _operX[])
  {
  if (_operX[0] != 0) errorp2(ERR_TOOLONG, _operX);
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
      ilen = 1;
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
  errorp2(ERR_BADREG, NULL);
  p2eval(11);        // Give 1 byte on error
  } // ins_error11
                      

//-----------------------------------------------------------------------------
//
//                             ins_error22
//
void ins_error22()
  {
  errorp2(ERR_BADREG, NULL);
  p2eval(22);        // Give 2 byte2 on error
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
  errorp2(ERR_ILLOPCD, NULL);
  p2eval(22);          // UPI-C42: Illegal opcode, 2 bytes nul
  } // ins0

//-----------------------------------------------------------------------------
//
//                            ins2
//
// INSTRUCTION GROUP 2
//  Single byte, no operands
//
void ins2()
  {
  p2eval(10);  // Single byte, no operands
  } // ins2

//-----------------------------------------------------------------------------
//
//                             ins3
//
// INSTRUCTION GROUP 3
//  DEC A  DEC Rn
//
void ins3()
  {
  if (StrCmpNI(oper1, operstr_A, strlen(operstr_A)) == 0)
    insv[1] = 0x07;

  else if (oper_Rn(oper1) == 0)
    insv[1] = lastCharAL | 0xC8;

  else ins_error11();

  p2eval(11);        // 1byte instr., 2nd operand must be empty
  } // ins3

//-----------------------------------------------------------------------------
//
//                           ins4
// INSTRUCTION GROUP 4
//  INC A  INC Rn  INC @Rr
//
void ins4()
  {
  int _labAdr;

  if (StrCmpNI(oper1, operstr_A, strlen(operstr_A)) == 0)
    insv[1] = 0x17;

  else if (oper_Rn(oper1) == 0)
    insv[1] = lastCharAL | 0x18;

  else if (oper_aRr(oper1) == 0)
    insv[1] = lastCharAL | 0x10;
  
  // Rn via EQU alias
  else if ((_labAdr = expr(oper1)) <= 7)
    insv[1] = (_labAdr & 0xFF) | 0x18;  

  else ins_error11();

  p2eval(11);        // 1byte instr., 2nd operand must be empty
  } // ins4

//-----------------------------------------------------------------------------
//
//                               ins5
// INSTRUCTION GROUP 5
// CLR/CPL operand = {A//C//F0//F1}
//
void ins5()
  {
  int op1Len;

  if (toupper(oper1[0]) == 'A')
    {
    op1Len=1;
    if (toupper(fopcd[1]) == 'L') insv[1] = 0x27; // CLR A
    else insv[1] = 0x37;                          // CPL A
    }

  else if (toupper(oper1[0]) == 'C')
    {
    op1Len=1;
    if (toupper(fopcd[1]) == 'L') insv[1] = 0x97; // CLR C
    else insv[1] = 0xA7;                          // CPL C
    }

  else if (StrCmpNI(oper1, operstr_F0, (op1Len=strlen(operstr_F0))) == 0)
    {
    if (toupper(fopcd[1]) == 'L') insv[1] = 0x85; // CLR F0
    else insv[1] = 0x95;                          // CPL F0
    }

  else if (StrCmpNI(oper1, operstr_F1, (op1Len=strlen(operstr_F1))) == 0)
    {
    if (toupper(fopcd[1]) == 'L') insv[1] = 0xA5; // CLR F1
    else insv[1] = 0xB5;                          // CPL F1
    }

  // Check if 0-terminated (comments are already filtered in dcomp.cpp)
  skip_leading_spaces(&oper1[op1Len]);     
  if (lastCharAL != 0)
    {
    insv[1] = 0;               // invalidate instruction opcode value
    errorp2(ERR_BADREG, NULL); // Assert error line in listing
    }

  p2eval(11);                  // 1byte instr., 2nd operand must be empty
  } // ins5

//-----------------------------------------------------------------------------
//
//                              ins6
// INSTRUCTION GROUP 6
//  JMPP @A
//
void ins6()
  {
  if (StrCmpNI(oper1, operstr_aA, strlen(operstr_aA)) != 0)
    ins_error11();

  else p2eval(11);  // 1byte instr., 2nd operand must be empty
  } // ins6

//-----------------------------------------------------------------------------
//
//                              ins7
// INSTRUCTION GROUP 7
//  DA RL RR RLC RRC SWAP {A}
//
void ins7()
  {
  if (StrCmpNI(oper1, operstr_A, strlen(operstr_A)) != 0)
    ins_error11();
  else p2eval(11);  // 1byte instr., 2nd operand must be empty
  } // ins7

//-----------------------------------------------------------------------------
//
//                                ins8
// INSTRUCTION GROUP 8
//  EN I/DMA/FLAGS/A20/T0/TCNTI
//
void ins8()
  {
  int op1Len;

  if (StrCmpNI(oper1, operstr_I, (op1Len=strlen(operstr_I))) == 0)
    ; // insv[1] = insv[1];

  else if (StrCmpNI(oper1, operstr_DMA, (op1Len=strlen(operstr_DMA))) == 0)
    insv[1] |= 0xE0;

  else if (StrCmpNI(oper1, operstr_FLAGS, (op1Len=strlen(operstr_FLAGS))) == 0)
    insv[1] |= 0xF0;

  else if (StrCmpNI(oper1, operstr_A20, (op1Len=strlen(operstr_A20))) == 0)
    insv[1] = 0x33;

  else if (StrCmpNI(oper1, operstr_TX, (op1Len=strlen(operstr_TX))) == 0)
    insv[1] = 0xC3;

  else if (StrCmpNI(oper1, operstr_TCNTI, (op1Len=strlen(operstr_TCNTI))) == 0)
    insv[1] |= 0x20;
  
  // Check if 0-terminated (comments are already filtered in dcomp.cpp)
  skip_leading_spaces(&oper1[op1Len]);     
  if (lastCharAL != 0)
    {
    insv[1] = 0;       // invalidate instruction opcode value
    errorp2(ERR_SYNTAX, NULL);
    }

  p2eval(11);          // 1byte instr., 2nd operand must be empty
  } // ins8

//-----------------------------------------------------------------------------
//
//                                ins9
// INSTRUCTION GROUP 9
//  STRT CNT  STRT T
//
void ins9()
  {
  if (StrCmpNI(oper1, operstr_CNT, strlen(operstr_CNT)) == 0)
    ; // insv[1] |= 0x00;

  else if (StrCmpNI(oper1, operstr_T, strlen(operstr_T)) == 0)
    insv[1] |= 0x10;

  else ins_error11();

  p2eval(11);        // 1byte instr., 2nd operand must be empty
  } // ins9

//-----------------------------------------------------------------------------
//
//                                ins10
// INSTRUCTION GROUP 10
//  STOP TCNT
//
void ins10()
  {
  if (StrCmpNI(oper1, operstr_TCNT, strlen(operstr_TCNT)) != 0)
    ins_error11();

  else p2eval(11);  // 1byte instr., 2nd operand must be empty
  } // ins10

//-----------------------------------------------------------------------------
//
//                                ins11
// INSTRUCTION GROUP 11
//  SEL RB0  SEL RB1  SEL PMB0  SEL PMB1
//
void ins11()
  {
  if (StrCmpNI(oper1, operstr_RB0, strlen(operstr_RB0)) == 0)
    ; // insv[1] |= 0x00; 

  else if (StrCmpNI(oper1, operstr_RB1, strlen(operstr_RB1)) == 0)
    insv[1] |= 0x10;

  else if (StrCmpNI(oper1, operstr_PMB0, strlen(operstr_PMB0)) == 0)
    insv[1] = 0x63;

  else if (StrCmpNI(oper1, operstr_PMB1, strlen(operstr_PMB1)) == 0)
    insv[1] = 0x73;

  else ins_error11();

  p2eval(11);        // 1byte instr., 2nd operand must be empty
  } // ins11

//-----------------------------------------------------------------------------
//
//                                ins12
// INSTRUCTION GROUP 12
//  DIS I  DIS TCNTI
//
void ins12()
  {
  if (StrCmpNI(oper1, operstr_I, strlen(operstr_I)) == 0)
    p2eval(11);
  
  else if (StrCmpNI(oper1, operstr_TCNTI, strlen(operstr_TCNTI)) == 0)
    {
    insv[1] |= 0x10;
    p2eval(11);        // 1byte instr., 2nd operand must be empty
    }

  else ins_error11();
  } // ins12

//-----------------------------------------------------------------------------
//
//                                ins13
// INSTRUCTION GROUP 13
//  2nd operand must be member of set {A}
//  OUTL P1/P2,A
//
void ins13()
  {
  if (StrCmpNI(oper2, operstr_A, strlen(operstr_A)) != 0)
    ins_error11();

  else if (oper_Pp(oper1) != 0)
   ins_error11();

  else
    {
    insv[1] |= lastCharAL;
    p2eval(12); // 1byte instr., 2 operands
    }
  } // ins13

//-----------------------------------------------------------------------------
//
//                                ins14
// INSTRUCTION GROUP 14
//  2nd operand must be member of set {A}
//  OUT DBB,A
//
void ins14()
  {
  if (StrCmpNI(oper1, operstr_DBB, strlen(operstr_DBB)) != 0)
    ins_error11();

  else if (StrCmpNI(oper2, operstr_A, strlen(operstr_A)) != 0)
    ins_error11();

  else p2eval(12);  // 1byte instr., 2 operands
  } // ins14

//-----------------------------------------------------------------------------
//
//                                ins15
// INSTRUCTION GROUP 15
// 1st operand must be member of set {A}
//  IN A,P1/P2
//  IN A,DBB
//
void ins15()
  {
  if (toupper(oper1[0]) == 'A' && StrCmpNI(oper2, operstr_DBB, strlen(operstr_DBB)) == 0)
    insv[1] = 0x22;

  else if (toupper(oper1[0]) == 'A' && oper_Pp(oper2) == 0)
    insv[1] |= (lastCharAL | 0x08);

  else ins_error11();
  
  p2eval(12); // 1byte instr., 2 operands
  } // ins15

//-----------------------------------------------------------------------------
//
//                                ins16
// INSTRUCTION GROUP 16
//  1st operand member of set {A} or of set {P1;P2}
//  2nd opd must be member of set {P1;P2} or of set {A}
//  MOVD A,Pp
//  Opcode: 000011pp  P4:pp=00 P5:pp=01 P6:pp=10 P7:pp=11
//
//  MOVD Pp,A
//  Opcode: 001111pp  P4:pp=00 P5:pp=01 P6:pp=10 P7:pp=11
//
//  ORLD Pp,A
//  Opcode: 100011pp  P4:pp=00 P5:pp=01 P6:pp=10 P7:pp=11
//
//  ANLD Pp,A
//  Opcode: 100111pp  P4:pp=00 P5:pp=01 P6:pp=10 P7:pp=11
//
void ins16()
  {
  if (StrCmpNI(oper1, operstr_A, strlen(operstr_A)) == 0 && oper_XPp(oper2) == 0)
    insv[1] |= (lastCharAL & 0x03);

  else if (StrCmpNI(oper2, operstr_A, strlen(operstr_A)) == 0 && oper_XPp(oper1) == 0)
    {
    insv[1] |= (lastCharAL & 0x03);
    if (toupper(fopcd[0]) == 'M') insv[1] |= 0x30; // MOVD Pp,A
    }

  else ins_error11();

  p2eval(12); // 1byte instr., 2 operands
  } // ins16

//-----------------------------------------------------------------------------
//
//                                ins17
// INSTRUCTION GROUP 17
//  MOVP/MOVP3 A,@A
//
void ins17()
  {
  if (StrCmpNI(oper1, operstr_A, strlen(operstr_A)) == 0 &&
      StrCmpNI(oper2, operstr_aA, strlen(operstr_aA)) == 0)
    p2eval(12); // 1byte instr., 2 operands
               
  else ins_error11();
  } // ins17

//-----------------------------------------------------------------------------
//
//                                ins18
// INSTRUCTION GROUP 18
//  1st operand must be member of set {A}
//  XCH A,Rn  XCH/XCHD A,@Rr  (XCHD A,Rn - illegal)
//
void ins18()
  {
  // 1st operand must be member of set {A}
  if (toupper(oper1[0]) != 'A') ins_error11();
  else 
    {
    // XCHD A,Rn - illegal
    if (oper_Rn(oper2) == 0 && (ins_group & 0x80) == 0x80)
      ins_error11();

    // XCH/XCHD A,@Rr
    else if (oper_aRr(oper2) == 0)
      { 
      insv[1] |= lastCharAL;  
      p2eval(12);  // 1byte instr., 2 operands
      }

    // XCH A,Rn
    else if (oper_Rn(oper2) == 0 || (lastCharAL = expr(oper2)) <= 7)
      {
      insv[1] |= (lastCharAL | 0x08);
      p2eval(12);  // 1byte instr., 2 operands
      }

    else ins_error11();
    }
  } // ins18

//-----------------------------------------------------------------------------
//
//                                ins19
// INSTRUCTION GROUP 19
// ADD/XRL/ANL/ORL
//
void ins19()
  {
  int _labAdr;

  // 2bytes, if 2nd operand is #data
  if (oper2[0] == '#')
    {
    insv[2] = expr(&oper2[1]);

    // ANL/ORL A,#dd - special
    if (toupper(oper1[0]) == 'A' && (ins_group & 0x40) == 0)
      {
      insv[1] |= 0x03;
      p2eval(22);
      return;                                                         
      }

    // ADD/ADDC A,#dd - special
    if ((ins_group & 0x40) == 0x40)
      {
      insv[1] &= 0x1F;
      insv[1] |= 0x03;  
      }
    else insv[1] |= 0x03;

    // ANL/ORL Pp,#dd - special
    if ((ins_group & 0x80) == 0x80)
      {
      insv[1] ^= 0xC8;
      insv[1] &= 0xFC;
      if (oper_Pp(oper1) == 0)
        {
        insv[1] |= lastCharAL;
        }
      else ins_error22();
      }

    p2eval(22);
    return;                                                         
    } // end if (#)
  //----------------------------------------------------------------------

  if (toupper(oper1[0]) == 'A')
    {
    // ADD/XRL/ANL/ORL A,@Rr
    if (oper_aRr(oper2) == 0)
      insv[1] |= lastCharAL;            

    // ADD/XRL/ANL/ORL A,Rn
    else if (oper_Rn(oper2) == 0)
      insv[1] |= (lastCharAL | 0x08);  

    // ADD/XRL/ANL/ORL A,symbl - Rn via pre-defined EQU aliases
    else if ((_labAdr = expr(oper2)) <= 7)
     insv[1] |= ((_labAdr &0xFF) | 0x08);

    else ins_error11();

    p2eval(12);
    return;
    } // end if (oper1[0] == 'A')
  

  if (oper_aRr(oper2) == 0)
    {
    insv[1] |= lastCharAL;
    p2eval(12); 
    return;
    }
  else ins_error11();

  if (oper_Pp(oper1) == 0)
    {
    insv[1] |= lastCharAL;
    insv[2] = expr(&oper2[1]);
    p2eval(22); 
    return;
    }
  else ins_error11(); 
  } // ins19

//-----------------------------------------------------------------------------
//
//                                ins20
// INSTRUCTION GROUP 20
// MOV {A;Rn;@Rr},#dd
//
void ins20()
  {
  int _labAdr;

  //-------------------------------
  // 1 byte MOV instructions
  // 1st operand is A
  // 2nd operand is Rn, @Rr, T, PSW
  //-------------------------------
  if (toupper(oper1[0]) == 'A' && oper2[0] != '#')
    {
    // MOV A,@Rn
    if (oper_aRr(oper2) == 0)
      {
      insv[1] = 0xF0;
      insv[1] |= lastCharAL;
      }

    // MOV A,Rn
    else if (oper_Rn(oper2) == 0)
      {
      insv[1] = 0xF8;
      insv[1] |= lastCharAL;
      }

    // MOV A,PSW   
    else if (StrCmpNI(oper2, operstr_PSW, strlen(operstr_PSW)) == 0)
      insv[1] = 0xC7;

    // MOV A,T
    else if (toupper(oper2[0]) == 'T' && oper2[2] == 0)
      insv[1] = 0x42;

    // Must stay here as the last statement (because PSW, T are no sysmbols)
    // A,symbl - Rn via pre-defined EQU aliases
    else if ((_labAdr = expr(oper2)) <= 7)
      {
      insv[1] = 0xF8;
      insv[1] |= (_labAdr & 0xFF);  
      }                                            

    else ins_error11(); 

    p2eval(12); // 1byte instr., 2 operands
    return; 
    } // end if (oper1[0] == 'A')

  //----------------------------------------------------------------------

  //--------------------------
  // 2 bytes MOV instructions
  // 1st operand is A, Rn, @Rr
  // 2nd operand is #data
  //--------------------------
  if (oper2[0] == '#')
    {
    insv[2] = expr(&oper2[1]);

    // MOV A,#dd
    if (toupper(oper1[0]) == 'A' && oper_aRr(oper2) != 0)
      {
      insv[1] = 0x23;
      }

    // MOV Rn,#dd
    else if (oper_Rn(oper1) == 0)
      {
      insv[1] = 0xB8;
      insv[1] |= lastCharAL;
      }

    // MOV @Rr,#dd
    else if (oper_aRr(oper1) == 0)             
      {
      insv[1] = 0xB0;
      insv[1] |= lastCharAL;
      }

    // Rn,#symbl - via EQU alias
    else if ((_labAdr = expr(oper1)) <= 7)
      {
      insv[1] = 0xB8;
      insv[1] |= (_labAdr & 0xFF);  
      }

    else ins_error22();

    p2eval(22); // 2bytes instr., 2 operands
    return;
    } // end if (#)
  
  //----------------------------------------------------------------------

  //------------------------------------
  // Special 1 byte MOV instructions
  // 1st operand is STS, T, PSW, Rn, @Rr
  // 2nd operand is A
  //------------------------------------
  if (toupper(oper2[0]) == 'A')  
    {
    // MOV STS,A
    if (StrCmpNI(oper1, operstr_STS, strlen(operstr_STS)) == 0)
      insv[1] = 0x90;

    // MOV PSW,A
    else if (StrCmpNI(oper1, operstr_PSW, strlen(operstr_PSW)) == 0)
      insv[1] = 0xD7;

    // MOV T,A
    else if (toupper(oper1[0]) == 'T' && oper1[1] == 0)
      insv[1] = 0x62;

    // MOV @Rn,A
    else if (oper_aRr(oper1) == 0)
      {
      insv[1] = 0xA0;
      insv[1] |= lastCharAL;
      }

    // MOV Rn,A
    else if (oper_Rn(oper1) == 0)
      {
      insv[1] = 0xA8;
      insv[1] |= lastCharAL;
      }

    // Must stay here as the last statement (because STS, PSW, T are no sysmbols)
    // symbl,A - Rn via pre-defined EQU aliases
    else if ((_labAdr = expr(oper1)) <= 7)
      {
      insv[1] = 0xA8;
      insv[1] |= (_labAdr & 0xFF);  
      }

    else ins_error11();

    p2eval(12); // 1byte instr., 2 operands
    return; 
    } // end if (oper2[0] == 'A')
  
  ins_error11();
  } // ins20

//-----------------------------------------------------------------------------
//
//                                ins21
// INSTRUCTION GROUP 21
//  Jxx aa conditional jumps (relative)
//
void ins21()
  {
  int _labAdr;

  _labAdr = expr(oper1);
  insv[2] = _labAdr & 0x00FF;

  if (((_labAdr & 0xFF00) >> 8) != (((pcc+2) & 0xFF00) >> 8))
    errorp2(ERR_JMPADDR, NULL); // Must be on same ROM page!

  p2eval(21);           // 2byte instr., 2nd operand must be empty
  } // ins21

//-----------------------------------------------------------------------------
//
//                                ins22
// INSTRUCTION GROUP 22
//  JMP aaaa CALL aaaa
//
void ins22()
  {
  int _labAdr;

  _labAdr = expr(oper1);
  insv[2] = _labAdr & 0x00FF;
  insv[1] |= ((_labAdr & 0xFF00) >> 3);

  p2eval(21);           // 2byte instr., 2nd operand must be empty
  } // ins22

//-----------------------------------------------------------------------------
//
//                                ins23
// INSTRUCTION GROUP 23
//  DJNZ Rn,aaaa
//
void ins23()
  {
  int _labAdr;

  _labAdr = expr(oper2);
  insv[2] = _labAdr & 0xFF;

  if (((_labAdr & 0xFF00) >> 8) != (((pcc+2) & 0xFF00) >> 8))
    errorp2(ERR_JMPADDR, NULL); // Must be on same ROM page!

  if (oper_Rn(oper1) == 0)
    insv[1] |= lastCharAL;

  else if ((_labAdr = expr(oper1)) <= 7)
    {
    _labAdr = expr(oper1);
    insv[1] |= (_labAdr & 0x0F);
    }
    
  else errorp2(ERR_BADREG, NULL);

  p2eval(22);          // 2bytes, 2 operands
  } // ins23

//-----------------------------------------------------------------------------
//
//                       ins24..33  Reserved
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
//ha//printf("fopcd=%s  oper1=%s  oper2=%s  insv[1]=%02X  lastCharAL=%02X",
//ha//        fopcd,    oper1,    oper2,    insv[1],      lastCharAL);
//ha//DebugStop(2, "ins16()", __FILE__);
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

