// haXASM - Cross-Assembler for 8bit Microprocessors
// finish.cpp - C++ Developer source file.
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

#include <sys\stat.h>  // For filesize
#include <iostream>    // I/O control
#include <fstream>     // File control

#include <shlwapi.h>   // StrStrI

#include <windows.h>   // For console specific functions

#include "equate.h"
#include "extern.h"    // Variables published in workst.cpp

using namespace std;

// Global variables

// Extern variables and functions
extern void DebugStop(int, char*, char*); // Usage: DebugStop(testNr, "Function()", __FILE__);
extern void DebugPrintBuffer(char*, int); // Usage: DebugPrintBuffer(buffer, count);
extern void DebugPrintSymbolArray(int);   // Usage: DebugPrintSymbolArray(count);

extern int predefCount;                   // If any pre-define symbols

extern char szLstFileName[];     //= "File.LST";
extern char szHexFileName[];     //= "File.HEX";
extern char szBinFileName[];     //= "File.BIN";
extern char szEEPHexFileName[];  //= "File.EEP.HEX"

extern void CloseSrcFile();

extern ifstream SrcFile;    // Filestream read (*.ASM)
extern ofstream LstFile;    // File write (*.LST)
extern ofstream HexFile;    // File write (*.HEX)
extern ofstream EEPHexFile; // Filestream write (*.EEP.HEX)
extern ofstream BinFile;    // File write (*.BIN)

extern void errorp2(int, char*);
extern void errchk(char*, int);
extern void newli(int);
extern void edwh(char*, int);
extern void edbh(char*, int);  // Displaying segment type 't' not implemented
extern BOOL symbolSort();
extern void cllin();
extern void lhead();
extern void erout();

extern void write_lstbuf(char*);    
extern int HexFileFormat();
extern void p2device();
extern void InstructionInfo();

extern void ActivityMonitor(int);

// Forward declaration of functions included in this code module:
void EmitSymbolsTable();
void SegmentUsageInfo();

