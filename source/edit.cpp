// haXASM - Cross-Assembler for 8bit Microprocessors
// edit.cpp - C++ Developer source file.
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

#include <shlwapi.h>   // StrStrI, StrCmpI, StrCmpNI

#include <sys/stat.h>  // For filesize
#include <iostream>    // I/O control                                                   
#include <fstream>     // File control

#include <windows.h>   // For console specific functions

#include "equate.h"
#include "extern.h"    // Variables published in workst.cpp

using namespace std;

// Global variables
ULONG detectedMaxBinsize=0;
ULONG offsetBinFilebuf=0;

// Extern variables and functions
extern void DebugStop(int, char*, char*); // Usage: DebugStop(testNr, "Function()", __FILE__);
extern void DebugPrintBuffer(char*, int); // Usage: DebugPrintBuffer(buffer, count);

extern char szHexFileName[];              // e.g = "file.HEX" "file.S19"
extern char szBinFileName[];              // e.g = "file.BIN"
extern char szEEPHexFileName[];           // e.g = "File.EEP.HEX"

extern void lineAppendCRLF(char*);
extern void errchk(char*, int);
extern void linut(char*);
extern void erout();
extern void warnout();
extern void errorp2(int, char*);

extern int HexFileFormat();

extern ofstream HexFile;      // File write (*.HEX)
extern ofstream EEPHexFile;   // Filestream write (*.EEP.HEX)

// Forward declaration of functions included in this code module:
void eddh(char*, ULONG);
void edwh(char*, UINT);
void edbh(char*, UINT);
void flhex();
void wrhex(int);
void edit_segtype();
void CheckPc();
void edpri();

//-----------------------------------------------------------------------------
//
//                      GetPcValue
//
// Load currently valid Program Counter (either Code, Data or EEPROM Segment)
// Output: ULONG pcValue (contains the currently valid pcc/pcd/pce
// Default is always code segment.
//
void GetPcValue()
  {
  CheckPc(); // Check Program Counter overun

  if (SegType == _CODE)
    {
    pcValue = pcc;
    pccw = pcc/2;
    }
  else if (SegType == _DATA) pcValue = pcd;
  else if (SegType == _EEPROM) pcValue = pce;
  } // GetPcValue

//-----------------------------------------------------------------------------
//
//                      AssignPc
//
// Assign Program Counter either to Code, Data or to EEPROM Segment
// Input: ULONG pcValue (contains the actually calculated pcc/pcd/pce
//
void AssignPc()
  {
  if (SegType == _CODE)
    {
    pcc = pcValue;
    pccw = pcc/2;       // For Atmel AVR code segment (word count)
    }
  else if (SegType == _DATA) pcd = pcValue;
  else if (SegType == _EEPROM) pce = pcValue;
  } // AssignPc

//-----------------------------------------------------------------------------
//
//                      CheckPc
//
// Pass2 only:
// Check Program Counter overun either in Code, Data or in EEPROM Segment
// Input: ULONG pcValue (contains the actually calculated pcc/pcd/pce
//
// Defaults:
//  RomSize    = 64*1024;
//  SRamStart  =   0x000;
//  SRamSize   = 16*1024; 
//  EEPromSize =  4*1024;
//
// Example 1 (not implemented):
// >>>.MESSAGE "Segment start above allowed high address: 0x1100"
// .MESSAGE "DATA: Start = 0x0100"
// .MESSAGE "      End = 0x1473"
// .MESSAGE "      Length = 0x1374 (4980 bytes)"
// .MESSAGE "Overlap=N"
// .ERROR   "Assembly aborted."
//
// Example 2 (implemented):
// >>>_XASMAVR_test.asm(2245): ERROR - Segment address out of range
//        CSEG: Start = 0x0000FFFE  End = 0x0001000E  Size = 16 byte(s)
// >>>_XASMAVR_test.asm: ERROR - Segment address out of range : 0x8000 < CSEG < 0xFFFF
//
void CheckPc()
  {
  // Pass2 only
  if (swpass == _PASS1) return;  
  // No error line dedicated to directives or 2 error lines max
  if (StrCmpI(fopcd, ".ORG") != 0 && (fopcd[0] == '.' || fopcd[0] == '#')) return;
  
  // No error info (info is given with memory map at end of listing)
  if (SegType == _CODE &&
      _errCSEG++ < 2   &&
      (pcc > (RomStart+RomSize) || pcc < RomStart))
    {
    errorp2(ERR_SEGADDR, NULL);
    }

  // No error info (info is given with memory map at end of listing)
  else if (SegType == _DATA &&
           _errDSEG++ < 2   &&
           (pcd > (SRamStart+SRamSize) || pcd < SRamStart))
    {
    errorp2(ERR_SEGADDR, NULL);
    }
  
  // No error info (info is given with memory map at end of listing)
  else if (SegType == _EEPROM &&
           _errESEG++ < 2     &&
           (pce > (EEPromStart+EEPromSize) || pce < EEPromStart))
    {
    errorp2(ERR_SEGADDR, NULL);
    }
  } // CheckPc

//-----------------------------------------------------------------------------
//
//                      cllin
//
// Space-out List buffer line
//
void cllin()
  {
  for (_i=0; _i<INBUFLEN; _i++) szListBuf[_i] = SPACE;
  } // cllin

