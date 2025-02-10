// haXASM - Cross-Assembler for 8bit Microprocessors
// assgn.cpp - C++ Developer source file.
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

#include <sys\types.h> // For _open( , , S_IWRITE) needed for VC 2010
#include <sys\stat.h>  // For filesize
#include <fcntl.h>     // File Modes
#include <io.h>        // File open, close, access, etc.
#include <conio.h>     // For _putch(), _getch() ..
#include <string>      // printf, etc.

#include <shlwapi.h>   // Library shlwapi.lib for PathFileExistsA

#include <iostream>    // I/O control
#include <fstream>     // File control

#include <windows.h>   // For console specific functions

#include "equate.h"
#include "extern.h"    // Variables published in workst.cpp

using namespace std;

// Global Variables
int Srcfh=0, Iclfh=0;  // Filehandle read (*.ASM, *.INC)

// Extern variables and functions
extern char* szErrorOpenFailed;
extern char* errtx_sys;
extern char* errtx_alloc;

extern char szSrcFileName[];              // e.g = "file.ASM"
extern char szLstFileName[];              // e.g = "file.LST"
extern char szHexFileName[];              // e.g = "file.HEX"
extern char szEEPHexFileName[];           // e.g = "File.EEP.HEX"
extern char szBinFileName[];              // e.g = "file.Bin"
extern char szIclFileName[];              // e.g = "file.INC"

extern char* pszCurFileName;              // e.g = "file.ASM / File.INC"

extern ifstream SrcFile;                  // Filestream read (*.ASM)
extern ifstream IclFile;                  // Filestream read (*.INC)

extern void DebugStop(int, char*, char*); // Usage: DebugStop(testNr, "Function()", __FILE__);
extern void DebugPrintBuffer(char*, int); // Usage: DebugPrintBuffer(buffer, count);
extern void DebugPrintSymbolArray(int);   // Usage: DebugPrintSymbolArray(count);

extern void ClearIfdefStack();
extern BOOL AllocateBuffers();
extern int HexFileFormat();
extern void oper_preval();
extern void errchk(char*, int);
extern void p1errorl(UCHAR, char*);

// Forward declaration of functions included in this code module:
void CloseSrcFile();
void AbortAssembly(int);

//-----------------------------------------------------------------------------
//
//                      GetDate
//
// Retrieves the system's date/time and emits date and time as strings.
//
// typedef struct _SYSTEMTIME {
//   WORD wYear;
//   WORD wMonth;
//   WORD wDayOfWeek;
//   WORD wDay;
//   WORD wHour;
//   WORD wMinute;
//   WORD wSecond;
//   WORD wMilliseconds;
// } SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;
//
void GetDate()
  {
  SYSTEMTIME stLocal;
 
  // Receive the system's local time & date.
  GetLocalTime(&stLocal);
  // Build a string (lh_date) representing the date: Day/Month/Year.
  //sprintf(lh_date, "%02d/%02d/%d", stLocal.wMonth, stLocal.wDay, stLocal.wYear); // US
  sprintf(lh_date, "%02d/%02d/%d", stLocal.wDay, stLocal.wMonth, stLocal.wYear);   // EUROPE
  sprintf(lh_time, "%02d:%02d:%02d", stLocal.wHour, stLocal.wMinute, stLocal.wSecond);
  } // Getdate

//-----------------------------------------------------------------------------
//
//                      AbortAssembly
//
// Abort assembly and perform an error-exit to system
//
void AbortAssembly(int errCode)
  {
  exit(errCode);
  } // AbortAssembly

