// haXASM - Cross-Assembler for 8bit Microprocessors
// dcomp.cpp - C++ Developer source file.
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

#include<math.h>
#include<float.h>

#include "equate.h"
#include "extern.h"    // Variables published in workst.cpp

using namespace std;

// Global variables
char  _patchBuf[LSBUFLEN];
char* _patchBufPtr = _patchBuf;
char* inbufPtr;

int _operFlag = 0;
int swbegincom=0;  // "/*.." comment begin flag

// Extern variables and functions
extern void DebugStop(int, char*, char*); // Usage: DebugStop(testNr, "Function()", __FILE__);
extern void DebugPrintBuffer(char*, int); // Usage: DebugPrintBuffer(buffer, count);
extern void DebugPrintMacroArray(int);    // Usage: DebugPrintSymbolArray(count);

extern char* pszFileInbuf;
extern char* pszFileInbuf0;

extern char* DQ_str;
extern char* DD_str;
extern char* DW_str;
extern char* DB_str;
extern char* DS_str;
extern char* END_str;
extern char* EQU_str;
extern char* DEF_str;
extern char* SET_str;
extern char* _defin_str;
extern char* DEFIN_str;

extern char szSrcFileName[];
extern char szIclFileName[];

extern char* pszCurFileName;

extern char* macroLineBuf2_ptr;
extern int macroLineBuf2Len;

extern BOOL p12ExpandMacro(int);
extern void p12CheckEndmacro();

extern void p12CCheckIfend();

extern char* skip_leading_spaces(char*);
extern char* skip_trailing_spaces(char*);

extern void p1LineContinuation();
extern void p2LineContinuation();
extern void lineAppendCRLF(char*);
extern void edit_inc_linenr();
//ha//extern void edbh(char*, int);
//ha//extern void edwh(char*, int);
extern void edbh(char*, UINT);
extern void edwh(char*, UINT);
extern void eddh(char*, ULONG);
extern void cllib();
extern void cllin();

extern UCHAR labfn(char*);

extern void warnout();
extern void clepc();
extern void edpri();
extern void finish();

extern void ActivityMonitor(int);

extern void erout();
extern void p1errorl(UCHAR, char*);
extern void errorp2(int, char*);
extern void errchk(char*, int);
extern UINT expr(char*);
extern BOOL delm2(char* _ptr);
extern void GetPcValue();

extern void CloseSrcFile();
extern void CloseIclFile();

extern ifstream SrcFile; // Filestream read  (*.ASM)
extern ifstream IclFile; // Filestream read  (*.INC)
extern ofstream LstFile; // Filestream write (*.LST)
extern ofstream HexFile; // Filestream write (*.HEX)

// Forward declaration of functions included in this code module:
void EvaluateInternFunction(char*);
BOOL EvaluatePredefinedMacro(char*);
 
//-----------------------------------------------------------------------------
//
//                      SyntaxAVRtoMASM
//
// Atmel/Microchip AVRASM2 syntax: .EQU <symbol> = <expr>  // ".EQU REV1=2+$AA"
//                                 .SET <symbol> = <expr>  // ".SET REV1=2+$AA"
// Input: flabl = 0
//        fopcd = .EQU/.SET/#define
//        oper1 = symbol = expr    // e.g. REV1=2+$AA"
//        oper2 = 0
//                                             [
// Microsoft MASM/ML syntax:       <symbol> EQU <expr>  // "REV1 EQU 2+$AA"
//                                 <symbol> SET <expr>  // "REV1 SET 2+$AA"
// Output: flabl = symbol          // e.g. "REV1"
//         fopcd = .EQU/.SET
//         oper1 = expr            // e.g. "2+$AA"
//         oper2 = 0
//
void SyntaxAVRtoMASM(int _p12sw)
  {
  // Atmel AVRASM2 compatible: Allow something like : ".SET     REV01 = 2+$AA"
  // >                                                fopcd    flabl  (inbuf)
//  if (AtmelFlag != 0 && oper2[0] == 0)
  if (oper2[0] == 0)  // also for Motorola (see Pass1/2 p1equ() p2equ())
    {                                                                                  
    _i=0;
    // Transfer <symbol> eliminating SPACEs and TABs
    while (oper1[_i] > SPACE && oper1[_i] != '=') flabl[_i] = oper1[_i++];
    while (oper1[_i] != '=' && oper1[_i] != 0) _i++;

    // Pass2 syntax error checking only
    if (_p12sw == _PASS2   && 
        oper1[_i] != '='   && 
        StrCmpI(fopcd, _defin_str) != 0 &&
        StrCmpI(fopcd,  DEFIN_str) != 0
       ) 
      errorp2(ERR_ILLEXPR, NULL);

    // Left-justify and 0-terminate the <expr> w/o touching the expr itself
    // (see 'replace_token()')
    _i++; _j=0; _k=0;
    while (oper1[_i] != 0)
      {
      if (oper1[_i] <=SPACE && _k == 0) _i++;
      else 
        {
        oper1[_j++] = oper1[_i++];
        _k=1;
        }
      }
    oper1[_j]=0;
    } // end if (AtmelFlag)
  } // SyntaxAVRtoMASM

//-----------------------------------------------------------------------------
//
//                      EvaluateOperand  (currently PC only)
//
// Support for assembler specific operands:
//
// PC - the current value of the Program memory location counter
//
void EvaluateOperand(char* _operStr)  // _operStr = "PC" only
  {
  char *inbufPtr, *tmpPtr;
  char  _patchBuf[LSBUFLEN];
  char* _patchBufPtr = _patchBuf;

  int inbufLead;

  if (AtmelFlag != FALSE)
    {
    // Don't evaluate _operStr within macro
    if (StrStrI(inbuf, "@") != 0) return; 
    if (StrStrI(inbuf, _operStr) == NULL) return; // not found

    inbufPtr = inbuf;
    while (*inbufPtr != 0)
      {
      if ((inbufPtr=StrStrI(inbufPtr, _operStr)) != NULL)
        {
        // Exclude any matches within Comments ';'
        if ((tmpPtr = strstr(inbuf, ";")) != NULL)
          {
          // Abort - "_operStr" is within comment
          if (tmpPtr < inbufPtr) break; 
          }
        }
      else break;

      if (delm2(&inbufPtr[-1]) == TRUE &&
          (delm2(&inbufPtr[+2]) == TRUE || inbufPtr[+2] < SPACE)) // CR/LF/0
        {
        // Replace 'PC' with '$'
        inbufLead = inbufPtr - inbuf;
        for (_i=0; _i<inbufLead; _i++) *_patchBufPtr++ = inbuf[_i];
        *_patchBufPtr++ = _BUCK;  // '$'

        // Update inbuf
        for (_i=(inbufLead+strlen(_operStr)); _i<INBUFLEN; _i++)
          *_patchBufPtr++ = inbuf[_i];
        for (_i=0; _i<INBUFLEN; _i++) inbuf[_i] = _patchBuf[_i];
        } // end if

      inbufPtr += strlen(_operStr); // advance inbuf index behind _operStr
      _patchBufPtr = _patchBuf;     // re-init for next loop.
      } // end while
    } // end if (AtmelFlag) 
  } // EvaluateOperand