//-----------------------------------------------------------------------------
//
//                      cllib
//
// Clr splitted instr field area to 0`s
//
void cllib()
  {
  for (_i=0; _i<SYMLEN+1; _i++)  flabl[_i] = 0;
  for (_i=0; _i<OPCDLEN+1; _i++) fopcd[_i] = 0;
  for (_i=0; _i<OPERLEN+1; _i++)
    {
    oper1[_i]  = 0;
    oper2[_i]  = 0;
    oper3[_i]  = 0;
    oper4[_i]  = 0;
    oper5[_i]  = 0;
    oper6[_i]  = 0;
    oper7[_i]  = 0;
    oper8[_i]  = 0;
    oper9[_i]  = 0;
    oper10[_i] = 0;
    oper11[_i] = 0;
    oper12[_i] = 0;
    oper13[_i] = 0;
    oper14[_i] = 0;
    oper15[_i] = 0;
    oper16[_i] = 0;
    }
  } //cllib

//------------------------------------------------------------------------------
//
//                      space_leading_0s
// Space_leading_0s
//
void space_leading_0s(char* _ptr)
  {
  while (*_ptr == '0') *_ptr++ = SPACE;
  } // space_leading_0s

//-----------------------------------------------------------------------------
//
//                        Bin2Hex
//
void Bin2Hex(char* _buf, UINT _val, int _cnt) 
  {
  static const char HexChars[] = "0123456789ABCDEF";

  int i;
  for (i=0; i<_cnt; i++)
    {
    if (AtmelFlag !=0 && (swmodel & _WORD) == _WORD && _hexFileEdit == FALSE)
      _buf[_cnt-1-i] = tolower(HexChars[_val & 0x0F]);
    else
      _buf[_cnt-1-i] = HexChars[_val & 0x0F];

    _val = _val >> 4;
    }
  } // Bin2Hex

//-----------------------------------------------------------------------------
//
//                      edipc
//
// Edit Program Counter to output line (LPBUF+2),
// clear instruction values, types and length
// .MODEL WORD or .MODEL BYTE 
//
void edipc()
  {
  GetPcValue();
  
  // Atmel:  pcValue=24/32bit - all segments
  if (AtmelFlag != 0)            
    {
    if ((swmodel & _WORD) == _WORD && SegType == _CODE)
      eddh(&szListBuf[lpLOC], pccw);          // Words
    else 
      eddh(&szListBuf[lpLOC], pcValue);       // Bytes
    edit_segtype();
    }

  // Intel, Motorola: pcc=16bit (SegType not edited in list line)
  else                           
    edwh(&szListBuf[lpLOC], pcValue);

  ilen    = 0;  // Instr length
  insv[1] = 0;  // Instr value
  insv[2] = 0;
  insv[3] = 0;
  insv[4] = 0;
  insv[5] = 0;
  } // edipc

//-----------------------------------------------------------------------------
//
//                      clepc
//
// Clear PC from list file
//
void clepc()
  {
  szListBuf[lpLOC+0] = SPACE;
  szListBuf[lpLOC+1] = SPACE;
  szListBuf[lpLOC+2] = SPACE;
  szListBuf[lpLOC+3] = SPACE;

  szListBuf[lpXSEG]   = SPACE;
  szListBuf[lpXSEG+1] = SPACE; 

  if (AtmelFlag != 0 && lpLOC == LOC32)      //ha// pcc=24/32bit
    {                                        //ha//
    szListBuf[lpLOC+4] = SPACE;              //ha//
    szListBuf[lpLOC+5] = SPACE;              //ha//
    szListBuf[lpLOC+6] = SPACE;              //ha//
    szListBuf[lpLOC+7] = SPACE;              //ha//
    }                                        //ha//
  } // clepc

//------------------------------------------------------------------------------
//
//                              edqh
// Edit Qword as 16 ascii chars
// chksum .. not needed for .HEX/.S19
//
void edqh(char* _buf, unsigned long long _qword)
  {
  Bin2Hex(_buf,     _qword >> 48, 4);
  Bin2Hex(&_buf[4], _qword >> 32, 4);
  Bin2Hex(&_buf[8], _qword >> 16, 4);
  Bin2Hex(&_buf[12],_qword >>  0, 4);
  } //edqh

//------------------------------------------------------------------------------
//
//                              eddh
// Edit Dword as 8 ascii chars
// chksum .. not needed for .HEX/.S19
//
void eddh(char*_buf, ULONG _dword)
  {
//ha//  chksum += (UCHAR)(_dword & 0x00FF);        
//ha//  chksum += (UCHAR)(_dword >>  8) & 0x00FF;
//ha//  chksum += (UCHAR)(_dword >> 16) & 0x00FF;
//ha//  chksum += (UCHAR)(_dword >> 24) & 0x00FF;

  Bin2Hex(_buf,     _dword >> 16, 4);
  Bin2Hex(&_buf[4], _dword >>  0, 4);
  } // eddh

//------------------------------------------------------------------------------
//
//                              edwh
// Edit word as 4 ascii chars
// chksum .. needed for .HEX/.S19 file (on-the-fly)
//
//void edwh(char*_buf, int _word)
void edwh(char*_buf, UINT _word)
  {
  chksum += (UCHAR)(_word & 0x00FF);        
  chksum += (UCHAR)((_word >> 8) & 0x00FF);

  Bin2Hex(_buf, _word, 4);
  } // edwh

//------------------------------------------------------------------------------
//
//                              edbh
// Edit byte as 2 ascii chars
// chksum .. needed for .HEX/.S19 file (on-the-fly)
//
//void edbh(char*_buf, int _byte)
void edbh(char*_buf, UINT _byte)
  {
  chksum += (UCHAR)_byte;
  Bin2Hex(_buf, _byte, 2);
  } // edbh

