// haXASM - Cross-Assembler for 8bit Microprocessors
// workst.cpp - C++ Developer source file.
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

#include <shlwapi.h>   // Library shlwapi.lib for PathFileExistsA

#include <windows.h>   // For console specific functions

#include "devAVR.h"
#include "equate.h"

using namespace std;

#pragma pack(1)       // Alignment of structure elements

//------------------------------------------------------------------------------
//
//                 CROSS ASSEMBLER GLOBAL BUFFERS
//
// Nostalgic DOS / ISIS-II System working area (those were the days..)  
//
//ha//UINT scaft  = 0; // source aft nr
//ha//UINT aftsve = 0; // save scaft nr
//ha//UINT lsaft  = 0; // list aft nr
//ha//UINT hxaft  = 0; // hex  aft nr
//ha//UINT act    = 0;
//ha//UINT xact   = 0;
//ha//
//ha//UCHAR buf[PATHLEN];       // command line buffer
//ha//UCHAR hex_name[PATHLEN];  // .HEX file name buffer
//ha//
//ha//// The byte bucket file :BB: (ISIS-II) handles the source file linewise.
//ha//// The command line from :CI: is transferred into the command buffer
//ha//// The RAM-area for the symbol table is assigned according to width
//ha////  of the RAM-area available (via MEMCHK on ISIS-II).
 
char* pszSrcFilebuf  = NULL;
char* pszSrcFilebuf0 = NULL;

//------------------------------------------------------------------------------
//
// typedef struct tagDEVICEAVR {   // Device AVR structure
//   char*     deviceName;
//   ULONG     FLASHStart;       
//   ULONG     FLASHSize;       
//   ULONG     SRAMStart;       
//   ULONG     SRAMSize;       
//   ULONG     EEPROMStart;       
//   ULONG     EEPROMSize;
//   UINT      missingInst;  
// } DEVICEAVR, *LPDEVICEAVR;
//
DEVICEAVR deviceUnknown[] = {
  {"Unknown", 0, 4*1024*1024, 0, 4*1024*1024, 0, 64*1024, _ALL_ },
  {NULL,      0,           0, 0,           0, 0,       0, _NONE_}
  }; // end of deviceUnknowe structure

LPDEVICEAVR deviceUnknownPtr = deviceUnknown; // Pointer to device default structure
LPDEVICEAVR devicePtr = deviceUnknownPtr;     // Global pointer to structure of .DEVICE

// typedef struct tagINCFILES {    // Include file cascade structure
//   char      iclPath[PATHLEN];   // Include file name string
//   int       iclSeekPos;         // Include file stream seek position
//   int       iclLine;            // Counter of current include file line read
// } INCFILES, *LPINCFILES;
//
INCFILES icfStack[ICFCOUNTMAX];     // Maximum of cascaded .INC flies 
LPINCFILES icfStack_ptr = icfStack; // Global pointer to array of .INC files
                                 
// typedef struct tagP1ERRTBL {  // Error line structure
//   int lineNr;                 // Source line number
//   UCHAR errCode;              // Error code
//   char  errsymbol[SYMLEN+1];  // Error symbol string
// } P1ERRTBL, *LPP1ERRTBL;      // Structured Pass1 error buffer
//
P1ERRTBL p1errTbl[P1ERRMAX];         // Maximum of errors in Pass1
LPP1ERRTBL p1errTbl_ptr = p1errTbl;  // Global pointer to array of Pass1 errors 

// typedef struct tagOPERXSTR {    // Operand structure
//   char operX[OPERLEN+1];        // operand expression string w/ symbols/labels 
//   char symbolX[SYMLEN+1];       // symbolname assigned to value of operX 
//   char errsymX[SYMLEN+1];       // name of an undefined symbol in operX
// } OPERXBUF, *LPOPERXBUF;        // Structured oprand string buffer
//
OPERXBUF p1undef[OPERXENTRIES+1];  // undefined expressions operands in Pass1
LPOPERXBUF p1undef_ptr = p1undef;  // pointer to undefined expressions in Pass1

// Assembly input buffer of src/icl line currently read
char inbuf[INBUFLEN+4];       
char* inbuf_ptr = inbuf;

// Formatted listing buffer of assembled input line
char szListBuf[LSBUFLEN];     
char* pszListBuf = szListBuf;

// Atmel/Microchip instruction availability info buffer
char missInstrBuf[MISS_INSTR_BUFLEN];
char* missInstrBuf_ptr = missInstrBuf;

// ifelse buffer for 40 labels being operands of #undef directive
char symUndefBuf[SYMUNDEFBUFSIZE+1];
char* symUndefBuf_ptr = symUndefBuf;

//------------------------------------------------------------------------------
//
//                 CROSS ASSEMBLER GLOBAL BUFFER ALOCATION
//
// typedef struct tag_ORGSEG {    // Segment usage and memory layout structure
//   ULONG     sStart;
//   ULONG     sEnd;         
// } ORGSEG, *LPORGSEG;
//
ORGSEG csegLayout[ORGMAX_C];           // Maximum of scattered CSEG blocks 
LPORGSEG csegLayout_ptr = csegLayout;  // Global pointer to array of csegLayout

ORGSEG dsegLayout[ORGMAX_D];           // Maximum of scattered DSEG blocks 
LPORGSEG dsegLayout_ptr = dsegLayout;  // Global pointer to array of csegLayout