//-----------------------------------------------------------------------------
//
//                      OpenSrcFile
//
// Open input source file: .ASM
// Open as CRLF or 0-terminated text filestream
// (fixed: SrcFile.getline problem)
//
// Open as binary filehandle (fast)
//  struct _stat Stat;
//  _stat(szSrcFileName, &Stat);   
//  IclFileSize = Stat.st_size     // file size (malloc)
//
//  if ((Srcfh=open(_filename, O_RDONLY|O_BINARY)) == ERR)
//    {
//    printf(szErrorOpenFailed, _filename);
//    exit(SYSERR_FROPN);
//    }
// System errors cause a program abort
//
void OpenSrcFile(char* _filename)
  {
  // ------------------
  // File handle method
  // ------------------
  struct _stat Stat;
  ULONG SrcFileSize, bytesrd;

  // Open as binary filehandle
  Srcfh = open(_filename, O_RDONLY|O_BINARY);
  errchk(szSrcFileName, GetLastError());     
 
  _stat(_filename, &Stat);     // Get File info structure
  SrcFileSize = Stat.st_size;  // file size (for malloc)

  pszSrcFilebuf = (char *)GlobalAlloc(GPTR,SrcFileSize+4);
  errchk("XASMAVR.EXE", GetLastError());     
  pszSrcFilebuf0 = pszSrcFilebuf;

  bytesrd = read(Srcfh, pszSrcFilebuf, SrcFileSize);
  errchk(szSrcFileName, GetLastError());     

  close(Srcfh);

  // Add a possibly missing CRLF in src at eof (required)
  if (pszSrcFilebuf[SrcFileSize-2] != CR)
    {
    pszSrcFilebuf[SrcFileSize]   = CR;
    pszSrcFilebuf[SrcFileSize+1] = LF;
    }

  // ------------------
  // File stream method
  // ------------------
  SrcFile.open(_filename, ios::in);       
  if (!SrcFile)
    {
    // Program abort
    printf(szErrorOpenFailed, _filename);
    exit(SYSERR_FROPN);
    }

  // Init globals
  _curline = 0;
  pszCurFileName = _filename;
  } // OpenSrcFile


//-----------------------------------------------------------------------------
//
//                      CloseSrcFile
//
// Close input source file: .ASM
// Close source file input filestream (filehandle: close(Srcfh))
//
// typedef struct tagINCFILES {    // Include file cascade structure
//   char      iclPath[PATHLEN];   // Include file name string
//   streampos iclSeekPos;         // Include file stream seek position
//   int       iclLine;            // Counter of current include file line read
// } INCFILES, *LPINCFILES;
//
// System errors cause a program abort
//
void CloseSrcFile()
  {
  // ------------------
  // File handle method
  // ------------------
  pszSrcFilebuf = pszSrcFilebuf0;  // reset src file buffer pointer to start

  // ------------------
  // File stream method
  // ------------------
  SrcFile.close();
  errchk(szSrcFileName,  GetLastError());    
  } // CloseSrcfile


//-----------------------------------------------------------------------------
//
//                      OpenIclFile
//
// Open input source file: *.INC
// Open as CRLF or 0-terminated text filestream
// (fixed: IclFile.getline problem)
//
//  struct _stat Stat;
//  _stat(szIclFileName, &stat);   
//  IclFileSize = stat.st_size     // file size (malloc)
//
// Open as binary filehandle (slow)
//  if ((Iclfh=open(_filename, O_RDONLY|O_BINARY)) == ERR)
//    {
//    printf(szErrorOpenFailed, _filename);
//    exit(SYSERR_FROPN);
//    }
//
// typedef struct tagINCFILES {    // Include file cascade structure
//   char      iclPath[PATHLEN];   // Include file name string
//   streampos iclSeekPos;         // Include file stream seek position
//   int       iclLine;            // Counter of current include file line read
// } INCFILES, *LPINCFILES;
//
// INCFILES icfStack[ICFCOUNTMAX];     // Maximum of cascaded .INC flies 
// LPINCFILES icfStack_ptr = icfStack; // Global pointer to array of .INC files
//
void OpenIclFile(char* _filename)
  {
  // ------------------
  // File stream method
  // ------------------
  // Save current .INC file position context and close file
  if (swicl != 0)
    {
    icfStack[swicl].iclSeekPos = IclFile.tellg();     
    icfStack[swicl].iclLine = _curline; 
    IclFile.close();            // no error check
    }
  else // Save source file context [swicl=0]
    { 
    for (_i=0; _i<PATHLEN; _i++) icfStack[swicl].iclPath[_i] = szSrcFileName[_i];
    icfStack[swicl].iclLine = _curline; 
    }

  //------------------------------------------------------------------  STD-OK
  IclFile.open(_filename, ios::in);                                  //
  if (!IclFile)                                                      //
    {                                                                //
    // Program abort                                                 //
    printf(szErrorOpenFailed, _filename);                            //
    exit(SYSERR_FROPN);                                              //
    }                                                                //
  pszCurFileName = _filename;                                        //
                                                                     //
  _curline = 0;              // Restart current source line counter  //
  swicl++;                   // Increment include file cascade       //
  if (swicl > ICFCOUNTMAX)                                           //
    {                                                                //
    p1errorl(ERR_INCDPTH, pszCurFileName);                           //
    IclFile.close();         // Close include file input filestream  //
    return;                                                          //
    }                                                                //
  //------------------------------------------------------------------

  // save new .INC file params
  for (_i=0; _i<PATHLEN; _i++) icfStack[swicl].iclPath[_i] = pszCurFileName[_i];
  icfStack[swicl].iclLine = _curline;

  // Reset any pending srcEOF                                        //ha//
  // Important when ".INCLUDE <file.inc>" statement is last w/o CRLF //ha//
  srcEOF = FALSE;                                                    //ha//
  } // OpenIclFile


