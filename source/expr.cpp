// haXASM - Cross-Assembler for 8bit Microprocessors
// expr.cpp - C++ Developer source file.
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

#include <shlwapi.h>   // StrStrI

#include <sys/stat.h>  // For filesize
#include <iostream>    // I/O control
#include <fstream>     // File control

#include <windows.h>   // For console specific functions

#include "equate.h"
#include "extern.h"    // Variables published in workst.cpp

using namespace std;

// Global variables
char _str[OPERLEN+1];
char _strexp[OPERLEN+1];
char _strnum[OPERLEN+1];

char unary = 0;

int error, symbol_stat;
int _radix = RADIX_10;

// Extern variables and functions
extern void DebugStop(int, char*, char*); // Usage: DebugStop(testNr, "Function()", __FILE__);
extern void DebugPrintBuffer(char*, int); // Usage: DebugPrintBuffer(buffer, count);
extern void DebugPrintSymbolArray(int);   // Usage: DebugPrintSymbolArray(count);

extern char* DQ_str;
extern char* DB_str;

extern void errorp2(int, char*);
extern BOOL label_syntax(char*);
extern UCHAR labfn(char*);

extern void eddh(char*, ULONG);
extern void edwh(char*, UINT);
extern void GetPcValue();

extern void erout();
extern void warnout();

// Forward declaration of functions included in this code module:
BOOL delm1(char*);
BOOL delm2(char*);
BOOL delm3(char*);

char* skip_leading_spaces(char*);

UINT expr(char*);
unsigned long DExpr(char*, int);
unsigned long _DExpr(int);
unsigned long long QExpr(char*, int);
unsigned long long _QExpr(int);

//------------------------------------------------------------------------------
//
//                    delm_precedence
//
// Checks operators that need precedence handling
// Note: if only '+'and '-' then no precedence handling (performance)
//
BOOL delm_precedence(char* _ptr)
  {
  int _boolRslt = FALSE;   // Delimiter not found

  if (*_ptr == '*' ||
      *_ptr == '/' ||
      *_ptr == '|' ||
      *_ptr == '&' ||
      *_ptr == '^' ||
      *_ptr == '~' ||
      *_ptr == '%' ||
      *_ptr == '=' ||     //ha//
      *_ptr == '!' ||     //ha//
      *_ptr == '<' ||
      *_ptr == '>')
    _boolRslt = TRUE;     // Delimiter found

  return(_boolRslt);
  } // delm_precedence

//------------------------------------------------------------------------------
//
//                    delm0
//
// DELM0 checks valid operators
//
BOOL delm0(char* _ptr)
  {
  int _boolRslt = FALSE;   // Delimiter not found

  if (*_ptr == '+' ||
      *_ptr == '-' ||
      *_ptr == '*' ||
      *_ptr == '/' ||
      *_ptr == '|' ||
      *_ptr == '&' ||
      *_ptr == '^' ||
      *_ptr == '~' ||
      *_ptr == '%' ||
      *_ptr == '=' ||     //ha//
      *_ptr == '!' ||     //ha//
      *_ptr == '<' ||
      *_ptr == '>')
    _boolRslt = TRUE;     // Delimiter found

  return(_boolRslt);
  } // delm0

//------------------------------------------------------------------------------
//
//                    delm1
//
// DELM1 checks NUL, SPACE, SEMICOLON, COMMA, CR
//
BOOL delm1(char* _ptr)
  {
  int _boolRslt = FALSE;   // Delimiter not found

  if (*_ptr == SPACE ||
      *_ptr == ';'   ||
      *_ptr == COMMA ||
      *_ptr == CR    ||
      *_ptr == TAB)
    _boolRslt = TRUE;      // Delimiter found

  return(_boolRslt);
  } // delm1

//------------------------------------------------------------------------------
//
//                    delm2
//
// DELM2 checks NUL, SPACE, SEMICOLON, COMMA, CR
//       also checks valid operators and valid trailing delimiter
//
BOOL delm2(char* _ptr)
  {
  int _boolRslt = FALSE;   // Delimiter not found

  if (*_ptr == '+' ||
      *_ptr == '-' ||
      *_ptr == '*' ||
      *_ptr == '/' ||
      *_ptr == '|' ||
      *_ptr == '&' ||
      *_ptr == '^' ||
      *_ptr == '~' ||
      *_ptr == '%' ||
      *_ptr == '=' ||     //ha//
      *_ptr == '!' ||     //ha//
      *_ptr == '<' ||
      *_ptr == '>')
    _boolRslt = TRUE;     // Delimiter found

  else if (delm1(_ptr) == TRUE) _boolRslt = TRUE;
  else if (delm3(_ptr) == TRUE) _boolRslt = TRUE;

  return(_boolRslt);
  } // delm2

//------------------------------------------------------------------------------
//
//                    delm3
//
// DELM3 checks valid trailing delimiter
//
BOOL delm3(char* _ptr)
  {
  int _boolRslt = FALSE;  // Delimiter not found

  if (*_ptr == '(' || *_ptr == ')')
    _boolRslt = TRUE;     // Delimiter found

  return(_boolRslt);      
  } // delm3

//------------------------------------------------------------------------------
//
//                    delm4
//
// DELM4 checks valid leading/trailing delimiters 1 and 3
//
BOOL delm4(char* _ptr)
  {
  int _boolRslt = FALSE;  // Delimiter not found

  if (delm1(_ptr) == TRUE) _boolRslt = TRUE;
  else if (delm3(_ptr) == TRUE) _boolRslt = TRUE;

  return(_boolRslt);
  } // delm4

//------------------------------------------------------------------------------
//
//                    delm5
//
// DELM5 checks NUL, SPACE, SEMICOLON, COMMA, CR
//       also checks valid operators and valid trailing delimiter
//       % operator is not checked (see 'replace_MotorolaBin()')
//
BOOL delm5(char* _ptr)
  {
  int _boolRslt = FALSE;   // Delimiter not found

  if (*_ptr == '+' ||
      *_ptr == '-' ||
      *_ptr == '*' ||
      *_ptr == '/' ||
      *_ptr == '|' ||
      *_ptr == '&' ||
      *_ptr == '^' ||
      *_ptr == '~' ||
      //*_ptr == '%' ||
      *_ptr == '=' ||     //ha//
      *_ptr == '!' ||     //ha//
      *_ptr == '<' ||
      *_ptr == '>')
    _boolRslt = TRUE;     // Delimiter found

  else if (delm1(_ptr) == TRUE) _boolRslt = TRUE;
  else if (delm3(_ptr) == TRUE) _boolRslt = TRUE;

  return(_boolRslt);
  } // delm5

//------------------------------------------------------------------------------
//
//                    skipToNextdelm2
//
// Return the pointer to the delimiter character (and the char itself)
//
char* skipToNextdelm2(char* _ptr)
  {
  char* _tmp_ptr = _ptr;
  lastCharAL=0; //lastCharAH=0;

  while (delm2(_ptr) != TRUE && *_ptr != 0) _ptr++;
  if (_ptr > _tmp_ptr) lastCharAL = *_ptr;
  return(_ptr);
  } // skipToNextdelm2

//------------------------------------------------------------------------------
//
//                    find_delim_precedence
//                                                    
int find_delim_precedence(char* _ptr)
  {
  int _boolRslt = FALSE;   // Assume delimiter not found

  int _i=0;
  while (_ptr[_i] != 0 && (_boolRslt=delm_precedence(&_ptr[_i])) != TRUE) _i++;
  return(_boolRslt);
  } // find_delim_precedence

//------------------------------------------------------------------------------
//
//                    remove_white_spaces
//
// Eliminate all TABs and spaces in a string (collapse the string)
// Something like "Z + N_TEST" gets "Z+N_TEST"
//
void remove_white_spaces(char* _ptr)
  {
  for (_i=0; _i<OPERLEN; _i++)
    {
    if (_ptr[_i] <= SPACE) 
      {
      _j=0;
      while (_j < strlen(_ptr))
        {
        for (_j=_i; _j<strlen(_ptr); _j++) _ptr[_j] = _ptr[_j+1];
        } // end while
      } // end if
    } // end for
  } // remove_white_spaces

//------------------------------------------------------------------------------
//
//                    skip_leading_spaces
//
// Return the pointer to the non-space character (and the char itself)
//
char* skip_leading_spaces(char* _ptr)
  {
  while (*_ptr <= SPACE && *_ptr != 0) _ptr++;    // TABs  and SPACEs
  lastCharAL = *_ptr;
  return(_ptr);
  } // skip_leading_spaces

//------------------------------------------------------------------------------
//
//                    skip_trailing_spaces
//
// Zero out all possible ctnls and spaces at the end of a string
// Return the pointer to the non-space character (and the char itself)
//
char* skip_trailing_spaces(char* _ptr)
  {
  char* _tmpPtr;

  _tmpPtr = _ptr + strlen(_ptr)-1;  // _tmpPtr points to 0-terminator

  // if _tmpPtr points not to a 0-terminator clear SPACE & TAB garbage
  if (*_tmpPtr != 0)      
    {
    while (*_tmpPtr <= SPACE && *_tmpPtr != 0) *(_tmpPtr--) = 0; // clear
    }
  return(_tmpPtr);
  } // skip_trailing_spaces