//------------------------------------------------------------------------------
//
//                                 flhex
//
// Get pointer to 1st free byte in &pszBinbuf[0]
// Transfer the object code values
// If &pszBinbuf[0] full or overflow then write to disk
//
void flhex()
  {
  if (SegType == _CODE)                             // CSEG contents
    {
    // Store Flash PROM data/code for hex file
    for (_i=1; _i<=ilen; _i++) *pszBinbuf++ = insv[_i];
    if (pszBinbuf > (pszBinbuf+HXBLEN)) wrhex(_CODE); 
    }
  else if (AtmelFlag != 0 && SegType == _EEPROM)    // ESEG contents 
    {
    // Store EEPROM data/code for hex file
    for (_i=1; _i<=ilen; _i++) *pszBinEEbuf++ = insv[_i];
    if (pszBinEEbuf > (pszBinEEbuf+EEHXBLEN)) wrhex(_EEPROM);
    }
  } // flhex

//------------------------------------------------------------------------------
//
//                            MotorolaSrec
//
// Write Motorola S-Record object format into srec file *.S19
//
// Type(Sn) ByteCount(=16) Address(word) RecordType(=00) Data(16 bytes) Checksum(byte)
//
// 'S0' |1-byte data count |16bit unused='0000' |Header info fields [20] [2] [2] [0..36] |chksum
// 'S1' |1-byte data count |16bit address field |data field                              |chksum
// 'S2' |1-byte data count |24bit address field |data field                              |chksum
// 'S3' |1-byte data count |32bit address field |data field                              |chksum
// 'S5' |1-byte data count |16bit counter. The sum of S1+S2+S3 records(no data field)    |chksum
// 'S7' |1-byte count='05' |32bit address field |execution start address (no data field) |chksum
// 'S8' |1-byte count='04' |24bit address field |execution start address (no data field) |chksum
// 'S9' |1-byte count='03' |16bit address field |execution start address (no data field) |chksum
//
// File example 1
//  S00600004844521B                             // = S0 06 0000 'HDR' 1B (chksum)
//  S1130000285F245F2212226A000424290008237C2A
//  S11300100002000800082629001853812341001813
//  S113002041E900084E42234300182342000824A952
//  S107003000144ED492
//  S5030004F8 (optional)                                                      
//  S9030000FC
//
// File example 2
//  S113087010740104015A4647345355885909090E26
//  S11308801073453001107393114168616E6E657089
//  S1130890856027003023011000702724261114BB23
//  S11308A0080207FA1FFFFFFF0000000000000027F6
//  S11308B00100592500150200000000000000009608
//  S11311E0FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF0B
//  S11311F0FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFB
//  S9030000FC
//
void MotorolaSrec(int _segType, ULONG pcSegType, int _hexPc, char* binbuf_ptr, char* hexfbuf_ptr)
  {
  pcSegType &= 0x0000FFFF;   // Force 16bit (Intel Hex record 00)

  UINT hexdataCount, hexquot, hexrem, _i, _k;
 
  // Calculate amount of data bytes to convert
  // 16 data bytes max in a .HEX line
  hexdataCount = 16;             
  hexquot = (pcSegType - _hexPc) / hexdataCount;  // Quotient: nr of Hex-Lines
  hexrem =  (pcSegType - _hexPc) % hexdataCount;  // Remainder: last Hex-Line
  int _hexCnt = hexquot * hexdataCount;           // Total bytes w/o remainder 

  if (_hexCnt > HXBLEN)   
    {
    printf(">>>FATAL ERROR - .HEX buffer overflow!\n");
    exit(SYSERR_ABORT);                  // ERROR -EXIT
    }
  
  _hexFileEdit = TRUE;                   // All ascii data in 'toupper'

  for (_i=0; _i<_hexCnt; _i+=hexdataCount)
    {
    if (detectedMaxBinsize < (hexdataCount+_hexPc)) detectedMaxBinsize = hexdataCount+_hexPc;

    chksum = 0;
    *hexfbuf_ptr++ = 'S';                // 'S1' Motorola S-Record file marker
    *hexfbuf_ptr++ = '1';
    edbh(hexfbuf_ptr, (hexdataCount+3)); // Edit nr of data bytes +2bytes addr +1byte count
    hexfbuf_ptr +=2;
    edwh(hexfbuf_ptr, _hexPc);           // Edit address record
    hexfbuf_ptr += 2*2;                  // _hexPc = 4 chars ascii-hex

    // Emit nr of data bytes in line and move global binbuf_ptr
    for (_k=0; _k<hexdataCount; _k++)
      {
      // Patch the binary Flash PROM Buffer                                 
      if (pszBinFilebuf != NULL && offsetBinFilebuf < RomSize)   //ha//
        pszBinFilebuf[_hexPc+_k+offsetBinFilebuf] = *binbuf_ptr; //ha//
      edbh(hexfbuf_ptr, *binbuf_ptr++);
      hexfbuf_ptr +=2;                   
      }

    // Emit 1s complement of checksum
    edbh(hexfbuf_ptr, (0xFF-chksum));  
    hexfbuf_ptr += 2;
    *hexfbuf_ptr = CR;
    *hexfbuf_ptr++ = LF;
    *hexfbuf_ptr = 0;

    _hexPc += hexdataCount;         // Advance to next address
    } // end for (_i)

  // Emit the remaining data
  if (hexrem > 0)               
    {
    if (detectedMaxBinsize < (hexrem+_hexPc)) detectedMaxBinsize = hexrem+_hexPc;

    chksum = 0;
    *hexfbuf_ptr++ = 'S';           // 'S1' Motorola S-Record file marker
    *hexfbuf_ptr++ = '1';
    edbh(hexfbuf_ptr, (hexrem+3));  // Edit nr of data bytes +2bytes addr +1byte count
    hexfbuf_ptr +=2;
    edwh(hexfbuf_ptr, _hexPc);      // Edit address record (big endian)
    hexfbuf_ptr += 2*2;             // _hexPc = 4 chars ascii-hex

    // Emit nr of data bytes in line and move global binbuf_ptr
    for (_k=0; _k<hexrem; _k++)
      {
      // Patch the binary Flash PROM Buffer                                 
      if (pszBinFilebuf != NULL && offsetBinFilebuf < RomSize)   //ha//
        pszBinFilebuf[_hexPc+_k+offsetBinFilebuf] = *binbuf_ptr; //ha//
      edbh(hexfbuf_ptr, *binbuf_ptr++);
      hexfbuf_ptr +=2;
      }

    // Emit 1s complement of checksum
    edbh(hexfbuf_ptr, (0xFF-chksum)); 
    hexfbuf_ptr += 2;
    *hexfbuf_ptr = CR;
    *hexfbuf_ptr++ = LF;
    *hexfbuf_ptr = 0;
    } // end if (hexrem)

  _hexFileEdit = FALSE;                 // All ascii data normal

  if (pszBinbuf == pszBinbuf0) return;  // Initially no data to process

  HexFile.write(pszHexFilebuf0, strlen(pszHexFilebuf0));
  errchk(szHexFileName, GetLastError());
  pszBinbuf  = pszBinbuf0;              // Reset &pszBinbuf[0] pointer
  pszHexFilebuf = pszHexFilebuf0;       // Reset &pszHexFilebuf[0] pointer

  if (!AtmelFlag) BinFilesize=detectedMaxBinsize;
  } // MotorolaSrec