//-----------------------------------------------------------------------------
//
//                      Fpoint2Bin (Atmel AVR specific)
//
// IEEE-754 Single Precision Floating Point Representation of "1864.780029"
//          as a 32bit number 
// 
//   0 1 0 0 0 1 0 0 1 1 1 0 1 0 0 1 0 0 0 1 1 0 0 0 1 1 1 1 0 1 1 0
//  |- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -|
//  |s|      exp      |                  mantissa                   |
//  |1|       8       |                     23                      | bit(s)
// 
// Using Fractional Numbers
// Unsigned 8-bit fractional numbers use a format where numbers in the
// range [0, 2] are allowed. Bit[6:0] represent the fraction and
// bit7 represents the integer part (0 or 1), i.e., a "1.7 format". 
// The FMUL instruction performs the same operation as the MUL instruction, 
// except that the result is left-shifted 1-bit so that the high byte
// of the 2-byte result will have the same "1.7 format" as the operands
// (instead of a 2.6 format). 
// Note that if the product is >= 2, the result will not be correct.
//
// To fully understand the "1.7 format" of the fractional numbers,
// a comparison with the integer number format is useful: 
//   If the byte "1011 0010" is interpreted as an unsigned integer,
//   it will be interpreted as 128 +32 +16 +2 = 178. 
//   On the other hand, if it is interpreted as an unsigned fractional number,
//   it will be interpreted as 1 +0.25 +0.125 +0.015625 = 1.390625.
//   If the byte is assumed to be a signed (negative) number,
//   it will be interpreted as 178 -256 = -122 (integer)
//   or as 1.390625 -2 = -0.609375 (fractional number).
//
// Comparison of Integer and Fractional Formats
// --------------------------------------------
// Bit number     7             6              5               4     
// Unsigned int   2°7 =128      2°6 =64        2°5 =32         2°4 =16
// Bit number     3             2              1               0
// Unsigned int   2°3 =8        2°2 =4         2°2 =2          2°0 =1
//
// Bit number     7             6              5               4     
// Unsigend float 2°0 =1        2°-1 =0.5      2°-2 =0.25      2°-3 =0.125    
// Bit number     3             2              1               0
// Unsigend float 2°-4 =0.0625  2°-5 =0.03125  2°-6 =0.015625  2°-7 =0.0078125
//
// Example 8bit: Fraction 0.75    (=0+0.5+0.25) is        "0110 0000" (=0x60)
//                        1.81125 (=1+0.5+0.25+0.0625) is "1110 1000" (=0xE8)
//
// Using signed fractional numbers in the range [-1, 1] has one main advantage
// to integers: When multiplying two numbers in the range [-1, 1],
// the result will be in the range [-1, 1], and an approximation
// (the highest byte(s)) of the result may be stored in the same number of bytes
// as the factors, with one exception: when both factors are -1,
// the product should be 1, but since the number 1 cannot be represented
// using this number format, the FMULS instruction will instead place
// the number -1 in R1:R0. The user should therefore assure that
// at least one of the operands is not -1 when using the FMULS instruction.
// The 16bit x 16bit fractional multiply also has this restriction.
//
// An example shows the assembly code that reads port A input value
// and multiplies that value with a fractional constant (-0.625) or (+0.625)
// before storing the result in register pair R17:R16.
//   -----------------------
//   in    R16, PINA        ; Read pin values
//   ldi   R17, $B0         ; R17= -0.625 "1011 0000" (= 0.625-2 = -1.375)
//   ;ldi  R17, $50        ;; Load +0.625 "0101 0000" into R17
//   fmuls R16, R17         ; R1:R0 = R17 * R16
//   movw  R17:R16, R1:R0   ; Move the result to the r17:r16 register pair
//   -----------------------
//
UINT Fpoint2Bin(char* _fpExpr, int _precision)
  {
  UINT _fpBinary = 0xFFFF;
  float _fpValue, _fpFract, _fpj;  // double _fpValue, _fpFract, _fpj;
  UINT _fpInt, _fpBit;

  //---------------------------------------------------------------------------
  // Dummy to prevent runtime error 6002 - floating point support not loaded:  |
  // This error can occur when the floating-point library was not linked.      |
  // A format string for a printf_s or scanf_s function contained a            |
  // floating-point format specification and the program did not contain       |
  // any floating-point values or variables.                                   |
  // To fix this issue, use a floating-point argument to correspond            |
  // to the floating-point format specification, or perform a floating-point   |
  // assignment elsewhere in the program.                                      |
  // This causes floating-point support to be loaded.                          |
  // Nowhere in the DLL is any floating point arithmeics carried out,          |
  // it's all string handling but does use vsprintf.                           |
  // Force loading the DLL by making the dll entry point use floating point    |
  //                                                                           |
  float _dummyFract =1.7; // Dummy floating-point assignment                   |
  //---------------------------------------------------------------------------

  if (_fpExpr[0] == '-')
    { 
    _fpValue = atof(&_fpExpr[1]);
    _fpValue -= 2;
    if (_fpValue < 0) _fpValue *= (-1);
    }
  else _fpValue = atof(_fpExpr);

  if (_fpValue == 0)
   {
   errorp2(ERR_ILLEXPR, NULL);
   return (0);      // Return from atof() must be != 0
   }

  _fpInt   = (int)_fpValue;
  _fpFract = _fpValue - _fpInt;

  _i=0; _fpj=1; _fpBinary = 0;
  // Example: 1.8125 = 1110 1000  = 0xE8
  if (_precision == 7)
    {
    _fpBit=0x0040; // =0000000001000000b
    if (_fpInt == 1) _fpBinary |= 0x0080;
    }
  // Example: 1.780029   = 1110 0011 1101 1000  = 0xE3D8
  else if (_precision == 15)                  
    {
    _fpBit=0x4000; // =0100000000000000b
    if (_fpInt == 1) _fpBinary |= 0x8000;
    }

  // Calculate fraction
  while (_i < _precision)
    {
    _fpj /= 2;
    if (_fpFract >= _fpj)
      { 
      _fpBinary |= _fpBit;
      _fpFract -= _fpj;
      }
    _fpBit >>= 1;
    _i++;
    }
     
  return(_fpBinary);
  } // Fpoint2Bin