//------------------------------------------------------------------------------
//
//                         check_num
//
// If Bin-/Dec or Hex-Number: Return the length of the ascii number string
// Else return 0
//
int check_num(char* _ptr, int _radix)
  {
  int _i=0, numLen=0;      // Init

  // -------------------   // 'b' suffix  can be interpreted as hex B in labels
  // Check binary number   //  using '#' instead for Intel/Microsoft 'b' suffix 
  // -------------------   
  if (_radix == RADIX_02 && StrStr(_ptr, "#") != 0)  // only if '#' instead of 'b'
    {
    while (_ptr[_i] != 0 && delm2(&_ptr[_i]) != TRUE)
      { 
      if (_ptr[_i] == '0' || _ptr[_i] == '1') _i++;     // 1st char is Bin-digit
      else if (_ptr[_i] == '#') { numLen = _i; break; } // using '#' instead of 'b'
      else break;                                       // Not a Bin-number
      } // end while
    } // end if (RADIX_02)

  // --------------------
  // Check decimal number  // no prefix, no suffix
  // --------------------
  else if (_radix == RADIX_10)
    {
    while (_ptr[_i] != 0 && delm2(&_ptr[_i]) != TRUE)
      {
      if (_ptr[_i] >= '0' && _ptr[_i] <= '9') { _i++; numLen = _i; }
      else { numLen = 0; break; }                        // Not a Dec-number
      } // end while
    } // end else if (RADIX_10)

  // ------------------------
  // Check hexadecimal number
  // ------------------------
  // Note on algorithm below:
  // "StrCSpnIA(string1, string2)":   string2=Group of characters searched
  //  Search a string for the first occurrence of any of a group of characters.
  //  The search method is not case-sensitive, and the terminating NULL character
  //  is included within the search pattern match. Return: Position of matching
  //  character in string1 (1st char RETURN=0). If no match found it returns
  //  the postion of the terminating 0 of the string1 (RETURN=strlen(string1)).
  //
  else if (_radix == RADIX_16)
    {
    char* HexDigits = "0123456789ABCDEF";
    while (_ptr[_i] != 0 && delm2(&_ptr[_i]) != TRUE)
      {
      if (StrCSpnIA(&_ptr[_i], HexDigits) == 0) _i++;
      else if (tolower(_ptr[_i]) == 'h') { numLen = _i; break; }  // Default: Suffix 'h'
      else break;                                                 // Not a Hex-number
      } // end while
    } // end else if (RADIX_16)

  // else if (_radix == RADIX_08) - octal not supported

  return(numLen);       // return numLen
  } // check_num

//------------------------------------------------------------------------------
//
//                    check_number  (stream version)
//
BOOL check_number(char* _operand, int _radix)
  {
  BOOL b_strnum_stat = FALSE;    // Assume _strnum[] = invalid  contents

  char _strnum_tmp[OPERLEN+1];
  char* _strnum_ptr =_strnum;    // Set pointer to _strnum[] buffer
  char* _operand_ptr = _operand;

  ULONG _numval=0;               // allowing a 32bit number
  unsigned long long _qnumval=0; // allowing a 64bit number

  int _numlen=0, _k=0;
  int _minDigits, _maxDigits, _ResultDigitsHex;

  switch (_radix)
    {
    case RADIX_02:               // Binary number
      _minDigits =  1;           // bin nr: allow 1 digit (0..1)
      _maxDigits = 64;           // bin nr: 64 digits (range=0 - 11111...11111...)
      break;
    case RADIX_10:               // Decimal number
      _minDigits =  1;           // dec nr: 1 digit  (0..9)
      _maxDigits = 20;           // dec nr: 20 digits (range=0..18446744073709551615)
      break;
    case RADIX_16:               // Hexadecimal number
      _minDigits =  1;           // hex nr: 1 digit  (0..F)
      _maxDigits =  16;          // hex nr: 16 digits (range=0..0FFFFFFFFFFFFFFFFh)
      break;
    default:
      return(b_strnum_stat);     // RADIX_8 not supported
      break;
    } // end switch

  for (_i=0; _i<OPERLEN; _i++)
    {     
    _strnum[_i] = 0;     // Clear with 0
    _strnum_tmp[_i] = 0; // Clear with 0
    }

  // ----------------------
  // Process the expression
  // ----------------------
  while (*_operand_ptr != 0)     // Parse the whole operand string
    {
    while (*_operand_ptr != 0 && ((_numlen=check_num(_operand_ptr, _radix)) == 0))
      {
      *_strnum_ptr++ = toupper(*_operand_ptr++);   // Skip invalid number
      }

    // Error if number(radix) is too long (_radix == 2 || _radix ==10)
    if (_numlen > _maxDigits)    
      {
      for (_i=0; _i<_numlen; _i++) pszErrtextbuf[_i] = _operand_ptr[_i];
      pszErrtextbuf[_i] = 0;         // terminate info text string
      errorp2(ERR_LONGCONST, pszErrtextbuf);  // show error and part of expr as nfo text
      }

    if (*_operand_ptr != 0 && _numlen >= _minDigits && _numlen <= _maxDigits)
      {
      // flag that at least one valid number has been found                               
      _k=1;
                                   
      // Copy expression non-destructively
      for (_i=0; _i<_numlen; _i++) _strnum_tmp[_i] = toupper(_operand_ptr[_i]);
      _strnum_tmp[_numlen] = 0;    // 0-terminate for labfn()

      error = 0;                   // Clear any possible previous error condition
      _qnumval = QExpr(_strnum_tmp, _radix);  // Convert number string into hex-value

      // Emit symbol address value into expression string buffer
      // This is to eventually save space in _strexp buffer,
      //  allowing longer expression constructs if appropriate.
      if ((_qnumval & 0xFFFFFFFF00000000) != 0)
        {
        edqh(_strnum_tmp, _qnumval);           // Emit a string of 16 ascii-hex digits
        _ResultDigitsHex=16;
        }
      else if ((unsigned long long)(_qnumval & 0xFFFFFFFFFFFF0000) != 0)
        {
        eddh(_strnum_tmp, (UINT)_qnumval);     // Emit a string of 8 ascii-hex digits
        _ResultDigitsHex=8;
        }
      else
        {
        edwh(_strnum_tmp, (UINT)_qnumval);     // Emit a string of 4 ascii-hex digits
        _ResultDigitsHex=4;
        }
      // Max 8 digit hex-number (range=0..0xFFFFFFFF)
      // (may be truncated later according to 4 digit CPU address range 0..FFFF)
      for (_i=0; _i<_ResultDigitsHex; _i++)
        *_strnum_ptr++ = _strnum_tmp[_i];

      // Skip delimiters
      while (delm2(_operand_ptr) != TRUE && *_operand_ptr != 0) _operand_ptr++;
      *_strnum_ptr++ = *_operand_ptr;
      } // end if (_numlen >= ..)

    _operand_ptr++;                                                                      
    } // end while (*_operand_ptr != 0)

  *(_strnum_ptr) = 0;       // 0-terminate _strnum[] buffer

  // IMPORTANT: Always restore original _operand if no valid nr found in expression
  // (
  //  important if jmp-labels are hex-nrs containing a 'B'.
  //  For example see kbc.lst "013B atkbin_2:" the "01" is swallowed
  //  by means of 'check_numbin(_operand_ptr)' and causes >>>ERROR jmp out of range.
  // )
  // NOTE: opcodes should only have labels as operands, never any expression
  if (_k == 0)
    {              
    for (_i=0; _i<OPERLEN-1; _i++) _strnum[_i] = toupper(_operand[_i]);  
    }

  //----------------------------------------------------------------------------
  // Eliminate all 'h' suffixes and collapse the string
  for (_i=0; _i<OPERLEN; _i++)
    {
    // Eliminate all 'h' suffixes witin _strnum string
    // (suffixes are not allowed in expression - see 'D/QExpr(str,RADIX)')
    if (toupper(_strnum[_i]) == 'H') //  || toupper(_strnum[_i]) == 'B')
      {
      _j=0; _k=0;
      while (_j < strlen(_strnum))
        {
        for (_j=_i; _j<strlen(_strnum); _j++) _strnum[_j] = toupper(_strnum[_j+1]);
        } // end while
      } // end if
    } // end for

  // If some kind of garbage '% /' .. at end of _strnum - prevent exception
  if (delm0(&_strnum[strlen(_strnum)]-1) == TRUE)
    errorp2(ERR_ILLEXPR, pszErrtextbuf);
  //----------------------------------------------------------------------------

  // Hex expression in _strnum[] is valid if no error occurs
  // Return the result in _strexp[]
  else
    {
    for (_i=0; _i<OPERLEN; _i++) _strexp[_i] = toupper(_strnum[_i]);  
    if (error == 0) b_strnum_stat = TRUE;
    } // end else

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//if (swpass == _PASS2)
//ha//{
//ha//printf("_operand=%s\n _strexp=%s", _operand, _strexp);
//ha//DebugStop(33, "check_number()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

  return(b_strnum_stat);     // Return _strnum[] status
  } // check_number

