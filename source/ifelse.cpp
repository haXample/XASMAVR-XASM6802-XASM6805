// haXASM - Cross-Assembler for 8bit Microprocessors
// ifelse.cpp - C++ Developer source file.
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

#include <fcntl.h>   // Console
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <iostream>
#include <conio.h>

#include <shlwapi.h>  // Library shlwapi.lib for PathFileExistsA
#include <commctrl.h> // Library Comctl32.lib
#include <commdlg.h>
#include <winuser.h>
#include <windows.h>

#include <string.h>
#include <string>     // sprintf, etc.
#include <tchar.h>     
#include <strsafe.h>  // <strsafe.h> must be included after <tchar.h>

#include "equate.h"
#include "extern.h"

using namespace std;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// https://stackoverflow.com/questions/9283288/parsing-if-else-if-statement-algorithm
// Parsing if-else if statement algorithm
// 
// The first step is always to write down the syntax of the language
// in an appropriate way, e.g. in EBNF, which is very simple to understand.
// 
//    | separates alternatives.
// [..] enclose options.
// {..} denote repetitions (zero, one or more).
// (..) group expressions (not used here).
// 
// The EBNF syntax
// ---------------
// SrcLine      = { TextLine | IfStatement }.
// TextLine     = <string>.
// IfLine       = ".if" "(" Condition ")".
// ElseLine     = ".else".
// ElseIfLine   = ".elseif" "(" Condition ")".
// EndLine      = ".endif".
// Condition    = Identifier "=" Identifier.
// Identifier   = <letter_or_underline> { <letter_or_underline> | <digit> }.
// IfStatement  = IfLine SrcLine { ElseIfLine SrcLine } [ ElseLine SrcLine ] EndLine.
// 
// The parser follows closely the syntax, i.e. a repetition is translated
//  into a loop, an alternative into an if-else statement, and so on.
//  The syntax elements (evaluation) are processed recursively.
//  This reflects the recursive nature of the syntax:
//  An IfStatement can contain yet another IfStatement in one of its branches.
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//--DEBUG--DEBUG--DEBUG
//#define _DEBUG //-DEBUG
//--DEBUG--DEBUG--DEBUG

#define CNONE         0x00  // No conditional operator 

#define CTRUE         0x01  // Condition = TRUE
#define CTRUE_IFDEF   0x11  // CMOD_IFDEF  | CTRUE  
#define CTRUE_IFNDEF  0x21  // CMOD_IFNDEF | CTRUE
#define CTRUE_IF      0x31  // CMOD_IF     | CTRUE

#define CFALSE        0x0E  // Condition = FALSE
#define CFALSE_IFDEF  0x1E  // CMOD_IFDEF  | CFALSE
#define CFALSE_IFNDEF 0x2E  // CMOD_IFNDEF | CFALSE
#define CFALSE_IF     0x3E  // CMOD_IF     | CFALSE

// Global variables
int p12_swifdef  = 0;
int p12_swifndef = 0;
int p12_swif     = 0;
int p12_swelif   = 0;
int p12_swelse   = 0;

int p12Cswifdef  = 0;
int p12Cswifndef = 0;
int p12Cswif     = 0;
int p12Cswelif   = 0;
int p12Cswelse   = 0;

// Extern variables and functions
extern void DebugStop(int, char*, char*); // Usage: DebugStop(testNr, "Function()", __FILE__);
extern void DebugPrintBuffer(char*, int); // Usage: DebugPrintBuffer(buffer, count);
extern void DebugPrintIfelseArray(int);
extern void DebugPrintSymbolArray(int);

extern char* pszCurFileName;

extern char* _elif_str;      
extern char* _elif_def_str;   
extern char* _elif_ndef_str;  
extern char* _elseif_str;     
extern char* _elseifdef_str;  
extern char* _elseifndef_str; 

extern void errorp2(int, char*);

extern BOOL p1pse();
extern BOOL p2pse();                              
extern void clepc();
extern UCHAR labfn(char*);
extern UINT expr(char*);
extern BOOL p1lab();

// Forward declaration of functions included in this code module:
void p12Cifdef(int);
void p12Cifndef(int);
void p12Cif(int);
void p12Celseif(int);
void p12Celse(int);
void p12Cendif(int);
BOOL GetCurrentCondition();

//-----------------------------------------------------------------------------
//
// .IF .IFDEF .IFNDEF .ELSE .ELIF .ELSEIF .ENDIF
// Atmel/Microchip AVR Assembler Manual (c)2017, DS40001917A-page 19:
// "Conditional assembly includes a set of commands at assembly time.
//  The .IFDEF <symbol> directive will include code until the corresponding
//  .ELSE directive, if <symbol> is defined.
//  The symbol must be defined with the .EQU or .SET directive.
//  (Will not work with the .DEF directive.)
//  The IF <expr> directive will include code if <expr> is evaluated != 0.
//  The .IF directive is Valid until the corresponding ELSE or ENDIF directive.
//  Up to five levels of nesting is possible."
//
// #if #ifdef #ifndef #else #elif #elseif #endif
// Atmel/Microchip AVR Assembler Manual (c)2017, DS40001917A-page 26:
//  "All the following lines until the corresponding #endif, #else, or #elif
//  are conditionally assembled if  condition is true (not equal to 0).
//  Condition may be any integer expression, including preprocessor macros, 
//  which are expanded. The preprocessor recognizes the special operator
//  #defined (name) that  returns 1 if the name is defined and 0 otherwise.
//  Any undefined symbols used in condition are silently evaluated to 0."
//
// Syntax
// #define symbol
// #define symbol (expr)
//
// #ifdef       <symbol>
// #ifndef      <symbol>
// #if defined  <symbol>
// #if !defined <symbol>
//
// .IF   <condition>
// .ELIF <condition>
//
// #elif defined  <symbol>
// #elif !defined <symbol>
// #else <>
// #endif
//

//-----------------------------------------------------------------------------
//
//                         ClearIfdefStack
//
// Clear structured ifdef buffer
//  typedef struct tagCIFDEF {      // #ifelse cascade structure
//    char      Cifdef[2];         
//    char      Cifndef[2];        
//    char      Cif[2];
//    union {
//      char    CelifCnt;        
//      char    Celif[IFELSEMAX];
//    };          
//    char      Celse[2];          
//  } CIFDEF, *LPCIFDEF;
//
void ClearIfdefStack()
  {                                 
  // Define a local pointer to the global array of operandX structures
  LPCIFDEF ifdefstruc_ptr = preprocessStack;

  // Init-clear global array of ifdefstruc structures 
  for (_i=0; _i<IFELSEMAX; _i++)
    {
    ifdefstruc_ptr->Cifdef[0]  = 0; //_j | _i<<4; // Check
    ifdefstruc_ptr->Cifdef[1]  = 0; //_j | _i<<4; // Check
    ifdefstruc_ptr->Cifndef[0] = 0; //_j | _i<<4;
    ifdefstruc_ptr->Cifndef[1] = 0; //_j | _i<<4;
    ifdefstruc_ptr->Cif[0]     = 0; //_j | _i<<4;
    ifdefstruc_ptr->Cif[1]     = 0; //_j | _i<<4;
    ifdefstruc_ptr->Celse[0]   = 0; //_j | _i<<4;
    ifdefstruc_ptr->Celse[1]   = 0; //_j | _i<<4;
    for (_j=0; _j<IFELSEMAX; _j++) ifdefstruc_ptr->Celif[_j] = 0;
    // Advance pointer by 'sizeof(OPERXBUF)' and point to next structure
    ifdefstruc_ptr++;                           
    } // end for

  // Init global pointer to array of #ifelse
  preprocessStack_ptr = preprocessStack;

  p1Flagifndef = 0;
  CifndefCnt = 0, CifdefCnt = 0, CifCnt = 0;
  } // ClearIfdefStack

