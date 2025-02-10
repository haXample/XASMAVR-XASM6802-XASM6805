// haXASM - Cross-Assembler for 8bit Microprocessors
// pass1.cpp - C++ Developer source file.
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
char * _dirStr;    // Temporary pointer 

ifstream IclFile;      // File read (*.INC)

// Extern variables and functions
extern void DebugStop(int, char*, char*); // Usage: DebugStop(testNr, "Function()", __FILE__);
extern void DebugPrintBuffer(char*, int); // Usage: DebugPrintBuffer(buffer, count);
extern void DebugPrintSymbolArray(int);   // Usage: DebugPrintSymbolArray(count);

extern char* MODEL_str;
extern char* TITLE_str;
extern char* SUBTL_str;
extern char* MSG_str;
extern char* ERR_str;
extern char* PW_str;
extern char* PL_str;
extern char* NOLST_str;
extern char* NOSYM_str;
extern char* ICL_str;
extern char* EJECT_str;
extern char* LST_str;
extern char* DB_str;
extern char* _defin_str;
extern char* DEFIN_str;
extern char* DEF_str;
extern char* DEVICE_str;
extern char* CSEGSIZ_str;
extern char* EVEN_str;

extern char szIclFileName[];  // Current inlude file name (*.INC)
extern char* pszCurFileName;

extern char* szErrorOpenFailed;

extern char* skip_leading_spaces(char*);
extern char* skip_trailing_spaces(char*);
extern void lineAppendCRLF(char*);

extern UCHAR labfn(char*);
extern BOOL p1lab();
extern BOOL delm2(char*);

extern int opcod();
extern UINT expr(char*);
extern void AssignPc();
extern void GetPcValue();

extern void Decomp();
extern void p1errorl(UCHAR, char*);
extern void errorp2(int, char*);

extern void p1eval();

extern BOOL OpenIncludeFile(char*, char*);
extern void CloseIclFile();

extern void read_source_line();
extern ifstream SrcFile;    // Filestream read  (*.ASM)

// Conditional assembly and pre-processing functions
extern BOOL p12Cundef(int);
extern void p12Cifdef(int);
extern void p12Cifndef(int);
extern void p12Cif(int);
extern void p12Celse(int);
extern void p12Celseif(int);
extern void p12Cendif(int);
extern int p12CifelseSkipProcess(int);

extern void p12macro(int);
extern void p12DefineMacro(int);
extern BOOL p12ExpandMacro(int);
extern void p12endmacro();
extern void edwh(char*, UINT);

extern void SyntaxAVRtoMASM(int); // For Atmel's syntax
extern void EvalDeviceAVR(int);

// Forward declaration of functions included in this code module:
void titleXfer(char*, char*, char*), lhead();

//void p1ext();
//void p1pub();
BOOL p1pse();
void p1org();
void p1dq();
void p1dd();
void p1dw();     // Motorola: FDB
void p1db();     // Motorola: FCB
void p1ds();     // Motorola: RMB
void p1end();
void p1equ();
void p1set();

void p1device(); // For Atmel's various µC cores
void p1pragma(); // For Atmel's various µC cores

//-----------------------------------------------------------------------------
//
//                      pass1
//
// source is read until 'END' statement or
// end-of-file, symbol table and error table are created
//
void pass1()
  {
  _debugSwPass = _PASS1; // Flag indicates processing Pass1
  swpass = _PASS1;       // Flag indicates processing Pass1

PASS1:
  Decomp();

  _dirStr = skip_leading_spaces(inbuf);  // Prepare inbuf

  opcod();  // Load insv[1], ins_group, flabl, fopcd, oper1..4, ..

  ////////////////////////////////////////////////////////////////////////////////
  //                                                                            //
  // Macro processing Pass1: Macro definition                                   //
  //                                                                            //
  if (swdefmacro)                   // .MACRO (unitl .ENDMAC/.ENDMACRO)         //
    {                                                                           //
    // Collect all macro definition lines in a struct array                     //    
    p12CifelseSkipProcess(_PASS1);  // Get .IF-ELSE condition within macro      //
    p12DefineMacro(_PASS1);                                                     //
    goto PASS1;                     // Resume without any further processing    //
    }                                                                           //
  ////////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////////
  //                                                                            //
  // Conditionally suppressed via .IF .IFDEF .IFNDEF .EL(SE)IF .ELSE .ENDIF     //
  // Conditionally suppressed via #if #ifdef #ifndef #el(se)if #else #endif     //
  //                                                                            //
  int _rsltIfElse = p12CifelseSkipProcess(_PASS1); // Get .IF-ELSE condition    //
  if (_rsltIfElse == TRUE) goto PASS1;             // Skip src line             //
  ////////////////////////////////////////////////////////////////////////////////

  // Normal unconditional processing
  else
    {
    // Comment line
    if (swcom != 0) goto PASS1;

    p1lab();  // Handle label if present //ha//
  
    ////////////////////////////////////////////////////////////////////////////////
    if (ins_group == ERR)                                                         //
      {                                                                           //
      if (p12ExpandMacro(_PASS1) == TRUE) goto PASS1;                             //
      }                                                                           //
    ////////////////////////////////////////////////////////////////////////////////

    // Check opcode
    if (fopcd[0] == 0) goto PASS1;      // Next line if opcode is empty.             

    p1eval();                           // Further evaluation done in BYTVAL.CPP

    // Done, if not pseudo instructions and directives
    if (ins_group != 1) goto PASS1;   
    else if (p1pse() == FALSE) return;  // END: Finish and return from pass1  
    
    else goto PASS1;                    // Next line
    } // end else
  } // pass1


