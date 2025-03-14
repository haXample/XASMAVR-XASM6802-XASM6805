// haXASM - Cross-Assembler for 8bit Microprocessors
// macro.cpp - C++ Developer source file.
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
// .MACRO .ENDMACRO .LISTMAC
// The .MACRO directive tells the Assembler that this is the start of a Macro.
// The .MACRO directive takes the Macro name as parameter.
// When the name of the Macro is written later in the program, the Macro
// definition is expanded at the place it was used. A Macro can take up
// to 10 parameters. These parameters are referred to as @0..@9 within the
// Macro definition. When issuing a Macro Staement, the parameters are given
// as a comma separated list. The Macro is terminated by an .ENDMACRO directive.
// By default, only the Macro statement is shown in the listfile
// generated by the Assembler. In order to include the macro expansion
// in the listfile, a .LISTMAC directive must be used.
// A macro is marked with a '+' in the opcode field of the listfile.
// 
// Example
// .MACRO SUBI16       ;; Start macro definition
//   subi @1, LOW(@0)  ;; Subtract low byte
//   sbci @2, HIGH(@0) ;; Subtract high byte
// .ENDMACRO           ;; End macro definition
//
// .CSEG                   ; Start code segment
// SUBI16 0x1234, r16, r17 ; Subtract 0x1234 from r17:r16
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

// Global variables
char* tmpMacBufPtr = NULL;
char* tmpMacLabelPtr = NULL; 

char  macroLabelSuffix[8+1] = "LLLLLLLL";

char* MacroTooLarge = "%s(%d): ERROR - Macro '%s' overflow, .ENDM not found\n";
char* MissinfEndmac = "%s(%d): ERROR - Missing .ENDMACRO\n";

int defMacroLen, expMacroLen, macroIdx;

// Extern variables and functions
extern char* DB_str;

extern void DebugStop(int, char*, char*); // Usage: DebugStop(testNr, "Function()", __FILE__);
extern void DebugPrintBuffer(char*, int); // Usage: DebugPrintBuffer(buffer, count);
extern void DebugPrintMacroArray(int);    // Usage: DebugPrintSymbolArray(count);

extern char* errtx_alloc;
extern char* pszCurFileName;

extern void CloseSrcFile();

extern BOOL delm2(char*);
extern char* skip_leading_spaces(char*);
extern void errorp2(int, char*);
extern BOOL p2pse();                              
extern BOOL p1pse();
extern void clepc();

// Forward declaration of functions included in this code module:
void p12DefineMacro();
void p12endmacro();
void p12CheckEndmacro();

//-----------------------------------------------------------------------------
//
//                         p12CheckEndmacro
//
// Not before eof/.END/.EXIT
//
void p12CheckEndmacro()
  {
  if ((srcEOF == TRUE || iclEOF == TRUE) &&
      (StrStrI(inbuf, ".ENDM") || StrStrI(inbuf, ".ENDMACRO")))
    {
    if (macroBegin > 0) macroBegin--;
    }

  if (macroBegin > 0)
    {
    printf(MissinfEndmac, pszCurFileName, _curline);
    exit(SYSERR_ABORT); 
    }
  } // p12CheckEndmacro

