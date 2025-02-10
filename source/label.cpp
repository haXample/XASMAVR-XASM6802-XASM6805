// haXASM - Cross-Assembler for 8bit Microprocessors
// label.cpp - C++ Developer source file.
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

// Extern variables and functions
extern void DebugStop(int, char*, char*); // Usage: DebugStop(testNr, "Function()", __FILE__);
extern void DebugPrintBuffer(char*, int); // Usage: DebugPrintBuffer(buffer, count);
extern void DebugPrintSymbolArray(int);   // Usage: DebugPrintSymbolArray(count);

extern char* errtx_sys;
extern char* errtx_alloc;

extern char* EQU_str;
extern char* DEF_str;
extern char* SET_str;
extern int predefCount;                   // If any pre-define symbols

extern void AssignPc();
extern void GetPcValue();
char* skip_trailing_spaces(char*);

// Forward declaration of functions included in this code module:
BOOL label_syntax(char *);
UCHAR labfn(char*);
BOOL errorp1();
void errorp2(int, char*);

//------------------------------------------------------------------------------
//
//                           errorp2
//
void errorp2(int _errNr, char* _errtext)
  {
  erno = _errNr;            // Error code
  errSymbol_ptr = _errtext; // may be NULL
  } // errorp2

//------------------------------------------------------------------------------
//
//                           errorp1
//
// PASS2: Check if current source line is entered in Pass1 error table.
// If so, put error number from table in 'erno' to be printed later in Pass2.
// Additionally point to a descriptive error text, if present.
// Existing undefined symbol names are also printed with the error line.
//
// typedef struct tagP1ERRTBL {  // Error line structure
//   int lineNr;                 // Source line number
//   UCHAR errCode;              // Error code
//   char  errText[OPERLEN+1];   // Error symbol string
// } P1ERRTBL, *LPP1ERRTBL;      // Structured Pass1 error buffer
//
BOOL errorp1()
  {
  BOOL bResult = FALSE;                           // assume no Pass1 error

  if (swlst == 0 && swicl != 0) return(bResult);  //ha// Only main source file if .NOLIST
                                                  //ha// to do: need curfilename  in tagP1ERRTBL..
  p1errTbl_ptr = p1errTbl;                        //ha// (or swsyntax) possible mismatching lineNr in wrong file

  for (_i=0; _i<P1ERRMAX; _i++)
    {
    if (p1errTbl_ptr->lineNr == 0) break;         // no more entries - skip
    else if (p1errTbl_ptr->lineNr == _curline && p1errTbl_ptr->errCode > 0)
      {
      erno = p1errTbl_ptr->errCode;               // error code from Pass1
      if (p1errTbl_ptr->errText[0] != 0)
        {        
        errSymbol_ptr = p1errTbl_ptr->errText;    // set pointer to error symbol (may be NULL)
        bResult = TRUE;                           // Found a Pass1 error
        break;
        }
      } // end else if
    p1errTbl_ptr++;                               // advance pointer
    } // end for(_i)
  return(bResult);
  } // errorp1

//------------------------------------------------------------------------------
//
//                           p1errorl
//
// PASS1: Error lines from pass1 to be reported on the pass2 listing
// are put into a structred table. There are four entries:
//  the line number, an error code, a pointer to an unknown, undefined
//  or forward-referenced symbol in pass1 (may be NULL) and a 0-terminator.
// The table can contain P1ERCMAX entries. If more, the rest is skipped.
//
// typedef struct tagP1ERRTBL {  // Error line structure
//   int lineNr;                 // Source line number
//   UCHAR errCode;              // Error code
//   char  errsymbol[SYMLEN+1];  // Error symbol string
// } P1ERRTBL, *LPP1ERRTBL;      // Structured Pass1 error buffer
//
void p1errorl(UCHAR _errNr, char* _symbol)
  {
  int _i;

  p1errTbl_ptr = p1errTbl;

  // Pass1 error table full? Skip the rest.
  if (p1erc >= P1ERRMAX) return;
    
  (p1errTbl_ptr+p1erc)->lineNr  = _curline;  // Store line number
  (p1errTbl_ptr+p1erc)->errCode = _errNr;    // Store errNr

  if (_symbol != NULL)  // Pointer to descriptive errSymbol text
    {
    for (_i=0; _i<strlen(_symbol); _i++)
      {
      if (_i > OPERLEN) break;
      (p1errTbl_ptr+p1erc)->errText[_i] = _symbol[_i];
      }
    } // end if
  p1erc++;                       // Error counter
  } // p1errorl