ORGSEG esegLayout[ORGMAX_E];           // Maximum of scattered ESEG blocks 
LPORGSEG esegLayout_ptr = esegLayout;  // Global pointer to array of csegLayout

// Conditional cascade structure
//                              
// typedef struct tagCIFDEF {      
//   char      Cifdef[2];          // #ifdef/#ifndef/#if/#elif/#else/#endif
//   char      Cifndef[2];         // .IFDEF/.IFNDEF/.IF/.ELIF/.ELSE/.ENDIF       
//   char      Cif[2];    
//   union {          
//     char    CelifCnt;           // Nr of #elif associated w/ #ifdef/#ifndef/#if
//     char    Celif[IFELSEMAX];   // current #elif[CelifCnt] condition
//   };          
//   char      Celse[2];        
// } CIFDEF, *LPCIFDEF;
// 
//
CIFDEF preprocessStack[IFELSEMAX];               // Maximum of cascaded #ifelse 
LPCIFDEF preprocessStack_ptr = preprocessStack;  // Global pointer to array of #ifelse

int CifdefCnt    = 0;    // .IFDEF/#ifdef
int CifndefCnt   = 0;    // .IFNDEF/#ifndef
int CifCnt       = 0;    // .IF/#if
int CelseCnt     = 0;    // .ELSE/#else
int CendifCnt    = 0;    // .ENDIF/#endif
int p12CSkipFlag = 0xFF; // Conditional control flag

int p1Flagifndef = 0;       // #ifndef/.IFNDEF statement before symbol definition
char p1Symifndef[SYMLEN+1]; // ifndef label field

// Symbol table array buffer area
//
//  typedef struct tagSYMBOLBUF {  // Symbol structure
//    char  symString[SYMLEN + 1]; // Symbol/Label name string
//    ULONG symAddress;            // Symbol value or pointer to operand string
//    UCHAR symType;               // Symbol type (ABS, C-SEG, UNDEFINED)
//  } SYMBOLBUF, *LPSYMBOLBUF;
//
LPSYMBOLBUF symboltab     = NULL;
LPSYMBOLBUF symboltab_ptr = NULL;

// Macro table array buffer area
//
//  typedef struct tagMACROBUF {    // Macro structure
//    char  macNameStr[SYMLEN+1];   // Macro name string
//    char  macLabelBuf[10*SYMLEN]; // Buffer for local labels in macro
//    char* macBufPtr;              // Pointer to macro body (instruction lines)
//    int   macBufLen;              // Length of macro buffer contents
//    int   macDefCount;            // Assigned when a macro is being defined
//    int   macExpCountP1;          // Incremented when a macro is being expanded in _PASS1
//    int   macExpCountP2;          // Incremented when a macro is being expanded in _PASS2
//    UCHAR macType;                // Macro type reserved = 0
//  } MACROBUF, *LPMACROBUF;        // Structured macro buffer
//
LPMACROBUF macrotab    = NULL;

int p1defmacroFlag = FALSE;  // pass1 macro flag
int p2defmacroFlag = FALSE;  // pass2 macro flag

int swlistmac  = FALSE; // Macro expansion shown in .LST file
int swdefmacro = FALSE; // Macro definition control src line
int swexpmacro = FALSE; // Macro expansion src line read control
int macroCount = 0;     // Number of .MACRO statements before .ENDM/.ENDMACRO (=1)
int macroBegin = 0;     // Flag for .MACRO statement

// Macro buffer pointers
char* pszDefMacroBuf     = NULL;          
char* pszDefMacroBuf0    = NULL;          
char* pszExpMacroBuf     = NULL;
char* pszExpMacroBuf0    = NULL;
char* pszExpLabMacroBuf  = NULL;
char* pszExpLabMacroBuf0 = NULL;

// Macro cascade line buffer pointer array
//
//  #define MACCASCADEMAX 10        // Maximum number of cascaded macros
//  typedef struct tagMACCASCADE {  // Macro cascading structure (Macros within macros)
//    char* macLineBufPtr;          // Pointer to current macro line string
//    int   macLineBufLen;          // length of current macro line string
//  } MACCASCADE, *LPMACCASCADE
//
MACCASCADE macrocascade[MACCASCADEMAX];       // Maximum of cascaded macros 
LPMACCASCADE macrocascade_ptr = macrocascade; // Global pointer to array of cascaded macros

// Note - pszFileInbuf:
// Necessary because the function '.getline(..)' always reads until CRLF.
// This behaviour can cause a buffer overflow, freezing the .SRC input.
// Allocate enough to prevent buffer overflow for a (very long 32K!) text line.
// (see 'read_source_line()' in dcomp.cpp).
// 
char* pszFileInbuf     = NULL;
char* pszFileInbuf0    = NULL;

// Data buffer for bin lines
char* pszBinFilebuf    = NULL;

// Data buffer for hex lines
char* pszBinbuf        = NULL;  
char* pszBinbuf0       = NULL;  
char* pszBinEEbuf      = NULL;
char* pszBinEEbuf0     = NULL;

// Ascii hex file format buffer
char* pszHexFilebuf    = NULL;  
char* pszHexFilebuf0   = NULL;  
char* pszHexFileEEbuf  = NULL;
char* pszHexFileEEbuf0 = NULL;         