//-----------------------------------------------------------------------------
//
//                         LabelExpandMacro
//
// Bulid unique LOCAL macro Labelnames to prevent doubly defined labels.
// A macro may expand more than once and the macro may have label names
// that are also used elsewhere in the source.
//
// typedef struct tagMACROBUF {    // Macro structure
//   char  macNameStr[SYMLEN+1];   // Macro name string
//   char  macLabelBuf[MACLABBUFSIZE]; // Buffer for local labels in macro
//   char* macBufPtr;              // Pointer to macro body (instruction lines)
//   int    macBufLen;             // Length of macro buffer contents
//   int   macDefCount;            // Assigned when a macro is being defined
//   int   macExpCountP1;          // Incremented when expanded in _PASS1
//   int   macExpCountP2;          // Incremented when expanded in _PASS2
//   UCHAR macType;                // Macro type reserved = 0
// } MACROBUF, *LPMACROBUF;        // Structured macro buffer
// 
BOOL LabelExpandMacro(char* _macBufPtr, int _macLen)
  {
  char* _macLine  = NULL;
  char* _labelPtr = NULL;

  char  _macLabLine[INBUFLEN] = {0};
  char* _macLabLinePtr        = _macLabLine;
  char  _tmpLine[INBUFLEN]    = {0};
  char* _tmpLinePtr           = _tmpLine;

  ULONG _i, _j, _k;
  
  // Nothing to do if no local macro labels exist.
  if (*macrotab[macroIdx].macLabelBuf == 0) return(FALSE);

  //-------------------------------------------------------------------------
  // Local macro labels exist in some macro line(s). Labels will be localized
  // by appending a unique expansion to each label (prolonging the respective
  // macro line). An expansion buffer intecepts lines w/ the appended labels.
  pszExpLabMacroBuf = pszExpLabMacroBuf0;   

  _j=0;               // Macro line buffer index
  while (_j<_macLen)  // Process all macro lines
    {
    // Init pointer to the list of existing macro label names
    //  and get the next line of the macro body 
    _labelPtr = macrotab[macroIdx].macLabelBuf;
    _macLine  = &_macBufPtr[_j];    

    // Try all macro labels (skip comment lines)
    while (*_labelPtr != 0 && *_macLine != ';') 
      {
      // ---------------------------------------------------------------------
      // Process a local macro label within this macro line
      if (
          (_macLabLinePtr=StrStrI(_macLine, _labelPtr)) != NULL &&
          (_macLabLinePtr[strlen(_labelPtr)] == ':'  ||
           (delm2(&_macLabLinePtr[-1]) && delm2(&_macLabLinePtr[strlen(_labelPtr)])))
         )
        {
        //---------------------Begin Inner loop-----------------------
        // Copy (save) and use macro label line buffer
        for (_i=0; _i<=strlen(_macLine); _i++)
         _macLabLine[_i] = _macLine[_i];

        _macLabLinePtr = StrStrI(_macLabLine, _labelPtr);

        while (
               (_macLabLinePtr=StrStrI(_macLabLinePtr, _labelPtr)) != NULL &&
               (_macLabLinePtr[strlen(_labelPtr)] == ':'  ||
                (delm2(&_macLabLinePtr[-1]) && delm2(&_macLabLinePtr[strlen(_labelPtr)])))
              )
          {
          _tmpLinePtr = _tmpLine;              // Re-init to buffer start
          _macLabLinePtr += strlen(_labelPtr); // Move pointer behind macro label

          // Transfer 1st part of _macLabLine including macro label
          _k = (_macLabLinePtr-_macLabLine);
          for (_i=0; _i<_k; _i++) *_tmpLinePtr++ = _macLabLine[_i];

          // Append local suffix to macro label
          for (_i=0; _i<strlen(macroLabelSuffix); _i++)
            *_tmpLinePtr++ = macroLabelSuffix[_i];   

          // Transfer the rest of macro line including CR,LF,0
          //  starting behind label suffix
          for (_i=0; _i<=strlen(_macLabLinePtr); _i++) 
            *_tmpLinePtr++ = _macLabLinePtr[_i];

          // Check tmp buffer overflow
          if ((_tmpLinePtr-_tmpLine) > INBUFLEN-1)
            { errorp2(ERR_MACRO, NULL); return(FALSE); }  // abort

          // Copy (save) and toggle tmp buffer
          for (_i=0; _i<=strlen(_tmpLine); _i++)
           _macLabLine[_i] = _tmpLine[_i];
          } // end while (_macLabLinePtr=StrStrI(..))
        //---------------------End Inner loop-----------------------

        _macLine = _macLabLine; // Macro line may contain other labels

        // Check if there are other labels in this macro line
        _labelPtr += strlen(_labelPtr)+1;
        while (*_labelPtr != 0 && StrStrI(_macLine, _labelPtr) == NULL)
          _labelPtr += strlen(_labelPtr)+1;

        // If no other labels found break and continue w/ next macro line
        if (*_labelPtr == 0) break;
        } // end if (_macLabLinePtr=StrStrI(..))

      // --------------------------------------------------------
      // Try next label within current macro line and check again
      else _labelPtr += strlen(_labelPtr)+1;
      } // end while (_labelPtr != 0)

    // ------------------------------------------------------//
    // Transfer the whole macro line untouched, if there was //
    // no local macro label within this macro line.          //
    // Copy the macro line as is (including CR,LF,0)         //
    if (_macLine == &_macBufPtr[_j])  // untouched?          //
      {                                                      //
      for (_i=0; _i<=strlen(_macLine); _i++)                 //
        *pszExpLabMacroBuf++ = _macLine[_i];                 //
      } // end if                                            //
    // ------------------------------------------------------//

    // ------------------------------------------------------//
    // Transfer the whole macro line w/ localized label(s)   //
    //  and advance index to get next macro line             //
    else                              // label(s) modified   //
      {                                                      //
      for (_i=0; _i<=strlen(_tmpLine); _i++)                 //
        *pszExpLabMacroBuf++ = _tmpLine[_i];                 //
      } // end else                                          //
    // ------------------------------------------------------//

    _j += strlen(&_macBufPtr[_j])+1;  // Try next macro line
    } // end while (_j < macLen)

  return(TRUE);
  } // LabelExpandMacro