//------------------------------------------------------------------------------
//
//                   symbolSort (using array of structures)
//
// The structures in symboltab are sorted in alphabetical order
//
// Example:
// USER SYMBOLS
// done_firstLEDChoiceCorrect1 . . . . . .  00000478 C  done_firstLEDChoiceP2 . . . . . . . . .  00000084 C  
//
// Find lowest symbol in table (alphabetic order).
// Place symbol in 'flabel', value in 'value' and type in 'SymType'.
// Replace with 0FFh in the table
//
//  typedef struct tagSYMBOLBUF {  // Symbol structure
//    char  symString[SYMLEN + 1]; // Symbol/Label name string
//    ULONG symAddress;            // Symbol value or pointer to operand string
//    UCHAR symType;               // Symbol type (ABS, C-SEG, UNDEFINED)
//  } SYMBOLBUF, *LP_SYMBOLBUF;
//
//
#define SORT_MARKER '\xFF'
BOOL symbolSort()
  {
  // Allocate a temporary buffer to intercept the sorted symbols
  LPSYMBOLBUF sortSymboltab = (LPSYMBOLBUF)LocalAlloc(GPTR, (SYMENTRIES+1) * sizeof(SYMBOLBUF));
  if (sortSymboltab == NULL)
    {
    // >>>>>>>>> ERROR: MEMORY ALLOC <<<<<<<<<<
    printf("\n%s%s", errtx_sys, errtx_alloc);   
    exit(SYSERR_MALLOC);
    }

  LPSYMBOLBUF sortSymboltab_ptr = sortSymboltab;
  LPSYMBOLBUF pickadr_ptr;
  int symbolCount=0;

  do {
    // Skip oper_predev area from symboltable (should be kept invisible)
    symboltab_ptr  = symboltab;       
    symboltab_ptr += predefCount;  // Skip pre-defined symbols in symboltab 

///--SORT_LOOP--/////////////////////////////////////////////////////////////////////
SORT_LOOP:                                                                         //
    // Pick a symString[] and save it in flabl[]                                   //
    StrCpy(flabl, symboltab_ptr->symString);      // flabl[] = symString[]         //
    value = (UINT)symboltab_ptr->symAddress;      // value = symAddress            //
    SymType = symboltab_ptr->symType;             // SymType                       //
    pickadr_ptr = symboltab_ptr;                                                   //
                                                                                   //
    while (symboltab_ptr < &symboltab[SYMENTRIES-1])  // < 'symboltab_top'         //
      {                                                                            //
      symboltab_ptr++;     // Take next label/symbol & try that one                //
                                                                                   //
      // Compare symbol strings (example)                                          //
      //  Equal:  _rslt = 0 e.g. flabl= REV  symboltab_ptr= REV       [ 0]  0      //
      //  Lower:  _rslt > 0 e.g. flabl= REV  symboltab_ptr= RAMSIZE   [ 4] +1..+26 //
      //  Higher: _rslt < 0 e.g. flabl= REV  symboltab_ptr= STACKSIZE [-1] -1..-26 //
      //                                                                           //
      //  >0: flabl > .symString - try next and goto SORT_LOOP                     //
      //  <0: .symString > flabl - xfer into sortSymboltab                         //          
      if (stricmp(flabl, symboltab_ptr->symString) > 0)       // !BUG in StrCmpI!  //
        {                                                                          //
        if (symboltab_ptr->symString[0] != 0) goto SORT_LOOP; // Try next          //
        else break;                                           // Rest is empty     //
        }                                                                          //
      } // end while                                                               //
///--SORT_LOOP--/////////////////////////////////////////////////////////////////////

    // Transfer flabl --> sortSymboltab_ptr->symString  if not yet done yet 
    if (flabl[0] != SORT_MARKER)
      {
      StrCpy(sortSymboltab_ptr->symString, flabl);                             
      sortSymboltab_ptr->symAddress = (UINT)value;                             
      sortSymboltab_ptr->symType = SymType;
      symbolCount++;                    // Count the structures transferred       
      sortSymboltab_ptr++;              // Advance to next free array space

      pickadr_ptr->symString[0] = SORT_MARKER; // Mark string 'handled'
      }
    } while (flabl[0] != SORT_MARKER);  // end do-while()

  // ---------------------------------------------------------------
  // Transfer sorted symbol array into global symboltab array buffer
  symboltab_ptr  = symboltab;           // start of symbol array buffer
  symboltab_ptr += predefCount;         // Skip pre-defined symbols in symboltab 
  sortSymboltab_ptr = sortSymboltab;    // start of sorted symbol array buffer
  for (_i=0; _i<=symbolCount; _i++)     // Transfer the sorted structures
    {
    // Copy sorted elements back to symbol structure
    StrCpy(symboltab_ptr->symString, sortSymboltab_ptr->symString);
    symboltab_ptr->symAddress = sortSymboltab_ptr->symAddress;
    symboltab_ptr->symType = sortSymboltab_ptr->symType;

    // Advance pointers and point to next structure
    symboltab_ptr++;
    sortSymboltab_ptr++;                           
    } // end for (symbolCount..)

  LocalFree(sortSymboltab);  
  return(TRUE);
  } // symbolSort