//-----------------------------------------------------------------------------
//
//                         p12CCheckIfend
//
// Invoked if SrcFile.eof() is encountered.
// 
void p12CCheckIfend()
  {
  char* MissingfEndif = "%s(%d): ERROR - Missing .ENDIF / #endif\n";

  // In case that #endif/.ENDIF is last and CRLF is missing
  if (srcEOF == TRUE && (StrStrI(inbuf, ".ENDIF") || StrStrI(inbuf, "#endif")))
    {
    if (CifndefCnt > 0)      CifndefCnt--;
    else if (CifdefCnt > 0)  CifdefCnt--;
    else if (CifCnt > 0)     CifCnt--;
    }
  // Final check: no open IFs   
  if (CifndefCnt != 0 || CifdefCnt != 0 || CifCnt != 0)
    {
    printf(MissingfEndif, pszCurFileName, _curline);
    exit(SYSERR_ABORT); 
    }
  } // p12CCheckIfend
 
//-----------------------------------------------------------------------------
//
//                         p12CifelseSkipProcess()
//
// Conditionally suppressed via #if #ifdef #ifndef #elseif
// Return: TRUE  = Suppress (skip) src line
//         FALSE = Process (assemble) src line                
//
int p12CifelseSkipProcess(int _p12Csw)
  {
  int _rIFELSESKIP = FALSE;  // Initially normal (unconditional) processing

  if (ins_group == 1 && (insv[1] == 52 || insv[1] == 53))
    {
    _rIFELSESKIP = FALSE;    // show .MACRO/.ENDM directives
    } // end else if

  // Next line if not a conditional directive
  else if (ins_group != 1 || (ins_group == 1 && (insv[1] < 57 || insv[1] > 62)))
    {                                   
    // p12CSkipFlag == FALSE:  Suppress src line                 --> _rIFELSESKIP == TRUE                                                   
    // p12CSkipFlag == TRUE:   Process  src line                 --> _rIFELSESKIP == FALSE
    // p12CSkipFlag == ~CNONE: Normal (unconditional) processing --> _rIFELSESKIP == FALSE                                                  
    if (p12CSkipFlag != ~CNONE) _rIFELSESKIP = !(p12CSkipFlag & TRUE); // need invert
    }

  // Process .IF/.ELSE/.ENDIF/... directives
  else if (ins_group == 1 && insv[1] >= 57 && insv[1] <= 62)
    {
    // Pass1: Allow label if present, although it's a syntax error     //ha//
    // Example: "label1: .IF (TSTLED == 1)" ..should not be allowed!   //ha//
    if (_p12Csw == _PASS1) { p1lab(); _rIFELSESKIP = p1pse() & TRUE; }
    // Pass2: Normal directive processing
    else if (_p12Csw == _PASS2) { _rIFELSESKIP = p2pse() & TRUE; }   
    }

#ifdef _DEBUG //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//if (_debugSwPass == _PASS2 && _rIFELSESKIP == FALSE) // stop          //--DEBUG--
if (_debugSwPass ==   NULL && _rIFELSESKIP == FALSE) // no stop       //--DEBUG--
{                                                                     //--DEBUG--
printf("p12CSkipFlag=%02X    _rIFELSESKIP=%d\n"
       "ins_group=%d insv[1]=%d pcc=%08X [%08X]",
        (UCHAR)p12CSkipFlag, _rIFELSESKIP,
        ins_group,   insv[1],   pcc, pccw);
DebugStop(77, "p2CifelseSkipProcess()", __FILE__);                    //--DEBUG--
}                                                                     //--DEBUG--
else if (_p12Csw == _PASS2 && ins_group == 1 && insv[1] >= 57 && insv[1] <= 62)
  printf("PASS2 [088--p2CifelseSkipProcess()] p12CSkipFlag=%02X  _rIFELSESKIP=%d\n"
         "-------------------------------------------------------------------\n",
                           (UCHAR)p12CSkipFlag, _rIFELSESKIP);        //--DEBUG--
#endif //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

  return(_rIFELSESKIP);                                                     
  } // p12CifelseSkipProcess()


//-----------------------------------------------------------------------------
//
//                               p12Cundef
// #undef <symbol>
//
// Undefine symbol name previously defined with a #define directive.
// If name is not defined, the #undef directive is silently ignored.
//
// Return: FALSE = symbol is undefined
//         TRUE  = symbol is defined 
//
BOOL p12Cundef(int _p12Csw)
  {
  int _rUNDEF = FALSE;   // Initially an undefined symbol is assumed
  int _i;

  if (oper1[0] == 0)     // Check operand for #ifdef #ifndef #undef
    {
    errorp2(ERR_SYNTAX, "Missing operand");
    return(_rUNDEF);
    }

  // Invoked: p12Cundef(0x01)
  // Invoked: p12Cundef(0x02)
  if (_p12Csw == _PASS1 || _p12Csw == _PASS2)
    {
    // Get symType, get 'symboltab_ptr'
    if (labfn(oper1) != FALSE)           
      {
      // Symbol name in oper1 is defined. 
      // Find next empty location in symUndefBuf and store as undef'd symbol
      _i=0;
      while (_i < (SYMUNDEFBUFSIZE-SYMLEN)) 
        {
        if (symUndefBuf[_i] == 0 && symUndefBuf[0] != 0) _i++;  // _i==0
        if (symUndefBuf[_i] == 0 && symUndefBuf[_i+1] == 0)     // double zero
          {
          StrCpy(&symUndefBuf[_i], oper1);
          symUndefBuf[_i+strlen(oper1)] = 0;   // terminate string
          _i += strlen(oper1)+1;
          // Set warning code   
          if (_i >= (SYMUNDEFBUFSIZE-SYMLEN)) warno = WARN_IGNORED;       
          else _rUNDEF = TRUE;
          break;                               // symbol is now #undef'd
          } 
        _i++;                                  // 'Baby Steps' forward
        } // end while
      } // end if (labfn)
    } // end if (_PASS1 || _PASS2)

  // Invoked: p12Cundef(0x00)
  // Check if symbol has been previously undef'd by #undef directive
  // FALSE = Initially not undef'd
  // TRUE  = Symbol is no longer defined and must be ignored
  else if (_p12Csw == _PASS12)  // Pass1 / Pass2
    {
    // Search symbol name (oper1) in symUndefBuf to check if it's undef'd
    symUndefBuf_ptr = symUndefBuf;
    _i=0;
    while (_i < (SYMUNDEFBUFSIZE-SYMLEN)) 
      {
      if (*symUndefBuf_ptr == 0 && *(symUndefBuf_ptr+1) == 0) break; // not found
      else if (StrCmpI(symUndefBuf_ptr, oper1) == 0)
        {
        _rUNDEF = TRUE;  // found symbol
        break;
        }
      _k = strlen(symUndefBuf_ptr)+1;  // Advance ptr and index count
      symUndefBuf_ptr += _k; _i +=_k;
      } // end while
    } // end if (_PASS12)
  
  // Invoked: p12Cundef(0x03)
  else if (_p12Csw == (_PASS1|_PASS2) && StrCmp(fopcd, "#define") == 0)   
    {
    // Search symbol name (oper1) in symUndefBuf to check if it's undef'd
    symUndefBuf_ptr = symUndefBuf;
    _i=0;
    while (_i < (SYMUNDEFBUFSIZE-SYMLEN)) 
      {
      if (*symUndefBuf_ptr == 0 && *(symUndefBuf_ptr+1) == 0) break; // not found
      else if (StrCmpI(symUndefBuf_ptr, oper1) == 0)
        {
        // Invalidate the #undef'd symbol (#define it again)
        *symUndefBuf_ptr = '!'; 
        _rUNDEF = TRUE;  
        break;
        }
      _k = strlen(symUndefBuf_ptr)+1;  // Advance ptr and index count
      symUndefBuf_ptr += _k; _i +=_k;
      } // end while
    } // end if (_PASS12)

  return(_rUNDEF);
  } // p12Cundef


