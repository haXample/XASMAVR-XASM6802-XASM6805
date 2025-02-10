// haXASM - Cross-Assembler for 8bit Microprocessors
// lstng.cpp - C++ Developer source file.
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

extern char szLstFileName[];              // e.g = "file.LST"
extern char* pszCurFileName;              // e.g = "file.ASM / File.INC"

extern char* TITLE_str;                   // just to truncate .title, .subttl, .. in source

extern ofstream LstFile;                  // File write (*.LST)

extern BOOL errorp1();
extern void finish();

extern void ActivityMonitor(int);

// Forward declaration of functions included in this code module
void linut(char*);
void write_lstbuf(char*);
void errchk(char*, int);

//-----------------------------------------------------------------------------
//
//                      warnout
//
// Warn nr.:
//  WARN_NONE      // 0  reserved for no warning
//  WARN_IGNORED   // 1  "Directive ignored (not supported).";                    
//  WARN_DEVICE    // 2  "Directive ignored (refer to CPU manual and data sheet)";
//  WARN_TOOLONG   // 3  "Source line too long  (>%d characters)";                
//  WARN_CHKEXIT   // 4  "Found END-OF-FILE directive in include file";
//  WARN_MSKVALUE  // 5  "Out of range, value is masked";   
//
void warnout()
  {
  // Don't list warning when it is conditionally suppessed by 'if/endif' //ha//
  if ((p12CSkipFlag & TRUE) == FALSE)
    {
    warnSymbol_ptr = NULL;   // Clear warning info text
    warno = 0;               // Reset warno
    return;                   
    }
  // Return if no warning found in Pass2
  if (warno == WARN_NONE) return;

  // IMPORTANT NOTE:
  // Do not patch the char* warnSymbol_ptr --> Exception! 
  // First copy the string into a buffer, then patch the string
  //  and write it to ListBuf (.LST file) or display it on console.
  //
  if (warnSymbol_ptr != NULL)
    {
    sprintf(szListBuf, "<<<%s: |WARNING| %s: %s\n", pszCurFileName, warnText[warno], warnSymbol_ptr);
    linut(szListBuf);   // Transfer into .LST file
    // display on console.
    printf("%s(%d) |WARNING| %s: %s\n", pszCurFileName, _curline, warnText[warno], warnSymbol_ptr);
    }
  else
    {
    sprintf(szListBuf, "<<<%s: |WARNING| %s\n", pszCurFileName, warnText[warno]);
    linut(szListBuf);   // Transfer into .LST file
    // display on console.
    printf("%s(%d) |WARNING| %s\n", pszCurFileName, _curline, warnText[warno]);
    }

  warnSymbol_ptr = NULL;   // Clear warning info text
  warno = 0;               // Reset warno
  } // warnout