//-----------------------------------------------------------------------------
//
//                      p1lab
//
// Pass 1 label routine
//
// Return if label field is empty or label contains bad characters.
// Enter the symbol in the label table. If already defined,
// mark as doubly defined.
//
//  typedef struct tagSPLITFIELD {
//    char flabl[SYMLEN  + 1];      // Label field
//    char fopcd[OPCDLEN + 1];      // Opcode field
//    char oper1[OPERLEN + 1];      // Operand1 field
//    char oper2[OPERLEN + 1];      // Operand2 field
//    char oper3[OPERLEN + 1];      // Operand3 field
//    } SPLITFIELD, *LPSPLITFIELD;
//
//  typedef struct tagSYMBOLBUF {   // Symbol structure
//    char  symString[SYMLEN+1];    // Symbol/Label name  string
//    ULONG symAddress;             // Symbol value or pointer to operand string
//    UCHAR symType;                // Symbol type (ABS, C-SEG, UNDEFINED)
//  } SYMBOLBUF, *LPSYMBOLBUF;      // Structured symbol buffer
//
BOOL p1lab()
  {
  BOOL bResult = FALSE;
  int _rsltLabfn = FALSE;           // Assume label not valid
  int _value;

  GetPcValue();

  // Check if label present or symbol table is full
  if (flabl[0] == 0 || swlab == ERR_SYMFULL) return(FALSE);                    

  // Check if free space available
  if (symboltab_ptr >= &symboltab[SYMENTRIES-1])  // >= 'symboltab_top' 
    {
    swlab = ERR_SYMFULL;            // Symbol table is full
    p1errorl(ERR_SYMFULL, NULL);
    return(FALSE);                    
    }

  // Label is present: Check if label syntax is okay
  if (!label_syntax(flabl))
    {
    p1errorl(ERR_SYNTAX, NULL);
    return(FALSE);
    }

  // ---------------------------------------------------------
  // Get symboltab_ptr to label or free to next space in table
  // >0: Label found in table, FALSE: Label not found
  // _rstlabfn == FALSE or SegType > 0 (dont expect TRUE!!) 
  _rsltLabfn = labfn(flabl);        

  // Macro labels (demand special treatment)
  if (StrCmpI(flabl,  symboltab_ptr->symString) == 0 &&
       pcValue != symboltab_ptr->symAddress          &&
       symboltab_ptr->symType != SegType)
    {
    symboltab_ptr->symAddress = pcValue;
    symboltab_ptr->symType = SegType;
    } // if (macro labels)

  if (StrStrI(fopcd, SET_str) != 0)
    {
    for (_i=0; _i<=strlen(flabl); _i++) symboltab_ptr->symString[_i] = flabl[_i];
    symboltab_ptr->symAddress = pcValue;  // Enter current pcc/pcd
    symboltab_ptr->symType = _ABSSET;     // Define label type
    bResult = TRUE;                                               
    }

  // Check if 'label' already defined or already doubly defined             
  else if ((_rsltLabfn != FALSE && symboltab_ptr->symType != 0)  &&
           (StrStrI(fopcd, EQU_str) == 0 || StrStrI(fopcd, DEF_str) == 0)) 
    {
    // Mark as doubly defined
    symboltab_ptr->symType = (UCHAR)(SYMERR_FLAG | ERR_DBLSYM);  
    p1errorl(ERR_DBLSYM, flabl);         // use ERR_SYMFULL, distinguish test only
    bResult = TRUE;                      // Label found, but already defined                         
    }

  // Store 'label' into symboltab
  else if (_rsltLabfn == FALSE &&
           (StrStrI(fopcd, EQU_str) == 0 || StrStrI(fopcd, DEF_str) == 0)) 
    {
    for (_i=0; _i<=strlen(flabl); _i++) symboltab_ptr->symString[_i] = flabl[_i];
    symboltab_ptr->symAddress = pcValue;  // Enter current pcc/pcd
    symboltab_ptr->symType = SegType;     // Define label type
    bResult = TRUE;                                               
    }

  AssignPc();
  return(bResult);
  } // p1lab
                                                                   