//------------------------------------------------------------------------------
//
//                            IntelHex
//
// Write Intel hex object format into hex file *.HEX
//
// Colon(:) ByteCount(=16) Address(word) RecordType(=00) Data(16 bytes) Checksum(byte)
// File example 1
//  :020000020000FC
//  :1003400094F969A9B45969A9D4E769A9F4F269A92A
//  :10035000E830962EB82BB020859A7F00566B8A80A5
//  :1003600000466B1AF5BE1023CF64748A0409326309
//  :0B037000BE30234B3A2355F484041ADE
// End of file: Colon(:) ByteCount(=00) Address(0000) RecordType(=01) Checksum(0FFh)
//  :00000001FF
//
// ------------------------------------------------------------------
// Extended Segment Address Record (16- or 32-bit formats)
// | RECORD  |     LOAD     |         |         |         |         |
// |  MARK   |    RECLEN    | OFFSET  | RECTYP  |  USBA   | CHKSUM  |
// |  ':'    |    '02'      | '0000'  |  '02'   |         |         |
//   1-byte       1-byte      2-bytes   1-byte    2-bytes   1-byte
// ------------------------------------------------------------------
// 
// Colon(:) ByteCount(=2) Offset(=0000) RecordType(=02) UpperSegmentBaseAddress(word) Checksum(byte)
// File example 2
// :020000020000FC
// :1002000033333333333333333333333333333333BE
// :108000004444444444444444444444444444444430
// :10C0000066666666666666666666666666666666D0
// :02FFFE00EEEE25
// :020000021000EC
// :0E100000EEEEEEEEEEEEEEEEEEEEEEEEEEEEDE
// :102000009999999999999999999999999999999940
// :022010000000CE
// :10400000AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA10
// :02408000CCCCA6
// :10F60000BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB4A
// :10F610000E9416FB80E081E0C050D1400590199017
// :10F6200001100C9415FB0297C9F708953FB6F894A2
// :0EF6300024B620FCFDCF84BFE8953FBE0895B0
// End of file: Colon(:) ByteCount(=00) Address(0000) RecordType(=01) Checksum(0FFh)
// :00000001FF
//------------------------------------------------
//
void IntelHex(int _segType, ULONG pcSegType, int _hexPc, char* binbuf_ptr, char* hexfbuf_ptr)
  {
  pcSegType &= 0x0000FFFF;   // Force 16bit (Intel Hex record 00)
                                                                       
  UINT hexdataCount, hexquot, hexrem, _i, _k;
 
  // Calculate amount of data bytes to convert
  // 16 data bytes max in a .HEX line
  hexdataCount = 16;             
  if (pcSegType < _hexPc)
    {
    hexquot = (0x10000 - _hexPc) / hexdataCount;   // Quotient: nr of Hex-Lines
    hexrem =  (0x10000 - _hexPc) % hexdataCount;   // Remainder: last Hex-Line
    }
  else
    {   
    hexquot = (pcSegType - _hexPc) / hexdataCount; // Quotient: nr of Hex-Lines
    hexrem =  (pcSegType - _hexPc) % hexdataCount; // Remainder: last Hex-Line
    }
  int _hexCnt = hexquot * hexdataCount;            // Total bytes w/o remainder 

  if (_hexCnt > HXBLEN)   
    {
    printf(">>>FATAL ERROR - .HEX buffer overflow!\n");
    exit(SYSERR_ABORT);                 // ERROR -EXIT
    }
  
  _hexFileEdit = TRUE;                  // All ascii data in 'toupper'

  for (_i=0; _i<_hexCnt; _i+=hexdataCount)
    {
    if (detectedMaxBinsize < (hexdataCount+_hexPc)) detectedMaxBinsize = hexdataCount+_hexPc;

    chksum = 0;
    *hexfbuf_ptr++ = ':';               // ':' Intel-.HEX file marker
    edbh(hexfbuf_ptr, hexdataCount);    // Edit nr of data bytes in line
    hexfbuf_ptr +=2;
    edwh(hexfbuf_ptr, _hexPc);          // Edit address record (big endian)
    hexfbuf_ptr += 2*2;                 // _hexPc = 4 chars ascii-hex
    edbh(hexfbuf_ptr, 0);               // Edit record type '00'
    hexfbuf_ptr +=2;

    // Emit nr of data bytes in line and move global pszBinbuf
    for (_k=0; _k<hexdataCount; _k++)
      {
      // Patch the binary Flash PROM Buffer                                 
      if (pszBinFilebuf != NULL && offsetBinFilebuf < RomSize)   //ha//
        pszBinFilebuf[_hexPc+_k+offsetBinFilebuf] = *binbuf_ptr; //ha//
      edbh(hexfbuf_ptr, *binbuf_ptr++);
      hexfbuf_ptr +=2;                   
      }

    // Emit 2s complement of checksum
    edbh(hexfbuf_ptr, -chksum);
    hexfbuf_ptr += 2;
    *hexfbuf_ptr = CR;
    *hexfbuf_ptr++ = LF;
    *hexfbuf_ptr = 0;

    _hexPc += hexdataCount;         // Advance to next address
    } // end for (_i)

  // Emit the remaining data
  if (hexrem > 0)               
    {
    if (detectedMaxBinsize < (hexrem+_hexPc)) detectedMaxBinsize = hexrem+_hexPc;

    chksum = 0;
    *hexfbuf_ptr++ = ':';           // ':' Intel-.HEX file marker
    edbh(hexfbuf_ptr, hexrem);      // Edit nr of data bytes in line
    hexfbuf_ptr +=2;
    edwh(hexfbuf_ptr, _hexPc);      // Edit address record   (big endian)
    hexfbuf_ptr += 2*2;             // _hexPc = 4 chars ascii-hex
    edbh(hexfbuf_ptr, 0);           // Edit record type '00'
    hexfbuf_ptr +=2;

    // Emit nr of data bytes in line and move global pszBinbuf
    for (_k=0; _k<hexrem; _k++)
      {
      // Patch the binary Flash PROM Buffer                                 
      if (pszBinFilebuf != NULL && offsetBinFilebuf < RomSize)   //ha//
        pszBinFilebuf[_hexPc+_k+offsetBinFilebuf] = *binbuf_ptr; //ha//
      edbh(hexfbuf_ptr, *binbuf_ptr++);
      hexfbuf_ptr +=2;
      }

    // Emit 2s complement of checksum
    edbh(hexfbuf_ptr, -chksum);     
    hexfbuf_ptr += 2;
    *hexfbuf_ptr = CR;
    *hexfbuf_ptr++ = LF;
    *hexfbuf_ptr = 0;
    } // end if (hexrem)

  _hexFileEdit = FALSE;                  // All ascii data normal

  if (_segType == _EEPROM)  
    {
    if (pszBinEEbuf == pszBinEEbuf0) return; // Initially no data to process

    EEPHexFile.write(pszHexFileEEbuf0, strlen(pszHexFileEEbuf0));
    errchk(szEEPHexFileName, GetLastError());
    pszBinEEbuf  = pszBinEEbuf0;         // Reset pszBinEEbuf pointer
    pszHexFileEEbuf = pszHexFileEEbuf0;  // Reset pszHexFileEEbuf pointer
    } // end if
  else if (_segType == _CODE)
    {
    if (pszBinbuf == pszBinbuf0) return; // Initially no data to process

    HexFile.write(pszHexFilebuf0, strlen(pszHexFilebuf0));
    errchk(szHexFileName, GetLastError());
    pszBinbuf  = pszBinbuf0;             // Reset pszBinbuf pointer
    pszHexFilebuf = pszHexFilebuf0;      // Reset pszHexFilebuf pointer
    } // end else if

  if (!AtmelFlag) BinFilesize=detectedMaxBinsize;
  } // IntelHex