//-----------------------------------------------------------------------------
//
//                      OpenIncludeFile
//
// Parse Include directive string and Open input source file: *.INC
// Input: swicl=0 (see Pass1&2.cpp);
//        _dirstr[] = (filename.inc) or  _dirstr[] = "filename.inc"
//
int OpenIncludeFile(char* ICL_dirstr, char* _dirStr)
  {
  char* _pszINCstrt;
  char* _pszINCend;
  char _savINC;
  
  // check '('
  if ((_pszINCstrt = strstr(&_dirStr[strlen(ICL_dirstr)], "("))  != 0)
    _pszINCend = strstr(_pszINCstrt+1, ")"); // find ')'
  
  // check ' "'
  else if ((_pszINCstrt = strstr(&_dirStr[strlen(ICL_dirstr)], "\x22")) != 0)
    _pszINCend = strstr(_pszINCstrt+1, "\x22"); // find '"'

  // check ' <'
  else if ((_pszINCstrt = strstr(&_dirStr[strlen(ICL_dirstr)], "<")) != 0)
    _pszINCend = strstr(_pszINCstrt+1, ">"); // find '>'

  if (_pszINCstrt != NULL && _pszINCend != NULL)
    {
    _savINC = _pszINCend[0];         // Save  ')' or '"'
    _pszINCstrt++;                   // point to 1st char of filename
    _pszINCend[0] = 0;               // 0-Teminate filename string
    StrCpy(szIclFileName, _pszINCstrt); 
    OpenIclFile(szIclFileName);      // Open include file     
    _pszINCend[0] = _savINC;         // Restore ')' or '"'
    }

  return(swicl);                     // success: swicl!=0, failure: swicl==0
  } // OpenIncludeFile

//-----------------------------------------------------------------------------
//
//                      CloseIclFile
//
// Close input include file: *.INC
// Close include file input filestream  (filehandle: close(Iclfh))
//
// typedef struct tagINCFILES {    // Include file cascade structure
//   char      iclPath[PATHLEN];   // Include file name string
//   streampos iclSeekPos;         // Include file stream seek position
//   int       iclLine;            // Counter of current include file line read
// } INCFILES, *LPINCFILES;
//
// INCFILES icfStack[ICFCOUNTMAX];     // Maximum of cascaded .INC flies 
// LPINCFILES icfStack_ptr = icfStack; // Global pointer to array of .INC files
//
void CloseIclFile()
  {
  // ------------------
  // File stream method
  // ------------------
  swicl--;                         // Decrement include file cascade
  IclFile.close();                 // Close include file input filestream
  errchk(szIclFileName, GetLastError());

  // Restore current .INC file position and open file
  if (swicl != 0)
    {
    pszCurFileName = icfStack[swicl].iclPath;
    IclFile.open(pszCurFileName, ios::in);                           
    if (!IclFile)                                                    
      {                                                              
      // Program abort                                               
      printf(szErrorOpenFailed, pszCurFileName);                           
      exit(SYSERR_FROPN);                                                      
      }
    IclFile.seekg(icfStack[swicl].iclSeekPos);   
    _curline = icfStack[swicl].iclLine;
    }
  
  else // Restore source file context [swicl=0]
    { 
    pszCurFileName = icfStack[swicl].iclPath;
    _curline = icfStack[swicl].iclLine;
    }
  } // CloseIclfile