//------------------------------------------------------------------------------
//
//                         p2lab
//
// Label check in pass 2. Invalid syntax: ERROR(1)
//        Bad value: ERROR(8)
//        Doubly defined: ERROR(12)
//
//  typedef struct tagSPLITFIELD {
//    char flabl[SYMLEN  + 1];      // Label field
//    char fopcd[OPCDLEN + 1];      // Opcode field
//    char oper1[OPERLEN + 1];      // Operand1 field
//    char oper2[OPERLEN + 1];      // Operand2 field
//    char oper3[OPERLEN + 1];      // Operand3 field
//    } SPLITFIELD, *LPSPLITFIELD;
//
void p2lab()
  {
  GetPcValue();

  if (flabl[0] == 0 || erno != 0) return;  // already done if errors

  if (!label_syntax(flabl))                // Check label syntax
    {                   
    errorp2(ERR_SYNTAX, flabl);
    return;
    }

  if (labfn(flabl) == FALSE)               // (not found, shouldn`t occur)
    {
    errorp2(ERR_UNDFSYM, flabl);
    return;
    }

  // Check all labels '_xyz:' (not the EQU / DEF symbols)
  if (StrStrI(fopcd, EQU_str) == 0 || StrStrI(fopcd, DEF_str) == 0) 
    {
    // Issue an ERROR if the label is doubly defined
    if (symboltab_ptr->symType == (UCHAR)(SYMERR_FLAG | ERR_DBLSYM))
      errorp2(ERR_DBLSYM, symboltab_ptr->symString);  // Symbol pointer

    // PHASE ERROR if symAddress != pcValue and symbol type = CSEG or DSEG
    // A "PHASE ERROR" occurs whenever Pass1 and Pass2 are not synchronized
    if (symboltab_ptr->symAddress != pcValue &&
        (symboltab_ptr->symType == _CODE || symboltab_ptr->symType == _DATA) &&
        swdefmacro == FALSE)  // Exclude the macro definition phase
      {
      errorp2(ERR_P1PHASE, NULL);                   
      // -----------------------------------------------------------------//
      // Try to correct phase.                                            //
      // This is done to suppress annoying multiple phase error lines     //
      //  occurring in .LST at every label following the 1st phase error. //      
      pcValue = symboltab_ptr->symAddress;                                //
      // -----------------------------------------------------------------//
      }
    } // end if (EQU_str || DEF_str)

  AssignPc();
  } // p2lab