//-----------------------------------------------------------------------------
//
//                               DetermineConditionIF
// #ifdef  <symbol>
// #ifndef <symbol>
// #if     <expr>
//
// Return the dependent #if condition determined by _symexp.
//
//  typedef struct tagCIFDEF {      // #ifelse cascade structure
//    char      Cifdef[2];         
//    char      Cifndef[2];        
//    char      Cif[2];
//    union {
//      char    CelifCnt;        
//      char    Celif[IFELSEMAX];
//    };          
//    char      Celse[2];          
//  } CIFDEF, *LPCIFDEF;
//
int DetermineConditionIF(UINT _symexp, int _cmod)
  {
  int _pIF, _cIF, _cELIF, _cELSE, _rIF;

  // Check previous #if conditions. 
  if ((preprocessStack_ptr-1)->Cifdef[1] == CFALSE_IFDEF)        _pIF = CFALSE;
  else if ((preprocessStack_ptr-1)->Cifndef[1] == CFALSE_IFNDEF) _pIF = CFALSE;
  else if ((preprocessStack_ptr-1)->Cif[1] == CFALSE_IF)         _pIF = CFALSE;
  else _pIF = CTRUE;
  // Check current #if conditions.  
  if (preprocessStack_ptr->Cifdef[1] == CFALSE_IFDEF)        _cIF = CFALSE;
  else if (preprocessStack_ptr->Cifndef[1] == CFALSE_IFNDEF) _cIF = CFALSE;
  else if (preprocessStack_ptr->Cif[1] == CFALSE_IF)         _cIF = CFALSE;
  else _cIF = CTRUE;

  // Get current #elif condition
  _cELIF = preprocessStack_ptr->Celif[preprocessStack_ptr->CelifCnt];
    
  // Check all possible current #else/.ELSE
  // 00:    #else condition is missing
  // 01/0E: #else condition is true/false
  _i=0; 
  while (_i++ < IFELSEMAX)
    {
    _cELSE = preprocessStack[_i].Celse[1];
    if (_cELSE != CNONE) break; 
    } // end while

  // Process/suppress src according to #if/.IF operand value
  // First check and process #else followed by #if condition (= #elif)
  if (_cELSE == CTRUE) _rIF = (UCHAR)_symexp;   
  // Next check and process #elif followed by #if condition
  else if (_cELIF == CTRUE) _rIF = (UCHAR)_symexp;

  // Process follower conditions depending on #elif
  else if (_cELIF == CTRUE  && _cIF == CFALSE) _rIF = (UCHAR)_symexp;
  else if (_cELIF == CFALSE || _cIF == CFALSE || _pIF == CFALSE) _rIF = CFALSE;

  // Process/suppress src according to #if/.IF operand value
  else if (_cELSE == CNONE) _rIF = (UCHAR)_symexp;
  else _rIF = CFALSE;
 
  // Get current #else condition (avrtest08.asm)
  _cELSE = preprocessStack_ptr->Celse[1];  
  // Establish the dedicated current #ifdef/#ifndef/#if conditions
  switch(_cmod)                                            
    {
    case CMOD_IFDEF:
      if (_cELIF == CTRUE) _rIF = (UCHAR)_symexp;                    //ha//
      else if (preprocessStack_ptr->Cifdef[1] == CFALSE_IFDEF &&     //ha//
               (_cELSE == CFALSE || _cELSE == CNONE)) _rIF = CFALSE; //ha//
      // Advance 'ifelse' stack pointer
      preprocessStack_ptr++;  
      preprocessStack_ptr->Cifdef[1] = _rIF|_cmod;
      break;

    case CMOD_IFNDEF:
      if (_cELIF == CTRUE) _rIF = (UCHAR)_symexp;                    //ha//
      else if (preprocessStack_ptr->Cifndef[1] == CFALSE_IFNDEF &&   //ha//
               (_cELSE == CFALSE || _cELSE == CNONE)) _rIF = CFALSE; //ha//
      // Advance 'ifelse' stack pointer
      preprocessStack_ptr++;  
      preprocessStack_ptr->Cifndef[1] = _rIF|_cmod;
      break;

    case CMOD_IF:
      if (_cELIF == CTRUE) _rIF = (UCHAR)_symexp;                    //ha//
      else if (preprocessStack_ptr->Cif[1] == CFALSE_IF &&           //ha//
               (_cELSE == CFALSE || _cELSE == CNONE)) _rIF = CFALSE; //ha//
      // Advance 'ifelse' stack pointer
      preprocessStack_ptr++;  
      preprocessStack_ptr->Cif[1] = _rIF|_cmod;     
      break;
    default:
      _rIF = CTRUE;
      break;
    } // end switch

#ifdef _DEBUG //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
if (_debugSwPass == _PASS2)                                                         //--DEBUG--
{                                                                                   //--DEBUG--
DebugPrintIfelseArray(8);
printf("_pIF=%02X _cIF=%02X _cELIF=%02X _cELSE=%02X _rIF=%02X",
        _pIF,     _cIF,     _cELIF,     _cELSE,     _rIF|_cmod);
if (_cmod == CMOD_IFDEF)  DebugStop(11, "DetermineConditionIFDEF()",  __FILE__);    //--DEBUG--
if (_cmod == CMOD_IFNDEF) DebugStop(22, "DetermineConditionIFNDEF()", __FILE__);    //--DEBUG--
if (_cmod == CMOD_IF)     DebugStop(33, "DetermineConditionIF()", __FILE__);        //--DEBUG--
}                                                                                   //--DEBUG--
#endif //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

  // Return the current 'ifelse' condition
  return(_rIF|_cmod);          
  } // DetermineConditionIF

