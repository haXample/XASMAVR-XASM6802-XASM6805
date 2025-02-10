// haXASM - Cross-Assembler for 8bit Microprocessors
// pass2.cpp - C++ Developer source file.
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

#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>  // For filesize
#include <iostream>    // I/O control
#include <fstream>     // File control

#include <windows.h>   // For console specific functions

#include "equate.h"
#include "extern.h"    // Variables published in workst.cpp

using namespace std;

// Global variables
char* pragmaPartinc   = "partinc";
char* pragmaAVRPART   = "AVRPART";
char* pragmaPROGFLASH = "MEMORY PROG_FLASH";
char* pragmaEEPROM    = "MEMORY EEPROM";
char* pragmaSRAMSIZE  = "MEMORY INT_SRAM SIZE";
char* pragmaSRAMADDR  = "MEMORY INT_SRAM START_ADDR";

// Extern variables and functions
extern void DebugStop(int, char*, char*); // Usage: DebugStop(testNr, "Function()", __FILE__);
extern void DebugPrintBuffer(char*, int); // Usage: DebugPrintBuffer(buffer, count);
extern void DebugPrintSymbolArray(int);   // Usage: DebugPrintSymbolArray(count);

extern char* szErrorOpenFailed; 
extern char* szErrorFileExists; 

extern char szLstFileName[];
extern char szHexFileName[];
extern char szSrcFileName[];
extern char szBinFileName[];
extern char szEEPHexFileName[];           // e.g = "File.EEP.HEX"
extern char szIclFileName[];
extern char* pszCurFileName;

extern char* TITLE_str;
extern char* SUBTL_str;
extern char* PW_str;
extern char* PL_str;
extern char* NOLST_str;
extern char* NOSYM_str;
extern char* ICL_str;
extern char* EJECT_str;
extern char* LST_str;
extern char* END_str;
//extern char* DB_str;
extern char* _defin_str;
extern char* EVEN_str;

extern void OpenSrcFile(char*);
extern void CloseSrcFile();
extern void CloseIclFile();
extern BOOL OpenIncludeFile(char*, char*);

extern void read_source_line();
extern ifstream SrcFile;    // Filestream read  (*.ASM)
extern ifstream IclFile;    // Filestream read  (*.INC)
extern ofstream LstFile;    // Filestream write (*.LST)
extern ofstream HexFile;    // Filestream write (*.HEX)
extern ofstream EEPHexFile; // Filestream write (*.EEP.HEX)
extern ofstream BinFile;    // Filestream write (*.BIN)

extern UCHAR labfn(char*);

extern int opcod();
extern UINT expr(char*);

extern void lineAppendCRLF(char*);
extern void linut(char*);
extern void Decomp();
extern void p2lab();
extern void edipc(), clepc();
extern void edpri(), edpri_db(), edpri_dw(), edpri_dd(), edpri_dq(); 

extern void ins0(),  ins2(),  ins3(),  ins4(),  ins5(),  ins6();
extern void ins7(),  ins8(),  ins9(),  ins10(), ins11(), ins12();
extern void ins13(), ins14(), ins15(), ins16(), ins17(), ins18();
extern void ins19(), ins20(), ins21(), ins22(), ins23(), ins24();
extern void ins25(), ins26(), ins27(), ins28(), ins29(), ins30();
extern void ins31(), ins32(), ins33();

extern BOOL errorp1();
extern void errorp2(int, char*);

extern void wrhex(int), cllin(), dw_endian(), dd_endian(), dq_endian();

extern void erout();
extern void warnout();
extern void errchk(char*);
extern void eddh(char*, ULONG);
extern void edwh(char*, UINT);
extern void edbh(char*, UINT);
extern void AssignPc();
extern void GetPcValue();
extern void titleXfer(char*, char*, char*), lhead();

extern char* skip_leading_spaces(char*);
extern char* skip_trailing_spaces(char*);
extern BOOL delm2(char*);
extern void replace_token(char*);

// Conditional assembly and pre-processing functions
extern void ClearIfdefStack();
extern BOOL p12Cundef(int);
extern void p12Cifdef(int);
extern void p12Cifndef(int);
extern void p12Cif(int);
extern void p12Celse(int);
extern void p12Celseif(int);
extern void p12Cendif(int);
extern void p12CCheckIfend();
extern int p12CifelseSkipProcess(int);

// Macro assembler functions
extern void p12macro(int);
extern void p12DefineMacro(int);
extern BOOL p12ExpandMacro(int);
extern void p12CheckEndmacro();
extern void p12endmacro();

extern void SyntaxAVRtoMASM(int); // For Atmel's syntax
extern BOOL label_syntax(char*);

extern void ActivityMonitor(int);

// Forward declaration of functions included in this code module:
//void p2ext();
//void p2pub();
void p2org();
void p2dq();
void p2dd();
void p2dw();    // Motorola: FDB
void p2db();    // Motorola: FCB
void p2ds();    // Motorola: RMB
void p2end();
void p2equ();
void p2set();   // Atmel specific

void p2device();
void p2pragma(char*);

BOOL p2pse();                              
void reopf();

//-----------------------------------------------------------------------------
//
//                      reopen
//
// Reopen source file
// start hex-file output, reset counters nad switches
// prepare list-file output
//
void reopn()
  {
  reopf();
  
  // Clear structured ifdef buffer
  ClearIfdefStack();

  // Init-clear global symUndefBuf 
  for (_i=0; _i<SYMUNDEFBUFSIZE; _i++) symUndefBuf[_i] = 0;
  // Init segment values
  pcc     = RomStart;           
  pcd     = SRamStart;          
  pce     = EEPromStart;
  
  // Init hxpc (Code segment)
  hxpc = RomStart;
  // Atmel:  Init eehxpc (EEPROM segment)
  eehxpc = EEPromStart;

  // Reset counters, pointers and Flags
  errSymbol_ptr = NULL;

  srcEOF  = FALSE;     
  iclEOF  = FALSE;     
  ercnt   = 0;     // Only 0..255
  SegType = _CODE; // Assume currently CSEG

  _errCSEG=0, _errDSEG=0, _errESEG=0; // Segment range error counter

  _lstline  = 0;   // Clear .LST line counter
  _curline  = 0;   // Clear current file line counter

  macroBegin = 0;  // Reset .MACRO flag
  } // reopen

//------------------------------------------------------------------------------
//
//                         reopf
//
//  Reopen source-file, open .LST-file, .HEX-file and .BIN-file
//
void reopf()
  {
  // lose and re-open input .SRC text-file (text-mode)
  CloseSrcFile();
  OpenSrcFile(szSrcFileName); 

  // Delete current .LST and open new output ansi-lst file (standard mode)
  DeleteFile(szLstFileName);  // no errchk!

  LstFile.open(szLstFileName, ios::out | ios::binary | ios::app); 
  if (!LstFile)
    {
    printf(szErrorOpenFailed, szLstFileName);
    exit(SYSERR_FROPN);
    }

  // Delete current .BIN and open new output binary .Bin file (standard mode)
  DeleteFile(szBinFileName);  // no errchk
  if ((binPadByte & 0xFF) == 0xFF || binPadByte == 0x00)    
    {
    BinFile.open(szBinFileName, ios::binary | ios::out);
    if (!BinFile)
      {
      printf(szErrorOpenFailed, szBinFileName);
      exit(SYSERR_FROPN);
      }
    }

//ha//  if (_access(szHexFileName, 0) == 0)  // Check if file already exists
//ha//    {                               
//ha//    printf(szErrorFileExists, szHexFileName);   
//ha//    exit(SYSERR_FACCESS);
//ha//    }                               

  // Delete current .HEX and open new output ascii-hex file (append-mode)
  DeleteFile(szHexFileName);  // no errchk
  HexFile.open(szHexFileName, ios::out | ios::app);
  if (!HexFile)
    {
    printf(szErrorOpenFailed, szHexFileName);
    exit(SYSERR_FROPN);
    }

  if (AtmelFlag != 0)
    {
    // Intel Hex-file 02 Xaddr hxrec02 not yet emmitted (swhxrec02=0)  //ha//
    swhxrec02 = 0;                                                     //ha//
    // Delete current EEP.HEX and open new output ascii-hex file (append-mode)
    DeleteFile(szEEPHexFileName);  // no errchk
    EEPHexFile.open(szEEPHexFileName, ios::out | ios::app);
    if (!EEPHexFile)
      {
      printf(szErrorOpenFailed, szEEPHexFileName);
      exit(SYSERR_FROPN);
      }
    } // end if (AtmelFlag)
  } // reopf