//-----------------------------------------------------------------------------
//
//                      EvaluateInternFunction
//
// Support for internal assembler fuctions:
//
// LOW(expr)
// HIGH(expr)
// BYTE1(expr)
// BYTE2(expr)
// BYTE3(expr)
// BYTE4(expr)
//
// LWRD(expr) Returns bits 0-15 of an expression
// HWRD(expr) Returns bits 16-31 of an expression
// PAGE(expr) Returns bits 16-21 of an expression
//
// EXP2(expr) Returns 2 to the power of expression
// LOG2(expr) Returns the integer part of log2(expression)
//
// ABS(expr);  // Absolute value of a constant expression
// STRLEN();   // Length of a string constant, in bytes (not implemented)
//
// Extract floating point parts resulting from FMUL/FMHLU/FMULSU instructions
//  INT(expr)  // Truncate floating point to integer (discard fractional part)
//  FRAC(expr) // Extract fractional part of floating point (discard interger part)
//
// Convert floating point expressions for FMUL/FMHLU/FMULSU instructions
//  Q7(expr)   // Sign +  7 bit fraction
//  Q15(expr)  // sign + 15 bit fraction
//
void EvaluateInternFunction(char* _funcStr)
  {
  char *inbufPtr, *tmpPtr1=NULL, *tmpPtr2;

  inbufPtr = inbuf;
  while (*inbufPtr != 0)
    {
    // Don't evaluate expr within macros
    if (StrStrI(inbuf, "@") != 0) break; 

    if ((inbufPtr=StrStrI(inbuf, _funcStr)) != 0)   // ???? subi adlow, low(-10000)
      {
      // maybe something like: "subi adlow, low(-10000)"
      // Exclude any matches within Comments ';'
      if ((tmpPtr2 = strstr(inbuf, ";")) != NULL)
        {
        // Abort - "_funcStr" is within comment
        if (tmpPtr2 < inbufPtr) break; 
        }
      // parse "subi adlow, low(-10000) ; init low value"
      //       ".DB  DEFINED(symbol0)   ; symbol0 is defined = 1
      if ((tmpPtr1=StrStrI(&inbufPtr[1], _funcStr)) != 0)
        {
        // // Re-init if not within comment
        if (tmpPtr2 > tmpPtr1) inbufPtr = tmpPtr1;  
        }
      tmpPtr1 = skip_leading_spaces(&inbufPtr[strlen(_funcStr)]);
      } // end if
    else break;

    // find braces '()'
    if (lastCharAL != '(') break;      // Abort if no adjacent '('
    else                               // Continue - next is '('
      {            
      // find matching brace ')'
      _k=0;
      for (_i=0; _i<strlen(tmpPtr1); _i++)
        {
        if (tmpPtr1[_i] == '(') _k++;
        if (tmpPtr1[_i] == ')')
          {
          _k--;
          tmpPtr2 = &tmpPtr1[_i];
          }
        if (_k==0) break;
        } // end for

      tmpPtr1++;
      tmpPtr2[0] = 0;                  // 0-Teminate string
      
      // Convert 8bit floating point expression
    if (StrCmpI(_funcStr, "Q7") == 0)
        {
        if (StrStr(tmpPtr1, ".") == NULL && swpass == _PASS1)
          p1errorl(ERR_ILLEXPR, tmpPtr1);
        value = Fpoint2Bin(tmpPtr1, 7); 
        *inbufPtr++ = '0';             // patch 1st letter w/ leading '0..h'
        edbh(inbufPtr, value);         // emit hex value as 2 ASCII digits
        inbufPtr += 2;                 // 2 ASCII digits
        *inbufPtr++ = 'h';             // patch 'h' suffix to indicate RADIX16
        *inbufPtr = 0;                 // 0-terminator
        } // end else if
      // Convert 16bit floating point expression
      else if (StrCmpI(_funcStr, "Q15") == 0)
        {
        if (StrStr(tmpPtr1, ".") == NULL && swpass == _PASS1)
          p1errorl(ERR_ILLEXPR, tmpPtr1);
        value = Fpoint2Bin(tmpPtr1, 15); 
        *inbufPtr++ = '0';             // patch 1st letter w/ leading '0....h'
        edwh(inbufPtr, value);         // emit hex value as 4 ASCII digits
        inbufPtr += 4;                 // 4 ASCII digits
        *inbufPtr++ = 'h';             // patch 'h' suffix to indicate RADIX16
        *inbufPtr = 0;                 // 0-terminator
        } // end else if
      // Extract fractional part of floating point
      else if (StrCmpI(_funcStr, "FRAC") == 0) 
        {
        if (StrStr(tmpPtr1, ".") == NULL && swpass == _PASS1)
          p1errorl(ERR_ILLEXPR, tmpPtr1);
        if (tmpPtr1[0] == '-') value = expr(&tmpPtr1[3]);
        else value = expr(&tmpPtr1[2]);
        *inbufPtr++ = '0';             // patch 1st letter w/ leading '0........h'
        eddh(inbufPtr, value);         // emit hex value as 8 ASCII digits
        inbufPtr += 8;                 // 8 ASCII digits
        *inbufPtr++ = 'h';             // patch 'h' suffix to indicate RADIX16
        *inbufPtr = 0;                 // 0-terminator
        }// end else if
      // Truncate floating point to integer
      else if (StrCmpI(_funcStr, "INT") == 0)  
        {
        if (StrStr(tmpPtr1, ".") == NULL && swpass == _PASS1)
          p1errorl(ERR_ILLEXPR, tmpPtr1);
        if (tmpPtr1[0] == '-') tmpPtr1[2] = 0;
        else tmpPtr1[1] = 0;
        value = expr(tmpPtr1); 
        *inbufPtr++ = '0';             // patch 1st letter w/ leading '0....h'
        edwh(inbufPtr, value);         // emit hex value as 4 ASCII digits
        inbufPtr += 4;                 // 4 ASCII digits
        *inbufPtr++ = 'h';             // patch 'h' suffix to indicate RADIX16
        *inbufPtr = 0;                 // 0-terminator
        } // end else if
      
      else if (StrCmpI(_funcStr, "LWRD") == 0)
        {
        value = expr(tmpPtr1);         // Evalulate the expression
        value &= 0x0000FFFF;           // extract bits[15:0] of dword
        *inbufPtr++ = '0';             // patch 1st letter w/ leading '0....h'
        edwh(inbufPtr, value);         // emit hex value as 2 ASCII digits
        inbufPtr += 4;                 // 2 ASCII digits
        *inbufPtr++ = 'h';             // patch 'h' suffix to indicate RADIX16
        *inbufPtr = 0;                 // 0-terminator
        }
      else if (StrCmpI(_funcStr, "HWRD") == 0)
        {
        value = expr(tmpPtr1);         // Evalulate the expression
        value = (value & 0xFFFF0000) >> 16; // extract bits[31:16] of dword
        *inbufPtr++ = '0';             // patch 1st letter w/ leading '0....h'
        edwh(inbufPtr, value);         // emit hex value as 2 ASCII digits
        inbufPtr += 4;                 // 2 ASCII digits
        *inbufPtr++ = 'h';             // patch 'h' suffix to indicate RADIX16
        *inbufPtr = 0;                 // 0-terminator
        }

      else  if (StrCmpI(_funcStr, "STRLEN") == 0)
        {
        if (*tmpPtr1 == 0)
          {
          *inbufPtr++ = '0';           // patch 1st letter w/ leading '0....h'
          edwh(inbufPtr, strlenDB);    // emit hex value as 4 ASCII digits
          inbufPtr += 4;               // 4 ASCII digits
          *inbufPtr++ = 'h';           // patch 'h' suffix to indicate RADIX16
          *inbufPtr = 0;               // 0-terminator
          }
        else if (*tmpPtr1 == '\x22' && tmpPtr1[strlen(tmpPtr1)-1] == '\x22')
          {
          *inbufPtr++ = '0';           // patch 1st letter w/ leading '0....h'
          edwh(inbufPtr, strlen(++tmpPtr1)-1);
          inbufPtr += 4;               // 4 ASCII digits
          *inbufPtr++ = 'h';           // patch 'h' suffix to indicate RADIX16
          *inbufPtr = 0;               // 0-terminator
          }
        else
          {
          errorp2(ERR_SYNTAX, NULL);
          clepc();
          edpri();
          }
        } // end else if ("STRLEN")
      
      // ABS(expr) Absolute value of a constant expression
      else if (StrCmpI(_funcStr, "ABS") == 0)  
        {
        lvalue = expr(tmpPtr1);
        if (strlen(inbufPtr) >= 10)      // >"ABS(12345)"
          {                              // 32bit
          if ((lvalue & 0x80000000) != 0) lvalue = (ULONG)-lvalue; // Negate value to yield absolute 
          *inbufPtr++ = '0';             // patch 1st letter w/ prefix '0........h'
          eddh(inbufPtr, lvalue);        // emit hex value as 8 ASCII digits
          inbufPtr += 8;                 // 8 ASCII digits (restricted to just overwrite "ABS(12345)"
          *inbufPtr++ = 'h';             // patch 'h' suffix to indicate RADIX16
          }
        else                             // >="ABS(1)"
          {                              // 16bit
          if ((lvalue & 0x8000) != 0) lvalue = (UINT)-lvalue; // Negate value to yield absolute 
          *inbufPtr++ = '0';             // patch 1st letter w/ leading '0....h'
          edwh(inbufPtr, lvalue);        // emit hex value as 4 ASCII digits
          inbufPtr += 4;                 // 4 ASCII digits (restricted to just overwrite "ABS(1)"
          *inbufPtr++ = 'h';             // patch 'h' suffix to indicate RADIX16
          }
        *inbufPtr = 0;                   // 0-terminate stuff
        } // end else if (.."ABS")

      // .IF DEFINED (symbol), same as the shorthand: .IFDEF (symbol)
      else if (StrCmpI(_funcStr, "DEFINED") == 0) 
        {
        if (labfn(tmpPtr1) != FALSE)  value = TRUE;
        else value = FALSE;
        *inbufPtr++ = '0';               // patch 1st letter w/ leading '0....h'
        edbh(inbufPtr, value);           // emit hex value as 2 ASCII digits
        inbufPtr += 2;                   // 2 ASCII digits
        *inbufPtr++ = 'h';               // patch 'h' suffix to indicate RADIX16
        *inbufPtr = 0;                   // 0-terminate stuff
        } // end else if (.."DEFINED")

      // 'EXP2(4)' = '000010h' max 7 ascii chars can be patched rest must be faked
      //             '1<<04h ' is a better patching
      else if (StrCmpI(_funcStr, "EXP2")  ==  0) 
        {
        value = expr(tmpPtr1);           // Evalulate the expression
        if (value < 64)                  // range 00..3F 
          {
          *inbufPtr++ = '1';             // patch 'EXP2(4)' with '1<<4h' 
          *inbufPtr++ = '<';
          *inbufPtr++ = '<';
          }                              // (leading '0' patch not required: 0..1Fh)
        else value = 0;                  // out of range Bites [31:0] set value=0
        edbh(inbufPtr, value);           // emit hex value as 2 ASCII digits
        inbufPtr += 2;                   // 4 ASCII digits
        *inbufPtr++ = 'h';               // patch 'h' suffix to indicate RADIX16
        *inbufPtr = 0;                   // 0-terminator
        }

      // Other functions returning 8bit
      else
        {
        value = expr(tmpPtr1);           // Evalulate the expression

        if (StrCmpI(_funcStr, "LOW")   ==  0)
          value &= 0xFF;                      // extract bits[7:0]   of word
        if (StrCmpI(_funcStr, "HIGH")  ==  0)
          value = (value & 0x0000FF00) >>  8; // extract bits[15:8]  of word
        if (StrCmpI(_funcStr, "BYTE1") ==  0)
          value &= 0xFF;                      // extract bits[7:0]   of word
        if (StrCmpI(_funcStr, "BYTE2") ==  0)
          value = (value & 0x0000FF00) >>  8; // extract bits[15:8]  of word
        if (StrCmpI(_funcStr, "BYTE3") ==  0)
          value = (value & 0x00FF0000) >> 16; // extract bits[23:16] of dword
        if (StrCmpI(_funcStr, "BYTE4") ==  0)
          value = (value & 0xFF000000) >> 24; // extract bits[31:24] of dword
        if (StrCmpI(_funcStr, "PAGE")  ==  0)
          value = (value & 0x003F0000) >> 16; // extract bits[21:16] of dword

        if (StrCmpI(_funcStr, "LOG2")  ==  0) // Shift right
          {
          if (qvalue > 0)
            {                                   
            _i=0;                             // _i = LOG2(qvalue)
            while (qvalue != 1) { qvalue >>= 1; _i++; } 
            value = _i;
            }
          else value = 64;             // Fake max=32 if out of range Bit [63:0]                     
          }

        *inbufPtr++ = '0';             // patch 1st letter w/ leading '0..h'
        edbh(inbufPtr, value);         // append hex value as 2 ASCII digits
        inbufPtr += 2;                 // advance ptr 2 ASCII digits
        *inbufPtr++ = 'h';             // patch 'h' suffix to indicate RADIX16
        *inbufPtr = 0;                 // 0-terminator
        } // end else

      // space out and remove "_funcStr" from inbuf
      for (_i=0; _i<=(tmpPtr2-inbufPtr); _i++) inbufPtr[_i] = SPACE;
      } // end if (lastCharAL)
    } // end while
  } // EvaluateInternFunction

//-----------------------------------------------------------------------------
//
//                      FileDateTimeMac2Str
//
BOOL FileDateTimeMac2Str(char* _localStr)
  {
  // Put string in quotes '_localStr'
  *_patchBufPtr++ = '\x27';            
  for (_i=0; _i<strlen(_localStr); _i++) *_patchBufPtr++ =_localStr[_i];
  *_patchBufPtr++ = '\x27';            
  return(TRUE);
  } // FileDateTimeMac2Str

//-----------------------------------------------------------------------------
//
//                      DateTimeMac2IntStr
//
BOOL DateTimeMac2IntStr(char* _macDatTimStr, WORD _localDateTime)
  {
  if (_localDateTime > 255) // 4 ASCII digits (YEAR)      
    {
    sprintf(&_patchBufPtr[0], "%04d", _localDateTime);
    _patchBufPtr += 4;                  
    }
  else                      // 2 ASCII digits (DAY, MONTH, CENTURY,..)
    {
    sprintf(&_patchBufPtr[0], "%02d", _localDateTime);
    _patchBufPtr += 2;                  
    }
  *_patchBufPtr = 0;        // terminate expr string

  return(TRUE);             // No additional chars to be skipped
  } // DateTimeMac2IntStr