//-----------------------------------------------------------------------------
//
//                         DetermineConditionELIF()
// #elif <symbol>
// #elif <expr>
//
// Establish the dependent #elif condition determined by _symexp.
//
//  typedef struct tagCIFDEF {      // #ifelse cascade structure
//   char      Cifdef[2];         
//   char      Cifndef[2];        
//   char      Cif[2];          
//    union {
//      char    CelifCnt;        
//   char      Celif[IFELSEMAX];          
//    };          
//   char      Celse[2];          
//  } CIFDEF, *LPCIFDEF;
//
int DetermineConditionELIF(UINT _symexp)
  {
  int _cIF, _pIF, _nopIF, _cELIF, _rELIF;

  // Check previous #if conditions
  _nopIF = (preprocessStack_ptr-1)->Cifdef[1]  |
           (preprocessStack_ptr-1)->Cifndef[1] |
           (preprocessStack_ptr-1)->Cif[1];

  // Suppress src if one of the previous #ifdef/#ifndef/#elif was FALSE
  if ((preprocessStack_ptr-1)->Cifdef[1] == CFALSE_IFDEF   ||
      (preprocessStack_ptr-1)->Cifndef[1] == CFALSE_IFNDEF ||
      (preprocessStack_ptr-1)->Cif[1] == CFALSE_IF)             
    {  _pIF = CFALSE;  }
  else _pIF = CTRUE;

  // Check current #if conditions
  _cIF = preprocessStack_ptr->Cifdef[1]  |
         preprocessStack_ptr->Cifndef[1] |
         preprocessStack_ptr->Cif[1];
  // Missing current #if conditions cause an error
  if (_cIF == CNONE) { errorp2(ERR_SYNTAX, NULL); return(ERR); }

  // Check current #if conditions. #else is true if all #if.. are false. 
  if (preprocessStack_ptr->Cifdef[1] == CFALSE_IFDEF   ||
      preprocessStack_ptr->Cifndef[1] == CFALSE_IFNDEF ||
      preprocessStack_ptr->Cif[1] == CFALSE_IF)             
    {  _cIF = CFALSE;  }
  else _cIF = CTRUE;
  
  // Check all possible current #elif
  _i=0; _cELIF = CFALSE;
  while (_i++ < IFELSEMAX && preprocessStack_ptr->Celif[_i] != 0)
    {
    if (preprocessStack_ptr->Celif[_i] == CTRUE)
      { _cELIF = CTRUE;  break; }
    } // end while

  // Previous #ifdef/#ifndef/#if ==CFALSE or #elif ==CTRUE
  if (_pIF == CFALSE || _cELIF == CTRUE) _rELIF = CFALSE;  

  // Previous #ifdef/#ifndef/#if ==CTRUE or _nopIF ==CNONE (not present)
  else if (_pIF == CTRUE || _nopIF == CNONE)
    {
    // current #elif ==CTRUE
    // current #ifdef/#ifndef/#if ==CTRUE
    if (_cELIF == CTRUE || _cIF == CTRUE) _rELIF = CFALSE;

    // current  #elif ==CFALSE
    // current  #ifdef/#ifndef/#if ==CFALSE
    else if (_cELIF == CFALSE && _cIF == CFALSE) _rELIF = _symexp;
    } // end else if

  // Store current #elif condition into next #elif slot
  (preprocessStack_ptr->CelifCnt)++;        
  preprocessStack_ptr->Celif[preprocessStack_ptr->CelifCnt] = _rELIF;

#ifdef _DEBUG //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
if (_debugSwPass == _PASS2)                                           //--DEBUG--
{                                                                     //--DEBUG--
DebugPrintIfelseArray(8);
printf("_pIF=%02X _nopIF=%02X _cIF=%02X _cELIF=%02X  p12CSkipFlag=%02X",
        _pIF,     _nopIF,     _cIF,     _cELIF,      _rELIF);
DebugStop(40, "DetermineConditionELIF()", __FILE__);                  //--DEBUG--
}                                                                     //--DEBUG--
#endif //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

  return(_rELIF);
  } // DetermineConditionELIF()


//-----------------------------------------------------------------------------
//
//                         DetermineConditionELSE()
// #else / .ELSE
//
// Establish the #else condition depending on #ifdef/#ifndef/#if/#elif
//
//  typedef struct tagCIFDEF {      // #ifelse cascade structure
//    char      Cifdef[2];         
//    char      Cifndef[2];        
//    char      Cif[2];
//    union {
//      char    CelifCnt;        
//      char    Celif[IFELSEMAX];
//    };          
//    char      Celse[2];          
//  } CIFDEF, *LPCIFDEF;
//
int DetermineConditionELSE()
  {
  int  _pIF, _nopIF, _cIF, _cELIF, _rELSE=CFALSE;  // _rELSE: default = FALSE
  
  // Check previous #if conditions
  _nopIF = (preprocessStack_ptr-1)->Cifdef[1]  |
           (preprocessStack_ptr-1)->Cifndef[1] |
           (preprocessStack_ptr-1)->Cif[1];

  if ((preprocessStack_ptr-1)->Cifdef[1]  == CFALSE_IFDEF  ||
      (preprocessStack_ptr-1)->Cifndef[1] == CFALSE_IFNDEF ||
      (preprocessStack_ptr-1)->Cif[1]     == CFALSE_IF)             
    { _pIF = CFALSE; }
  // One of the previous #ifdef/#ifndef/#if was TRUE
  else _pIF = CTRUE;

  // Check current #if conditions
  _cIF = preprocessStack_ptr->Cifdef[1]    |
           preprocessStack_ptr->Cifndef[1] |
           preprocessStack_ptr->Cif[1];
  // Missing current #if conditions cause an error
  if (_cIF == CNONE) { errorp2(ERR_SYNTAX, NULL); return(ERR); }

  // Check current #if conditions. #else is true if all #if.. are false. 
  if (preprocessStack_ptr->Cifdef[1]  == CFALSE_IFDEF  ||
      preprocessStack_ptr->Cifndef[1] == CFALSE_IFNDEF ||
      preprocessStack_ptr->Cif[1]     == CFALSE_IF)             
    { _cIF = CFALSE; }
  // One of the current #ifdef/#ifndef/#if is TRUE
  else _cIF = CTRUE;
  
  // Check all possible current #elif
  // #else is true if all #elif conditions are false (or not present).
  _i=0; _cELIF = CFALSE;
  while (_i++ < IFELSEMAX && preprocessStack_ptr->Celif[_i] != 0)
    {
    if (preprocessStack_ptr->Celif[_i] == CTRUE)
      {
      _cELIF = CTRUE; 
      break;
      }
    } // end while

  // Build current #else condition
  // While previous #ifdef/#ifndef/#if was FALSE
  //  check if any current #ifdef/#ifndef/#if/#elif is TRUE
  if ((_pIF == CFALSE) && (_cIF == CTRUE || _cELIF == CTRUE)) _rELSE = CFALSE;

  // Previous #ifdef/#ifndef/#if _pIF==CTRUE  (or not present)
  // current  #ifdef/#ifndef/#if _cIF==CFALSE
  // current  #elif              _cELIF==CFALSE
  else if ((_pIF == CTRUE) && _cIF == CFALSE && _cELIF == CFALSE) _rELSE = CTRUE;

  // else _rELSE = CFALSE; // default (see init of local variable _rELSE)

  preprocessStack_ptr->Celse[1] = _rELSE;

#ifdef _DEBUG //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
if (_debugSwPass == _PASS2)                                           //--DEBUG--
{                                                                     //--DEBUG--
DebugPrintIfelseArray(8);
printf("_pIF=%02X  _cIF=%02X  _cELIF=%02X  p12CSkipFlag=%02X",
        _pIF,      _cIF,      _cELIF,      _rELSE);
DebugStop(50, "DetermineConditionELSE()", __FILE__);                  //--DEBUG--
}                                                                     //--DEBUG--
#endif //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

  return(_rELSE);
  } // DetermineConditionELSE