// Multi purpose buffer
char* pszScratchbuf    = NULL;
char* pszScratchbuf0   = NULL;
char* pszErrtextbuf    = NULL;
char* pszWarntextbuf   = NULL;

//------------------------------------------------------------------------------
//
//                            AllocateBuffers
//
// IMPORTANT NOTE: 
//  if a pointer to the uppermost symbol structure is defined in order 
//  to check when symboltab array is full, like:
//
//      symboltab_top = &symboltab[SYMENTRIES-1]; // NOK
//      if (symboltab_ptr > symboltab_top) {..};  // NOK
//
//  !!! DON'T USE 'symboltab_top' VALUE, as it is NOT RELIABLE (Windows?) !!!
//  !!! IT SPORADICLLY GETS TRUNCATED TO A 16BIT VALUE, DEPENDING ON      !!!
//  !!! THE ADDRESS ASSIGNED BY GlobalAlloc()                             !!!
//  !!! THEREFORE ALWAYS USE '&symboltab[SYMENTRIES-1]' DIRECTLY.         !!!
//
//       if (symboltab_ptr > &symboltab[SYMENTRIES-1] {..}; // OK
//                    
BOOL AllocateBuffers()
  {
  // Symbol table array buffer area
  symboltab = (LPSYMBOLBUF)GlobalAlloc(GPTR, (SYMENTRIES+1) * sizeof(SYMBOLBUF));

  // Macro table array buffer area
  macrotab = (LPMACROBUF)GlobalAlloc(GPTR, (MACENTRIES+1) * MACBUFFERS);
  // Macro buffers
  pszDefMacroBuf     = (char*)GlobalAlloc(GPTR, MACBUFSIZE);          
  pszExpMacroBuf     = (char*)GlobalAlloc(GPTR, MACBUFSIZE);
  pszExpLabMacroBuf  = (char*)GlobalAlloc(GPTR, MACXLABBUFSIZE);
  pszDefMacroBuf0    = pszDefMacroBuf;                    // Buffer start address
  pszExpMacroBuf0    = pszExpMacroBuf;
  pszExpLabMacroBuf0 = pszExpLabMacroBuf;

  // Src/Inc file '.getline(..)' stub buffer
  pszFileInbuf  = (LPSTR)GlobalAlloc(GPTR, LONGLINE); 
  pszFileInbuf0 = pszFileInbuf;

  // Bin data buffer for hex lines
  pszBinbuf    = (char*)GlobalAlloc(GPTR, HXBLEN+16);     // Flash Prom bin buffer + overun area
  pszBinEEbuf  = (char*)GlobalAlloc(GPTR, EEHXBLEN+16);   // EEPROM bin buffer + overun area
  pszBinbuf0   = pszBinbuf;                               // Bin data buffer start address
  pszBinEEbuf0 = pszBinEEbuf;                             
  
  // Ascii hex file format buffer
  pszHexFilebuf    = (char*)GlobalAlloc(GPTR, HXFBLEN);   // Flash Prom hex file buffer
  pszHexFileEEbuf  = (char*)GlobalAlloc(GPTR, EEHXFBLEN); // EEPROM hex file buffer   
  pszHexFilebuf0   = pszHexFilebuf;                       // Hex file buffer start address
  pszHexFileEEbuf0 = pszHexFileEEbuf;

  // Multi purpose buffer
  pszScratchbuf  = (char*)GlobalAlloc(GPTR, 1024+1);      // Scratch buffer (for temporary use)
  pszScratchbuf0 = pszScratchbuf;                         // Buffer start address

  // Multi purpose buffer
  pszErrtextbuf  = (char*)GlobalAlloc(GPTR, OPERLEN+1);   // error info text buffer (for temporary use)
  pszWarntextbuf = (char*)GlobalAlloc(GPTR, OPERLEN+1);   // warning info text buffer (for temporary use)

  //-----------------------------------------------------
  // Check if the necessary allocated buffer is available
  if (macrotab == NULL          ||
      symboltab == NULL         ||
      pszBinbuf == NULL         ||
      pszBinEEbuf == NULL       || 
      pszFileInbuf == NULL      ||
      pszHexFilebuf == NULL     ||
      pszScratchbuf == NULL     ||
      pszErrtextbuf == NULL     ||
      pszWarntextbuf == NULL    ||
      pszDefMacroBuf == NULL    || 
      pszExpMacroBuf == NULL    ||
      pszExpLabMacroBuf == NULL ||
      pszHexFileEEbuf == NULL)
    {
    return(FALSE);   // Buffer alloc failure
    }
  else return(TRUE);
  } // AllocateBuffers

//------------------------------------------------------------------------------
//
//                           FreeBuffers
//
void FreeBuffers()
  {
  GlobalFree(macrotab);
  GlobalFree(symboltab);
  GlobalFree(pszBinbuf0);
  GlobalFree(pszBinEEbuf0);
  GlobalFree(pszFileInbuf0);
  GlobalFree(pszSrcFilebuf0);                 
  GlobalFree(pszHexFilebuf0);
  GlobalFree(pszScratchbuf0);
  GlobalFree(pszErrtextbuf);
  GlobalFree(pszWarntextbuf);
  GlobalFree(pszHexFileEEbuf0);
  GlobalFree(pszDefMacroBuf0);   
  GlobalFree(pszExpMacroBuf0);   
  GlobalFree(pszExpLabMacroBuf0);
  } // FreeBuffers