//------------------------------------------------------------------------------
//
//                            wrhex
//
// Write hex object format into hex file
//
// ":020000020000FC" = Intel Record02 string for .HEX file
//
void wrhex(int _segtype)
  {
  char pcSegTypeASCII[4+1] = {0};
  ULONG _li;
  
  //------------------------------------------------------------------------//
  // Allocate buffer (512M max) for .BIN-file and init with binPadByte      //
  if (((binPadByte & 0xFF) == 0xFF || binPadByte == 0x00) &&                //
      pszBinFilebuf == NULL                               &&                //
      RomSize <= HXBLEN)       // max 512K (default may be greater)         //
    {                                                                       //
    if (pszBinFilebuf == NULL) // skip if already allocated                 //
      {                                                                     //
      pszBinFilebuf = (char*)GlobalAlloc(GPTR, RomSize+1);                  //
      errchk(szBinFileName, GetLastError());                                //
      for (_li=0; _li<RomSize; _li++) pszBinFilebuf[_li] = binPadByte;      //
      if (BinFilesize == 0) BinFilesize = RomSize;                          //
      }                                                                     //
    } // end if                                                             //
  else if (((binPadByte & 0xFF) == 0xFF || binPadByte == 0x00) &&           //
           pszBinFilebuf == NULL                               &&           //
           RomSize > HXBLEN)       // max 512K (default may be greater)     //
    {                                                                       //
    printf("MEMORY PROG_FLASH > 512K: '%s' not generated\n", szBinFileName);//
    }                                                                       //
  //------------------------------------------------------------------------//

  //----------
  // Intel Hex
  //----------
  if (HexFileFormat() == INTEL)
    {
    // All ascii edited data in 'toupper'
    _hexFileEdit = TRUE;

    // Check if any Flash PROM data to be stored: 
    if (_segtype == _CODE && pszBinbuf > pszBinbuf0) 
      {
      // Atmel: Default hxrec02 (:020000020000FC) not yet emitted?          
      if (AtmelFlag && !swhxrec02)                    
        {                                                 
        swhxrec02 = TRUE;                                     
        offsetBinFilebuf = 0;                                   
        HexFile.write(hxrec02, strlen(hxrec02)); 
        } // end if

      // if pcc>64K: hxrec02 (:02000002aaaacc) must be emitted.
      if (offsetBinFilebuf != (pcc & 0x000F0000))
        {
        UINT _pcc  = (pcc & 0x0000FFFF);                     // pcc 16bit offset 
        UINT _hxpc = 0;                                      // Hex Record00 Address

        if (_pcc < hxpc)
          IntelHex(_CODE, pcc, hxpc, pszBinbuf0, pszHexFilebuf0); // .CSEG rest data < 0x10000

        // Build Intel Hex Record02       
        _hexFileEdit = TRUE;                                 // All ascii edited data in 'toupper'
        offsetBinFilebuf = pcc & 0x000F0000;                                                  
        edwh(&hxrec02[9], (offsetBinFilebuf >> 4));          // Record02 extended address offset
        edbh(&hxrec02[13], -(2+2+(offsetBinFilebuf >> 12))); // Record02 chksum update
        HexFile.write(hxrec02, strlen(hxrec02));             // emit

        pszBinbuf += (_pcc-_hxpc);                           // remaining rest  data  
        if (_pcc < hxpc)
          IntelHex(_CODE, _pcc, _hxpc, pszBinbuf0, pszHexFilebuf0);   // .CSEG rest data < (pcc&0xF0000)
        else
          IntelHex(_CODE, pcc, hxpc, pszBinbuf0, pszHexFilebuf0);     // .CSEG data >= (pcc&0xF0000)
        } // end if (offsetBinFilebuf != pcc)

      else IntelHex(_CODE, pcc, hxpc, pszBinbuf0, pszHexFilebuf0);    // .CSEG
      } // end if (pszBinbuf > &pszBinbuf[0])

    // Check if any Atmel AVR EEPROM data to be stored
    else if (AtmelFlag != 0 && pszBinEEbuf > pszBinEEbuf0)
      IntelHex(_EEPROM, pce, eehxpc, pszBinEEbuf0, pszHexFileEEbuf0); // .ESEG
    } // end if (HexFileFormat)

  //--------------
  // Motorola Srec
  //--------------
  else MotorolaSrec(_CODE, pcc, hxpc, pszBinbuf0, pszHexFilebuf0);    // .CSEG
  
  // All ascii edited data in 'tolower'
  _hexFileEdit = FALSE;                                      
  } // end wrhex