//-----------------------------------------------------------------------------
//
//                      erout
//
// System ERRORS: see definitions in 'equate.h'
//
// Example- Format error message
// XASMAVR_errTest.ASM(38) 'ERROR' Unknown directive: .osiduf
//
// Error nr.: 1..31
//  #define ERR_NONE       0  // reserved for no error
//  #define ERR_SYNTAX     1  // syntax error in source line
//  #define ERR_ILLOPCD    2  // illegal opcode
//  #define ERR_TOOLONG    3  // Line too long or statement too complex
//  #define ERR_SYMFULL    4  // symbol table overflow
//  #define ERR_ILLOP      5  // Illegal or unknown operand
//  #define ERR_ILLOPER    6  // Illegal operator
//  #define ERR_ILLBYTE    7  // Illegal byte expression
//  #define ERR_P1PHASE    8  // pass1 phase error
//  #define ERR_ILLEXPR    9  // illegal expression
//  #define ERR_MISSEXP   10  // missing expression
//  #define ERR_UNDFSYM   11  // undefined symbol
//  #define ERR_DBLSYM    12  // doubly defined symbol
//  #define ERR_BADREG    13  // bad register
//  #define ERR_JMPADDR   14  // jump out of range
//  #define ERR_INCDPTH   15  // INCLUDE depth > 1
//  #define ERR_ORGINS    16  // ORG directive misplaced (segment overlap)
//  #define ERR_P1UNDFSYM 17  // Pass1 equate w/ unknown (forwarded) symbol
//  #define ERR_DSEG      18  // Instructions not allowed in Data Segment
//  #define ERR_SEGADDR   19  // Segment address out of range
//  #define ERR_ILLDIR    20  // Unknown directive
//  #define ERR_SYMABS    21  // Symbol must be of type 'ABS'
//  #define ERR_MACRO     22  // Macro definition or macro too complex
//  #define ERR_LONGCONST 23  // Numeric constant too long
//  #define ERR_25        25  // ""; 
//  #define ERR_26        26  // ""; 
//  #define ERR_27        27  // ""; 
//  #define ERR_28        28  // ""; 
//  #define ERR_29        29  // ""; 
//  #define ERR_30        30  // ""; 
//  #define ERR_31        31  // "";
//
void erout()
  {
  char _errUnknown[10];            // Define a local buffer
  char* _errtxt_ptr = _errUnknown; // Point to the local buffer

  // Don't list error when it is conditionally suppessed by 'if/endif' //ha//
  if ((p12CSkipFlag & TRUE) == FALSE)
    {
    errSymbol_ptr = NULL;     // Clear error info
    erno = 0;                 // Reset erno
    return;                   
    }

  // Check if any error found in Pass1
  // PASS2: If 'erno' for current source line is already occupied with error code 
  // issued in Pass1, take that one. Otherwise take the error detected in Pass2.
  // Eventually ignore pass1 errors if opportune (see p2db)
  if (swp1err != ERR_P1_OFF) errorp1(); 

  // Return if no error found in Pass1/Pass2
  if (erno == ERR_NONE) return;

  ////////////////////////
  ActivityMonitor(OFF); //
  ////////////////////////

  // Just print the error number if error is unspecified
  if (erno >= _ERREND_) sprintf(_errtxt_ptr, "%04d", (UINT)erno);
  else _errtxt_ptr = errText[erno];

  // IMPORTANT NOTE:
  // Do not patch the char* errSymbol_ptr --> Exception! 
  // First copy the string into a buffer, then patch the string
  //  and write it to ListBuf (.LST file) or display it on console.
  //
  if (errSymbol_ptr != NULL)
    {
    sprintf(szListBuf, ">>>%s: |ERROR| %s: %s\n", pszCurFileName, _errtxt_ptr, errSymbol_ptr);
    linut(szListBuf);   // Transfer into .LST file
    // display on console.
    printf("%s(%d): |ERROR| %s: %s\n", pszCurFileName, _curline, _errtxt_ptr, errSymbol_ptr);
    }
  else
    {
    sprintf(szListBuf, ">>>%s: |ERROR| %s\n", pszCurFileName, _errtxt_ptr);
    linut(szListBuf);   // Transfer into .LST file
    // display on console.
    printf("%s(%d): |ERROR| %s\n", pszCurFileName, _curline, _errtxt_ptr);
    }
  
  if (ercnt == 0 && swlst == 1) errline1 = _lstline;
  ercnt++;
  if (ercnt > 200)
    {
    printf("Errorcount > 200 - Assembly aborted.\n"); 
    exit(SYSERR_ABORT);     // Exit to Operating System (whatsoever)
    }
  errSymbol_ptr = NULL;     // Clear error info
  erno = 0;                 // Reset erno
  } // erout

//-----------------------------------------------------------------------------
//
//                      lineAppendCRLF
//
// Terminate a line with CR,LF,0
//
void lineAppendCRLF(char* _lineBuf)
  {
  _lineBuf[0] = CR; 
  _lineBuf[1] = LF; 
  _lineBuf[2] =  0;
  } // lineAppendCRLF

