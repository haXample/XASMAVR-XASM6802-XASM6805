// haXASM - Cross-Assembler for 8bit Microprocessors
// haXASM.cpp - C++ Developer source file.
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

//******************************************************************************
//*        -----------------------------------------------                     *
//*       |   C R O S S - A S S E M B L E R  Version 2.1  |                    *
//*        -----------------------------------------------                     *
//* PURPOSE: Cross-Assembler for ISIS-II & MS-DOS Systems                      *
//*           re-written from 8085 src-code, running under MS-DOS > 3.20       *
//*           re-written running under Windows XP 32bit or greater             *
//*          .LST-file output                                                  *
//*          .HEX-file output (Intel)                                          *
//*          .S19-file output (Motorola)                                       *
//*          .BIN-File output (PROM-programming)                               *
//*                                                                            *
//* AUTHOR:  helmut altmann,   (C)Copyright 1980,1990   Siemens AG.            *
//*                            (C)Copyright 2023 by ha.                        *
//*                                                                            *
//* ABSTRACT: The X-Assembler was originally written for Intel ISIS-II systems *
//*           running on the 8085. To make the XASM run on an IBM-XT/AT PC     *
//* under MS-DOS operating system the following steps were performed:          *
//* The 8085 source has been converted into 8086 compatible source using the   *
//* Intel tool "CONV86" on an AT-PC DOS 3.20 system under "XIRUN" environment. *
//* Then the ISIS-II system-macros had to be re-written according to the MS-DOS*
//* V3.20 conventions. After the correct functionality had been verified,      *
//* further optimizations were applied to the 8086 source including the        *
//* implementation of the UPI-41/42 instruction set and some helpful functions *
//* for expression evaluation. Previously this XASM had been designed to       *
//* assemble Motorola 6800/6802 code on an ISIS-II system.                     *
//* Finally the old x86 assembler source code was transscribed into  32bit C   *
//* (compiled with Microsoft C++ compiler) as a Windows console application.   *
//* Next step should port the C-source to C++.                                 *
//*                                                                            *
//* Version 2.0 and future updates of XASM now run under Windows console.      *
//* Version 2.1 of XASMAVR includes conditional assembly and macros.           *
//*                                                                            *
//;* Currently the following derivations are tested and available:             *
//;*  XASM8042 - Cross-Assembler for Intel UPI-8041/8042, UPI-C42              *
//;*  XASM6805 - Cross-Assembler for Motorola MC68HC05 (all types)             *
//;*  XASM6802 - Cross-Assembler for Motorola MC6800/6802                      *
//;*  XASMAVR  - Macro-Assembler for Atmel/Microchip AVR uC family             *
//;*                                                                           *
//******************************************************************************

#include <sys\stat.h>  // For _open( , , S_IWRITE) needed for VC 2010
#include <fcntl.h>     // File Modes

#include <io.h>        // File open, close, access, etc.
#include <conio.h>     // For _putch(), _getch() ..
#include <string>      // printf, etc.

#include <shlwapi.h>   // Library shlwapi.lib for PathFileExistsA

#include <sys/stat.h>  // For filesize
#include <iostream>    // I/O control
#include <fstream>     // File control
#include <shlwapi.h>   // Library shlwapi.lib for PathFileExistsA

#include <windows.h>   // For console specific functions

#include "equate.h"
#include "extern.h"    // Variables published in workst.cpp

using namespace std;

// Global variables
char* szErrorOpenFailed    = "ERROR %s: FILE OPEN FAILED.\n";
char* szErrorFileExists    = "ERROR %s: FILE ALREADY EXISTS.\n";  // [] vs * ?!
char* szErrorIllegalOption = "Illegal or misspelled option";
char* szErrorCommandSyntax = "Command syntax error\n";
char* szErrorSourceFileASM = "ERROR: Unrecognizable source file. <filename.ASM> expected.\n";

char szSrcFileName[PATHLEN];    // e.g = "File.ASM"
char szLstFileName[PATHLEN];    // e.g = "File.LST"
char szHexFileName[PATHLEN];    // e.g = "File.HEX"
char szBinFileName[PATHLEN];    // e.g = "File.BIN"
char szEEPHexFileName[PATHLEN]; // e.g = "File.EEP.HEX"
char szIclFileName[PATHLEN];    // Current inlude file name (*.INC)