//------------------------------------------------------------------------------
//
//                 CROSS ASSEMBLER GLOBAL VARIABLE AREA
//
int _i, _j, _k;  // Multi purpose global index variables

ULONG RomStart  =       0x000;  // Program Flash ROM: uC specific (dflt=0)
ULONG RomSize   = 4*1024*1024;  // Program Flash ROM: Board design specific (dflt=4M)
UINT  RomPage   =         512;  // Program Flash ROM: Flash part specific (dflt=512)
UINT  SRamStart =       0x000;  // Internal SRAM: Board design specific (dflt=0)
ULONG SRamSize  = 4*1024*1024;  // Internal SRAM: Board design specific (dflt=4M)
UINT EEPromStart=      0x0000;  // EEPROM: Board design specific (dflt =0)
UINT EEPromPage =          32;  // EEPROM: Flash part specific (dflt=32)
UINT EEPromSize =     64*1024;  // EEPROM: Board design specific (dflt =64K)

ULONG BinFilesize = 0;          // Minimal size required for .BIN-file 

char lastCharAL = 0; // Last character (AL) after buffer processing
char lastCharAH = 0; // Last character (AH) after buffer processing
char lastCharAX = 0; // Last character (AX) after buffer processing

// symboltab_ptr->symType (..not fully implemented.)
//  0x00            ; Reserved 
//  0x01            ; _CODE   Code segment      (CSEG)
//  0x02            ; _DATA   Data segment      (DSEG)
//  0x03            ; _EEPROM EEPROM segment    (ESEG)
//  0x04            ; _ABS    Absolute constant (EQU)
//  0x05            ; Extern, not implemented
//  0x06            ; Public, not implemented
//  0x07..0x0F      ; reserved
// ------
// Flags:
//  0x10            ; No expression//
//  0x20            ; Illegal expression
//  0x40            ; Symbol is defined during Pass1 with .IFNDEF and
//                  ;  the definition src line should be shown in listing.
//  0x80 | 0..0x1F  ; SYMERR_FLAG with symbol error codes 0..31
//
UCHAR SymType =0;       // ABS/SegType at at symboltab_ptr->symType

ULONG pcc     =0;       // CSEG Program counter at symboltab_ptr->symAddress
ULONG pccw    =0;       // Atmel CSEG 16bit (Word) Program counter
ULONG strlenDB=0;       // Atmel CSEG Program counter difference after .DB statement (in Bytes)
ULONG pcd     =0;       // DSEG Program counter at symboltab_ptr->symAddress
ULONG pce     =0;       // ESEG Program counter at symboltab_ptr->symAddress
ULONG pcValue =0;       // Calculated unassigned Program counter (D- or C-Segment)

UINT  value      =0;    // Interception from expr() routine output (16bit)          
ULONG lvalue     =0;    // Global output available from Expr() routine (32bit)
unsigned long long qvalue =0;
char *qstr, *dstr;

int SegType    = _CODE; // Indicates current segment type

char _PCsymbol = _BUCK; // Default: Intel syntax = '$'
                                            
char _exprInfo   = 0;   // Undefined symbol indicator
char binPadByte  = '1'; // Default: No .BIN-file generated (see cmdline option /Bp0x..)

int AtmelFlag    =FALSE; // Init-clear: No Atmel-Syntax
int IntelFlag    =FALSE; // Init-clear: No Intel-Syntax
int MotorolaFlag =FALSE; // Init-clear: No Motorola-Syntax

int srcEOF    = FALSE;   // .SRC EOF encountered (Atmel)
int iclEOF    = FALSE;   // .INC EOF encountered (Atmel)

int errline1  =0;   // Linenr where the 1st error occurred

int pws       =  0; // Symbols per line _symlen+1+4+1+1+2
                   
int pagel     = 84; // Default Pagelength 55
int linel     =180; // Default Pagewidth 180
int linct     =  0; // Listing line counter
int pge       =  0; // Listing page bcd-number (0..9999)

int swpass    =_PASS2; // Indicates the current assembly stage (default=_PASS2)

int swoverlap =0; // overlap CSEG org address areas                         
int swstx     =0; // string expr                          
int nosym     =0; // Allow symbol table in listing
int swsym     =0; // Print formatted headline in listing
int swlst     =0; // Only print $.<directive> in listing
int swlab     =0; // Symbol table full flag
int swcom     =0; // comment switch
int swicl     =0; // include file
int swicleof  =0; // end-of-include file

int swif      = FALSE; // Conditionally control src line:  .IF 
int swifdef   = FALSE; // Conditionally control src line: .IFDEF/.IFNDEF
int swelseif  = FALSE; // Conditionally control src line: .ELSEIF/(.ELSEIFDEF)

int swCif     = FALSE; // Conditionally pre-proc control src line:  #if 
int swCifdef  = FALSE; // Conditionally pre-proc src line:   #ifdef/#ifndef
int swCelseif = FALSE; // Conditionally pre-proc src line:   #elseif/(#elseifdef)

int swdevice  = 0;     // Atmel AVR specific (.DEVICE. missing or unknown device)
int swpragma  = 0;     // Atmel AVR specific

int swmodel   = _WORD; // Default: Atmel AVR only (word addresses in listing)

int cmdmodel  = -1;    // Invalidate command line option
int cmdnosym  = -1;    // Invalidate command line option