//-----------------------------------------------------------------------------
//
//                      newli
// New lines
//
void newli(int _cnt)
  {
  linct -= _cnt;
  if (_cnt == 0) _cnt++;      // Force CRLF w/o linct decrement
  for (_i=0; _i<_cnt; _i++)
    linut("\n");       // Write number of CRLFs into .LST file
  } // newli

//-----------------------------------------------------------------------------
//
//                      lhead
// Listing page heading
//
void lhead()
  {
  // No header if .NOLIST or within macro expansion
  if (swlst == 0) return;

  linct = pagel;                     // Re-init page length line counter
  pge++;                             // Increment page number
  sprintf(pagpos_ptr, "%4d\r", pge); // Emit BCD converted page count into lhbuf

  // Header
  // ".......... Cross-Assembler, Version 2.0...................09/11/2001 PAGE....0\n"
  write_lstbuf(lhbuf);       // Transfer into .LST file
  // "Title"
  write_lstbuf("\n");        // newli(1);
  if (strlen(lhtit) > 0)     // Any Title at all?
    write_lstbuf(lhtit);     // Transfer into .LST file

  // "Subtitle"
  if (strlen(lhsubttl) > 0)  // Any Subtitle at all?
    write_lstbuf(lhsubttl);  // Transfer into .LST file

  // Symbols and field headline
  if (swsym != 0)
    {
    write_lstbuf("\n");      // newli(1);
    return;                  // No symbols to be printed - skip text
    }
  else                       // Print formatted headline in listing
    {
    // ".LOC...OBJ...............LINE...SOURCE\n"
    write_lstbuf("\n");      // newli(1);
    write_lstbuf(lhtxt_ptr); // Transfer into .LST file
    write_lstbuf("\n");      // newli(1);
    }

  linct -= 3;                // Header + Title line + Subttl line
  } // lhead

//-----------------------------------------------------------------------------
//
//                      linut
// Print szListBuf
//
void linut(char* _lstbuf)
  {
  // Don't list macro body when it is conditionally suppessed by 'if/endif' //ha//
  if ((p12CSkipFlag & TRUE) == FALSE && p2defmacroFlag == TRUE) return;     //ha//
  
  if ((p12CSkipFlag & TRUE) == FALSE) return;                               //ha//

  // Check if to suppress macro expansion in .LST file
  if (swlistmac == FALSE && swexpmacro) return; 

  // default: prevent ListBuf overrun
  if (linel > INBUFLEN/2) linel = INBUFLEN/2;                 

  if (swlst == 0 && erno == 0) return; // Note: swlst should be =1
  else if (linct == 0) lhead();        // Print header into ListBuf

  //--------------------------------
  // Append source line to .LST file
  //--------------------------------
  write_lstbuf(_lstbuf);            // Transfer into .LST file
  linct--;                            // Decrement line counter
  } // linut


//------------------------------------------------------------------------------
//
//                    write_lstbuf
//
// Write .LST file, fill lstbuf from other buffers
// Note: 0-terminator not included if text-line mode
//
void write_lstbuf(char* _listbuf)    
  {
  if (swlst == 1)
    {
    // Re-init Listbuffer if appropriate
    if (_listbuf == szListBuf)
      {
      // Truncate list line according to .PAGEWIDTH()
      // Note: TABs are ignored (may prolong some list lines a little)
      if (strlen(szListBuf) >= linel) lineAppendCRLF(&szListBuf[linel]);
      LstFile.write(szListBuf, strlen(szListBuf)); 
      for (_i=0; _i<LSBUFLEN; _i++) szListBuf[_i] = 0;
      pszListBuf = szListBuf; 
      }

    else LstFile.write(_listbuf, strlen(_listbuf));
    errchk(szLstFileName, GetLastError());       // Check for file I/O errors
    } // end if else

  // Reset macro statement line flag (in case of .(NO)LISTMAC)
  if (swlistmac == 0x80) swlistmac &= ~0x80;           
  } // write_lstbuf