//------------------------------------------------------------------------------
//
//                              edpri_dq
//
// Edit instr values for lst & hex
//
void edpri_dq()
  {
  if (ilen > 0)
    {
    GetPcValue();
    pcValue += ilen;
    AssignPc();            // Set PC according segment

    edbh(&szListBuf[lpOBJ+ 0], qvalue >> 56); // 1st byte displayed is msb
    edbh(&szListBuf[lpOBJ+ 2], qvalue >> 48); // 1st byte displayed 
    edbh(&szListBuf[lpOBJ+ 4], qvalue >> 40); // 1st byte displayed 
    edbh(&szListBuf[lpOBJ+ 6], qvalue >> 32); // 1st byte displayed 
    edbh(&szListBuf[lpOBJ+ 8], qvalue >> 24); // 1st byte displayed 
    edbh(&szListBuf[lpOBJ+10], qvalue >> 16); // 2nd byte displayed
    edbh(&szListBuf[lpOBJ+12], qvalue >>  8); // 3rd byte displayed
    edbh(&szListBuf[lpOBJ+14], qvalue >>  0); // 4th byte displayed is lsb

    // Store code and data for hex file (according to little / big endian)
    flhex();  
    } // end if

  if (strstr(szListBuf, "\x0D\x0A") != 0) linut(szListBuf);  //ha//
  warnout();
  erout();
  } // edpri_dq

//------------------------------------------------------------------------------
//
//                              edpri_dd
//
// Edit instr values for lst & hex
//
void edpri_dd()
  {
  if (ilen > 0)
    {
    GetPcValue();
    pcValue += ilen;
    AssignPc();            // Set PC according segment

    edbh(&szListBuf[lpOBJ+0], (value >> 24) & 0xFF); // 1st byte displayed is msb
    edbh(&szListBuf[lpOBJ+2], (value >> 16) & 0xFF); // 2nd byte displayed
    edbh(&szListBuf[lpOBJ+4], (value >> 8) & 0xFF);  // 3rd byte displayed
    edbh(&szListBuf[lpOBJ+6], value & 0xFF);         // 4th byte displayed is lsb

    // Store code and data for hex file (according to little / big endian)
    flhex();  
    } // end if

  if (strstr(szListBuf, "\x0D\x0A") != 0) linut(szListBuf);  //ha//
  warnout();
  erout();
  } // edpri_dd