//-----------------------------------------------------------------------------
//
//                      assgn
//
// Switches and default values are initialized.
// Establish pointers to "allocated" buffers
//
void assgn()
  {                                  
  char* _ptrFiletype = NULL;
  
  // System buffer allocation (global)
  if (AllocateBuffers() == FALSE)
    {
    // >>>>>>>>> ERROR: MEMORY ALLOC <<<<<<<<<<
    printf("%s%s", errtx_sys, errtx_alloc);   
    exit(SYSERR_MALLOC);
    }
  
  // Buffer "allocation" (static)
  pszListBuf = szListBuf;            // Init listing buffer
  
  // Clear structured segment layout buffers
  // Segment usage and memory layout structure
  // typedef struct tag_ORGSEG {    
  //   ULONG     sStart;
  //   ULONG     sEnd  ;         
  // } ORGSEG, *LPORGSEG;
  //
  // Define a local pointer to the global array of operandX structures
  LPORGSEG segstruc_ptr = csegLayout_ptr;
  for (_i=0; _i<ORGMAX_C; _i++)
    {
    segstruc_ptr->sStart = 0; // CSEG
    segstruc_ptr->sEnd   = 0; 
    segstruc_ptr++;           // Advance pointer
    } // end for

  segstruc_ptr = dsegLayout_ptr;
  for (_i=0; _i<ORGMAX_D; _i++)
    {
    segstruc_ptr->sStart = 0; // DSEG
    segstruc_ptr->sEnd   = 0; 
    segstruc_ptr++;           // Advance pointer
    } // end for

  segstruc_ptr = esegLayout_ptr;
  for (_i=0; _i<ORGMAX_E; _i++)
    {
    segstruc_ptr->sStart = 0; // ESEG
    segstruc_ptr->sEnd   = 0; 
    segstruc_ptr++;           // Advance pointer
    } // end for

  // Clear structured Pass1 error buffer
  // typedef struct tagP1ERRTBL {  // Error line structure
  //   int lineNr;                 // Source line number
  //   UCHAR errCode;              // Error code
  //   char  errText[OPERLEN+1];   // Error symbol string
  // } P1ERRTBL, *LPP1ERRTBL;      // Structured Pass1 error buffe
  //
  // Define a local pointer to the global array of symbol structures
  LPP1ERRTBL p1errstruc_ptr = p1errTbl;

  for (_i=0; _i<P1ERRMAX/sizeof(P1ERRTBL); _i++)
    {
    // Init elements of current symbol structure
    p1errstruc_ptr->lineNr  = 0;
    p1errstruc_ptr->errCode = 0;
    for (_i=0; _i<OPERLEN+1; _i++) p1errstruc_ptr->errText[_i] = 0;
    // Advance pointer by 'sizeof(P1ERRTABLE)' and point to next structure
    p1errstruc_ptr++;                           
    } // end for


  // Clear structured symbol buffer
  //  typedef struct tagSYMBOLBUF {  // Symbol structure
  //    char  symString[SYMLEN+1];   // Symbol/Label name string
  //    ULONG symAddress;            // Symbol value or pointer to operand string
  //    UCHAR symType;               // Symbol type (ABS, C-SEG, UNDEFINED)
  //  } SYMBOLBUF, *LPSYMBOLBUF;     // Structured symbol buffer
  //
  // Define a local pointer to the global array of symbol structures
  symboltab_ptr = symboltab;
  LPSYMBOLBUF symstruc_ptr = symboltab_ptr;

  // Init-clear global array of symbol structures 
  for (_i=0; _i<SYMENTRIES; _i++)
    {
    // Init elements of current symbol structure
    for (_j=0; _j<SYMLEN+1; _j++) symstruc_ptr->symString[_j] = 0;
    symstruc_ptr->symAddress = NULL;
    symstruc_ptr->symType = 0x00;
    // Advance pointer by 'sizeof(SYMBOLBUF)' and point to next structure
    symstruc_ptr++;                           
    } // end for

//ha//  // Clear structured macro buffer    // ..Already done in GlobalAlloc(GPTR, ..)
//ha//  //  typedef struct tagMACROBUF {    // Macro structure
//ha//  //    char  macNameStr[SYMLEN+1];   // Macro name string
//ha//  //    char  macLabelBuf[10*SYMLEN]; // Buffer for local labels in macro
//ha//  //    char* macBufPtr;              // Pointer to macro body (instruction lines)
//ha//  //    int   macBufLen;              // Length of macro buffer contents
//ha//  //    int   macDefCount;            // Assigned when a macro is being defined
//ha//  //    int   macExpCountP1;          // Incremented when a macro is being expanded in _PASS1
//ha//  //    int   macExpCountP2;          // Incremented when a macro is being expanded in _PASS2
//ha//  //    UCHAR macType;                // Macro type reserved = 0
//ha//  //  } MACROBUF, *LPMACROBUF;        // Structured macro buffer
//ha//  //
//ha//  // Define a local pointer to the global array of symbol structures
//ha//  LPMACROBUF macstruc_ptr = macrotab;
//ha//
//ha//  // Init-clear global array of symbol structures 
//ha//  for (_i=0; _i<MACENTRIES; _i++)
//ha//    {
//ha//    // Init elements of current symbol structure
//ha//    for (_j=0; _j<SYMLEN+1; _j++) macstruc_ptr->macNameStr[_j] = 0;
//ha//    for (_j=0; _j<10*SYMLEN; _j++) macstruc_ptr->macLabelBuf[_j] = 0;
//ha//    macstruc_ptr->macBufPtr     = NULL;
//ha//    macstruc_ptr->macBufLen     = 0x00;
//ha//    macstruc_ptr->macDefCount   = 0x00;
//ha//    macstruc_ptr->macExpCountP1 = 0x00;
//ha//    macstruc_ptr->macExpCountP2 = 0x00;
//ha//    macstruc_ptr->macType       = 0x00;
//ha//    // Advance pointer by 'sizeof(MACROBUF)' and point to next structure
//ha//    macstruc_ptr++;                           
//ha//    } // end for

  // Clear structured macro cascade buffer
  // typedef struct tagMACCASCADE { // Macro cascading structure (Macros within macros)
  //   char* macXferBufPtr;         // Pointer to current macro Xfer line string
  //   int   macXferBufLen;         // Length of macro Xfer contents
  //   char* macSaveBufPtr;         // Pointer to complete macro contents
  //   int   macSaveBufLen;         // Length of complete macro contents
  // } MACCASCADE, *LPMACCASCADE;
  //
  //
  // Define a local pointer to the global array of symbol structures
  LPMACCASCADE maccasstruc_ptr = macrocascade;

  // Init-clear global array of maccascade structures 
  for (_i=0; _i<MACCASCADEMAX; _i++)
    {
    maccasstruc_ptr->macXferBufPtr = NULL;
    maccasstruc_ptr->macXferBufLen = 0; 
    maccasstruc_ptr->macSaveBufPtr = NULL;
    maccasstruc_ptr->macSaveBufLen = 0; 
    maccasstruc_ptr++;             // Advance pointer
    } // end for

  // Clear structured ifdef buffer
  ClearIfdefStack();

  // Init-clear global symUndefBuf 
  for (_i=0; _i<SYMUNDEFBUFSIZE; _i++) symUndefBuf[_i] = 0;

  // Clear global array of operandX structures
  //  typedef struct tagOPERXSTR { // Operand structure
  //    char operX[OPERLEN+1];     // operand expression string w/ symbols/labels 
  //    char symbolX[SYMLEN+1];    // symbolname assigned to value of operX 
  //    char errsymX[SYMLEN+1];    // name of an undefined symbol in operX
  //  } OPERXBUF, *LPOPERXBUF;     // Structured oprand string buffer
  //
  // Define a local pointer to the global array of operandX structures
  LPOPERXBUF p1ustruc_ptr = p1undef_ptr;

  // Init-clear global array of operandX structures 
  for (_i=0; _i<OPERXENTRIES; _i++)
    {
    // Init elements of current symbol structure
    for (_j=0; _j<OPERLEN; _j++) p1ustruc_ptr->operX[_j] = 0;
    for (_j=0; _j<SYMLEN; _j++)
      {
      p1ustruc_ptr->symbolX[_j] = 0;
      p1ustruc_ptr->errsymX[_j] = 0;
      }
    // Advance pointer by 'sizeof(OPERXBUF)' and point to next structure
    p1ustruc_ptr++;                           
    } // end for

  printf(signon);
  printf(mssge);
  
  // Open source file .ASM
  OpenSrcFile(szSrcFileName);
  pszCurFileName = szSrcFileName;

  // Build filenames for .LST .HEX or .S19 and .BIN
  StrCpy(szLstFileName, szSrcFileName); 
  _ptrFiletype = PathFindExtension(szLstFileName);
  _ptrFiletype[1] = 0;
  strcat(_ptrFiletype, "LST");

  StrCpy(szBinFileName, szSrcFileName); 
  _ptrFiletype = PathFindExtension(szBinFileName);
  _ptrFiletype[1] = 0;
  strcat(_ptrFiletype, "BIN");   // Intel HEX-FORMAT file extension

  StrCpy(szHexFileName, szSrcFileName); 
  _ptrFiletype = PathFindExtension(szHexFileName);
  _ptrFiletype[1] = 0;

  if (HexFileFormat() == INTEL)  // Intel or Motorola)
    strcat(_ptrFiletype, "HEX"); // Intel HEX-FORMAT file extension
  else 
    strcat(_ptrFiletype, "S19"); // Motorola S-RECORD file extension

  // Clear
  pccw    = 0;       // CSEG 16bit (Word) program counter
  pcc     = 0;       // CSEG program counter
  pcd     = 0;       // DSEG program counter
  pce     = 0;       // ESEG program counter
  pcValue = 0;       // Unassigned Program counter

  p1erc = 0;
  swlab = 0;

  swicl = 0;         // Deactivate '$INCLUDE' directive
  swicleof = 0;

  srcEOF   = FALSE;  // Source file EOF encountered (.EXIT directive is missing, Atmel)
  iclEOF   = FALSE;  // Include file EOF encountered (.EXIT directive is missing, Atmel)
  swif     = FALSE;  // Conditionally control src line: .IF/.ELSE/.ELSEIF 
  swifdef  = FALSE;  // Conditionally control src line: .IFDEF/.IFNDEF/.ELSE/(.ELSEIFDEF)
  swelseif = FALSE;  // Conditionally control src line: .ELSEIF/(.ELSEIFDEF)

  swlistmac  = FALSE; // Macro expansion shown in .LST file
  swdefmacro = FALSE; // Macro definition control src line
  swexpmacro = FALSE; // Macro expansion src line read control
  macroCount = 0;     // Number of macros defined within .SRC file
  macroBegin = 0;     // Flag for .MACRO statement

  // Pre-set
  SegType = _CODE;                      // Assume currently CSEG
  swlst   = 1;                          // List src lines in lst file
  if (cmdnosym != -1) nosym = cmdnosym; // '/S' from cmd-line (=.SYMBOLS)
  else nosym = 0;                       // Append symbol table in lst file

  GetDate();       // Get date into sign-on 

  // Init label field w/ predefined operands (if any)
  // Init signon header for .LST
  // Init XASM CPU type (e.g. Intel, Motorola, Atmel (Microchip), ..)
  oper_preval();

  if (AtmelFlag != 0)
    {
    // Atmel default = .NOSYMBOLS (some *.INCs had >5000 symbols)
    if (cmdnosym == -1) nosym = 1;
    StrCpy(szEEPHexFileName, szSrcFileName); 
    _ptrFiletype = PathFindExtension(szEEPHexFileName);
    _ptrFiletype[1] = 0;
    strcat(_ptrFiletype, "EEP.HEX"); // Intel HEX-FORMAT EEPROM file extension
    }
  } // assgn

//-----------------------------------------------------------------------------

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("srcEOF=%d  iclEOF=%d  swicl=%d  pszCurFileName=%s", srcEOF, iclEOF, swicl, pszCurFileName);
//ha//DebugStop(1, "OpenIclFile()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("symboltab=%08X\n&symboltab[SYMENTRIES-1]=%08X\n&(SYMBOLBUF)symboltab[SYMENTRIES-1]=%08X\n"
//ha//       "sizeof(SYMBOLBUF)=%08X\n(SYMENTRIES+1) * sizeof(SYMBOLBUF)=%08X\n"
//ha//       "&symboltab[SYMENTRIES-1]-symboltab=%08lX\nGlobalSize(symboltab)=%08X" ,
//ha//        symboltab, &symboltab[SYMENTRIES-1], &(SYMBOLBUF)symboltab[SYMENTRIES-1],
//ha//        sizeof(SYMBOLBUF), (SYMENTRIES+1) * sizeof(SYMBOLBUF),
//ha//        ((ULONG)&symboltab[SYMENTRIES-1]-(ULONG)symboltab), GlobalSize(symboltab));
//ha////DebugPrintSymbolArray(3);  
//ha//DebugStop(45, "assgn()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//--------------------------end-of-c++-module-----------------------------------

