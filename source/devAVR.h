// Topic: "Missing instructions in various AVR cores"
// haXASM - Cross-Assembler for 8bit Microprocessors
// devAVR.h - C++ Developer source file.
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

// ------------------------------------------------------------------------ //
// Example: 8515def.inc (Atmel) for device "AT90S8515"                      //
// ---------------------------------------------------                      //
// ;* Number            : AVR000                                            //
// ;* File Name         : "8515def.inc"                                     //
// ;* Title             : Register/Bit Definitions for the AT90S8515        //
// ;* Date              : 2011-02-09                                        //
// ;* Version           : 2.35                                              //
// ;* Support E-mail    : avr@atmel.com                                     //
// ;* Target MCU        : AT90S8515                                         //
// #pragma partinc 0                                                        //
// .DEVICE AT90S8515                                                        //
// #pragma AVRPART ADMIN PART_NAME AT90S8515                                //
//   #define SIGNATURE_000  = 0x1e                                          //
//   #define SIGNATURE_001  = 0x93                                          //
//   #define SIGNATURE_002  = 0x01                                          //
//                                                                          //
// #pragma AVRPART CORE CORE_VERSION V1                                     //
//                                                                          //
// #pragma AVRPART MEMORY PROG_FLASH 8192                                   //
// #pragma AVRPART MEMORY EEPROM 512                                        //
// #pragma AVRPART MEMORY INT_SRAM SIZE 512                                 //
// #pragma AVRPART MEMORY INT_SRAM START_ADDR 0x60                          //
// #pragma AVRPART CORE INSTRUCTIONS_NOT_SUPPORTED break                    //
//                                                                          //
// NOTE:                                                                    //
// It is expected (though not guaranteed) that in *def.inc files,           //
// provided by Atmel the "#pragma AVRPART CORE INSTRUCTIONS_NOT_SUPPORTED"  //
// lists all missing instructions of a certain AVR device correctly.        //
// The .DEVICE directive, however, can only just rely on data published     //
// by Atmel/Microchip. This data is ambiguous, so all possible missing      //
// instructions, if found in the assembler source file, are detected here.  //
// The developer must verify if there's any support of the chosen           //
// instruction(s) with the selected uC device.                              //
// (by the way it's anyway recommended to study the uC DataSheet before     //
//  beginning a microchip project and writing the software, isn't it?)      //
// ------------------------------------------------------------------------ //

//------------------------------------------------------------------------------
//
//  Atmel/Microchip AVR(R) Instruction Set
//
// Instruction Set Summary
//  Several updates of the AVR CPU during its lifetime has resulted in different
//  flavors of the instruction set, especially for the timing of the instructions. 
//  Machine code level of compatibility is intact for all CPU versions with very few
//  exceptions related to the Reduced Core (AVRrc), though not all instructions are 
//  included in the instruction set for all devices. The table below contains 
//  the major versions of the AVR 8-bit CPUs. In addition to the different versions,
//  there are differences depending on the size of the device memory map. 
//  Typically these differences are handled by a C/EC++ compiler, but users that are 
//  porting code should be aware that the code execution can vary slightly in the
//  number of clock cycles.
//  
//  Versions of AVR® 8-bit CPU
//  Name   Description
//  AVR    Original instruction set from 1995
//  
//  AVRe   AVR instruction set extended with the Move Word (MOVW) instruction, 
//         and the Load Program Memory (LPM) instruction has been enhanced. 
//         Same timing as AVR.
//  
//  AVRe+  AVRe instruction set extended with the Multiply (xMULxx) instruction. 
//         Same timing as AVR and AVRe.
//  
//  AVRxm  AVRe+ instruction set extended with the Read Modify Write (RMW) and 
//         Data Encryption Standard (DES) instructions. 
//         SPM extended to include SPM Z+2. Significantly different timing 
//         compared to AVR, AVRe, AVRe+.
//  
//  AVRxt  A combination of AVRe+ and AVRxm. Available instructions are the 
//         same as AVRe+, but the timing has been improved compared 
//         to AVR, AVRe, AVRe+ and AVRxm.
//  
//  AVRrc  AVRrc has only 16 registers in its register file (R31-R16), 
//         and the instruction set is reduced. The timing is significantly 
//         different compared to the AVR, AVRe, AVRe+, AVRxm and AVRxt. 
//         Refer to the instruction set summary for further details.
//