//-----------------------------------------------------------------------------
//
//                      EvaluatePredefinedMacro
//
// Support for Pre-defined assembler macros:
//
// __AVRASM_VERSION__           Assembler version, (1000*major + minor)
//
// The effect of the format directives should be tested. It is
//  recommended to put the following line in the source file for testing this:
//  #message "__DATE__ =" __DATE__ "__TIME__ =" __TIME__
//
// Some relevant strftime() format specifiers are NOT SUPPORTED.
// (see strftime(3) manual page for full details):
// • %Y - Year, four digits
// • %m - Month number (01-12)
// • %b - Abbreviated month name
// • %B - Full month name
// • %d - Day number in month (01-31)
// • %a - Abbreviated weekday name
// • %A - Full weekday name
// • %H - Hour, 24-hour clock (00-23)
// • %M - Minute (00-59)
// • %S - Second (00-59)
//
// typedef struct _SYSTEMTIME {
//   WORD wYear;
//   WORD wMonth;
//   WORD wDayOfWeek;
//   WORD wDay;
//   WORD wHour;
//   WORD wMinute;
//   WORD wSecond;
//   WORD wMilliseconds;
// } SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;
//
// Build date. Format: "dd:mm:yyyy" ("Jun 28 2004"). 
// (char*)__DATE__   (int)(__CENTURY__  __YEAR__  __MONTH__  __DAY__)
//
// Build time. Format: "HH:MM:SS". 
// (char*)__TIME__   (int)(__HOUR__  __MINUTE__  __SECOND__)
//
// __FILE__                     Source file name
//
// __PART_NAME__ __partname__   partname corresponds to the value of
//                              PART_NAME__. Example: #ifdef __ATmega8__
//                              (#pragma)
//
// __CORE_VERSION__             AVR core version (#pragma)
// __CORE_coreversion__         coreversion corresponds to the value of
//                              _CORE_VERSION__. Example: #ifdef __CORE_V2__
//                              (#pragma)
//
BOOL EvaluatePredefinedMacro(char* _macStr)
  {
  BOOL bResult = FALSE;

  char *tmpPtr2;
  int inbufLead;

  SYSTEMTIME stLocal;
 
  // Receive the system's time & date.
  GetSystemTime(&stLocal);

  _patchBufPtr = _patchBuf; // Init pointers
  inbufPtr = inbuf;

  while (*inbufPtr != 0)    // Allow multiple strings
    {
    bResult = FALSE;        // initially FALSE for each new loop

    // Don't evaluate expr within macro
    if (StrStrI(inbuf, "@") != 0) break; 

    if ((inbufPtr=StrStr(inbuf, _macStr)) != 0)
      {
      // Exclude any matches within Comments ';'
      if ((tmpPtr2 = strstr(inbuf, ";")) != NULL)
        {
        // Abort - "_macStr" is within comment
        if (tmpPtr2 < inbufPtr) break; 
        }
      }                                                                
    else break;             // _macStr not found

    inbufLead = inbufPtr - inbuf;
    for (_i=0; _i<inbufLead; _i++) *_patchBufPtr++ = inbuf[_i];

    // __FILE__
    if (StrCmp(_macStr, "__FILE__") == 0)
      bResult = FileDateTimeMac2Str(szSrcFileName);

    // __DATE__ 
    else if (StrCmp(_macStr, "__DATE__") == 0)
      bResult = FileDateTimeMac2Str(lh_date);

    // __TIME__
    else if (StrCmp(_macStr, "__TIME__") == 0)
      bResult = FileDateTimeMac2Str(lh_time);

    // __DAY__
    else if (StrCmp(_macStr, "__DAY__") == 0)
      bResult = DateTimeMac2IntStr(_macStr, stLocal.wDay);

    // __MONTH__
    else if (StrCmp(_macStr, "__MONTH__") == 0)
      bResult = DateTimeMac2IntStr(_macStr, stLocal.wMonth);

    // __CENTURY__ (..for the time being ..)
    else if (StrCmp(_macStr, "__CENTURY__") == 0)
      bResult = DateTimeMac2IntStr(_macStr, (stLocal.wYear/100)+1); 

    // __YEAR__
    else if (StrCmp(_macStr, "__YEAR__") == 0)
      bResult = DateTimeMac2IntStr(_macStr, stLocal.wYear);

    // __HOUR__
    else if (StrCmp(_macStr, "__HOUR__") == 0)
      bResult = DateTimeMac2IntStr(_macStr, stLocal.wHour);

    // __MINUTE__
    else if (StrCmp(_macStr, "__MINUTE__") == 0)
      bResult = DateTimeMac2IntStr(_macStr, stLocal.wMinute);

    // __SECOND__
    else if (StrCmp(_macStr, "__SECOND__") == 0)
      bResult = DateTimeMac2IntStr(_macStr, stLocal.wSecond);

    // ----------------------------------------------------
    // Update inbuf if any predefined macro was encountered
    if (bResult == TRUE)
      {
      for (_i=(inbufLead+strlen(_macStr)); _i<INBUFLEN; _i++)
        *_patchBufPtr++ = inbuf[_i];
      for (_i=0; _i<INBUFLEN; _i++) inbuf[_i] = _patchBuf[_i];
      } // end if (bresult)

    inbufPtr += strlen(_patchBuf);  // advance inbuf pointer
    } // end while

  return(bResult);   // bResult reflects last loop
  } // EvaluatePredefinedMacro

//-----------------------------------------------------------------------------
//
//                      dcskp
//
// Skip delimiters from a buffer (incl. TAB=09h < 20h).
// _inbuf_ptr = fetch address
// exit when CR or first non-delimiter is found
//
char* dcskp(char* _inbuf_ptr)
  {
  while (*_inbuf_ptr <= SPACE || *_inbuf_ptr == COMMA)  // Skip cntl, comma
    {
    if (*_inbuf_ptr == CR || *_inbuf_ptr == ';') break; // Done if CR, semicolon
    _inbuf_ptr++; 
    } // end while

  return(_inbuf_ptr);
  } // dcskp

//-----------------------------------------------------------------------------
//
//                      dctrf
//
// Transfer from one buffer to another.
// _inbuf_ptr = fetch address
// _outbuf_ptr = put address
// _len = max length
//
// The transfer stops if max exceeded or when delimiter is found.
// Space, Comma, Semicolon or any control-character is delimiter.
// If a Colon (i.e. Label) is found, it is always transferred
// (possibly replacing the last character), and will also act as a 
// delimter.
//
// on exit: _inbuf_ptr points to the actual delimiter,
//          or behind it if the delimiter is a Colon.
//          lastCharAL contains the last character (or Colon) transferred
//
char* dctrf(char* _inbuf_ptr, char* _outbuf_ptr, int _len)
  {
  int _strexpFlag = 0; // String switch

  // Skip and return if reached end of inbuf line
  if (
      (_inbuf_ptr[0] == 0) || 
      (_inbuf_ptr[1] == 0  && _inbuf_ptr[0] == LF) ||
      (_inbuf_ptr[2] == 0  && _inbuf_ptr[0] == CR  && _inbuf_ptr[1] == LF)
     )
    {
    return(_inbuf_ptr);
    }

  // Parse inbuf line: Skip leading cntls
  _inbuf_ptr = dcskp(_inbuf_ptr);                

  for (_i=0; _i<_len; _i++) _outbuf_ptr[_i] = 0; // Clear _outbuf

  _i=0;
  while (_i<_len)
    {
    // Always stop on end-of-line
    if (*_inbuf_ptr == CR) break;

    // Delimiter-checking - Stop on cntls, space, comma, comment and colon
    // Always stop on comma and comment
    if (_strexpFlag == 0 && (*_inbuf_ptr == COMMA || *_inbuf_ptr == ';')) break;

    // If operands: only stop on Cntls (allowing TABs), but transfer spaces
    // Otherwise (no operands): stop on cntls and space
    if (_operFlag == 1)                            
      {                                            
//ha//      if (*_inbuf_ptr == TAB) *_inbuf_ptr = TAB; // Don't convert TABs to SPACEs
      if (*_inbuf_ptr == TAB) *_inbuf_ptr = SPACE; // convert TABs to SPACEs
      else if (*_inbuf_ptr < SPACE) break;         // stop on other ctrls
      }                                            
    else if (_operFlag == 0 && *_inbuf_ptr <= SPACE) break;

    // Check if start-of-string
    if (_strexpFlag == 0 && (*_inbuf_ptr == STRNG || *_inbuf_ptr == '('))
      _strexpFlag = 1;              // Delimiter-checking = OFF within string
                                    
    // Check if end-of-string
    else if (_strexpFlag == 1 && (*_inbuf_ptr == STRNG || *_inbuf_ptr == ')'))
      _strexpFlag = 0;              // Delimiter-checking = ON

    *_outbuf_ptr++ = *_inbuf_ptr++; // Transfer the buffer contents

    // Stop if label found (=colon) 
    if (_strexpFlag == 0 && *_inbuf_ptr == ':') break;
 
    _i++;
    } // end while (_i<len)
  
  // Check error condition (see bytval*.cpp) e.g. "INC R18, ;text" <<<ERROR..
  if (lastCharAL == COMMA && (*_inbuf_ptr == ';' || *_inbuf_ptr<= SPACE))
    lastCharAX =  lastCharAL;

  lastCharAL = *_inbuf_ptr;         // Save last char

  // Point to the actual delimiter, or behind it, if it is a Colon.
  if (*_inbuf_ptr == ':') _inbuf_ptr++;  // Point behind the colon

  // Remove any possible trailing cntls and spaces
  skip_trailing_spaces(_outbuf_ptr);     
  return(_inbuf_ptr);
  } // dctrf