//------------------------------------------------------------------------------
//
//                    check_symbol (Block-Version)
//
// Intel syntax:  'h' 'b' '$' ' '
//
BOOL check_symbol(char* _operand)
  {
  BOOL b_sym_stat = FALSE;    // Assume _strexp[] = invalid contents
  ULONG valSave;

  char _operandSav[OPERLEN+1] = {0};
  char* _tmp_ptr = _operand;
  char* _strexp_ptr =_strexp; // Set pointer to _strexp[] buffer
  char* _operand_ptr = _operand;

  int _operandLen=0;
  _exprInfo = 0;              //ha//

  for (_i=0; _i<OPERLEN; _i++) _strexp[_i] = 0; // Clear with 0

  // Parse the whole operand string
  while (*_operand_ptr != 0)
    { 
    // 1st: Transfer all possible delimiters
    while (delm2(_operand_ptr) == TRUE) *_strexp_ptr++ = *_operand_ptr++;

    // 2nd: Transfer the expression (label, symbol, decNr, hexNr, BinNr, $)
    _tmp_ptr = skipToNextdelm2(_operand_ptr); 
    _operandLen = _tmp_ptr - _operand_ptr;
    // Prevent buffer overrun
    if (_operandLen > sizeof(_operandSav)) _operandLen = OPERLEN; 

    // Copy and check possible symbol non-destructively
    for (_i=0; _i<sizeof(_operandSav); _i++) _operandSav[_i] = 0;
    for (_i=0; _i<_operandLen; _i++) _operandSav[_i] = _operand_ptr[_i]; // Save locally

    if (label_syntax(_operandSav) == FALSE) // Label syntax is invalid
      {
      //----------------------------------------------------------------------------
      // Process the '$' operand ($ = pcc/pcd/pce = current program counter address)
      if (_operandSav[0] == _BUCK)
        {
        // Syntax check
        char* _buck_ptr = _operand_ptr;
        _buck_ptr = skip_leading_spaces(++_buck_ptr);
        if (*_buck_ptr >= '0') errorp2(ERR_ILLEXPR, pszErrtextbuf);

        GetPcValue();

        // Atmel: Emit pcc ascii to buffer
        if (AtmelFlag != 0 && SegType == _CODE)
          {
          if ((pcValue/2 & 0xFFFF0000) == 0)
            { 
            edwh(_strexp_ptr, pcValue/2);
            _strexp_ptr += 2*2;           // pcc = 4 chars ascii-hex
            }
          else
            {
            eddh(_strexp_ptr, pcValue/2);
            _strexp_ptr += 4*2;           // pcc = 8 chars ascii-hex
            }
          } // end if (AtmelFlag .. _CODE)
             
        // Atmel: Emit pcd/pce ascii to buffer
        else if (AtmelFlag != 0 && (SegType == _DATA || SegType == _EEPROM))
          {
          if ((pcValue & 0xFFFF0000) == 0)
            { 
            edwh(_strexp_ptr, pcValue);
            _strexp_ptr += 2*2;           // pcc/pcd = 4 chars ascii-hex
            }
          else
            {
            eddh(_strexp_ptr, pcValue);
            _strexp_ptr += 4*2;           // pcc/pcd = 8 chars ascii-hex
            }
          } // end if (AtmelFlag .. _DATA, _EEPROM)

        // Intel/Motorola: Emit 16bit pcc/pcd ascii to buffer
        else
          {
          edwh(_strexp_ptr, pcValue);     // Emit pcc/pcd ascii to buffer
          _strexp_ptr += 2*2;             // pcc/pcd = 4 chars ascii-hex
          } // end else

        *(_strexp_ptr++) = 'h';  // ..still needed for 'check_number(RADIX_10)'
        b_sym_stat = TRUE;       // _strexp[] = valid
        } // end if ('$')
      //----------------------------------------------------------------------------

      else
        {
        for (_i=0; _i<_operandLen; _i++) *_strexp_ptr++ = *_operand_ptr++;
        }
      } // end if (label_syntax == FALSE)

    // Inform the bytval*.cpp about unkmown symbols in Pass1
    else if (labfn(_operandSav) == FALSE)
      {
      _exprInfo = ERR_UNDFSYM;             // for Pass1 only
      errorp2(ERR_UNDFSYM, pszErrtextbuf); // for Pass2, ignored in Pass1
      }

    else if (labfn(_operandSav) != FALSE)  // may be >=1 (don't use 'TRUE')
      {
      valSave = symboltab_ptr->symAddress; // Get symbol address value   //ha//
      if (AtmelFlag != 0 && symboltab_ptr->symType == _CODE)             //ha//
        valSave /= 2;                                                    //ha//

      // Set error if doubly defined or undefined symbol or label
      if ((symboltab_ptr->symType & SYMERR_FLAG) != 0)
        errorp2(ERR_ILLEXPR, pszErrtextbuf);

      // Emit symbol address value into expression string buffer
      // This is to eventually save space in _strexp buffer,
      //  allowing longer expression constructs if appropriate.
      if ((valSave & 0xFFFF0000) == 0)
        //  (16bit require 4 chars ascii-hex in buffer) 
        {
        edwh(_strexp_ptr, valSave);   
        _strexp_ptr += 2*2;       // 4 chars ascii-hex
        }
      else
        //  (32bit require 8 chars ascii-hex in buffer) 
        {
        eddh(_strexp_ptr, valSave);   
        _strexp_ptr += 2*4;       // 8 chars ascii-hex
        }
      *(_strexp_ptr++) = 'h';     // ..still needed for 'check_number(RADIX_10)'
      b_sym_stat = TRUE;          // _strexp[] = valid
      }

    // Error checking special cases in Pass2 are handled by means of 'errStatus'
    else if (swpass == 2 && errStatus == 0)
      errorp2(ERR_UNDFSYM, pszErrtextbuf);

    if (lastCharAL != 0) *_strexp_ptr++ = lastCharAL;
    _operand_ptr = _tmp_ptr+1;
    } // end while (*_operand_ptr)

  *_strexp_ptr = 0;             // 0-terminate _strexp[] buffer

  return(b_sym_stat);
  } // check_symbol


//------------------------------------------------------------------------------
//
//                        parse_expr 
//
// Intel syntax:  'h' 'b' '$' ' '
//
BOOL parse_expr(char* _operand)
  {
  BOOL b_strexp_stat = FALSE; // Assume _strexp[] = invalid contents

  /////////////////////////////////////////////////////////
  symbol_stat = check_symbol(_operand);                  //
  // Status of all num checks is relevant                //
  b_strexp_stat |= check_number(_strexp, RADIX_10);      //  
  b_strexp_stat |= check_number(_strexp, RADIX_02);      //
  //b_strexp_stat |= check_number(_operand, RADIX_16);   //
  /////////////////////////////////////////////////////////

  _radix = RADIX_16;        // _strexp[] = all numbers in ascii Hex rendition

  return(b_strexp_stat);    // Return _strexp[] status
  } // parse_expr


//------------------------------------------------------------------------------
//
//                    replace_MotorolaHex
//
// Motorola Syntax:     Intel Syntax:
// Motorola Hex '$'     Intel Hex 'h'
// Motorola Bin '%'     Intel Bin 'b'
// Motorola Oct '@'     Intel Oct '0' deprecated
//
// Replace the Motrola $-hex prefix with intel 'h'-hex suffix
//
void replace_MotorolaHex(char* _bkPtr)
  {
  char _bkTmp[OPERLEN+1] = {0};
  int bkHexfl = 0;

  _k=0; _j=0;
  while (_bkPtr[_k] !=0)
    {
    if (_bkPtr[_k] == '$' && 
        ((toupper(_bkPtr[_k+1]) >= 'A' && toupper(_bkPtr[_k+1]) <= 'F') ||
        (_bkPtr[_k+1] >= '0' && _bkPtr[_k+1] <= '9'))
       )
      {
      bkHexfl = 1;
      _bkTmp[_j] = '0';
      }
    else _bkTmp[_j] = _bkPtr[_k];  // decimal or symbol: no $ hex-prefix

    _k++; _j++;
    if (bkHexfl == 1 && (_bkPtr[_k] == 0 || delm2(&_bkPtr[_k]) == TRUE))
      {
      _bkTmp[_j++] = 'h';
      bkHexfl = 0;
      while (delm2(&_bkPtr[_k]) == TRUE) _bkTmp[_j++] = _bkPtr[_k++];
      }
    else _bkTmp[_j] = _bkPtr[_k];
    } // end while

  // Provide the Intel syntax as the current operand
  for (_i=0; _i<OPERLEN; _i++) _bkPtr[_i] = _bkTmp[_i];
  } // replace_MotorolaHex

//------------------------------------------------------------------------------
//
//                    replace_MotorolaBin
//
// Motorola Syntax:     Intel Syntax:
// Motorola Hex '$'     Intel Hex 'h'
// Motorola Bin '%'     Intel Bin 'b'
// Motorola Oct '@'     Intel Oct '0' deprecated
//
// Check number in expression (Intel/Motorola/Atmel syntax)
//   Bin:  10001111b, %10001111, 0b10001111
//   Hex: 0ABCD1234h, $ABCD1234, 0xABCD1234
//   Dec:  12346789
//   Oct: not supported
//
// Replace the Motrola %-bin prefix with intel 'b'-bin suffix
// (However, Intel/Microsoft 'b' suffix can be interpreted as hex B in labels.
//  Internally '#' is used instead as a place holder to reflect bin numbers)
//
void replace_MotorolaBin(char* _bkPtr)
  {
  char _bkTmp[OPERLEN+1] = {0};

  _k=0; _j=0;
  while (_bkPtr[_k] !=0)           // parse the whole expression mixture
    {
    // Check Motorola binary notation '%' bin-prefix  (e.g. '%0101' or '%1010') 
    if (_bkPtr[_k] == '%' && (_bkPtr[_k+1] == '0' || _bkPtr[_k+1] == '1'))
      _j--;                        // exclude '%' prefix from _bkTmp[_j]
    else _bkTmp[_j] = _bkPtr[_k];  // it's hex/dec/symbol: no '%' bin-prefix

    _j++; _k++;
    if (_j != _k && (_bkPtr[_k] == 0 || delm5(&_bkPtr[_k]) == TRUE))
      {
      // Substitute '#' instead of Intel 'b' binary suffix at end of number string
      // !! 'b' could be interpreted as hex '0xB' in labels.
      _bkTmp[_j++] = '#';          // Place holder '#' for Intel 'b' binary suffix
      // Walk behind any delimiter and operand or stop at end of number string
      while (delm5(&_bkPtr[_k]) == TRUE) _bkTmp[_j++] = _bkPtr[_k++];
      }
    else _bkTmp[_j] = _bkPtr[_k];
    } // end while (_bkPtr[_k] !=0)

  // Provide '#' suffix at the current operand  (e.g. '%0101' becomes '0101#')
  for (_i=0; _i<OPERLEN; _i++) _bkPtr[_i] = _bkTmp[_i];
  } // replace_MotorolaBin
                                                              
//------------------------------------------------------------------------------
//
//                    replace_MotorolaPC
//
// Motorola Syntax:     Intel Syntax:
// Motorola pcc '*'     Intel pcc '$'
//
// Replace the Motrola location counter '*' with intel '$'
//
void replace_MotorolaPC(char* _bkPtr)
  {
  char _bkTmp[OPERLEN+1] = {0};
  int bkBinfl = 0;

  _k=0;                                                 
  while (_bkPtr[_k] != 0)
    {
    if (_bkPtr[0] == _STAR) _bkPtr[0] = _BUCK;
    else if (_bkPtr[_k] == _STAR)                          //ha//
      {                                                    //ha//
      char *_ptrTmp = skip_leading_spaces(&_bkPtr[_k+1]);  //ha//
      if (lastCharAL < SPACE)                              //ha//
        {                                                  //ha//
        _bkPtr[_k] = _BUCK;                                //ha//
        _bkPtr[_k+1] = 0;                                  //ha//
        }                                                  //ha//
      } // end if                                          //ha//

    _bkTmp[_k] = _bkPtr[_k];   // it's not the pcc symbol
    _k++;
    } // end while

  // Provide the Intel syntax as the current operand
  for (_i=0; _i<OPERLEN; _i++) _bkPtr[_i] = _bkTmp[_i];
  } // replace_MotorolaPC
                                                              