//-----------------------------------------------------------------------------
//
//                      pass2
//
// source is read until 'END' statement
// .LST and .HEX-file output is produced
//
void pass2()
  {
  _debugSwPass = _PASS2; // Flag indicates processing Pass2
  swpass = _PASS2;       // Flag indicates processing Pass1

PASS2:
  Decomp();
  edipc();         // Edit current program counter

  opcod(); // Load insv[1], ins_group, flabl, fopcd, oper1..4, ..

  ////////////////////////////////////////////////////////////////////////////////
  //                                                                            //
  // Macro processing Pass2: Macro definition                                   //
  //                                                                            //
  if (swdefmacro)                                                               //
    {                                                                           //
    clepc();                 // Space out LOC in Listing                        //
    edpri();                 // Emit the src line only                          //
    p12DefineMacro(_PASS2);  // Macro definitions are not processed in Pass2    //
    goto PASS2;              // Macro lines are treated like comments in Pass2  //
    }                                                                           //
  ////////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////////
  //                                                                            //
  // Conditionally suppressed via .IF .IFDEF .IFNDEF .EL(SE)IF .ELSE .ENDIF     //
  // Conditionally suppressed via #if #ifdef #ifndef #el(se)if #else #endif     //
  //                                                                            //
  int _rsltIfElse = p12CifelseSkipProcess(_PASS2); // Get .IF-ELSE condition    //
  if (_rsltIfElse == TRUE) goto PASS2;             // Skip src line             //
  ////////////////////////////////////////////////////////////////////////////////
 
  // Normal unconditional processing
  else
    {
    // Skip comment line
    if (swcom != 0)  
      {
      clepc();
      edpri();       // emit the src line only
      goto PASS2;    // skip and get next src line
      }

    errorp1();       // Check for Pass1 errors

    p2lab();         // evaluate symbols and labels  //ha//

    // Check opcode and label
    if (fopcd[0] == 0)          // Check if any opcode present
      {
      if (flabl == 0)           // Dont clear pc if label present
        {
        errorp2(ERR_ILLOPCD, NULL); // ..must be something illegal
        clepc();                    // Opcode empty and label empty
        edpri();
        }
      else edpri();                 // Emit the label only
      goto PASS2;                   // Next line if opcode is empty.             
      }

    // GROUP 0, 1, 2..32: PROCESSOR INSTRUCTIONS
    // (See Module: BYTVAL.ASM)
    errStatus = 0;

    switch (ins_group & ~(0x80 | 0x40))   
      {
      case 0:         // Group 0: illegal instruction
        ins0();       
        break;
      case 1:         // Group 1: pseudo instructions
        if (p2pse() == FALSE) return;  // "END" Pass 2 completed - finished
        break;
      case 2:         // Group 2
        ins2();       
        break;
      case 3:         // Group 3
        ins3();       
        break;
      case 4:         // ...
        ins4();       
        break;
      case 5:
        ins5();
        break;
      case 6:
        ins6();
        break;
      case 7:
        ins7();
        break;
      case 8:
        ins8();
        break;
      case 9:
        ins9();
        break;
      case 10:
        ins10();
        break;
      case 11:
        ins11();
        break;
      case 12:
        ins12();
        break;
      case 13:
        ins13();
        break;
      case 14:
        ins14();
        break;
      case 15:
        ins15();
        break;
      case 16:
        ins16();
        break;
      case 17:
        ins17();
        break;
      case 18:
        ins18();
        break;
      case 19:
        ins19();
        break;
      case 20:
        ins20();
        break;
      case 21:
        ins21();
        break;
      case 22:
        ins22();
        break;
      case 23:
        ins23();        
        break;
      case 24:
        ins24();
        break;
      case 25:
        ins25();
        break;
      case 26:
        ins26();
        break;
      case 27:
        ins27();
        break;
      case 28:
        ins28();
        break;
      case 29:
        ins29();
        break;
      case 30:
        ins30();
        break;
      case 31:          // ...
        ins31();
        break;
      case 32:          // Group 32
        ins32();
        break;
      case 33:          // Group 33
        ins33();
        break;

      // ..must be a Macro or something illegal
      default:
        if (p12ExpandMacro(_PASS2) == TRUE)
          {
          // Emit MACRO line marker on macro statement
          szListBuf[lpMACR] = MACRO_IDM;  // '+'
          // .LST control: 0x80 Flag that this is the macro statement (not the body)
          // that should always show up in the list file (disregarding .NOLISTMAC)
          swlistmac |= 0x80;
          edpri();
          } // end if (p12ExpandMacro)

        // Illegal directive
        else if (fopcd[0] == '.' || fopcd[0] == '#')
          {
          errorp2(ERR_ILLDIR, fopcd);
          edpri();
          }

        // Unknown instruction (=ERR_ILLOPCD)
        else ins0();
                 
        break;  // break default:
      } // end switch (ins_group)
    } // end else (Normal unconditional processing)

  goto PASS2;         // Next line
  } // pass2

//-----------------------------------------------------------------------------
//
//                               p2pse
//
// Input: errSymbol_ptr
//
BOOL p2pse()
  {
  char* _dirStr;
  _dirStr = skip_leading_spaces(inbuf);
  
  // Check some directive syntax
  // Label and conditional directives should be in the same line
  // Example: _lTest: .IF (_cTest == 1) is a wrong syntax, --> but allowed by Atmel's assembler
  // Example: _lTest: .IFDEF (_cTest == 1) is a wrong syntax, not allowed
  // Example: _lTest: .IFNDEF (_cTest == 1) is a wrong syntax, not allowed
  // Example: _lTest: .ELIF (_cTest == 1) is a wrong syntax, not allowed
  // Example: _lTest: .ELSE (_cTest == 1) is a wrong syntax, not allowed
  // Example: _lTest: .ENDIF (_cTest == 1)  is a wrong syntax, not allowed
  if (flabl[0] != 0 &&
      (StrCmpNI(fopcd, ".IFN", 4) == 0 ||
       StrCmpNI(fopcd, ".IFD", 4) == 0 ||
       StrCmpNI(fopcd, ".E", 2) == 0
      ))  
    {
    sprintf(pszErrtextbuf, "%s: %s", flabl, fopcd);
    errorp2(ERR_SYNTAX, pszErrtextbuf);                   
    }

  // PASS2: Pseudo instructions and directives
  switch (insv[1])
    {
    case 0:     // Process next line if opcode is empty.
      break;
    case 2:     // {4,"ORG"   ,1, 2}, (Set Program counter address)
      p2org();          
      edipc();  // Edit pc
      edpri();  // Emit .LST line
      break;            
    case 3:     // {4,"MODEL" ,1, 2}, "MODEL" Directive (see pass1.cpp)
      clepc();
      edpri();                        // Only print $.<directive> src-line
      break;            
    case 4:     // {5,"CSEG"  ,1, 4}, "CSEG" DIRECTIVE      
      SegType = _CODE;      
      if (pcc == 0 && RomStart > 0)
        {
        pcc = RomStart;  
        csegLayout_ptr->sStart = RomStart; 
        }
      GetPcValue();
      csegLayout_ptr->sEnd = pcc;     // update PCs in info structure
      dsegLayout_ptr->sEnd = pcd; 
      esegLayout_ptr->sEnd = pce; 
      clepc();                        // Don't show pc from previous segment
      edpri();                        // Print $.<directive> src-line
      break;            
    case 6:     // {5,"DSEG"  ,1, 6}, "DSEG" DIRECTIVE       
      SegType = _DATA;      
      // Re-init initial value (forward reference)
      if (pcd == 0 && SRamStart > 0)
        {
        pcd = SRamStart;  
        dsegLayout_ptr->sStart = SRamStart; 
        }
      GetPcValue();
      csegLayout_ptr->sEnd = pcc;     // update PCs in info structure
      dsegLayout_ptr->sEnd = pcd; 
      esegLayout_ptr->sEnd = pce; 
      clepc();                        // Don't show pc from previous segment
      edpri();                        // Print $.<directive> src-line
      break;
    case 8:     // {5,"ESEG"  ,1, 8}, "ESEG" Directive (Atmel EEPROM Data)       
      SegType = _EEPROM;      
      if (pce == 0 && EEPromStart > 0)
        {
        pce = EEPromStart;  
        esegLayout_ptr->sStart = EEPromStart; 
        }
      GetPcValue();
      csegLayout_ptr->sEnd = pcc;     // update PCs in info structure
      dsegLayout_ptr->sEnd = pcd; 
      esegLayout_ptr->sEnd = pce; 
      clepc();                        // Don't show pc from previous segment
      edpri();                        // Print $.<directive> src-line
      break;
    case 10:    // {3,"DW"    ,1,10}, (Define Word)            
      p2dw();   // Motorola: FDB      (Form Double Byte Constant)
      break;    
    case 12:    // {3,"DB"    ,1,12}, (Define Byte)
      p2db();   // Motorola: FCB      (Form Constant Byte)
      break;
    case 14:    // {3,"DS"    ,1,14}, (Define Storage)
      p2ds();   // Motorola: RMB      (Reserve Memory Block)
      break;

    case 16:    // {4,"END"   ,1,16}, "END/.EXIT" directive is encountered
      p12CCheckIfend();   // Preprocessing conditionals
      p12CheckEndmacro();
      if (AtmelFlag != 0 && swicl != 0)     // filestream .INC
        {
        // The EXIT directive tells the Assembler to stop assembling the file.
        // Normally, the Assembler runs until end of file (EOF).
        // If an EXIT directive appears in an included file, the Assembler
        // continues from the line following the INCLUDE directive in the file
        // containing the INCLUDE directive.
        //
        // Issue at least a "WARNING - Check END-OF-FILE directive",
        // since this is no good programming practice and not recommended.
        warno = WARN_CHKEXIT;
        warnSymbol_ptr = END_str; 
        warnout();
        CloseIclFile();           // Stop assembling file .INC
        } // end if (AtmelFlag)

      else                        // EXIT/END: Terminate assembling source file
        {
        clepc();
        // ".END" with a 'START' address operator is not applicable
        if (oper1[0] != 0) errorp2(ERR_TOOLONG, NULL);
        edpri();                  // Normal exit
        wrhex(_CODE);             // Finish and complete Flash PROM hex-file
        if (AtmelFlag != 0)
          {
          wrhex(_EEPROM);// Finish and complete EEPROM hex-file
          csegLayout_ptr->sEnd = pcc;  // Terminate the memory info structures
          dsegLayout_ptr->sEnd = pcd;
          esegLayout_ptr->sEnd = pce;
          }
        return(FALSE);   // Pass 2 completed - return from pass2
        } // end else
      break;

    case 18:
      p2equ();  // {4,"EQU"   ,1,18}, (Define a symbol equal to an expression)
      break;                    
    case 20:    // {6,"EXTRN" ,1,20},
      clepc();
      edpri();                        // Only print $.<directive> src-line
      break;
    case 22:    // {6,"PUBLC" ,1,22},
      clepc();
      edpri();                        // Only print $.<directive> src-line
      break;

    case 24:    // {TITLE_str,1,24},  "TITLE" Directive       
      clepc();
      edpri();                        // Transfer title text (already done in pass1)
      break;                                             
    case 26:    // {SUBTL_str,1,26},  "SUBTTL" Directive        
      titleXfer(_dirStr, SUBTL_str, lhsubttl);
      clepc();
      edpri();                        // Print $.<directive> src-line
      break;                                              
    case 27:    // {MSG_str,  1,27},  "MESSAGE" Directive
      clepc();
      edpri();                        // List $.<directive> src-line
      break;
    case 28:    // {PW_str,   1,28},  "PAGEWIDTH(" Directive        
      clepc();
      edpri();                        // Only print $.<directive> src-line
      break;                                              
    case 30:    // {PL_str,   1,30},  "PAGELENGTH(" Directive       
      clepc();
      edpri();                        // Only print $.<directive> src-line
      break;
 
    case 32:    // {ICL_str,  1,32},  "INCLUDE(" Directive
      OpenIncludeFile(ICL_str, _dirStr); // Open include file (error see Pass1)
      clepc();
      edpri();                        // Print $.<directive> src-line
      break;

    case 34:    // {NOLST_str,1,34},  "NOLIST" Directive
      clepc();
      edpri();                        // Print $.<directive> src-line
      swlst = 0;                      // Disable listing src-lines
      break;
    case 36:    // {LST_str,  1,36},  "LIST" Directive       
      if (swlst == 0)                 // Only if "NOLIST" currently
        {
        sprintf(&szListBuf[lpLINE], "%5d", ++_lstline); // Emit lstline count
        szListBuf[lpLINE+5] = ' ';    // Remove 0-terminator
        }
      swlst = 1;                      // Enable listing
      clepc();
      edpri();                        // Print $.<directive> src-line
      break;
    case 38:    // {NOSYM_str,1,38},  "NOSYMBOLS / SYMBOLS" Directives
      clepc();
      edpri();                        // Only print $.<directive> src-line
      break;
    case 40:    // {EJECT_str,1,40},  "EJECT" Directive       
      if (swlst == 0) ;               // If 'NOLIST" then don't do anything
      else
        {
        clepc();
        edpri();                      // Print $.<directive> src-line
        lhead();                      // Skip to next page
        } 
      break;

    case 41:    // {".DEVICE",1,41},  ".DEVICE" Directive (Atmel AVR specific)         
      if (pcc>RomStart || pcd>SRamStart || pce>EEPromStart) // if (errorp1() GetPcValue()
        {
        errorp2(ERR_P1PHASE, "Missplaced .DEVICE directive");                                   
        clepc();
        edpri();                      // Print $.<directive> src-line
        return(FALSE);                // "Missplaced .DEVICE directive"
        }
      else
        {
        clepc();
        edpri();                      // Print $.<directive> src-line
        }   
      break;
    case 42:    // {DD_str,   1,42}, (Define Double Word 32bit)         
      p2dd();
      break;
    case 43:    // {SET_str,  1,43},  ".SET" Directive       
      p2set();
      break;

    case 44:    // {0,""     1,50}, Reserve         
      clepc();
      edpri();                        // Print $.<directive> src-line
      break;

    case 45:    // {""     ,1,45}, Undefine .DEF'ed regs (silently Ignored / not needed) 
      clepc();
      edpri();                        // Print $.<directive> src-line
      break;

    case 46:    // {_undef_str   ,1,46}, "#undef" Directive         
      p12Cundef(_PASS2);
      clepc();
      edpri();  // Print $.<directive> src-line
      break;
    case 47:    // {DQ_str,   1,47}, (Define Quad Word 64bit)         
      p2dq();
      break;
    case 48:    // {CSEGSIZE,""     1,48}, ".CSEGSIZE Directive (program memory size)         
      clepc();
      edpri();                        // Only print $.<directive> src-line
      break;
    case 49:    // // {EVEN,""     1,48}, ".EVEN Directive (align labels to WORD boundaries)         
      GetPcValue();
      clepc();
      edpri();                // print $.<directive> src-line
      if ((pcValue % 2) != 0)  
        {
        cllin();                // Print an aligning zero on a new line
        edipc();
        insv[1] = nopAlign;     // Alignment on even program address boundary
        ilen = 1;               // One NOP instruction inserted
        edpri_db();             // Emit an aligning zero into ListBuf
        } // end if (AtmelFlag)
      break;

    case 50:    // {0,""     1,50}, Reserve         
      break;

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
    case 51:    // {LSTMAC_str ,1,51}, ".LISTMAC" Directive 
      if (toupper(_dirStr[1]) == 'N') swlistmac = FALSE;        
      else swlistmac = TRUE;          // Enable listing of macro expansion
      clepc();
      edpri();  // Print $.<directive> src-line
      break;
    case 52:    // {MACRO_str  ,1,52}, ".MACRO" Directive
        p12macro(_PASS2);
        clepc();
        edpri();  // Print $.<directive> src-line
      p2defmacroFlag = TRUE;          // Indicate .MACRO (global)
      break;
    case 53:    // {ENDM_str   ,1,53}, ".ENDM/.ENDMACRO" Directive 
      p2defmacroFlag = FALSE;         // Indicate .ENDM/.ENDMACRO (global)
        p12endmacro();
        clepc();
        edpri();  // Print $.<directive> src-line
      break;

    case 54:    // {_pragm_str, 1,54}, "#pragma" Directive
      p2pragma(&oper1[0]);
      clepc();
      edpri();  // Print $.<directive> src-line
      break;

    // .OVERLAP and .NOOVERLAP
    // Set up overlapping section.
    // These directives are for special projects and should normally not be used.
    // Only the currently active segment is affected (CSEG, DSEG and ESEG).
    // Example
    // .OVERLAP
    // .ORG 0        ; section #1
    // rjmp default
    // .NOOVERLAP
    // .ORG 0        ; section #2
    // rjmp reset    ; No error given here
    // .ORG 0        ; section #3
    // rjmp reset    ; Error here because overlap with #2
    //
    case 55:    // {OVLAP_str, 1,,55}, ".OVERLAP" Org address overlap allowed
      swoverlap = 1;
      clepc();
      edpri();  // Print $.<directive> src-line
      break;
    case 56:    // {NOOVLAP_str,1,56},  ".NOOVERLAP" Org address may not overlap        
      swoverlap = 0;
      clepc();
      edpri();  // Print $.<directive> src-line
      break;

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
    // #ifndef / .IFNDEF <condition>
    // #ifdef  / .IFDEF <condition>
    // #if     / .IF <condition>
    // #elif   / .ELIF <condition>
    //
    case 57:    // {_if_str     ,1,57}, "#if" Directive         
      clepc();
      edpri();  // Print $.<directive> src-line
      p12Cif(_PASS2);
      if ((p12CSkipFlag & TRUE) == TRUE) edpri();
      break;
    case 58:    // {_else_str   ,1,58}, "#else" Directive         
      clepc();
      edpri();  // Print $.<directive> src-line
      p12Celse(_PASS2);
      if ((p12CSkipFlag & TRUE) == TRUE) edpri();
      break;
    case 59:    // {_elif_str   ,1,59}, "#elif/.elseif" Directive         
      clepc();
      edpri();  // Print $.<directive> src-line
      p12Celseif(_PASS2);
      if ((p12CSkipFlag & TRUE) == TRUE) edpri();
      break;
    case 60:    // {_ifdef_str  ,1,60}, "#ifdef" Directive         
      clepc();
      edpri();  // Print $.<directive> src-line
      p12Cifdef(_PASS2);
      if ((p12CSkipFlag & TRUE) == TRUE) edpri();
      break;
    case 61:    // {_ifndef_str ,1,61}, "#ifndef" Directive         
      clepc();
      edpri();  // Print $.<directive> src-line
      p12Cifndef(_PASS2);
      if ((p12CSkipFlag & TRUE) == TRUE) edpri();
      break;
    case 62:    // {_endif_str  ,1,62}, "#endif" Directive         
      clepc();
      edpri();  // Print $.<directive> src-line
      p12Cendif(_PASS2);
      if ((p12CSkipFlag & TRUE) == TRUE) edpri();
      break;

    default:
      errorp2(ERR_ILLDIR, fopcd);
      edpri();
      break;
    } // end switch (insv[1])

  return(TRUE); // Return to pass2
  } // p2pse 