//-----------------------------------------------------------------------------
//
//                  SrcBufferGetline (File handle method)
//
// Three differnt implematations:
// 1: filestream solution - 'SrcFile.getline(inbuf, INBUFLEN-3)'
//    (fast, but potential buffer overflow when Src-Line > INBUFLEN).
//
// 2: filehandle solution - 'GetLine(Iclfh, inbuf, INBUFLEN-3)'
//    (slow, but no buffer overflow possible).
//
// 3: fileBuffer solution - 'GetLine(Iclfh, inbuf, INBUFLEN-3)'
//    (fast, no buffer overflow possible).
//
BOOL SrcBufferGetline()
  {
  int b_result = TRUE;           // assume src line fits into inbuf
  int _r; // local index counter

//ha//  // Read next source line into inbuf (terminated with CR,LF,0)
//ha//  _r=0;
//ha//  while (*pszSrcFilebuf != LF)
//ha//    { 
//ha//    if (_r < INBUFLEN-3) inbuf[_r++] = *pszSrcFilebuf++;  // store src line
//ha//    else { pszSrcFilebuf++; b_result = FALSE; } // discard (truncate src line)
//ha//    }
//ha//  pszSrcFilebuf ++; // skip LF in src file buffer
//ha//
//ha//  // Terminate inbuf with CR,LF,0
//ha//  if (inbuf[_r-1] != CR) inbuf[_r++] = CR; // ensure CR
//ha//  inbuf[_r]   = LF;                        // store LF
//ha//  inbuf[_r+1] =  0;                        // 0-terminator


  // Read tne next source line (0-terminated -->'.getline' compatible)
  _r=0;
  while (*pszSrcFilebuf != LF)
    { 
    if (_r < INBUFLEN) pszFileInbuf[_r++] = *pszSrcFilebuf++;  // like 'getline'
    else { pszSrcFilebuf++; b_result = FALSE; } // discard (truncate src line)                                     // discard
    } // end while
  pszSrcFilebuf++;  // skip LF in src file buffer

  // Terminate inbuf with 0 only (no CRLF)
  if (pszFileInbuf[_r-1] != CR) pszFileInbuf[_r] = 0; // 0-terminator ('getline')
  else pszFileInbuf[_r-1] = 0;    // discard CRLF and 0-terminate     ('getline')

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha////if (_debugSwPass == _PASS2)
//ha//{
//ha////ha//printf("inbuf ");
//ha////ha//DebugPrintBuffer(inbuf, strlen(inbuf)+3);
//ha//printf("pszFileInbuf ");
//ha//DebugPrintBuffer(pszFileInbuf,  strlen(pszFileInbuf)+3);
//ha//DebugStop(2, "SrcBufferGetline()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
  return(b_result);
  } // SrcBufferGetline


//-----------------------------------------------------------------------------
//
//                 read_source_line (file stream method)
//
// Two differnt implematations:
// 1: filestream solution - 'SrcFile.getline(inbuf, INBUFLEN-3)'
//    (fast, but potential buffer overflow when Src-Line > INBUFLEN).
//
// 2: filehandle solution - 'GetLine(Iclfh, inbuf, INBUFLEN)'
//    (slow, but no buffer overflow possible).
//

// Fix for getline filestream solution:
// Assuming that no single source line will consist of >32K chars...
//ha//#define LONGLINE 32*1024 // !! 'getline' bug fix !!

