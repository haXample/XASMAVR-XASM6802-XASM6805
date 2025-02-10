// haXASM - Cross-Assembler for 8bit Microprocessors
// equate.h - C++ Developer source file.
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

//------------------------------------------------------------------------------
//
//      haXASM miscellaneous definitions
//
#define ULONG64 unsigned long long int  // unsigned _int64, unsigned __int64
#define LONG64  signed long long int    // _int64, __int64

#define ERR    255       // General error

#define OFF      0       // General purpose switch: OFF
#define ON       1       // General purpose switch: ON

#define INTEL    0       // Syntax: Intel/Microsoft MASM 
#define MOTOROLA 1       // Syntax: Motorola ASM

#define _PASS12  0       // Pass1 as well as Pass2 control 
#define _PASS1   1       // Pass1 control only
#define _PASS2   2       // Pass2 control only

                         // (Atmel AVR only)
#define _BYTE    0x01    // 00000001: .MODEL BYTE   (Byte addresses in listing)
#define _WORD    0x02    // 00000010: .MODEL WORD   (Word addresses in listing) 
#define _SYNTAX  0x04    // 00000100: .MODEL SYNTAX (Check syntax for MOVW, ADIW, SBIW, #endif, ..)
#define _NOINFO  0x08    // 00001000: .MODEL NOINFO (Supress memory/instr info display)

#define RADIX_02   2     // binary
#define RADIX_10  10     // decimal
#define RADIX_16  16     // hexadecimal

#define TAB       0x09   // Tabulator = 1 9 17 .. for assembler
#define CR        0x0D
#define LF        0x0A
#define FF        0x0C   // Form feed (new page)
#define ESC       0x1B
#define SPACE      ' '
#define STRNG     0x27   // ''' Single quote
#define DQUOTE    0x22   // '"' Double quote
#define COMMA      ','                                   
#define _BUCK      '$'   // Intel Program Counter 
#define _STAR      '*'   // Motorola Program Counter
#define COLON      ':'
#define SEMICOLON  ';'   // Comment introducer
#define HASHTAG    '#'   // Alternate suffix for Intel binary notation
#define UNDERSCORE '_'   // Allowed character in symbol names

//------------------------------------------------------------------------------
//
// Enumerated Warning definitions
//
enum WarnCode {
  WARN_NONE,       // 0  reserved for no warning
  WARN_IGNORED,    // 1  Directive ignored (not supported).         
  WARN_DEVICE,     // 2  Directive ignored (refer to uC Data Sheet).
  WARN_TOOLONG,    // 3  Source line too long (>300 characters)      
  WARN_CHKEXIT,    // 4  Check END-OF-FILE directive                
  WARN_MSKVALUE,   // 5  Out of range, value is masked
  WARN_MSKWRONG,   // 6  Check range in expression, masked result is wrong
  //next goes here
  _WARNEND_        // do not use, exceeded maximum warning number of (255) 
  }; // end enum WarnCode

//------------------------------------------------------------------------------
//
// Enumerated Error definitions bit[4:0]  0..1F
//
enum ErrorCode {
  ERR_NONE,        //  0   reserved for no error
  ERR_SYNTAX,      //  1  "Wrong syntax";                             
  ERR_ILLOPCD,     //  2  "Unknown opcode";                           
  ERR_TOOLONG,     //  3  "Line too long or statement too complex";   
  ERR_SYMFULL,     //  4  "Symbol table full";                        
  ERR_ILLOP,       //  5  "Illegal or unknown operand";               
  ERR_ILLOPER,     //  6  "Illegal operator";                         
  ERR_ILLBYTE,     //  7  "Illegal byte value expression";            
  ERR_P1PHASE,     //  8  "Pass1 phase error";                        
  ERR_ILLEXPR,     //  9  "Illegal expression";                       
  ERR_MISSEXP,     // 10  "Missing expression";                       
  ERR_UNDFSYM,     // 11  "Undefined symbol";                         
  ERR_DBLSYM,      // 12  "Doubly defined symbol";                    
  ERR_BADREG,      // 13  "Bad register";                             
  ERR_JMPADDR,     // 14  "Branch out of range";                      
  ERR_INCDPTH,     // 15  "Too many cascaded include files";          
  ERR_ORGINS,      // 16  "ORG directive misplaced (segment overlap)";
  ERR_P1UNDFSYM,   // 17  "Unknown or undefined symbol in Pass1";     
  ERR_DSEG,        // 18  "Instruction not expected in Data Segment"; 
  ERR_SEGADDR,     // 19  "Address out of range";                     
  ERR_ILLDIR,      // 20  "Unknown directive";                        
  ERR_SYMABS,      // 21  "Symbol must be of type 'ABS'";             
  ERR_MACRO,       // 22  "Macro definition or macro too complex";    
  ERR_ILLWORD,     // 23  "Illegal word value, too large";            
  ERR_LONGCONST,   // 24  "Numeric constant too large";               
  ERR_DIVZERO,     // 25  "Divisiom by zero"; 
  ERR_26,          // 26  ""; 
  ERR_27,          // 27  ""; 
  ERR_28,          // 28  ""; 
  ERR_29,          // 29  ""; 
  ERR_30,          // 30  ""; 
  ERR_31,          // 31  "";
  _ERREND_         // do not use, exceeded maximum error number of (31) 
  };               // end enum ErrorCode
                