//------------------------------------------------------------------------------
//
//                    replace_token
//
void replace_token(char* _operand)
  {
  typedef struct tagTOKENSTR {
    char*  tokStr;
    char*  rplStr;
  } TOKENSTR, *LPTOKENSTR;

  // No TABs are allowed in tokenstrings
  TOKENSTR tokenstr[] = {  
    {" XOR ","   ^ "},   
    {" XOR(","   ^("},   
    {" AND ","   & "},
    {" AND(","   &("},
    {" OR " ,"  | "},
    {" OR(" ,"  |("},
    {"NOT " ,"   ~"},
    {"NOT(" ,"  ~("},
    {" SHR ","  >> "},
    {" SHR(","  >>("},
    {" SHL ","  << "},
    {" SHL(","  <<("},
    {" MOD ","    %"},
    {" MOD(","   %("},
    }; //end-of-table

  BOOL b_token_stat = TRUE; // Set recoursive
  char* _ptrTmp =_operand;
  char _opTmp[OPERLEN+1] = {0};

  //-------------------------------------------------------------------
  // Hex/Bin constant (special handling, three different notations)
  // Atmel: Allow and evaluate '0x' '0b' prefix in expression
  // In _operand replace all prefixes '0x' w/ ' $' and all '0b' w/ ' %'
  //  and the Intel binrary suffix 'b' is replaced by '#'
  //  to be processed later as Motorola syntax (Atmel supported).
  //
  // Skip unary forms ['-' '+' '~' '!' '(']
  _i=0;
  while (delm2(&_operand[_i]) == TRUE && _operand[_i] != 0) _i++;

  while (_operand[_i] != 0)
    {
    // clear pending expr errors
    error = 0;  
    // After delimiters/operators 
    // C-Style '0x' Hex-prefix replaced by ' $'
    if (_operand[_i] == '0' && tolower(_operand[_i+1]) == 'x')
      {
      _j=_i;
      while (delm2(&_operand[_j]) != TRUE && _operand[_j] != 0) _j++;
      // _j = delim at END of operand, check valid digit
      if (tolower(_operand[_j-1]) > 'f') errorp2(ERR_ILLOPER, &_operand[_j-1]);
      else
        {
        _operand[_i]   = SPACE;
        _operand[_i+1] = _BUCK;  // ' $'
        }
      }
    // C-Style '0b' Bin-prefix replaced by ' %'
    else if (_operand[_i] == '0' && tolower(_operand[_i+1]) == 'b' &&
             (_operand[_i+2] == '0' || _operand[_i+2] == '1'))
      {
      _j=_i;
      while (delm2(&_operand[_j]) != TRUE && _operand[_j] != 0) _j++;
      // _j = delim at END of operand, check valid digit
      if (_operand[_j-1] == '0' || _operand[_j-1] == '1')
        {
        _operand[_i]   = SPACE;  // ' %'
        _operand[_i+1] = '%';
        }
      } // end else if
    //////////////////////////////////////////////////////////////////////////////
    // Intel 'b' Bin-suffix (special handling required)                         //
    else if (MotorolaFlag == FALSE) // not if Motorola                          //
      {                                                                         //
      int _l=_i, _m=0, _n=0;                                                    //
      while (delm2(&_operand[_l]) != TRUE && _operand[_l] != 0) _l++;           //
      // _l = 1st delim at END of operand                                       //
      if (_l > 1 && tolower(_operand[_l-1]) == 'b')                             //
        {                                                                       //
        // Check binary constant (_n = walks to delim at BEGIN of operand)      //
        _n=_l-2, _m=0;                                                          //
        while (_n > 0 && delm2(&_operand[_n]) != TRUE)                          //
          {                                                                     //
          if (_operand[_n] != '0' && _operand[_n] != '1') { _m = 1; break; }    //
          else _n--;                                                            //
          } // end while                                                        //
        // Also check hex '$' prefix ($11B vs +11b)                             //
        if (_m == 0 && _operand[_n] != '$') _operand[_l-1] = '#'; // 'b'-->'#'  //
        }                                                                       //
      } // end else if                                                          //
     /////////////////////////////////////////////////////////////////////////////

    while (delm2(&_operand[_i]) != TRUE && _operand[_i] != 0) _i++;
    if (_operand[_i] == 0) break;
    else _i++;
    } // end while

  //----------------------------------------------
  // Replace recoursively all tokens in expression  
  while (b_token_stat == TRUE)  
    { 
    _ptrTmp = skip_leading_spaces(_operand);
    // Motorola Syntax (also allowed with Atmel AVR(R) syntax):
    // Replace the Motrola $-hex prefix with Intel 'h'-hex suffix
    // Replace the Motrola %-bin prefix with Intel 'b'-bin suffix
    // Replace the Motrola *-program counter with Intel '$'
    if (AtmelFlag == TRUE || MotorolaFlag == TRUE)
      {
      replace_MotorolaBin(_ptrTmp);
      replace_MotorolaHex(_ptrTmp);
      replace_MotorolaPC(_ptrTmp);
      }

    b_token_stat = FALSE;         // Assume no more Tokens

    for (_i=0; _i<sizeof(tokenstr)/sizeof(TOKENSTR); _i++)
      {
      if ((_ptrTmp=StrStrI(_operand, tokenstr[_i].tokStr)) != 0)
        {
        for (_j=0; _j<strlen(tokenstr[_i].rplStr); _j++) _ptrTmp[_j] = tokenstr[_i].rplStr[_j];
        b_token_stat = TRUE;      // Demand another run
        } // end if
      } // end for(_i)
    } // end while

  // Allow and evaluate chars in expression,  e.g. 'a'+10 = 6Bh
  _i=0;
  while (_operand[_i] != 0)
    {
    if (_operand[_i] == STRNG)
      {
      if (_operand[_i+2] == STRNG)
        {
        // In _operand replace all chars w/ Asc2Hex, e.g.:  ['A'] w/ [41h]
        // Save - gets destroyed by sprintf w 0-terminator
        char _opSav = _operand[_i+3];   
        sprintf(&_operand[_i], "%02Xh", _operand[_i+1]);
        // Restore - was overwritten by sprintf w/ 0-terminator
        _operand[_i+3] = _opSav;        
        }
      else _operand[_i] = '?'; // force an error  
      }
    _i++;
    } // end while

  for (_i=0; _i<OPERLEN+1; _i++) _opTmp[_i] = 0;   // Init-clear

  _i=0; _k=0;
  while (_operand[_i] != 0)
    {
    if (_operand[_i] > SPACE)
      {
      _opTmp[_k] = _operand[_i];  // Collapse SPACEs and TABs
      _k++;
      }
    _i++;
    } // end while

  for (_i=0; _i<OPERLEN; _i++) _operand[_i] = _opTmp[_i];
  } // replace_token


//-----------------------------------------------------------------------------
//
//                      checkParenthesis
//
int checkParenthesis(char* _operand)
  {
  int __i;
  int __k=0;

  for (__i=0; __i<strlen(_operand); __i++)
    {
    if (_operand[__i] == '(') __k++;
    if (_operand[__i] == ')') __k--;
    }
  return(__k);
  } // checkParenthesis

//-----------------------------------------------------------------------------
//
//                 OperatorPrecedence / InsertParenthesis
//
// Although not obvious, the algorithm is correct, and, in the words of D.Knuth,
// "The resulting formula is properly parenthesized, believe it or not."
//
// The following table lists the order of precedence of operators in C.
//  Here, operators with the highest precedence appear at the top of the table,
//  and those with the lowest appear at the bottom.
// 
//    -------------------------------------
//   |       Operators (Precedence)        |
//    -------------------------------------
//   Category         Operator    Associativity  Pre
//   Parenthesis      ()          Left to right  0
//   Unary            + - ! ~     Right to left  0
//   Multiplicative   * / %       Left to right  1
//   Additive         + -         Left to right  2
//   Shift            << >>       Left to right  2
//   Relational       < <= > >=   Left to right  2
//   Equality         == !=       Left to right  2
//   Bitwise AND      &           Left to right  3
//   Bitwise XOR      ^           Left to right  4
//   Bitwise OR       |           Left to right  4
//   Logical AND      &&          Left to right  3
//   Logical OR       ||          Left to right  3
// 
//  ---------------------------------------
// |        Operators (Precedence)         |
//  ---------------------------------------
// ; (       Left parenthesis      (((     ;
// ; )       Right parenthesis         ))) ;
// ; +       Unary PLUS                    ;
// ; -       Unary MINUS                   ;
// ; ~  NOT  Bitwise NOT                   ;
// ; !       Logical NOT                   ;
// ; *       Multiplication          ) (   ;
// ; /       Division                ) (   ;
// ; %       Modulo                  ) (   ;
// ; +       Addition               )) ((  ;
// ; -       Subtraction            )) ((  ;
// ; << SHL  Shift left              ) (   ;
// ; >> SHR  Shift right             ) (   ;
// ; <       Less than              )) ((  ;
// ; <=      Less than or equal     )) ((  ;
// ; >       Greater than           )) ((  ;
// ; >=      Greater than or equal  )) ((  ;
// ; ==      Equal                  )) ((  ;
// ; !=      Not equal              )) ((  ;
// ; &  AND  Bitwise AND             ) (   ;
// ; ^  XOR  Bitwise XOR             ) (   ;
// ; |  OR   Bitwise OR             )) ((  ;
// ; &&      Logical AND            )) ((  ;
// ; ||      Logical OR             )) ((  ;
//  ---------------------------------------
//
// Example:
// "((-1-1==-2)*0007|1-0)<<1|0012&1"                      // Simple                                     // =00 w/o precedence
// "((((-1-1 == -2) * 0007) | (1-0)) << 1) | (0012 & 1)"  // Intended                                   // =0E 'embraced' as intended
// "((( ((( (((-1 ))-(( 1 ))==(( -2))) )*(  0007 ))|(( 1 ))-(( 0))) )<<( 1 ))|((        0012  )&( 1)))" // =0E w/ parethesis algo
// "((( ((( (((-1 ))-(( 1 ))==(( -2))) )*( 0b111 ))|(( 1 ))-(( 0))) )<<( 1 ))|(( high(0x1234) )&( 1)))" // =0E w/ parethesis algo
//
// "((( 2 )*( 1 ))+(( 3 )<<( 4 ))|(( $08 )))"             // =$003A NOK  (expected =$0058)
// ---------------------------------------------------------------------------
// "((( 2 * 1 )+( 3 ))<<(( 4 )))|((( $08 )))              // =$0058 OK ($0058)
// ---------------------------------------------------------------------------
//
#define MAXPARENTHESIS 5  // ((( .. )))
#define PRECEDENCE_NUL 0  // unary
#define PRECEDENCE_1ST 1 
#define PRECEDENCE_2ND 2 
#define PRECEDENCE_3RD 3 
#define PRECEDENCE_4TH 4 