//------------------------------------------------------------------------------
//
//                          ParamChk
//
// Check if the needed operand field is not empty. Otherwise give error
//
void ParamChk(char* _paramPtr, char* _param)
  {
  if (swpass == _PASS2 &&_param[0] == 0)
    {
    sprintf(pszErrtextbuf, "Missing macro argument for '%c%c'", _paramPtr[-1], _paramPtr[0]);
    errorp2(ERR_SYNTAX, pszErrtextbuf);
    }
  } // ParamChk

//-----------------------------------------------------------------------------
//
//                         ParamExpandMacro
//
// PatchMacroParams  (Atmel macro type)
// Replace the @ tokens with the parameters given in the macro statement
// _macBufPtr points to a buffer containing the actual macro body
//
// Input (streamed):
//  oper1..oper10 = parameters from macro statement line
//  _macBuf = Lines of the macro body, possibly including param tokens @0..@9
//  _macLen = Size of the macro body
//
// Output (streamed):
//  _expMacroBuf = Lines of the macro body with param tokens replaced by params
//  _expMacroLen  = Size of macro of expanded macro body
//
int ParamExpandMacro(char* _expMacroBufPtr, char* _macBufPtr, int _macLen)
  {
  char* _macBuf = _macBufPtr;     // Save initial start of _macBuf
  char* _expMacroBuf = _expMacroBufPtr; // Save initial start of _expMacroBuf
  int _m, _o;
  char* tmpPtr = NULL;

  _expMacroBufPtr = _expMacroBuf; // Init ptr to start of _expMacroBuf

  for (_j=0; _j<_macLen; _j++)
    {
    if (*_macBufPtr != '@')
      {
      _o=0;     // Flag assuming no comment line
      // At beginning of line:
      // Skip the whole line expansion, if it is a comment (including 'CR,LF,0')
      if (*_macBufPtr == 0)
        {
        tmpPtr = skip_leading_spaces(_macBufPtr+1);
        if (*tmpPtr == ';') // && StrStrI(inbuf, DB_str) == NULL)
          {
          while (*tmpPtr != 0) tmpPtr++;
          _macBufPtr = tmpPtr;
          _o=1; // flag it's a comment line
          }
        } // end if (*_macBufPtr == 0)

      // At end of line:
      // Don't expand comments (TABs look ugly). Skip comments until 'CR,LF,0'
      // CAVEAT: .DB "abcd;efg"  within macro body..!
      if (*_macBufPtr == ';' && StrStrI(inbuf, DB_str) == NULL) 
        {
        while (*_macBufPtr >= SPACE || *_macBufPtr == TAB) _macBufPtr++;
        } 
      // transfer if not a comment line
      if (_o == 0) *_expMacroBufPtr++ = *_macBufPtr++;

      if ((_macBufPtr-_macBuf) >= _macLen) break;
      } // end if (*_macBufPtr != '@')

    else
      {
      _macBufPtr++;  // Skip '@' token
      _m=0;
      switch(*_macBufPtr)
        {
        // Atmel AVR: Params sum up to a total of 10 params
        case '0':    // Atmel AVR parameter place holder = '@0'
          ParamChk(_macBufPtr, oper1);
          while (oper1[_m] != 0) *_expMacroBufPtr++ = oper1[_m++];
          break;
        case '1':    // Atmel AVR parameter place holder = '@1'
          ParamChk(_macBufPtr, oper2);
          while (oper2[_m] != 0) *_expMacroBufPtr++ = oper2[_m++];
          break;
        case '2':    // Atmel AVR parameter place holder = '@21'
          ParamChk(_macBufPtr, oper3);
          while (oper3[_m] != 0) *_expMacroBufPtr++ = oper3[_m++];
          break;
        case '3':    // Atmel AVR parameter place holder = '@3'
          ParamChk(_macBufPtr, oper4);
          while (oper4[_m] != 0) *_expMacroBufPtr++ = oper4[_m++];
          break;
        case '4':    // Atmel AVR parameter place holder = '@4'
          (_macBufPtr, oper5);
          while (oper5[_m] != 0) *_expMacroBufPtr++ = oper5[_m++];
          break;
        case '5':    // Atmel AVR parameter place holder = '@5'
          ParamChk(_macBufPtr, oper6);
          while (oper6[_m] != 0) *_expMacroBufPtr++ = oper6[_m++];
          break;
        case '6':    // Atmel AVR parameter place holder = '@6'
          ParamChk(_macBufPtr, oper7);
          while (oper7[_m] != 0) *_expMacroBufPtr++ = oper7[_m++];
          break;
        case '7':    // Atmel AVR parameter place holder = '@7'
          ParamChk(_macBufPtr, oper8);
          while (oper8[_m] != 0) *_expMacroBufPtr++ = oper8[_m++];
          break;
        case '8':    // Atmel AVR parameter place holder = '@8'
          ParamChk(_macBufPtr, oper9);
          while (oper9[_m] != 0) *_expMacroBufPtr++ = oper9[_m++];
          break;
        case '9':    // Atmel AVR parameter place holder = '@9'
          ParamChk(_macBufPtr, oper10);
          while (oper10[_m] != 0) *_expMacroBufPtr++ = oper10[_m++];
          break;

        // XASMAVR feature: Additional params sum up to a total of 16
        case 'A':    // XASMAVR parameter place holder = '@A'
          while (oper11[_m] != 0) *_expMacroBufPtr++ = oper11[_m++];
          break;
        case 'B':    // XASMAVR parameter place holder = '@B'
          while (oper12[_m] != 0) *_expMacroBufPtr++ = oper12[_m++];
          break;
        case 'C':    // XASMAVR parameter place holder = '@C'
          while (oper13[_m] != 0) *_expMacroBufPtr++ = oper13[_m++];
          break;
        case 'D':    // XASMAVR parameter place holder = '@D'
          while (oper14[_m] != 0) *_expMacroBufPtr++ = oper14[_m++];
          break;
        case 'E':    // XASMAVR parameter place holder = '@E'
          while (oper15[_m] != 0) *_expMacroBufPtr++ = oper15[_m++];
          break;
        case 'F':    // XASMAVR parameter place holder = '@F'
          while (oper16[_m] != 0) *_expMacroBufPtr++ = oper16[_m++];
          break;

        default:
          errorp2(ERR_SYNTAX, &_macBufPtr[-1]);
          break;
        } // end switch

      _macBufPtr++;  // Skip '0', '1', '2', ..
      } // end else
    } // end for
  
  return(_expMacroBufPtr-_expMacroBuf); // _expMacroLen = _expMacroBufPtr-_expMacroBuf
  } // ParamExpandMacro