// AVR® Instruction Set Manual
// Appendix A Device Core Overview
// © 2021 Microchip Technology Inc. Manual DS40002198B-page 149++
//
// The table lists all instructions that vary between the different cores
// and marks if it is included in the core. If the instruction is not a
// part of the table, then it is included in all cores.
//
//  Instructions   AVR   AVRe  AVRe+ AVRxm AVRxt AVRrc   // Group
//  --------------------------------------------------   // -----
//  ADIW         | x   | x   | x   | x   | x   |      |  //   12
//  BREAK        |     | x   | x   | x   | x   | x    |  //    2
//  CALL         |     | x   | x   | x   | x   |      |  //   22
//  DES          |     |     |     | x   |     |      |  //   21
//  EICALL       |     |     | x   | x   | x   |      |  //    2
//  EIJMP        |     |     | x   | x   | x   |      |  //    2
//  ELPM         |     |     | x   | x   | x   |      |  //   32
//  FMUL         |     |     | x   | x   | x   |      |  //    8
//  FMULS        |     |     | x   | x   | x   |      |  //    8
//  FMULSU       |     |     | x   | x   | x   |      |  //    8
//  JMP          |     | x   | x   | x   | x   |      |  //   22
//  LAC          |     |     |     | x   |     |      |  //   10
//  LAS          |     |     |     | x   |     |      |  //   10
//  LAT          |     |     |     | x   |     |      |  //   10
//  LDD          | x   | x   | x   | x   | x   |      |  //   29
//  LPM          | x   | x   | x   | x   | x   |      |  //   32
//  LPM Rd, Z    |     | x   | x   | x   | x   |      |  //   32
//  LPM Rd, Z+   |     | x   | x   | x   | x   |      |  //   32
//  MOVW         |     | x   | x   | x   | x   |      |  //   24
//  MUL          |     |     | x   | x   | x   |      |  //    4
//  MULS         |     |     | x   | x   | x   |      |  //   20
//  MULSU        |     |     | x   | x   | x   |      |  //    8
//  SBIW         | x   | x   | x   | x   | x   |      |  //   12
//  SPM          |     | x   | x   | x   | x   |      |  //    2
//  SPM Z+       |     |     |     | x   | x   |      |  //    2
//  STD          | x   | x   | x   | x   | x   |      |  //   31
//  XCH          |     |     |     | x   |     |      |  //   10
//  --------------------------------------------------   // -----
//  PUSH/POP ???                                         //   11