//------------------------------------------------------------------------------
//
//                            labfn
//
// Find symbol/label in symbol table. 
// 'symbl_ptr' pointer to symbol name.
// return: FALSE   = symbol/label not found
//         SegType = symbol/label found
//
// typedef struct tagSYMBOLBUF {  // Symbol structure
//   char  symString[SYMLEN+1];   // Symbol/Label name  string
//   ULONG symAddress;            // Symbol value or pointer to operand string
//   UCHAR symType;               // Symbol type (ABS, C-SEG, UNDEFINED)
//} SYMBOLBUF, *LPSYMBOLBUF;      // Structured symbol buffer
//
UCHAR labfn(char* symbl_ptr)
  {
  UCHAR bresult = FALSE;

  // Eliminate any possible trailing spaces & cntrls
  skip_trailing_spaces(symbl_ptr);
                                       // Don't use 'symboltab_top' C++ compiler ERR
  symboltab_ptr = symboltab;           // && Start of symbol table < 'symboltab_top'
  while (symboltab_ptr->symString[0] != 0 && symboltab_ptr<&symboltab[SYMENTRIES-1]) 
    {
    // Compare symbol strings 
    //  Equal:  _rslt = 0 e.g. flabl= REV  symboltab_ptr= REV       [ 0]  0
    //  Lower:  _rslt > 0 e.g. flabl= REV  symboltab_ptr= RAMSIZE   [ 4] +1..+26
    //  Higher: _rslt < 0 e.g. flabl= REV  symboltab_ptr= STACKSIZE [-1] -1..-26
    if (stricmp(symboltab_ptr->symString, symbl_ptr) == 0)
      {
      bresult = symboltab_ptr->symType; // return ->symType != FALSE
      break;                            // label found
      }
    else symboltab_ptr++;               // skip to next structure
    } // end while

  return(bresult);  
  } // labfn
                                                          
//------------------------------------------------------------------------------
//
//                          label_syntax
//
// Symbol syntax check: Label or symbol must start w/ letter or underscore
//
// "StrCSpnIA(string1, string2)":   string2=Group of characters searched
//  Search a string for the first occurrence of any of a group of characters.
//  The search method is not case-sensitive, and the terminating NULL character
//  is included within the search pattern match. Return: Position of matching
//  character in string1 (1st char RETURN=0). If no match found it returns
//  the postion of the terminating 0 of the string1 (RETURN=strlen(string1)).
//
BOOL label_syntax(char* _flabl_ptr)
  {
  BOOL b_syntax_stat = FALSE;  // Assume label wrong syntax

  char* LabelChars = "_@0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"; // Allowed chars

  if (_flabl_ptr[0] > '9')     // Digits '0'..'9' not allowed as 1st label char
    {                         
    b_syntax_stat = TRUE;      // Assume valid label
    _i = 0;
    while (_i <= SYMLEN && _flabl_ptr[_i] != 0)        // End of label string
      {
      if (StrCSpnIA(&_flabl_ptr[_i], LabelChars) != 0) // 'Pole position0' must match
        { 
        b_syntax_stat = FALSE; // Syntax error: Illegal char in label name
        break;
        }
      _i++;                    // shift string ptr index
      } // end while
    } // end if (>'9')

  return (b_syntax_stat); 
  } // label_syntax