//-----------------------------------------------------------------------------
//
//                               p12Cifdef()
// #ifdef <symbol>
//
// All the following lines until the corresponding #endif/#else/#elif
// are conditionally assembled if symbol is previously #defined. 
// Shorthand for #if defined (symbol).
//
// Note: ;; Using a forward reference in an assembler conditional
//       ;; may cause surprises, and in some cases is not allowed.
//
void p12Cifdef(int _p12Csw)
  {
  UCHAR _rsltLabfn;
  
  // Get symType, get 'symboltab_ptr'
  _rsltLabfn = labfn(oper1);

  // check if symbol is undef'd
  if (p12Cundef(_PASS12) == TRUE) _rsltLabfn = FALSE;

  // needed in Pass1
  if (_p12Csw == _PASS1 && oper1[0] != 0) StrCpy(p1Symifndef, oper1);   //ha//

#ifdef _DEBUG //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
if (_debugSwPass == _PASS2)                                           //--DEBUG--
{                                                                     //--DEBUG--
DebugPrintIfelseArray(8);
printf("..Stack=%08X [size=%d]  ..Stack_ptr=%08X  =..Stack[%d][]\n"
       "  CifdefCnt=%d    CifndefCnt=%d    CifCnt=%d  CelifCnt=%d\n"
       "p12Cswifdef=%d  p12Cswifndef=%d  p12Cswif=%d\n"
       "_rsltLabfn  =%02X  p1Symifndef=%s\n"
       "p12CSkipFlag=%02X  pcc=%08X [%08X]",
        preprocessStack, sizeof(CIFDEF), preprocessStack_ptr, (preprocessStack_ptr-preprocessStack),
          CifdefCnt,      CifndefCnt,      CifCnt,    preprocessStack_ptr->CelifCnt,
        p12Cswifdef,    p12Cswifndef,    p12Cswif,
        _rsltLabfn,        p1Symifndef,
        (UCHAR)p12CSkipFlag, pcc, pccw);
DebugStop(10, "p12Cifdef()", __FILE__);                               //--DEBUG--
}                                                                     //--DEBUG--
#endif //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

  CifdefCnt++;

  // Symbol, if found, must be of type absolute (_ABSEQU or _ABSSET)
  if (CifdefCnt > IFELSEMAX && _rsltLabfn != FALSE &&
      (!(_rsltLabfn & _ABSEQU) || !(_rsltLabfn & _ABSSET)))
    {
    if (_p12Csw == _PASS1) return;
    else
      {
      errorp2(ERR_TOOLONG, symboltab_ptr->symString); // > IFELSEMAX
      return; 
      }
    } // end if (CifdefCnt)

  // XASMAVR_test.ASM(997): |ERROR| Pass1 phase error (fixed)                   //ha//
  // Process follower conditions (needed from Pass1)                            //ha//   
  // Check 'symType' if Symbol is defined during Pass1 within #ifndef...#endif  //ha//
  // and if the definition src line should be shown in listing                  //ha//
  // Force FALSE to show src line.                                              //ha//
  if (_p12Csw == _PASS2 && (_rsltLabfn & SYMIFDEF_FLAG) == SYMIFDEF_FLAG)       //ha//
    _rsltLabfn = FALSE;                                                         //ha//

  // Suppress src if symbol not found (CFALSE), process src if found (TRUE)
  if (_rsltLabfn != FALSE) _rsltLabfn = CTRUE_IFDEF;  // Process src line  if _rsltLabfn != 0
  else _rsltLabfn = CFALSE_IFDEF;                     // Suppress src line if _rsltLabfn == 0

  p12CSkipFlag = DetermineConditionIF((UINT)_rsltLabfn, CMOD_IFDEF);

  p12Cswifdef  = 1; // #ifdef / '#if defined' is active
  p12Cswelif   = 0;
  p12Cswelse   = 0;

  // Pass1: p1Flagifndef handling, is needed in Pass1                           //ha//
  if (_p12Csw == _PASS1) p1Flagifndef = CMOD_IFDEF; // CMOD_IFDEF;              //ha//
  // Pass2: Clear SYMIFDEF_FLAG, was used in Pass1                              //ha//
  else if (_p12Csw == _PASS2 && preprocessStack_ptr->Cifdef[1] == _rsltLabfn)   //ha//
    symboltab_ptr->symType &= ~SYMIFDEF_FLAG;                                   //ha//
    
  } // p12Cifdef

//-----------------------------------------------------------------------------
//
//                               p12Cifndef()
//
// Observing: PASS1 vs PASS2..
// #ifdef <symbol>
//
// The opposite of #ifdef:
// All the following lines until the corresponding #endif/#else/#elif
// are conditionally assembled if symbol is not #defined. 
// Shorthand for #if !defined (symbol).
//
// Note: ;; Using a forward reference in an assembler conditional
//       ;; may cause surprises, and in some cases is not allowed.
//
void p12Cifndef(int _p12Csw)
  {
  UCHAR _rsltLabfn;

  // Get symType, get 'symboltab_ptr' and save symbol in Pass1 for later
  _rsltLabfn = labfn(oper1);

  // check if symbol is undef'd
  if (p12Cundef(_PASS12) == TRUE) _rsltLabfn = FALSE;

  // needed in Pass1
  if (_p12Csw == _PASS1 && oper1[0] != 0) StrCpy(p1Symifndef, oper1); //ha//

#ifdef _DEBUG //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
if (_debugSwPass == _PASS2)                                           //--DEBUG--
{                                                                     //--DEBUG--
DebugPrintIfelseArray(8);
printf("..Stack=%08X [size=%d]  ..Stack_ptr=%08X  =..Stack[%d][]\n"
       "  CifdefCnt=%d    CifndefCnt=%d    CifCnt=%d  CelifCnt=%d\n"
       "p12Cswifdef=%d  p12Cswifndef=%d  p12Cswif=%d\n"
       "_rsltLabfn  =%02X  p1Symifndef=%s\n"
       "p12CSkipFlag=%02X  pcc=%08X [%08X]",
        preprocessStack, sizeof(CIFDEF), preprocessStack_ptr, (preprocessStack_ptr-preprocessStack),
        CifdefCnt,  CifndefCnt, CifCnt,  preprocessStack_ptr->CelifCnt,
        p12Cswifdef,  p12Cswifndef, p12Cswif,
        _rsltLabfn, p1Symifndef,
        (UCHAR)p12CSkipFlag, pcc, pccw);
DebugStop(20, "p12Cifndef()", __FILE__);                              //--DEBUG--
}                                                                     //--DEBUG--
#endif //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

  CifndefCnt++;
  
  // Symbol, if found, must be of type absolute (_ABSEQU or _ABSSET)
  if (CifndefCnt > IFELSEMAX && _rsltLabfn != FALSE &&
      (!(_rsltLabfn & _ABSEQU) || !(_rsltLabfn & _ABSSET)))
    {
    if (_p12Csw == _PASS1) return;
    else
      {
      errorp2(ERR_TOOLONG, symboltab_ptr->symString); // > IFELSEMAX
      return; 
      }
    } // end if (CifndefCnt)

  // Process follower conditions (needed from Pass1)
  // Check 'symType' if Symbol is defined during Pass1 within #ifndef...#endif
  // and if the definition src line should be shown in listing
  // Force FALSE to show src line.
  if (_p12Csw == _PASS2 && (_rsltLabfn & SYMIFNDEF_FLAG) == SYMIFNDEF_FLAG)
    _rsltLabfn = FALSE;

  if (_rsltLabfn != FALSE) _rsltLabfn = CFALSE_IFNDEF; // Suppress src line symbol not found
  else _rsltLabfn = CTRUE_IFNDEF;                      // Process src line if symbol found

  p12CSkipFlag = DetermineConditionIF((UINT)_rsltLabfn, CMOD_IFNDEF);

  p12Cswifndef = 1;   // #ifndef  / '#if !defined' is active
  p12Cswelif   = 0;
  p12Cswelse   = 0;

  // Pass1: p1Flagifndef handling, is needed in Pass1
  if (_p12Csw == _PASS1) p1Flagifndef = CMOD_IFNDEF;
  // Pass2: Clear SYMIFNDEF_FLAG, was used in Pass1
  else if (_p12Csw == _PASS2 && preprocessStack_ptr->Cifndef[1] == _rsltLabfn)
    symboltab_ptr->symType &= ~SYMIFNDEF_FLAG;        
    
  } // p12Cifndef