//-----------------------------------------------------------------------------
//
//                               p2device()
//
//  DEVICE - Define which device to assemble for (Atmel specific)
//
//  Syntax:
//  .DEVICE <device code>
//  Example:
//  .DEVICE AT90S1200 ; Use the AT90S1200 µC 
//
//   The DEVICE directive allows the user to tell the Assembler which device
//   the code is to be executed on. Using this directive, a warning is issued
//   if an instruction not supported by the specified device occurs.
//   If the Code Segment or EEPROM Segment are larger than supplied by
//   the device, a warning message is given. If the directive is not used,
//   it is assumed that all instructions are supported and that there are
//   no restrictions on Program and EEPROM memory.
//
//   Table: Device codes (2003):
//   Classic      Tiny          Mega        Other
//   ------------------------------------------------
//   AT90S120     ATtiny11      ATmega8     AT94K
//   AT90S2313    ATtiny12      ATmega16    AT86RF401
//   AT90S2323    ATtiny22      ATmega161
//   AT90S2333    ATtiny26      ATmega162
//   AT90S4414                  ATmega163 
//   AT90S4434                  ATmega32  
//   AT90S8515                  ATmega323 
//   AT90S8534                  ATmega103 
//   AT90S8535                  ATmega104
//   AT90S2343                  ATmega8515
//   AT90S4433                  ATmega8535
//                              ATmega64  
//                              ATmega128
//   ------------------------------------------------
//
// About 20 years later..
//  see the list of currently existing devices by Microchip (->opcodeAVR.cpp):
//  Device Tables
//  © 2020 Microchip Technology Inc. Manual DS40002198A-page 148ff
//
void p2device()
  {
  // Set warning code only if no Atmel *def.inc file 
  // Not implemented: i.e., no restrictions on Program and EEPROM memory.
  //
  // Default hardware layout (see InitMemorySize() in bytvalAVR.cpp):
  //  RomSize    = 256*1024*1024; // Program Flash ROM: Board design specific (default=256M)
  //  SRamStart  =         0x000; // Internal SRAM start address: Board design specific (default=0x000)
  //  SRamSize   =   8*1024*1024; // Internal SRAM: Board design specific (default=8M)
  //  EEPromSize =       64*1024; // EEPROM: Board design specific (default = 64K)
  //

  ////////////////////////
  ActivityMonitor(OFF); //
  ////////////////////////

  char pragma1[] = "#pragma AVRPART MEMORY PROG_FLASH 268435456";
  char pragma3[] = "#pragma AVRPART MEMORY INT_SRAM SIZE 8388608";
  char pragma4[] = "#pragma AVRPART MEMORY INT_SRAM START_ADDR 0x000";
  char pragma2[] = "#pragma AVRPART MEMORY EEPROM 65536";

  // Display on console
  if (AtmelFlag !=0 && swpragma == 0 && swdevice == 0)
    {
    printf("  -------------------------------------------------------------\n"
           " | WARNING - .DEVICE: Unrecognizable PART_NAME                 |\n" 
           " | Refer to device's Data Sheet for the correct memory layout, |\n"
           " | or .INCLUDE \x22%sdef.inc\x22 file, if appropriate.\n"
           "  -------------------------------------------------------------\n"
           " Info - Default memory range assumed:\n", partNameAVR);
    p2pragma(&pragma1[0]);
    p2pragma(&pragma2[0]);
    p2pragma(&pragma3[0]);
    p2pragma(&pragma4[0]);
    }
 
  // Display info on console
  else if (AtmelFlag !=0 && swpragma == 0 && swdevice == 1)  
    {
    printf("\tDEVICE PART_NAME %s %s\n", pragmaAVRPART, partNameAVR);
    printf("\t%s START_ADDR 0x%04X\n", pragmaPROGFLASH, RomStart);
    printf("\t%s SIZE %d (%dK)\n", pragmaPROGFLASH, RomSize, RomSize/1024);
  
    printf("\t%s 0x%04X\n", pragmaSRAMADDR, SRamStart);
    printf("\t%s %d", pragmaSRAMSIZE, SRamSize);
    if (!(swmodel & _NOINFO) && SRamSize > 1024) printf(" (%dK)", SRamSize/1024);    
    printf("\n");
    
    printf("\t%s START_ADDR 0x%04X\n", pragmaEEPROM, EEPromStart);
    printf("\t%s SIZE %d", pragmaEEPROM, EEPromSize);
    if (!(swmodel & _NOINFO) && EEPromSize > 1024) printf(" (%dK)", EEPromSize/1024);
    printf("\n");
    }
  } // p2device