int lastErr   =0; // result from system function: GetLastError()
int errStatus =0; // error status
int swp1err   =0; // Turn pass1 error reporting ON=0xFF / OFF=0 (see p1db())
int p1erc =0;     // Pass1 error count
int ercnt =0;     // error messages count
int erno  =0;     // error code nr
int warno =0;     // warning code nr

int _errCSEG=0, _errDSEG=0, _errESEG=0; // Segment range error counter

int _lstline, _curline;      // .LST .ASM .INC line counters

char* errSymbol_ptr  = NULL; // pointer to string of descriptive error text
char* warnSymbol_ptr = NULL; // pointer to string of descriptive warning text

//------------------------------------------------------------------------------
//    .SRC: file input & split buffer area
//

//SPLITFIELD srcLine;                  // Global srcLine structure
//LPSPLITFIELD srcLine_ptr = &srcLine; // Global pointer to srcLine structure

// Splitted source line fields
char flabl[SYMLEN+1];   // label field
char fopcd[OPCDLEN+1];  // opcode field
char oper1[OPERLEN+1];  // operand1 field
char oper2[OPERLEN+1];  // operand2 field
char oper3[OPERLEN+1];  // operand3 field
char oper4[OPERLEN+1];  // operand4 field
char oper5[OPERLEN+1];  // operand5 field
char oper6[OPERLEN+1];  // operand6 field
char oper7[OPERLEN+1];  // operand7 field
char oper8[OPERLEN+1];  // operand8 field
char oper9[OPERLEN+1];  // operand9 field
char oper10[OPERLEN+1]; // operand10 field
char oper11[OPERLEN+1]; // operand11 field
char oper12[OPERLEN+1]; // operand12 field
char oper13[OPERLEN+1]; // operand13 field
char oper14[OPERLEN+1]; // operand14 field
char oper15[OPERLEN+1]; // operand15 field
char oper16[OPERLEN+1]; // operand16 field

UCHAR ilen      = 0;    // instruction length (bytes)
UCHAR ins_group = 0;    // CPU instruction group
UCHAR insv[6]   = {0};  // CPU instruction value insv[1] insv[2] .. etc
UCHAR nopAlign  = 0;    // NOP instruction value for .ALIGN / .EVEN directives

//------------------------------------------------------------------------------
//      .HEX: File area
//
int _hexFileEdit = FALSE;             // TRUE: Force edit all hexfile Data 'toupper'
UCHAR chksum =0;                      // hex/s19 format bcc char                        

UINT hxpc   = 0;                      // pcc address for .HEX   file
UINT eehxpc = 0;                      // pce address for .EEHEX file
                      
int swhxrec02  = 0;                   // Flag to indicate hxrec02 not yet emmitted
char hxrec02[] = ":020000020000FC\n"; // Intel:    rec02 ext addr string for .HEX file
char* hxeof    = ":00000001FF\n";     // Intel:    rec01 EOF string for      .HEX file
char* srstart  = "S9030000FC\n";      // Motorola: exec startaddr string for .S19 file

//------------------------------------------------------------------------------
//    .LST: Header & title buffer area
//
// start position for date =[59]
// start position for PageNumber =[59+16]
//
// "09/11/2001 PAGE    1"
//
char lhbuf[LHBUFLEN];        // lhbuf[0] = FF FormFeed and page Header
char lhtit[LHTITL+2];        // Title buffer+newline
char lhsubttl[LHTITL+2];     // Subtitle buffer+newline

char lh_date[10+1];          // = "dd/mm/yyyy"   
char lh_time[8+1];           // = "hh:mm:ss"   
char* datpos_ptr;            // position for date
char* pagpos_ptr;            // = "00/00/0000.PAGE.0000" position for page nr

//------------------------------------------------------------------------------
//    .LST: Headline formatting (pcc = 16bit)
//
// See also equate.inc
//                       |       |       |       |       |  <-- tab positions
//               01234567890123456789012345678901234567890  <-- column numbers
//                LOC   OBJ               LINE   SOURCE     <-- headline text 
//                1234  12 34 56 78 90:E+12345 C program source code.........
//                ^     ^             ^^^^     ^ ^ 
char* lhtxt16 = " LOC   OBJ               LINE   SOURCE\n";
//               01234567890123456789012345678901234567890
//                ^     ^             ^^^^       ^ 
int lpLOC  = LOC16;  // start column of 16bit program counter
int lpOBJ  = OBJ16;  // start column of object code bytes
int lpSRC  = SRC16;  // start column (in TABs 9..17..) of source statement

int lpLINE = LINE16; // start column of line count
int lpMARK = MARK16; // special marker, e.g. 'C' for include file line
int lpMACR = MACR16; // special marker, e.g. '+' for macro expansion
int lpXSEG = XSEG16; // start column of segment type indicator

char* lhtxt_ptr = lhtxt16;  // Default pcc = 16bit