//-----------------------------------------------------------------------------
//
//                         p12ExpandMacro
//
// fopcd = macNameStr (if any)
// inbuf = param0, param1, .. param9
// MACROBUF macrotab[MACENTRIES+1]; // Global array of macro structures
//
// typedef struct tagMACROBUF {   // Macro structure
//   char  macNameStr[SYMLEN+1];  // Macro name string
//   char  macLabelBuf[10*SYMLEN] // Buffer for local labels in macro
//   char* macBufPtr;             // Pointer to macro instruction body
//   int   macBufLen;             // Length of macro buffer contents
//   int   macDefCount;           // Assigned when a macro is being defined
//   int   macExpCountP1;         // Incremented when macro is expanded in Pass1
//   int   macExpCountP2;         // Incremented when macro is expanded in Pass2
//   UCHAR macType;               // Macro type reserved = 0
// } MACROBUF, *LPMACROBUF;       // Structured macro buffer
//
// typedef struct tagMACCASCADE { // Macro cascading structure (Macros within macros)
//   char* macXferBufPtr;         // Pointer to current macro Xfer line string
//   int   macXferBufLen;         // Length of macro Xfer contents
//   char* macSaveBufPtr;         // Pointer to complete macro contents
//   int   macSaveBufLen;         // Length of complete macro contents
// } MACCASCADE, *LPMACCASCADE;
//
// MACRO expansion process:
//  Search macrotab[i].macNameStr in fopcd. If found  retrieve the
//  corresponding instruction lines of length macrotab[i].macBufLen 
//  residing in macrotab[i].macBufPtr and patch the params appropriately.
//  Then provide the lines to Pass1/2 as if been read normally
//  from the src file for assembling.
//
// Pass1: Arriving here on every src line (except comments)
// Pass2: Arriving here only if src line contains an unknown instruction
//        (which possibly may be a macro name to be expanded)
// 
BOOL p12ExpandMacro(int _p12sw)
  {
  BOOL b_macroStat = FALSE;       // Initially unknown opcode (no expansion)

  // Empty opcode or Pseudo Instructions/Directives are skipped.
  if (fopcd[0] == 0 || ins_group == 0 || ins_group == 1) return(FALSE); 

  // Search macrotab[] to see if a macro name presents the unknown opcode                
  for (macroIdx=1; macroIdx<MACENTRIES; macroIdx++) // Entry _i=0 = Reserved
    {
    // Return if no more macros (end of macrotab[])
    if (macrotab[macroIdx].macBufPtr == NULL) break;

    if (StrCmpI(macrotab[macroIdx].macNameStr, fopcd) == 0)
      {
      swexpmacro++;               // Flag we're expanding a macro
      b_macroStat = TRUE;         // 'fopcd' is a macro name - expand.
      break;                      // continue w/ TRUE
      }
    } // end for

  // Currently only 1 additional macro within a macro definition allowed
  if (swexpmacro > MACCASCADEMAX)   
    {
    swexpmacro = 0;
    errorp2(ERR_MACRO, NULL);
    return(b_macroStat);
    }   
     
  //-------------------------------------------------------------------------
  // Expand the valid macro with the buffered macro instruction lines.
  // Construct a new src line replacing the params @0..@9 with oper1..oper10.
  // The new src line is provided in expMacroBuf for interception in dcomp.cpp.  
  if (b_macroStat)
    {
    // Build unique suffix for possible LOCAL macro labels,
    //  increment separate macro expansion counters for Pass1/Pass2
    if (_p12sw == _PASS1)
      {
      macrotab[macroIdx].macExpCountP1++;
      sprintf(&macroLabelSuffix[0], "_%04d%04d", macroIdx, macrotab[macroIdx].macExpCountP1);
      }
    if (_p12sw == _PASS2)
      {
      macrotab[macroIdx].macExpCountP2++;
      sprintf(&macroLabelSuffix[0], "_%04d%04d", macroIdx, macrotab[macroIdx].macExpCountP2);
      }

    // Clean current buffers
    for (_j=0; _j<LSBUFLEN; _j++)
      {
      pszDefMacroBuf0[_j] = 0;
      pszExpMacroBuf0[_j] = 0;
      } // end for

    // Save macro body contents and initialize current .macBufLen / .macBufPtr
    defMacroLen = macrotab[macroIdx].macBufLen;
    for (_j=0; _j<=defMacroLen; _j++) pszDefMacroBuf0[_j] = macrotab[macroIdx].macBufPtr[_j];

    pszDefMacroBuf = pszDefMacroBuf0;              // Init current .macBufPtr

    ///////////////////////////////////////////////////////////////////////////////////
    // Macro label(s) may be present somewhere in the macro body                     // 
    if (LabelExpandMacro(pszDefMacroBuf, defMacroLen))                               //
      {                                                                              //
      // --------------------------------------------------------------------------- //
      // Provide the macro body buffer (with localized labels)                       //
      // in 'defMacroBuf[]' for further processing of the macro params               //
      //                                                                             //
      defMacroLen = (pszExpLabMacroBuf-pszExpLabMacroBuf0);                          //
      for (_j=0; _j<defMacroLen; _j++) pszDefMacroBuf0[_j] = pszExpLabMacroBuf0[_j]; //
      }                                                                              //
    // Insert the parameters given in the macro statement                            //
    expMacroLen = ParamExpandMacro(pszExpMacroBuf, pszDefMacroBuf, defMacroLen);     //
    ///////////////////////////////////////////////////////////////////////////////////

    // Init initial (outmost) Macro struct
    macrocascade[swexpmacro].macXferBufPtr = pszExpMacroBuf0; // see "dcomp.cpp"
    macrocascade[swexpmacro].macXferBufLen = expMacroLen;     // see "dcomp.cpp"
    macrocascade[swexpmacro].macSaveBufLen = expMacroLen;     // see "dcomp.cpp"
    // Allocate a temporary buffer
    macrocascade[swexpmacro].macSaveBufPtr = (char*)GlobalAlloc(GPTR, expMacroLen+1);
    if (macrocascade[swexpmacro].macSaveBufPtr == NULL)
      {
      CloseSrcFile();
      printf(errtx_alloc);   // >>>>>>>>> ERROR: MEMORY ALLOC <<<<<<<<<<
      exit(SYSERR_MALLOC);
      }
    // Save outmost Macro contents (in case of macros within a macro)
    // Save Xfer buffer
    for (_j=0; _j<macrocascade[swexpmacro].macSaveBufLen; _j++)
      macrocascade[swexpmacro].macSaveBufPtr[_j] = macrocascade[swexpmacro].macXferBufPtr[_j]; 
    } // end if (b_macroStat)

  return(b_macroStat); 
  } // p12ExpandMacro