//-----------------------------------------------------------------------------
//
//                               p2pragma
//
//  Specification
//  1. #pragma AVRPART ADMIN PART_NAME string
//  2. #pragma AVRPART CORE CORE_VERSION version-string
//  3. #pragma AVRPART CORE INSTRUCTIONS_NOT_SUPPORTED
//      mnemonic[ operand[,operand] ][:...]
//  4. #pragma AVRPART CORE NEW_INSTRUCTIONS
//     mnemonic[ operand[,operand]][:...]
//  5. #pragma AVRPART MEMORY PROG_FLASH size
//  6. #pragma AVRPART MEMORY EEPROM size
//  7. #pragma AVRPART MEMORY INT_SRAM SIZE size
//  8. #pragma AVRPART MEMORY INT_SRAM START_ADDR address
// 
//  Examples
//  Note: The combination of these examples does not describe a real AVR part!
//  1. #pragma AVRPART ADMIN PART_NAME ATmega32
//  2. #pragma AVRPART CORE CORE_VERSION V2
//  3. #pragma AVRPART CORE INSTRUCTIONS_NOT_SUPPORTED movw:break:lpm rd,z
//  4. #pragma AVRPART CORE NEW_INSTRUCTIONS lpm rd,z+
//  5. #pragma AVRPART MEMORY PROG_FLASH 131072
//  6. #pragma AVRPART MEMORY EEPROM 4096
//  7. #pragma AVRPART MEMORY INT_SRAM SIZE 4096
//  8. #pragma AVRPART MEMORY INT_SRAM START_ADDR 0x60
//  -. #pragma partinc 0 (not specified, ignored)
//  
//  Real example: Specify the device "AT90S8515" (Date 2011-02-09)
//  .DEVICE AT90S8515
//  #pragma AVRPART ADMIN PART_NAME AT90S8515
//   .equ SIGNATURE_000 = 0x1e
//   .equ SIGNATURE_001 = 0x93
//   .equ SIGNATURE_002 = 0x01
//  
//  #pragma AVRPART CORE CORE_VERSION V1
//  #pragma AVRPART CORE INSTRUCTIONS_NOT_SUPPORTED break
//  
//  #pragma AVRPART MEMORY PROG_FLASH 8192
//  #pragma AVRPART MEMORY EEPROM 512
//  #pragma AVRPART MEMORY INT_SRAM SIZE 512
//  #pragma AVRPART MEMORY INT_SRAM START_ADDR 0x60
//
//  Example ATA8515 (date 2020)
//  #pragma AVRPART CORE INSTRUCTIONS_NOT_SUPPORTED elpm:eijmp:eicall
// 
// ----------------------------------------------------------------------------
//
// fopcd = "#pragma"  (Atmel AVR specific)
// oper1 = "AVRPART MEMORY PROG_FLASH 8192" (example)
//
//   RomSize
//   EEPromSize
//   SRamStart 
//   SRamSize  
//
void p2pragma(char* _operand)
  {
  char* tmpPtr = NULL;
  int _segVal;

  ////////////////////////
  ActivityMonitor(OFF); //
  ////////////////////////

  // #pragma partinc 0 = Atmel Include File
  if ((tmpPtr=StrStrI(_operand, pragmaPartinc)) != 0)
    {
    swpragma = 1;                     // Indicate an Atmel *def.inc file 
    if (!(swmodel & _NOINFO)) printf("\t%s \x22%s\x22\n", tmpPtr, pszCurFileName);              
    }

  // #pragma, AVR Part Related
  else if ((tmpPtr=StrStrI(_operand, pragmaAVRPART)) != 0)
    {
    // Save current segment
    _segVal = SegType;
                 
    tmpPtr += strlen(pragmaAVRPART);        // skip to category
    *tmpPtr = TAB; 
    if (!(swmodel & _NOINFO)) printf("%s ", tmpPtr); // display on console
    tmpPtr = StrStrI(oper1, "PART_NAME ");
    tmpPtr += strlen("PART_NAME ");

    StrCpy(partNameAVR, tmpPtr);            // Save partNameAVR string

    if ((tmpPtr=StrStrI(_operand, pragmaSRAMADDR)) != 0)
      {
      tmpPtr += strlen(pragmaSRAMADDR)+1;
      SegType = _DATA;                      // DATA segment (SRAM)
      GetPcValue();
      pcValue = expr(tmpPtr);               // Get SRAM start address
      SRamStart = pcValue;                  // Set SRamSart
      dsegLayout[0].sStart = SRamStart;     // Init pcd within info structure
      AssignPc();                           // Init data segment start pcd
      }
    else if ((tmpPtr=StrStrI(_operand, pragmaPROGFLASH)) != 0)
      {
      tmpPtr += strlen(pragmaPROGFLASH)+1;  // +SPACE
      RomSize = expr(tmpPtr);               // Set RomSize
      if (!(swmodel & _NOINFO) && RomSize > 1024) printf("(%dK)", RomSize/1024);      // display on console
      }
    else if ((tmpPtr=StrStrI(_operand, pragmaSRAMSIZE)) != 0)
      {
      tmpPtr += strlen(pragmaSRAMSIZE)+1;   // +SPACE
      SRamSize = expr(tmpPtr);              // Set SRamSize
      if (!(swmodel & _NOINFO) && SRamSize > 1024) printf("(%dK)", SRamSize/1024);    // display on console
      }
    else if ((tmpPtr=StrStrI(_operand, pragmaEEPROM)) != 0)
      {                                                                       
      tmpPtr += strlen(pragmaEEPROM)+1;     // +SPACE
      EEPromSize = expr(tmpPtr);            // Set EEPromSize
      if (!(swmodel & _NOINFO) && EEPromSize > 1024) printf("(%dK)", EEPromSize/1024); // display on console
      }
    if (!(swmodel & _NOINFO)) printf("\n"); // CR/LF on console
    
    // Restore current segment
    SegType = _segVal;           
    } // end if

  // #pragma, General Purpose (currently ignored)
  // Syntax
  //  #pragma warning range byte option
  //  #pragma overlap option
  //  #pragma error instruction
  //  #pragma warning instruction
  //
  // Display on console
  else printf("%s(%d): %s %s - Directive ignored\n", pszCurFileName, _curline, fopcd, _operand);
  } // p2pragma

//-----------------------------------------------------------------------------
//
//                               p2org
// ORG Directive
// input: oper1
//
void p2org()
  {
  char* tmpPtr=oper1;
  ULONG _pcval;

  GetPcValue();

  _pcval = expr(oper1);

  // A statement like '.ORG PC' or ' ORG $' can be ignored in .CSEG
  if (AtmelFlag != 0 && SegType == _CODE && (_pcval*2) == pcValue) return;

  // Code segment: Terminate old rest, if any, and emit to disk
  if (SegType == _CODE) wrhex(_CODE);                 
  if (AtmelFlag != 0 && SegType == _EEPROM) wrhex(_EEPROM);                 
  // Set PC according to ORG (get new pc-value)

  // Motorola specific: Insert error line, if ORG statement is misplaced.
  // May result in unknown and wrong forward label values.
  if (AtmelFlag == FALSE && MotorolaFlag == TRUE && _pcval < pcValue)
    errorp2(ERR_ORGINS, NULL);

  else if (AtmelFlag != FALSE)
    {
    // All Atmel provided .INC-files assume .CSEG "WORD Addressing", so 
    // we must double all .CSEG .ORG addresses since we're always in ".MODEL BYTE".
    //ha//if ((swmodel & _BYTE) == _BYTE && SegType == _CODE) pcValue *= 2;   
    if (SegType == _CODE) _pcval *= 2;    

    // Insert error line, if ORG statement is misplaced.
    // May result in unknown and wrong forward label values.
    //
    // Example source (Forbes.asm) allow;
    //  1) .ORG 0x1000  
    //  2) .ORG 0  
    //  3) .ORG 0x100
    //
    // ..makes checking for .CSEG overlap complicated)
    // 
    //   Solution: Example source (Forbes.asm)  - .MODEL WORD
    //   Info - Memory segments organization
    //          CSEG: Start = 0x00000000  End = 0x00000001  Size = 1 word(s)
    //          CSEG: Start = 0x00000100  End = 0x00000200  Size = 256 word(s)
    //          CSEG: Start = 0x00001000  End = 0x000010B8  Size = 184 word(s)
    //          CSEG: Code size = 882 bytes
    // 
    //          DSEG: Start = 0x00000100  End = 0x00000100  Size = 0 byte(s)
    //          DSEG: Data size = 0 bytes
    // 
    //          ESEG: Data size = 0 bytes
    //
    //  Solution: Example source (Forbes.asm) - .MODEL BYTE
    //  Info - Memory segments organization
    //         CSEG: Start = 0x00000000  End = 0x00000002  Size = 2 byte(s)
    //         CSEG: Start = 0x00000200  End = 0x00000400  Size = 512 byte(s)
    //         CSEG: Start = 0x00002000  End = 0x00002170  Size = 368 byte(s)
    //         CSEG: Code size = 882 bytes
    // 
    //         DSEG: Start = 0x00000100  End = 0x00000100  Size = 0 byte(s)
    //         DSEG: Data size = 0 bytes
    // 
    //         ESEG: Data size = 0 bytes
    //
   if (SegType == _CODE)
      {
      csegLayout_ptr->sEnd = pcValue;
      (++csegLayout_ptr)->sStart = _pcval;
      }                                                       

    else if (SegType == _DATA)  
      {
      dsegLayout_ptr->sEnd = pcValue;
      (++dsegLayout_ptr)->sStart = _pcval;
      }

    else if (SegType == _EEPROM)  
      {
      esegLayout_ptr->sEnd = pcValue;
      (++esegLayout_ptr)->sStart = _pcval;
      }
    } // end if (AtmelFlag)

  pcValue = _pcval;        // store the ORG value

  // Update hxpc (Code segment)
  if (SegType == _CODE) hxpc = (pcValue & 0x0000FFFF);

  // Atmel:  Update eehxpc (EEPROM segment)
  else if (SegType == _EEPROM) eehxpc = (pcValue & 0x0000FFFF);

  AssignPc();              // Set new PC according to segment
  _errCSEG = 0;            // Enable error check (see CheckPc())
  } // p2org