void InsertParenthesis(int _pcnt, char _op1, char _op2)
  {
  switch(_pcnt)
    {
//ha//    case PRECEDENCE_NUL:
//ha//      *pszScratchbuf++ = _op1;
//ha//      break;
    case PRECEDENCE_1ST:
      *pszScratchbuf++ = ')';
      *pszScratchbuf++ = _op1;
      if (_op2 != '#') *pszScratchbuf++ = _op2;
      *pszScratchbuf++ = '(';
      break;
    case PRECEDENCE_2ND:
      *pszScratchbuf++ = ')';
      *pszScratchbuf++ = ')';
      *pszScratchbuf++ = _op1;
      if (_op2 != '#') *pszScratchbuf++ = _op2;
      *pszScratchbuf++ = '(';
      *pszScratchbuf++ = '(';
      break;
    case PRECEDENCE_3RD:
      *pszScratchbuf++ = ')';
      *pszScratchbuf++ = ')';
      *pszScratchbuf++ = ')';
      *pszScratchbuf++ = _op1;
      if (_op2 != '#') *pszScratchbuf++ = _op2;
      *pszScratchbuf++ = '(';
      *pszScratchbuf++ = '(';
      *pszScratchbuf++ = '(';
      break;
    case PRECEDENCE_4TH:
      *pszScratchbuf++ = ')';
      *pszScratchbuf++ = ')';
      *pszScratchbuf++ = ')';
      *pszScratchbuf++ = ')';
      *pszScratchbuf++ = _op1;
      if (_op2 != '#') *pszScratchbuf++ = _op2;
      *pszScratchbuf++ = '(';
      *pszScratchbuf++ = '(';
      *pszScratchbuf++ = '(';
      *pszScratchbuf++ = '(';
      break;
    case MAXPARENTHESIS:
      *pszScratchbuf++ = _op1;
      *pszScratchbuf++ = _op1;
      *pszScratchbuf++ = _op1;
      *pszScratchbuf++ = _op1;
      *pszScratchbuf++ = _op1;
      break;
    }
  } // InsertParenthesis

//-----------------------------------------------------------------------------
//
//                 OperatorPrecedence
//
char* OperatorPrecedence(char* _strExpr)
  {
  char* operChars = "!~-*/%+<>=&^|()";
 
  // Init clear buffer
  pszScratchbuf = pszScratchbuf0;
  for (_i=0; _i<4*OPERLEN; _i++) pszScratchbuf[_i] = 0;
  
  // Starting sequence '((((('
  InsertParenthesis(MAXPARENTHESIS,'(', '#'); 

  _i = 0;
  while (_i <= OPERLEN && _strExpr[_i] != 0)       // End of oper string
    {
    _k=0;                                          // Assume no operand
    if (StrCSpnIA(&_strExpr[_i], operChars) == 0)  // 'Pole position0' must match
      {
      _k=1; 
      switch(_strExpr[_i])
        {
        case '!':
          if (_strExpr[_i+1] == '=')
            {
            InsertParenthesis(PRECEDENCE_2ND,_strExpr[_i],'=');
            _i++;
            }
          else *pszScratchbuf++ = _strExpr[_i];                  // unary
          break;   
        case '~': 
          *pszScratchbuf++ = _strExpr[_i];                       // unary
          break;   
        case '-':
          if (_i != 0 && !delm2(&_strExpr[_i-1]))
            InsertParenthesis(PRECEDENCE_2ND,_strExpr[_i],'#');
          else if (_i != 0 && _strExpr[_i-1] == ')') // e.g. 2*(6)-1 = (((2 )*( (((6))) ))-(( 1)))
            InsertParenthesis(PRECEDENCE_2ND,_strExpr[_i],'#');
          else *pszScratchbuf++ = _strExpr[_i];                  // unary
          break;   
        case '+':
          if (_i != 0 && !delm2(&_strExpr[_i-1]))
            InsertParenthesis(PRECEDENCE_2ND,_strExpr[_i],'#');
          else if (_i != 0 && _strExpr[_i-1] == ')') // e.g. 2*(6)+1 = (((2 )*( (((6))) ))+(( 1)))
            InsertParenthesis(PRECEDENCE_2ND,_strExpr[_i],'#');
          else *pszScratchbuf++ = _strExpr[_i];                  // unary
          break;

        case '*':
          InsertParenthesis(PRECEDENCE_1ST,_strExpr[_i],'#');    // [*]  )*(
          break;
        case '/':
          InsertParenthesis(PRECEDENCE_1ST,_strExpr[_i],'#');    // [/]  )/(
          break;
        case '%':
          InsertParenthesis(PRECEDENCE_1ST,_strExpr[_i],'#');    // [% MOD] )%(
          break;

        case '<':
          if (_strExpr[_i+1] == _strExpr[_i])                 
            {
            InsertParenthesis(PRECEDENCE_2ND,_strExpr[_i],_strExpr[_i]);  // [<< SHL] = ))<<((
            _i++;
            }
          else if (_strExpr[_i+1] == '=')
            {
            InsertParenthesis(PRECEDENCE_2ND,_strExpr[_i],'=');           // [<= LE] = ))<=((
            _i++;
            }
          else InsertParenthesis(PRECEDENCE_2ND,_strExpr[_i],'#');        // [< LT] = ))<((
          break;
        case '>':                              // (($a600*256+$75a2)>>8) = 0000A675 OK  expected
          if (_strExpr[_i+1] == _strExpr[_i])  // (($a600*256+$75a2)>>8) = 00A60075 NOK (this algo)
            {
            InsertParenthesis(PRECEDENCE_2ND,_strExpr[_i],_strExpr[_i]);  // [>> SHR] = ))>>((
            _i++;
            }
          else if (_strExpr[_i+1] == '=')                     
            {
            InsertParenthesis(PRECEDENCE_2ND,_strExpr[_i],'=');           // [>= GE] = ))>=((
            _i++;
            }
          else InsertParenthesis(PRECEDENCE_2ND,_strExpr[_i],'#');        // [> GT] = ))>((
          break;

        case '=':
          InsertParenthesis(PRECEDENCE_2ND,_strExpr[_i],_strExpr[_i]);    // [== EQ] = ))==((
          _i++;
          break;

        case '&':                                             
          if (_strExpr[_i+1] == _strExpr[_i])                 
            {
            InsertParenthesis(PRECEDENCE_3RD,_strExpr[_i],_strExpr[_i]);  // [&&] = )))&&(((
            _i++;
            }
         else InsertParenthesis(PRECEDENCE_3RD,_strExpr[_i],'#');         // [& AND]  = )))&(((
          break;
        case '^':
           InsertParenthesis(PRECEDENCE_4TH,_strExpr[_i],'#');            // [^ XOR]  = ))))^((((
          break;
        case '|':
          if (_strExpr[_i+1] == _strExpr[_i])
            {
            InsertParenthesis(PRECEDENCE_3RD,_strExpr[_i],_strExpr[_i]);  // [||] = )))||(((
            _i++;
            }
          else InsertParenthesis(PRECEDENCE_4TH,_strExpr[_i],'#');        // [| OR] = ))))|((((
          break;

        case '(':
          InsertParenthesis(MAXPARENTHESIS,_strExpr[_i],'#');             // [(] = (((((
          break;
        case ')':
          InsertParenthesis(MAXPARENTHESIS,_strExpr[_i],'#');             // [)] = )))))
          break;
        default:
          break;
        } // end switch

      _i++;
      } // end if

    if (_k == 0) { *pszScratchbuf++ = _strExpr[_i]; _i++; }
    } // end while
  
  // Terminating sequence ')))'
  InsertParenthesis(MAXPARENTHESIS,')','#');

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha////if (_debugSwPass == _PASS2)
//ha//{
//ha//printf("pszScratchbuf0=%s\n", pszScratchbuf0);    
//ha////printf("pszScratchbuf0 ");    
//ha////DebugPrintBuffer(pszScratchbuf0, OPERLEN/4);
//ha//DebugStop(33, "OperatorPrecedence()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

  return(pszScratchbuf0);
  } // OperatorPrecedence