#define ERR_P1_OFF 0xFF // Ignore Pass1 errors reported by 'errorp1()'
                                        
#define SYSERR_FACCESS 253  //STATUS 249: File access error
#define SYSERR_ABORT   250  //STATUS 250: Assembly aborted
#define SYSERR_CMDLINE 251  //STATUS 251: Command line error
#define SYSERR_FNAME   252  //STATUS 252: Missing extension in src-file name
#define SYSERR_FROPN   253  //STATUS 253: File reopen error
#define SYSERR_MALLOC  254  //STATUS 254: Symbol space allocation error

// symType [3:0] in SYMBOLBUF
#define _CODE    1              // Segment type: CSEG Code segment
#define _DATA    2              // Segment type: DSEG Data segment
#define _EEPROM  3              // Segment type: ESEG EEPROM segment
#define _ABSEQU  4              // ALIS Symblol type: ABSOLUTE value assigned by EQU 
#define _ABSSET  5              // Symblol type: ABSOLUTE value assigned by .SET 
//#define _EXTERN  6            // Segment type: EXTRN segment (deprecated)
//#define _PUBLIC  7            // Segment type: PUBLC segment (deprecated)

// If SymType bit[7] in SYMBOLBUF indicates an error
//    SymType bit[4:0] in SYMBOLBUF = error number
//    (e.g. 0xA2=ERR_DBLSYM)
#define SYMERR_FLAG    0x80 

// SymType [6:5] in SYMBOLBUF special control
// Symbol is defined during Pass1 with .IFNDEF/.IFDEF and 
//  the definition src line should be expanded and shown in listing
#define SYMIFNDEF_FLAG 0x40
#define SYMIFDEF_FLAG  0x20

//------------------------------------------------------------------------------
//
// .LST: Sub headline and formatting (see also 'workst.cpp') (pcc = 16bit)
// 
//        |       |       |       |       |  <-- tab positions
//01234567890123456789012345678901234567890  <-- column numbers
// LOC   OBJ               LINE   SOURCE     <-- sub headline text 
// 1234  12 34 56 78 90:E+12345 C program source code text ................
//
#define LOC16    1          // start column of 16bit program counter
#define OBJ16    LOC16+4+2  // start column of object code bytes
#define SRC16    4*8        // start column (in TABs 9..17..) of source statement

#define LINE16   SRC16-8    // start column of line count
#define MARK16   SRC16-2    // special marker, e.g. 'C' for include file line
#define MACR16   LINE16-1   // special marker, e.g. '+' for macro expansion
#define XSEG16   LINE16-3   // start column of segment type indicator

//------------------------------------------------------------------------------
//
// .LST: Sub headline and formatting (see also 'workst.cpp') (pcc = 24bit)
//
//        |       |       |       |       |          <-- tab positions
//01234567890123456789012345678901234567890          <-- column numbers
// LOC       OBJ                   LINE   SOURCE     <-- headline text 
// 00001234  12 34 56 78 90  :E + 12345 C program source code............................
// ^         ^               ^^ ^ ^     ^ ^
#define LOC32  1            // start column of 16bit program counter
#define OBJ32  LOC32+8+2    // start column of object code bytes
#define SRC32  (5*8)        // start column (in TABs 9..17..) of source statement

#define LINE32 SRC32-8      // start column of line count
#define MARK32 SRC32-2      // special marker, e.g. 'C' for include file line
#define MACR32 LINE32-2     // special marker, e.g. '+' for macro expansion
#define XSEG32 LINE32-5     // start column of segment type indicator

#define INCLUDE_IDM 'C'     // Include file line identifier in listing
#define MACRO_IDM   '+'     // Macro line identifier in listing (not implemented)
#define EQUATE_IDM  '='     // EQU directive line identifier in listing