//-----------------------------------------------------------------------------
//
//                          EmitSymbolsTable
//
// Supress symbol table if errors
// Compute symbols per line - SYMLEN+3(type)+4(addr)+4(spaces)
// Calculate number of fields
// ;01234567890123456............
// ;abcdef t 1234   abcdef t 1234
// Find lowest symb (alpha-order)
//
void EmitSymbolsTable()
  {
  UCHAR _symType;
  int _symfld, _symlen;
  swsym = 1;               // skip ".LOC...OBJ...............LINE...SOURCE\n"

  newli(1);                // blank line after END directive

  if (nosym == 1) return;  // No symbol table appended in .LST file
  if (swlst == 0) return;  // No list

  // "USER SYMBOLS"        // Emit Headline
  write_lstbuf(symtx);     // Write .LST file
  newli(1);                // adjust line count

  // The symbols should be in alphabetical order
  // Create an alphabetically sorted symbol table
  if (AtmelFlag != 0) printf(" Creating Symbol Xref Table..."); // Atmel *.INC table, may take time
  symbolSort();
  if (AtmelFlag != 0) printf("\n");                             // Atmel *.INC table done   

  //----------------------------------------------------
  // Format and append the symbol table to the listing
  // Calculate start of next field (SYMLEN see equate.h)
  if (AtmelFlag != 0)
    {
    // Atmel AVR Fields = SYMLEN(40 chars) +1(space) +8(addr) +1(space) +1(type) +2(spaces);
    // ([0123456789012345678901234567890123456789 0123/01234567 t  ])
    _symlen = SYMLEN;            // Atmel's *.INC files require: 40 chars/symbol
    _symfld = _symlen+1+8+1+1+2; // pcc/pcd = 32bit
    }
  else
    {
    // Standard Fields = SYMLEN(20 chars) +1(space) +4(addr) +1(space) +1(type) +2(spaces);
    // ([01234567890123456789 0123/01234567 t  ])
    _symlen = SYMLEN/2;          // Truncated symbols: 20 chars/symbol are enough
    _symfld = _symlen+1+4+1+1+2; // pcc/pcd = 16bit
    }
  linel += 1+1+2;                // (eventually allowing type byte overscan)

  flabl[0] = TRUE;               // Init

  // Skip oper_predev area from symboltable (should be invisible)             //ha//
  symboltab_ptr  = symboltab;    // Init start of sorted symbols table  
  symboltab_ptr += predefCount;  // Hide any pre-defined symbols in symboltab //ha//

  while (symboltab_ptr->symString[0] !=0)
    {
    // Calculate number of fields per line (eventually allowing type byte overscan)
    pws = linel / _symfld;      // Calculate number of fields per line (rounded)
    pszListBuf = szListBuf;     // Init start of Listing line buffer

    for (_j=0; _j<pws; _j++)
      {
      // Print header if linct exhausted, re-init linct
      if (linct == 1) { linct=0; lhead(); linct+=1; }   //ha//

      // Transfer flabl[] <-- symbol[]. No more symbols - Skip
      symboltab_ptr->symString[_symlen] = 0;            //ha// Truncate symbol string
      if (symboltab_ptr->symString[0] != 0) 
        {        
        StrCpy(pszListBuf,symboltab_ptr->symString);    // Emit symbol

        // Extend short symbols with pattern '. . ';
        _k = strlen(symboltab_ptr->symString);          // Length of symbol string
        pszListBuf += _k;
        for (_i=(_k %2); _i<(_symlen-_k); _i++)
          {
          if (_i %2) *pszListBuf++  = ' ';
          else  *pszListBuf++  = '.';
          } // end for
        if (_i %2) *pszListBuf++ = SPACE;    // Tabulate
        *pszListBuf++ = SPACE;               // Add distance before symbol value

        // Emit Symbol/Label value as ascii chars
        if (AtmelFlag != 0)
          {
          if (swmodel == _WORD && symboltab_ptr->symType == _CODE)
            sprintf(pszListBuf, "%08X ", (UINT)(symboltab_ptr->symAddress/2));
          else
            sprintf(pszListBuf, "%08X ", (UINT)(symboltab_ptr->symAddress));
          pszListBuf += 8+1;      // avance ptr 8 ascii chars + 1 space (see sprintf(..))
          }
        else
          {
          sprintf(pszListBuf, "%04X ", (UINT)(symboltab_ptr->symAddress));
          pszListBuf += 4+1;      // avance ptr 4 ascii chars + 1 space (see sprintf(..))
          }

        // Clear .IFNDEF/#ifndef flag (see pass1.cpp, ifelse.cpp)
        // (all symbols within .IFNDEF/#ifndef condition have this flag set)
        _symType = (symboltab_ptr->symType &  ~SYMIFNDEF_FLAG); 
        if (_symType == _ABSEQU || _symType == _ABSSET)   sprintf(pszListBuf, "A  ");
        else if (_symType == _CODE)   sprintf(pszListBuf, "C  ");
        else if (_symType == _DATA)   sprintf(pszListBuf, "D  ");
        else if (_symType == _EEPROM) sprintf(pszListBuf, "E  ");

        pszListBuf += 1+2;        // avance ptr 1 ascii char + 2 spaces (see sprintf(..))

        symboltab_ptr++;          // next structre
        } // end if (symString[0] != 0)

      // Terminate the symbols line and skip to next line (if any)
      if (_j >= pws-1)
        {
        // Emit the terminated symbols line to ListBuf
        write_lstbuf(szListBuf);  // Write .LST file
        newli(1);                 
        } // end if (_j>=pws-1)
      } // end for (_j<pws)
    } // end while

  // Add CRLF before "ASSEMBLY COMPLETE" message if symbol line is incomplete
  if (_j != 0) newli(1);    
  
  SegmentUsageInfo();  // Always appended at end of listing //ha//
  } // EmitSymbolsTable

//-----------------------------------------------------------------------------
//
//                      NumberSort
//
void NumberSort(ORGSEG _segLayout[], int _maxCnt)
  {
  int swapFlag = TRUE;
  ULONG _tmpNr;

  while (swapFlag == TRUE)
    {
    _i=0; swapFlag = FALSE;

    // skip if 1st structure is empty (1st .ORG in src was > .sStart)
    if (_segLayout[0].sStart == 0 && _segLayout[0].sEnd == 0) _i++;  
    while (_i<_maxCnt)
      {
      if (_segLayout[_i].sStart > _segLayout[_i+1].sStart && _segLayout[_i+1].sEnd != 0)
        {
        swapFlag = TRUE;
        // Swap number
        _tmpNr = _segLayout[_i].sStart;
        _segLayout[_i].sStart = _segLayout[_i+1].sStart;
        _segLayout[_i+1].sStart = _tmpNr;

        _tmpNr = _segLayout[_i].sEnd;
        _segLayout[_i].sEnd = _segLayout[_i+1].sEnd;
        _segLayout[_i+1].sEnd = _tmpNr;
        }
      _i++;
      if (_segLayout[_i].sEnd == 0) break;
      } // end while (_i)
    } // end while (swapFlag)
  } // NumberSort