//-----------------------------------------------------------------------------
//
//                  AVRInstructionCheck / FindMissingInstr
//
// Implmentation in deviceAVR.cpp
//
// AVR® Instruction Set Manual
// Appendix A Device Core Overview
// © 2021 Microchip Technology Inc. Manual DS40002198B-page 149++
//
// Instructions   AVR AVRe AVRe+ AVRxm AVRxt AVRrc      ins_group        .DEVICE name (it's just a Chaos! No chance to keep track or recognize any structure)
// -----------------------------------------------      ---------        ------------
// BREAK          -   x    x     x     x     x     |    02 'R' [1]       ATmega8*[e+][xt], ATtiny[ ][rc]
// EICALL         -   -    x     x     x     -     |    02 'A' [3]       ATmega  [e+][xt], ATxmega[xm], ATtiny[ ][e][xt][rc], AVR*[xt], ATA*[e][e+], AT90*[e+]
// EIJMP          -   -    x     x     x     -     |    02 'M' [3]       ATmega  [e+][xt], ATxmega[xm], ATtiny[ ][e][xt][rc], AVR*[xt], ATA*[e][e+]
// SPM            -   x    x     x     x     -     |    02 'P' [1]       ATmega  [e+][xt], ATtiny[ ][e][xt][rc]
// SPM Z+         -   -    -     x     x     -     |    02 'P' [1] =SPM  ATmega  [e+][xt], ATtiny[ ][e][xt][rc]
//                                                 |
// MUL            -   -    x     x     x     -     |    04 'L' [2]       =ATmega[e+][xt], =ATxmega[xm], ATtiny[ ][e][xt][rc], =AVR*[xt], ATA*[e][e+], =AT90*[e+]
//                                                 |
// FMUL           -   -    x     x     x     -     |    08 'F' [0]       =ATmega[e+][xt], =ATxmega[xm], ATtiny[ ][e][xt][rc], =AVR*[xt], ATA*[e][e+], =AT90*[e+]
// FMULS          -   -    x     x     x     -     |    08 'S' [4]       =ATmega[e+][xt], =ATxmega[xm], ATtiny[ ][e][xt][rc], =AVR*[xt], ATA*[e][e+], =AT90*[e+]
// FMULSU         -   -    x     x     x     -     |    08 'U' [5]       =ATmega[e+][xt], =ATxmega[xm], ATtiny[ ][e][xt][rc], =AVR*[xt], ATA*[e][e+], =AT90*[e+]
// MULSU          -   -    x     x     x     -     |    08 'M' [0]       =ATmega[e+][xt], =ATxmega[xm], ATtiny[ ][e][xt][rc], =AVR*[xt], ATA*[e][e+], =AT90*[e+]
//                                                 |
// LAC            -   -    -     x     -     -     |    10 'C' [2]       =AVR*, ATxmega[xm]
// LAS            -   -    -     x     -     -     |    10 'S' [2]       =AVR*, ATxmega[xm]
// LAT            -   -    -     x     -     -     |    10 'T' [2]       =AVR*, ATxmega[xm]
// XCH            -   -    -     x     -     -     |    10 'H' [2]       =AVR*, ATxmega[xm]
//                                                 |
// ADIW           x   x    x     x     x     -     |    12 'A' [0]       ATtiny1*[ ][e][xt][rc]
// SBIW           x   x    x     x     x     -     |    12 'S' [0]       ATtiny1*[ ][e][xt][rc]
//                                                 |
// MULS           -   -    x     x     x     -     |    20               =ATmega[e+][xt], =ATxmega[xm], ATtiny[ ][e][xt][rc], =AVR*[xt], ATA*[e][e+], =AT90*[e+]
//                                                 |
// DES            -   -    -     x     -     -     |    21               =AVR*[xt]
//                                                 |
// CALL           -   x    x     x     x     -     |    22 'C' [0]       ATmega[e+][xt], =ATxmega[xm], ATtiny[ ][e][xt][rc], ATA*[e][e+], AT90*[e+]
// JMP            -   x    x     x     x     -     |    22 'J' [0]       ATmega[e+][xt], =ATxmega[xm], ATtiny[ ][e][xt][rc], ATA*[e][e+], AT90*[e+]
//                                                 |
// MOVW           -   x    x     x     x     -     |    24               ATtiny  [ ][e][xt][rc]
//                                                 |
// LDD            x   x    x     x     x     -     |    29               ATtiny1*[ ][e][xt][rc]
//                                                 |
// STD            x   x    x     x     x     -     |    31               ATtiny1*[ ][e][xt][rc]
//                                                 |
// ELPM           -   -    x     x     x     -     |    32 'E' [0]       ATmega     [e+][xt], ATxmega[xm], ATtiny[ ][e][xt][rc], AVR*[xt], ATA*[e][e+], AT90*[e+]
// LPM            x   x    x     x     x     -     |    32 'L' [0]       ATtiny1*[ ][e][xt][rc]
// LPM Rd,Z       -   x    x     x     x     -     |    32 'L' [0] =LPM  ATtiny1*[ ][e][xt][rc]
// LPM Rd,Z+      -   x    x     x     x     -     |    32 'L' [0] =LPM  ATtiny1*[ ][e][xt][rc]
// -----------------------------------------------      ---------
//
// ATtiny1X only: Additional instructions should be checked
// ICALL                                           |    02 'C' [1]       ATtiny
// IJMP                                            |    02 'J' [1]       ATtiny
// LD                                              |    28               ATtiny1*
// POP                                             |    11 'O' [1]       ??
// PUSH                                            |    11 'U' [1]       ??
// -----------------------------------------------      ---------

// Reduced instruction set (push/pop is missing in only very few uChips)
// ?? PUSH/POP: The Note below was found in Atmel/Microchip documentation: ??
//   "This instruction is not available on all devices. Refer to Appendix A."
// Atmel's summary documentation is not clesr about push/pop.
#define _ALL_       0xFFFF // Reduced instruction set
#define _NONE_      0x0000 // Full instruction set support
#define _MUL_       0x0001 // missing: FMUL:FMULS:FMULSU:MUL:MULS:MULSU 
#define _DES_       0x0002 // missing: DES
#define _LD_        0x0004 // missing: LD:LD X:LD Y:LD -Z:LD Z+
#define _LDDSTD_    0x0008 // missing: LDD:STD 
#define _JMPCALL_   0x0010 // missing: CALL:JMP 
#define _EIJMPCALL_ 0x0020 // missing: EICALL:EIJMP 
#define _IJMPCALL_  0x0040 // missing: IJMP:ICALL 
#define _MOVW_      0x0080 // missing: MOVW                   
#define _XCHLAx_    0x0100 // missing: LAC:LAS:LAT:XCH 
#define _LPM_       0x0200 // missing: LPM:LPM Rd,Z:LPM Rd,Z+  
#define _ELPM_      0x0400 // missing: ELPM
#define _SPM_       0x0800 // missing: SPM:SPM Z+ 
#define _BREAK_     0x1000 // missing: BREAK
#define _PUSHPOP_   0x2000 // missing: PUSH:POP
#define _ADIWSBIW_  0x4000 // missing: ADIW:SBIW                    
  