//-----------------------------------------------------------------------------
//
//                               p12Cif()
// #if <condition>
// #if defined <symbol>
// #if !defined <symbol>
//
// All the following lines until the corresponding #endif/#else/#elif
// are conditionally assembled if condition is TRUE (=not equal to 0). 
// Condition may be any integer expression, including preprocessor macros, which 
// are expanded. The preprocessor recognizes the operator #defined (symbol) that
// returns 1 if the symbol is defined and 0 otherwise. 
// Any undefined symbols used in condition are silently evaluated to FALSE (=0).
// Conditionals may be nested to arbitrary depth.
//
void p12Cif(int _p12Csw)
  {
  UINT _value;

  if (oper1[0] == 0)         // Check operand for #ifdef #ifndef #undef
    {
    errorp2(ERR_SYNTAX, "Missing expression");
    return; 
    }

  // Skip macro params
  if (strstr(oper1, "@") != NULL) _value = 0;
  // Evaluate .IF <expr>
  else _value = expr(oper1);

#ifdef _DEBUG //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
if (_debugSwPass == _PASS2)                                           //--DEBUG--
{                                                                     //--DEBUG--
DebugPrintIfelseArray(8);
printf("..Stack=%08X [size=%d]  ..Stack_ptr=%08X  =..Stack[%d][]\n"
       "  CifdefCnt=%d    CifndefCnt=%d    CifCnt=%d  CelifCnt=%d\n"
       "p12Cswifdef=%d  p12Cswifndef=%d  p12Cswif=%d\n"
       "p12CSkipFlag=%02X  pcc=%08X [%08X]  _value=%04X",
        preprocessStack, sizeof(CIFDEF), preprocessStack_ptr, (preprocessStack_ptr-preprocessStack),
          CifdefCnt,      CifndefCnt,      CifCnt,    preprocessStack_ptr->CelifCnt,
        p12Cswifdef,    p12Cswifndef,    p12Cswif,
        (UCHAR)p12CSkipFlag, pcc, pccw,     _value);
DebugStop(30, "p12Cif()", __FILE__);                                  //--DEBUG--
}                                                                     //--DEBUG--
#endif //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
  
  CifCnt++;

  // Operand must be present
  if (CifCnt > IFELSEMAX || oper1[0] == 0) 
    {
    if (_p12Csw == _PASS1) return;
    else
      {
      errorp2(ERR_TOOLONG, symboltab_ptr->symString);
      return; 
      }
    } // end if (CifCnt)

  // Suppress src if expr = 0 (CFALSE), process src if expr > 1 (TRUE)
  if (_value > 0) _value = CTRUE_IF;   // Process src line  if expr >= 1
  else _value = CFALSE_IF;             // Suppress src line if expr == 0

  p12CSkipFlag = DetermineConditionIF((UINT)_value, CMOD_IF);
                                                                                  
  p12Cswif     = 1;  // #if is active
  p12Cswelif   = 0;
  p12Cswelse   = 0;
  p1Flagifndef = 0;  // needed in PASS1
  } // p12Cif

//-----------------------------------------------------------------------------
//
//                               p12Celseif()
// #elif <condition>
// #elif defined <symbol>
// #elif !defined <symbol>
//
// #elif evaluates condition in the same manner as #if, except that
// it is only evaluated if no previous branch of a compound #if/#elif 
// sequence has been evaluated to TRUE.
//
// Note: ;; Using a forward reference in an assembler conditional
//       ;; may cause surprises, and in some cases is not allowed.
//
void p12Celseif(int _p12Csw)
  {
  UCHAR _rsltLabfn;
  UINT _value;
  int swelifMode = 0; // init = #elif <expr>

  if (oper1[0] == 0)         // Check operand for #ifdef #ifndef #undef
    {
    errorp2(ERR_SYNTAX, "Missing operand");
    return; 
    }

  // #elif <condition> / #elif <symbol> (allow wrong syntax, value=0 if undefined symbol)
  if (strcmp(fopcd, _elif_str) == 0            ||
      strcmp(fopcd, _elseif_str) == 0)           swelifMode = 0;  // _value
  // #elif defined <symbol>
  else if (strcmp(fopcd, _elif_def_str) == 0   ||
           strcmp(fopcd, _elseifdef_str) == 0)   swelifMode = 1;  // _rsltLabfn
  // #elif !defined <symbol>
  else if (strcmp(fopcd, _elif_ndef_str) == 0  ||
           strcmp(fopcd, _elseifndef_str) == 0)  swelifMode = 2;  // _rsltLabfn

  if ((CifdefCnt == 0 && CifndefCnt == 0 && CifCnt == 0)       || 
      (p12Cswifdef == 0 && p12Cswifndef == 0 && p12Cswif == 0) ||
      p12Cswelse != 0                                          || 
      oper1[0] == 0)               // Operand required
    {
    if (_p12Csw == _PASS1) return; // Pass1: No error handling
    else
      {
      errorp2(ERR_SYNTAX, fopcd);  // Pass2: Issue an error
      return;
      }
    } // end if

  switch(swelifMode)
    {
    case 0:                           
      // Skip macro params
      if (strstr(oper1, "@") != NULL) _value = 0;
      // Evaluate .ELIF <expr>
      else _value = expr(oper1);
      // allow <expr> to be an undefined symbol:
      //  #elif <symbol> - correct syntax would be: '#elif DEFINED(symbol)'
      if (erno == ERR_UNDFSYM) erno = 0;                  // value = 0
      if (_value > 0) _value = CTRUE;                     // Process src line  if expr >= 1
      else _value = CFALSE;                               // Suppress src line if expr == 0
      p12CSkipFlag = DetermineConditionELIF(_value);
      break;

    case 1:
      _rsltLabfn = labfn(oper1);                          // #elif defined <Symbol>
      // check if symbol is undef'd
      if (p12Cundef(_PASS12) == TRUE) _rsltLabfn = FALSE; // #undef
      if (_rsltLabfn != FALSE) _rsltLabfn = CTRUE;        // Process src line  if found
      else _rsltLabfn = CFALSE;                           // Suppress src line if not found
      p12CSkipFlag = DetermineConditionELIF(_rsltLabfn);
      break;

    case 2:
      _rsltLabfn = labfn(oper1);                          // #elif !defined <Symbol>
      // check if symbol is undef'd
      if (p12Cundef(_PASS12) == TRUE) _rsltLabfn = FALSE; // #undef
      if (_rsltLabfn != FALSE) _rsltLabfn = CFALSE;       // Suppress src line if found
      else _rsltLabfn = CTRUE;                            // Process src line  if not found
      p12CSkipFlag = DetermineConditionELIF(_rsltLabfn);
      break;

    default:
      p12CSkipFlag = ~CNONE; // normal (unconditional) processing 
      break;
    } // end switch

  p12Cswelif   = 1;   // #elseif is active
  p1Flagifndef = 0;   // needed in PASS1

#ifdef _DEBUG //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
if (_debugSwPass == _PASS2)                                           //--DEBUG--
{                                                                     //--DEBUG--
DebugPrintIfelseArray(8);
printf("..Stack=%08X [size=%d]  ..Stack_ptr=%08X  =..Stack[%d][]\n"
       "  CifdefCnt=%d    CifndefCnt=%d    CifCnt=%d  CelifCnt=%d\n"
       "p12Cswifdef=%d  p12Cswifndef=%d  p12Cswif=%d\n"
       "p12CSkipFlag=%02X  pcc=%08X [%08X]  _value=%02X",
        preprocessStack, sizeof(CIFDEF), preprocessStack_ptr, (preprocessStack_ptr-preprocessStack),
          CifdefCnt,      CifndefCnt,      CifCnt,   preprocessStack_ptr->CelifCnt,
        p12Cswifdef,    p12Cswifndef,    p12Cswif,
        (UCHAR)p12CSkipFlag, pcc, pccw, _value);
DebugStop(44, "p12Celseif()", __FILE__);                              //--DEBUG--
}                                                                     //--DEBUG--
#endif //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
  } // p12Celseif