//-----------------------------------------------------------------------------
//
//                          CheckRangeOverlap
//
// Example 1 (not implmented):
// Error: Overlapping data segments:
//   data      :  Start = 0x0200, End = 0x02AB, Length = 0x00AC (172 bytes), Overlap=N
//   data      :  Start = 0x0200, End = 0x0206, Length = 0x0007 (7 bytes), Overlap=N
// Please check your .ORG directives !
//
// Example 2a (implmented, console display):
// >>>_XASMAVR_test.asm(2298): ERROR - ORG directive misplaced (segment overlap)
//         CSEG: Start = 0x00000A00  End = 0x00000A30  Size = 48 byte(s)
//         CSEG: Start = 0x00000A10  End = 0x00000A20  Size = 16 byte(s)
//
// Example 2b (implmented, info at end of listing):
//  Info - Memory segments organization
//         CSEG: Start = 0x00000A00  End = 0x00000A30  Size = 48 byte(s)
// >>>_XASMAVR_test.asm: ERROR - ORG directive misplaced (segment overlap)
//         CSEG: Start = 0x00000A10  End = 0x00000A20  Size = 16 byte(s)
//
// Example 2c (implmented, info at end of listing):
//  Info - Memory segments organization (.OVERLAP)
//         CSEG: Start = 0x00000A00  End = 0x00000A30  Size = 48 byte(s)
//         CSEG: Start = 0x00000A10  End = 0x00000A20  Size = 16 byte(s)
//         CSEG: Start = 0x00008000  End = 0x00008010  Size = 16 byte(s)
//         ...
//
// Example 3 (implmented, .DEVICE ATtiny3224):
//         CSEG: Start = 0x0000FFFE  End = 0x0001000E  Size = 16 byte(s)
// >>>_XASMAVR_test.asm: ERROR - Segment address out of range : 0x8000 < Limit < 0xFFFF
//
// typedef struct tag_ORGSEG {
//   ULONG     sStart;         
//   ULONG     sEnd;         
// } ORGSEG, *LPORGSEG;
//
void CheckRangeOverlap(LPORGSEG segLayoutPtr, char* _infoStr,
                       int _div, int _segType, ULONG _memStart, ULONG _memSize)
  {
  char* segString = NULL;
  ULONG _sSize, totalSize;
  int _i, _maxOrg;          // Need "_i" locally (erout() destroys "global _i")

  switch(_segType)
    {
    case _CODE:
      segString = "CSEG: Code";
      _maxOrg = ORGMAX_C;
      break;
    case _DATA:
      segString = "DSEG: Data";
      _maxOrg = ORGMAX_D;
      break;
    case _EEPROM:
      segString = "ESEG: Data";
      _maxOrg = ORGMAX_E;
      break;
    default:
      break;
    } // end switch

  if (segLayoutPtr->sStart == 0 && segLayoutPtr->sEnd == 0) segLayoutPtr++;

  _i=0; totalSize=0;
  while (segLayoutPtr->sEnd != 0 && _i < _maxOrg)
    {
    _sSize = segLayoutPtr->sEnd - segLayoutPtr->sStart;
    totalSize += _sSize;

    sprintf(pszScratchbuf, _infoStr, segLayoutPtr->sStart/_div, segLayoutPtr->sEnd/_div, _sSize/_div);
    // Emit to listfile
    LstFile.write(pszScratchbuf, strlen(pszScratchbuf));

    // Check if any segment blocks overlap
    if ((segLayoutPtr+1)->sStart < segLayoutPtr->sEnd &&
        (segLayoutPtr+1)->sEnd != 0 &&
        swoverlap == 0 &&
        ercnt == 0)
      {
      printf(pszScratchbuf);       // Display on console
      errorp2(ERR_ORGINS, NULL);
      erout();
      _sSize = (segLayoutPtr+1)->sEnd - (segLayoutPtr+1)->sStart;
      sprintf(pszScratchbuf, _infoStr, (segLayoutPtr+1)->sStart/_div, (segLayoutPtr+1)->sEnd/_div, _sSize/_div);
      LstFile.write(pszScratchbuf, strlen(pszScratchbuf));
      printf(pszScratchbuf);
      break;
      } // end if (overlap)

    // Error if segment is out of range
    else if (segLayoutPtr->sStart >= (_memStart+_memSize) ||
             segLayoutPtr->sStart < _memStart             ||
             segLayoutPtr->sEnd   > (_memStart+_memSize))
      {
      printf(pszScratchbuf);       // Display on console
      sprintf(pszErrtextbuf,"0x%X < Limit < 0x%X", _memStart, _memStart+_memSize-1); 
      errorp2(ERR_SEGADDR, pszErrtextbuf);
      erout();
      break;
      }

    segLayoutPtr++; _i++;
    } // end while
  
  sprintf(pszScratchbuf, "        %s size = %ld bytes\n\n", segString, totalSize);
  LstFile.write(pszScratchbuf, strlen(pszScratchbuf));
  } // CheckRangeOverlap