//-----------------------------------------------------------------------------
//
//                               p2ds
// DS DIRECTIVE - Define storage
// input: oper1
//
void p2ds()
  {
  GetPcValue();                // Get current pc
  
  if (AtmelFlag != 0 && SegType == _CODE) 
    {
    errorp2(ERR_SYNTAX, fopcd); // Atmel: .BYTE not allowed in CSEG
    edpri();                    // Emit .LST line
    return;   
    }

  // XASM6505, XASM6802, XASM8042:
  //  Don't advance "hxbuf_ptr += pcValue;" although DS is allowed.
  //  (Because of the programming style in the 1980s)
  // Write update CSEG contents into .HEX file
  if (SegType == _CODE && hxpc <= pcValue) wrhex(_CODE);       
  
  // Atmel: Write update ESEG EEPROMcontents into EEP.HEX file
  //else if (AtmelFlag != 0 && SegType == _EEPROM) wrhex(_EEPROM);
  else if (AtmelFlag != 0 && SegType == _EEPROM && eehxpc <= pcValue) wrhex(_EEPROM);

  pcValue += expr(oper1);      // Update current pc
  AssignPc();                  // Set PC according to gap

  // Update pcd in info DSEG structure
  dsegLayout_ptr->sEnd = pcd;  

  // Update hxpc, skipping the gap
  if (SegType == _CODE) hxpc = (UINT)pcc;

  // Update eehxpc, skipping the gap            
  else if (AtmelFlag != 0 && SegType == _EEPROM) eehxpc = (UINT)pce;        

  edpri();                     // Emit .LST line
  } // p2ds

//-----------------------------------------------------------------------------
//
//                       p2LineContinuation
//
// Pass2 line continuation for .DD, .DW .DB  directives.
// Point at the last char before any possible
// trailing cntls and spaces at the end of inbuf, and check 
// if the last char is the line continuation '\' or a comma.
// Note: No comment is allowed within a continuated line.
//
// Line continuation '\',CR,LF,0 for .DB directives
//  Example
//   .DB   0, 1, "This is a long string", '\n', 0, 2, \
//   "Here is another one", '\n', 0, 3, 0
//
// ----------------------------------------------------------------------------
// Better solution (Microsoft ML Version 14.28.299910 - Listing):
//  
//  0025 18 0F 06 13 14 1C         TEST1 \
//        14 1C 0B 1B 10 00
//        10 00 0E 16 19 04
//        19 04 11 1E 09 01
//        09 01 07 17 0D 1F
//        0D 1F 1A 02 08 12
//        08 12 0C 1D 05 15
//        05 15 0A 03 18 0F
//                                         DB      24, 15,  6, 19, 20, 28, 20, 28, 11, 27, 16,  0, \
//                                                 16,  0, 14, 22, 25,  4, 25,  4, 17, 30,  9,  1,
//                                                  9,  1,  7, 23, 13, 31, 13, 31, 26,  2,  8, 18,
//                                                  8, 18, 12, 29,  5, 21,  5, 21, 10,  3, 24, 15
//                                 ;;
//  0055                           TEST2 LABEL BYTE
//  0055  18 0F 06 13 14 1C                DB      24, 15,  6, 19, 20, 28, 20, 28, 11, 27, 16,  0,
//        14 1C 0B 1B 10 00
//        10 00 0E 16 19 04
//        19 04 11 1E 09 01
//        09 01 07 17 0D 1F
//        0D 1F 1A 02 08 12
//        08 12 0C 1D 05 15
//        05 15 0A 03 18 0F
//                                                 16,  0, 14, 22, 25,  4, 25,  4, 17, 30,  9,  1,
//                                                  9,  1,  7, 23, 13, 31, 13, 31, 26,  2,  8, 18,
//                                                  8, 18, 12, 29,  5, 21,  5, 21, 10,  3, 24, 15
//                                 ;;
//  0085  18 0F 06 13 14 1C                DB      24, 15,  6, 19, 20, 28, 20, 28, 11, 27, 16,  0,
//        14 1C 0B 1B 10 00                        16,  0, 14, 22, 25,  4, 25,  4, 17, 30,  9,  1,
//        10 00 0E 16 19 04                         9,  1,  7, 23, 13, 31, 13, 31, 26,  2,  8, 18,
//        19 04 11 1E 09 01                         8, 18, 12, 29,  5, 21,  5, 21, 10,  3, 24, 15
//        09 01 07 17 0D 1F
//        0D 1F 1A 02 08 12
//        08 12 0C 1D 05 15
//        05 15 0A 03 18 0F
//                                 ;;
//  0085  30 31 32 34 35 36                DB      '012456789ABCDEF 012456789ABCDEF 012456789ABCDEF', 
//        37 38 39 41 42 43
//        44 45 46 20 30 31
//        32 34 35 36 37 38
//        39 41 42 43 44 45
//        46 20 30 31 32 34
//        35 36 37 38 39 41
//        42 43 44 45 46 18
//        0F 06 13 14 1C 14
//        1C 0B 1B 10 00 10
//        00 0E 16 19 04 19
//        04 11 1E 09 01 09
//        01 07 17 0D 1F 0D
//        1F 1A 02 08 12 08
//        12 0C 1D 05 15 05
//        15 0A 03 18 0F
//                                                 24, 15,  6, 19, 20, 28, 20, 28, 11, 27, 16,  0,
//                                                 16,  0, 14, 22, 25,  4, 25,  4, 17, 30,  9,  1,
//                                                  9,  1,  7, 23, 13, 31, 13, 31, 26,  2,  8, 18,
//                                                  8, 18, 12, 29,  5, 21,  5, 21, 10,  3, 24, 15
//                                 ;;
//                                         DB      '012456789ABCDEF 012456789ABCDEF 012456789ABCDEF', \
// c:\temp600\___\_ml_test.asm(54) : error A2042:statement too complex
//                                                 '012456789ABCDEF 012456789ABCDEF 012456789ABCDEF',
//                                                 24, 15,  6, 19, 20, 28, 20, 28, 11, 27, 16,  0,
//                                                 16,  0, 14, 22, 25,  4, 25,  4, 17, 30,  9,  1,
//                                                  9,  1,  7, 23, 13, 31, 13, 31, 26,  2,  8, 18,
//                                                  8, 18, 12, 29,  5, 21,  5, 21, 10,  3, 24, 15
//
// ----------------------------------------------------------------------------
//
void p2LineContinuation()
  {
  static char tmpLstbuf[LSBUFLEN];
  static char tmpInbuf[LSBUFLEN];
  char* tmpInbufPtr = tmpInbuf;
  char* inbufPtr = NULL;

  int loopCount = 0;
  while ((*(inbufPtr = skip_trailing_spaces(inbuf)) == '\\' ||
         *skip_trailing_spaces(inbuf) == COMMA)             && 
         strstr(inbuf, ";") == NULL)
    {
    // Clear tmpInbuf, space out tmpLstbuf
    for (_i=0; _i<LSBUFLEN; _i++)
      {
      tmpInbuf[_i] = 0;
      tmpLstbuf[_i] = SPACE;            
      }

    // Only list incoming line once
    if (loopCount == 0)                             // Initial loop only
      {
      StrCpy(&tmpLstbuf[lpSRC], inbuf);             // Save inbuf line '\'
      sprintf(&tmpLstbuf[lpLINE], "%5d", _lstline); // Emit lstline count
      tmpLstbuf[lpLINE+5] = ' ';                    // Remove 0-terminator from sprintf

      //////////////////////////////////////////////////////////////////////////      
      if (flabl[0] !=0)                   // Emit current pc into szListBuf
        {
        GetPcValue();
        // Atmel:  pcValue=24/32bit - all segments
        if (AtmelFlag != 0)            
          {
          if ((swmodel  & _WORD) == _WORD && SegType == _CODE)
            eddh(&tmpLstbuf[lpLOC], pcValue/2);  // .MODE WORD
          else 
            eddh(&tmpLstbuf[lpLOC], pcValue);    // .MODE BYTE
          }
        // Intel, Motorola: pcc=16bit (SegType not edited in list line)
        else                           
          edwh(&tmpLstbuf[lpLOC], pcValue);
        } // end if (flabl[0] !=0)
      //////////////////////////////////////////////////////////////////////////      

      lineAppendCRLF(&tmpLstbuf[lpSRC+strlen(inbuf)]);
      // Xfer incoming src line '\' into .LST file
      if ((swlistmac && swexpmacro) || !swexpmacro)
        {
        if (swexpmacro) tmpLstbuf[lpMACR] = MACRO_IDM; // Emit MACRO line marker
        linut(tmpLstbuf);                              //write_lstbuf(tmpLstbuf);      
        }
      for (_i=lpLOC; _i<=lpSRC; _i++) tmpLstbuf[_i] = SPACE; // Space out lpLINE
      } // end if (loopcount) 

    // Eliminate src line continuation char '\' (would cause errors otherwise)
    if (*inbufPtr == '\\') *inbufPtr = SPACE;            

    // Terminate the incoming src line (dont list src line twice) 
    lineAppendCRLF(&szListBuf[lpLINE]);

    tmpInbufPtr = tmpInbuf;
    StrCpy(tmpInbufPtr, inbuf);            // Save inbuf src line into tmpInbuf
    tmpInbufPtr += strlen(inbuf);          // Advance tmpInbufPtr

    read_source_line();                    // Read inbuf next line after '\'

    StrCpy(&tmpLstbuf[lpSRC], inbuf);      // Format list line and store
    // Xfer next line after '\' into .LST file
    if ((swlistmac && swexpmacro) || !swexpmacro)
      {
      if (swexpmacro) tmpLstbuf[lpMACR] = MACRO_IDM; // Emit MACRO line marker
      linut(tmpLstbuf);                    //write_lstbuf(tmpLstbuf);      
      }

    inbufPtr = skip_leading_spaces(inbuf);
    StrCpy(tmpInbufPtr, inbufPtr);         // Append inbuf to tmpInbuf
    tmpInbufPtr += strlen(inbufPtr);       // Advance tmpInbufPtr

    if (strlen(tmpInbuf) > INBUFLEN)
      {
      errorp2(ERR_TOOLONG, NULL);         // Src line is too long!
      erout();
      return;                // Abort
      }
 
    StrCpy(inbuf, tmpInbuf); // Src line concatenation done.
    loopCount++;             // Try and indicate next loop
    } // end While

  lineAppendCRLF(&inbuf[strlen(inbuf)]);        
  } // p2LineContinuation
                                                              