//------------------------------------------------------------------------------
//
//                              edpri_dw
//
// Edit instr values for lst & hex
//
void edpri_dw()
  {
  if (ilen > 0)
    {
    GetPcValue();
    pcValue += ilen;
    AssignPc();           // Set PC according to current segment

    edbh(&szListBuf[lpOBJ+0], (value >> 8) & 0xFF); // 1st byte displayed is msb
    edbh(&szListBuf[lpOBJ+2], value & 0xFF);        // 2nd byte displayed is lsb

    // Store code and data for hex file (according to little / big endian)
    flhex();  
    } // end if

  if (strstr(szListBuf, "\x0D\x0A") != 0) linut(szListBuf);  //ha//
  warnout();
  erout();
  } // edpri_dw

//------------------------------------------------------------------------------
//
//                              edpri_db
//
// Edit instr values for lst & hex
//
void edpri_db()
  {
  if (ilen > 0)
    {
    GetPcValue();
    pcValue += ilen;
    AssignPc();          // Set PC according segment

if (AtmelFlag != 0 && swmodel == _WORD && SegType == _CODE)
  {                                      
    switch (ilen)
      {
      case 1:
        edbh(&szListBuf[lpOBJ+0], insv[1]); // Ilen=1: 1st byte is opcode
        break;
      case 2:
        edbh(&szListBuf[lpOBJ+0], insv[2]); // Ilen=1: 1st byte is opcode
        edbh(&szListBuf[lpOBJ+2], insv[1]); // Ilen=1: 1st byte is opcode
        break;
      case 3:
        edbh(&szListBuf[lpOBJ+0], insv[2]);    
        edbh(&szListBuf[lpOBJ+2], insv[1]);    
        edbh(&szListBuf[lpOBJ+5], insv[3]); 
        break;
      case 4:
        edbh(&szListBuf[lpOBJ+0], insv[2]); 
        edbh(&szListBuf[lpOBJ+2], insv[1]); 
        edbh(&szListBuf[lpOBJ+5], insv[4]); 
        edbh(&szListBuf[lpOBJ+7], insv[3]); 
        break;
      case 5:                                // Standard and Atmel AVR (.MODEL BYTE)
        edbh(&szListBuf[lpOBJ+0],  insv[2]); // DB list as 2.5 words: 2301 6745 AB                  
        edbh(&szListBuf[lpOBJ+2],  insv[1]);  
        edbh(&szListBuf[lpOBJ+5],  insv[4]);  
        edbh(&szListBuf[lpOBJ+7],  insv[3]);  
        edbh(&szListBuf[lpOBJ+10], insv[5]);
        break;
      case 6:                                // Atmel AVR (.MODEL WORD)
        edbh(&szListBuf[lpOBJ+0],  insv[2]); // DB list as 3 words: 2301 6745 AB89                  
        edbh(&szListBuf[lpOBJ+2],  insv[1]);  
        edbh(&szListBuf[lpOBJ+5],  insv[4]);  
        edbh(&szListBuf[lpOBJ+7],  insv[3]);  
        edbh(&szListBuf[lpOBJ+10], insv[6]);
        edbh(&szListBuf[lpOBJ+12], insv[5]);
        break;
      default:
        break;
      } // end switch
  } // end if (AtmelFlag)
else
  {
    switch (ilen)
      {
      case 1:
        edbh(&szListBuf[lpOBJ+0], insv[1]); // Ilen=1: 1st byte is opcode (or DB)
        break;
      case 2:
        edbh(&szListBuf[lpOBJ+0], insv[1]); // 1st byte is opcode   (or DB)
        edbh(&szListBuf[lpOBJ+3], insv[2]); // 2nd byte is operand  (or DB)
        break;
      case 3:
        edbh(&szListBuf[lpOBJ+0], insv[1]); 
        edbh(&szListBuf[lpOBJ+3], insv[2]); 
        edbh(&szListBuf[lpOBJ+6], insv[3]); 
        break;
      case 4:
        edbh(&szListBuf[lpOBJ+0], insv[1]); 
        edbh(&szListBuf[lpOBJ+3], insv[2]); 
        edbh(&szListBuf[lpOBJ+6], insv[3]); 
        edbh(&szListBuf[lpOBJ+9], insv[4]); 
        break;
      case 5:                                // Standard and Atmel AVR (.MODEL BYTE)
        edbh(&szListBuf[lpOBJ+0],  insv[1]); // DB list as 5 bytes: 01 23 45 67 89 
        edbh(&szListBuf[lpOBJ+3],  insv[2]);  
        edbh(&szListBuf[lpOBJ+6],  insv[3]);  
        edbh(&szListBuf[lpOBJ+9],  insv[4]);  
        edbh(&szListBuf[lpOBJ+12], insv[5]);
        break;                                                    
      default:
        break;
      } // end switch
  } // end else

    // Store code and data for hex file
    flhex();  
    } // end if

  if (strstr(szListBuf, "\x0D\x0A") != 0) linut(szListBuf);  //ha//
  warnout();
  erout();
  } // edpri_db