//------------------------------------------------------------------------------
//    .LST: Headline formatting (pcc = 32bit
//
// See also equate.inc
//                       |       |       |       |       |       |  <-- tab positions
//               0123456789012345678901234567890123456789012345578  <-- column numbers
//                LOC       OBJ                   LINE   SOURCE     <-- headline text 
//                00001234  12 34 56 78 90  :E + 12345 C program source code..........
//                ^         ^               ^^ ^ ^     ^ ^
char* lhtxt32 = " LOC       OBJ                   LINE   SOURCE\n";
//               0123456789012345678901234567890123456789012345578
//                ^         ^               ^^ ^  ^    ^ ^
//int lpLOC  = LOC32;  // start column of 32bit program counter
//int lpOBJ  = OBJ32;  // start column of object code bytes
//int lpSRC  = SRC32;  // start column (in TABs 9..17..) of source statement
//
//int lpLINE = LINE32; // start column of line count
//int lpMARK = MARK32; // special marker, e.g. 'C' for include file line
//int lpMACR = MACR32; // special marker, e.g. '+' for macro expansion
//int lpXSEG = XSEG32; // start column of segment type indicator
//
//char* lhtxt_ptr = lhtxt32;  // Default pcc = 32bit


//------------------------------------------------------------------------------
//    Text strings and messages
//
// WARNING text strings: Printed info lines if warnings
char* warnText[_WARNEND_] = {
  NULL,                                                // 00 reserved for no warning
  "Directive not supported",                           // 01  WARN_IGNORED
  "Directive ignored (refer to uC Data Sheet)",        // 02  WARN_DEVICE
  "Source line too long (>1000 characters)",           // 03  WARN_TOOLONG (>INBUFLEN=1000)
  "Check END-OF-FILE directive",                       // 04  WARN_CHKEXIT
  "Out of range, value is masked",                     // 05  WARN_MSKVALUE
  "Check range in expression, masked result is wrong"  // 06  WARN_MSKWRONG
};

// ERROR text strings: Printed info lines if errors
char* errText[_ERREND_] = {
  NULL,                                           // 00 reserved for no error
  "Wrong syntax",                                 // 01 ERR_SYNTAX    
  "Unknown instruction",                          // 02 ERR_ILLOPCD   
  "Line too long or statement too complex",       // 03 ERR_TOOLONG   
  "Symbol table full",                            // 04 ERR_SYMFULL   
  "Illegal or unknown operand",                   // 05 ERR_ILLOP    
  "Illegal operator",                             // 06 ERR_ILLOPER   
  "Illegal byte value expression",                // 07 ERR_ILLBYTE   
  "Pass1 phase error",                            // 08 ERR_P1PHASE   
  "Illegal expression",                           // 09 ERR_ILLEXPR   
  "Missing expression",                           // 10 ERR_MISSEXP   
  "Undefined symbol",                             // 11 ERR_UNDFSYM   
  "Doubly defined symbol",                        // 12 ERR_DBLSYM    
  "Bad register",                                 // 13 ERR_BADREG    
  "Branch out of range",                          // 14 ERR_JMPADDR   
  "Too many cascaded include files",              // 15 ERR_INCDPTH   
  "ORG directive misplaced (segment overlap)",    // 16 ERR_ORGINS    
  "Unknown or undefined symbol in Pass1",         // 17 ERR_P1UNDFSYM 
  "Instruction not expected in Data Segment",     // 18 ERR_DSEG      
  "Address out of range",                         // 19 ERR_SEGADDR   
  "Unknown directive",                            // 20 ERR_ILLDIR   
  "Symbol must be of type 'ABS'",                 // 21 ERR_SYMABS   
  "Macro definition or macro too complex",        // 22 ERR_MACRO      
  "Illegal word value, too large",                // 23 ERR_ILLWORD    
  "Numeric constant too large",                   // 24 ERR_LONGCONST
  "Division by zero",                             // 25 ERR_DIVZERO
  "NULL26",                                       // 26 ERR_
  "NULL27",                                       // 27 ERR_
  "NULL28",                                       // 28 ERR_
  "NULL29",                                       // 29 ERR_
  "NULL30",                                       // 30 ERR_
  "NULL31"                                        // 31 ERR_
};
 
char partNameAVR[20] = "*";   // Atmel/Microchip product core/name string

char* mssge = "";             // No (c)copyright message - it's all Freeware

char* symtx      = "USER SYMBOLS\n";
char* errtx      = "ASSEMBLY COMPLETE,   NO ERRORS\r\n";
char* errtx_err  = "  ERROR ";
char* errtx_sys  = "\n>>>SYSTEM ERROR - ";
char* errtx_file = "UNRECOGNIZABLE SOURCE FILE NAME\n";
char* errtx_alloc= "MEMORY ALLOCATION FAILURE\n";

// 'char* txerr' - the string will be patched at runtime (see finish.cpp).
char txerr[]     = "ASSEMBLY COMPLETE  ***      ERROR(S), (      )\r\n";

//----------------------------------------------------------------------------
//
//                          ActivityMonitor
//
int swActivityMonitor = ON;     // global: the activity monitor is displayed
int _ActivityMonitorCount = 0;  // global: outside world
int _ticker = 0;                // global: this function

void ActivityMonitor(int _mod)
  {
  char _tickChar;

  if (swActivityMonitor == OFF) return;   // Disabled - return

  // _mod = OFF: stop hour-glass
  if (_mod == OFF) { _ActivityMonitorCount = 0; printf("\r \r"); }

  // _mod == ON
  else if (!(swmodel & _NOINFO))  // suppress if '.MODEL NOINFO'
    {
    _ticker++;
    _ticker &= 0x07;
    switch(_ticker % 4)
      {
      case 0:
        _tickChar = '|';
        break;
      case 1:
        _tickChar = '/';
        break;
      case 2:
        _tickChar = '~'; 
        break;
      case 3:
        _tickChar = '\\';
        break;
      } // end switch
    printf("\r%c\r", _tickChar);
    _ActivityMonitorCount = 0;
    }   
  } // ActivityMonitor