//-----------------------------------------------------------------------------
//
//                        CheckRadixSyntax
//
// Check number in expression (Intel/Motorola/Atmel syntax)
//   Bin:  10001111b, %10001111, 0b10001111
//   Hex: 0ABCD1234h, $ABCD1234, 0xABCD1234
//   Dec:  12346789
//   Oct: not supported
//
// Result: FALSE; // illegal number syntax in expression
//         TRUE;  // legal expression
// 
BOOL CheckRadixSyntax(char _operStr[])
  {
//ha//  int _l, _m;
//ha//  replace_token(_operStr);        // unary NOT --> '~'
//ha//  skip_trailing_spaces(_operStr); // scan to end of operStr number
//ha//
//ha//  ////////////////////////////////////////////////////////////////////////
//ha//  // Check binary expression (Intel/Microsoft syntax) 10001111b         //
//ha//  _l = strlen(_operStr);                                                //
//ha//  while (delm2(&_operStr[--_l]) == TRUE);                               //
//ha//  _m = 0;                                                               //
//ha//  while (delm2(&_operStr[_m]) == TRUE && _operStr[_m] != 0) _m++;       //
//ha//                                                                        //
//ha//  // Check strlen(operStr) also, may be a symbol/label named 'b'/'B'    //
//ha//  if (toupper(_operStr[_l]) == 'B' && strlen(_operStr) > 1)             //
//ha//    {                                                                   //
//ha//    // Intel binary syntax ('b'/'B' suffix replaced by '#')             //
//ha//    if (_operStr[_m] == '0' || _operStr[_m] == '1') _operStr[_l] = '#'; //
//ha//    // Should be a hex expression ('h' suffix expected)                 //
//ha//    else if (toupper(_operStr[_l]) != 'H'     &&                        //
//ha//             _operStr[_m] != '$'              &&                        //
//ha//             StrCmpNI(_operStr,"0x",2) != 0   &&                        //
//ha//             label_syntax(_operStr) == FALSE)                           //
//ha//      {                                                                 //
//ha//      errorp2(ERR_ILLEXPR, NULL); // 'h'/'H' suffix is missing          //
//ha//      edpri();                    // Emit Error line                    //
//ha//      return(FALSE);                                                    //
//ha//      }                                                                 //
//ha//    } // end if                                                         //
//ha//  ////////////////////////////////////////////////////////////////////////
  return(TRUE);                                                       
  } // CheckRadixSyntax                                              

//-----------------------------------------------------------------------------
//
//                               p2dq
//
// DQ DIRECTIVE - Define 64bit number
// input: fopcd = ".DQ", inbuf
//
// Example: Get .DQ element value (TEST .DQ directive)
// extern unsigned long long QExpr(char*, int);
// qvalue = QExpr("(11100011-1)*10", RADIX_02);  
// qvalue = QExpr("(123456789-1)*2", RADIX_16);  
// qvalue = QExpr("(123456789-1)*2", RADIX_10);  
//
void p2dq()
  {
  static char _operDQ[OPERLEN];

  p2LineContinuation();

  // scan to 1st DQ, .. element
  char* _inbufDQ_ptr = StrStrI(inbuf, fopcd);   
  if (!delm2(&_inbufDQ_ptr[strlen(fopcd)])) ++_inbufDQ_ptr = StrStrI(_inbufDQ_ptr, fopcd);  
  _inbufDQ_ptr += strlen(fopcd);

  // -----------------------------------------
  // '.DQ' line may not contain any STRNG char
  // -----------------------------------------
  while (*_inbufDQ_ptr != 0 && *_inbufDQ_ptr != ';')
    {
    _inbufDQ_ptr = skip_leading_spaces(_inbufDQ_ptr);
    for (_i=0; _i<OPERLEN; _i++)
      {
      if (*_inbufDQ_ptr == COMMA || *_inbufDQ_ptr == ';')
        {
        _operDQ[_i] = 0;
        break;
        }
      else _operDQ[_i] = *_inbufDQ_ptr++;
      } // end for

    if (!CheckRadixSyntax(_operDQ)) return;

    error = 0;
    // .DQ expression value via parser: qvalue = result (64bit)
    // !! value = dummy, do not use qvalue here
    value = expr(_operDQ); 

    if (error != 0)
      {
      // More than one .(directive) in same line
      if (strstr(_operDQ, "."))     errorp2(ERR_ILLOPER, "'.'");
      else if (erno != ERR_UNDFSYM && error != 6 && error != 7) // Division by zero
        errorp2(ERR_ILLEXPR, _operDQ);
      edpri();                // Emit Error line 
      } // end if (error)
    
    // Intel: little endian,  Motorola: big endian
    dq_endian();            
    edpri_dq();               // Emit in qword rendition into ListBuf
    swp1err = ERR_P1_OFF;     // It's opportune to Ignore all further Pass1 errors

    cllin();                  // Prepare a new ListBuf line
    edipc();                  // Emit current pc
    lineAppendCRLF(&szListBuf[lpXSEG+2]);
    if (*_inbufDQ_ptr == ';') break;
    else _inbufDQ_ptr++;   
    } // end while            // Resume parsing DW line of inbuf

  swp1err ^= ERR_P1_OFF;      // Toggle pass1 error detection = ON
  } // p2dq


//-----------------------------------------------------------------------------
//
//                               p2dd
// DD DIRECTIVE - Define storage
// input: fopcd, inbuf
//
void p2dd()
  {                                                      
  static char _operDD[OPERLEN];

  p2LineContinuation();

  // scan to 1st DD, FDW, .. element
  char* _inbufDD_ptr = StrStrI(inbuf, fopcd);   
  if (!delm2(&_inbufDD_ptr[strlen(fopcd)])) ++_inbufDD_ptr = StrStrI(_inbufDD_ptr, fopcd);  
  _inbufDD_ptr += strlen(fopcd);

  // ----------------------------------------
  // 'DD' line may not contain any STRNG char
  // ----------------------------------------
  while (*_inbufDD_ptr != 0 && *_inbufDD_ptr != ';')
    {
    _inbufDD_ptr = skip_leading_spaces(_inbufDD_ptr);
    for (_i=0; _i<OPERLEN; _i++)
      {
      if (*_inbufDD_ptr == COMMA || *_inbufDD_ptr == ';')
        {
        _operDD[_i] = 0;
        break;
        }
      else _operDD[_i] = *_inbufDD_ptr++;
      } // end for

    if (!CheckRadixSyntax(_operDD)) return;

    error  = 0;
    value = expr(_operDD);  // Get DD element value

    if (error != 0 ||
        !((qvalue & 0xFFFFFFFF00000000) == 0x0FFFFFFFF0000000 || (qvalue & 0xFFFFFFFF00000000) == 0))
      {
      // More than one .(directive) in same line
      if (strstr(_operDD, ".")) errorp2(ERR_ILLOPER, "'.'");
      else
        {
        eddh(&szListBuf[lpOBJ], value);
        // If an undefined symbol was detected, don't show illegal value
        // if Warning don't give error 
//ha//        if (erno != 0 && erno != ERR_UNDFSYM)
//ha//          {
//ha//          sprintf(pszErrtextbuf, "0x%llX", qvalue);
//ha//          errorp2(ERR_ILLEXPR, pszErrtextbuf);
//ha//          }
        if (error != 6 && error != 7) // Division by zero
          {
          warno = WARN_MSKVALUE;                                                   
          sprintf(pszWarntextbuf, "0x%llX", qvalue);                                   
          warnSymbol_ptr = pszWarntextbuf;
          }
        } // end else
      edpri();                // Emit Error line
      } // end if (error)


    // Intel: little endian,  Motorola: big endian
    dd_endian();            
    //edpri();                // Emit bytewise (deprecated)
    edpri_dd();               // Emit in dword rendition into ListBuf
    swp1err = ERR_P1_OFF;     // It's opportune to Ignore all further Pass1 errors

    cllin();                  // Prepare a new ListBuf line
    edipc();                  // Emit current pc
    lineAppendCRLF(&szListBuf[lpXSEG+2]);
    if (*_inbufDD_ptr == ';') break;
    else _inbufDD_ptr++;   
    } // end while            // Resume parsing DW line of inbuf

  swp1err ^= ERR_P1_OFF;      // Toggle pass1 error detection = ON
  } // p2dd


//-----------------------------------------------------------------------------
//
//                               p2dw
//
// DW Directive - Define WORD  (not used if 8bit µP UPI-41/42).
// input: fopcd, inbuf
//
// DW line consisting of elements or expressions separated by COMMA
// No string(s) allowed in DW line.
//
void p2dw()
  {
  static char _operDW[OPERLEN];

  p2LineContinuation();

  // inbuf scan to 1st DW, FDB, .. element provided in fopcd
  char* _inbufDW_ptr = StrStrI(inbuf, fopcd);   
  if (!delm2(&_inbufDW_ptr[strlen(fopcd)])) ++_inbufDW_ptr = StrStrI(_inbufDW_ptr, fopcd);  
  _inbufDW_ptr += strlen(fopcd);

  // ----------------------------------------
  // 'DW' operand ends at 0, comment or comma
  // ----------------------------------------
  while (*_inbufDW_ptr != 0 && *_inbufDW_ptr != ';')
    {
    _inbufDW_ptr = skip_leading_spaces(_inbufDW_ptr);
    for (_i=0; _i<OPERLEN; _i++)
      {
      if (*_inbufDW_ptr == COMMA || *_inbufDW_ptr == ';')
        {
        _operDW[_i] = 0;                   // terminate
        break;
        }
      else _operDW[_i] = *_inbufDW_ptr++;  // Copy
      } // end for

    if (!CheckRadixSyntax(_operDW)) return;

    error = 0;
    value = expr(_operDW); // Get DW element value

    if (error != 0 || !((lvalue & 0xFFFF0000) == 0xFFFF0000 || (lvalue & 0xFFFF0000) == 0))
      {
      // More than one .(directive) in same line
      if (strstr(_operDW, ".")) errorp2(ERR_ILLOPER, "'.'");
      else
        {
        edwh(&szListBuf[lpOBJ], value);
        // If an undefined symbol was detected, don't show illegal value 
        // if Warning don't give error 
        if (error != 6 && error != 7) // Division by zero
          {
          warno = WARN_MSKVALUE;                                                   
          sprintf(pszWarntextbuf, "0x%llX", qvalue);                                   
          warnSymbol_ptr = pszWarntextbuf;
          }
        } // end else
      edpri();                // Emit Error line
      } // end if (error)

    // Intel: little endian,  Motorola: big endian
    dw_endian();            
    //edpri();                // Emit bytewise (deprecated)
    edpri_dw();               // Emit in word rendition into ListBuf
    swp1err = ERR_P1_OFF;     // It's opportune to Ignore all further Pass1 errors

    cllin();                  // Prepare a new ListBuf line
    edipc();                  // Emit current pc
    lineAppendCRLF(&szListBuf[lpXSEG+2]);

    if (*_inbufDW_ptr == ';') break;
    else _inbufDW_ptr++;   
    } // end while            // Resume parsing DW line of inbuf

  swp1err ^= ERR_P1_OFF;      // Toggle pass1 error detection = ON
  } // p2dw