//------------------------------------------------------------------------------
//
// haXASM (workst.cpp) definitions
//
// Size of Hex data buffer for files .HEX
// :  02 0000 02 0000 FC (CR LF)                             // 9 +4 +4    =17 bytes
// :  10 0200 00 33333333333333333333333333333333 BE (CR LF) // 9 +2*16 +4 =45 bytes
// :  00 0000 01 FF (CR LF)                                  // 9 +4       =13 Bytes
//
// Size of Hex data buffer for files .S19
// S1 13 11F0    FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF FB (CR LF) // = 44 bytes
// S9 03 0000 FC (CR LF)                                     // = 12 bytes
//
// 
// flash PROM Binary data
#define HXBLEN 512*1024  // Flash PROM big enough 512K
//#define HXBLEN 256*1024  // Flash PROM big enough 256K
//#define HXBLEN 64*1024  // Flash PROM big enough 256K
// ASCII hex data =721K (Size of Flash PROM .HEX file buffer) 
//  HXFBLEN = ((HXBLEN/16)*9 +HXBLEN*2 +(HXBLEN/16)*4) +13 +17
#define HXFBLEN (HXBLEN/16)*(9 +32 +4) +13 +17 +(1024)

// EEPROM Binary data
#define EEHXBLEN 16*1024  // EEPROM big enough 16K
//#define EEHXBLEN  8*1024  // EEPROM big enough 8K
// ASCII hex data =23K (Size of EEPROM .HEX file buffer) 
//  EEHXFBLEN = ((EEHXBLEN/16)*9 +EEHXBLEN*2 +(EEHXBLEN/16)*4) +13 +17
#define EEHXFBLEN (EEHXBLEN/16)*(9 +32 +4) +13 +17 +(512)

#define LHTITL       88         // Maximum length of title string
#define LHBUFLEN    150         // Maximum length of Headline buffer

#define OPERLEN     150         // Maximum length of operands fields
#define OPCDLEN      40         // Maximum length of opcode field (==SYMLEN)
#define PATHLEN     200         // Maximum length for filenames (paths)
//#define INBUFLEN  2*150         // Maximum length of line for SRC/.LST file
#define INBUFLEN  2*500         // Maximum length of line for SRC/.LST file
#define LONGLINE 32*1024        // !! 'getline' bug fix (see dcomp.cpp!!

#define LSBUFLEN 4*(INBUFLEN+SRC32+3) // Size of lst line(s) buffer

#define MISS_INSTR_LENGTH 10    // Missing instructions in some AVR microchips
#define MISS_INSTR_BUFLEN 40*10

#pragma pack(1)                 // Alignment of structure elements

//ha//typedef struct tagSPLITFIELD {  // Source line structure (not used)
//ha//  char label[SYMLEN+1];         // Label field
//ha//  char opcod[OPCDLEN+1];        // Opcode field
//ha//  char oper1[OPERLEN+1];        // Operand1 field
//ha//  char oper2[OPERLEN+1];        // Operand2 field
//ha//  char oper3[OPERLEN+1];        // Operand3 field
//ha//} SPLITFIELD, *LPSPLITFIELD;

typedef struct tagINSTR {       // CPU instruction structure
  char*  mneStr;                // Instruction Mnemonic string
  UCHAR  mneGrp;                // Instruction group
  UCHAR  mneVal;                // 8bit Instruction default value
} INSTR, *LPINSTR;

typedef struct tagINSTR16 {     // CPU instruction structure
  char*  mneStr;                // Instruction Mnemonic string
  UCHAR  mneGrp;                // Instruction group
  UINT   mneVal16;              // 16bit Instruction default value
} INSTR16, *LPINSTR16;

#define SYMLEN 40               // Maximum length of symbolic names (Atmel AVR)
#define SYMENTRIES 9000         // Maximum of 9000 symbols in SYMTAB
typedef struct tagSYMBOLBUF {   // Symbol structure
  char  symString[SYMLEN+1];    // Symbol/Label name  string
  ULONG symAddress;             // Symbol value or pointer to operand string
  UCHAR symType;                // Symbol type (ABS, C-SEG, UNDEFINED)
} SYMBOLBUF, *LPSYMBOLBUF;      // Structured symbol buffer

#define SYMUNDEFBUFSIZE 20*SYMLEN  // should be enough (not all symbols are of size SYMLEN)

#define P1ERRMAX 100            // Register up to 40 Pass1 errors only
typedef struct tagP1ERRTBL {    // Error line structure
  int lineNr;                   // Source line number
  UCHAR errCode;                // Error code
  char  errText[OPERLEN+1];     // Error info text string
} P1ERRTBL, *LPP1ERRTBL;        // Structured Pass1 error buffer

#define MACBUFSIZE     30*INBUFLEN // 30*300 ~ 9K Buffer for src lines a macro can define
#define MACXLABBUFSIZE 40*INBUFLEN // 40*300 ~12K Buffer for src lines + local labels in macros
#define MACLABBUFSIZE  10*SYMLEN   // 10*40  ~.5K Buffer for local labels in macro