//-----------------------------------------------------------------------------
//
//                               p1pse
//
BOOL p1pse()
  {
  char* tmpPtr = NULL;
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("MODEL_str=%s  _dirStr=%s  insv[1]=%d  swmodel=%04X",
//ha//        MODEL_str,    _dirStr,    insv[1],    swmodel);  
//ha//DebugStop(1, "pass1()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

  // PASS1: Pseudo Instructions and Directives
  switch(insv[1])
    {
    case 0:     // Process next line if opcode is empty.
      break;    
    case 2:     // {4,"ORG"   ,1, 2},
      p1org();          
      break;            

    case 3:     // {4,"MODEL" ,1, 2}, "MODEL" Directive
      if (AtmelFlag != 0)
        {
        tmpPtr = &_dirStr[strlen(MODEL_str)+1];
        // Comment is allowed, garbage issues an error
        if (StrCmpNI(tmpPtr, "WORD", 4) == 0)
          {
          tmpPtr = skip_leading_spaces(&tmpPtr[4]);
          if (*tmpPtr != 0 && *tmpPtr != ';') p1errorl(ERR_ILLDIR, &_dirStr[strlen(MODEL_str)+1]);
          else { swmodel &= ~_BYTE; swmodel |= _WORD; }
          }
        else if (StrCmpNI(tmpPtr, "BYTE", 4) == 0)
          {
          tmpPtr = skip_leading_spaces(&tmpPtr[4]);
          if (*tmpPtr != 0 && *tmpPtr != ';') p1errorl(ERR_ILLDIR, &_dirStr[strlen(MODEL_str)+1]);
          else { swmodel &= ~_WORD; swmodel |= _BYTE; }
          }
        else if (StrCmpNI(tmpPtr, "SYNTAX", 6) == 0)
          {
          tmpPtr = skip_leading_spaces(&tmpPtr[6]);
          if (*tmpPtr != 0 && *tmpPtr != ';') p1errorl(ERR_ILLDIR, &_dirStr[strlen(MODEL_str)+1]);
          else swmodel |= _SYNTAX;
          }
        else if (StrCmpNI(tmpPtr, "NOINFO", 6) == 0)
          {
          tmpPtr = skip_leading_spaces(&tmpPtr[6]);
          if (*tmpPtr != 0 && *tmpPtr != ';') p1errorl(ERR_ILLDIR, &_dirStr[strlen(MODEL_str)+1]);
          else swmodel |= _NOINFO;
          }
        else 
          p1errorl(ERR_ILLDIR, &_dirStr[strlen(MODEL_str)+1]);

        // Override .MODEL w/ command line option anyway
        if (cmdmodel != -1) 
          { swmodel &= ~(_BYTE | _WORD); swmodel |= cmdmodel; } 
        } // end if (AtmelFlag)
      break;            

    case 4:     // {5,"CSEG"  ,1, 4}, "CSEG" Directive
      SegType = _CODE;      
      break;            
    case 6:     // {5,"DSEG"  ,1, 6}, "DSEG" Directive       
      SegType = _DATA;      
      break;
    case 8:     // {5,"ESEG"  ,1, 8}, "ESEG" Directive (Atmel EEPROM Data)       
      SegType = _EEPROM;      
      break;
    case 10:    // {3,"DW"    ,1,10}, (Define Word)      
      p1dw();   // Motorola: FDB      (Form Double Byte Constant)
      break;    
    case 12:    // {3,"DB"    ,1,12}, (Define Byte)
      p1db();   // Motorola: FCB      (Form Constant Byte)
      break;
    case 14:    // {3,"DS"    ,1,14}, (Define Storage)
      p1ds();   // Motorola: RMB      (Reserve Memory Block)
      break;
    case 16:    // {4,"END"   ,1,16}, "END" directive is encountered
      if (AtmelFlag != 0 && swicl != 0)     // filestream .INC
        {
        // The EXIT directive tells the Assembler to stop assembling the file.
        // Normally, the Assembler runs until end of file (EOF).
        // If an EXIT directive appears in an included file, the Assembler
        // continues from the line following the INCLUDE directive in the file
        // containing the INCLUDE directive.
        //
        // Issue at least a "WARNING - Check End-Of-File directive",
        // since this is no good programming practice and not recommended.
        CloseIclFile();           // Stop assembling file .INC
        } // end if (AtmelFlag)
      else return(FALSE);   // Pass 1 completed - return from pass1
      break;
    case 18:
      p1equ();  // {4,"EQU"   ,1,18},
      break;
    case 20:    // {6,"EXTRN" ,1,20},
      p1errorl(ERR_SYNTAX, NULL);
      break;
    case 22:    // //{6,"PUBLC" ,1,22},
      p1errorl(ERR_SYNTAX, NULL);
      break;

    case 24:    // {TITLE_str,1,24},  "TITLE" Directive       
      if (lhtit[0] != 0) return(TRUE);      // Only 1st appearing title in PASS1
      else titleXfer(_dirStr, TITLE_str, lhtit);
      break;                                             
    case 26:    // {SUBTL_str,1,26},  "SUBTTL" Directive        
      break;                                              
    case 27:    // {MSG_str,  1,27},  ".MESSAGE" ".WARNING and ".ERROR" Directives
      if ((tmpPtr = strstr(_dirStr, "\x22")) == NULL)// ||         // find 1st '"'
        {
        p1errorl(ERR_SYNTAX, "Missing double quote(s) \x22\x22");
        return(TRUE);
        }
      *(tmpPtr-1) = 0;                                         // 0-terminator
      printf("%s(%d): %s", pszCurFileName, _curline, _dirStr); // Print directive
      printf(" %s", tmpPtr);
      // If .ERROR  "...\n" abort assembly, no listing file
      if (toupper(_dirStr[1]) == 'E') exit(SYSERR_ABORT);
      break;
    case 28:    // {PW_str,   1,28},  "PAGEWIDTH(" Directive        
      linel = atoi(&_dirStr[strlen(PW_str)]);
      break;                                              
    case 30:    // {PL_str,   1,30},  "PAGELENGTH(" Directive       
      pagel = atoi(&_dirStr[strlen(PL_str)]);
      break;

    case 32:    // {ICL_str,  1,32},  "INCLUDE(" Directive       
      if (OpenIncludeFile(ICL_str, _dirStr) != 0); // Open include file
      else p1errorl(ERR_SYNTAX, NULL);             // Open failure
      break;

    case 34:    // {NOLST_str,1,34},  "NOLIST" Directive       
      break;
    case 36:    // {LST_str,  1,36},  "LIST" Directive       
      break;
    case 38:    // {NOSYM_str,1,38}, {SYM_str,1,38}, ".NOSYMBOLS/.SYMBOLS" Directives       
      if (toupper(_dirStr[1]) == 'N' && cmdnosym == -1) nosym = 1;
      // .SYMBOL also lists symbols if errors (nosym=2). Default: nosym=0
      if (toupper(_dirStr[1]) == 'S' && cmdnosym == -1) nosym = 2;
      else if (cmdnosym != -1) nosym = cmdnosym; // '/S' from cmd-line
      break;
    case 40:    // {EJECT_str,1,40},  "EJECT" Directive       
      break;

    // The DEVICE directive allows the user to tell the Assembler which device
    // the code is to be executed on. Using this directive, a warning is issued
    // if an instruction not supported by the specified device occurs.
    // If the Code Segment or EEPROM Segment are larger than supplied by
    // the device, a warning message is given. If the directive is not used,
    // it is assumed that all instructions are supported and that there are
    // no restrictions on Program and EEPROM memory.
    //
    case 41:    // {".DEVICE",1,0x2929}, ".DEVICE" Directive (Atmel AVR specific)
      _i = 0;
      while (_dirStr[strlen(DEVICE_str)+1 + _i] > SPACE) _i++; // find end of device name
      _dirStr[strlen(DEVICE_str)+1+_i] = 0;                    // 0-terminate
      StrCpy(partNameAVR, &_dirStr[strlen(DEVICE_str)+1]);     // Save partNameAVR string
      EvalDeviceAVR(_PASS1);                                   // Evaluate device properties 
      break;
    case 42:    // {DD_str,   1,42}, (Define Double Word 32bit)         
      p1dd();
      break;
    case 43:    // {SET_str,  1,43},  ".SET" Directive       
      p1set();
      break;

    case 44:    // {""     ,1,44}, Reserve         
      break;

    case 45:    // {""     ,1,45}, Undefine .DEF'ed regs (Ignored / Warning: not implemented) 
      break;

    case 46:    // {_undef_str   ,1,46}, "#undef" Directive         
      p12Cundef(_PASS1);
      break;
    case 47:    // {DQ_str,   1,47}, (Define Quad Word 64bit)         
      p1dq();
      break;
    case 48:    // {CSEGSIZE,     1,48}, ".CSEGSIZE Directive (program memory size)         
      RomSize = atoi(&_dirStr[strlen(CSEGSIZ_str)+1]) * 2 * 1024;
      break;
    case 49:    // // {EVEN,     1,49}, ".EVEN Directive (align labels to WORD boundaries)         
      if ((pcValue % 2) != 0)  
        {
        GetPcValue();
        pcValue++;
        AssignPc();
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
    case 51:    // {LSTMAC_str ,1,51}, ".LISTMACRO" Directive         
      // set swlistmac flag in Pass2
      break;
    case 52:    // {MACRO_str  ,1,52}, ".MACRO" Directive                
      //ha//if ((p12CSkipFlag & TRUE) == TRUE)
      p12macro(_PASS1);
      break;
    case 53:    // {ENDM_str   ,1,53}, ".ENDM/.ENDMACRO" Directive               
      //ha//if ((p12CSkipFlag & TRUE) == TRUE)
      p12endmacro();
      break;

    case 54:    // {_pragm_str, 1,54}, "#pragma" Directive
      p1pragma();      
      break;

    case 55:    // {OVLAP_str,  1,,55}, ".OVERLAP" Org address overlap allowed
      break;
    case 56:    // {NOOVLAP_str,1,56},  ".NOOVERLAP" Org address may not overlap        
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
      p12Cif(_PASS1);
      break;
    case 58:    // {_else_str   ,1,58}, "#else" Directive         
      p12Celse(_PASS1);
      break;
    case 59:    // {_elif_str   ,1,59}, "#elif/.elseif" Directive         
      p12Celseif(_PASS1);
      break;
    case 60:    // {_ifdef_str  ,1,60}, "#ifdef" Directive         
      p12Cifdef(_PASS1);
      break;
    case 61:    // {_ifndef_str ,1,61}, "#ifndef" Directive         
      p12Cifndef(_PASS1);
      break;
    case 62:    // {_endif_str  ,1,62}, "#endif" Directive         
      p12Cendif(_PASS1);
      break;

    default:
      p1errorl(ERR_SYNTAX, NULL);
      break;
    } // end switch

  return(TRUE); // Return to pass1
  } // p1pse

//-----------------------------------------------------------------------------
//
//                               p1pragma
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
// fopcd = "#pragma"
// oper1 = "AVRPART MEMORY PROG_FLASH 8192" (example)
//
void p1pragma()
  {
  char* tmpPtr = NULL;
  int _segVal;

  char* pragmaAVRPART    = "AVRPART";
  char* pragmaROMSIZE    = "MEMORY PROG_FLASH";
  char* pragmaSRAMADDR   = "MEMORY INT_SRAM START_ADDR";
  char* pragmaSRAMSIZE   = "MEMORY INT_SRAM SIZE";
  char* pragmaEEPROMSIZE = "MEMORY EEPROM";

  if ((tmpPtr=StrStrI(oper1, pragmaAVRPART)) != 0)
    {
    // Save current segment
    _segVal = SegType;

    // Preset .DSEG directive
    if ((tmpPtr=StrStrI(oper1, pragmaSRAMADDR)) != 0)
      {
      tmpPtr += strlen(pragmaSRAMADDR);  
      SegType = _DATA;      
      GetPcValue();
      pcValue = expr(tmpPtr);  // Get pcd initial value 
      SRamStart = pcValue;     // Set SRAM start address
      AssignPc();              // Set data segment pcd      
      }
    
    if ((tmpPtr=StrStrI(oper1, pragmaROMSIZE)) != 0)
      {
      tmpPtr += strlen(pragmaROMSIZE)+1;   // +SPACE
      RomSize = expr(tmpPtr);              // Set flash RomSize 
      }

    if ((tmpPtr=StrStrI(oper1, pragmaSRAMSIZE)) != 0)
      {
      tmpPtr += strlen(pragmaSRAMSIZE)+1;  // +SPACE
      SRamSize = expr(tmpPtr);             // Set SRamSize 
      }

    if ((tmpPtr=StrStrI(oper1, pragmaEEPROMSIZE)) != 0)
      {
      tmpPtr += strlen(pragmaEEPROMSIZE)+1; // +SPACE
      EEPromSize = expr(tmpPtr);            // Set EEPromSize 
      }

    // Restore current segment
    SegType = _segVal;           
    } // end if
  } // p1pragma

//-----------------------------------------------------------------------------
//
//                               p1org
// ORG Directive 
// Input: oper1, p1undef_ptr
//
// typedef struct tagOPERXSTR { // Operand structure
//   char operX[OPERLEN+1];     // operand expression string w/ symbols/labels 
//   char symbolX[SYMLEN+1];    // symbolname assigned to value of operX 
//   char errsymX[SYMLEN+1];    // name of an undefined symbol in operX
// } OPERXBUF, *LPOPERXBUF;     // Structured oprand string buffer
//
void p1org()
  {
  LPOPERXBUF tmpp1undef_ptr = p1undef;
  int _i;   // MUST be local (..global _i is used in expr()!)

  GetPcValue();

  for (_i=0; _i<OPERXENTRIES; _i++)
    {
    if (StrCmpNI(oper1, tmpp1undef_ptr->symbolX, strlen(tmpp1undef_ptr->symbolX)) == 0 &&
        strlen(tmpp1undef_ptr->symbolX) > 0)
      {
      // Evaluate original operand expression string
      pcValue = expr(tmpp1undef_ptr->operX); 
      // Give error code + error line to Pass1 error table
      //  if symbol is unknown or forward referenced
      if (_exprInfo == ERR_UNDFSYM) p1errorl(ERR_UNDFSYM, symboltab_ptr->symString);
      break;
      }
    tmpp1undef_ptr++;
    } // end for

  // Normal processing: Evaluate operand symbol/expr if not forward referenced
  if (_i >=OPERXENTRIES) pcValue = expr(oper1);  

  if (AtmelFlag != 0)
    {
    // All Atmel provided .INC-files assume .CSEG "WORD Addressing", so 
    // we must double all .CSEG .ORG addresses since we're always in ".MODEL BYTE".
    //ha//if (swmodel == _BYTE && SegType == _CODE) pcValue *= 2;   
    if (SegType == _CODE) pcValue *= 2;   
    } // end if (AtmelFlag)

  AssignPc();
  } // p1org

//-----------------------------------------------------------------------------
//
//                       p1LineContinuation
//
// Pass1 line continuation for .DD, .DW .DB  directives.
// Point at the last char before any possible
// trailing cntls and spaces at the end of inbuf, and check 
// if the last char is the line continuation '\' or a comma.
// Note: No comment is allowed within a continuated line.
//
void p1LineContinuation()
  {
  static char tmpInbuf[LSBUFLEN];
  static char* tmpInbufPtr = tmpInbuf;
  static char* inbufPtr = NULL;

  static int loopCount = 0;
  while ((*(inbufPtr = skip_trailing_spaces(inbuf)) == '\\' ||
         *skip_trailing_spaces(inbuf) == COMMA)             && 
         strstr(inbuf, ";") == NULL)
    {
    // Eliminate src line continuation char '\' (would cause errors otherwise)
    if (*inbufPtr == '\\') *inbufPtr = SPACE;            

    // Clear tmpInbuf
    for (_i=0; _i<LSBUFLEN; _i++) tmpInbuf[_i] = 0;

    tmpInbufPtr = tmpInbuf;
    StrCpy(tmpInbufPtr, inbuf);              // Save inbuf src line into tmpInbuf
    tmpInbufPtr += strlen(inbuf);            // Advance tmpInbufPtr

    read_source_line();                      // Read inbuf next line after '\'

    inbufPtr = skip_leading_spaces(inbuf);
    StrCpy(tmpInbufPtr, inbufPtr);           // Append inbuf to tmpInbuf
    tmpInbufPtr += strlen(inbufPtr);         // Advance tmpInbufPtr

    if (strlen(tmpInbuf) > INBUFLEN) return; // Abort

    StrCpy(inbuf, tmpInbuf); // Src line concatenation done.
    loopCount++;             // Try and indicate next loop
    } // end While

  lineAppendCRLF(&inbuf[strlen(inbuf)]);        

  } // p1LineContinuation

//-----------------------------------------------------------------------------
//
//                               p1dq
// DQ Directive (Atmel)
// Input: inbuf
//
void p1dq()
  {
  p1LineContinuation();

  GetPcValue();
  pcValue += 8;

  _i=0;
  while (_i < INBUFLEN && inbuf[_i] != ';' && inbuf[_i] != 0)
    {
    if (inbuf[_i] == COMMA) pcValue += 8;
    _i++;
    }
  AssignPc();
  } // p1dq

//-----------------------------------------------------------------------------
//
//                               p1dd
// DW Directive (Motorola: FDB)
// Input: inbuf
//
void p1dd()
  {
  p1LineContinuation();

  GetPcValue();
  pcValue += 4;

  _i=0;
  while (_i < INBUFLEN && inbuf[_i] != ';' && inbuf[_i] != 0)
    {
    if (inbuf[_i] == COMMA) pcValue += 4;
    _i++;
    }
  AssignPc();
  } // p1dd

//-----------------------------------------------------------------------------
//
//                               p1dw
// DW Directive (Motorola: FDB)
// Input: inbuf
//
void p1dw()
  {
  p1LineContinuation();

  GetPcValue();
  pcValue += 2;

  _i=0;
  while (_i < INBUFLEN && inbuf[_i] != ';' && inbuf[_i] != 0)
    {
    if (inbuf[_i] == COMMA) pcValue += 2;
    _i++;
    }
  AssignPc();
  } // p1dw

//-----------------------------------------------------------------------------
//
//                               p1db
//
// Parse Define-Byte-String
// DB Directive (Intel/Microsoft="DB", Atmel=".DB", Motorola="FCB")
// Input: oper1, inbuf
//
// Line continuation '\',CR,LF,0 for .DB directives
//  Example
//   .DB  0, 1, "This is a long string", '\n', 0, 2, \
//        " here continued", '\n', 0, 3, 0
//
// This is another nice Example: 
//   01 0D 30 31 32  .DB  1,13,'012"3"45 ,"', 'A', '"', ''', '', "", 22, 25, 4
//   22 33 22 34 35
//   20 2C 22 41 22
//   27 00 00 16 19
//   04            
//
// And this Example: 
//   01 0D 30 31 32  .DB  1,13,"012'3'45 ,'", 'A', '"', ''', '', "";, 22, 25, 4 \ 
//   27 33 27 34 35  
//   20 2C 27 41 22  
//   27 00 00 
//     
void p1db()
  {
  p1LineContinuation();

  GetPcValue();

  ULONG pccDB = pcValue;  //  Save for later
  int swEol = 0;          // string or expr was last before end-of-line (eol) 
  char* _inPtr = inbuf;
  
  // Init pointer to .DB operands
  _inPtr = StrStrI(_inPtr, fopcd);  
  if (!delm2(&_inPtr[strlen(fopcd)])) ++_inPtr = StrStrI(_inPtr, fopcd);  
  _inPtr += strlen(fopcd);
  _inPtr = skip_leading_spaces(_inPtr);

  swstx=0; // Initially no string assuned
  while (*_inPtr != 0 && !(*_inPtr == ';' && swstx == 0))
    {
    // Check 'string'
    if (*_inPtr == STRNG && swstx == 0)
      {       
      // Check if single quoted char, treated as <expr> (example 'A')
      //  and skip 3 chars: ' A ' (either COMMA or swEol count it as <expr>)  
      if (_inPtr[1] > SPACE && _inPtr[2] == STRNG) _inPtr+=3;

      // Check NULL strings and skip 2 chars: '' or '"
      else if (_inPtr[1] == STRNG || _inPtr[1] == DQUOTE) _inPtr+=2;
      else 
        {
        swstx = STRNG;           // Set 'string in progress'
        swEol = swstx;           // Assume 'string last before eol'
        }
      } // end if (STRNG && !swstx)

    // Check "string"
    else if (*_inPtr == DQUOTE && swstx == 0)
      {
      // Check NULL strings "" or "'       
      if (_inPtr[1] == DQUOTE || _inPtr[1] == STRNG) _inPtr+=2;
      else
        { 
        swstx = DQUOTE;          // Set "string in progress"
        swEol = swstx;           // Assume "string last before eol"
        }
      } // end if (DQUOTE && !swstx)

    // Check end of 'string'
    else if (*_inPtr == STRNG && swstx == STRNG)
      {  
      _inPtr++;                  // skip STRNG '
      swstx = 0;                 // reset 'string' in progress
      }

    // Check end of "string"
    else if (*_inPtr == DQUOTE && swstx == DQUOTE)
      { 
      _inPtr++;                  // skip DQUOTE "
      swstx = 0;                 // reset "string" in progress
      }
    
    // Check if 'string'  or "string" in progress and count 1 char
    else if (*_inPtr != STRNG  && swstx == STRNG)  pcValue++;
    else if (*_inPtr != DQUOTE && swstx == DQUOTE) pcValue++;
    
    // Check if <expr> and count expr as 1 char
    if (*_inPtr == COMMA && swstx == 0)
      {
      if (swEol == 0) pcValue++;  // count 1 char
      swEol = 0;                  // Assume <expr> last before eol
      }

    // Break if rest of line is a comment
    if (*_inPtr == ';' && swstx == 0) break;
    _inPtr++;                     // skip next char and continue
    } // end while                                                    
  
  if (swstx != 0) pcValue--;      // correct phase if no closing QUOTE/DQUOTE
  else if (swEol == 0) pcValue++; // must count last <expr>, if any

  // Only for AVR assembly: Code must be aligned 2 (16bit boundary).
  // Calculation for STRLEN(expr) function (but valuta in pass2)
  strlenDB = pcValue-pccDB; 
  
  if (AtmelFlag != 0 && SegType == _CODE && (pcValue % 2) != 0) pcValue++; 
  AssignPc();
  } // p1db


//-----------------------------------------------------------------------------
//
//                               p1ds
// DS Directive (Motorola: RMB)
// Input: oper1, p1undef_ptr
//
// typedef struct tagOPERXSTR { // Operand structure
//   char operX[OPERLEN+1];     // operand expression string w/ symbols/labels 
//   char symbolX[SYMLEN+1];    // symbolname assigned to value of operX 
//   char errsymX[SYMLEN+1];    // name of an undefined symbol in operX
// } OPERXBUF, *LPOPERXBUF;     // Structured oprand string buffer
//
void p1ds()
  {
  int _i;   // MUST be local (..global _i is used in expr()!)

  GetPcValue();

  if (AtmelFlag != 0 && SegType == _CODE) return;   // .BYTE not allowed in CSEG

  LPOPERXBUF tmpp1undef_ptr = p1undef;

  for (_i=0; _i<OPERXENTRIES; _i++)
    {
    // Check if oper1 is a currently undefined symbol (may be forward reference)
    if (StrCmpNI(oper1, tmpp1undef_ptr->symbolX, strlen(tmpp1undef_ptr->symbolX)) == 0 &&
        strlen(tmpp1undef_ptr->symbolX) > 0)
      {
      pcValue += expr(tmpp1undef_ptr->operX); // Evaluate original operand expression string
      break;
      }
    tmpp1undef_ptr++;
    } // end for

  // Normal processing: Evaluate operand symbol/expr if not forward referenced
  if (_i >=OPERXENTRIES) pcValue += expr(oper1);

//ha//  // Give error code + error line to Pass1 error table
//ha//  //  if symbol is unknown or forward referenced
//ha//  if (_exprInfo == ERR_UNDFSYM) p1errorl(ERR_UNDFSYM, symboltab_ptr->symString);

  AssignPc();
  } // p1ds

//-----------------------------------------------------------------------------
//
//                               p1equ
// .EQU <expression>
// .DEFINE <symbol>
// #define <symbol>
//
// Input: oper1, symboltab_ptr, p1undef_ptr
//
//  typedef struct tagSYMBOLBUF {   // Symbol structure
//    char  symString[SYMLEN+1];    // Symbol/Label name  string
//    ULONG symAddress;             // Symbol value or pointer to operand string
//    UCHAR symType;                // Symbol type (ABS, C-SEG, UNDEFINED)
//  } SYMBOLBUF, *LPSYMBOLBUF;      // Structured symbol buffer
// 
void p1equ()
  {
  char* tmpPtr;

  ULONG _value;
  UINT _rsltLabfn;

  ////////////////////////////////////////////////////
  // re-define an #undef'd symbol                   //
  if (p12Cundef(_PASS1|_PASS2) == TRUE) return;     //
  ////////////////////////////////////////////////////

  // Atmel AVRASM2 compatible: Allow something like: ".EQU     REV01 = 2+$AA"
  if (AtmelFlag != 0) SyntaxAVRtoMASM(_PASS1);

  //#define  .DEFINE (also for Motorola) //ha//
  // Atmel AVRASM2 compatible: Allow something like: "#define  REV01"
  //                                                 "#define  REV01 123"
  // >                                                fopcd    flabl  (inbuf)
  // Note: some assemblers allow .DEFINE (not specified by Atmel)
  if ((AtmelFlag != 0 || MotorolaFlag != 0) && oper2[0] == 0 &&  //ha// 
      (StrCmpI(fopcd, _defin_str) == 0 || StrCmpI(fopcd, DEFIN_str) == 0))
    {
    SyntaxAVRtoMASM(_PASS1); //ha// for #define <symbol>
    char* tmpPtr;
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

    if (oper1[0] == 0) { oper1[0] ='1'; oper1[1] = 0; } // set value to 1
    } // end if (AtmelFlag)

  // STRLEN(string)
  if ((tmpPtr = StrStrI(oper1, "STRLEN(")) != NULL)
    {
    if (tmpPtr[7] == ')')                // STRLEN() 
      {
      edwh(oper1, strlenDB);             // From previous .DB
      oper1[4] = 'h';
      oper1[5] = 0;
      }
    else if (tmpPtr[7] == '\x22'                && 
             tmpPtr[strlen(tmpPtr)-2] == '\x22' &&
             tmpPtr[strlen(tmpPtr)-1] == ')')
      {
      edwh(oper1, strlen(&tmpPtr[8])-2); // From previous .DB
      oper1[4] = 'h';
      oper1[5] = 0;
      }
    } // end if ("STRLEN(")

  _value = expr(oper1); 

  SymType = _ABSEQU;  

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("p1Flagifndef=%02X", p1Flagifndef);  
//ha//DebugStop(1, "p1equ()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
  
  ///////////////////////////////////////////////////////////////////////
  // (all symbols within .IFNDEF/#ifndef condition have this flag set) //
  if (p1Flagifndef == CMOD_IFNDEF && StrCmpI(p1Symifndef, flabl) == 0) //
    SymType |= SYMIFNDEF_FLAG;                                         //
  ///////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////// //ha//
  // (all symbols within .IFNDEF/#ifndef condition have this flag set) // //ha//
  if (p1Flagifndef == CMOD_IFDEF && StrCmpI(p1Symifndef, flabl) == 0)  // //ha//
    SymType |= SYMIFDEF_FLAG;                                          // //ha//
  /////////////////////////////////////////////////////////////////////// //ha//

  _rsltLabfn = labfn(flabl);  // Get symboltab_ptr

  // if not found insert symbol into symboltab
  if (_rsltLabfn == FALSE)
    {
    for (_i=0; _i<SYMLEN; _i++) symboltab_ptr->symString[_i] = flabl[_i];
    symboltab_ptr->symAddress = (UINT)_value;   // Enter current value
    symboltab_ptr->symType = SymType;           // Type byte

    // Check if undefined in PASS1 (forward reference)
    // The symbol can be evaluated, if it is found and re-evaluated later in PASS2.
    // In this case an error code is not necessary.
    // NOTE: "_exprInfo" indicates that the symbol/label was not (yet) found in PASS1.
    if (_exprInfo == 0 && error == 0) symboltab_ptr->symType = SymType;  // Type byte
    else 
      {
      symboltab_ptr->symType = (UCHAR)(SYMERR_FLAG | ERR_UNDFSYM); // Mark as Pass1 undefined

      // Solution works with "EQU" directive, however, still causes errors
      // with program counter at "DS, RMB, .BYTE" directives in certain cases. 
      //
      //  typedef struct tagOPERXSTR {    // Operand structure
      //    char operX[OPERLEN+1];        // operand expression string w/ symbols/labels
      //    char symbolX[SYMLEN+1];       // symbolname assigned to value of operX 
      //    char errsymX[SYMLEN+1];       // name of an undefined symbol in operX
      //  } OPERXSTR, *LPOPERXSTR;        // Structured oprand string buffer
      //
      if (p1undef_ptr < &p1undef[OPERXENTRIES-1])              
        {
        // Save erroneous original operand expression string
        for (_i=0; _i<OPERLEN; _i++) p1undef_ptr->operX[_i] = oper1[_i];  
        for (_i=0; _i<SYMLEN; _i++)
          {
          // Save name string of erroneous symbol assigned to the operand expression string
          p1undef_ptr->symbolX[_i] = flabl[_i]; 
          // Save the name string of the unknown symbol within the operand expression string
          p1undef_ptr->errsymX[_i] = flabl[_i]; 
          }

        p1undef_ptr++;  // Advance to next error correction structure
        } // end if
      }
    return;
    }                          

  // Check if already defined or already doubly defined             
  if (_rsltLabfn != FALSE && symboltab_ptr->symType == (UCHAR)(SYMERR_FLAG | ERR_DBLSYM))
    return;

  else if (_rsltLabfn != FALSE && symboltab_ptr->symType != 0)
     symboltab_ptr->symType = (UCHAR)(SYMERR_FLAG | ERR_DBLSYM);  // Mark as doubly defined
  
  } // p1equ

//-----------------------------------------------------------------------------
//
//                               p1set
// .SET Directive <expression>
// .DEF Directive <register>
//
// Input: oper1, symboltab_ptr, p1undef_ptr
//
//  typedef struct tagSYMBOLBUF {   // Symbol structure
//    char  symString[SYMLEN+1];    // Symbol/Label name  string
//    ULONG symAddress;             // Symbol value or pointer to operand string
//    UCHAR symType;                // Symbol type (ABS, C-SEG, UNDEFINED)
//  } SYMBOLBUF, *LPSYMBOLBUF;      // Structured symbol buffer
// 
void p1set()
  {
  ULONG _value;
  UINT _rsltLabfn;

  // Atmel AVRASM2 compatible: Allow something like : ".SET     REV01 = 2+$AA"
  // >                                                fopcd    flabl  (inbuf)
  if (AtmelFlag != 0) SyntaxAVRtoMASM(_PASS1);

  // .DEF - check valid regidster (Example: .DEF X = r16)
  if (AtmelFlag != 0 && oper2[0] == 0 && StrCmpI(fopcd, DEF_str) == 0)
    {
    if (toupper(oper1[0]) != 'R' ||
        (toupper(oper1[0]) == 'R' && (_value=expr(&oper1[1])) > 31))
      p1errorl(ERR_BADREG, oper1);
    } // end if (AtmelFlag)

  // .SET (.DEF = .SET)
  // Must be before labfn(flabl) (destroys symboltab_ptr)
  _value = expr(oper1);
  if (error != 0) return;   // Abort on error

  labfn(flabl);  // Get symboltab_ptr

  if (symboltab_ptr->symType == _ABSEQU) p1errorl(ERR_DBLSYM, flabl);
  else
    {
    // Just to insert symbol into symboltab (value may be re-assigned in pass2)
    for (_i=0; _i<SYMLEN; _i++) symboltab_ptr->symString[_i] = flabl[_i];
    symboltab_ptr->symAddress = (UINT)_value;   // Enter current value
    symboltab_ptr->symType = _ABSSET;           // Type byte
    }
   
  ///////////////////////////////////////////////////////////////////////
  // (all symbols within .IFNDEF/#ifndef condition have this flag set) //
  if (p1Flagifndef == CMOD_IFNDEF && StrCmpI(p1Symifndef, flabl) == 0) //
    symboltab_ptr->symType |= SYMIFNDEF_FLAG;                          //
  ///////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////// //ha//
  // (all symbols within .IFNDEF/#ifndef condition have this flag set) // //ha//
  if (p1Flagifndef == CMOD_IFDEF && StrCmpI(p1Symifndef, flabl) == 0)  // //ha//
    SymType |= SYMIFDEF_FLAG;                                          // //ha//
  /////////////////////////////////////////////////////////////////////// //ha//
  } //p1set


//-----------------------------------------------------------------------------
//
//                               titleXfer
//
void titleXfer(char* _inbuf, char* _titleStr, char* _titleBuf)
  {
  // Skip $TITLE directive
  _inbuf = &_inbuf[strlen(_titleStr)];
  // Check if 1st char after $TITLE is either ' ' or TAB or CR
  if (_inbuf[0] != SPACE && _inbuf[0] != TAB && _inbuf[0] != CR)
    {
    p1errorl(ERR_SYNTAX, NULL);
    return;              // Done.
    }
  else _inbuf++;         // skip to 1st char of title string

  for (_i=0; _i<LHTITL; _i++) _titleBuf[_i] = *_inbuf++;
  _titleBuf[LHTITL] = 0; // truncate and terminate
  } // titleXfer

//-----------------------------------------------------------------------------
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("swEol=%02X  swstx=%02X  strlenDB=%d   pcValue=%04X\n_inPtr=%s",
//ha//        swEol,      swstx,      pcValue-pccDB,pcValue,      _inPtr);
//ha//DebugStop(3, "p1db()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("inbuf ");
//ha//DebugPrintBuffer(inbuf, OPERLEN);
//ha//DebugStop(2, "p1LineContinuation()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha////if (_debugSwPass == _PASS2)
//ha//{
//ha//printf("pcc=%08X lvalue=%08X\nRomStart= %04X RomSize= %04X\nSRamStart=%04X SRamSize=%04X EEPromSize=%4X",
//ha//        pcc,     lvalue,      RomStart,      RomSize,      SRamStart,     SRamSize,     EEPromSize);
//ha//DebugStop(1, "pass1()", __FILE__);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("flabl                = %s\n", flabl);
//ha//printf("oper1                = %s\n", oper1);
//ha//printf("tmpp1undef_ptr=%08X\n", tmpp1undef_ptr);
//ha//printf("tmpp1undef_ptr->symbolX = %s =[%08X]\n", tmpp1undef_ptr->symbolX, symboltab_ptr->symAddress);
//ha//printf("tmpp1undef_ptr->operX   = %s\n", tmpp1undef_ptr->operX);
//ha//printf("tmpp1undef_ptr->errsymX = %s\n", tmpp1undef_ptr->errsymX);
//ha//printf("pcValue=%08X  error=%d  _exprInfo=%d  _i=%d\n", pcValue, error, _exprInfo, _i);
//ha//DebugStop(2, "p1org()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("flabl                = %s\n", flabl);
//ha//printf("oper1                = %s\n", oper1);
//ha//printf("p1undef_ptr=%08X\n", p1undef_ptr);
//ha//printf("p1undef_ptr->symbolX = %s =[%08X]\n", p1undef_ptr->symbolX, symboltab_ptr->symAddress);
//ha//printf("p1undef_ptr->operX   = %s\n", p1undef_ptr->operX);
//ha//printf("p1undef_ptr->errsymX = %s\n", p1undef_ptr->errsymX);
//ha//printf("pcValue=%08X  error=%d  _exprInfo=%d  _i=%d\n", pcValue, error, _exprInfo, _i);
//ha//DebugStop(1, "pass1()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("_value=%d  flabl=%s  fopcd=%s  oper1=%s  oper2=%s\n",
//ha//        _value,    flabl,    fopcd,    oper1,    oper2);
//ha//printf("oper1 ");
//ha//DebugPrintBuffer(oper1, OPERLEN/3);
//ha//DebugStop(3, "p1equ()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("Flabl=%s  fopcd=%s  oper1=%s  ins_group=%d  insv[1]=%d\n"
//ha//       "swdefmacro=%d",
//ha//        flabl,    fopcd,    oper1,    ins_group,    insv[1],
//ha//        swdefmacro);  
//ha////printf("macrotab[macroCount].macBufPtr ");
//ha////DebugPrintBuffer(macrotab[macroCount].macBufPtr, strlen(macrotab[macroCount].macBufPtr));
//ha//DebugStop(1, "pass1()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("flabl=%s  fopcd=%s  oper1=%s  _rsltLabfn=%d  swif=%d  ifCount=%d\n",
//ha//        flabl,    fopcd,    oper1,    _rsltLabfn,    swif,    ifCount);  
//ha////printf("oper1 ");
//ha////DebugPrintBuffer(oper1, OPERLEN);
//ha//DebugStop(1, "p1ifndef()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("flabl=%s  fopcd=%s  oper1=%s\n  errSymbol=[%s]  errSymbol_ptr=[%s]  insv[1]=%d\n",
//ha//        flabl,    fopcd,    oper1,      errSymbol,      errSymbol_ptr,      insv[1]);  
//ha//printf("oper1 ");
//ha//DebugPrintBuffer(oper1, OPERLEN);
//ha//DebugStop(1, "pass1()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("_dirStr=%s  _savINC=%02X\n  szIclFileName=%s  pszCurFileName=%s\n",
//ha//        _dirStr,    _savINC,        szIclFileName, pszCurFileName);  
//ha////printf("oper1 ");
//ha////DebugPrintBuffer(oper1, OPERLEN);
//ha//DebugStop(1, "pass1()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("_dirStr=%s  inbuf=%s flabl=%s  fopcd=%s  oper1=%s  oper2=%s\n",
//ha//        _dirStr,    inbuf,   flabl,    fopcd,    oper1,    oper2);
//ha////printf("oper1 ");
//ha////DebugPrintBuffer(oper1, OPERLEN);
//ha//DebugStop(1, "pass1()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("tmpp1undef_ptr=%08X\n", tmpp1undef_ptr);
//ha//printf("oper1               = [%04X] %s\n", expr(oper1), oper1);
//ha//printf("flabl               = %s\n"
//ha//       "tmpp1undef_ptr->symbolX= [%04X] %s\n",
//ha//        flabl, expr(tmpp1undef_ptr->symbolX), tmpp1undef_ptr->symbolX);
//ha//printf("oper2               = %s\n"
//ha//       "tmpp1undef_ptr->operX  = [%04X] %s\n",
//ha//        oper2, expr(tmpp1undef_ptr->operX), tmpp1undef_ptr->operX);
//ha//printf("tmpp1undef_ptr->errsymX= [%04X] %s\n", expr(tmpp1undef_ptr->errsymX), tmpp1undef_ptr->errsymX);
//ha//printf("strlen(tmpp1undef_ptr->errsymX)=%d\n", strlen(tmpp1undef_ptr->errsymX));
//ha//printf("pcc=%04X  error=%d  _exprInfo=%d\n",
//ha//        pcc,      error,    _exprInfo);
//ha//DebugStop(1, "pass1()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("oper1                = [%04X] %s\n", expr(oper1), oper1);
//ha//printf("flabl                = %s\n"
//ha//       "p1undef_ptr->symbolX = [%04X] %s\n",
//ha//        flabl, expr(p1undef_ptr->symbolX), p1undef_ptr->symbolX);
//ha//printf("oper2                = %s\n"
//ha//       "p1undef_ptr->operX   = [%04X] %s\n",
//ha//        oper2, expr(p1undef_ptr->operX), p1undef_ptr->operX);
//ha//printf("p1undef_ptr->errsymX = [%04X] %s\n", expr(p1undef_ptr->errsymX), p1undef_ptr->errsymX);
//ha//printf("strlen(p1undef_ptr->errsymX)=%d\n", strlen(p1undef_ptr->errsymX));
//ha//printf("pcc=%04X  _pc1=%04X  _pc2=%04X  error=%d\n", pcc, _pc1, _pc2,  error);
//ha//DebugStop(1, "pass1()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha////printf("flabl                = %s\n", flabl);
//ha//printf("p1undef_ptr=%08X\n", p1undef_ptr);
//ha//printf("p1undef_ptr->symbolX = %s =[%08X]\n", p1undef_ptr->symbolX, symboltab_ptr->symAddress);
//ha////printf("oper2                = %s\n", oper2);
//ha//printf("p1undef_ptr->operX   = %s\n", p1undef_ptr->operX);
//ha//printf("p1undef_ptr->errsymX = %s\n", p1undef_ptr->errsymX);
//ha//printf("_value=%04X  error=%d  _exprInfo=%d\n", _value, error, _exprInfo);
//ha//DebugStop(1, "pass1()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//DebugPrintSymbolArray(3);  
//ha//DebugStop(1, "pass1()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("pcValue=%d  flabl=%s  fopcd=%s  oper1=%s  oper2=%s  oper3=%s\n",
//ha//        pcValue   , flabl,    fopcd,    oper1,    oper2,    oper3);
//ha//DebugStop(1, "p1db()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//--------------------------end-of-c++-module-----------------------------------