//-----------------------------------------------------------------------------
//
//                               p2db
// DB Directive - Define BYTE
// input: fopcd, inbuf
//
// Allowable forms are:
//  - numeric constant (dec or hex)  (oct, bin - not supported)
//  - symbols
//  - Program Counter ($)
//  - ASCII strings in single quotes
// Several of these may be combined with operators.
// There are certain constraints in which types of symbols can be combined.
//
void p2db()
  {
  static char _operDB[INBUFLEN];
  static int bytLstCount = 5;

  char* _tmpPtr = NULL;

  // 00000000  01 23 45 67 89  :C = Listing 'XASMAVR.EXE' (.MODEL BYTE)
  // 00000000  2301 6745 AB89  :C = Listing 'XASMAVR.EXE' (.MODEL WORD)
  if (AtmelFlag != 0 && swmodel == _WORD) bytLstCount = 6;

  ULONG pccDB = pcValue;  // Save for later

  p2LineContinuation();

  if (AtmelFlag != 0 && SegType == _DATA)
    {
    errorp2(ERR_DSEG, NULL);
    edpri_db();
    return;
    }

  // Scan to 1st .DB, DB, FCB element which is in 'fopcd'
  char* _inbufDB_ptr = StrStrI(inbuf, fopcd);  
  if (!delm2(&_inbufDB_ptr[strlen(fopcd)])) ++_inbufDB_ptr = StrStrI(_inbufDB_ptr, fopcd);  
  _inbufDB_ptr += strlen(fopcd);
  _inbufDB_ptr = skip_leading_spaces(_inbufDB_ptr);

  erno = ERR_NONE;  // Clear any pending errors

  // -----------------------------------------------------
  // Mixed line; Strings, Expressions, Comment
  // String "'" found in this DB line (including comment!) 
  // Comment ";" "," may be found within DB string!
  // Example: 'V',123, '.', 123 ==> ,'..  
  //          ..string after comma - one increment only
  //
  swstx = 0;            // Default: "no string in progress"

  while (*_inbufDB_ptr != 0)
    {
    ilen = 0;           // Init DB element count

    for (_i=0; _i< bytLstCount; _i++) insv[_i] = 0; // Init-clear

    // Allow a maximum of 5 chars (elements) in each line of listing
    while (ilen < bytLstCount && *_inbufDB_ptr != 0)
      {
      // Check if Start of STRNG
      if (*_inbufDB_ptr == STRNG && swstx == 0)                              
        {                                                                    
        //--------------------------------------------------------------------
        if (_inbufDB_ptr[1] >  SPACE &&                                      //
            _inbufDB_ptr[2] == STRNG)                                        //
          {                                                                  //
          // In _operand replace all chars w/ Asc2Hex, e.g.:  ['A'] w/ [41h] //
          // Save - gets destroyed by sprintf w 0-terminator                 //
          char _inSav = _inbufDB_ptr[3];                                     //
          sprintf(_inbufDB_ptr, "%02Xh", _inbufDB_ptr[1]);                   //
          // Restore - was overwritten by sprintf w/ 0-terminator            //
          _inbufDB_ptr[3] = _inSav;                                          //
          } // end if                                                        //
        //--------------------------------------------------------------------

        else // is Start of single quote '''
          {
          swstx = STRNG;      // Set "str in progress"
          _inbufDB_ptr++;
          if (*_inbufDB_ptr == STRNG || *_inbufDB_ptr == DQUOTE) // '' or '"
            {
            insv[++ilen] = 0;      // NULL string
            *_inbufDB_ptr++;       // Store a string character                          
            swstx = 0;             // reset
            }
          }
        } // end if (STRNG)

      // Check if Start of double quote '"'
      else if (*_inbufDB_ptr == DQUOTE && swstx == 0)
        {
        swstx = DQUOTE;      // Set "str in progress"
        _inbufDB_ptr++;
        if (*_inbufDB_ptr == DQUOTE || *_inbufDB_ptr == STRNG) // "" or "'
          {
          insv[++ilen] = 0;      // NULL string
          *_inbufDB_ptr++;       // Store a string character                          
          swstx = 0;             // reset
          }
        }

      // ----------------
      // Process a string
      // ----------------
      if (swstx != 0)
        { 
        insv[++ilen] = *_inbufDB_ptr++;       // Store a string character                          
        }

      // ---------------------
      // Process an expression
      // ---------------------
      else if (*_inbufDB_ptr != CR && 
               *_inbufDB_ptr != LF && 
               *_inbufDB_ptr != 0)
        {
        // Build current operand separately
        for (_i=0; _i<OPERLEN; _i++)
          {
          if (*_inbufDB_ptr == COMMA || (*_inbufDB_ptr == ';' && swstx == 0) || *_inbufDB_ptr == 0)
            {
            _operDB[_i] = 0;                  // terminate
            break;
            }
          else _operDB[_i] = *_inbufDB_ptr++; // Copy
          } // end for

        if (_operDB[0] != 0)                  // Skip if no more DB elements
          {
          if (!CheckRadixSyntax(_operDB)) return;

          error = 0;
          value = expr(_operDB);                                          

          ilen++;                // Count DB elements
          insv[ilen] = value;    // Store DB element value

          // Byte is limited -256 <= lvalue <= 255
          if (error == 0 && !((lvalue & 0xFFFFFF00) == 0xFFFFFF00) && value > 255)
            errorp2(ERR_ILLBYTE, pszErrtextbuf);

          // More than one .(directive) in same line
          else if (erno != 0 && strstr(_operDB, "."))
            {
            if ((_tmpPtr=strstr(pszErrtextbuf,"'")) == NULL)  
              {
              GetPcValue();      // Prevent pass1 phase error   
              pcValue--;
              AssignPc();   
              }
            errorp2(ERR_ILLOPER, "'.'");  // or this one
            //errorp2(ERR_ILLOPER, pszErrtextbuf);
            // Skip .DB directive
            _tmpPtr = StrStrI(inbuf, fopcd);
            _tmpPtr += strlen(fopcd);
            _tmpPtr = StrStrI(_tmpPtr, ".");
            // Skip 2nd (illegal) .directive
            while (*_tmpPtr > SPACE) _tmpPtr++;
            //  (synchronize with pass1 - prevents pass1 phase  error)
            // May be something more to evaluate
            _inbufDB_ptr = _tmpPtr;
            _inbufDB_ptr = skip_leading_spaces(_inbufDB_ptr);

            } // end else if ('.')

          // Error result from expr()
          else if (erno != 0) errorp2(erno, pszErrtextbuf);
          } // end if (_operDB)
        } // end else if                                                

      // Check if end of STRNG
      if (swstx != 0)
        {     
        if (*_inbufDB_ptr == (char)swstx)     
          {
          swstx = 0;            // Stop str processing
          _inbufDB_ptr++;  
          _inbufDB_ptr = skip_leading_spaces(_inbufDB_ptr);  // Skip spaces after end of STRNG

          if (*_inbufDB_ptr != COMMA &&                         //ha// nasty stuff
              *_inbufDB_ptr > SPACE  &&                         //ha//
              *_inbufDB_ptr != ';')                             //ha// comment is allowed
            {                                                   //ha//
            sprintf(pszErrtextbuf, "\x22 %s", _inbufDB_ptr);    //ha// just to show what's wrong
            errorp2(ERR_ILLEXPR, pszErrtextbuf);                //ha//
            }                                                   //ha//
          }
        }                

      // Check if end of this expression, skip COMMA - more expressions follow
      if (*_inbufDB_ptr == COMMA && swstx == 0)
        { 
        _inbufDB_ptr++;
        _inbufDB_ptr = skip_leading_spaces(_inbufDB_ptr);  // Skip spaces after delim
        }

      // Drop out if reached end of relevant info in line
      if (*_inbufDB_ptr == CR  ||
          *_inbufDB_ptr == LF  || 
          *_inbufDB_ptr == 0   || 
          (*_inbufDB_ptr == ';' && swstx == 0))
        break;
      } // end while (ilen < bytLstCount)
        // ------------------------------

    edpri_db();              // Emit insv[1..5] elements into ListBuf
    swp1err = ERR_P1_OFF;    // It's opportune to Ignore all further Pass1 errors

    // Check if reached end of relevant info in line
    if (*_inbufDB_ptr == CR  || 
        *_inbufDB_ptr == LF  || 
        *_inbufDB_ptr == 0   || 
        (*_inbufDB_ptr == ';' && swstx == 0))
      {
      break;
      } // end while (*_inbufDB_ptr!=0)
        // ----------------------------

    // Prepare for a possible next line of DB elements
    else
      {
      cllin();              // print more on a new line
      edipc();
      lineAppendCRLF(&szListBuf[lpXSEG+2]);
      ilen = 0;             // Reset for new line
      } // end else

    } // end while (*_inbufDB_ptr!=0)

  // Only for AVR assembly: Code must be aligned 2 (16bit boundary)
  GetPcValue();
  strlenDB = pcValue-pccDB; // calc for STRLEN(expr) function

  if (AtmelFlag != 0 && SegType == _CODE && (pcValue % 2) != 0)  
    {
    cllin();                // Print an aligning zero on a new line
    edipc();
    insv[1] = 0;            // Alignment needed for AVR 16bit architeczure
    ilen = 1;               // One char (zero padding)
    sprintf(&szListBuf[lpSRC], "\t.EVEN\r\n");
    edpri_db();             // Emit an aligning zero into ListBuf
    } // end if (AtmelFlag)

  swp1err ^= ERR_P1_OFF;    // Toggle pass1 error detection = ON

  if (swstx != 0) 
    {
    errorp2(ERR_ILLBYTE, NULL);
    erout();                // print error line (needed in macro expansion)
    }
  } // p2db