void read_source_line()
  {
  int Srcbytesrd, Iclbytesrd;

  for (_i=0; _i<INBUFLEN; _i++) inbuf[_i] =0; // Clear inbuf
  if (srcEOF == TRUE)
    {
    ///////////////////////////////////////////
    // Missing end of comment block "*/"     //
    if (swbegincom == 1)                     //
      {                                      //
      errorp2(ERR_SYNTAX, "/* comment */");  //
      // fake end of block "*/"              //
      inbuf[0] = '*'; inbuf[1] = '/';        //
      lineAppendCRLF(&inbuf[strlen(inbuf)]); //  
      srcEOF = FALSE;                        //
      return;                                //
      }                                      //
    ///////////////////////////////////////////

    // Fake a .EXIT directive since it is missing at the end of source file.
    // To termitnate the assembly gracefully, an exit directive is needed.  
    for (_i=0; _i<=sizeof(END_str); _i++) inbuf[_i] = END_str[_i];
    lineAppendCRLF(&inbuf[strlen(inbuf)]);        
    srcEOF = FALSE;
    return;
    }

  ///////////////////////////////////////////////////////////////////////
  //                                                                   //
  // Macro processing Pass1/2: macro.getline streaming                 //
  // Currently cascading ONE macro within a master macro is allowed    //
  //                                                                   //
  // typedef struct tagMACCASCADE { // Macro cascading structure       //
  //   char* macXferBufPtr;         // Ptr to macro Xfer line string   //
  //   int   macXferBufLen;         // Len of macro Xfer contents      //
  //   char* macSaveBufPtr;         // Ptr to complete macro contents  //
  //   int   macSaveBufLen;         // Len of complete macro contents  //
  // } MACCASCADE, *LPMACCASCADE;                                      //
  //                                                                   //
  // Example:                                                          //
  //                                                                   //
  // .MACRO init            ;; Macro 'master'                          //
  //    rcall initHW        ;; a                                       //
  //    rcall delay         ;; b                                       //
  //                        ;; Macro 'slaves' within 'master'          //
  //    WR_CMD  (1<<_F)|(0<<_F_8B)                               ;;1   //
  //    WR_CMD  (1<<_F)|(0<<_F_8B)|(1<<_F_2L)                    ;;2   //
  //    WR_CMD  (1<<_CLR)                                        ;;3   //
  //    WR_CMD  (1<<_ENTRY_MODE)|(1<<LCD_ENTRY_INC)              ;;4   //
  //    WR_CMD  (1<<_ON)|(1<<_ON_DISPLAY)|(0<<_ON_BLINK) ;;5           //
  //    WR_CMD  (1<<_HOME)                                       ;;6   //
  // .ENDM                                                             //
  // ================================================================= //
  //                                                                   //
  // .MACRO wr_cmd          ;; Macro 'slave'                           //
  //    ldi R17,@0          ;; c                                       //
  //    rcall cmd_wr        ;; d                                       //
  // .ENDM                                                             //
  // ================================================================= //
  //                                                                   //
_MACRO_CHECK_:                                                         //
  if (macrocascade[swexpmacro].macXferBufLen > 0 && swexpmacro > 0)    //     
    {                                                                  //
    // Read a macro source line (Note: macrocascade[0] = reserved)     //
    _i = 0;                                                            //
    while (*macrocascade[swexpmacro].macXferBufPtr != 0)               //     
      {                                                                //
      if (macrocascade[swexpmacro].macXferBufLen >0)                   //
        {                                                              //
        inbuf[_i] = *macrocascade[swexpmacro].macXferBufPtr++;         //       
        macrocascade[swexpmacro].macXferBufLen--;                      //     
        // Synchronize the with the previous macro ptr                 //
        macrocascade[swexpmacro].macSaveBufPtr++;                      //             
        macrocascade[swexpmacro].macSaveBufLen--;                      //             
        }                                                              //
      _i++;                                                            //
      } // end while                                                   //
                                                                       //
    // Skip and read the 0-terminator                                  //
    _i++;                                                              //
    inbuf[_i] = *macrocascade[swexpmacro].macXferBufPtr++;             //                 
    macrocascade[swexpmacro].macXferBufLen--;                          //   
    // Synchronize the with the previous macro ptr                     //
    macrocascade[swexpmacro].macSaveBufPtr++;                          //           
    macrocascade[swexpmacro].macSaveBufLen--;                          //           
                                                                       //
    // Feed and process the expanded macro line                        //
    return;                                                            //
    } // end if                                                        //
                                                                       //
  // Check if there are cascaded 'slave' macros                        //
  // Decrement macro cascade index if current macro xferred            //
  else if (swexpmacro >1 && macrocascade[swexpmacro].macSaveBufLen==0) //
    {                                                                  //
    swexpmacro--;                                                      //
    macrocascade[swexpmacro].macXferBufLen = macrocascade[swexpmacro].macSaveBufLen;              
    macrocascade[swexpmacro].macXferBufPtr = macrocascade[swexpmacro].macSaveBufPtr;              
    goto _MACRO_CHECK_;                                                //
    } // end else if                                                   //
                                                                       //
  // macrocascade[0].macXferBufLen=0: Macro done - continue normal     //
  else swexpmacro = FALSE;                                             //
  ///////////////////////////////////////////////////////////////////////

  // pszFileInbuf[]
  // Note: Necessary because the function '.getline(..)' always reads until CRLF.
  // This behaviour can cause a buffer overflow, freezing the .SRC input.
  // Allocate enough to prevent buffer overflow for a (very long 32K!) text line.
  for (_i=0; _i<INBUFLEN; _i++) pszFileInbuf[_i] = 0; // Clear inbuf chunk

  // read source file .ASM
  if (swicl == 0)
    { 
    // ------------------
    // File handle method
    // ------------------
    // ERROR/WARNING if Source line is too long
//ha//    if (!SrcBufferGetline()) warno = WARN_TOOLONG;  // WARNING only

    // ------------------
    // File stream method
    // ------------------
    SrcFile.getline(pszFileInbuf, LONGLINE);
    errchk(szSrcFileName, GetLastError());
    // Copy the line from the allocated buffer into the static inbuf[]
    //  and truncate the line if it's too long (prevent buf overflow).
    for (_i=0; _i<INBUFLEN; _i++) inbuf[_i] = pszFileInbuf[_i];  // fill inbuf
    if (sizeof(pszFileInbuf) > INBUFLEN) inbuf[INBUFLEN] = 0;    // truncate

    _curline++;                              // current file line counter

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//if (swpass == _PASS2)
//ha//{
//ha////ha//printf("inbuf ");
//ha////ha//DebugPrintBuffer(inbuf, strlen(inbuf));
//ha////ha////DebugPrintBuffer(inbuf, 100);
//ha//printf("pszSrcFilebuf ");
//ha//DebugPrintBuffer(pszSrcFilebuf, 100);
//ha//DebugStop(2, "read_source_line()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
    }

  // read include file .INC
  else if (swicl != 0)
    {
    IclFile.getline(pszFileInbuf, LONGLINE); // getline filestream solution
    errchk(pszCurFileName, GetLastError());
    // Copy the line from the allocated buffer into the static inbuf[]
    //  and truncate the line if it's too long (prevent buf overflow).
    for (_i=0; _i<INBUFLEN; _i++) inbuf[_i] = pszFileInbuf[_i];  // fill inbuf
    if (sizeof(pszFileInbuf) > INBUFLEN) inbuf[INBUFLEN] = 0;    // truncate

    _curline++;                              // current file line counter

    // A strange curiousity
    // .INC and comment line within .INC Example:
    // see Project "__AVR-Matopeli_XASM\pinnit.inc"
    //  Within "pinnit.inc" (Linux file LF only)
    //  include "tn85def.inc" (Windows CRLF):
    //    [.include "tn85def.inc"]  >>> Reads [tn85def.inc"] as opcode. Why ???.
    //    [.include "tn85def.inc" ;0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ]
    //                            >>> Reads [OPQRSTUVWXYZ] as opcode. Why ???.  
    // Must read junk and discard as garbage ??? getline() problem ???
    // The problem:
    //   "pinnit.inc" is a Linux file ending lines with LF.
    //   "tn85def.inc" is a Windows file ending lines with CRLF.
    //   When "tn85def.inc" is included within "pinnit.inc" a mixing of
    //   file ending occurs causing the strange behaviour described above.
    // 
    // If you are mixing files from different platforms having
    // lines ending with LF and lines ending with CRLF or, even worse, ending
    // just with last char >= SPACE, this causes such a problem.
    // As long as reading lines with ifstream.getline() there's no workaround.
    // Alternatively we must read in binary mode and handle line endings ourselves.
    // The following "workaround" for mixed LF & CRLF is commented out again,
    // as it will swallow one line with CRLF in plain windows files. 
    //  
    if (iclEOF == TRUE)                        // .INC within .INC reaches EOF
      {
      ///////////////////////////////////////////
      // Missing end of comment block "*/"     //
      if (swbegincom == 1)                     //
        {                                      //
        errorp2(ERR_SYNTAX, "/* comment */");  //
        // fake end of block "*/"              //
        inbuf[0] = '*'; inbuf[1] = '/';        //
        lineAppendCRLF(&inbuf[strlen(inbuf)]); //  
        srcEOF = FALSE;                        //
        return;                                //
        }                                      //
      ///////////////////////////////////////////

//ha//???//     IclFile.getline(pszFileInbuf, LONGLINE); // Dummy read to discard garbage
      iclEOF = FALSE;                          // reset EOF flag
      }
    } // else if (swicl)

  // ERROR/WARNING Source line too long
  if (strlen(pszFileInbuf) > INBUFLEN)       // Must Abort if too long, symbol buffer overwrite!?
    {
    warno = WARN_TOOLONG;                    // WARNING only
    //exit(SYSERR_ABORT);                    // ERROR abort??
    }

  // At this point:
  // The contents of the allocated buffer were saved, continuing with src line in 'inbuf'

  // ---------------
  // filestream .ASM
  // ---------------
  if (SrcFile.eof() && swicl == 0)           // filestream .ASM
    {
    // Arriving here if 'END/.EXIT' directive (no CRLF) is encountered:
    // inbuf[] contains "END" or ".EXIT" as 0-terminated string,
    // and got a 'SrcFile.eof' from 'SrcFile.getline'.
    if (AtmelFlag != 0)  // Atmel: Fake ".EXIT" and allow termination w/o error
      {
      lineAppendCRLF(&inbuf[strlen(inbuf)]); // Terminate the last Instruction        
      srcEOF = TRUE;                         // This will pseudo read .EXIT next
      p12CheckEndmacro();  // Check for incomplete macro defiinitions
//ha//      p12CCheckIfend();    // Check for incomplete conditions
      } // end if (AtmelFlag)

    else                   // Issue an ERROR if no "END" directive is found
      {  
      p12CheckEndmacro();  // Check for incomplete macro defiinitions
      p12CCheckIfend();    // Check for incomplete conditions
      inbuf_ptr = skip_leading_spaces(inbuf);
      if (StrCmpI(inbuf_ptr, END_str) != 0) // 0-terminated 'END/EXIT' in inbuf1[]?
        {                                   // No - exit with error.
        CloseSrcFile();                     // Close include input file .ASM
        // >>>>>>>>> ERROR: No END/EXIT directive found! <<<<<<<<<<
        warno = WARN_CHKEXIT; 
        warnSymbol_ptr = END_str; 
        warnout();
        exit(SYSERR_ABORT);                           // Exit with error (SYSERR_ABORT)
        }
      else // Restore CRLF in 'END/.EXIT' source line 
        {
        // Must patch inbuf w/ CR,LF,0 because it's read in text-line mode
        //  or it may have been truncated with trailing 0s
        lineAppendCRLF(&inbuf[strlen(inbuf)]);        
        }
      } // end else
    } // end if (SrcFile.eof)

  // ---------------
  // filestream .INC
  // ---------------
  else if (IclFile.eof() && swicl != 0)     // filestream .INC
    {
    iclEOF=TRUE;
    // NOTE: An EOF generated by IclFile.getline(..) can have two causes:
    // (1) If the last line read before was terminated (as expected) with CRLF,
    //     then the line just read read will be empty, i.e. no data in inbuf, length=0.
    //     *IclFile must be closed 'CloseIclFile()' because no more data read.
    // (2) if (for some obscure reason) the line just read was not correctly
    //     terminated and ended with any char (but not CRLF), then this line
    //     is read into inbuf[] and the EOF flags that inbuf[] holds an
    //     incorrectly terminated last line, i.e. data of length >0. 
    //     *IclFile must be left open, because data was read.
    //
    // If (1) Resume reading the next line from .ASM file into inbuf[]
    //        since IclFile.getline(..) read no valid data, but caused an EOF. 
    // If (2) Patch the missing CRLF to correctly terminate the line in inbuf[]
    //        and resume processing this CRLF terminated line. 
    //        The next line read then causes an EOF as described at cause (1). 
    //
    p12CheckEndmacro();    // Check for incomplete macro defiinitions
    //ha//p12CheckIfend(); // NO check for incomplete .IF(N)DEF (may be around .INC)

    if (inbuf[0] == 0)
      {
      CloseIclFile();      // Close include input file .INC
      iclEOF=FALSE;
      //Recoursively read .SRC line next
      read_source_line();  // Read next line from source file into empty inbuf[]
      }
    else                   // Restore CRLF in .INC source line 
      {
      // Must patch inbuf w/ CR,LF,0 because it's read in text-line mode
      //  or it may have been truncated with trailing 0s
      lineAppendCRLF(&inbuf[strlen(inbuf)]);        
      }
    } // end else if (IclFile.eof)

  // Aside from the "EOF" issue lines read by means of IclFile.getline(..)
  // principally truncate CRLF, such that we must restore CRLF on all lines
  // before continuing line parsing.
  else  
    {
    // ------------------
    // File stream method
    // ------------------
    // Must restore inbuf w/ CR,LF,0 because it's read in text-line mode
    //  or it may have been truncated with trailing 0s
    lineAppendCRLF(&inbuf[strlen(inbuf)]);
    }                          
  } // read_source_line

//-----------------------------------------------------------------------------
//
//                      Decomp
//
//ha//int swbegincom=0;  // "/*.." comment begin flag
int swendcom=0;    // "..*/" comment end flag