//-----------------------------------------------------------------------------
//
//                      SegmentUsageInfo
//
// typedef struct tag_ORGSEG {
//   ULONG     sStart;         
//   ULONG     sEnd;         
// } ORGSEG, *LPORGSEG;
//
void SegmentUsageInfo()
  {
  int _i;  // Need this locally (erout() destroys global _i)

  char* infoSEG   = " Info - Memory segments organization";
  char* infoOVL   = " (.OVERLAP)\n";

  char* infoCSEGw = "        CSEG: Start = 0x%08X  End = 0x%08X  Size = %ld word(s)\n";
  char* infoCSEGb = "        CSEG: Start = 0x%08X  End = 0x%08X  Size = %ld byte(s)\n";
  char* infoDSEG  = "        DSEG: Start = 0x%08X  End = 0x%08X  Size = %ld byte(s)\n";
  char* infoESEG  = "        ESEG: Start = 0x%08X  End = 0x%08X  Size = %ld byte(s)\n";

  ULONG _sSize, totalSize;

  if (AtmelFlag == FALSE) return;          // only for XASMAVR

  LstFile.write(infoSEG, strlen(infoSEG)); // Emit to listfile
  if (swoverlap) LstFile.write(infoOVL, strlen(infoOVL));
  else LstFile.write("\r\n", 2);

  //  ------ 
  // | CSEG |
  //  ------
  csegLayout_ptr->sEnd = pcc;
  NumberSort(csegLayout, ORGMAX_C);
  if (swmodel == _WORD)
    CheckRangeOverlap(csegLayout, infoCSEGw, _WORD, _CODE, RomStart, RomSize);
  else // swmodel == _BYTE
    CheckRangeOverlap(csegLayout, infoCSEGb, _BYTE, _CODE, RomStart, RomSize);

  //  ------ 
  // | DSEG |
  //  ------
  dsegLayout_ptr->sEnd = pcd; 
  NumberSort(dsegLayout, ORGMAX_D);
  CheckRangeOverlap(dsegLayout, infoDSEG, _BYTE, _DATA, SRamStart, SRamSize);

  //  ------ 
  // | ESEG |
  //  ------
  esegLayout_ptr->sEnd = pce; 
  NumberSort(esegLayout, ORGMAX_E);
  CheckRangeOverlap(esegLayout, infoESEG, _BYTE, _EEPROM, EEPromStart, EEPromSize);
  } // SegmentUsageInfo