//-----------------------------------------------------------------------------
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha////if (_debugSwPass == 2)
//ha//{
//ha//printf("p1erc=%d  _errNr=%d  _symbol=%s  _lstline=%d  sizeof(P1ERRTBL)=%d\n",
//ha//        p1erc,    _errNr,    _symbol,    _lstline,    sizeof(P1ERRTBL));
//ha//printf("p1errTbl_ptr->lineNr    =%d\n", (p1errTbl_ptr+p1erc)->lineNr   );   
//ha//printf("p1errTbl_ptr->errCode   =%d\n", (p1errTbl_ptr+p1erc)->errCode  );   
//ha//printf("p1errTbl_ptr->errsymbol =%s\n", (p1errTbl_ptr+p1erc)->errsymbol);
//ha//printf("strlen(_symbol)=%d\n", strlen(_symbol));   
//ha//printf("p1errTbl ");    
//ha//DebugPrintBuffer((char*)&p1errTbl[0], 3*sizeof(P1ERRTBL));
//ha//DebugStop(1, "p1errorl()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//if (pcc >0)
//ha//{
//ha//printf("pcc=%08X  pcValue=%08X  flabl=%s  fopcd=%s  oper1=%s\n"
//ha//       "symboltab_ptr->symString=%s  symboltab_ptr->symAddress=%08X",
//ha//        pcc,      pcValue,      flabl,    fopcd,    oper1,
//ha//        symboltab_ptr->symString,    symboltab_ptr->symAddress);  
//ha////printf("macrotab[macroCount].macBufPtr ");
//ha////DebugPrintBuffer(macrotab[macroCount].macBufPtr, strlen(macrotab[macroCount].macBufPtr));
//ha//DebugStop(1, "p2lab()", __FILE__);
//ha//}//--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("flabl=%s  fopcd=%s\n"
//ha//        "_rsltLabfn=%02X  symboltab_ptr->symType=%02X\n", 
//ha//         flabl,   fopcd,
//ha//         _rsltLabfn,      symboltab_ptr->symType);
//ha//DebugStop(1, "p1lab()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("symboltab_ptr ");
//ha//DebugPrintBuffer(symboltab_ptr->symString, 450);
//ha////DebugPrintSymbolArray(100);  
//ha//printf("symbolCount=%d  flabl=[%s][%02X]  symboltab_ptr->symString=[%s]", 
//ha//        symbolCount,    flabl,(UCHAR)flabl[0], symboltab_ptr->symString);
//ha//DebugStop(1, "symbolSort()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("_flabl_ptr=%s  _i=%d  strlen(_flabl_ptr)=%d  b_syntax_stat=%d\n", 
//ha//        _flabl_ptr,    _i,    strlen(_flabl_ptr),    b_syntax_stat);
//ha//DebugStop(2, "label_syntax()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//if (pcc >0)
//ha//{
//ha//printf("pcc=%08X  pcValue=%08X  Flabl=%s  fopcd=%s  oper1=%s\n"
//ha//       "swdefmacro=%d  \nsymboltab_ptr->symString=%s  symboltab_ptr->symAddress=%08X",
//ha//        pcc,      pcValue,       flabl,    fopcd,    oper1,
//ha//        swdefmacro, symboltab_ptr->symString, symboltab_ptr->symAddress);  
//ha////printf("macrotab[macroCount].macBufPtr ");
//ha////DebugPrintBuffer(macrotab[macroCount].macBufPtr, strlen(macrotab[macroCount].macBufPtr));
//ha//DebugStop(2, "labfn()", __FILE__);
//ha//}//--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//DebugPrintSymbolArray(50);  
//ha//DebugStop(2, "labfn()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//if (_debugSwPass == 2)
//ha//{
//ha//printf("_rslt=%d  flabl=%s\nsortSymboltab_ptr->symString=%s\nsymboltab_ptr->symString=%s\n",
//ha//        _rslt,    flabl,   sortSymboltab_ptr->symString, symboltab_ptr->symString);
//ha//DebugPrintBuffer((char*)symboltab_ptr, 100);
//ha////DebugPrintSymbolArray(3);  
//ha//DebugStop(1, "labfn()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//--------------------------end-of-c++-module-----------------------------------