//-----------------------------------------------------------------------------
//
//                                expr
// Result:
//  value in global 'lvalue', 'qvalue'
//  error in global 'error'
//
UINT expr(char* _operand)
  {
  SymType = _ABSSET; // Set "Absolute constant"

  // Store _operand for a posssible error info (including 0-terminator)
  if (erno == 0)
    {for (_i=0; _i<=strlen(_operand); _i++) pszErrtextbuf[_i] = _operand[_i];}

  // return error if nothing to evaluate
  if (_operand[0] == 0)
    {
    errorp2(ERR_ILLOP, _operand);
    return(0); 
    }

  if (checkParenthesis(_operand) != 0) 
    {
    if (erno == 0) errorp2(ERR_SYNTAX, _operand); // parenthesis mismatch
    return(0);
    }


  // Copy _str <-_operand, including SPACEs and TABs;
  //StrCpy(_str, _operand); // StrCpy(_str,..) // _str is not guaranteed to be 0-terminated.
  for (_i=0; _i<OPERLEN+1; _i++) _str[_i] = _operand[_i];
  replace_token(_str);

  // PASS1 / PASS2
  // If parse_expr was TRUE:  '_strexp' = Pointer to expression for D/QExpr(_radix)
  // If parse_expr was FALSE: Return with error
  if (parse_expr(_str) == FALSE)
    {
    if (erno == 0)
      {
      //  (error == 3)  // illegal operator
      //  (error == 4)  // Numeric constant too long
      //  (error == 5)  // illegal character in expression
      if (error == 4) errorp2(ERR_LONGCONST, pszErrtextbuf);
      else if (error == 5)
        {
        errorp2(ERR_ILLEXPR, _str);
        lvalue = 0;
        qvalue = 0;
        return(lvalue);  // Error: Return value = 0
        }
      } // end if (erno)
    } // end if (parse_expr)
  
  // At this point: 
  // _strexp contains the expression rendered in ascii (RADIX16)
  // Convert the expression into the expected operator precedence,
  //  and, then evaluate straight from left to right.
  // (only if there are operators in expression)
  //
  // Init 'dstr/qstr' = Pointers for _DExpr/_QEXPR(_radix) routines
  if (find_delim_precedence(_strexp)) qstr = OperatorPrecedence(_strexp);
  
  // No operators in expression: Normal evaluation
  else qstr = _strexp;  

  dstr = qstr;          
  // 32bit / 64bit evaluate the same buffer contents
  // However a result greater than 4G is only reflected in qvalue
 
  //------------------------------------------------------------------------
  // Invoking the standard 'QExpr/DExpr(dstr,_radix)' function (RADIX_16)
  // (expression in 'dstr/qstr', must consist of ASCII hex nrs and valid operators)
  // dstr/qstr = Bin, Hex or Dec ASCII numbers here all converted into  Hex!)
  //
  error = 0; // Clear any possible previous error condition
  lvalue=0, qvalue=0;

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//if (_debugSwPass == _PASS2)
//ha//{
//ha//printf("fopcd=%s  _strexp=%s  qstr=%s  _radix=%d",  fopcd, _strexp, qstr, _radix);
//ha////printf("_strexp ");    
//ha////DebugPrintBuffer(_strexp, OPERLEN/2);
//ha//DebugStop(77, "expr()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

  // Here: qstr=dstr
  qvalue = QExpr(qstr, _radix);  // qvalue=64bit: Global result of QExpr(..)
  lvalue = DExpr(dstr, _radix);  // lvalue=32bit: Global result of DExpr(..) 

  ///////////////////////////////////////////////////////////////////////////////////
  // .MODEL SYNTAX --Extended Syntax Check-- (could be used here..)                //
  //                                                                               //
  // If 'lvalue' is derived from the 64bit expression evaluation there might be    //
  // a differnt (but correct) result.                                              //
  // In this case there will be no truncation of intermediate 64bit results.       //
  //                                                                               //
  // Example1:                                                                     //
  // Consider the following expression (was found in a real source!):              //
  //  32bit: .DW $1f4*$abc000/($400*$3E8) ;; $04F730000/($FA000) =$0515 (=lvalue)  //
  //  64bit: .DW $1f4*$abc000/($400*$3E8) ;; $14F730000/($FA000) =$1578 (=lvalue)  //
  // The 32bit result is incorrect, leads to unintended program behaviour.         //
  //                                                                               //
  // Example2:                                                                     //
  // Consider the following expression (also found in a real source!):             //
  //  32bit: .DW HWRD(~(1<<0xfe)) ;; ~(1<<0xfe)= ~(40000000) = BFFFFFFF            //
  //                              ;; HWRD(~(1<<0xfe)) = BFFF (expected)            //
  //  Compiler: shlFactor masked "SHL &= 0x1F". So 1<<0xfe = 1<<0x1e = 1<<30       //
  //                                                                               //
  //  64bit: .DW HWRD(~(1<<0xfe)) ;; ~(1<<0xfe)= ~(00000000) = FFFFFFFF            //
  //                              ;; HWRD(~(1<<0xfe)) = FFFF (Not expected !!??)   //
  //  Compiler: If (shlFactor > 63) result=0. So 1<<0xfe = 1<<254) = 0;            //
  //                                                                               //
  // Conclusion: AQbove examples rely on compiler's intrinsic calculation.         //
  // This is no good programming practice, causing unexpected program behaviour.   //
  //                                                                               //
  // No warning for Example2, programming style is silently accepted.              //
  //                                                                               //
  if (StrCmpI(fopcd, ".DQ") != 0   && // allow .DQ directive                       //
      lvalue != (ULONG)qvalue      &&                                              //
      lvalue != 0xFFFFFFFF         && // -1                                        //
      lvalue != 0x00000000         &&                                              //
      qvalue != 0xFFFFFFFFFFFFFFFF && // ~(1<<0xFE) != 0xBFFFFFFFF (32bit lvalue)  //
      qvalue != 0x0000000000000000)   //  (1<<0xFE) != 0x400000000 (32bit lvalue)  //
    {                                                                              //
    warno = WARN_MSKWRONG;                                                         //
    sprintf(pszWarntextbuf, "%s=0x%llX != 0x%X", _strexp, qvalue, lvalue);         //                          
    warnSymbol_ptr = pszWarntextbuf;                                               //
    }                                                                              //
  ///////////////////////////////////////////////////////////////////////////////////

  // If qvalue exceeds 32bit (means lvalue overflow) give a Warning
  if (swpass == _PASS2                                    &&
      (qvalue & 0xFFFFFFFF00000000) != 0xFFFFFFFF00000000 &&
      StrCmpI(fopcd, DQ_str)                              &&
      qvalue > (unsigned long long)lvalue)                  
    {
//ha//    warno = WARN_MSKVALUE;                                                   
//ha//    sprintf(pszWarntextbuf, "0x%llX", qvalue);                                   
//ha//    warnSymbol_ptr = pszWarntextbuf;
    }                                          

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//if (_debugSwPass == _PASS2)// && pcc > 0x128)
//ha//{
//ha//printf("fopcd=%s  _strexp=%s\nerror=%d  lvalue=%08lX  qvalue=%016llX\n"
//ha//       "(unsigned long long)lvalue=%016llX  (ULONG)qvalue=%08X",
//ha//        fopcd, _strexp, error, lvalue,  qvalue, (unsigned long long)lvalue, (ULONG)qvalue);
//ha////printf("_strexp ");    
//ha////DebugPrintBuffer(_strexp, OPERLEN/2);
//ha//DebugStop(99, "expr()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

  // error = 4  errorp2(ERR_LONGCONST, pszErrtextbuf); // Numeric constant too long 
  // error = 3  errorp2(ERR_ILLOP, pszErrtextbuf);     //illegal operator
  // error = 5  illegal character in expression
  // error = 6  Division by zero
  // error = 7  Division by zero MOD 0
  if (error == 6)      errorp2(ERR_DIVZERO, NULL);
  else if (error == 7) errorp2(ERR_DIVZERO, "MOD 0");
  else if (error != 0 && erno == 0) errorp2(ERR_ILLEXPR, pszErrtextbuf);
  return(lvalue);  // return the global 32bit variable as 16bit value
  } // expr


//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//----------------------------------------------------------------------------
//
//                               ValidDigit
//
// Binary    -Example str="00000000000000000011111110000101" value= 00003F85h
// Decimal   -Example str="1234567890"                       value= 499602D2h
// Hexdecimal-Example str="FFFF1234"                         value= FFFF1234h
//                                                       max value= FFFFFFFFh
int ValidDigit(unsigned char radix)
  {
  *dstr = toupper(*dstr);  // Ensure upper case for hex digit validation

  // RADIX_02
  if ((radix == RADIX_02) && (*dstr >= '0' && *dstr <= '1')) return(TRUE); //ha//
  // RADIX_10 and RADIX_16
  else if (*dstr >= '0' && *dstr <= '9') return(TRUE); 
  // RADIX_16
  else if ((radix == RADIX_16) && (*dstr >= 'A' && *dstr <= 'F')) return(TRUE);
  // Invalid  
  else return(FALSE);
  } // ValidDigit

//----------------------------------------------------------------------------
//
//                           Constant (32bit)
//                                                                         
unsigned long Constant(unsigned char radix)
  {
  unsigned long rval = 0L;
  unsigned char digit = 0, i = 0;

  if (!ValidDigit(radix))
    error = 1;              // error break if invalid char

  if (dstr[0] == '0') dstr++; // skip any leading 0

  // loop until end of string
  while (*dstr && ValidDigit(radix))               
    {
    if (*dstr >= '0' && *dstr <= '9') digit = *dstr - '0';
    else digit = *dstr - '7';

    // calculate the constant
    rval = (rval*(unsigned long)radix) + (unsigned long)digit;
    dstr++;                                    // next ascii char
    i++;                                      // count number of ascii chars
    } // end while

  switch (*dstr)  // check the syntax of allowed operators
    {
    case '+':
    case '-':
    case '*':
    case '/':
    case '|':
    case '&':
    case '^':                                       
    case '%':
    case '<':
    case '>':
    case '(':
    case ')':
    case '=':    //ha//
    case '!':    //ha//
    case ' ':    // allow spaces
    case 0:      // end-of-string
      break;
    default:     
      //error = 3;                // illegal operator
      error = 5;                  // illegal char in expression
//ha//      if (erno == 0) qstr[1] = 0; // Terminate after illegal char/operator
      break;
    } // end switch

  // Check length of ascii constant for 64bit number (16 Exabyte)
  if (((radix == RADIX_16 && i > 16)  ||  // 16E-1 =     FFFFFFFFFFFFFFFF
       (radix == RADIX_10 && i > 20)  ||  // 16E-1 = 18446744073709551615
       (radix == RADIX_02 && i > 64)) &&  // 16E-1 = 1111111111111111..11111111..11
      error == 0
     )
    { 
    error = 4;   // Error 4: ascii constant too long
    }

  return(rval);
  } // Constant

//----------------------------------------------------------------------------
//
//                             Factor (32bit)
//
unsigned long Factor(unsigned char radix)
  {
  unsigned long rval = 0L;
  char u;

  while (*dstr == ' ') dstr++;        // skip leading spaces
                                      // save and skip any unary: '-' or '~'
  if (((u = *dstr) == '~') || (u == '+') || (u == '-') || (u == '!')) dstr++;

  if (*dstr != '(') rval = Constant(radix);    // push the next constant
  else
    {                                 // handle expresion in parentheses
    dstr++;                           // skip leading parenthesis
    rval = _DExpr(radix);             // evaluate and push next expression
    if (*dstr == ')') dstr++;         // expression must have termination here
    }

  switch (u)                          // return the evaluated expression term
    {
    case '+':                         // unary: positive value returned
      unary = u;                      // set global unary
      return(rval);                   // just the (positive) value itself
      break;
    case '-':                         // unary: '-' negative value returned
      unary = u;                      // set global unary
      return(~rval+1L);               // negate value
      break;
    case '~':                         // unary: '~' NOT - binary invert
      unary = u;                      // set global unary
      return(~rval);                  // invert value
    case '!':                         // unary: '!' logical invert   //ha//
      unary = u;                      // set global unary            //ha//
      return(!rval);                  // invert value                //ha//
    default:
      return(rval);                   // return the factor value
      break;
    } // end switch
  } // Factor