//-----------------------------------------------------------------------------
//
//                         p12CheckMacro()
//
// An unknown opcode was found.
// If it is a name of a macro - expand the macro. 
//
BOOL p12CheckMacro()
  {
  return(p12ExpandMacro(_PASS2)); 
  } // p12CheckMacro

//-----------------------------------------------------------------------------
//
//                          p12DefineMacro()
//
//  Arriving here from Pass1:
//   if (swdefmacro && ((ins_group == 1 && insv[1] != 53) || ins_group != 1))
//   ..we intercept each src line from macro body until .ENDM
//
//  // Copy the line from the allocated buffer into the static inbuf[]
//  //  and truncate the line if it's too long (prevent buf overflow).
//  for (_i=0; _i<INBUFLEN; _i++) inbuf[_i] = pszFileInbuf[_i];  // fill inbuf
//  if (sizeof(pszFileInbuf) > INBUFLEN) inbuf[INBUFLEN] = 0;    // truncate
//  
//  // The buffer has been saved, so free and discard allocated memory 
//  if (pszFileInbuf != NULL) pszFileInbuf = (LPSTR)GlobalFree(pszFileInbuf);
//
// typedef struct tagMACROBUF {   // Macro structure
//   char  macNameStr[SYMLEN+1];  // Macro name string
//   char  macLabelBuf[10*SYMLEN] // Buffer for local labels in macro
//   char* macBufPtr;             // Pointer to macro instruction body
//   int   macBufLen;             // Length of macro buffer contents
//   int   macDefCount;           // Assigned when a macro is being defined
//   int   macExpCountP1;         // Incremented when a macro is being expanded in _PASS1
//   int   macExpCountP2;         // Incremented when a macro is being expanded in _PASS2
//   UCHAR macType;               // Macro type reserved = 0
// } MACROBUF, *LPMACROBUF;       // Structured macro buffer
//
// typedef struct tagMACCASCADE { // Macro cascading structure (Macros within macros)
//   char* macLineBufPtr;         // Pointer to current macro line string
//   int   macLineBufLen;         // length of current macro line string
// } MACCASCADE, *LPMACCASCADE
//
void p12DefineMacro(int _p12sw)
  {
  int _l = FALSE;

//ha//  if (ins_group == 1 && insv[1] == 53) // ".ENDMAC/.ENDMACRO" directives
//ha//    {
//ha//    p12endmacro();                     // Pass1/Pass2
//ha//    return;
//ha//    }
//ha//  if (_p12sw == _PASS2) return;        // Already done in Pass1

  // Intercept ".ENDMAC/.ENDMACRO" directives (via Pass1/Pass2)
  if (     _p12sw == _PASS1 && ins_group == 1 && insv[1] == 53) { p1pse();  return; }
  else if (_p12sw == _PASS2 && ins_group == 1 && insv[1] == 53) { p2pse();  return; }
  else if (_p12sw == _PASS2) return;    // Following was already done in Pass1

  // PASS1 only:
  // Copy the inbuf line by line into the allocated buffer
  for (_i=0; _i<=strlen(inbuf); _i++)
    {
    // check if pseudo label (within comment)
    if (inbuf[_i] == ':' && StrStrI(inbuf, DB_str) == NULL)  
      {
      _j=_i-1;
      while (inbuf[_j] != 0)
        {
        // Comment: ignore  label 
        if (inbuf[_j] == ';') { _l=TRUE;  break; } 
        _j--;
        }
      
      // Collect the macro label(s) excluding the colon ':' (_k<_i)
      // Collection is .IF-ELSE condition dependent (also check .IF-ELSE condition)
      if (_l == FALSE && (~p12CSkipFlag & TRUE) == FALSE)
        {
        for (_k=0; _k<_i; _k++)
          {
          // skip over leading spaces / tabs
          if (inbuf[_j+_k+1] > SPACE) *tmpMacLabelPtr++ = inbuf[_j+_k+1];
          }
        tmpMacLabelPtr++;
        }
      } // end if (inbuf == ':')

//ha//// Don't define comment (expanding TABs later look ugly).   //ha//
//ha//// Skip comment until 'CR,LF,0'                             //ha//
//ha//if (inbuf[_i] == ';' && StrStrI(inbuf, DB_str) == NULL)     //ha//
//ha//  {                                                         //ha//
//ha//  while (inbuf[_i] != 0) _i++;                              //ha//
//ha//  if (inbuf[_i-1] == LF) _i--;                              //ha//
//ha//  if (inbuf[_i-2] == CR) _i--;                              //ha//
//ha//  }                                                         //ha//

    *tmpMacBufPtr++ = inbuf[_i]; 
    if (tmpMacBufPtr >= tmpMacBufPtr+(MACBUFSIZE-INBUFLEN))
      {
      printf("%s: Too many macros\n", pszCurFileName);
      exit(SYSERR_ABORT);              // Error-Exit to system
      }
    macrotab[macroCount].macBufLen++;
    if (macrotab[macroCount].macBufLen > MACBUFSIZE)
      {                                  
      printf(MacroTooLarge, pszCurFileName, _curline, macrotab[macroCount].macNameStr);
      exit(SYSERR_ABORT); 
      }
    } // end for
  } // p12DefineMacro 