//-----------------------------------------------------------------------------
//
//                      errchk                    
//
// Check System Error Codes (0-499) (0x0-0x1f3)
// int _lastErr = GetLastError();
//
// The following uses some of system error codes defined in 'WinError.h'.
//
//  
//
void errchk(char* _filename, int _lastErr)
  {
  char szSystemError[]       = "|SYSTEM ERROR|";
  char szErrorOpenFailed[]   = "Open failed";
  char szErrorNotReady[]     = "Device not ready";
  char szErrorFileNotFound[] = "File not found";
  char szErrorPathNotFound[] = "Path not found";
  char szErrorFileIsUsed[]   = "File is being used by another process.";
  char szErrorAccessDenied[] = "Access denied";         
  char szErrorDiskFull[]     = "Disk full.";
  char szErrorFileWrite[]    = "File write failed.";
  char szErrorFileRead[]     = "File read failed.";
  char szErrorFileExists[]   = "File exists.";

  if (_lastErr != 0)
    {
    switch(_lastErr)
      {
      case ERROR_NOT_READY:         // 0x15
        printf("%s %s: %s\n", szSystemError, _filename, szErrorNotReady);
        break;                                                                                                 
      case ERROR_FILE_NOT_FOUND:    // 0x02
        printf("%s %s: %s\n", szSystemError, _filename, szErrorFileNotFound);
        break;
      case ERROR_OPEN_FAILED:       // 0x6E
        printf("%s %s: %s\n", szSystemError, _filename, szErrorOpenFailed);
        break;
      case ERROR_BAD_NETPATH:       // 0x35
      case ERROR_PATH_NOT_FOUND:    // 0x03
        printf("%s %s: %s\n", szSystemError, _filename, szErrorPathNotFound);
        break;                                                                                                 
      case ERROR_SHARING_VIOLATION: // 0x20
        printf("%s %s: %s\n", szSystemError, _filename, szErrorFileIsUsed);
        break; 
      case ERROR_ACCESS_DENIED:     // 0x05
        printf("%s %s: %s\n", szSystemError, _filename, szErrorAccessDenied);
        break;
      case ERROR_HANDLE_DISK_FULL:  // 0x27
      case ERROR_DISK_FULL:         // 0x70
        printf("%s %s: %s\n", szSystemError, _filename, szErrorDiskFull);
        break;
      case ERROR_NET_WRITE_FAULT:   // 0x58
      case ERROR_WRITE_PROTECT:     // 0x13
      case ERROR_WRITE_FAULT:       // 0x1D
        printf("%s [%d] %s: %s\n", szSystemError, _lastErr, _filename, szErrorFileWrite);
        break;
      case ERROR_READ_FAULT:        // 0x1E
        printf("%s %s: %s\n", szSystemError, _filename, szErrorFileRead);
        break;
      case ERROR_FILE_EXISTS:       // 0x50
        printf("%s %s: %s\n", szSystemError, _filename, szErrorFileExists);
        break;
      default:                      // any other system error number
        printf("%s %s: LastErrorCode = 0x%08X [%d]\n", szSystemError, _filename, _lastErr, _lastErr);
        break;
      } // end switch

    exit(SYSERR_ABORT);
    } // end if

  } // errchk

//------------------------------------------------------------------------------

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//if (pcc >=0x7C)
//ha//{
//ha//printf("fopcd=%s  p12CSkipFlag=%d", fopcd, (p12CSkipFlag & TRUE));
//ha//DebugStop(1, "linut()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("TITLE_str=%s\n", TITLE_str);  
//ha//printf("lhtit ");
//ha//DebugPrintBuffer(lhtit, LHTITL+2);
//ha//DebugStop(2, "lhead()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("ListBuf ");
//ha//DebugPrintBuffer(ListBuf, strlen(ListBuf)+1);
//ha//DebugStop(1, "lhead()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("linel=%d INBUFLEN+lpSRC=%d  lpSRC=%d  linct=%d [%02X]", linel, INBUFLEN+lpSRC, lpSRC, linct, linct);
//ha//DebugStop(1, "linut()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//--------------------------end-of-c++-module-----------------------------------