//******************************************************************************
//******************************************************************************
//  --DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//
int _debugSwPass=_PASS2; // Default: currently processing Pass2 (=1 for Pass1)

//----------------------------------------------------------------------------
//
//                          DebugStop
//
//  Usage:  DebugStop(testNr, "Function()", __FILE__);
//
//  DEBUG Example:
//
// //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
// printf("pc=%04X, _pc1=%04X\n", pc, _pc1);
// DebugStop(5, "p2org()", __FILE__);
// //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//
void DebugStop(int num, char* _info, char* _pszfile)
  {
  int n;
  printf("\ninbuf=[%s]\n", inbuf); //ha//

  while (_kbhit() != 0) _getch();   // flush key-buffer 

  if (_debugSwPass == 1) printf("PASS1 [%03d--%s]", num, _info);
  else if (_debugSwPass == 2) printf("PASS2 [%03d--%s]", num, _info);
  printf(" %s --press <ESC> for exit--\n", PathFindFileName(_pszfile));
  for (n=0; n<strlen(_info); n++) printf("-");
  for (n=0; n<strlen(_pszfile); n++) printf("-");
  printf("-------------------------\n");
  if (_getch() == ESC) exit(0);
  } // DebugStop

//----------------------------------------------------------------------------
//
//                          DebugPrintBuffer
//
//  Usage:  DebugPrintBuffer(buffer, count);
//
//  DEBUG Example:
//
// //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
// if (_debugSwPass == _PASS2)
// {
// printf("[002a--p1equ()]\n");
// printf("pc=%04X  errSymbol_ptr[0]=%02X\n", pc, errSymbol_ptr[0]);
// printf("errSymbol_ptr ");    
// DebugPrintBuffer(errSymbol_ptr, SYMLEN);
// DebugStop(1, "dcomp()", __FILE__);
// }
// //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//
void DebugPrintBuffer(char *buf, int count)
  {
  int i;
  printf("Buf[0..%d]: ", count);
  // print as ASCII-HEX 00 010: ...
  for (i=0; i<count; i++) printf("%02X ", (unsigned char)buf[i]);
  // print as UNICODE characters: "..."
  printf("\n\x22");                           // "
  for (i=0; i<count; i++)
    {
    // show only the 0-terminated string
//ha//    if (buf[i] == 0) break;
    // print 0s as SPACE
    if (buf[i] == 0) printf("%c", SPACE);                           
    else printf("%c", (unsigned char)buf[i]);
    }
  printf("\x22\n");                           // "
  } // DebugPrintBuffer

//----------------------------------------------------------------------------
//
//                          DebugPrintSymbolArray
//
//  Usage:  DebugPrintSymbolArray(count);
//
//  DEBUG Example:
//
// //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
// DebugPrintSymbolArray(2000);  
// DebugStop(1, "finish()", __FILE__);
// //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//
//  typedef struct tagSYMBOLBUF {  // Symbol structure
//    char  symString[SYMLEN + 1]; // Symbol/Label name string
//    ULONG symAddress;            // Symbol value or pointer to operand string
//    UCHAR symType;               // Symbol type (ABS, C-SEG, UNDEFINED)
//  } SYMBOLBUF, *LP_SYMBOLBUF;
//
void DebugPrintSymbolArray(int count)
  {
  int i, j, k;
  k=0;
  while (_kbhit() != 0) _getch();   // flush key-buffer
  for (j=0; j<count; j++)
    {
    if (symboltab[j].symString[0] != 0)
      {
      k++;                          // count entries
      for (i=0; i<SYMLEN; i++) printf("%02X ", (UCHAR)symboltab[j].symString[i]);
      printf("%08X ", symboltab[j].symAddress);
      printf("%02X ", symboltab[j].symType);
      printf("   [");
      for (i=0; i<SYMLEN; i++) printf("%c", symboltab[j].symString[i]);
      printf("]\n");
      }
    } // end for
  printf("= %d structures in symboltab array =\n", k);
  } // DebugPrintSymbolArray


//----------------------------------------------------------------------------
//
//                          DebugPrintMacroArray
//
//  Usage:  DebugPrintMacroArray(count);
//
//  DEBUG Example:
//
// //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
// DebugPrintMacroArray(2000);  
// DebugStop(1, "p12macro()", __FILE__);
// //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//
//  typedef struct tagMACROBUF {    // Macro structure
//    char  macNameStr[SYMLEN+1];   // Macro name string
//    char  macLabelBuf[10*SYMLEN]  // Buffer for local labels in macro
//    char* macBufPtr;              // Pointer to macro body (instruction lines)
//    int   macBufLen;              // Length of macro buffer contents
//    UCHAR macType;                // Macro type reserved = 0
//  } MACROBUF, *LPMACROBUF;        // Structured macro buffer
//
void DebugPrintMacroArray(int count)
  {
  char* tmpPtr = NULL;
  int i, j, k;
  k=0;
  while (_kbhit() != 0) _getch();   // flush key-buffer
  for (j=0; j<count; j++)
    {
    if (macrotab[j].macNameStr[0] != 0)
      {
      k++;                          // count entries
      printf("macrotab[%d].macBufPtr=0x%llX ", j, (unsigned long long)macrotab[j].macBufPtr);
      printf(" .macBufLen=%d ",  macrotab[j].macBufLen);
      printf(" .macType=%02X\n", macrotab[j].macType);
      printf("macrotab[%d].macNameStr = <%s>\n",  j, macrotab[j].macNameStr);
      printf("[\n");
      for (i=0; i<macrotab[j].macBufLen; i++) printf("%c", macrotab[j].macBufPtr[i]);
      printf("]\n");

      // Display local macro label(s) 
      tmpPtr = macrotab[j].macLabelBuf;
      if (tmpPtr[0] == 0) printf("LOCAL Label: [ "); 
      else printf("LOCAL Label: ["); 
      while (tmpPtr[0] != 0)
        {
        printf("%s ", tmpPtr);
        tmpPtr += strlen(tmpPtr);
        tmpPtr++;
        } // end while
      printf("\b]\n");
      }
    } // end for
  printf("==> %d structure(s) in macrotab array <==\n", k);
  } // DebugPrintMacroArray