//-----------------------------------------------------------------------------
//
//                               p2equ
// EQU Directive
// input: oper1, flabl, symboltab_ptr, ListBuf
//
//  typedef struct tagSYMBOLBUF {   // Symbol structure
//    char  symString[SYMLEN+1];    // Symbol/Label name  string
//    ULONG symAddress;             // Symbol value or pointer to operand string
//    UCHAR symType;                // Symbol type (ABS, C-SEG, UNDEFINED)
//  } SYMBOLBUF, *LPSYMBOLBUF;      // Structured symbol buffer
// 
void p2equ()
  {
  char* tmpPtr;

  ULONG _value;
  int _rsltLabfn;

  clepc();

  ////////////////////////////////////////////////////////////////
  // re-define an #undef'd symbol                               //
  if (p12Cundef(_PASS1|_PASS2) == TRUE) { edpri(); return; }    //
  ////////////////////////////////////////////////////////////////

  // Atmel AVRASM2 compatible: Allow something like : ".EQU     REV01 = 2+$AA"
  if (AtmelFlag != 0) SyntaxAVRtoMASM(_PASS2);

  // #define  .DEFINE (also for Motorola) //ha//
  // Atmel AVRASM2 compatible: Allow something like: "#define  REV01"
  //                                                 "#define  REV01 123"
  // >                                                fopcd    flabl  (inbuf)
  // Note: some assemblers also allow '.DEFINE' (not specified by Atmel)
//ha//  if (AtmelFlag != 0 && oper2[0] == 0 && StrCmpI(fopcd, &_defin_str[1]) == 0)
  if ((AtmelFlag != 0 || MotorolaFlag != 0) && oper2[0] == 0 &&  //ha//
      StrCmpI(fopcd, &_defin_str[1]) == 0)
   {
    tmpPtr = strstr(inbuf, flabl);
    while (*tmpPtr > SPACE) tmpPtr++;
    tmpPtr = skip_leading_spaces(tmpPtr);
    _i=0;
    while (tmpPtr[_i] != 0)
      {
      if (tmpPtr[_i] == ';') break;
      oper1[_i] = tmpPtr[_i]; 
      _i++;
      }
    oper1[_i] = 0;

    if (oper1[0] == 0) { oper1[0] ='1'; oper1[1] = 0; } // set value to "1"
    } // end if (AtmelFlag)

  // STRLEN(..)
  if ((tmpPtr = StrStrI(oper1, "STRLEN(")) != NULL)
    {
    if (tmpPtr[7] == ')')        // STRLEN() 
      {
      edwh(oper1, strlenDB);     // From previous .DB
      oper1[4] = 'h';
      oper1[5] = 0;
      }
    else if (tmpPtr[7] == '\x22' && tmpPtr[strlen(tmpPtr)-2] == '\x22'&& tmpPtr[strlen(tmpPtr)-1] == ')')
      {
      edwh(oper1, strlen(&tmpPtr[8])-2);       // From previous .DB
      oper1[4] = 'h';
      oper1[5] = 0;
      }
    } // end else if ("STRLEN(")               // error handling see dcomp.cpp

  // At this point (forward reference example):
  // inbuf=[.EQU Symbol1 = p1undefSymbol]
  // oper1=p1undefSymbol
  // _value = expr(oper1); --> (error=0: 'p1undefSymbol' later defined in Pass1; error=1 still undefined)
  //   symboltab_ptr->symString=_Symbol1
  //   symboltab_ptr->symAddress=00000000
  //   symboltab_ptr->symType=8B
  // Must be before labfn(flabl) (destroys symboltab_ptr)
  //
  if (oper1[0] == 0) _value = 0;  // suppress error from expr() if oper1[0] = 0
  else 
    { 
    _value = expr(oper1); 
    if (erno != 0) errorp2(erno, oper1); // error from expr()
    }
 
  _rsltLabfn = labfn(flabl);             // Get 'symboltab_ptr'
 
  if (symboltab_ptr->symType == _ABSSET) errorp2(ERR_DBLSYM, flabl);

  SymType = _ABSEQU;  //ha//
 
  // Check if undefined in PASS1 (forward reference)
  // If found in PASS2 the symbol can be evaluated.
  // In this case an error code is not necessary.
  // May cause problem with "DS, RMB, BYTE" directives in certain cases,
  // that's why an error is still issued in pass1 'p1equ()'.
  //
  if (_rsltLabfn != FALSE && symboltab_ptr->symType == (UCHAR)(SYMERR_FLAG | ERR_DBLSYM))
    errorp2(ERR_DBLSYM, symboltab_ptr->symString);

  else if (
           _rsltLabfn != FALSE     &&
           error == 0 && erno == 0 &&
           (symboltab_ptr->symType == (UCHAR)(SYMERR_FLAG | ERR_UNDFSYM) || oper1[0] == _PCsymbol)
          )
    {
    symboltab_ptr->symAddress = _value; // Enter current Pass2 value
    symboltab_ptr->symType = SymType;   // Set valid type byte
    }

  szListBuf[lpLOC+0] = EQUATE_IDM;      // Special Marker displayed for EQUs
  szListBuf[lpLOC+1] = SPACE;

  if (AtmelFlag != 0 || MotorolaFlag != 0)
    eddh(&szListBuf[lpLOC+2], _value);  // 32bit
  else
    edwh(&szListBuf[lpLOC+2], _value);  // 16bit

  edpri();                              // Emit line to listing 
  } // p2equ


//-----------------------------------------------------------------------------
//
//                               p2set
// SET Directive
// DEF Directive
// Input: oper1, symboltab_ptr, p1undef_ptr, errSymbol_ptr
//
void p2set()
  {
  ULONG _value;

  clepc();  // Clear OBJ field in listing

  // Atmel AVRASM2 compatible: Allow something like : ".SET     REV01 = 2+$AA"
  if (AtmelFlag != 0) SyntaxAVRtoMASM(_PASS2);

  // Must be before labfn(flabl) (destroys symboltab_ptr)
  if (oper1[0] == 0) _value = 0; // suppress error from expr()  if oper1[0] = 0
  else _value = expr(oper1); 

  if (error != 0 && erno == 0) errorp2(ERR_UNDFSYM, NULL);

  labfn(flabl);  // Get 'symboltab_ptr'

  if (symboltab_ptr->symType == _ABSEQU) errorp2(ERR_DBLSYM, NULL);
  else
    {
    symboltab_ptr->symAddress = _value;  // Enter current Pass2 value
    symboltab_ptr->symType = _ABSSET;    // Set valid type byte
    }

  szListBuf[lpLOC+0] = EQUATE_IDM;       // Special Marker displayed for EQUs
  szListBuf[lpLOC+1] = SPACE;

  if (AtmelFlag != 0 || MotorolaFlag != 0)
    eddh(&szListBuf[lpLOC+2], _value);   // 32bit
  else
    edwh(&szListBuf[lpLOC+2], _value);   // 16bit
  edpri();                               // Print .<directive> src-line
  } //p2set

//-----------------------------------------------------------------------------

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("fopcd=%s  ins_group=%d  swlistmac=%04X  swlistmacstatement=%d\nszListBuf=%s",
//ha//        fopcd,    ins_group,    swlistmac,      swlistmacstatement,   szListBuf);
//ha//DebugStop(3, "pass2()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//if (swpass == _PASS2)
//ha//{
//ha//printf("_operStr=%s  _tmpPtr=%s  _l=%d  _m=%d  _n=%d",
//ha//        _operStr,    _tmpPtr,    _l,    _m,    _n);
//ha//DebugStop(1, "replace_MotorolaBin()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("error=%d  erno=%d  lvalue=%08X  (int)lvalue=%08X  value=%08X",
//ha//        error,    erno,    lvalue,      (int)lvalue,      value);
//ha//DebugStop(1, "p2db()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("flabl=%s  _rsltLabfn=%02X error=%d  erno=%d\n"
//ha//       " symboltab_ptr->symString=%s\n"
//ha//       " symboltab_ptr->symAddress=%08X\n"
//ha//       " symboltab_ptr->symType=%02X\n",
//ha//        flabl,    _rsltLabfn,     error,    erno,
//ha//         symboltab_ptr->symString,
//ha//         symboltab_ptr->symAddress,
//ha//         symboltab_ptr->symType); 
//ha//DebugStop(2, "p2equ()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//if (_debugSwPass == _PASS2)
//ha//{
//ha//printf("erno=%d  errSymbol_ptr=%s [%08X]", erno, errSymbol_ptr, errSymbol_ptr);
//ha//if (errSymbol_ptr != NULL)
//ha//  {
//ha//  printf("\nerrSymbol_ptr ");
//ha//  DebugPrintBuffer(errSymbol_ptr, OPERLEN/3);
//ha//  }
//ha//DebugStop(2, "p2set()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("oper1=%s  strlen(tmpPtr)-1=%d\n", oper1, strlen(tmpPtr)-1);
//ha//printf("tmpPtr ");
//ha//DebugPrintBuffer(tmpPtr, OPERLEN/2);
//ha//DebugStop(1, "p2equ()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("pcValue=%d  flabl=%s  fopcd=%s  ilen=%d  _operDB_ptr=%s\n_inbufDB_ptr=%s\n",
//ha//        pcValue   , flabl,    fopcd,    ilen,    _operDB_ptr,    _inbufDB_ptr);
//ha//printf("_inbufDB_ptr ");
//ha//DebugPrintBuffer(_inbufDB_ptr, OPERLEN);
//ha//DebugStop(1, "p2db()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("_dirStr=%s  ins_group=%d insv[1]=%d  flabl=%s  fopcd=%s  oper1=%s  oper2=%s\n",
//ha//        _dirStr,    ins_group,   insv[1],    flabl,    fopcd,    oper1,    oper2);
//ha////printf("oper1 ");
//ha////DebugPrintBuffer(oper1, OPERLEN);
//ha//DebugStop(1, "p2pse()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("ins_group=%d insv[1]=%d  flabl=%s  fopcd=%s  oper1=%s  oper2=%s\n",
//ha//        ins_group,   insv[1],    flabl,    fopcd,    oper1,    oper2);
//ha////printf("oper1 ");
//ha////DebugPrintBuffer(oper1, OPERLEN);
//ha//DebugStop(1, "pass2()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("flabl=%s  fopcd=%s  oper1=%s\np2swelse=%d  p2swif=%d p2swifdef=%d  ifCount=%d  swif=%d\n ",
//ha//        flabl,    fopcd,    oper1,    p2swelse,    p2swif,   p2swifdef,    ifCount,    swif);  
//ha////printf("oper1 ");
//ha////DebugPrintBuffer(oper1, OPERLEN);
//ha//DebugStop(2, "p2endif()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//{
//ha//printf("pcc=%04X error=%d  insv[%d]=%02X  _inbufDB_ptr=%s",
//ha//        pcc, error, ilen, insv[ilen], _inbufDB_ptr);
//ha//printf("_operDB_ptr ");   
//ha//DebugPrintBuffer(_operDB_ptr, (OPERLEN/4)*3);
//ha//printf("_operDB ");   
//ha//DebugPrintBuffer(_operDB, (OPERLEN/4)*3);
//ha//DebugStop(1, "pass2()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//if (_debugSwPass == _PASS2)
//ha//{
//ha//printf("[1] inbuf_ptr=%s\n", inbuf_ptr);
//ha//printf("flabl=%s   pcc=%04X   lastCharAL=[%02X]   ins_group=%d\n",
//ha//        flabl,     pcc,       lastCharAL,lastCharAL, ins_group);
//ha////ha//DebugPrintBuffer(flabl, SYMLEN);
//ha//printf("fopcd=%s\n", fopcd);
//ha////ha//DebugPrintBuffer(fopcd, OPCDLEN);
//ha//printf("oper1=%s\n", oper1);
//ha////ha//DebugPrintBuffer(oper1, OPERLEN);
//ha//printf("oper2=%s\n", oper2);
//ha////ha//DebugPrintBuffer(oper2, OPERLEN);
//ha//printf("oper3=%s", oper3);
//ha//DebugStop(1, "pass2()", __FILE__);
//ha////ha//DebugPrintBuffer(oper3, OPERLEN);
//ha//DebugStop(1, "pass2()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//DebugPrintSymbolArray(3);  
//ha//DebugStop(1, "pass2()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("fopcd=%s  oper1=%s  tmpPtr=%s  pcd=%04X\n ", fopcd, oper1, tmpPtr, pcd);
//ha//DebugStop(1, "p2pragma()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//--------------------------end-of-c++-module-----------------------------------