//----------------------------------------------------------------------------
//
//                               _DExpr (32bit)
//
// Operators
//  (The higher the precedence, the higher the priority.) 
//  Expressions may be enclosed in parentheses, and such expressions are always
//  evaluated before combined with anything outside the parentheses.
//  The associativity of binary operators indicates the evaluation order
//  of chained operators, left associativity meaning they are evaluated
//  left to right, i.e., 2 - 3 - 4 is (2 - 3) - 4, while right associativity
//  would mean 2-3-4 is 2 - (3 - 4). Some operators are not associative,
//  i.e., chaining has no meaning.
//
// Example:  result expected = 0004
//  ((1 > 0) * 0b1111  |   2-0) << 1  |  high($1234) & 1  ; =0000 not recommended
//  ((1 > 0) * 0b1111) | ((2-0) << 1) | (high($1234) & 1) ; =0004 recommended
//
// Expression <expr> Evaluation is done left to right and can be controlled by
// parentheses. Expressions may contain:
// {()0123456789ABCDEF} {+ - * / %} {^ & | ~ >> <<} {Space}
//
// The following grammar is applied:
// <expr>   ::=    <factor>
//     | <factor> + <expr>
//     | <factor> - <expr>
//     | <factor> * <expr>
//     | <factor> / <expr>
//     | <factor> % <expr>
//     | <factor> ^ <expr>
//     | <factor> & <expr>
//     | <factor> | <expr>
//     | <factor> ~ <expr>
//     | <factor> >> <expr>
//     | <factor> << <expr>
//
// <factor>   ::=     ( <expr> )
//     | -( <expr> )
//     |  <constant>
//     | -<constant>
//
// <constant> ::=  ASCII-str containing ('0'..'F')
//
unsigned long _DExpr(int radix)
  {
  unsigned long lval;
  unsigned long _tmplval;

  lval=Factor(radix);             // get the next factor

  while (*dstr == ' ') dstr++;    // skip leading spaces
  do                              // bnf grammar, left-to-right, no precedence
    {
    switch (*dstr)                // this is the list of available operators
      {
      case '+': dstr++; lval += Factor(radix); break;
      case '-': dstr++; lval -= Factor(radix); break;
      case '*': dstr++; lval *= Factor(radix); break;
//ha//      case '/': dstr++; lval /= Factor(radix); break;
//ha//      case '%': dstr++; lval %= Factor(radix); break;
      case '/':
        dstr++;
        _tmplval = Factor(radix);
        if (_tmplval == 0) error = 6;        // Division by 0
        else lval /= _tmplval;
        break;
      case '%':                              // '%' MOD
        dstr++;
        _tmplval = Factor(radix);
        if (_tmplval == 0) error = 7;        // Division by 0 "MOD 0"
        else lval %= _tmplval;
        break;

      case '^': dstr++; lval ^= Factor(radix); break;  // '^' XOR

      case '|':
        dstr++;
        if (*dstr == '|') {                  // operator, '||' 
          dstr++; lval = (lval || Factor(radix));
          }
        else lval |= Factor(radix);          // operator, '|' OR
        break;

      case '&':
        dstr++;
        if (*dstr == '&') {                  // operator, '&&' 
          dstr++; lval = (lval && Factor(radix));
          }
        else lval &= Factor(radix);          // operator, '&' AND
        break;

      case '<':
        dstr++;
        if (*dstr == '<') {                  // operator, '<<' SHL
          dstr++; lval <<= Factor(radix);
          }
        else if (*dstr == '=') {             // operator, '<='
          dstr++; lval = ((long int)lval <= (long int)Factor(radix));
          }
        else lval = ((long int)lval < (long int)Factor(radix)); // operator, '<'
        break;

      case '>':
        dstr++;
        if (*dstr == '>') {                  // operator, '>>' SHR
          dstr++; lval >>= Factor(radix);
          }
        else if (*dstr == '=') {             // operator, '>='
          dstr++; lval = ((long int)lval >= (long int)Factor(radix));
          }
        else lval = ((long int)lval > (long int)Factor(radix)); // operator, '>'
        break;

      case '=':                             // operator, '=='
        dstr++;
        if (*dstr != '=') error = 3;        // illegal operator
        dstr++; lval = (lval == Factor(radix));
        break;

      case '!':                             // operator, '!='
        dstr++;
        if (*dstr != '=') error = 3;        // illegal operator
        dstr++; lval = (lval != Factor(radix));
        break;

      default:
        while (*dstr == ' ') dstr++;  // skip trailing spaces
        return(lval);                 // expression evaluated: break loop.
        break;
      } // end switch
    } while (TRUE);                   // end expression evaluation loop

  } // _DExpr

//----------------------------------------------------------------------------
//
//                               DExpr (32bit)
//
// Stub Initializer (_DExpr(radix) --> destroys dstr!)
//
unsigned long DExpr(char* _str, int radix)
  {
  dstr = _str;    // init global pointer to ascii expression
  return(_DExpr(radix));
  } // DExpr

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//----------------------------------------------------------------------------
//
//                          QValidDigit
//
// Binary    -Example qstr="00000000000000000011111110000101" value= 00003F85h
// Decimal   -Example qstr="1234567890"                       value= 499602D2h
// Hexdecimal-Example qstr="FFFF1234"                         value= FFFF1234h
// Octal (not implmented, needed?) 
//                                                       
BOOL QValidDigit(int radix)
  {
  *qstr = toupper(*qstr);  // Ensure upper case for hex digit validation

  // RADIX_02
  if (radix == RADIX_02 && *qstr >= '0' && *qstr <= '1') return(TRUE); //ha//
  // RADIX_10 and RADIX_16
  else if (*qstr >= '0' && *qstr <= '9') return(TRUE); 
  // RADIX_16
  else if (radix == RADIX_16 && *qstr >= 'A' && *qstr <= 'F') return(TRUE);
  // Invalid  
  else return(FALSE);
  } // QValidDigit

//----------------------------------------------------------------------------
//
//                          QConstant (64bit)
//                                                                         
unsigned long long QConstant(int radix)
  {
  unsigned long long rval = 0L;
  unsigned int digit = 0;
  int i = 0;
  char* _tmpPtr = qstr;

  if (!QValidDigit(radix))
    error = 1;                // error break if invalid char

  if (qstr[0] == '0') qstr++; // skip any leading 0

  // loop until end of qstring
  while (*qstr && QValidDigit(radix))               
    {
    if (*qstr >= '0' && *qstr <= '9') digit = (UINT)(*qstr - '0');
    else digit = (UINT)(*qstr - '7');

    // calculate the constant
    rval = (rval*(unsigned long long)radix) + (unsigned long long)digit;
    qstr++;                                   // next ascii char
    i++;                                      // count number of ascii chars
    } // end while

  switch (*qstr)  // check the syntax of allowed operators
    {
    case '+':
    case '-':
    case '*':
    case '/':
    case '|':
    case '&':
    case '^':                                       
    case '%':
    case '<':
    case '>':
    case '(':
    case ')':
    case '=':    //ha//
    case '!':    //ha//
    case ' ':    // allow spaces
    case 0:      // end-of-qstring
      break;
    default:     // illegal operator
      //error = 3;             
      error = 5;             
//ha??//      if (erno == 0) qstr[1] = 0; // Terminate after illegal operator //ha??//
      break;
    } // end switch

  // Check length of ascii constant for 64bit number (16 Exabyte)
  if (((radix == RADIX_16 && i > 16)  ||  // 16E-1 =     FFFFFFFFFFFFFFFF
       (radix == RADIX_10 && i > 20)  ||  // 16E-1 = 18446744073709551615
       (radix == RADIX_02 && i > 64)) &&  // 16E-1 = 1111111111111111..11111111..11
      error == 0
     )
    { 
    error = 4;   // Error 4: ascii constant too long
    }

  return(rval);
  } // QConstant

//----------------------------------------------------------------------------
//
//                           QFactor (64bit)
//
unsigned long long QFactor(int radix)
  {
  unsigned long long rval = 0L;
  char u;

  while (*qstr == ' ') qstr++;        // skip leading spaces
                                      // save and skip any unary: '-' or '~'
  if (((u = *qstr) == '~') || (u == '+') || (u == '-') || (u == '!')) qstr++;

  if (*qstr != '(') rval = QConstant(radix);    // push the next constant
  else
    {                                 // handle expresion in parentheses
    qstr++;                           // skip leading parenthesis
    rval = _QExpr(radix);             // evaluate and push next expression

    if (*qstr == ')') qstr++;         // expression must have termination here
    }

  switch (u)                          // return the evaluated expression term
    {
    case '+':                         // unary: positive value returned
      unary = u;                      // set global unary
      return(rval);                   // just the (positive) value itself
      break;
    case '-':                         // unary: '-' negative value returned
      unary = u;                      // set global unary
      return(~rval+1L);               // negate value
      break;
    case '~':                         // unary: '~' NOT - binary invert
      unary = u;                      // set global unary
      return(~rval);                  // invert value
    case '!':                         // unary: '!' logical invert   //ha//
      unary = u;                      // set global unary            //ha//
      return(!rval);                  // invert value                //ha//
    default:
      return(rval);                   // return the factor value
      break;
    } // end switch
  } // QFactor