//------------------------------------------------------------------------------
//
//                              edpri
//
// Edit instr values for lst & hex
//
void edpri()
  {
  if (AtmelFlag == 0)
    edpri_db(); // Default is .MODEL BYTE

  else if (AtmelFlag != 0 && (swmodel & _BYTE) == _BYTE)
    {
    if (ilen > 0)
      {
      GetPcValue();
      pcValue += ilen;
      AssignPc();          // Set PC according segment

      switch (ilen)
        {
        case 1:
          edbh(&szListBuf[lpOBJ+0], insv[1]); // Ilen=1: 1st byte is opcode
          break;
        case 2:
          edbh(&szListBuf[lpOBJ+0], insv[1]); // 1st byte is opcode
          edbh(&szListBuf[lpOBJ+3], insv[2]); // 2nd byte is operand
          break;
        case 3:
          edbh(&szListBuf[lpOBJ+0], insv[1]); 
          edbh(&szListBuf[lpOBJ+3], insv[2]); 
          edbh(&szListBuf[lpOBJ+6], insv[3]); 
          break;
        case 4:
          edbh(&szListBuf[lpOBJ+0], insv[1]); 
          edbh(&szListBuf[lpOBJ+3], insv[2]); 
          edbh(&szListBuf[lpOBJ+6], insv[3]); 
          edbh(&szListBuf[lpOBJ+9], insv[4]); 
          break;
        case 5:
          edbh(&szListBuf[lpOBJ+0],  insv[1]);  
          edbh(&szListBuf[lpOBJ+3],  insv[2]);  
          edbh(&szListBuf[lpOBJ+6],  insv[3]);  
          edbh(&szListBuf[lpOBJ+9],  insv[4]);  
          edbh(&szListBuf[lpOBJ+12], insv[5]);
          break;
        default:
          break;
        } // end switch

      // Store code and data for hex file
      flhex();  
      } // end if (ilen)
    } // end if (AtmelFlag

  else if (AtmelFlag != 0 && (swmodel & _WORD) == _WORD)
    {
    if (ilen > 0)
      {
      GetPcValue();
      pcValue += ilen;
      AssignPc();          // Set PC according segment
                                        
      switch (ilen)
        {
        case 2:
          edbh(&szListBuf[lpOBJ+0], insv[2]); // 1st byte is opcode
          edbh(&szListBuf[lpOBJ+2], insv[1]); // 2nd byte is operand
          break;
        case 4:
          edbh(&szListBuf[lpOBJ+0], insv[2]); 
          edbh(&szListBuf[lpOBJ+2], insv[1]); 
          edbh(&szListBuf[lpOBJ+5], insv[4]); 
          edbh(&szListBuf[lpOBJ+7], insv[3]); 
          break;
        default:
          break;
        } // end switch

      // Store code and data for hex file
      flhex();  
      } // end if (ilen)
    } // end else if (AtmelFlag

  if (strstr(szListBuf, "\x0D\x0A") != 0) linut(szListBuf);  //ha//
  erout();
  warnout();
  } // edpri

//-----------------------------------------------------------------------------
//
//                      edit_inc_linenr
//
void edit_inc_linenr()
  {
  // Check if to suppress macro expansion in .LST file
  if (swlistmac == FALSE && swexpmacro != FALSE) return; 

  if (swlst == 1)
    { 
    _lstline++;                                   // listing file line counter
    sprintf(&szListBuf[lpLINE], "%5d", _lstline); // Emit converted lstline count
    szListBuf[lpLINE+5] = ' ';
    }
  if (swexpmacro != FALSE) szListBuf[lpMACR] = MACRO_IDM; // Emit MACRO line marker
  } // edit_inc_linenr

//-----------------------------------------------------------------------------
//
//                      edit_segtype
//
void edit_segtype()
  {
  szListBuf[lpXSEG]   = ':';                     // Emit colon
  szListBuf[lpXSEG+1] = SegType + 0x42;          // Emit segment letter
  } // edit_segtype

//------------------------------------------------------------------------------

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("fopcd=%s  SegType=%04X  pcc=%08X  pcd=%08X  pce=%08X\n",
//ha//        fopcd,    SegType,      pcc,      pcd,      pce);
//ha//DebugStop(11, "wrhex()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("\n&pszHexFilebuf[0] ");
//ha//DebugPrintBuffer(&pszHexFilebuf[0], 100);
//ha//printf("pcc=%08X  _pcc=%08X  hxpc=%08X  _hxpc=%08X  hxrec02=%s", pcc, _pcc, hxpc, _hxpc, hxrec02);  
//ha//DebugStop(2, "wrhex()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("hexfbuf ");
//ha//DebugPrintBuffer(hexfbuf, 100);
//ha//printf("\nhexfbuf_ptr ");
//ha//DebugPrintBuffer(hexfbuf_ptr, 100);
//ha//DebugStop(1, "IntelHex()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("pcc=%04X  hxpc=%04X  _hexCnt=%d  hexrem=%d  binbuf_ptr=%08X  &pszBinbuf[0]=%08X\nfopcd=%s  oper1=%s  oper2=%s  oper3=%s\n",
//ha//        pcc, hxpc, _hexCnt, hexrem, binbuf_ptr, &pszBinbuf[0], fopcd, oper1, oper2, oper3);
//ha//printf("&pszBinbuf[0] ");
//ha//DebugPrintBuffer(&pszBinbuf[0], 300);
//ha//DebugStop(1, "edpri()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//printf("fopcd=%s  pcd=%08X  SRamStart=%04X\n", fopcd, pcd, SRamStart);
//ha//DebugStop(1, "CheckPc()", __FILE__);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//--------------------------end-of-c++-module-----------------------------------