//-----------------------------------------------------------------------------
//
//                            p12macro
//
// .MACRO directive: Turn ON macro interception flag for Pass1/2 control
//
// Allocate macro buffer (ptr)
// Increment structured array pointer (1st entry = NULL reserved)
// Store macro buffer (ptr) into next entry of structured macro buffer array
// Store macro name into structured macro buffer array
//  (..must be evaluated later when macro statment appears in src)
//  DCOMP: if (swdefmacro = TRUE)
//           intercept and put macro lines into the macro buffer
//
// typedef struct tagMACROBUF {   // Macro structure
//   char  macNameStr[SYMLEN+1];  // Macro name string
//   char  macLabelBuf[10*SYMLEN] // Buffer for local labels in macro
//   char* macBufPtr;             // Pointer to macro instruction body
//   int   macBufLen;             // Length of macro buffer contents
//   int   macDefCount;           // Assigned when a macro is being defined
//   int   macExpCountP1;         // Incremented when a macro is being expanded in _PASS1
//   int   macExpCountP2;         // Incremented when a macro is being expanded in _PASS2
//   UCHAR macType;               // Macro type reserved = 0
// } MACROBUF, *LPMACROBUF;       // Structured macro buffer
//
// typedef struct tagMACCASCADE { // Macro cascading structure (Macros within macros)
//   char* macLineBufPtr;         // Pointer to current macro line string
//   int   macLineBufLen;         // length of current macro line string
// } MACCASCADE, *LPMACCASCADE
//
void p12macro(int _p12sw)
  {
  swdefmacro = TRUE;        // .MACRO definition statement

  // PASS1 only:
  // Macro array is already initialized in Pass1
  if (_p12sw == _PASS2) return;
  else
    {
    macroCount++;           // Number of macros defined within .SRC file
    macroBegin++;           // Open macro statment

    // Allocate buffer and check if the buffer is available
    //macrotab[macroCount].macBufPtr = (char*)GlobalAlloc(GPTR, MACBUFSIZE); 
    macrotab[macroCount].macBufPtr = (char*)malloc(MACBUFSIZE); 
       
    if (macrotab[macroCount].macBufPtr == NULL)
      {
      CloseSrcFile();
      printf(errtx_alloc);    // >>>>>>>>> ERROR: MEMORY ALLOC <<<<<<<<<<
      exit(SYSERR_MALLOC);
      }
    else 
      {
      tmpMacLabelPtr = macrotab[macroCount].macLabelBuf;
      tmpMacBufPtr = macrotab[macroCount].macBufPtr;
      for (_i=0; _i<strlen(oper1); _i++) 
        macrotab[macroCount].macNameStr[_i] = oper1[_i];
      macrotab[macroCount].macDefCount = macroCount;
      }
    } // end else Pass1
  } // p12macro