//----------------------------------------------------------------------------
//
//                          _QExpr (64bit)
//
// CAVEAT Programmer: The Compiler's 64bit support is not reliable
// Example:  000001F4*00A8C000/(00000400*000003E8)  (32bit result=000004B5) OK  
//                                                  (64bit result=00001518) NOK
//
// Operators
//  (The higher the precedence, the higher the priority.) 
//  Expressions may be enclosed in parentheses, and such expressions are always
//  evaluated before combined with anything outside the parentheses.
//  The associativity of binary operators indicates the evaluation order
//  of chained operators, left associativity meaning they are evaluated
//  left to right, i.e., 2 - 3 - 4 is (2 - 3) - 4, while right associativity
//  would mean 2-3-4 is 2 - (3 - 4). Some operators are not associative,
//  i.e., chaining has no meaning.
//
// Example:  result expected = 0004
//  ((1 > 0) * 0b1111  |   2-0) << 1  |  high($1234) & 1  ; =0000 not recommended
//  ((1 > 0) * 0b1111) | ((2-0) << 1) | (high($1234) & 1) ; =0004 recommended
//
// Expression <expr> Evaluation is done left to right and can be controlled by
// parentheses. Expressions may contain:
// {()0123456789ABCDEF} {+ - * / %} {^ & | ~ >> <<} {Space}
//
// The following grammar is applied:
// <expr>   ::=    <factor>
//     | <factor> + <expr>
//     | <factor> - <expr>
//     | <factor> * <expr>
//     | <factor> / <expr>
//     | <factor> % <expr>
//     | <factor> ^ <expr>
//     | <factor> & <expr>
//     | <factor> | <expr>
//     | <factor> ~ <expr>
//     | <factor> >> <expr>
//     | <factor> << <expr>
//
// <factor>   ::=     ( <expr> )
//     | -( <expr> )
//     |  <constant>
//     | -<constant>
//
// <constant> ::=  ASCII-str containing ('0'..'F')
//
unsigned long long _QExpr(int radix)
  {
  unsigned long long qval = 0L;
  unsigned long long _tmpqval;

  qval=QFactor(radix);            // get the next factor

  while (*qstr == ' ') qstr++;    // skip leading spaces

  do                              // bnf grammar, left-to-right, no precedence
    {
    switch (*qstr)                // this is the list of available operators
      {
      case '+': qstr++; qval += QFactor(radix); break;
      case '-': qstr++; qval -= QFactor(radix); break;
      case '*': qstr++; qval *= QFactor(radix); break;
//ha//      case '/': qstr++; qval /= QFactor(radix); break;
//ha//      case '%': qstr++; qval %= QFactor(radix); break;
      case '/': 
        qstr++; 
        _tmpqval = QFactor(radix);                              
        if (_tmpqval == 0) error = 6;        // Division by 0 
        else qval /= _tmpqval;
        break;
      case '%':                              // '%' MOD
        qstr++; 
        _tmpqval = QFactor(radix);
        if (_tmpqval == 0) error = 7;        // Division by 0 "MOD 0"
        else qval %= _tmpqval;
        break;

      case '^': qstr++; qval ^= QFactor(radix); break;   // '^' XOR

      case '|':
        qstr++;
        if (*qstr == '|') {                  // operator, '||' 
          qstr++; qval = (qval || QFactor(radix));
          }
        else qval |= QFactor(radix);         // operator, '|' OR
        break;

      case '&':
        qstr++;
        if (*qstr == '&') {                  // operator, '&&' 
          qstr++; qval = (qval && QFactor(radix));
          }
        else qval &= QFactor(radix);         // operator, '&' AND
        break;

      case '<':
        qstr++;
        if (*qstr == '<') {                  // operator, '<<' SHL
          qstr++; qval <<= QFactor(radix);
          }
        else if (*qstr == '=') {             // operator, '<='
          qstr++; qval = ((long long int)qval <= (long long int)QFactor(radix));
          }
        else qval = ((long long int)qval < (long long int)QFactor(radix)); // operator, '<'
        break;

      case '>':
        qstr++;
        if (*qstr == '>') {                  // operator, '>>' SHR
          qstr++; qval >>= QFactor(radix);
          }
        else if (*qstr == '=') {             // operator, '>='
          qstr++; qval = ((long long int)qval >= (long long int)QFactor(radix));
          }
        else qval = ((long long int)qval > (long long int)QFactor(radix)); // operator, '>'
        break;

      case '=':                             // operator, '=='
        qstr++;
        if (*qstr != '=') error = 3;        // illegal operator
        qstr++; qval = (qval == QFactor(radix));
        break;

      case '!':                             // operator, '!='
        qstr++;
        if (*qstr != '=') error = 3;        // illegal operator
        qstr++; qval = (qval != QFactor(radix));
        break;

      default:
        while (*qstr == ' ') qstr++;  // skip trailing spaces
        return(qval);                 // expression evaluated: break loop.
        break;
      } // end switch
    } while (TRUE);                   // end expression evaluation loop

  } // _QExpr

//----------------------------------------------------------------------------
//
//                               QExpr (64bit)
//
// Stub Initializer (_QExpr(radix) --> destroys qstr!)
//
unsigned long long QExpr(char* _str, int radix)
  {
  qstr = _str;    // init global pointer to ascii expression
  return(_QExpr(radix));
  } // _DExpr

//----------------------------------------------------------------------------

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//if (swpass == _PASS2)
//ha//{
//ha//printf("_operand=%s  &_operand[_i]=%s  _operand[_i]='%c'  _i=%d\n",
//ha//        _operand,    &_operand[_i],    _operand[_i],      _i);
//ha//printf("_operand ");    
//ha//DebugPrintBuffer(_operand, OPERLEN/3);
//ha//DebugStop(13, "replace_token()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//if (swpass == _PASS2)
//ha//{
//ha//printf("_bkPtr=%s  _bkTmp=%s  _j=%d  _k=%d  bkBinfl=%d",
//ha//        _bkPtr,    _bkTmp,    _j,    _k,    bkBinfl);
//ha//DebugStop(1, "replace_MotorolaBin()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//if (_debugSwPass == _PASS2)
//ha//{
//ha//printf("_strexp=%s\nlvalue=%08lX  qvalue=%016llX", _strexp, lvalue, qvalue);
//ha////printf("qstr=%s\nlvalue=%016llX  qvalue=%016llX", qstr, (unsigned long long)lvalue, qvalue);
//ha//DebugStop(2, "expr()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//if (_debugSwPass == _PASS2)
//ha//{
//ha//printf("fopcd=%s\n_strexp=%s\nerror=%d  lvalue=%08lX  qvalue=%016llX\n",
//ha//        fopcd,    _strexp,   error,     lvalue,       qvalue);
//ha//printf("_strexp ");    
//ha//DebugPrintBuffer(_strexp, OPERLEN/2);
//ha//DebugStop(22, "expr()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//char __str0a[] = "-(1+1) == -2";                                                                           // OK = 01
//ha//char __str0b[] = "(((-1 ))-(( 1 ))==(( -2)))";                                                             // OK = 01
//ha//char __str0c[] = "((-1-1) == -2) * 0007";                                                                  // OK = 07
//ha//char __str0d[] = "((( ((( (((-1))-((1))==((-2))) )*( 0007)))";                                             // OK = 07
//ha//
//ha//char __str1a[] = "((( ((( (((-1))-((1))==((-2))) )*( 0007 ))|(( 1 ))-(( 0 ))) )<<( 4)))";                  // OK = 70
//ha//char __str1b[] = "((( ((( (((-1))-((1))==((-2))) )*( 0007 ))|(( 1 ))-(( 0 ))) )<<( 4 ))|(( 0003)))";       // OK = 73
//ha//char __str1c[] = "((( ((( (((-1))-((1))==((-2))) )*( 0007 ))|(( 1 ))-(( 0 ))) )<<( 4 ))|(( 0003 )&( 1)))"; // OK = 71
//ha//char __str1d[] = "((( ((( (((-1))-((1))==((-2))) )*( 0007 ))|(( 1 ))-(( 0 ))) )<<( 1 ))|(( 0012 )&( 1)))"; // OK = 0E
//ha//
//ha//char __str3a[] = "((-1-1==-2)*0007|1-0)<<1|0012&1";                    // scratch                          // NOK ?00
//ha//char __str3b[] = "((((-1-1 == -2) * 0007) | (1-0)) << 1) | 0012 & 1)"; // OK Intended;                     // OK = 0E
//ha//char __str3c[] = "((( ((( (((-1))-((1))==((-2))) )*( 0007 ))|(( 1 ))-(( 0 ))) )<<( 1 ))|(( 0012 )&( 1)))"; // OK = 0E
//ha//
//ha//str = __str0a;  _radix=16;
//ha//printf("__str0a=%s\n", str);
//ha//lvalue = Expr(_radix);   // str = 32bit hex: Global result of expression in 
//ha//printf("__str0a: lValue=%08X\n", lvalue);
//ha//
//ha//str = __str0b;  _radix=16;
//ha//printf("__str0b=%s\n", str);
//ha//lvalue = Expr(_radix);   // str = 32bit hex: Global result of expression in 
//ha//printf("__str0b: lValue=%08X\n", lvalue);
//ha//
//ha//str = __str0c;  _radix=16;
//ha//printf("__str0c=%s\n", str);
//ha//lvalue = Expr(_radix);   // str = 32bit hex: Global result of expression in 
//ha//printf("__str0c: lValue=%08X\n", lvalue);
//ha//
//ha//str = __str0d;  _radix=16;
//ha//printf("__str0d=%s\n", str);
//ha//lvalue = Expr(_radix);   // str = 32bit hex: Global result of expression in 
//ha//printf("__str0d: lValue=%08X\n", lvalue);
//ha//
//ha//str = __str1a;  _radix=16;
//ha//printf("__str1a=%s\n", str);
//ha//lvalue = Expr(_radix);   // str = 32bit hex: Global result of expression in 
//ha//printf("__str1a: lValue=%08X\n", lvalue);
//ha//
//ha//str = __str1b;  _radix=16;
//ha//printf("__str1b=%s\n", str);
//ha//lvalue = Expr(_radix);   // str = 32bit hex: Global result of expression in 
//ha//printf("__str1b: lValue=%08X\n", lvalue);
//ha//
//ha//str = __str1c;  _radix=16;
//ha//printf("__str1c=%s\n", str);
//ha//lvalue = Expr(_radix);   // str = 32bit hex: Global result of expression in 
//ha//printf("__str1c: lValue=%08X\n", lvalue);
//ha//
//ha//str = __str1d;  _radix=16;
//ha//printf("__str1d=%s\n", str);
//ha//lvalue = Expr(_radix);   // str = 32bit hex: Global result of expression in 
//ha//printf("__str1d: lValue=%08X\n", lvalue);
//ha//
//ha//str = __str3a;  _radix=16;
//ha//printf("__str3a=%s\n", str);
//ha//lvalue = Expr(_radix);   // str = 32bit hex: Global result of expression in 
//ha//printf("__str3a: lValue=%08X\n", lvalue);
//ha//
//ha//str = __str3b;  _radix=16;
//ha//printf("__str3b=%s\n", str);
//ha//lvalue = Expr(_radix);   // str = 32bit hex: Global result of expression in 
//ha//printf("__str3b: lValue=%08X\n", lvalue);
//ha//
//ha//str = __str3c;  _radix=16;
//ha//printf("__str3c=%s\n", str);
//ha//lvalue = Expr(_radix);   // str = 32bit hex: Global result of expression in 
//ha//printf("__str3c: lValue=%08X\n", lvalue);
//ha//
//ha//exit(SYSERR_ABORT);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("fopcd=%s  str=%s  error=%d  errSymbol_ptr=[%s]", fopcd, str, error, errSymbol_ptr);
//ha//printf("str ");    
//ha//DebugPrintBuffer(str, OPERLEN/2);
//ha//DebugStop(11, "check_number()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha////if (_debugSwPass == PASS2)
//ha//{
//ha//printf("pcc=%04X  error=%d  _exprInfo=%02X  _numval=%08X  _operand=%s\nflabl=%s  fopcd=%s  oper1=%s  oper2=%s  oper3=%s\n",
//ha//        pcc,      error,    _exprInfo,      _numval,      _operand,    flabl,    fopcd,    oper1,    oper2,    oper3);
//ha//printf("_strnum_tmp ");   
//ha//DebugPrintBuffer(_strnum_tmp, OPERLEN);
//ha////ha//printf("_strexp ");   
//ha////ha//DebugPrintBuffer(_strexp, SYMLEN+1);
//ha//printf("_operand_ptr ");    
//ha//DebugPrintBuffer(_operand_ptr, OPERLEN/2);
//ha//DebugStop(1, "check_symbol()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//--------------------------end-of-c++-module-----------------------------------