#define MACENTRIES 1*1000       // Maximum nr macro struct for macros
#define MACBUFFERS 4*1000       // Maximum number of distributed macro buffers
typedef struct tagMACROBUF {    // Macro structure
  char  macNameStr[SYMLEN+1];   // Macro name string
  char  macLabelBuf[MACLABBUFSIZE]; // Buffer for local labels in macro
  char* macBufPtr;              // Pointer to macro body (instruction lines)
  int   macBufLen;              // Length of macro buffer contents
  int   macDefCount;            // Assigned when a macro is being defined
  int   macExpCountP1;          // Incremented when a macro is being expanded in _PASS1
  int   macExpCountP2;          // Incremented when a macro is being expanded in _PASS2
  UCHAR macType;                // Macro type reserved = 0
} MACROBUF, *LPMACROBUF;        // Structured macro buffer

#define MACCASCADEMAX 20        // Maximum number of cascaded macros
  typedef struct tagMACCASCADE {// Macro cascading structure (Macros within macros)
    char* macXferBufPtr;        // Pointer to current macro Xfer line string
    int   macXferBufLen;        // Length of macro Xfer contents
    char* macSaveBufPtr;        // Pointer to complete macro contents
    int   macSaveBufLen;        // Length of complete macro contents
  } MACCASCADE, *LPMACCASCADE;

//ha//#define OPERXENTRIES 20 ///////// Wont work with ibmkeyboard.asm - Why??? to be checked!!!
#define OPERXENTRIES 10         // Maximum of 10 undefined expr operands (forward ref.)
typedef struct tagOPERXSTR {    // Operand structure
  char operX[OPERLEN+1];        // operand expression string w/ symbols/labels 
  char symbolX[SYMLEN+1];       // symbolname assigned to value of operX 
  char errsymX[SYMLEN+1];       // name of an undefined symbol in operX
} OPERXBUF, *LPOPERXBUF;        // Structured oprand string buffer

#define ICFCOUNTMAX 100         // 200 Maximum of cascaded .INC files
typedef struct tagINCFILES {    // Include file cascade structure
  char      iclPath[PATHLEN];   // Include file name string
  int       iclSeekPos;         // Include file stream seek position
  int       iclLine;            // Counter of current include file line read
} INCFILES, *LPINCFILES;

#define CMOD_IFDEF    0x10      // Condition mode: #ifdef  / .IFDEF 
#define CMOD_IFNDEF   0x20      // Condition mode: #ifndef / .IFNDEF
#define CMOD_IF       0x30      // Condition mode: #if     / .IF    

#define IFELSEMAX 100           // 100 cascaded #if/#else/.IF/.ELSE.. conditions
typedef struct tagCIFDEF {      // #ifelse cascade structure
  char      Cifdef[2];         
  char      Cifndef[2];        
  char      Cif[2];
  union {
    char    CelifCnt;        
    char    Celif[IFELSEMAX];
  };          
  char      Celse[2];          
} CIFDEF, *LPCIFDEF;

//ha//typedef struct tagCIFDEF {      // #ifelse cascade structure
//ha//  int      Cifdef[2];         
//ha//  int      Cifndef[2];        
//ha//  int      Cif[2];
//ha//  union {
//ha//    int    CelifCnt;        
//ha//    int    Celif[IFELSEMAX];
//ha//  };          
//ha//  int      Celse[2];          
//ha//} CIFDEF, *LPCIFDEF;

//ha//typedef struct tag_IFDEF { // .IF/.ELSE separate structure (deprecated)
//ha//  char      _ifdef[2];         
//ha//  char      _ifndef[2];        
//ha//  char      _if[2];          
//ha//  char      _elif[IFELSEMAX];          
//ha//  char      _else[2];          
//ha//} _IFDEF, *LP_IFDEF;

#define ORGMAX_C 100          // max 100 scattered ORG'ed memory blocks
#define ORGMAX_D  50          // max  50 scattered ORG'ed memory blocks
#define ORGMAX_E  20          // max  20 scattered ORG'ed memory blocks
typedef struct tag_ORGSEG {   // Segment usage structure
  ULONG     sStart;         
  ULONG     sEnd;         
} ORGSEG, *LPORGSEG;

typedef struct tagDEVICEAVR { // Device AVR structure
  char*     deviceName;
  ULONG     FLASHStart;       
  ULONG     FLASHSize;       
  ULONG     SRAMStart;       
  ULONG     SRAMSize;       
  ULONG     EEPROMStart;       
  ULONG     EEPROMSize;
  UINT      missingInst;  
} DEVICEAVR, *LPDEVICEAVR;
  
//------------------------------------------------------------------------------