//-----------------------------------------------------------------------------
//
//                               p12Celse()
// #else <>
//
// All the following lines until the corresponding #endif are conditionally
// assembled if no previous branch in a compound #if/#elif sequence
// has been evaluated to TRUE.
//
// Note: The "dangling else problem"
//  Basically you can't tell to which #if an #else belongs. 
//  To quote from the wikipedia article:
//  The convention when dealing with the dangling #else is to attach the #else
//  to the nearby #if statement, allowing for unambiguous context-free grammars,
//  in particular. Programming languages like Pascal, C follow this convention,
//  so there is no ambiguity in the semantics of the language.
//
void p12Celse(int _p12Csw)
  {
  if ((CifdefCnt == 0 && CifndefCnt == 0 && CifCnt == 0)       ||
      (p12Cswifdef == 0 && p12Cswifndef == 0 && p12Cswif == 0) ||
      p12Cswelse != 0                                          ||
      oper1[0] != 0)                // no operand expected
    {
    if (_p12Csw == _PASS1) return;  // Pass1: No error handling
    else
      {
      errorp2(ERR_SYNTAX, fopcd);
      return;                                         
      }
    } // end if (..)

  p12CSkipFlag = DetermineConditionELSE();

  p12Cswelif   = 0;
  p12Cswelse   = 1; // #else is active
  p1Flagifndef = 0; // needed in PASS1
 
#ifdef _DEBUG //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
if (_debugSwPass == _PASS2)                                           //--DEBUG--
{                                                                     //--DEBUG--
DebugPrintIfelseArray(8);
printf("..Stack=%08X [size=%d]  ..Stack_ptr=%08X  =..Stack[%d][]\n"
       "  CifdefCnt=%d    CifndefCnt=%d    CifCnt=%d  CelifCnt=%d\n"
       "p12Cswifdef=%d  p12Cswifndef=%d  p12Cswif=%d\n"
       "p12CSkipFlag=%02X  pcc=%08X [%08X]",
        preprocessStack, sizeof(CIFDEF), preprocessStack_ptr, (preprocessStack_ptr-preprocessStack),
          CifdefCnt,      CifndefCnt,      CifCnt,    preprocessStack_ptr->CelifCnt,
        p12Cswifdef,    p12Cswifndef,    p12Cswif,
        (UCHAR)p12CSkipFlag, pcc, pccw);
DebugStop(55, "p12Celse()", __FILE__);                                //--DEBUG--
}                                                                     //--DEBUG--
#endif //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
  } // p12Celse

//-----------------------------------------------------------------------------
//
//                               p12Cendif()
// #endif
//
// Terminates a conditional block started with an #if/#ifdef/#ifndef directive.
//
void p12Cendif(int _p12Csw)
  {
  char* errTxtEndif = "Misplaced or too many";   //  #endif/.ENDIF error text

  // If no more pending #ifdef/#ifndef: Clear structured ifdef buffer
  if (CifndefCnt == 0 && CifdefCnt == 0 && CifCnt == 0)
    {
    sprintf(pszErrtextbuf, "%s %s", errTxtEndif, fopcd);
    errorp2(ERR_SYNTAX, pszErrtextbuf);
    return;                // ERROR return
    } // end if

  // '#endif wrzlbrnft' - Error only if '.MODEL SYNTAX' (other assembler allow this)
  if (oper1[0] != 0 && (swmodel & _SYNTAX)) 
    {
    sprintf(pszErrtextbuf, "%s %s", fopcd, oper1);
    errorp2(ERR_SYNTAX, pszErrtextbuf);
    }

  // #endif without previous #if/#ifdef/#ifndef is an error
  if (srcEOF == FALSE  &&
      CifdefCnt  == 0  &&
      CifndefCnt == 0  &&
      CifCnt     == 0)
    {
    sprintf(pszErrtextbuf, "%s %s", errTxtEndif, fopcd);
    errorp2(ERR_SYNTAX, pszErrtextbuf);

    p12CSkipFlag = ~CNONE; // normal (unconditional) processing
    p1Flagifndef = 0;      // needed in PASS1
    p12Cswifdef  = 0;
    p12Cswifndef = 0;
    p12Cswif     = 0;
    p12Cswelif   = 0;
    p12Cswelse   = 0;
    return;                // ERROR return
    } // end if

#ifdef _DEBUG //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
if (_debugSwPass == _PASS2)                                           //--DEBUG--
{                                                                     //--DEBUG--
DebugPrintIfelseArray(8);
printf("..Stack=%08X [size=%d]  ..Stack_ptr=%08X  =..Stack[%d][]\n"
       "  CifdefCnt=%d [%02X]  CifndefCnt=%d [%02X]  CifCnt=%d [%02X]  CelifCnt=%d\n"
       "p12Cswifdef=%d       p12Cswifndef=%d       p12Cswif=%d\n"
       "p12CSkipFlag=%02X  pcc=%08X [%08X]",
        preprocessStack, sizeof(CIFDEF), preprocessStack_ptr, (preprocessStack_ptr-preprocessStack),
        CifdefCnt, preprocessStack_ptr->Cifdef[1],
        CifndefCnt, preprocessStack_ptr->Cifndef[1],
        CifCnt, preprocessStack_ptr->Cif[1], 
        preprocessStack_ptr->CelifCnt,
        p12Cswifdef,  p12Cswifndef, p12Cswif,
        (UCHAR)p12CSkipFlag, pcc, pccw);
DebugStop(60, "p12Cendif()", __FILE__);                               //--DEBUG--
}                                                                     //--DEBUG--
#endif //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

  // Clean ifelse stack, decrement stack pointer and related if counter 
  if (CifdefCnt > 0 &&
      p12Cswifdef   &&
      (preprocessStack_ptr->Cifdef[1] & CMOD_IFDEF))
    {
    CifdefCnt--;
    if (CifdefCnt == 0) p12Cswifdef = 0;
    preprocessStack_ptr->Cifdef[1]  = 0;
    preprocessStack_ptr->Celse[1]   = 0;
    for (_j=0; _j<IFELSEMAX; _j++) preprocessStack_ptr->Celif[_j] = 0;
    preprocessStack_ptr--;
    } // end if (CifdefCnt..)

  else if (CifndefCnt > 0 &&
           p12Cswifndef   &&
           (preprocessStack_ptr->Cifndef[1] & CMOD_IFNDEF))
    { 
    CifndefCnt--;
    if (CifndefCnt == 0) p12Cswifndef = 0;
    preprocessStack_ptr->Cifndef[1]   = 0;
    preprocessStack_ptr->Celse[1]     = 0;
    for (_j=0; _j<IFELSEMAX; _j++) preprocessStack_ptr->Celif[_j] = 0;
    preprocessStack_ptr--;
    } // end if (CifndefCnt..)

  else if (CifCnt > 0 &&
           p12Cswif   && 
           (preprocessStack_ptr->Cif[1] & CMOD_IF))
    { 
    CifCnt--;
    if (CifCnt == 0) p12Cswif     = 0;
    preprocessStack_ptr->Cif[1]   = 0;
    preprocessStack_ptr->Celse[1] = 0;
    for (_j=0; _j<IFELSEMAX; _j++) preprocessStack_ptr->Celif[_j] = 0;
    preprocessStack_ptr--;
    } // end if (CifCnt..)

  // If no more pending #ifdef/#ifndef: Clear structured ifdef buffer
  if (CifndefCnt == 0 && CifdefCnt == 0 && CifCnt == 0)
    {
    //  typedef struct tagCIFDEF {      // #ifelse cascade structure
    //    char      Cifdef[2];         
    //    char      Cifndef[2];        
    //    char      Cif[2];
    //    union {
    //      char    CelifCnt;        
    //      char    Celif[IFELSEMAX];
    //    };          
    //    char      Celse[2];          
    //  } CIFDEF, *LPCIFDEF;
    //                                  
    preprocessStack_ptr = preprocessStack; // reset stack

    // Init-clear global array of symbol structures
    // Set normal processing src line(s)      
    preprocessStack_ptr->Cifdef[1]  = 0; //_j | _i<<4;  // Check
    preprocessStack_ptr->Cifndef[1] = 0; //_j | _i<<4;
    preprocessStack_ptr->Cif[1]     = 0; //_j | _i<<4;
    preprocessStack_ptr->Celse[1]   = 0; //_j | _i<<4;
    for (_j=0; _j<IFELSEMAX; _j++) preprocessStack_ptr->Celif[_j] = 0;

    p12CSkipFlag = ~CNONE; // normal (unconditional) processing
    p1Flagifndef = 0;      // needed in PASS1
    CifdefCnt    = 0;
    CifndefCnt   = 0;
    CifCnt       = 0;
    p12Cswifdef  = 0;
    p12Cswifndef = 0;
    p12Cswif     = 0;
    } // end if

  // Determine and re-establish current 'p12CSkipFlag' status
  // p12CSkipFlag == FALSE:  Suppress src line                                                                  
  // p12CSkipFlag == TRUE:   Process  src line                 
  // p12CSkipFlag == ~CNONE: Normal (unconditional) processing                                                  
  else if (((preprocessStack_ptr-1)->Cifdef[1]        == CFALSE_IFDEF  ||
            (preprocessStack_ptr-1)->Cifndef[1]       == CFALSE_IFNDEF || //ha//
            (preprocessStack_ptr-1)->Cif[1]           == CFALSE_IF)    &&
            ((preprocessStack_ptr-1)->Celif[(preprocessStack_ptr-1)->CelifCnt] == CFALSE ||
             (preprocessStack_ptr-1)->Celse[1]        == CFALSE)          //ha//
          )
    {
    p12CSkipFlag = CFALSE;  // Suppress src line
    }

  else if (preprocessStack_ptr->Cifdef[1]           == CTRUE_IFDEF  ||
           preprocessStack_ptr->Cifndef[1]          == CTRUE_IFNDEF ||
           preprocessStack_ptr->Cif[1]              == CTRUE_IF     ||
           preprocessStack_ptr->Celif[preprocessStack_ptr->CelifCnt] == CTRUE         ||
           (preprocessStack_ptr-1)->Celif[(preprocessStack_ptr-1)->CelifCnt] == CTRUE ||
           (preprocessStack_ptr-1)->Celse[1]        == CTRUE
          )
    {
    p12CSkipFlag = CTRUE;   // Process src line
    }

  else if ((preprocessStack_ptr->Cifdef[1]       == CFALSE_IFDEF   ||
            preprocessStack_ptr->Cifndef[1]      == CFALSE_IFNDEF  ||
            preprocessStack_ptr->Cif[1]          == CFALSE_IF)     &&
           (preprocessStack_ptr->Celif[preprocessStack_ptr->CelifCnt] == CTRUE ||
            preprocessStack_ptr->Celse[1]        == CTRUE)
          )
    {
    p12CSkipFlag = CTRUE;   // Process src line
    }

  // Suppress src line if current #elif is false  
  if (preprocessStack_ptr->Celif[preprocessStack_ptr->CelifCnt] == CFALSE)
    p12CSkipFlag = CFALSE;                                                    //ha//

  //////////////////////////////////////////////////////////////////////////  //ha//
  // Needed in PASS1:                                                     //  //ha//
  // if symbol was defined within #ifndef...#endif                        //  //ha//
  //  do not treat it as an undefined forward reference                   //  //ha//
  if (p1Flagifndef != 0 && labfn(p1Symifndef) != FALSE) p1Flagifndef = 0; //  //ha//
  //////////////////////////////////////////////////////////////////////////  //ha//

  p12Cswelif   = 0;
  p12Cswelse   = 0;

#ifdef _DEBUG //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
if (_debugSwPass == _PASS2)                                           //--DEBUG--
{                                                                     //--DEBUG--
DebugPrintIfelseArray(8);
printf("..Stack=%08X [size=%d]  ..Stack_ptr=%08X  =..Stack[%d][]\n"
       "  CifdefCnt=%d [%02X]  CifndefCnt=%d [%02X]  CifCnt=%d [%02X]  CelifCnt=%d\n"
       "p12Cswifdef=%d       p12Cswifndef=%d       p12Cswif=%d\n"
       "p12CSkipFlag=%02X  pcc=%08X [%08X]",
        preprocessStack, sizeof(CIFDEF), preprocessStack_ptr, (preprocessStack_ptr-preprocessStack),
        CifdefCnt, preprocessStack_ptr->Cifdef[1],
        CifndefCnt, preprocessStack_ptr->Cifndef[1],
        CifCnt, preprocessStack_ptr->Cif[1], 
        preprocessStack_ptr->CelifCnt,
        p12Cswifdef,  p12Cswifndef, p12Cswif,
        (UCHAR)p12CSkipFlag, pcc, pccw);
DebugStop(66, "p12Cendif()", __FILE__);                               //--DEBUG--
}                                                                     //--DEBUG--
#endif //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

  } // p12Cendif