//-----------------------------------------------------------------------------
//
//                      finish
//
// give end record to the absolute hex-file
// write symbol table
// close all files, and exit to operating system
//
void finish()
  {
  ////////////////////////
  ActivityMonitor(OFF); //
  ////////////////////////

  // Write .HEX file
  // Intel Hex-file EOF record (termination record)
  if (HexFileFormat() == INTEL) HexFile.write(hxeof, strlen(hxeof));     
  // Motorola Srec-File start record (termination record)
  else HexFile.write(srstart, strlen(srstart));  
  errchk(szHexFileName, GetLastError());      // Check System error

  // Atmel-AVR Write EEP.HEX file (Intel Hex-file EOF record)
  if (AtmelFlag != 0)
    {
    EEPHexFile.write(hxeof, strlen(hxeof));   
    errchk(szEEPHexFileName, GetLastError()); // Check System error
    EEPHexFile.close();                       // Close EEP.HEX file
    errchk(szEEPHexFileName, GetLastError()); // Check System error
    }
  // Write Flash PROM binary data file .BIN
  if (pszBinFilebuf != NULL)
    {
    BinFile.write(pszBinFilebuf, BinFilesize);// Emit pszBinFilebuf 
    errchk(szBinFileName, GetLastError());    // Check System error
    BinFile.close();                          // Close .BIN file
    errchk(szBinFileName, GetLastError());    // Check System error
    GlobalFree(pszBinFilebuf);                // Free .BIN-file buffer
    }
 
  CloseSrcFile();                             // Close .SRC file
  HexFile.close();                            // Close .HEX file
  errchk(szHexFileName, GetLastError());      // Check System error

  if (nosym == 2) EmitSymbolsTable(); // .SYMBOLS also lists symbols if errors (debug)

  if (ercnt != 0)  // "\nASSEMBLY COMPLETE  ***     ERROR(S),  (      )\n"
    {
    sprintf(&txerr[23], "%4d", ercnt); // Patch total number of errors
    txerr[23+4] = ' ';                 // Remove 0-terminator
    // Patch line number where the 1st error occurred
    //sprintf(&txerr_buf[40], "%5d)\r\n", (int)(p1ert[0] | (p1ert[1] << 8)));
    sprintf(&txerr[40], "%5d)\r\n", errline1);

    // Listing Message - Errors
    write_lstbuf(txerr);           // Write .LST file
    // Console Message - Errors
    printf(txerr);

    DeleteFile(szHexFileName);     // no errchk!
    DeleteFile(szEEPHexFileName);  // Atmel: no errchk!
    DeleteFile(szBinFileName);     // no errchk!
    } // end if

  else // "\nASSEMBLY COMPLETE,   NO ERRORS\n"
    {
    // Only displayed on console
    if (!(swmodel & _NOINFO))      // Atmel AVR only
      {
      if (swpragma == 0) p2device(); 
      InstructionInfo();
      }       

    // Append the sorted symbols table  at end of .LST file
    //  only if no errors in source file

    if (nosym != 2) EmitSymbolsTable();  

    // Listing Message - No Errors 
    for (_i=0; _i<strlen(errtx); _i++)  
      {                                 
      *pszListBuf = errtx[_i];
      *pszListBuf++;
      }
    write_lstbuf(szListBuf);       // Write .LST file

    // Console Message - No Errors
    printf("\n");                  // Add a blank line (Console)
    printf(errtx);      
    }  // end else

  LstFile.close();

  if (swlst == 0)
    DeleteFile(szLstFileName);     // no errchk!
  } // finish

//-----------------------------------------------------------------------------

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("pszBinFilebuf=%08X  RomSize=%08X (%d)  binPadByte=%02X\nrequiredPROMSize=%08X",
//ha//        pszBinFilebuf,      RomSize, RomSize,  (UCHAR)binPadByte, requiredPROMSize);
//ha//printf("pszBinFilebuf ");
//ha//DebugPrintBuffer(pszBinFilebuf, 255);
//ha//DebugStop(1, "finish()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//for (_i=0; _i<13; _i++)
//ha//  {
//ha//  printf("csegLayout[%d].sStart=%08X  csegLayout[%d].sEnd  =%08X\n",
//ha//          _i,csegLayout[_i].sStart, _i,csegLayout[_i].sEnd);
//ha//  }
//ha//DebugStop(2, "SegmentUsageInfo()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//for (_i=0; _i<13; _i++)
//ha//  {
//ha//  printf("_segLayout[%d].sStart=%08X  _segLayout[%d].sEnd  =%08X\n",
//ha//          _i,_segLayout[_i].sStart, _i,_segLayout[_i].sEnd);
//ha//  }
//ha//DebugStop(2, "NumberSort()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//DebugPrintSymbolArray(2000);  
//ha//DebugStop(1, "finish()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//--------------------------end-of-c++-module-----------------------------------