//----------------------------------------------------------------------------
//
//                          DebugPrintIfelseArray
//
//  Usage:  DebugPrintIfelseArray(count);
//
//  typedef struct tagCIFDEF {     // #ifelse cascade structure
//   char      Cifdef[IFELSEMAX];         
//   char      Cifndef[IFELSEMAX];        
//   char      Cif[IFELSEMAX];        
//   char      Celif[IFELSEMAX];          
//   char      Celse[IFELSEMAX];  
//  } CIFDEF, *LPCIFDEF;
//
void DebugPrintIfelseArray(int count)
  {
  char* tmpPtr = NULL;
  int i, j, _Celif;               // local vars
  while (_kbhit() != 0) _getch(); // flush key-buffer

  for (j=1; j<count; j++)         // (j=0) 0-struct is reserved (not displayed)
    {
    _Celif=0;
    for (i=0; i<15; i++) _Celif |= preprocessStack[j].Celif[i];
    if (preprocessStack[j].Cifdef[1]  != 0 ||
        preprocessStack[j].Cifndef[1] != 0 ||
        preprocessStack[j].Cif[1]     != 0 ||
                          _Celif      != 0 ||
        preprocessStack[j].Celse[1]   != 0)
      {
      printf("\nifelseStack[%d].Cifdef[01] = ", j);
      for (i=0; i<2; i++)
        printf("%02X ", (UCHAR)preprocessStack[j].Cifdef[i]);
      printf("\nifelseStack[%d].Cifndef[01]= ", j);
      for (i=0; i<2; i++)
        printf("%02X ", (UCHAR)preprocessStack[j].Cifndef[i]);
      printf("\nifelseStack[%d].Cif[01]    = ", j);
      for (i=0; i<2; i++)
        printf("%02X ", (UCHAR)preprocessStack[j].Cif[i]);
      printf("\nifelseStack[%d].Celif[%02d]  = ", j, preprocessStack[j].CelifCnt);
  //ha//    for (i=0; i<IFELSEMAX; i++)
      for (i=0; i<15; i++)
        printf("%02X ", (UCHAR)preprocessStack[j].Celif[i]);
      printf("\nifelseStack[%d].Celse[01]  = ", j);
      for (i=0; i<2; i++)
        printf("%02X ", (UCHAR)preprocessStack[j].Celse[i]);
      printf("\n");
      }
    } // end for
  printf("---------------------------------\n");
  } //  DebugPrintIfelseArray

//----------------------------------------------------------------------------
//
//                          printf_016llX
//
//  Usage:  printf_016llX();
//
// ..just to printf a 64bit number as 16 ascii hex characters
// Just an alternative to 'printf("%016llX", _val);'
//
// Note: Consider the variables ULONG lvalue and  __int64 qvalue
//       It is important to always 'cast' the correlated skope of the variable 
// The following is OK:
//   printf("nlvalue=%08lX    qvalue=%016llX", lvalue, qvalue);
//   printf("nlvalue=%016llX  qvalue=%016llX", (unsigned long long)lvalue, qvalue);
//
// But this is will printf wrong qvalues:
//   printf("nlvalue=%016llX  qvalue=%016llX", lvalue, qvalue);
extern void edqh(char*, unsigned long long);
void printf_016llX(char* _qvalName, unsigned long long _qvalNum)
  {
  /////////////////////////////////////////////
  char __16llX_Buf[16+1] = {0};              //
  edqh(__16llX_Buf, _qvalNum);               //
  printf("%s=%s\n", _qvalName, __16llX_Buf); //
  /////////////////////////////////////////////
  } // printf_016llX

//----------------------------------------------------------------------------
//
//                          DebugTestStop
//
//  Usage:  DebugTestStop();
//
// ..just to try out any temporary stuff
//
//ha//void DebugTestStop()
//ha//  {                                      
//ha//  printf("pc=%d lineNr=%d\n", pc, lineNr);
//ha//  if (_debugSwPass == 1) printf("PASS1:");
//ha//  else if (_debugSwPass == 2) printf("PASS2:");
//ha//  printf(" %s -- press <ESC> for exit --\n\n", PathFindFileName(_pszfile_));
//ha//  if (_getch() == ESC) exit(0);
//ha//  } // DebugTestStop

//  --DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//****************************************************************************
//****************************************************************************

//--------------------------end-of-c++-module-----------------------------------