void Decomp()
  {
  erno = 0;     // Clear error number
  warno = 0;    // Clear warning number
  swcom = 0;    // Clr comment switch

  // Read either from .ASM file or from .INC  file
  read_source_line();

  cllin();      // Space-out Listing line ListBuf
  cllib();      // Clr split area to 0`s

  // Transfer inbuf[] unchanged to line print buffer - ListBuf[]
  // (including CRLF and the 0-terminator for strlen(szListBuf) calculation)
  for (_i=0; _i<=strlen(inbuf); _i++) szListBuf[lpSRC+_i] = inbuf[_i];
  pszListBuf = &szListBuf[lpSRC];

  edit_inc_linenr();                               // Edit line number
  if (swicl != 0) szListBuf[lpMARK] = INCLUDE_IDM; // Emit Include file line marker

  // Blank lines and comment lines are ignored in pass1 / pass2
  while (*pszListBuf == SPACE || *pszListBuf == TAB) pszListBuf++;

  // ----------------------------------------------
  // Terminate "/*..*/" comment and continue normal
  if (swendcom == 1) // "/*..*/" comment
    {
    swcom = 0;
    swbegincom = 0;
    swendcom = 0;
    } // fall thru..               

  // ---------------------------------------------------
  // Standard Assembler semicolon comment introducer ';'
  if (*pszListBuf == CR || *pszListBuf == ';')
    {
    swcom = 1;       // Ignore blank/comment line
    return;          // Nothing left to do in pass1 / pass2
    }              

  // ----------------------------------------------------------------------------
  // C-style "/*..*/" multiple lines comment error handling (Atmel AVR assembler)
  // End Comment "*/" single line
  if ((pszListBuf[0] == '/' || pszListBuf[0] == '*') && strstr(&pszListBuf[1],"*/") != NULL)
    {
    errorp2(ERR_SYNTAX, "\x22*/\x22 - Single line comment starts with ';'");
    swcom = 0;       // Ignore the terminating "/*..*/" comment line
    swbegincom = 0;
    swendcom = 0;    // Prepare termination of "/*..*/" comment
    return;
    }
  // End Comment "*/" without beginning comment "/*"
  if (pszListBuf[0] == '*' && pszListBuf[1] == '/' && swbegincom == 0)
    {
    errorp2(ERR_SYNTAX, "\x22*/\x22");
    swcom = 0;       // Ignore the terminating "/*..*/" comment line
    swbegincom = 0;
    swendcom = 0;    // Prepare termination of "/*..*/" comment
    return;
    }
 
  // Double begin comment "/*"
  if (pszListBuf[0] == '/' && pszListBuf[1] == '*' && swbegincom == 1)
    {
    errorp2(ERR_SYNTAX, "\x22/*\x22");
    swcom = 0;         // Ignore the starting "/*..*/" comment line
    swbegincom = 0;
    swendcom = 0;      // Prepare termination of "/*..*/" comment
    }

  // -------------------------------------------------------------------
  // Allow C-style "/*..*/" multiple lines comment (Atmel AVR assembler)
  // Begin Comment block "/*"
  if (*pszListBuf == '/' && pszListBuf[1] == '*') 
    {
    if (swbegincom == 1)
      {
      errorp2(ERR_SYNTAX, "/*");
      swcom = 0;
      swbegincom = 0;
      swendcom = 0;
      }
    else
      {
      swcom = 1;       // set comment switch, ignore blank/comment line(s)
      swbegincom = 1;  // Begin "/*..*/" comment block
      swendcom = 0;
      return;          // Nothing left to do in pass1 / pass2
      }
    }              
  // End Comment "*/"
  else if (*pszListBuf == '*' && pszListBuf[1] == '/') 
    {
    swcom = 1;         // Ignore the terminating "/*..*/" comment line
    swbegincom = 0;
    swendcom = 1;      // Prepare termination of "/*..*/" comment block
    return;            // Nothing left to do in pass1 / pass2
    }

  // Pending comment block termination "/*..*/" in progress
  if (swbegincom == 1)
    { 
    swcom = 1;         // set comment switch
    return;            // Nothing left to do in pass1 / pass2
    }
  // -------------------------------------------------------------------

  //-----------------------------------------------------------------------,
  //                   Start manipulation of inbuf                         |
  //                                                                       |
  // Here is a good point where some manipulation of inbuf can be done     |
  // in order to achieve compatibility with later parsing and evaluation.  |
  //                                                                       |
  //-----------------------------------------------------------------------'
  char *inbufPtr, *tmpPtr1, *tmpPtr2;
  
  // Not if a macro is being defined!
  if (!swdefmacro)
    {  
    if (AtmelFlag != 0) // Atmel/Microchip assembler specific 
      {
      // 1a) Eliminate C-style comments "//" found in some Atmel *.INC files
      if ((inbufPtr=strstr(inbuf, "//")) != 0) *inbufPtr  = ';';
      // 1b) Ignore C-style blank/comment line - skip pass1 / pass2 processing
      if (pszListBuf[0] == '/' && pszListBuf[1] == '/') { swcom = 1; return; }
      
      // 2) Eliminate C-style "string" (Atmel/Microchip '_AVRASM2' Assembler) //ha//
      //    and force an ASM-style 'string'                                   //ha//
      //    (see p1db() and p2db() in pass1/2.cpp)                            //ha//
      } // end if (AtmelFlag)

    // 3) Assembler specific operands
    EvaluateOperand("PC");
    
    // 4) Internal assembler functions
    EvaluateInternFunction("LOW");
    EvaluateInternFunction("HIGH");
    EvaluateInternFunction("BYTE1");
    EvaluateInternFunction("BYTE2");
    EvaluateInternFunction("BYTE3");
    EvaluateInternFunction("BYTE4");
    EvaluateInternFunction("LWRD");    // Returns bits 15:0  of an expression
    EvaluateInternFunction("HWRD");    // Returns bits 31:16 of an expression
    EvaluateInternFunction("PAGE");    // Returns bits 21:16 of an expression
    EvaluateInternFunction("EXP2");
    EvaluateInternFunction("LOG2");
    EvaluateInternFunction("DEFINED");
    EvaluateInternFunction("ABS");     // Absolute value of a constant expression
    EvaluateInternFunction("STRLEN");  // .DB statement operand(s) length in bytes
    EvaluateInternFunction("INT");     // Truncate floating point expression
    EvaluateInternFunction("FRAC");    // Extract floating point expression
    EvaluateInternFunction("Q7");      // Convert floating point expression
    EvaluateInternFunction("Q15");     // Convert Floating point expression

    // 5) Pre-defined assembler macros
    if (strstr(inbuf, "__") != NULL)   // Don't check these macros if not necessary
      {
      //errSymbol_ptr = "\x22__";
      EvaluatePredefinedMacro("__FILE__");   // String Source file name
      EvaluatePredefinedMacro("__DATE__");   // String Build date. Format: "DD\MM\YYYY".
      EvaluatePredefinedMacro("__TIME__");   // String Build time. Format: "HH:MM:SS".
      EvaluatePredefinedMacro("__CENTURY__");// Integer (=21, for the time being..)
      EvaluatePredefinedMacro("__YEAR__");   // Integer
      EvaluatePredefinedMacro("__MONTH__");  // Integer
      EvaluatePredefinedMacro("__DAY__");    // Integer
      EvaluatePredefinedMacro("__HOUR__");   // Integer
      EvaluatePredefinedMacro("__MINUTE__"); // Integer
      EvaluatePredefinedMacro("__SECOND__"); // Integer
      } // end if

    // 6) '#if defined' / '#if !defined' aliases
    //    (converting '#if defined' --> '#if_defined' - see opcodeAVR.cpp)
    if ((inbufPtr=StrStrI(inbuf, "#if defined")) != 0)
      {
      inbufPtr[3]  = '_';
      }
     if ((inbufPtr=StrStrI(inbuf, "#if !defined")) != 0)
      {
      inbufPtr[3]  = '_';
      }

    // 7) '#elif defined' / '#elif !defined' aliases
    if ((inbufPtr=StrStrI(inbuf, "#elif defined"))  != 0)
      {
      inbufPtr[5]  = '_';
      }
    if ((inbufPtr=StrStrI(inbuf, "#elif !defined")) != 0)
      {
      inbufPtr[5]  = '_';
      }

    // 8) ..

    } // end if (!swdefmacro)

  //-----------------------------------------------------------------------,
  //                   End manipulation of inbuf                           |
  //-----------------------------------------------------------------------'

  // Decompose source line: Split into separate fields
  //
  lastCharAL = 0;              // Clear auxiliaries
  lastCharAX = 0;

  for (_i=0; _i<=SYMLEN;  _i++) flabl[_i] = 0; // Clear symbol/label field
  for (_i=0; _i<=OPCDLEN; _i++) fopcd[_i] = 0; // Clear opcode field
  for (_i=0; _i<=OPERLEN; _i++)                // Clear operand fields
    {  
    oper1[_i] = 0; // Clear operand1 field1
    oper2[_i] = 0; // Clear operand2 field2
    oper3[_i] = 0; // Clear operand3 field3
    oper4[_i] = 0; // Clear operand3 field4
    }

  // Fill inbuf data into relevant fields
  inbuf_ptr = dctrf(inbuf, fopcd, OPCDLEN);   // LABEL, FOPCD->FLABL
  inbuf_ptr = dcskp(inbuf_ptr);               // Skip delim`s & leading blanks

  // For "DS","DQ","DD","DW","DB" allow symbols without colon ':'
  // (fake a ':' suffix if it is not present in src-line)
  // This supports the standard syntax of most assemblers.
  // NOTE: Colon, however, will also be accepted (compatibility).
  if (fopcd[0] != 0)
    {
    if (
        (StrCmpNI(&inbuf_ptr[0], DS_str,  strlen(DS_str))  == 0  &&
        (inbuf_ptr[strlen(DS_str)]  <= SPACE || inbuf_ptr[strlen(DS_str)]  == ';')) ||
        (StrCmpNI(&inbuf_ptr[0], DB_str,  strlen(DB_str))  == 0  &&
        (inbuf_ptr[strlen(DB_str)]  <= SPACE || inbuf_ptr[strlen(DB_str)]  == ';')) ||
        (StrCmpNI(&inbuf_ptr[0], DW_str,  strlen(DW_str))  == 0  &&
        (inbuf_ptr[strlen(DW_str)]  <= SPACE || inbuf_ptr[strlen(DW_str)]  == ';')) ||
        (StrCmpNI(&inbuf_ptr[0], DD_str,  strlen(DD_str))  == 0  &&
        (inbuf_ptr[strlen(DD_str)]  <= SPACE || inbuf_ptr[strlen(DD_str)]  == ';')) ||
        (StrCmpNI(&inbuf_ptr[0], DQ_str,  strlen(DQ_str))  == 0  &&
        (inbuf_ptr[strlen(DQ_str)]  <= SPACE || inbuf_ptr[strlen(DQ_str)]  == ';')) ||
        (StrCmpNI(&inbuf_ptr[0], EQU_str,  strlen(EQU_str)) == 0 &&
        (inbuf_ptr[strlen(EQU_str)] <= SPACE || inbuf_ptr[strlen(EQU_str)] == ';')) ||
        (StrCmpNI(&inbuf_ptr[0], DEF_str,  strlen(DEF_str)) == 0 &&
        (inbuf_ptr[strlen(DEF_str)] <= SPACE || inbuf_ptr[strlen(DEF_str)] == ';')) ||
        (StrCmpNI(&inbuf_ptr[0], SET_str,  strlen(SET_str)) == 0 &&
        (inbuf_ptr[strlen(SET_str)] <= SPACE || inbuf_ptr[strlen(SET_str)] == ';'))
       )
      {
      lastCharAL = ':';
      } // end if
    } // end if (fopcd)

  // Check if it's a label (':' suffix)
  if (lastCharAL == ':')                    
    {
    for (_i=0; _i<SYMLEN;_i++) flabl[_i] = fopcd[_i];  // Copy label until ':'
    for (_i=0; _i<=OPCDLEN;_i++) fopcd[_i] = 0;        // Clear opcode field
    inbuf_ptr = dctrf(inbuf_ptr, fopcd, OPCDLEN);      // FOPCD<--inbuf
    }

  inbuf_ptr = dcskp(inbuf_ptr);  // Skip delim`s & leading blanks

  // Don't load operand field with any comment junk or $.DIRECTIVE in fopcd
  if (*inbuf_ptr != ';' && fopcd[0] != 0 &&
      (fopcd[0]  != '$' || fopcd[0] != '.'))         
    {
    _operFlag = 1;
    if (inbuf_ptr != 0) inbuf_ptr = dctrf(inbuf_ptr++, oper1,  OPERLEN); // ->OPER1
    if (inbuf_ptr != 0) inbuf_ptr = dctrf(inbuf_ptr++, oper2,  OPERLEN); // ->OPER2
    if (inbuf_ptr != 0) inbuf_ptr = dctrf(inbuf_ptr++, oper3,  OPERLEN); // ->OPER3              
    if (inbuf_ptr != 0) inbuf_ptr = dctrf(inbuf_ptr++, oper4,  OPERLEN); // ->OPER4
    if (inbuf_ptr != 0) inbuf_ptr = dctrf(inbuf_ptr++, oper5,  OPERLEN); // ->OPER5
    if (inbuf_ptr != 0) inbuf_ptr = dctrf(inbuf_ptr++, oper6,  OPERLEN); // ->OPER6
    if (inbuf_ptr != 0) inbuf_ptr = dctrf(inbuf_ptr++, oper7,  OPERLEN); // ->OPER7              
    if (inbuf_ptr != 0) inbuf_ptr = dctrf(inbuf_ptr++, oper8,  OPERLEN); // ->OPER8              
    if (inbuf_ptr != 0) inbuf_ptr = dctrf(inbuf_ptr++, oper9,  OPERLEN); // ->OPER9              
    if (inbuf_ptr != 0) inbuf_ptr = dctrf(inbuf_ptr++, oper10, OPERLEN); // ->OPER10              
    if (inbuf_ptr != 0) inbuf_ptr = dctrf(inbuf_ptr++, oper11, OPERLEN); // ->OPER11 .. for macro parms @A..@F             
    if (inbuf_ptr != 0) inbuf_ptr = dctrf(inbuf_ptr++, oper12, OPERLEN); // ->OPER12 ..             
    if (inbuf_ptr != 0) inbuf_ptr = dctrf(inbuf_ptr++, oper13, OPERLEN); // ->OPER13 ..             
    if (inbuf_ptr != 0) inbuf_ptr = dctrf(inbuf_ptr++, oper14, OPERLEN); // ->OPER14 ..             
    if (inbuf_ptr != 0) inbuf_ptr = dctrf(inbuf_ptr++, oper15, OPERLEN); // ->OPER15 ..             
    if (inbuf_ptr != 0) inbuf_ptr = dctrf(inbuf_ptr++, oper16, OPERLEN); // ->OPER16 ..             
    _operFlag = 0;
    }

  // Line continuation - expand the line if necessary
  if (StrCmpI(fopcd, DQ_str) == 0 ||
      StrCmpI(fopcd, DD_str) == 0 ||
      StrCmpI(fopcd, DW_str) == 0 ||
      StrCmpI(fopcd, DB_str) == 0)
    {
    if      (swpass == _PASS1) p1LineContinuation();
    else if (swpass == _PASS2) p2LineContinuation();
    }

  // Pre-defined assembler macros
  // Not if a macro is being defined!
  if (!swdefmacro)
    {  
    if (strstr(inbuf, "__") != NULL)   // Don't check these macros if not necessary
      {
      //errSymbol_ptr = "\x22__";
      EvaluatePredefinedMacro("__FILE__");   // String Source file name
      EvaluatePredefinedMacro("__DATE__");   // String Build date. Format: "DD\MM\YYYY".
      EvaluatePredefinedMacro("__TIME__");   // String Build time. Format: "HH:MM:SS".
      //errSymbol_ptr = NULL;
      EvaluatePredefinedMacro("__CENTURY__");// Integer (=21, for the time being..)
      EvaluatePredefinedMacro("__YEAR__");   // Integer
      EvaluatePredefinedMacro("__MONTH__");  // Integer
      EvaluatePredefinedMacro("__DAY__");    // Integer
      EvaluatePredefinedMacro("__HOUR__");   // Integer
      EvaluatePredefinedMacro("__MINUTE__"); // Integer
      EvaluatePredefinedMacro("__SECOND__"); // Integer
      } // end if

    EvaluateOperand("PC");

    // Internal assembler functions
    EvaluateInternFunction("LOW");
    EvaluateInternFunction("HIGH");
    EvaluateInternFunction("BYTE1");
    EvaluateInternFunction("BYTE2");
    EvaluateInternFunction("BYTE3");
    EvaluateInternFunction("BYTE4");
    EvaluateInternFunction("LWRD");    // Returns bits 15:0  of an expression
    EvaluateInternFunction("HWRD");    // Returns bits 31:16 of an expression
    EvaluateInternFunction("PAGE");    // Returns bits 21:16 of an expression
    EvaluateInternFunction("EXP2");
    EvaluateInternFunction("LOG2");
    EvaluateInternFunction("DEFINED");
    EvaluateInternFunction("ABS");     // Absolute value of a constant expression
    EvaluateInternFunction("STRLEN");  // .DB statement operand(s) length in bytes
    EvaluateInternFunction("INT");     // Truncate floating point expression
    EvaluateInternFunction("FRAC");    // Extract floating point expression
    EvaluateInternFunction("Q7");      // Convert floating point expression
    EvaluateInternFunction("Q15");     // Convert Floating point expression

    } // end if (!swdefmacro)

  ///////////////////////////////////////////////////////////
  if (_ActivityMonitorCount++ > 150)  ActivityMonitor(ON); //
  ///////////////////////////////////////////////////////////

  } // Decomp