//-----------------------------------------------------------------------------
//
//                          p12endmacro
//
// .ENDMACRO directive: Turn OFF macro interception flag for normal processing.
// Optimize Buffer ressources: allocate a new buffer .macBufLen+1
//                             copy old buffer .macBufLen   into new buffer
//                             free old buffer
//
// Not before eof/.END/.EXIT:
// Free all macro buffer ptrs(s) residing in structured macro buffer array
//
void p12endmacro()
  {
  if (srcEOF == FALSE && macroBegin > 0) macroBegin--; // Close macro definition   
  swdefmacro = FALSE;                                  // Stop macro interception
  } // p12endmacro

//------------------------------------------------------------------------------

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//if (_debugSwPass == _PASS2)
//ha//{
//ha////ha//DebugPrintBuffer(macrotab[macroIdx].macLabelBuf, 23);
//ha//printf("_labelPtr=[%s]\n_tmpLinePtr0='%s'\n _tmpLinePtr='%s'\n_macLabLinePtr='%s'\npszExpLabMacroBuf='%s'",
//ha//        _labelPtr,      _tmpLinePtr0,       _tmpLinePtr,      _macLabLinePtr,     pszExpLabMacroBuf);
//ha//DebugStop(4, "LabelExpandMacro()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//if (_debugSwPass == _PASS2)
//ha//{
//ha////ha//DebugPrintBuffer(macrotab[macroIdx].macLabelBuf, 23);
//ha//printf("delm2(&_macLabLinePtr[-1])=%d  _labelPtr=[%s]\n_tmpLine1='%s'\n_macLabLine='%s'\n_macLine ='%s'",
//ha//        delm2(&_macLabLinePtr[-1]),    _labelPtr,      _tmpLine1,      _macLabLine,      _macLine);
//ha//printf("&_macLabLinePtr[-1] ");                     
//ha//DebugPrintBuffer(&_macLabLinePtr[-1], strlen(&_macLabLinePtr[-1])+1);
//ha//DebugStop(4, "LabelExpandMacro()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha////if (_debugSwPass == 2)
//ha//{
//ha//char* tmpPtr1 = macrotab[_i].macLabelBuf;
//ha//char* tmpPtr2 = tmpPtr1 + strlen(tmpPtr1); tmpPtr2++;
//ha//char* tmpPtr3 = tmpPtr2 + strlen(tmpPtr2); tmpPtr3++;  // 3 are enough for test
//ha//printf("macro[%d]=<%s> LOCAL Label: [%s %s %s] macroLabelSuffix=%s\n"
//ha//       "_macLine=%s\n"
//ha//       "_macLabLinePtr=%s\n"
//ha//       "defMacroLen=%d  strlen(expLabMacroBuf)=%d  expLabMacroBufPtr-expLabMacroBuf=%d\n",
//ha//       _i, macrotab[_i].macNameStr, tmpPtr1, tmpPtr2, tmpPtr3,  macroLabelSuffix,
//ha//       _macLine, _macLabLinePtr,
//ha//       defMacroLen,      strlen(expLabMacroBuf),  expLabMacroBufPtr-expLabMacroBuf);
//ha//DebugStop(30, "p12ExpandMacro()", __FILE__);
//ha//printf("defMacroBuf ");                     
//ha//DebugPrintBuffer(defMacroBuf, defMacroLen);
//ha//DebugStop(31, "p12ExpandMacro()", __FILE__);
//ha////ha//printf("expLabMacroBuf ");
//ha////ha//DebugPrintBuffer(expLabMacroBuf, expLabMacroBufPtr-expLabMacroBuf);
//ha////ha//printf("defMacroLen=%d  strlen(expLabMacroBuf)=%d  expLabMacroBufPtr-expLabMacroBuf=%d\n",
//ha////ha//        defMacroLen,      strlen(expLabMacroBuf),  expLabMacroBufPtr-expLabMacroBuf);
//ha////ha//DebugStop(32, "p12ExpandMacro()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha////if (_debugSwPass == 2)
//ha//{
//ha//printf("<%s>\n", macrotab[macroCount].macNameStr);
//ha//printf("macrotab[%d].macLabelBuf ", macroCount);
//ha//DebugPrintBuffer(macrotab[macroCount].macLabelBuf, 2*SYMLEN);
//ha////ha//printf("expMacroBuf ");
//ha////ha//DebugPrintBuffer(expMacroBuf, expMacroBufPtr-expMacroBuf);
//ha//DebugStop(1, "p12DefineMacro()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha////if (_debugSwPass == 2)
//ha//{
//ha//printf("defMacroBuf=%08X  defMacroBufPtr=%08X\n"
//ha//       " defMacroBufPtr-defMacroBuf=%d  defMacroLen=%d  expMacroBufPtr-expMacroBuf=%d\n", 
//ha//       defMacroBuf, defMacroBufPtr, defMacroBufPtr-defMacroBuf, defMacroLen, expMacroBufPtr-expMacroBuf);
//ha//printf("defMacroBufPtr ");
//ha//DebugPrintBuffer(defMacroBufPtr, defMacroLen);
//ha//printf("expMacroBuf ");
//ha//DebugPrintBuffer(expMacroBuf, expMacroBufPtr-expMacroBuf);
//ha//DebugStop(11, "p12ExpandMacro()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//if (pcc >0) // Skip long Atmel .INCLUDE stuff
//ha//{
//ha//printf("Flabl=%s  fopcd=%s  oper1=%s  b_macroStat=%d  defMacroLen=%d\n"
//ha//       "macrotab[%d].macNameStr=<%s>\n",
//ha//        flabl,    fopcd,    oper1, b_macroStat, defMacroLen,
//ha//        _i, macrotab[_i].macNameStr);  
//ha////ha//printf("expMacroBufPtr ");
//ha////ha//DebugPrintBuffer(expMacroBufPtr, defMacroLen);
//ha////ha//DebugStop(1, "p12ExpandMacro()", __FILE__);
//ha////ha//printf("&expMacroBufPtr[_l] ");
//ha////ha//DebugPrintBuffer(&expMacroBufPtr[_l], defMacroLen);
//ha//DebugStop(1, "p12ExpandMacro()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("Flabl=%s  fopcd=%s  oper1=%s  ins_group=%d  insv[1]=%d\n"
//ha//       "swdefmacro=%d  macroBegin=%d  macroCount=%d  macrotab[%d].macBufLen=%d\n",
//ha//        flabl,    fopcd,    oper1, ins_group, insv[1],
//ha//        swdefmacro, macroBegin, macroCount, macrotab[macroCount].macBufLen);  
//ha////printf("macrotab[macroCount].macBufPtr ");
//ha////DebugPrintBuffer(macrotab[macroCount].macBufPtr, strlen(macrotab[macroCount].macBufPtr));
//ha//DebugStop(1, "p12macro()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//DebugPrintMacroArray(10);
//ha//DebugStop(15, "p12endMacro()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//--------------------------end-of-c++-module-----------------------------------