//------------------------------------------------------------------------------
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//if (_debugSwPass == _PASS1)                                    //--DEBUG--
//ha//{                                                              //--DEBUG--
//ha//printf("srcEOF=%d  CifndefCnt=%d  CifdefCnt=%d  CifCnt=%d  CelifCnt=%d\n",
//ha//        srcEOF, CifndefCnt,  CifdefCnt, CifCnt,  preprocessStack_ptr->CelifCnt);
//ha//DebugStop(0, "p12CCheckIfend()", __FILE__);                    //--DEBUG--
//ha//}                                                              //--DEBUG--
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha////if (_debugSwPass == _PASS2)                                  //--DEBUG--
//ha//{                                                              //--DEBUG--
//ha//printf("fopcd=%s  oper1=%s  symUndefBuf_ptr=%s\n"
//ha//       "_p12Csw=%d  _rUNDEF=%d  _k=%d\n", 
//ha//        fopcd,    oper1,    symUndefBuf_ptr,
//ha//        _p12Csw,    _rUNDEF,    _k);
//ha//printf("symUndefBuf ");
//ha//DebugPrintBuffer(symUndefBuf_ptr, SYMUNDEFBUFSIZE/10);
//ha//DebugStop(3, "p12Cundef()", __FILE__);                         //--DEBUG--
//ha//}                                                              //--DEBUG--
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha//#ifdef _DEBUG //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//if (_debugSwPass == _PASS2)                                    //--DEBUG--
//ha//{                                                              //--DEBUG--
//ha//DebugPrintIfelseArray(8);
//ha//printf("..Stack=%08X [size=%d]  ..Stack_ptr=%08X  =..Stack[%d][]\n"
//ha//       "  CifndefCnt=%d [%02X]  CifdefCnt=%d [%02X]  CifCnt=%d [%02X]  CelifCnt=%d\n"
//ha//       "p12Cswifndef=%d  p12Cswifdef=%d  p12Cswif=%d\n"
//ha//       "p12CSkipFlag=%02X  pcc=%08X [%08X]",
//ha//        preprocessStack, sizeof(CIFDEF), preprocessStack_ptr, (preprocessStack_ptr-preprocessStack),
//ha//        CifndefCnt, preprocessStack_ptr->Cifndef[1],
//ha//        CifdefCnt, preprocessStack_ptr->Cifdef[1],
//ha//        CifCnt, preprocessStack_ptr->Cif[1], 
//ha//        preprocessStack_ptr->CelifCnt,
//ha//        p12Cswifndef,  p12Cswifdef, p12Cswif,
//ha//        p12CSkipFlag, pcc, pccw);
//ha//DebugStop(66, "p12Cendif()", __FILE__);                        //--DEBUG--
//ha//}                                                              //--DEBUG--
//ha//#endif //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