//-----------------------------------------------------------------------------

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("srcEOF=%d  iclEOF=%d  swicl=%d  pszCurFileName=%s", srcEOF, iclEOF, swicl, pszCurFileName);
//ha//DebugStop(1, "OpenIclFile()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha////if (macroCount >=283)
//ha//if (macroCount > 27)
//ha//{
//ha//printf("macrotab[28].macNameStr=%s\n"
//ha//       " macrotab[28].macBufLen=%d\n"
//ha//       " macrotab[29].macNameStr=%s\n"
//ha//       " macrotab[29].macBufLen=%d\n"
//ha//       " macrotab[30].macNameStr=%s\n"
//ha//       " macrotab[30].macBufLen=%d\n"
//ha//       " macrotab[31].macNameStr=%s\n"
//ha//       " macrotab[31].macBufLen=%d\n",
//ha//        macrotab[28].macNameStr,
//ha//        macrotab[28].macBufLen,
//ha//        macrotab[29].macNameStr,
//ha//        macrotab[29].macBufLen,
//ha//        macrotab[30].macNameStr,
//ha//        macrotab[30].macBufLen,
//ha//        macrotab[31].macNameStr,
//ha//        macrotab[31].macBufLen);
//ha////printf("macrotab[macroCount].macBufPtr ");
//ha////DebugPrintBuffer(macrotab[macroCount].macBufPtr, strlen(macrotab[macroCount].macBufPtr));
//ha//DebugStop(4, "dctrf()", __FILE__);
//ha//}
//--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha////if (_debugSwPass == 2)
//ha//{
//ha//printf("inbufPtr=%s\n tmpPtr1=%s  tmpPtr2=%s  lastCharAL=%02X '%c'   value=%04X\n tmpPtr2-inbufPtr=%d\n", 
//ha//        inbufPtr,     tmpPtr1,    tmpPtr2,    lastCharAL,lastCharAL, value,       tmpPtr2-inbufPtr);
//ha//printf("inbufPtr ");
//ha//DebugPrintBuffer(inbufPtr, 20);
//ha//DebugStop(1, "dcomp()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("flabl ");
//ha//DebugPrintBuffer(flabl, SYMLEN);
//ha//printf("fopcd ");
//ha//DebugPrintBuffer(fopcd, OPCDLEN);
//ha//printf("oper1 ");
//ha//DebugPrintBuffer(oper1, OPERLEN);
//ha//printf("oper2 ");
//ha//DebugPrintBuffer(oper2, OPERLEN);
//ha//printf("oper3 ");
//ha//DebugPrintBuffer(oper3, OPERLEN);
//ha//DebugStop(1, "dcomp()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//--------------------------end-of-c++-module-----------------------------------