char szCmdlineSymbol[SYMLEN+1] = {0}; // Symbol defined via commandline

char* pszCurFileName;        // File currently read e.g = "File.ASM / file.INC"

ifstream SrcFile;            // Filestream read  (*.ASM)
ofstream LstFile;            // File write (*.LST)
ofstream HexFile;            // File write (*.HEX)
ofstream EEPHexFile;         // File write (*.EEP.HEX)
ofstream BinFile;            // File write (*.BIN)
streampos pos;               // File seek only

// Extern variables and functions
extern void DebugStop(int, char*, char*); // Usage: DebugStop(testNr, "Function()", __FILE__);
extern void DebugPrintBuffer(char*, int); // Usage: DebugPrintBuffer(buffer, count);

extern void ListDeviceAVR();
extern void assgn();
extern void pass1();
extern void reopn();
extern void pass2();
extern void finish();
extern void FreeBuffers();

//;******************************************************************************
//;*                                                                            *
//;*                           haXASM - main                                    *                      
//;*                                                                            *
//;*               -----------------------------------                          *
//;*              |   C R O S S - A S S E M B L E R   |                         *
//;*               -----------------------------------                          *
//;*                                                                            *
//;* Currently the following derivations are tested and available:              *
//;*  XASM8042 - Cross-Assembler for Intel UPI-8041/8042, UPI-C42               *
//;*  XASM6805 - Cross-Assembler for Motorola MC68HC05 (all types)              *
//;*  XASM6802 - Cross-Assembler for Motorola MC6800/6802                       *
//;*  XASMAVR  - Macro-Assembler for Atmel/Microchip AVR uC                     *
//;*                                                                            *
//;* Other instruction sets can easily be brought in by rewriting the specific  *
//;* modules: OPCODE*.cpp, BYTVAL*.cpp All other modules could possibly remain  *
//;* unchanged.                                                                 *
//;*                                                                            *
//;******************************************************************************
//
int main(int argc, char *argv[])
  {
  char* _ptrFiletype = NULL;

  // Copy input XASM-filename (looks nicer in help-text if toupper)
  for (_i=0; _i<=strlen(argv[0]); _i++)        
    szSrcFileName[_i] = toupper(argv[0][_i]);        

  // Get filename only, discarding extension
  pszCurFileName = PathFindFileName(szSrcFileName);       
  _ptrFiletype = PathFindExtension(pszCurFileName);
  pszCurFileName[strlen(pszCurFileName)-strlen(_ptrFiletype)] = 0; 
  
  // Atmel AVR is special                                                               
  if (StrCmpI(pszCurFileName, "XASMAVR")    == 0 ||
      StrCmpI(pszCurFileName, "XASMAVR_64") == 0)   AtmelFlag = 1;
  
  // Display signon, help and usage
  if (argc <= 1) {
    printf("Run \x22%s /?\x22 for more info\n", pszCurFileName); exit(0); }
  
  else if (argc <= 1 || strcmp(argv[1], "/?") == 0) {
    printf(signon);
    printf(  "%s [/options] srcfile.asm | srcfile.s\n"
             "  /Bp0xFF    Generate a .BIN-File with padding=0xFF for unused gaps\n"
             "  /Bp0x00    Generate a .BIN-File with padding=0x00 for unused gaps\n"
             "  /AmOFF     Suppress activity monitor display\n"
             "  /D<symbol> Define text symbol\n", pszCurFileName); 
    if (AtmelFlag != 0) {
      printf("  /Mb        .MODEL BYTE: List byte addresses in code segment\n"
             "  /Mw        .MODEL WORD: List word addresses in code segment\n"
             "  /S         .SYMBOLS: Append symbol map in listing\n"
             "  /d         .DEVICE: List the supported AVR devices on console\n"); }
    
    printf("\n  Note: [/options] are case-sensitive.\n");
    exit(0);
    } // end else if

  // "XASMAVR /d"
  int _b=0, _h=0, _m=0, _s=0, _d=0, _D=0;    // Valid option counters

  if (argc == 2       &&
      AtmelFlag != 0  && 
      (strcmp(argv[1], "/d") == 0 ||
       strcmp(argv[1], "-d") == 0))
    { ListDeviceAVR();  _d++; exit(0); }

  // "XASM srcfile.asm" or "XASM srcfile.s"
  else if (argc == 2                      &&
           StrStrI(argv[1], ".asm") == NULL  &&
           StrStrI(argv[1], ".s")   == NULL) 
    { printf(szErrorCommandSyntax); exit(SYSERR_FNAME); }

  // Check supported options (case sensitive!)
  // "XASM [/options] srcfile.asm"
  else if (argc > 2)
    {
    for (_i=1; _i<=(argc-1); _i++)
      {
      // Binary padding unused gaps with 0x00 in .BIN-file
      if (strcmp(argv[_i], "/Bp0x00") == 0 ||
          strcmp(argv[_i], "-Bp0x00") == 0) 
        { binPadByte = 0x00; _b++; }                

      // Binary padding unused gaps with 0xFF in .BIN-file
      else if (strcmp(argv[_i], "/Bp0xFF") == 0 ||
               strcmp(argv[_i], "-Bp0xFF") == 0)  
        { binPadByte = 0xFF; _b++; }

      // Disable Hour-glas activity monitor
      else if (strcmp(argv[_i], "/AmOFF") == 0 ||
               strcmp(argv[_i], "-AmOFF") == 0)  
        { swActivityMonitor = OFF; _h++;}

      // Atmel AVR relevant only
      // Byte addresses in listing
      else if (strcmp(argv[_i], "/Mb") == 0 ||
               strcmp(argv[_i], "-Mb") == 0)        
        { cmdmodel = _BYTE; swmodel =_BYTE; _m++; }

      // Word addresses in listing
      else if (strcmp(argv[_i], "/Mw") == 0 ||
               strcmp(argv[_i], "-Mw") == 0)        
        { cmdmodel = _WORD; swmodel =_WORD; _m++; } 
      
      // Symbol map in listing
      else if (stricmp(argv[_i], "/S") == 0 ||
               stricmp(argv[_i], "-S") == 0)         
        { cmdnosym = 2; _s++;}                      

      // Define symbol
      else if (StrCmpN(argv[_i], "/D",2) == 0 ||
               StrCmpN(argv[_i], "-D",2) == 0)
        {
        // Microsoft style: No space between option '/D' and <symbol> = '/DSYMBOL'
        if (strlen(argv[_i]) == 2) _D++;  // missing symbol
        // Truncate symbol string
        else if (strlen(argv[_i]) > SYMLEN) argv[_i][SYMLEN] = 0; 
        StrCpy(szCmdlineSymbol, &argv[_i][2]);
        _D++;
        }
  
      // Checking illegal options 
      if (_b>1 || _h>1 || _m>1 || _s>1 || _D>1 ||
          // XASMAVR specific options only supported with Atmel syntax.
          ((_m !=0 || _d !=0) && AtmelFlag == FALSE))        
        {  
        printf("%s: %s\n", szErrorIllegalOption, &argv[_i][0]); exit(SYSERR_CMDLINE);
        } //end else if
      } // end for
    } // end if (argc > 2)

  // argc >= 2
  // Copy input SRC-filename (.ASM / .S).
  // Source filename must be last argv[argc-1]
  StrCpy(szSrcFileName, argv[argc-1]);

  _ptrFiletype = PathFindExtension(szSrcFileName);
  if (stricmp(_ptrFiletype, ".asm") != 0 && stricmp(_ptrFiletype, ".s") != 0)
    {
    printf(szErrorSourceFileASM);
    exit(SYSERR_FNAME);
    }
 
  // ------------------------------
  // Perform the assembling process
  //
  assgn();       // Assign switches, defaults and allocate buffers
  pass1();       // Do the pass1
  reopn();       // Rescan .SRC, open .LST and .HEX files
  pass2();       // Do the pass2
  finish();      // CLose files and write symbol table

  FreeBuffers(); // Free allocated buffers
  exit(0);       // Normal Exit to Operating System
  } // main           

//--------------------------end-of-c++-module-----------------------------------






