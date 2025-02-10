// haXASM - Cross-Assembler for INtel 8bit processors
// deviceAVR.cpp - C++ Developer source file.
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

#include <io.h>        // File open, close, access, etc.
#include <conio.h>     // For _putch(), _getch() ..
#include <string>      // printf, etc.

#include <shlwapi.h>   // StrStrI, StrCmpI, StrCmpNI

#include <sys/stat.h>  // For filesize
#include <iostream>    // I/O control
#include <fstream>     // File control

#include <windows.h>   // For console specific functions

#include "devAVR.h"    // Supported/missing instructions in various uC cores
#include "equate.h"
#include "extern.h"    // Variables published in workst.cpp

using namespace std;

#pragma pack(1)        // Needed for structure 'sizeof(..)'

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
// AVR® Instruction Set Manual
// Appendix A Device Core Overview
// © 2021 Microchip Technology Inc. Manual DS40002198B-page 149++
//
// Instruction Set Summary
//  Several updates of the AVR CPU during its lifetime has resulted in different
//  flavors of the instruction set, especially for the timing of the instructions. 
//  Machine code level of compatibility is intact for all CPU versions with very
//  few exceptions related to the Reduced Core (AVRrc), though not all instructions 
//  are included in the instruction set for all devices. The table below contains 
//  the major versions of the AVR 8-bit CPUs. In addition to the different versions, 
//  there are differences depending on the size of the device memory map. 
//  Typically these differences are handled by a C/EC++ compiler, but users that 
//  are porting code should be aware that the code execution can vary slightly
//  in the number of clock cycles.
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

// https://en.wikipedia.org/wiki/Atmel_AVR_instruction_set
// 
// Less capable than the "classic" CPU cores are two subsets:
// The "AVR1" core and the "AVRtiny". 
//  Confusingly, "ATtiny" branded processors have a variety of cores:
//   AVR1     (ATtiny11, ATtiny28), 
//   classic  (ATtiny22, ATtiny26),
//  classic+ (ATtiny24), 
//   AVRtiny  (ATtiny20, ATtiny40).
// 
// The AVR1 subset was not popular and no new models have been introduced since 2000.
// It omits all RAM except for the 32 registers mapped at address 31
//  and the I/O ports at addresses 32..95.
//  The stack is replaced by a 3-level hardware stack,
//  and the PUSH and POP instructions are deleted.
// All 16-bit operations are deleted, as are IJMP, ICALL,
//  and all load and store addressing modes except indirect via Z. 
// 
// -------------------------------------------------------------------------------------------------------------
//     Family       | Members         | Arithmetic       | Branches         | Transfers        | Bitwise
// -------------------------------------------------------------------------------------------------------------
//  Minimal AVR1    | AT90S1200       | ADD (LSL)        | RJMP             | LD               | SBI
//     Core         | ATtiny11        | ADC (ROL)        | RCALL            | ST               | CBI
//                  | ATtiny12        | SUB              | RET              | MOV              | LSR
//                  | ATtiny15        | SUBI             | RETI             | LDI (SER)        | ROR
//                  | ATtiny28        | SBC              | CPSE             | IN               | ASR
//                  |                 | SBCI             | CP               | OUT              | SWAP
//                  |                 | AND (TST)        | CPC              | LPM (not in      | BSET (SEC, SEZ,
//                  |                 | ANDI (CBR)       | CPI              | AT90S1200)       |  SEN, SEV, SES,
//                  |                 | OR               | SBRC             |                  |  SEH, SET, SEI)
//                  |                 | ORI (SBR)        | SBRS             |                  | BCLR (CLC, CLZ,
//                  |                 | EOR (CLR)        | SBIC             |                  |  CLN, CLV, CLS,
//                  |                 | COM              | SBIS             |                  |  CLH, CLT, CLI)
//                  |                 | NEG              | BRBS (BRCS,BRLO, |                  | BST
//                  |                 | INC              |  BREQ,BRMI,BRVS, |                  | BLD
//                  |                 | DEC              |  BRLT,BRHS,BRTS, |                  | NOP
//                  |                 |                  |  BRIE)           |                  | SLEEP
//                  |                 | new instructions | BRBC (BRCC,BRSH, |                  | WDR
//                  |                 |                  |  BRNE,BRPL,BRVC, |                  |
//                  |                 |                  |  BRGE,BRHC,BRTC, |                  |
//                  |                 |                  |  BRID)           |                  |
// ---------------------------------------------------------------------------------------------------------------
// Classic Core up  | AT90S2313       | new instructions | new instructions | new instructions |
// to 8K Program    | AT90S2323       | ADIW             | IJMP             | LD (now 9 modes) |
//  Space ("AVR2")  | ATtiny22        | SBIW             | ICALL            | LDD              |
//                  | AT90S2333       |                  |                  | LDS              |
//                  | AT90S2343       |                  |                  | ST (9 modes)     |
//                  | AT90S4414       |                  |                  | STD              |  (nothing new)
//                  | AT90S4433       |                  |                  | STS              |
//                  | AT90S4434       |                  |                  | PUSH             |
//                  | AT90S8515       |                  |                  | POP              |
//                  | AT90C8534       |                  |                  |                  |
//                  | AT90S8535       |                  |                  |                  |
//                  | ATtiny26        |                  |                  |                  |
// ---------------------------------------------------------------------------------------------------------------
// AVR2, with       | ATa5272         |                  |                  |                  |
// MOVW and LPM     | ATtiny13/a      |                  |                  |                  |
// instructions     | ATtiny2313/a    |                  |                  |                  |
//  ("AVR2.5")      | ATtiny24/a      |                  |                  |                  |
//                  | ATtiny25        |                  |                  |                  |
//                  | ATtiny261/a     |                  |                  |                  |
//                  | ATtiny4313      |                  |                  |                  |
//                  | ATtiny43u       |                  |                  |                  |
//                  | ATtiny44/a      |  (nothing new)   |  (nothing new)   | new instructions |  (nothing new)
//                  | ATtiny45        |                  |                  | MOVW             |
//                  | ATtiny461/a     |                  |                  | LPM (Rx, Z[+])   |
//                  | ATtiny48        |                  |                  | SPM              |
//                  | ATtiny828       |                  |                  |                  |
//                  | ATtiny84/a      |                  |                  |                  |
//                  | ATtiny85        |                  |                  |                  |
//                  | ATtiny861/a     |                  |                  |                  |
//                  | ATtiny87        |                  |                  |                  |
//                  | ATtiny88        |                  |                  |                  |
// ---------------------------------------------------------------------------------------------------------------
//   Classic Core   | ATmega103       |                  | new instructions | new instructions |
// with up to 128K  | ATmega603       |  (nothing new)   | JMP              | ELPM in "AVR3.1" |  (nothing new)
//     ("AVR3")     | AT43USB320      |                  | CALL             |                  |
//                  | AT76C711        |                  |                  |                  |
// ---------------------------------------------------------------------------------------------------------------
// Enhanced Core    | ATmega8         | new instructions |                  | new instructions |
//  with up to 8K   | ATmega83        | MUL              |                  | MOVW             |
//     ("AVR4")     | ATmega85        | MULS             |                  | LPM (3 modes)    |
//                  | ATmega8515      | MULSU            |  (nothing new)   | SPM              |  (nothing new)
//                  |                 | FMULS            |                  |                  |
//                  |                 | FMULSU[3]        |                  |                  |
// ---------------------------------------------------------------------------------------------------------------
//  Enhanced Core   | ATmega16        |                  | new instruction  |                  | new instruction
// with up to 128K  | ATmega161       |                  | ELPMX ("AVR5.1") |                  | BREAK
// ("AVR5","AVR5.1")| ATmega163       |                  |                  |                  |
//                  | ATmega32        |                  |                  |                  |
//                  | ATmega323       |                  |                  |                  |
//                  | ATmega64        |                  |                  |                  |
//                  | ATmega128       |                  |                  |                  |
//                  | AT43USB355      |                  |                  |                  |
//                  | AT94 (FPSLIC)   |                  |                  |                  |
//                  | AT90CAN series  |  (nothing new)   |                  | (nothing new)    |
//                  | AT90PWM series  |                  |                  |                  |
//                  | ATmega48        |                  |                  |                  |
//                  | ATmega88        |                  |                  |                  |
//                  | ATmega168       |                  |                  |                  |
//                  | ATmega162       |                  |                  |                  |
//                  | ATmega164       |                  |                  |                  |
//                  | ATmega324       |                  |                  |                  |
//                  | ATmega328       |                  |                  |                  |
//                  | ATmega644       |                  |                  |                  |
//                  | ATmega165       |                  |                  |                  |
//                  | ATmega169       |                  |                  |                  |
//                  | ATmega325       |                  |                  |                  |
//                  | ATmega3250      |                  |                  |                  |
//                  | ATmega645       |                  |                  |                  |
//                  | ATmega6450      |                  |                  |                  |
//                  | ATmega406       |                  |                  |                  |
// ---------------------------------------------------------------------------------------------------------------
//  Enhanced Core   | ATmega640       |                  | new instructions |                  |
//  with up to 4M   | ATmega1280      |                  | EIJMP            |                  |
// ("AVR5","AVR6")  | ATmega1281      | (nothing new)    | EICALL           | (nothing new)    | (nothing new)
//                  | ATmega2560      |                  |                  |                  |
//                  | ATmega2561      |                  |                  |                  |
// ---------------------------------------------------------------------------------------------------------------
// XMEGA Core       | ATxmega series  | new instruction  |                  | new instructions |
// ("avrxmega" 2-6) |                 | DES              |                  | (from second     |
//                  |                 |                  | (nothing new)    | revision silicon | (nothing new)
//                  |                 |                  |                  | - AU,B,C parts)  |
//                  |                 |                  |                  | XCH              |
//                  |                 |                  |                  | LAS              |
//                  |                 |                  |                  | LAC              |
//                  |                 |                  |                  | LAT              |
//---------------------------------------------------------------------------------------------------------------
//  Reduced AVRtiny | ATtiny40        | (Identical to    | (Identical to    | (Identical to    | (Identical to   
//      Core        | ATtiny20        | minimal core,    | classic core     | classic core     | enhanced core    
//   ("avrtiny10")  | ATtiny10        | except for       | with up to 8K,   | with up to 8K,   | with up to 128K,  
//                  | ATtiny9         | reduced CPU      | except for       | with the         | except for      
//                  | ATtiny5         | register set[a]) | reduced CPU      | following        | reduced CPU     
//                  | ATtiny4         |                  | register set[a]) | exceptions:      | register set[a])
//                  |                 |                  |                  | LPM (removed)    |
//                  |                 |                  |                  | LDD (removed)    |
//                  |                 |                  |                  | STD (removed)    |
//                  |                 |                  |                  | LD  (also        |
//                  |                 |                  |                  |     accesses     |       
//                  |                 |                  |                  |     program      |       
//                  |                 |                  |                  |     memory)      |       
//                  |                 |                  |                  | LDS (access is   |               
//                  |                 |                  |                  | STS limited to   |               
//                  |                 |                  |                  |     the first    |               
//                  |                 |                  |                  |     128 bytes    |
//                  |                 |                  |                  |     of SRAM)     |
//                  |                 |                  |                  | Reduced CPU      |
//                  |                 |                  |                  | register set[a]  |
// ---------------------------------------------------------------------------------------------------------------
// 
// -------------------------------------------------------------------------
// [a] Reduced register set is limited to R16 through R31. 
// 
// [3] Atmel. Application Note "AVR201: Using the AVR Hardware Multiplier".
//  2002. quote: "The megaAVR is a series of new devices in the
//  AVR RISC Microcontroller family that includes,
//    among other new enhancements, a hardware multiplier."    
// -------------------------------------------------------------------------

// XASMAVR Global variables
//
// Device Tables
// © 2021 Microchip Technology Inc. Manual DS40002198B-page 149++
// ---------------------------------------------------------------------------------------------------------------------------
// megaAVR® Devices                                                          |   FLASH FLASH     RAM      RAM   EEPROM  EEPROM
// Device           Core     Missing Instructions                            |   Size  Pagesize  Start    Size  Start   Size
// ---------------------------------------------------------------------------------------------------------------------------
// AT90CAN128       AVRe+          EIJMP, EICALL                             |   128K  -         0x100     4K   -        4K
// AT90CAN32        AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -        1K
// AT90CAN64        AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x100     4K   -        2K
// ATmega1280       AVRe+          EIJMP, EICALL                             |   128K  -         0x200     8K   -        4K
// ATmega1281       AVRe+          EIJMP, EICALL                             |   128K  -         0x200     8K   -        4K
// ATmega1284P      AVRe+          EIJMP, EICALL                             |   128K  -         0x100    16K   -        4K
// ATmega1284RFR2   AVRe+          EIJMP, EICALL                             |   128K  -         0x200    16K   -        4K       
// ATmega1284       AVRe+          EIJMP, EICALL                             |   128K  -         0x100    16K   -        4K                             
// ATmega128A       AVRe+          EIJMP, EICALL                             |   128K  -         0x100     4K   -        4K
// ATmega128RFA1    AVRe+          EIJMP, EICALL                             |   128K  -         0x200    16K   -        4K                             
// ATmega128RFR2    AVRe+          EIJMP, EICALL                             |   128K  -         0x200    16K   -        4K                             
// ATmega128        AVRe+          EIJMP, EICALL                             |   128K  -         0x100     4K   -        4K
// ATmega1608       AVRxt    ELPM, EIJMP, EICALL,            SPM, SPM Z+     |    16K  -         0x2800    2K   0x1400  256                             
// ATmega1609       AVRxt    ELPM, EIJMP, EICALL,            SPM, SPM Z+     |    16K  -         0x2800    2K   0x1400  256                             
// ATmega162        AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x060     1K   -       512                       
// ATmega164A       AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x100     1K   -       512                       
// ATmega164PA      AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x100     1K   -       512
// ATmega164P       AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x100     1K   -       512
// ATmega165A       AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x100     1K   -       512                            
// ATmega165PA      AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x100     1K   -       512                            
// ATmega165P       AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x100     1K   -       512                            
// ATmega168A       AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x100     1K   -       512
// ATmega168PA      AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x100     1K   -       512
// ATmega168PB      AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x100     1K   -       512                             
// ATmega168P       AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x100     1K   -       512
// ATmega168        AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x100     1K   -       512
// ATmega169A       AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x100     1K   -       512                            
// ATmega169PA      AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x100     1K   -       512                            
// ATmega169P       AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x100     1K   -       512                            
// ATmega16A        AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x060     1K   -       512                            
// ATmega16HVA      AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x100    512   -       256                            
// ATmega16HVB      AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x100     1K   -       512                            
// ATmega16HVBrevB  AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x100     1K   -       512                             
// ATmega16M1       AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x100     1K   -       512                             
// ATmega16U2       AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x100    512   -       512                             
// ATmega16U4       AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x100   1.25K  -       512
// ATmega16         AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x060     1K   -       512
// ATmega2560       AVRe+                                                    |   256K  -         0x200     8K   -        4K
// ATmega2561       AVRe+                                                    |   256K  -         0x200     8K   -        4K
// ATmega2564RFR2   AVRe+                                                    |   256K  -         0x200    32K   -        8K      
// ATmega256RFR2    AVRe+                                                    |   256K  -         0x200    32K   -        8K                            
// ATmega3208       AVRxt    ELPM, EIJMP, EICALL,            SPM, SPM Z+     |    32K  -         0x2800    4K   0x1400  256                            
// ATmega3209       AVRxt    ELPM, EIJMP, EICALL,            SPM, SPM Z+     |    32K  -         0x2800    4K   0x1400  256                            
// ATmega324A       AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -        1K
// ATmega324PA      AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -        1K
// ATmega324PB      AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -        1K                            
// ATmega324P       AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -        1K
// ATmega3250A      AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -       512                           
// ATmega3250PA     AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -       512                            
// ATmega3250P      AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -        1K                            
// ATmega3250       AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -        1K                            
// ATmega325A       AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -       512                            
// ATmega325PA      AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -       512                            
// ATmega325P       AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -        1K                            
// ATmega325        AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -        1K                            
// ATmega328PB      AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -        1K
// ATmega328P       AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -        1K
// ATmega328        AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -        1K
// ATmega3290A      AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -        1K                             
// ATmega3290PA     AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -        1K                             
// ATmega3290P      AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -        2K                             
// ATmega3290       AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -        1K                             
// ATmega329A       AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -        1K                             
// ATmega329PA      AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -        1K                             
// ATmega329P       AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -        1K                             
// ATmega329        AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -        1K                             
// ATmega32A        AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x060     2K   -        1K                             
// ATmega32C1       AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -        1K                             
// ATmega32HVB      AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -        1K                             
// ATmega32HVBrevB  AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -        1K                             
// ATmega32M1       AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     2K   -        1K                            
// ATmega32U2       AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100     1K   -        1K                       
// ATmega32U4       AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x100    2.5K  -      1.25K
// ATmega32         AVRe+    ELPM, EIJMP, EICALL                             |    32K  -         0x060     2K   -        1K
// ATmega406        AVRe+    ELPM, EIJMP, EICALL                             |     4K  -         0x100     2K   -       512                       
// ATmega4808       AVRxt    ELPM, EIJMP, EICALL,            SPM, SPM Z+     |    48K  -         0x2800    6K   0x1400  256                       
// ATmega4809       AVRxt    ELPM, EIJMP, EICALL,            SPM, SPM Z+     |    48K  -         0x2800    6K   0x1400  256   
// ATmega48A        AVRe+    ELPM, EIJMP, EICALL, CALL, JMP                  |     4K  -         0x100     512  -       256                       
// ATmega48PA       AVRe+    ELPM, EIJMP, EICALL, CALL, JMP                  |     4K  -         0x100     512  -       256
// ATmega48PB       AVRe+    ELPM, EIJMP, EICALL, CALL, JMP                  |     4K  -         0x100     512  -       256                             
// ATmega48P        AVRe+    ELPM, EIJMP, EICALL, CALL, JMP                  |     4K  -         0x100     512  -       256
// ATmega48         AVRe+    ELPM, EIJMP, EICALL, CALL, JMP                  |     4K  -         0x100     512  -       256
// ATmega640        AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x200     8K   -        4K                            
// ATmega644A       AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x100     4K   -        2K                            
// ATmega644PA      AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x100     4K   -        2K
// ATmega644P       AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x100     4K   -        2K
// ATmega644RFR2    AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x200     8K   -        2K     
// ATmega644        AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x100     4K   -        2K
// ATmega6450A      AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x100     4K   -       512                             
// ATmega6450P      AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x100     4K   -       512                             
// ATmega6450       AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x100     4K   -        2K                             
// ATmega645A       AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x100     4K   -       512                             
// ATmega645P       AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x100     4K   -       512                             
// ATmega645        AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x100     4K   -        2K                             
// ATmega6490A      AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x100     4K   -        2K                            
// ATmega6490P      AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x100     4K   -        2K                            
// ATmega6490       AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x100     4K   -        2K                            
// ATmega649A       AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x100     4K   -        2K                            
// ATmega649P       AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x100     4K   -        2K                            
// ATmega649        AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x100     4K   -        2K                            
// ATmega64A        AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x100     4K   -        2K                             
// ATmega64C1       AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x100     4K   -        2K                             
// ATmega64HVE2     AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x100     4K   -        2K                             
// ATmega64M1       AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x100     4K   -        2K                             
// ATmega64RFR2     AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x200     8K   -        2K
// ATmega64         AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x060     4K   -        2K                             
// ATmega808        AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |     8K  -         0x2800    1K   0x1400  256                       
// ATmega809        AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |     8K  -         0x2800    1K   0x1400  256                       
// ATmega8515       AVRe+    ELPM, EIJMP, EICALL, CALL, JMP, BREAK           |     8K  -         0x060    512   -       512
// ATmega8535       AVRe+    ELPM, EIJMP, EICALL, CALL, JMP, BREAK           |     8K  -         0x060    512   -       512                       
// ATmega88A        AVRe+    ELPM, EIJMP, EICALL, CALL, JMP                  |     8K  -         0x100     1K   -       512
// ATmega88PA       AVRe+    ELPM, EIJMP, EICALL, CALL, JMP                  |     8K  -         0x100     1K   -       512
// ATmega88PB       AVRe+    ELPM, EIJMP, EICALL, CALL, JMP                  |     8K  -         0x100     1K   -       512                            
// ATmega88P        AVRe+    ELPM, EIJMP, EICALL, CALL, JMP                  |     8K  -         0x100     1K   -       512
// ATmega88         AVRe+    ELPM, EIJMP, EICALL, CALL, JMP                  |     8K  -         0x100    512   -       512                            
// ATmega8A         AVRe+    ELPM, EIJMP, EICALL, CALL, JMP, BREAK           |     8K  -         0x060     1K   -       512
// ATmega8HVA       AVRe+    ELPM, EIJMP, EICALL, CALL, JMP                  |     8K  -         0x100    512   -       256                            
// ATmega8U2        AVRe+    ELPM, EIJMP, EICALL, CALL, JMP                  |     8K  -         0x100    512   -       512                            
// ATmega8          AVRe+    ELPM, EIJMP, EICALL, CALL, JMP, BREAK           |     8K  -         0x060     1K   -       512
// AT90PWM161       AVRe+    ELPM, EIJMP, EICALL, CALL, JMP                  |    16K  -         0x100     1K   -       512
// AT90PWM1         AVRe+    ELPM, EIJMP, EICALL, CALL, JMP                  |     8K  -         0x100    512   -       512     
// AT90PWM216       AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x100     1K   -       512     
// AT90PWM2B        AVRe+    ELPM, EIJMP, EICALL, CALL, JMP                  |     8K  -         0x100    512   -       512     
// AT90PWM316       AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x100     1K   -       512     
// AT90PWM3B        AVRe+    ELPM, EIJMP, EICALL, CALL, JMP                  |     8K  -         0x100    512   -       512     
// AT90PWM81        AVRe+    ELPM, EIJMP, EICALL, CALL, JMP                  |     8K  -         0x100    256   -       512     
// AT90USB1286      AVRe+          EIJMP, EICALL                             |   128K  -         0x100     8K   -        4K     
// AT90USB1287      AVRe+          EIJMP, EICALL                             |   128K  -         0x100     5K   -        4K     
// AT90USB162       AVRe+    ELPM, EIJMP, EICALL                             |    16K  -         0x100    512   -       512     
// AT90USB646       AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x100     4K   -        2K     
// AT90USB647       AVRe+    ELPM, EIJMP, EICALL                             |    64K  -         0x100     4K   -        2K     
// AT90USB82        AVRe+    ELPM, EIJMP, EICALL, CALL, JMP                  |     8K  -         0x100    512   -       512     
//
char* _AT90CAN128     = "AT90CAN128";
char* _AT90CAN32      = "AT90CAN32";
char* _AT90CAN64      = "AT90CAN64";
char* _ATmega1280     = "ATmega1280";
char* _ATmega1281     = "ATmega1281";
char* _ATmega1284P    = "ATmega1284P";
char* _ATmega1284RFR2 = "ATmega1284RFR2";
char* _ATmega1284     = "ATmega1284";
char* _ATmega128A     = "ATmega128A";
char* _ATmega128RFA1  = "ATmega128RFA1";
char* _ATmega128RFR2  = "ATmega128RFR2";
char* _ATmega128      = "ATmega128";
char* _ATmega1608     = "ATmega1608";
char* _ATmega1609     = "ATmega1609";
char* _ATmega162      = "ATmega162";
char* _ATmega164A     = "ATmega164A";
char* _ATmega164PA    = "ATmega164PA";
char* _ATmega164P     = "ATmega164P";
char* _ATmega165A     = "ATmega165A";
char* _ATmega165PA    = "ATmega165PA";
char* _ATmega165P     = "ATmega165P";
char* _ATmega168A     = "ATmega168A";
char* _ATmega168PA    = "ATmega168PA";
char* _ATmega168PB    = "ATmega168PB";
char* _ATmega168P     = "ATmega168P";
char* _ATmega168      = "ATmega168";
char* _ATmega169A     = "ATmega169A";
char* _ATmega169PA    = "ATmega169PA";
char* _ATmega169P     = "ATmega169P";
char* _ATmega16A      = "ATmega16A";
char* _ATmega16HVA    = "ATmega16HVA";
char* _ATmega16HVB    = "ATmega16HVB";
char* _ATmega16HVBrevB= "ATmega16HVBrevB";
char* _ATmega16M1     = "ATmega16M1";
char* _ATmega16U2     = "ATmega16U2";
char* _ATmega16U4     = "ATmega16U4";
char* _ATmega16       = "ATmega16";
char* _ATmega2560     = "ATmega2560";
char* _ATmega2561     = "ATmega2561";
char* _ATmega2564RFR2 = "ATmega2564RFR2";
char* _ATmega256RFR2  = "ATmega256RFR2";
char* _ATmega3208     = "ATmega3208";
char* _ATmega3209     = "ATmega3209";
char* _ATmega324A     = "ATmega324A";
char* _ATmega324PA    = "ATmega324PA";
char* _ATmega324PB    = "ATmega324PB";
char* _ATmega324P     = "ATmega324P";
char* _ATmega3250A    = "ATmega3250A";
char* _ATmega3250PA   = "ATmega3250PA";
char* _ATmega3250P    = "ATmega3250P";
char* _ATmega3250     = "ATmega3250";
char* _ATmega325A     = "ATmega325A";
char* _ATmega325PA    = "ATmega325PA";
char* _ATmega325P     = "ATmega325P";
char* _ATmega325      = "ATmega325";
char* _ATmega328PB    = "ATmega328PB";
char* _ATmega328P     = "ATmega328P";
char* _ATmega328      = "ATmega328";
char* _ATmega3290A    = "ATmega3290A";
char* _ATmega3290PA   = "ATmega3290PA";
char* _ATmega3290P    = "ATmega3290P";
char* _ATmega3290     = "ATmega3290";
char* _ATmega329A     = "ATmega329A";
char* _ATmega329PA    = "ATmega329PA";
char* _ATmega329P     = "ATmega329P";
char* _ATmega329      = "ATmega329";
char* _ATmega32A      = "ATmega32A";
char* _ATmega32C1     = "ATmega32C1";
char* _ATmega32HVB    = "ATmega32HVB";
char* _ATmega32HVBrevB= "ATmega32HVBrevB";
char* _ATmega32M1     = "ATmega32M1";
char* _ATmega32U2     = "ATmega32U2";
char* _ATmega32U4     = "ATmega32U4";
char* _ATmega32       = "ATmega32";
char* _ATmega406      = "ATmega406";
char* _ATmega4808     = "ATmega4808";
char* _ATmega4809     = "ATmega4809";
char* _ATmega48A      = "ATmega48A";
char* _ATmega48PA     = "ATmega48PA";
char* _ATmega48PB     = "ATmega48PB";
char* _ATmega48P      = "ATmega48P";
char* _ATmega48       = "ATmega48";
char* _ATmega640      = "ATmega640";
char* _ATmega644A     = "ATmega644A";
char* _ATmega644PA    = "ATmega644PA";
char* _ATmega644P     = "ATmega644P";
char* _ATmega644RFR2  = "ATmega644RFR2";
char* _ATmega644      = "ATmega644";
char* _ATmega6450A    = "ATmega6450A";
char* _ATmega6450P    = "ATmega6450P";
char* _ATmega6450     = "ATmega6450";
char* _ATmega645A     = "ATmega645A";
char* _ATmega645P     = "ATmega645P";
char* _ATmega645      = "ATmega645";
char* _ATmega6490A    = "ATmega6490A";
char* _ATmega6490P    = "ATmega6490P";
char* _ATmega6490     = "ATmega6490";
char* _ATmega649A     = "ATmega649A";
char* _ATmega649P     = "ATmega649P";
char* _ATmega649      = "ATmega649";
char* _ATmega64A      = "ATmega64A";
char* _ATmega64C1     = "ATmega64C1";
char* _ATmega64HVE2   = "ATmega64HVE2";
char* _ATmega64M1     = "ATmega64M1";
char* _ATmega64RFR2   = "ATmega64RFR2";
char* _ATmega64       = "ATmega64";
char* _ATmega808      = "ATmega808";
char* _ATmega809      = "ATmega809";
char* _ATmega8515     = "ATmega8515";
char* _ATmega8535     = "ATmega8535";
char* _ATmega88A      = "ATmega88A";
char* _ATmega88PA     = "ATmega88PA";
char* _ATmega88PB     = "ATmega88PB";
char* _ATmega88P      = "ATmega88P";
char* _ATmega88       = "ATmega88";
char* _ATmega8A       = "ATmega8A";
char* _ATmega8HVA     = "ATmega8HVA";
char* _ATmega8U2      = "ATmega8U2";
char* _ATmega8        = "ATmega8";
char* _AT90PWM161     = "AT90PWM161";
char* _AT90PWM1       = "AT90PWM1";
char* _AT90PWM216     = "AT90PWM216";
char* _AT90PWM2B      = "AT90PWM2B";
char* _AT90PWM316     = "AT90PWM316";
char* _AT90PWM3B      = "AT90PWM3B";
char* _AT90PWM81      = "AT90PWM81";
char* _AT90USB1286    = "AT90USB1286";
char* _AT90USB1287    = "AT90USB1287";
char* _AT90USB162     = "AT90USB162";
char* _AT90USB646     = "AT90USB646";
char* _AT90USB647     = "AT90USB647";
char* _AT90USB82      = "AT90USB82";

char* _AT90S1200      = "AT90S1200";
char* _AT90S2313      = "AT90S2313";
char* _AT90S2323      = "AT90S2323";
char* _AT90S2333      = "AT90S2333";
char* _AT90S2343      = "AT90S2343";
char* _AT90S4414      = "AT90S4414";
char* _AT90S4433      = "AT90S4433";
char* _AT90S4434      = "AT90S4434";
char* _AT90S8515      = "AT90S8515";
char* _AT90C8534      = "AT90C8534";
char* _AT90S8535      = "AT90S8535";
                                   
// ---------------------------------------------------------------------------------------------------------------------------
// tinyAVR® Devices                                                          |   FLASH FLASH     RAM      RAM   EEPROM  EEPROM
// Device           Core     Missing Instructions                            |   Size  Start     Start    Size  Start   Size
// ---------------------------------------------------------------------------------------------------------------------------
// ATtiny102        AVRrc                                                    |     1K  0         0x040     32    -        0 
// ATtiny104        AVRrc                                                    |     1K  0         0x040     32    -        0 
// ATtiny10         AVRrc    BREAK                                           |     1K  0         0x040     32    -        0
// ATtiny11         AVR      BREAK, LPM, LPM Z+ ADIW, SBIW, IJMP, ICALL, LD  |     1K  0         -          0    -        0
// ATtiny12         AVR      BREAK, LPM, LPM Z+ ADIW, SBIW, IJMP, ICALL, LD  |     1K  0         -          0    0       64
// ATtiny13A        AVRe     ELPM,                CALL, JMP                  |     1K  0         0x060     64    0       64
// ATtiny13         AVRe     ELPM,                CALL, JMP                  |     1K  0         0x060     64    0       64
// ATtiny15         AVR      BREAK, LPM, LPM Z+ ADIW, SBIW, IJMP, ICALL, LD  |     1K  0         -          0    0       64
// ATtiny1604       AVRxt    ELPM, EIJMP, EICALL,            SPM, SPM Z+     |    16K  0x8000    0x3C00    1K    0x1400 256
// ATtiny1606       AVRxt    ELPM, EIJMP, EICALL,            SPM, SPM Z+     |    16K  0x8000    0x3C00    1K    0x1400 256
// ATtiny1607       AVRxt    ELPM, EIJMP, EICALL,            SPM, SPM Z+     |    16K  0x8000    0x3C00    1K    0x1400 256
// ATtiny1614       AVRxt    ELPM, EIJMP, EICALL,            SPM, SPM Z+     |    16K  0x8000    0x3800    2K    0x1400 256
// ATtiny1616       AVRxt    ELPM, EIJMP, EICALL,            SPM, SPM Z+     |    16K  0x8000    0x3800    2K    0x1400 256
// ATtiny1617       AVRxt    ELPM, EIJMP, EICALL,            SPM, SPM Z+     |    16K  0x8000    0x3800    2K    0x1400 256
// ATtiny1624       AVRxt    ELPM, EIJMP, EICALL,            SPM, SPM Z+     |    16K  0x8000    0x3400    3K    0x1400 256
// ATtiny1626       AVRxt    ELPM, EIJMP, EICALL,            SPM, SPM Z+     |    16K  0x8000    0x3400    3K    0x1400 256
// ATtiny1627       AVRxt    ELPM, EIJMP, EICALL,            SPM, SPM Z+     |    16K  0x8000    0x3400    3K    0x1400 256
// ATtiny1634       AVRe                                                     |    16K  0         0x100     1K    0      256
// ATtiny167        AVRe                                                     |    16K  0         0x100    512    0      512
// ATtiny202        AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |     2K  0x8000    0x3F80   128    0x1400  64
// ATtiny204        AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |     2K  0x8000    0x3F80   128    0x1400  64
// ATtiny20         AVRrc    BREAK                                           |     2K  0         0x040    128    -        0
// ATtiny212        AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |     2K  0x8000    0x3F80   128    0x1400  64
// ATtiny214        AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |     2K  0x8000    0x3F80   128    0x1400  64
// ATtiny22         ---                                                      |     2K  0         0x060    128    0      128
// ATtiny2313A      AVRe     ELPM, CALL, JMP                                 |     2K  0         0x060    128    0      128
// ATtiny2313       AVRe     ELPM, CALL, JMP                                 |     2K  0         0x060    128    0      128
// ATtiny24A        AVRe     ELPM, CALL, JMP                                 |     2K  0         0x060    128    0      128
// ATtiny24         AVRe     ELPM, CALL, JMP                                 |     2K  0         0x060    128    0      128
// ATtiny25         AVRe     ELPM, CALL, JMP                                 |     2K  0         0x060    128    0      128
// ATtiny261A       AVRe     ELPM, CALL, JMP                                 |     2K  0         0x060    128    0      128
// ATtiny261        AVRe     ELPM, CALL, JMP                                 |     2K  0         0x060    128    0      128
// ATtiny26         AVR      BREAK                                           |     2K  0         0x060    128    0      128
// ATtiny28         ---                                                      |     2K  0         -          0    -        0      
// ATtiny3216       AVRxt    ELPM, EIJMP, EICALL,            SPM, SPM Z+     |    32K  0         0x060     2K    0      256
// ATtiny3217       AVRxt    ELPM, EIJMP, EICALL,            SPM, SPM Z+     |    32K  0         0x060     2K    0      256
// ATtiny3224       AVRxt    ELPM, EIJMP, EICALL,            SPM, SPM Z+     |    32K  0x8000    0x3400    3K    0x1400 256   
// ATtiny3226       AVRxt    ELPM, EIJMP, EICALL,            SPM, SPM Z+     |    32K  0x8000    0x3400    3K    0x1400 256   
// ATtiny3227       AVRxt    ELPM, EIJMP, EICALL,            SPM, SPM Z+     |    32K  0x8000    0x3400    3K    0x1400 256   
// ATtiny402        AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |     4K  0x8000    0x3F00   256    0x1400 128
// ATtiny404        AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |     4K  0x8000    0x3F00   256    0x1400 128
// ATtiny406        AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |     4K  0x8000    0x3F00   256    0x1400 128
// ATtiny40         AVRrc                                                    |     4K  0         0x100     64    -        0                         
// ATtiny412        AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |     4K  0x8000    0x3F00   256    0x1400 128
// ATtiny414        AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |     4K  0x8000    0x3F00   256    0x1400 128
// ATtiny416        AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |     4K  0x8000    0x3F00   256    0x1400 128
// ATtiny417        AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |     4K  0x8000    0x3F00   256    0x1400 128
// ATtiny424        AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |     4K  0x8000    0x3E00   512    0x1400 128                         
// ATtiny426        AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |     4K  0x8000    0x3E00   512    0x1400 128                         
// ATtiny427        AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |     4K  0x8000    0x3E00   512    0x1400 128                         
// ATtiny4313       AVRe     ELPM,                CALL, JMP                  |     4K  0         0x060    256    0      256
// ATtiny43U        AVRe     ELPM,                CALL, JMP                  |     4K  0         0x060    256    0       64                         
// ATtiny441        AVRe     ELPM,                CALL, JMP                  |     4K  0         0x100    256    0      256                         
// ATtiny44A        AVRe     ELPM,                CALL, JMP                  |     4K  0         0x060    256    0      256
// ATtiny44         AVRe     ELPM,                CALL, JMP                  |     4K  0         0x060    256    0      256
// ATtiny45         AVRe     ELPM,                CALL, JMP                  |     4K  0         0x060    256    0      256
// ATtiny461A       AVRe     ELPM,                CALL, JMP                  |     4K  0         0x060    256    0      256
// ATtiny461        AVRe     ELPM,                CALL, JMP                  |     4K  0         0x060    256    0      256
// ATtiny48         AVRe     ELPM,                CALL, JMP                  |     4K  0         0x010    256    0       64
// ATtiny4          AVRrc    BREAK                                           |    512  0         0x040     32    -        0
// ATtiny5          AVRrc    BREAK                                           |    512  0         0x040     32    -        0
// ATtiny804        AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |     8K  0x8000    0x3E00   512    0x1400 128                         
// ATtiny806        AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |     8K  0x8000    0x3E00   512    0x1400 128                         
// ATtiny807        AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |     8K  0x8000    0x3E00   512    0x1400 128                         
// ATtiny814        AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |     8K  0x8000    0x3E00   512    0x1400 128
// ATtiny816        AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |     8K  0x8000    0x3E00   512    0x1400 128
// ATtiny817        AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |     8K  0x8000    0x3E00   512    0x1400 128
// ATtiny824        AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |     8K  0x8000    0x3C00    1K    0x1400 128                         
// ATtiny826        AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |     8K  0x8000    0x3C00    1K    0x1400 128                         
// ATtiny827        AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |     8K  0x8000    0x3C00    1K    0x1400 128                         
// ATtiny828        AVRe     ELPM,                CALL, JMP                  |     8K  0         0x100    512    0      256                         
// ATtiny841        AVRe     ELPM,                CALL, JMP                  |     8K  0         0x100    512    0      256                         
// ATtiny84A        AVRe     ELPM,                CALL, JMP                  |     8K  0         0x060    512    0      512                         
// ATtiny84         AVRe     ELPM,                CALL, JMP                  |     8K  0         0x060    512    0      512
// ATtiny85         AVRe     ELPM,                CALL, JMP                  |     8K  0         0x060    512    0      512
// ATtiny861A       AVRe     ELPM,                CALL, JMP                  |     8K  0         0x060    512    0      512
// ATtiny861        AVRe     ELPM,                CALL, JMP                  |     8K  0         0x060    512    0      512
// ATtiny87         AVRe     ELPM,                CALL, JMP                  |     8K  0         0x100    512    0      512
// ATtiny88         AVRe     ELPM,                CALL, JMP                  |     8K  0         0x100    512    0       64
// ATtiny9          AVRrc    BREAK                                           |     1K  0         0x040     32    -        0
//
char* _ATtiny102   = "ATtiny102";
char* _ATtiny104   = "ATtiny104";
char* _ATtiny10    = "ATtiny10";
char* _ATtiny11    = "ATtiny11";      // no PUSH/POP
char* _ATtiny12    = "ATtiny12";      // no PUSH/POP
char* _ATtiny13A   = "ATtiny13A";
char* _ATtiny13    = "ATtiny13";
char* _ATtiny15    = "ATtiny15";      // no PUSH/POP
char* _ATtiny1604  = "ATtiny1604";
char* _ATtiny1606  = "ATtiny1606";
char* _ATtiny1607  = "ATtiny1607";
char* _ATtiny1614  = "ATtiny1614";
char* _ATtiny1616  = "ATtiny1616";
char* _ATtiny1617  = "ATtiny1617";
char* _ATtiny1624  = "ATtiny1624";
char* _ATtiny1626  = "ATtiny1626";
char* _ATtiny1627  = "ATtiny1627";
char* _ATtiny1634  = "ATtiny1634";
char* _ATtiny167   = "ATtiny167";
char* _ATtiny202   = "ATtiny202";
char* _ATtiny204   = "ATtiny204";
char* _ATtiny20    = "ATtiny20";
char* _ATtiny212   = "ATtiny212";
char* _ATtiny214   = "ATtiny214";
char* _ATtiny22    = "ATtiny22";
char* _ATtiny2313A = "ATtiny2313A";
char* _ATtiny2313  = "ATtiny2313";
char* _ATtiny24A   = "ATtiny24A";
char* _ATtiny24    = "ATtiny24";
char* _ATtiny25    = "ATtiny25";
char* _ATtiny261A  = "ATtiny261A";
char* _ATtiny261   = "ATtiny261";
char* _ATtiny26    = "ATtiny26";
char* _ATtiny28    = "ATtiny28";
char* _ATtiny3216  = "ATtiny3216";
char* _ATtiny3217  = "ATtiny3217";
char* _ATtiny3224  = "ATtiny3224";
char* _ATtiny3226  = "ATtiny3226";
char* _ATtiny3227  = "ATtiny3227";
char* _ATtiny402   = "ATtiny402";
char* _ATtiny404   = "ATtiny404";
char* _ATtiny406   = "ATtiny406";
char* _ATtiny40    = "ATtiny40";
char* _ATtiny412   = "ATtiny412";
char* _ATtiny414   = "ATtiny414";
char* _ATtiny416   = "ATtiny416";
char* _ATtiny417   = "ATtiny417";
char* _ATtiny424   = "ATtiny424";
char* _ATtiny426   = "ATtiny426";
char* _ATtiny427   = "ATtiny427";
char* _ATtiny4313  = "ATtiny4313";
char* _ATtiny43U   = "ATtiny43U";
char* _ATtiny441   = "ATtiny441";
char* _ATtiny44A   = "ATtiny44A";
char* _ATtiny44    = "ATtiny44";
char* _ATtiny45    = "ATtiny45";
char* _ATtiny461A  = "ATtiny461A";
char* _ATtiny461   = "ATtiny461";
char* _ATtiny48    = "ATtiny48";
char* _ATtiny4     = "ATtiny4";
char* _ATtiny5     = "ATtiny5";
char* _ATtiny804   = "ATtiny804";
char* _ATtiny806   = "ATtiny806";
char* _ATtiny807   = "ATtiny807";
char* _ATtiny814   = "ATtiny814";
char* _ATtiny816   = "ATtiny816";
char* _ATtiny817   = "ATtiny817";
char* _ATtiny824   = "ATtiny824";
char* _ATtiny826   = "ATtiny826";
char* _ATtiny827   = "ATtiny827";
char* _ATtiny828   = "ATtiny828";
char* _ATtiny841   = "ATtiny841";
char* _ATtiny84A   = "ATtiny84A";
char* _ATtiny84    = "ATtiny84";
char* _ATtiny85    = "ATtiny85";
char* _ATtiny861A  = "ATtiny861A";
char* _ATtiny861   = "ATtiny861";
char* _ATtiny87    = "ATtiny87";
char* _ATtiny88    = "ATtiny88";
char* _ATtiny9     = "ATtiny9";

// ---------------------------------------------------------------------------------------------------------------------------
// AVR® Dx Devices                                                           |   FLASH FLASH     RAM      RAM   EEPROM  EEPROM
// Device           Core     Missing Instructions                            |   Size  Pagesize  Start    Size  Start   Size
// ---------------------------------------------------------------------------------------------------------------------------
// AVR128DA28       AVRxt          EIJMP, EICALL                             |   128K  512       0x4000   16K   0x1400  512
// AVR128DA32       AVRxt          EIJMP, EICALL                             |   128K  512       0x4000   16K   0x1400  512
// AVR128DA48       AVRxt          EIJMP, EICALL                             |   128K  512       0x4000   16K   0x1400  512
// AVR128DA64       AVRxt          EIJMP, EICALL                             |   128K  512       0x4000   16K   0x1400  512
// AVR32DA28        AVRxt    ELPM, EIJMP, EICALL                             |    32K  512       0x7000    4K   0x1400  512
// AVR32DA32        AVRxt    ELPM, EIJMP, EICALL                             |    32K  512       0x7000    4K   0x1400  512
// AVR32DA48        AVRxt    ELPM, EIJMP, EICALL                             |    32K  512       0x7000    4K   0x1400  512
// AVR64DA28        AVRxt    ELPM, EIJMP, EICALL                             |    64K  512       0x6000    8K   0x1400  512
// AVR64DA32        AVRxt    ELPM, EIJMP, EICALL                             |    64K  512       0x6000    8K   0x1400  512
// AVR64DA48        AVRxt    ELPM, EIJMP, EICALL                             |    64K  512       0x6000    8K   0x1400  512
// AVR64DA64        AVRxt    ELPM, EIJMP, EICALL                             |    64K  512       0x6000    8K   0x1400  512
// AVR128DB28       AVRxt          EIJMP, EICALL                             |   128K  512       0x4000   16K   0x1400  512
// AVR128DB32       AVRxt          EIJMP, EICALL                             |   128K  512       0x4000   16K   0x1400  512
// AVR128DB48       AVRxt          EIJMP, EICALL                             |   128K  512       0x4000   16K   0x1400  512
// AVR128DB64       AVRxt          EIJMP, EICALL                             |   128K  512       0x4000   16K   0x1400  512
// AVR32DB28        AVRxt    ELPM, EIJMP, EICALL                             |    32K  512       0x7000    4K   0x1400  512
// AVR32DB32        AVRxt    ELPM, EIJMP, EICALL                             |    32K  512       0x7000    4K   0x1400  512
// AVR32DB48        AVRxt    ELPM, EIJMP, EICALL                             |    32K  512       0x7000    4K   0x1400  512         
// AVR64DB28        AVRxt    ELPM, EIJMP, EICALL                             |    64K  512       0x6000    8K   0x1400  512         
// AVR64DB32        AVRxt    ELPM, EIJMP, EICALL                             |    64K  512       0x6000    8K   0x1400  512         
// AVR64DB48        AVRxt    ELPM, EIJMP, EICALL                             |    64K  512       0x6000    8K   0x1400  512         
// AVR64DB64        AVRxt    ELPM, EIJMP, EICALL                             |    64K  512       0x6000    8K   0x1400  512         
// AVR128DD28       AVRxt          EIJMP, EICALL                             |   128K  512       0x4000   16K   0x1400  512         
// AVR128DD32       AVRxt          EIJMP, EICALL                             |   128K  512       0x4000   16K   0x1400  512         
// AVR128DD48       AVRxt          EIJMP, EICALL                             |   128K  512       0x4000   16K   0x1400  512         
// AVR128DD64       AVRxt          EIJMP, EICALL                             |   128K  512       0x4000   16K   0x1400  512 ?        
// AVR32DD28        AVRxt    ELPM, EIJMP, EICALL                             |    32K  512       0x7000    4K   0x1400  512
// AVR32DD32        AVRxt    ELPM, EIJMP, EICALL                             |    32K  512       0x7000    4K   0x1400  512
// AVR32DD48        AVRxt    ELPM, EIJMP, EICALL                             |    32K  512       0x7000    4K   0x1400  512
// AVR64DD28        AVRxt    ELPM, EIJMP, EICALL                             |    64K  512       0x6000    8K   0x1400  512
// AVR64DD32        AVRxt    ELPM, EIJMP, EICALL                             |    64K  512       0x6000    8K   0x1400  512
// AVR64DD48        AVRxt    ELPM, EIJMP, EICALL                             |    64K  512       0x6000    8K   0x1400  512
// AVR64DD64        AVRxt    ELPM, EIJMP, EICALL                             |    64K  512       0x6000    8K   0x1400  512
//
char* _AVR128DA28 = "AVR128DA28";
char* _AVR128DA32 = "AVR128DA32";
char* _AVR128DA48 = "AVR128DA48";
char* _AVR128DA64 = "AVR128DA64";
char* _AVR32DA28  = "AVR32DA28";
char* _AVR32DA32  = "AVR32DA32";
char* _AVR32DA48  = "AVR32DA48";
char* _AVR64DA28  = "AVR64DA28";
char* _AVR64DA32  = "AVR64DA32";
char* _AVR64DA48  = "AVR64DA48";
char* _AVR64DA64  = "AVR64DA64";
char* _AVR128DB28 = "AVR128DB28";
char* _AVR128DB32 = "AVR128DB32";
char* _AVR128DB48 = "AVR128DB48";
char* _AVR128DB64 = "AVR128DB64";
char* _AVR32DB28  = "AVR32DB28";
char* _AVR32DB32  = "AVR32DB32";
char* _AVR32DB48  = "AVR32DB48";
char* _AVR64DB28  = "AVR64DB28";
char* _AVR64DB32  = "AVR64DB32";
char* _AVR64DB48  = "AVR64DB48";
char* _AVR64DB64  = "AVR64DB64";
char* _AVR128DD28 = "AVR128DD28";
char* _AVR128DD32 = "AVR128DD32";
char* _AVR128DD48 = "AVR128DD48";
char* _AVR128DD64 = "AVR128DD64";
char* _AVR32DD28  = "AVR32DD28";
char* _AVR32DD32  = "AVR32DD32";
char* _AVR32DD48  = "AVR32DD48";
char* _AVR64DD28  = "AVR64DD28";
char* _AVR64DD32  = "AVR64DD32";
char* _AVR64DD48  = "AVR64DD48";
char* _AVR64DD64  = "AVR64DD64";

// ---------------------------------------------------------------------------------------------------------------------------
// XMEGA® Devices                                                            |   FLASH FLASH     RAM      RAM   EEPROM  EEPROM
// Device           Core     Missing Instructions                            |   Size  Pagesize  Start    Size  Start   Size
// ---------------------------------------------------------------------------------------------------------------------------
// ATxmega128A1     AVRxm                         LAC, LAT, LAS, XCH         |   128K  ---       0x2000    8K   0x1000  2K  
// ATxmega128A1U    AVRxm                                                    |   128K  ---       0x2000    8K   0x1000  2K
// ATxmega128A3     AVRxm                         LAC, LAT, LAS, XCH         |   128K  ---       0x2000    8K   0x1000  2K
// ATxmega128A3U    AVRxm                                                    |   128K  ---       0x2000    8K   0x1000  2K
// ATxmega128A4U    AVRxm                                                    |   128K  ---       0x2000    8K   0x1000  2K
// ATxmega128B1     AVRxm                                                    |   128K  ---       0x2000    8K   0x1000  2K
// ATxmega128B3     AVRxm                                                    |   128K  ---       0x2000    4K   0x1000  2K
// ATxmega128C3     AVRxm                                                    |   128K  ---       0x2000    8K   0x1000  2K
// ATxmega128D3     AVRxm                    DES, LAC, LAT, LAS, XCH         |   128K  ---       0x2000    8K   0x1000  2K
// ATxmega128D4     AVRxm                    DES, LAC, LAT, LAS, XCH         |   128K  ---       0x2000    8K   0x1000  2K
// ATxmega16A4      AVRxm                    DES, LAC, LAT, LAS, XCH         |    16K  ---       0x2000    2K   0x1000  1K
// ATxmega16A4U     AVRxm                                                    |    16K  ---       0x2000    2K   0x1000  1K
// ATxmega16C4      AVRxm                                                    |    16K  ---       0x2000    2K   0x1000  1K
// ATxmega16D4      AVRxm                    DES, LAC, LAT, LAS, XCH         |    16K  ---       0x2000    2K   0x1000  1K
// ATxmega16E5      AVRxm                                                    |    16K  ---       0x2000    2K   0x1000  512
// ATxmega192A3     AVRxm                         LAC, LAT, LAS, XCH         |   192K  ---       0x2000   16K   0x1000  2K
// ATxmega192A3U    AVRxm                                                    |   192K  ---       0x2000   16K   0x1000  2K
// ATxmega192C3     AVRxm                                                    |   192K  ---       0x2000   16K   0x1000  2K
// ATxmega192D3     AVRxm                    DES, LAC, LAT, LAS, XCH         |   192K  ---       0x2000   16K   0x1000  2K
// ATxmega256A3B    AVRxm                    DES, LAC, LAT, LAS, XCH         |   256K  ---       0x2000   16K   0x1000  4K
// ATxmega256A3BU   AVRxm                    DES                             |   256K  ---       0x2000   16K   0x1000  4K
// ATxmega256A3     AVRxm                         LAC, LAT, LAS, XCH         |   256K  ---       0x2000   16K   0x1000  4K
// ATxmega256A3U    AVRxm                                                    |   256K  ---       0x2000   16K   0x1000  4K
// ATxmega256C3     AVRxm                                                    |   256K  ---       0x2000   16K   0x1000  4K
// ATxmega256D3     AVRxm                    DES, LAC, LAT, LAS, XCH         |   256K  ---       0x2000   16K   0x1000  4K
// ATxmega32C3      AVRxm                                                    |    32K  ---       0x2000    4K   0x1000  1K
// ATxmega32D3      AVRxm                    DES, LAC, LAT, LAS, XCH         |    32K  ---       0x2000    4K   0x1000  1K
// ATxmega32A4      AVRxm                    DES, LAC, LAT, LAS, XCH         |    32K  ---       0x2000    4K   0x1000  1K
// ATxmega32A4U     AVRxm                                                    |    32K  ---       0x2000    4K   0x1000  1K
// ATxmega32C4      AVRxm                                                    |    32K  ---       0x2000    4K   0x1000  1K
// ATxmega32D4      AVRxm                    DES, LAC, LAT, LAS, XCH         |    32K  ---       0x2000    4K   0x1000  1K
// ATxmega32E5      AVRxm                                                    |    32K  ---       0x2000    4K   0x1000  1K
// ATxmega384C3     AVRxm                                                    |   384K  ---       0x2000   32K   0x1000  4K
// ATxmega384D3     AVRxm                    DES, LAC, LAT, LAS, XCH         |   384K  ---       0x2000   32K   0x1000  4K
// ATxmega64A1      AVRxm                         LAC, LAT, LAS, XCH         |    64K  ---       0x2000    4K   0x1000  2K
// ATxmega64A1U     AVRxm                                                    |    64K  ---       0x2000    4K   0x1000  2K
// ATxmega64A3      AVRxm                         LAC, LAT, LAS, XCH         |    64K  ---       0x2000    4K   0x1000  2K
// ATxmega64A3U     AVRxm                                                    |    64K  ---       0x2000    4K   0x1000  2K
// ATxmega64A4U     AVRxm                                                    |    64K  ---       0x2000    4K   0x1000  2K
// ATxmega64B1      AVRxm                                                    |    64K  ---       0x2000    8K   0x1000  2K
// ATxmega64B3      AVRxm                                                    |    64K  ---       0x2000    8K   0x1000  2K
// ATxmega64C3      AVRxm                                                    |    64K  ---       0x2000    4K   0x1000  2K
// ATxmega64D3      AVRxm                    DES, LAC, LAT, LAS, XCH         |    64K  ---       0x2000    4K   0x1000  2K
// ATxmega64D4      AVRxm                    DES, LAC, LAT, LAS, XCH         |    64K  ---       0x2000    4K   0x1000  2K
// ATxmega8E5       AVRxm                                                    |     8K  ---       0x2000    1K   0x1000  512
//
char* _ATxmega128A1   = "ATxmega128A1";
char* _ATxmega128A1U  = "ATxmega128A1U";
char* _ATxmega128A3   = "ATxmega128A3";
char* _ATxmega128A3U  = "ATxmega128A3U";
char* _ATxmega128A4U  = "ATxmega128A4U";
char* _ATxmega128B1   = "ATxmega128B1";
char* _ATxmega128B3   = "ATxmega128B3";
char* _ATxmega128C3   = "ATxmega128C3";
char* _ATxmega128D3   = "ATxmega128D3";
char* _ATxmega128D4   = "ATxmega128D4";
char* _ATxmega16A4    = "ATxmega16A4";
char* _ATxmega16A4U   = "ATxmega16A4U";
char* _ATxmega16C4    = "ATxmega16C4";
char* _ATxmega16D4    = "ATxmega16D4";
char* _ATxmega16E5    = "ATxmega16E5";
char* _ATxmega192A3   = "ATxmega192A3";
char* _ATxmega192A3U  = "ATxmega192A3U";
char* _ATxmega192C3   = "ATxmega192C3";
char* _ATxmega192D3   = "ATxmega192D3";
char* _ATxmega256A3B  = "ATxmega256A3B";
char* _ATxmega256A3BU = "ATxmega256A3BU";
char* _ATxmega256A3   = "ATxmega256A3";
char* _ATxmega256A3U  = "ATxmega256A3U";
char* _ATxmega256C3   = "ATxmega256C3";
char* _ATxmega256D3   = "ATxmega256D3";
char* _ATxmega32C3    = "ATxmega32C3";
char* _ATxmega32D3    = "ATxmega32D3";
char* _ATxmega32A4    = "ATxmega32A4";
char* _ATxmega32A4U   = "ATxmega32A4U";
char* _ATxmega32C4    = "ATxmega32C4";
char* _ATxmega32D4    = "ATxmega32D4";
char* _ATxmega32E5    = "ATxmega32E5";
char* _ATxmega384C3   = "ATxmega384C3";
char* _ATxmega384D3   = "ATxmega384D3";
char* _ATxmega64A1    = "ATxmega64A1";
char* _ATxmega64A1U   = "ATxmega64A1U";
char* _ATxmega64A3    = "ATxmega64A3";
char* _ATxmega64A3U   = "ATxmega64A3U";
char* _ATxmega64A4U   = "ATxmega64A4U";
char* _ATxmega64B1    = "ATxmega64B1";
char* _ATxmega64B3    = "ATxmega64B3";
char* _ATxmega64C3    = "ATxmega64C3";
char* _ATxmega64D3    = "ATxmega64D3";
char* _ATxmega64D4    = "ATxmega64D4";
char* _ATxmega8E5     = "ATxmega8E5";

// ---------------------------------------------------------------------------------------------------------------------------
// Automotive Devices                                                        |   FLASH FLASH     RAM      RAM   EEPROM  EEPROM
// Device           Core     Missing Instructions                            |   Size  Pagesize  Start    Size  Start   Size
// ---------------------------------------------------------------------------------------------------------------------------
// ATA5272          AVRe     ELPM,                CALL, JMP                  |
// ATA5505          AVRe                                                     |
// ATA5700M322      AVRe+    ELPM, EIJMP, EICALL                             |
// ATA5702M322      AVRe+    ELPM, EIJMP, EICALL                             |
// ATA5781          AVRe+    ELPM, EIJMP, EICALL                             |    20K  UHF ASK/FSK Receiver
// ATA5782          AVRe+    ELPM, EIJMP, EICALL                             |
// ATA5783          AVRe+    ELPM, EIJMP, EICALL                             |
// ATA5787          AVRe+    ELPM, EIJMP, EICALL                             |
// ATA5790N         AVRe+    ELPM, EIJMP, EICALL                             |
// ATA5790          AVRe+    ELPM, EIJMP, EICALL                             |
// ATA5791          AVRe+    ELPM, EIJMP, EICALL                             |
// ATA5795          AVRe+    ELPM, EIJMP, EICALL, CALL, JMP                  |
// ATA5831          AVRe+    ELPM, EIJMP, EICALL                             |
// ATA5832          AVRe+    ELPM, EIJMP, EICALL                             |
// ATA5833          AVRe+    ELPM, EIJMP, EICALL                             |
// ATA5835          AVRe+    ELPM, EIJMP, EICALL                             |
// ATA6285          AVRe+    ELPM, EIJMP, EICALL, CALL, JMP                  |
// ATA6286          AVRe+    ELPM, EIJMP, EICALL, CALL, JMP                  |
// ATA6612C         AVRe+    ELPM, EIJMP, EICALL, CALL, JMP                  |
// ATA6613C         AVRe+    ELPM, EIJMP, EICALL                             |
// ATA6614Q         AVRe+    ELPM, EIJMP, EICALL                             |
// ATA6616C         AVRe     ELPM,                CALL, JMP                  |
// ATA6617C         AVRe                                                     |
// ATA664251        AVRe                                                     |
// ATA8210          AVRe+    ELPM, EIJMP, EICALL                             |    20K  UHF ASK/FSK Receiver
// ATA8215          AVRe+    ELPM, EIJMP, EICALL                             |      0  UHF ASK/FSK Receiver
// ATA8510          AVRe+    ELPM, EIJMP, EICALL                             |    24K  ---       0x????    ?K   0x????  1K
// ATA8515          AVRe+    ELPM, EIJMP, EICALL                             |    24K  ---       0x????    ?K   0x????  1K
// ATtiny416auto    AVRxt    ELPM, EIJMP, EICALL, CALL, JMP, SPM, SPM Z+     |
//
char* _ATA5272       = "ATA5272";
char* _ATA5505       = "ATA5505";
char* _ATA5700M322   = "ATA5700M322";
char* _ATA5702M322   = "ATA5702M322";
char* _ATA5781       = "ATA5781";
char* _ATA5782       = "ATA5782";
char* _ATA5783       = "ATA5783";
char* _ATA5787       = "ATA5787";
char* _ATA5790N      = "ATA5790N";
char* _ATA5790       = "ATA5790";
char* _ATA5791       = "ATA5791";
char* _ATA5795       = "ATA5795";
char* _ATA5831       = "ATA5831";
char* _ATA5832       = "ATA5832";
char* _ATA5833       = "ATA5833";
char* _ATA5835       = "ATA5835";
char* _ATA6285       = "ATA6285";
char* _ATA6286       = "ATA6286";
char* _ATA6612C      = "ATA6612C";
char* _ATA6613C      = "ATA6613C";
char* _ATA6614Q      = "ATA6614Q";
char* _ATA6616C      = "ATA6616C";
char* _ATA6617C      = "ATA6617C";
char* _ATA664251     = "ATA664251";
char* _ATA8210       = "ATA8210";
char* _ATA8215       = "ATA8215";
char* _ATA8510       = "ATA8510";
char* _ATA8515       = "ATA8515";
char* _ATtiny416auto = "ATtiny416auto";
char* _ATtiny417auto = "ATtiny417auto";
char* _ATtiny816auto = "ATtiny816auto";
char* _ATtiny817auto = "ATtiny817auto";

// ---------------------------------------------------------------------------------------------------------------------------

// XASMAVR Extern variables and functions
extern void DebugStop(int, char*, char*); // Usage: DebugStop(testNr, "Function()", __FILE__);
extern void DebugPrintBuffer(char*, int); // Usage: DebugPrintBuffer(buffer, count);

extern ofstream LstFile;  // File write (*.LST)

extern void AssignPc();
extern void GetPcValue();
extern void newli(int);

// Forward declaration of functions included in this code module:
void WriteDeviceInfo(LPDEVICEAVR);
void ListDeviceAVRNoInstructions();

//-----------------------------------------------------------------------------
//
//                 Microchip available 'megaAVR® Devices'
//
// © 2021 Microchip Technology Inc. Manual DS40002198B-page 149++
//
// typedef struct tagDEVICEAVR {
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
DEVICEAVR deviceATmega[] = {
  {_AT90CAN128     ,  0x0000, 128*1024, 0x0100,  4*1024, 0x0000, 4*1024, _DES_|_XCHLAx_|_EIJMPCALL_},               
  {_AT90CAN32      ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_AT90CAN64      ,  0x0000,  64*1024, 0x0100,  4*1024, 0x0000, 2*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega1280     ,  0x0000, 128*1024, 0x0200,  8*1024, 0x0000, 4*1024, _DES_|_XCHLAx_|_EIJMPCALL_},               
  {_ATmega1281     ,  0x0000, 128*1024, 0x0200,  8*1024, 0x0000, 4*1024, _DES_|_XCHLAx_|_EIJMPCALL_},               
  {_ATmega1284P    ,  0x0000, 128*1024, 0x0100, 16*1024, 0x0000, 4*1024, _DES_|_XCHLAx_|_EIJMPCALL_},               
  {_ATmega1284RFR2 ,  0x0000, 128*1024, 0x0200, 16*1024, 0x0000, 4*1024, _DES_|_XCHLAx_|_EIJMPCALL_},               
  {_ATmega1284     ,  0x0000, 128*1024, 0x0100, 16*1024, 0x0000, 4*1024, _DES_|_XCHLAx_|_EIJMPCALL_},               
  {_ATmega128A     ,  0x0000, 128*1024, 0x0100,  4*1024, 0x0000, 4*1024, _DES_|_XCHLAx_|_EIJMPCALL_},               
  {_ATmega128RFA1  ,  0x0000, 128*1024, 0x0200, 16*1024, 0x0000, 4*1024, _DES_|_XCHLAx_|_EIJMPCALL_},               
  {_ATmega128RFR2  ,  0x0000, 128*1024, 0x0200, 16*1024, 0x0000, 4*1024, _DES_|_XCHLAx_|_EIJMPCALL_},               
  {_ATmega128      ,  0x0000, 128*1024, 0x0100,  4*1024, 0x0000, 4*1024, _DES_|_XCHLAx_|_EIJMPCALL_},               
  {_ATmega1608     ,  0x0000,  16*1024, 0x2800,  2*1024, 0x1400,    256, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_SPM_},  
  {_ATmega1609     ,  0x0000,  16*1024, 0x2800,  2*1024, 0x1400,    256, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_SPM_},  
  {_ATmega162      ,  0x0000,  16*1024, 0x0060,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega164A     ,  0x0000,  16*1024, 0x0100,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega164PA    ,  0x0000,  16*1024, 0x0100,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega164P     ,  0x0000,  16*1024, 0x0100,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega165A     ,  0x0000,  16*1024, 0x0100,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega165PA    ,  0x0000,  16*1024, 0x0100,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega165P     ,  0x0000,  16*1024, 0x0100,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega168A     ,  0x0000,  16*1024, 0x0100,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega168PA    ,  0x0000,  16*1024, 0x0100,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega168PB    ,  0x0000,  16*1024, 0x0100,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega168P     ,  0x0000,  16*1024, 0x0100,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega168      ,  0x0000,  16*1024, 0x0100,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega169A     ,  0x0000,  16*1024, 0x0100,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega169PA    ,  0x0000,  16*1024, 0x0100,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega169P     ,  0x0000,  16*1024, 0x0100,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega16A      ,  0x0000,  16*1024, 0x0060,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega16HVA    ,  0x0000,  16*1024, 0x0100,     512, 0x0000,    256, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega16HVB    ,  0x0000,  16*1024, 0x0100,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega16HVBrevB,  0x0000,  16*1024, 0x0100,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega16M1     ,  0x0000,  16*1024, 0x0100,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega16U2     ,  0x0000,  16*1024, 0x0100,     512, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega16U4     ,  0x0000,  16*1024, 0x0100,    1280, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega16       ,  0x0000,  16*1024, 0x0060,  1*1024, 0x0000,    512, _DES_|_XCHLAx_},                           
  {_ATmega2560     ,  0x0000, 256*1024, 0x0200,  8*1024, 0x0000, 4*1024, _DES_|_XCHLAx_},                           
  {_ATmega2561     ,  0x0000, 256*1024, 0x0200,  8*1024, 0x0000, 4*1024, _DES_|_XCHLAx_},                           
  {_ATmega2564RFR2 ,  0x0000, 256*1024, 0x0200, 32*1024, 0x0000, 8*1024, _DES_|_XCHLAx_},                           
  {_ATmega256RFR2  ,  0x0000, 256*1024, 0x0200, 32*1024, 0x0000, 8*1024, _DES_|_XCHLAx_},                           
  {_ATmega3208     ,  0x0000,  32*1024, 0x2800,  4*1024, 0x1400,    256, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_SPM_},  
  {_ATmega3209     ,  0x0000,  32*1024, 0x2800,  4*1024, 0x1400,    256, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_SPM_},  
  {_ATmega324A     ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega324PA    ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega324PB    ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega324P     ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega3250A    ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega3250PA   ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega3250P    ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega3250     ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega325A     ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega325PA    ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega325P     ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega325      ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega328PB    ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega328P     ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega328      ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega3290A    ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega3290PA   ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega3290P    ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000, 2*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega3290     ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega329A     ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega329PA    ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega329P     ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega329      ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega32A      ,  0x0000,  32*1024, 0x0060,  2*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega32C1     ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega32HVB    ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega32HVBrevB,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega32M1     ,  0x0000,  32*1024, 0x0100,  2*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega32U2     ,  0x0000,  32*1024, 0x0100,  1*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega32U4     ,  0x0000,  32*1024, 0x0100,    2560, 0x0000,   1280, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega32       ,  0x0000,  32*1024, 0x0060,  2*1024, 0x0000, 1*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega406      ,  0x0000,   4*1024, 0x0100,  2*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega4808     ,  0x0000,  48*1024, 0x2800,  6*1024, 0x1400,    256, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_SPM_},  
  {_ATmega4809     ,  0x0000,  48*1024, 0x2800,  6*1024, 0x1400,    256, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_SPM_},  
  {_ATmega48A      ,  0x0000,   4*1024, 0x0100,     512, 0x0000,    256, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega48PA     ,  0x0000,   4*1024, 0x0100,     512, 0x0000,    256, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega48PB     ,  0x0000,   4*1024, 0x0100,     512, 0x0000,    256, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega48P      ,  0x0000,   4*1024, 0x0100,     512, 0x0000,    256, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},        
  {_ATmega48       ,  0x0000,   4*1024, 0x0100,     512, 0x0000,    256, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_ATmega640      ,  0x0000,  64*1024, 0x0200,  8*1024, 0x0000, 4*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_ATmega644A     ,  0x0000,  64*1024, 0x0100,  4*1024, 0x0000, 2*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_ATmega644PA    ,  0x0000,  64*1024, 0x0100,  4*1024, 0x0000, 2*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_ATmega644P     ,  0x0000,  64*1024, 0x0100,  4*1024, 0x0000, 2*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_ATmega644RFR2  ,  0x0000,  64*1024, 0x0200,  8*1024, 0x0000, 2*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_ATmega644      ,  0x0000,  64*1024, 0x0100,  4*1024, 0x0000, 2*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_ATmega6450A    ,  0x0000,  64*1024, 0x0100,  4*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_ATmega6450P    ,  0x0000,  64*1024, 0x0100,  4*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_ATmega6450     ,  0x0000,  64*1024, 0x0100,  4*1024, 0x0000, 2*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_ATmega645A     ,  0x0000,  64*1024, 0x0100,  4*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_ATmega645P     ,  0x0000,  64*1024, 0x0100,  4*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_ATmega645      ,  0x0000,  64*1024, 0x0100,  4*1024, 0x0000, 2*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_ATmega6490A    ,  0x0000,  64*1024, 0x0100,  4*1024, 0x0000, 2*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_ATmega6490P    ,  0x0000,  64*1024, 0x0100,  4*1024, 0x0000, 2*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_ATmega6490     ,  0x0000,  64*1024, 0x0100,  4*1024, 0x0000, 2*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_ATmega649A     ,  0x0000,  64*1024, 0x0100,  4*1024, 0x0000, 2*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_ATmega649P     ,  0x0000,  64*1024, 0x0100,  4*1024, 0x0000, 2*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_ATmega649      ,  0x0000,  64*1024, 0x0100,  4*1024, 0x0000, 2*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_ATmega64A      ,  0x0000,  64*1024, 0x0100,  4*1024, 0x0000, 2*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_ATmega64C1     ,  0x0000,  64*1024, 0x0100,  4*1024, 0x0000, 2*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_ATmega64HVE2   ,  0x0000,  64*1024, 0x0100,  4*1024, 0x0000, 2*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_ATmega64M1     ,  0x0000,  64*1024, 0x0100,  4*1024, 0x0000, 2*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_ATmega64RFR2   ,  0x0000,  64*1024, 0x0200,  8*1024, 0x0000, 2*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_ATmega64       ,  0x0000,  64*1024, 0x0060,  4*1024, 0x0000, 2*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_ATmega808      ,  0x0000,   8*1024, 0x2800,  1*1024, 0x1400,    256, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_|_SPM_},
  {_ATmega809      ,  0x0000,   8*1024, 0x2800,  1*1024, 0x1400,    256, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_|_SPM_},
  {_ATmega8515     ,  0x0000,   8*1024, 0x0060,     512, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_|_BREAK_},
  {_ATmega8535     ,  0x0000,   8*1024, 0x0060,     512, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_|_BREAK_},
  {_ATmega88A      ,  0x0000,   8*1024, 0x0100,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_},
  {_ATmega88PA     ,  0x0000,   8*1024, 0x0100,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_},
  {_ATmega88PB     ,  0x0000,   8*1024, 0x0100,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_},
  {_ATmega88P      ,  0x0000,   8*1024, 0x0100,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_},
  {_ATmega88       ,  0x0000,   8*1024, 0x0100,     512, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_},
  {_ATmega8A       ,  0x0000,   8*1024, 0x0060,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_|_BREAK_},
  {_ATmega8HVA     ,  0x0000,   8*1024, 0x0100,     512, 0x0000,    256, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_},
  {_ATmega8U2      ,  0x0000,   8*1024, 0x0100,     512, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_},
  {_ATmega8        ,  0x0000,   8*1024, 0x0060,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_|_BREAK_},
  {_AT90PWM161     ,  0x0000,  16*1024, 0x0100,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_},
  {_AT90PWM1       ,  0x0000,   8*1024, 0x0100,     512, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_},
  {_AT90PWM216     ,  0x0000,  16*1024, 0x0100,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_AT90PWM2B      ,  0x0000,   8*1024, 0x0100,     512, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_},
  {_AT90PWM316     ,  0x0000,  16*1024, 0x0100,  1*1024, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_AT90PWM3B      ,  0x0000,   8*1024, 0x0100,     512, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_},
  {_AT90PWM81      ,  0x0000,   8*1024, 0x0100,     256, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_},
  {_AT90USB1286    ,  0x0000, 128*1024, 0x0100,  8*1024, 0x0000, 4*1024, _DES_|_XCHLAx_|_EIJMPCALL_},
  {_AT90USB1287    ,  0x0000, 128*1024, 0x0100,  5*1024, 0x0000, 4*1024, _DES_|_XCHLAx_|_EIJMPCALL_},
  {_AT90USB162     ,  0x0000,  16*1024, 0x0100,     512, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_AT90USB646     ,  0x0000,  64*1024, 0x0100,  4*1024, 0x0000, 2*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_AT90USB647     ,  0x0000,  64*1024, 0x0100,  4*1024, 0x0000, 2*1024, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_},
  {_AT90USB82      ,  0x0000,   8*1024, 0x0100,     512, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_},

  {_AT90S1200      ,  0x0000,     1024, 0x0000,       0, 0x0000,     64, _ALL_},
  // AT90S2313 is replaced by ATtiny2313
  {_AT90S2313      ,  0x0000,   2*1024, 0x0060,     128, 0x0000,    128, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_|_MOVW_|_MUL_|_SPM_|_BREAK_},
  {_AT90S2323      ,  0x0000,   2*1024, 0x0060,     128, 0x0000,    128, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_|_MOVW_|_MUL_|_SPM_|_BREAK_},
  {_AT90S2333      ,  0x0000,   2*1024, 0x0060,     128, 0x0000,    128, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_|_MOVW_|_MUL_|_SPM_|_BREAK_},
  {_AT90S2343      ,  0x0000,   2*1024, 0x0060,     128, 0x0000,    128, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_|_MOVW_|_MUL_|_SPM_|_BREAK_},
  {_AT90S4414      ,  0x0000,   4*1024, 0x0060,     256, 0x0000,    256, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_|_MOVW_|_MUL_|_SPM_|_BREAK_},
  {_AT90S4433      ,  0x0000,   4*1024, 0x0060,     128, 0x0000,    256, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_|_MOVW_|_MUL_|_SPM_|_BREAK_},
  {_AT90S4434      ,  0x0000,   4*1024, 0x0060,     256, 0x0000,    256, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_|_MOVW_|_MUL_|_SPM_|_BREAK_},
  {_AT90S8515      ,  0x0000,   8*1024, 0x0060,     512, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_|_MOVW_|_MUL_|_SPM_|_BREAK_},
  {_AT90C8534      ,  0x0000,   8*1024, 0x0060,     256, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_|_MOVW_|_MUL_|_SPM_|_BREAK_},
  {_AT90S8535      ,  0x0000,   8*1024, 0x0060,     512, 0x0000,    512, _DES_|_XCHLAx_|_EIJMPCALL_|_ELPM_|_JMPCALL_|_MOVW_|_MUL_|_SPM_|_BREAK_},
  // Device END:
  {NULL,                   0,        0,      0,       0,      0,      0, _NONE_}
  }; // end of deviceATmega structure

LPDEVICEAVR deviceATmegaPtr = deviceATmega; // Pointer to deviceATmega structure

//-----------------------------------------------------------------------------
//
//                 Microchip available 'tinyAVR® Devices'
//
// © 2021 Microchip Technology Inc. Manual DS40002198B-page 149++
//
// typedef struct tagDEVICEAVR {
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
DEVICEAVR deviceTiny[] = {
  {_ATtiny102      ,  0x0000,   1*1024, 0x0040,      32,      0,      0, _ALL_ & ~(_ADIWSBIW_|_IJMPCALL_|_PUSHPOP_|_LPM_)}, 
  {_ATtiny104      ,  0x0000,   1*1024, 0x0040,      32,      0,      0, _ALL_ & ~(_ADIWSBIW_|_IJMPCALL_|_PUSHPOP_|_LPM_)}, 
  {_ATtiny10       ,  0x0000,   1*1024, 0x0040,      32,      0,      0, (_ALL_|_ADIWSBIW_) & ~_IJMPCALL_}, 
  {_ATtiny11       ,  0x0000,   1*1024,      0,       0,      0,      0, _ALL_ & ~_LPM_}, 
  {_ATtiny12       ,  0x0000,   1*1024,      0,       0,      0,     64, _ALL_ & ~_LPM_}, 
  {_ATtiny13A      ,  0x0000,   1*1024, 0x0060,      64,      0,     64, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny13       ,  0x0000,   1*1024, 0x0060,      64,      0,     64, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny15       ,  0x0000,   1*1024,      0,       0,      0,     64, _ALL_ & ~_LPM_}, 
  {_ATtiny1604     ,  0x8000,  16*1024, 0x3C00,  1*1024, 0x1400,    256, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny1606     ,  0x8000,  16*1024, 0x3C00,  1*1024, 0x1400,    256, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny1607     ,  0x8000,  16*1024, 0x3C00,  1*1024, 0x1400,    256, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny1614     ,  0x8000,  16*1024, 0x3800,  2*1024, 0x1400,    256, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny1616     ,  0x8000,  16*1024, 0x3800,  2*1024, 0x1400,    256, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny1617     ,  0x8000,  16*1024, 0x3800,  2*1024, 0x1400,    256, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny1624     ,  0x8000,  16*1024, 0x3400,  3*1024, 0x1400,    256, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny1626     ,  0x8000,  16*1024, 0x3400,  3*1024, 0x1400,    256, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny1627     ,  0x8000,  16*1024, 0x3400,  3*1024, 0x1400,    256, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny1634     ,  0x0000,  16*1024, 0x0100,  1*1024,      0,    256, _DES_|_XCHLAx_}, 
  {_ATtiny167      ,  0x0000,  16*1024, 0x0100,     512,      0,    512, _DES_|_XCHLAx_}, 
  {_ATtiny202      ,  0x8000,   2*1024, 0x3F80,     128, 0x1400,     64, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny204      ,  0x8000,   2*1024, 0x3F80,     128, 0x1400,     64, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny20       ,  0x0000,   2*1024, 0x0040,     128,      0,      0, _DES_|_XCHLAx_|_BREAK_}, 
  {_ATtiny212      ,  0x8000,   2*1024, 0x3F80,     128, 0x1400,     64, _ALL_}, // UPDI instruction set.
  {_ATtiny214      ,  0x8000,   2*1024, 0x3F80,     128, 0x1400,     64, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_SPM_}, 
  {_ATtiny22       ,  0x0000,   2*1024, 0x0060,     128,      0,    128, _ALL_ & ~(_ADIWSBIW_|_IJMPCALL_|_PUSHPOP_|_LPM_)}, 
  {_ATtiny2313A    ,  0x0000,   2*1024, 0x0060,     128,      0,    128, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny2313     ,  0x0000,   2*1024, 0x0060,     128,      0,    128, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny24A      ,  0x0000,   2*1024, 0x0060,     128,      0,    128, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny24       ,  0x0000,   2*1024, 0x0060,     128,      0,    128, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny25       ,  0x0000,   2*1024, 0x0060,     128,      0,    128, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny261A     ,  0x0000,   2*1024, 0x0060,     128,      0,    128, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny261      ,  0x0000,   2*1024, 0x0060,     128,      0,    128, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny26       ,  0x0000,   2*1024, 0x0060,     128,      0,    128, _DES_|_XCHLAx_|_BREAK_}, 
  {_ATtiny28       ,  0x0000,   2*1024,      0,       0,      0,      0, _ALL_ & ~_LPM_}, 
  {_ATtiny3216     ,  0x0000,  32*1024, 0x0060,  2*1024,      0,    256, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_SPM_}, 
  {_ATtiny3217     ,  0x0000,  32*1024, 0x0060,  2*1024,      0,    256, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_SPM_}, 
  {_ATtiny3224     ,  0x8000,  32*1024, 0x3400,  3*1024, 0x1400,    256, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_SPM_}, 
  {_ATtiny3226     ,  0x8000,  32*1024, 0x3400,  3*1024, 0x1400,    256, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_SPM_}, 
  {_ATtiny3227     ,  0x8000,  32*1024, 0x3400,  3*1024, 0x1400,    256, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_SPM_}, 
  {_ATtiny402      ,  0x8000,   4*1024, 0x3F00,     256, 0x1400,    128, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny404      ,  0x8000,   4*1024, 0x3F00,     256, 0x1400,    128, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny406      ,  0x8000,   4*1024, 0x3F00,     256, 0x1400,    128, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny40       ,  0x0000,   4*1024, 0x0100,      64,      0,      0, _ALL_ & ~(_IJMPCALL_|_PUSHPOP_)}, 
  {_ATtiny412      ,  0x8000,   4*1024, 0x3F00,     256, 0x1400,    128, _ALL_}, // UPDI instruction set.
  {_ATtiny414      ,  0x8000,   4*1024, 0x3F00,     256, 0x1400,    128, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_SPM_}, 
  {_ATtiny416      ,  0x8000,   4*1024, 0x3F00,     256, 0x1400,    128, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny417      ,  0x8000,   4*1024, 0x3F00,     256, 0x1400,    128, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny424      ,  0x8000,   4*1024, 0x3E00,     512, 0x1400,    128, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny426      ,  0x8000,   4*1024, 0x3E00,     512, 0x1400,    128, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny427      ,  0x8000,   4*1024, 0x3E00,     512, 0x1400,    128, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny4313     ,  0x0000,   4*1024, 0x0060,     256,      0,    256, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny43U      ,  0x0000,   4*1024, 0x0060,     256,      0,     64, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny441      ,  0x0000,   4*1024, 0x0100,     256,      0,    256, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny44A      ,  0x0000,   4*1024, 0x0060,     256,      0,    256, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny44       ,  0x0000,   4*1024, 0x0060,     256,      0,    256, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny45       ,  0x0000,   4*1024, 0x0060,     256,      0,    256, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny461A     ,  0x0000,   4*1024, 0x0060,     256,      0,    256, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny461      ,  0x0000,   4*1024, 0x0060,     256,      0,    256, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny48       ,  0x0000,   4*1024, 0x0010,     256,      0,     64, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny4        ,  0x0000,      512, 0x0040,      32,      0,      0, _DES_|_XCHLAx_|_BREAK_}, 
  {_ATtiny5        ,  0x0000,      512, 0x0040,      32,      0,      0, _DES_|_XCHLAx_|_BREAK_}, 
  {_ATtiny804      ,  0x8000,   8*1024, 0x3E00,     512, 0x1400,    128, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny806      ,  0x8000,   8*1024, 0x3E00,     512, 0x1400,    128, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny807      ,  0x8000,   8*1024, 0x3E00,     512, 0x1400,    128, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny814      ,  0x8000,   8*1024, 0x3E00,     512, 0x1400,    128, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny816      ,  0x8000,   8*1024, 0x3E00,     512, 0x1400,    128, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny817      ,  0x8000,   8*1024, 0x3E00,     512, 0x1400,    128, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny824      ,  0x8000,   8*1024, 0x3C00,  1*1024, 0x1400,    128, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny826      ,  0x8000,   8*1024, 0x3C00,  1*1024, 0x1400,    128, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny827      ,  0x8000,   8*1024, 0x3C00,  1*1024, 0x1400,    128, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_|_JMPCALL_|_SPM_}, 
  {_ATtiny828      ,  0x0000,   8*1024, 0x0100,     512,      0,    256, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny841      ,  0x0000,   8*1024, 0x0100,     512,      0,    256, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny84A      ,  0x0000,   8*1024, 0x0060,     512,      0,    512, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny84       ,  0x0000,   8*1024, 0x0060,     512,      0,    512, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny85       ,  0x0000,   8*1024, 0x0060,     512,      0,    512, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny861A     ,  0x0000,   8*1024, 0x0060,     512,      0,    512, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny861      ,  0x0000,   8*1024, 0x0060,     512,      0,    512, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny87       ,  0x0000,   8*1024, 0x0100,     512,      0,    512, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny88       ,  0x0000,   8*1024, 0x0100,     512,      0,     64, _DES_|_XCHLAx_|_ELPM_|_JMPCALL_}, 
  {_ATtiny9        ,  0x0000,   1*1024, 0x0040,      32,      0,      0, _DES_|_XCHLAx_|_BREAK_}, 
  {_ATtiny416auto  ,  0x8000,   4*1024, 0x3F00,     256, 0x1400,    128, _ALL_}, // UPDI instruction set. AVR uC Advanced Peripherals
  {_ATtiny417auto  ,  0x8000,   4*1024, 0x3F00,     256, 0x1400,    128, _ALL_}, // UPDI instruction set. AVR uC Advanced Peripherals
  {_ATtiny816auto  ,  0x8000,   8*1024, 0x3E00,     512, 0x1400,    128, _ALL_}, // UPDI instruction set. AVR uC Advanced Peripherals
  {_ATtiny817auto  ,  0x8000,   8*1024, 0x3E00,     512, 0x1400,    128, _ALL_}, // UPDI instruction set. AVR uC Advanced Peripherals
  // device END:       
  {NULL,                   0,        0,      0,       0,      0,      0, _NONE_}
  }; // end of deviceTiny structure

LPDEVICEAVR deviceTinyPtr = deviceTiny; // Pointer to deviceTiny structure

//-----------------------------------------------------------------------------
//
//                 Microchip available 'AVR® Dx Devices'
//
// © 2021 Microchip Technology Inc. Manual DS40002198B-page 149++
//
// typedef struct tagDEVICEAVR {
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
DEVICEAVR deviceAVRDx[] = {
  {_AVR128DA28     ,  0x0000, 128*1024, 0x4000,  16*102, 0x1400,    512, _DES_|_XCHLAx_|_EIJMPCALL_},
  {_AVR128DA32     ,  0x0000, 128*1024, 0x4000,  16*102, 0x1400,    512, _DES_|_XCHLAx_|_EIJMPCALL_},
  {_AVR128DA48     ,  0x0000, 128*1024, 0x4000,  16*102, 0x1400,    512, _DES_|_XCHLAx_|_EIJMPCALL_},
  {_AVR128DA64     ,  0x0000, 128*1024, 0x4000,  16*102, 0x1400,    512, _DES_|_XCHLAx_|_EIJMPCALL_},
  {_AVR32DA28      ,  0x0000,  32*1024, 0x7000,   4*102, 0x1400,    512, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_},
  {_AVR32DA32      ,  0x0000,  32*1024, 0x7000,   4*102, 0x1400,    512, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_},
  {_AVR32DA48      ,  0x0000,  32*1024, 0x7000,   4*102, 0x1400,    512, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_},
  {_AVR64DA28      ,  0x0000,  64*1024, 0x6000,   8*102, 0x1400,    512, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_},
  {_AVR64DA32      ,  0x0000,  64*1024, 0x6000,   8*102, 0x1400,    512, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_},
  {_AVR64DA48      ,  0x0000,  64*1024, 0x6000,   8*102, 0x1400,    512, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_},
  {_AVR64DA64      ,  0x0000,  64*1024, 0x6000,   8*102, 0x1400,    512, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_},
  {_AVR128DB28     ,  0x0000, 128*1024, 0x4000,  16*102, 0x1400,    512, _DES_|_XCHLAx_|_EIJMPCALL_},
  {_AVR128DB32     ,  0x0000, 128*1024, 0x4000,  16*102, 0x1400,    512, _DES_|_XCHLAx_|_EIJMPCALL_},
  {_AVR128DB48     ,  0x0000, 128*1024, 0x4000,  16*102, 0x1400,    512, _DES_|_XCHLAx_|_EIJMPCALL_},
  {_AVR128DB64     ,  0x0000, 128*1024, 0x4000,  16*102, 0x1400,    512, _DES_|_XCHLAx_|_EIJMPCALL_},
  {_AVR32DB28      ,  0x0000,  32*1024, 0x7000,   4*102, 0x1400,    512, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_},
  {_AVR32DB32      ,  0x0000,  32*1024, 0x7000,   4*102, 0x1400,    512, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_},
  {_AVR32DB48      ,  0x0000,  32*1024, 0x7000,   4*102, 0x1400,    512, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_},
  {_AVR64DB28      ,  0x0000,  64*1024, 0x6000,   8*102, 0x1400,    512, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_},
  {_AVR64DB32      ,  0x0000,  64*1024, 0x6000,   8*102, 0x1400,    512, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_},
  {_AVR64DB48      ,  0x0000,  64*1024, 0x6000,   8*102, 0x1400,    512, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_},
  {_AVR64DB64      ,  0x0000,  64*1024, 0x6000,   8*102, 0x1400,    512, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_},
  {_AVR128DD28     ,  0x0000, 128*1024, 0x4000,  16*102, 0x1400,    512, _DES_|_XCHLAx_|_EIJMPCALL_},
  {_AVR128DD32     ,  0x0000, 128*1024, 0x4000,  16*102, 0x1400,    512, _DES_|_XCHLAx_|_EIJMPCALL_},
  {_AVR128DD48     ,  0x0000, 128*1024, 0x4000,  16*102, 0x1400,    512, _DES_|_XCHLAx_|_EIJMPCALL_},
  {_AVR128DD64     ,  0x0000, 128*1024, 0x4000,  16*102, 0x1400,    512, _DES_|_XCHLAx_|_EIJMPCALL_},
  {_AVR32DD28      ,  0x0000,  32*1024, 0x7000,   4*102, 0x1400,    512, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_},
  {_AVR32DD32      ,  0x0000,  32*1024, 0x7000,   4*102, 0x1400,    512, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_},
  {_AVR32DD48      ,  0x0000,  32*1024, 0x7000,   4*102, 0x1400,    512, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_},
  {_AVR64DD28      ,  0x0000,  64*1024, 0x6000,   8*102, 0x1400,    512, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_},
  {_AVR64DD32      ,  0x0000,  64*1024, 0x6000,   8*102, 0x1400,    512, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_},
  {_AVR64DD48      ,  0x0000,  64*1024, 0x6000,   8*102, 0x1400,    512, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_},
  {_AVR64DD64      ,  0x0000,  64*1024, 0x6000,   8*102, 0x1400,    512, _DES_|_XCHLAx_|_ELPM_|_EIJMPCALL_},
  // device END:                                         
  {NULL,                   0,        0,      0,       0,      0,      0, _NONE_}
  }; // end of deviceAVRDx structure

LPDEVICEAVR deviceAVRDxPtr = deviceAVRDx; // Pointer to deviceAVRDx structure

//-----------------------------------------------------------------------------
//
//                 Microchip available 'XMEGA® Devices'
//
// © 2021 Microchip Technology Inc. Manual DS40002198B-page 149++
//
// typedef struct tagDEVICEAVR {
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
DEVICEAVR deviceXMEGA[] = {
  {_ATxmega128A1   ,  0x0000, 128*1024, 0x2000,  8*1024, 0x1000, 2*1024, _XCHLAx_}, 
  {_ATxmega128A1U  ,  0x0000, 128*1024, 0x2000,  8*1024, 0x1000, 2*1024, _NONE_},
  {_ATxmega128A3   ,  0x0000, 128*1024, 0x2000,  8*1024, 0x1000, 2*1024, _XCHLAx_},
  {_ATxmega128A3U  ,  0x0000, 128*1024, 0x2000,  8*1024, 0x1000, 2*1024, _NONE_},
  {_ATxmega128A4U  ,  0x0000, 128*1024, 0x2000,  8*1024, 0x1000, 2*1024, _NONE_},
  {_ATxmega128B1   ,  0x0000, 128*1024, 0x2000,  8*1024, 0x1000, 2*1024, _NONE_},
  {_ATxmega128B3   ,  0x0000, 128*1024, 0x2000,  4*1024, 0x1000, 2*1024, _NONE_},
  {_ATxmega128C3   ,  0x0000, 128*1024, 0x2000,  8*1024, 0x1000, 2*1024, _NONE_},
  {_ATxmega128D3   ,  0x0000, 128*1024, 0x2000,  8*1024, 0x1000, 2*1024, _DES_|_XCHLAx_},
  {_ATxmega128D4   ,  0x0000, 128*1024, 0x2000,  8*1024, 0x1000, 2*1024, _DES_|_XCHLAx_},
  {_ATxmega16A4    ,  0x0000,  16*1024, 0x2000,  2*1024, 0x1000, 1*1024, _DES_|_XCHLAx_},
  {_ATxmega16A4U   ,  0x0000,  16*1024, 0x2000,  2*1024, 0x1000, 1*1024, _NONE_},
  {_ATxmega16C4    ,  0x0000,  16*1024, 0x2000,  2*1024, 0x1000, 1*1024, _NONE_},
  {_ATxmega16D4    ,  0x0000,  16*1024, 0x2000,  2*1024, 0x1000, 1*1024, _DES_|_XCHLAx_},
  {_ATxmega16E5    ,  0x0000,  16*1024, 0x2000,  2*1024, 0x1000,    512, _NONE_},
  {_ATxmega192A3   ,  0x0000, 192*1024, 0x2000, 16*1024, 0x1000, 2*1024, _XCHLAx_},
  {_ATxmega192A3U  ,  0x0000, 192*1024, 0x2000, 16*1024, 0x1000, 2*1024, _NONE_},
  {_ATxmega192C3   ,  0x0000, 192*1024, 0x2000, 16*1024, 0x1000, 2*1024, _NONE_},
  {_ATxmega192D3   ,  0x0000, 192*1024, 0x2000, 16*1024, 0x1000, 2*1024, _DES_|_XCHLAx_},
  {_ATxmega256A3B  ,  0x0000, 256*1024, 0x2000, 16*1024, 0x1000, 4*1024, _DES_|_XCHLAx_},
  {_ATxmega256A3BU ,  0x0000, 256*1024, 0x2000, 16*1024, 0x1000, 4*1024, _DES_},
  {_ATxmega256A3   ,  0x0000, 256*1024, 0x2000, 16*1024, 0x1000, 4*1024, _XCHLAx_},
  {_ATxmega256A3U  ,  0x0000, 256*1024, 0x2000, 16*1024, 0x1000, 4*1024, _NONE_},
  {_ATxmega256C3   ,  0x0000, 256*1024, 0x2000, 16*1024, 0x1000, 4*1024, _NONE_},
  {_ATxmega256D3   ,  0x0000, 256*1024, 0x2000, 16*1024, 0x1000, 4*1024, _DES_|_XCHLAx_},
  {_ATxmega32C3    ,  0x0000,  32*1024, 0x2000,  4*1024, 0x1000, 1*1024, _NONE_},
  {_ATxmega32D3    ,  0x0000,  32*1024, 0x2000,  4*1024, 0x1000, 1*1024, _DES_|_XCHLAx_},
  {_ATxmega32A4    ,  0x0000,  32*1024, 0x2000,  4*1024, 0x1000, 1*1024, _DES_|_XCHLAx_},
  {_ATxmega32A4U   ,  0x0000,  32*1024, 0x2000,  4*1024, 0x1000, 1*1024, _NONE_},
  {_ATxmega32C4    ,  0x0000,  32*1024, 0x2000,  4*1024, 0x1000, 1*1024, _NONE_},
  {_ATxmega32D4    ,  0x0000,  32*1024, 0x2000,  4*1024, 0x1000, 1*1024, _DES_|_XCHLAx_},
  {_ATxmega32E5    ,  0x0000,  32*1024, 0x2000,  4*1024, 0x1000, 1*1024, _NONE_},
  {_ATxmega384C3   ,  0x0000, 384*1024, 0x2000, 32*1024, 0x1000, 4*1024, _NONE_},
  {_ATxmega384D3   ,  0x0000, 384*1024, 0x2000, 32*1024, 0x1000, 4*1024, _DES_|_XCHLAx_},
  {_ATxmega64A1    ,  0x0000,  64*1024, 0x2000,  4*1024, 0x1000, 2*1024, _XCHLAx_},
  {_ATxmega64A1U   ,  0x0000,  64*1024, 0x2000,  4*1024, 0x1000, 2*1024, _NONE_},
  {_ATxmega64A3    ,  0x0000,  64*1024, 0x2000,  4*1024, 0x1000, 2*1024, _XCHLAx_},
  {_ATxmega64A3U   ,  0x0000,  64*1024, 0x2000,  4*1024, 0x1000, 2*1024, _NONE_},
  {_ATxmega64A4U   ,  0x0000,  64*1024, 0x2000,  4*1024, 0x1000, 2*1024, _NONE_},
  {_ATxmega64B1    ,  0x0000,  64*1024, 0x2000,  8*1024, 0x1000, 2*1024, _NONE_},
  {_ATxmega64B3    ,  0x0000,  64*1024, 0x2000,  8*1024, 0x1000, 2*1024, _NONE_},
  {_ATxmega64C3    ,  0x0000,  64*1024, 0x2000,  4*1024, 0x1000, 2*1024, _NONE_},
  {_ATxmega64D3    ,  0x0000,  64*1024, 0x2000,  4*1024, 0x1000, 2*1024, _DES_|_XCHLAx_},
  {_ATxmega64D4    ,  0x0000,  64*1024, 0x2000,  4*1024, 0x1000, 2*1024, _DES_|_XCHLAx_},
  {_ATxmega8E5     ,  0x0000,   8*1024, 0x2000,  1*1024, 0x1000,    512, _NONE_},
  // Device END:
  {NULL,                   0,        0,      0,       0,      0,      0, _NONE_}
  }; // end of deviceXMEGA structure

LPDEVICEAVR deviceXMEGAPtr = deviceXMEGA; // Pointer to deviceXMEGA structure

//-----------------------------------------------------------------------------
//
//                 Microchip available 'Automotive Devices'
//
// © 2021 Microchip Technology Inc. Manual DS40002198B-page 149++
//
// typedef struct tagDEVICEAVR {
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
DEVICEAVR deviceAutomotive[] = {
//{_ATA5272        ,       0,        0,      0,       0,      0,      0, _ALL_},   // Device is indeterminable
  {_ATA5505        ,  0x0000,  16*1024, 0x0100,     512, 0x0000,    512, _ALL_},   // AVR+25kHz LF Reader/Writer
  {_ATA5700M322    ,  0x0000,  32*1024,      0,       0,      0, 2*1056, _ALL_},   // 3D LF/UHF Transceiver
  {_ATA5702M322    ,  0x0000,  32*1024,      0,       0,      0, 2*1056, _ALL_},   // 3D LF/UHF Transceiver
  {_ATA5781        ,       0,        0,      0,       0,      0,      0, _ALL_},   // UHF ASK/FSK Receiver
  {_ATA5782        ,  0x0000,  20*1024,      0,       0,      0,      0, _ALL_},   // UHF ASK/FSK Receiver
  {_ATA5783        ,  0x0000,  20*1024,      0,       0,      0,      0, _ALL_},   // UHF ASK/FSK Receiver
  {_ATA5787        ,       0,        0,      0,       0,      0,      0, _ALL_},   // RF transmitter
  {_ATA5790N       ,  0x0000,  16*1024, 0x0000,     512, 0x0000, 2*1056, _ALL_},   // AVR+LF Passive Entry/Start Keys
  {_ATA5790        ,  0x0000,  16*1024, 0x0000,     512, 0x0000, 2*1056, _ALL_},   // AVR+LF Passive Entry/Start Keys
//{_ATA5791        ,       0,        0,      0,       0,      0,      0, _ALL_},   // Device is no longer in production
  {_ATA5795        ,  0x0000,   8*1024, 0x0000,     512, 0x0000, 2*1056, _ALL_},   // AVR+RF/LF Remote Keyless
  {_ATA5831        ,  0x0000,  20*1024,      0,       0,      0,      0, _ALL_},   // ASK/FSK Transceiver
  {_ATA5832        ,  0x0000,  20*1024,      0,       0,      0,      0, _ALL_},   // ASK/FSK Transceiver
  {_ATA5833        ,  0x0000,  20*1024,      0,       0,      0,      0, _ALL_},   // ASK/FSK Transceiver
  {_ATA5835        ,       0,        0,      0,       0,      0,      0, _ALL_},   // RF transmitter
  {_ATA6285        ,  0x0000,   8*1024, 0x0000,     512, 0x0000,    320, _ALL_},   // ASK/FSK Transmitter
  {_ATA6286        ,  0x0000,   8*1024, 0x0000,     512, 0x0000,    320, _ALL_},   // ASK/FSK Transmitter
  {_ATA6612C       ,  0x0000,   8*1024,      0,       0,      0,      0, _ALL_},   // AVR+LIN Transceiver
  {_ATA6613C       ,  0x0000,  16*1024,      0,       0,      0,      0, _ALL_},   // AVR+LIN Transceiver
  {_ATA6614Q       ,  0x0000,  32*1024,      0,       0,      0,      0, _ALL_},   // AVR+LIN Transceiver
  {_ATA6616C       ,  0x0000,   8*1024,      0,       0,      0,      0, _ALL_},   // AVR+LIN Transceiver
  {_ATA6617C       ,  0x0000,  16*1024,      0,       0,      0,      0, _ALL_},   // AVR+LIN Transceiver
  {_ATA664251      ,  0x0000,  16*1024,      0,       0,      0,      0, _ALL_},   // AVR+LIN Transceiver
  {_ATA8210        ,  0x0000,  20*1024,      0,       0,      0,      0, _ALL_},   // AVR+LIN Transceiver
  {_ATA8215        ,  0x0000,  20*1024,      0,       0,      0,      0, _ALL_},   // UHF ASK/FSK Receiver
  {_ATA8510        ,  0x0000,  20*1024,      0,       0,      0,      0, _ALL_},   // UHF ASK/FSK Receiver
  {_ATA8515        ,       0,        0,      0,       0,      0,      0, _ALL_},   // UHF ASK/FSK Receiver
//{_ATtiny416auto  ,  0x8000,   4*1024, 0x3F00,     256, 0x1400,    128, _ALL_},   // AVR uC Advanced Peripherals (see ATtiny..)
//{_ATtiny417auto  ,  0x8000,   4*1024, 0x3F00,     256, 0x1400,    128, _ALL_},   // AVR uC Advanced Peripherals (see ATtiny..)
//{_ATtiny816auto  ,  0x8000,   8*1024, 0x3E00,     512, 0x1400,    128, _ALL_},   // AVR uC Advanced Peripherals (see ATtiny..)
//{_ATtiny817auto  ,  0x8000,   8*1024, 0x3E00,     512, 0x1400,    128, _ALL_},   // AVR uC Advanced Peripherals (see ATtiny..)
  {NULL,                   0,        0,      0,       0,      0,      0, _NONE_}   // Device END
  }; // end of deviceAutomotive structure

LPDEVICEAVR deviceAutomotivePtr = deviceAutomotive; // Pointer to deviceAutomotive structure

//-----------------------------------------------------------------------------
//
//                      EvalDeviceAVR
//
// typedef struct tagDEVICEAVR {
//   char* deviceName;
//   ULONG FLASHStart;   
//   ULONG FLASHSize;    
//   ULONG SRAMStart;      
//   ULONG SRAMSize;     
//   ULONG EEPROMStart;  
//   ULONG EEPROMSize;   
//   UINT  missingInst;
//   } DEVICEAVR, *LPDEVICEAVR;             
//                                         
void EvalDeviceAVR(int _p12sw)
  {
  if (_p12sw == _PASS1)
    {
    swdevice = 0;                 // .DEVICE missing or 'partNameAVR' unknown
    devicePtr = deviceUnknownPtr; // Default global pointer to unknown device

    if (StrCmpNI(partNameAVR, "ATm", 3) == 0)      devicePtr = deviceATmegaPtr;
    else if (StrCmpNI(partNameAVR, "AT9", 3) == 0) devicePtr = deviceATmegaPtr;
    else if (StrCmpNI(partNameAVR, "ATt", 3) == 0) devicePtr = deviceTinyPtr;
    else if (StrCmpNI(partNameAVR, "AVR", 3) == 0) devicePtr = deviceAVRDxPtr;
    else if (StrCmpNI(partNameAVR, "ATx", 3) == 0) devicePtr = deviceXMEGAPtr;
    else if (StrCmpNI(partNameAVR, "ATA", 3) == 0) devicePtr = deviceAutomotivePtr;
    else return;                  // .DEVICE missing or 'partNameAVR' unknown

    // Search partNameAVR and init values
    while (devicePtr->deviceName != NULL)
      {
      if (StrCmpI(devicePtr->deviceName, partNameAVR) == 0)
        {
        RomStart    = devicePtr->FLASHStart;
        pcc = RomStart;                            // Init pcc initial value 
        csegLayout_ptr->sStart = RomStart; 
        RomSize     = devicePtr->FLASHSize;  
        SRamStart   = devicePtr->SRAMStart;  
        pcd = SRamStart;                           // Init pcd initial value 
        dsegLayout_ptr->sStart = SRamStart; 
        SRamSize    = devicePtr->SRAMSize;   
        EEPromStart = devicePtr->EEPROMStart;
        pce = EEPromStart;                         // Init pce initial value 
        esegLayout_ptr->sStart = EEPromStart; 
        EEPromSize  = devicePtr->EEPROMSize; 

        swdevice = 1;             // 'partNameAVR' successfully evaluated
        break;                    // done
        }
      devicePtr++;
      } // end while
    } // end if (_PASS1)
  
//ha//  else // _PASS2
//ha//    {
//ha//    // if (swpragma == 0) ;
//ha//    // check #pragma and issue an error if device not found
//ha//    }

  } // EvalDeviceAVR


//-----------------------------------------------------------------------------
//
//        AVRInstructionCheck / FindMissingInstr / StoreMissingInstr 
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

//-----------------------------------------------------------------------------
//
//                        FindMissingInstr
//
BOOL FindMissingInstr()
  {
  BOOL b_rslt = FALSE;            // Assume opcode not already stored

  _k=0;
  while (_k < MISS_INSTR_BUFLEN)
    {
    if (StrCmpI(&missInstrBuf[_k], fopcd) == 0)
      { 
      b_rslt = TRUE;
      break;
      }
    else _k += MISS_INSTR_LENGTH; // Size for each instr string
    }
  return(b_rslt); 
  }

//-----------------------------------------------------------------------------
//
//                        StoreMissingInstr
//
void StoreMissingInstr(int _instID)
  {
  // Skip if the special instruction is supported.
  if ((devicePtr->missingInst & _instID) == 0) return; 

  // Skip if the special instruction is already stored.
  if (devicePtr == NULL                 ||
      FindMissingInstr() == TRUE        ||
      devicePtr->missingInst == _NONE_)
    return;

  // Store the missing special instruction.
  for (_i=0; _i<MISS_INSTR_LENGTH; _i++)  *missInstrBuf_ptr++ = fopcd[_i];
  } // StoreMissingInstr

//-----------------------------------------------------------------------------
//
//                        AVRInstructionCheck
//
// It is expected (though not guaranteed) that in *def.inc files, provided by
// Atmel the "#pragma AVRPART CORE INSTRUCTIONS_NOT_SUPPORTED ..." lists all
// missing instructions of a certain AVR device correctly. The .DEVICE 
// directive, however, can only just rely on data published by Atmel/Microchip.
// This data is ambiguous, so all possible missing instructions, if found in the
// assembler source file, are detected here. The developer must verify if 
// there's any support of the chosen instruction(s) with the selected uC device.
// (by the way it's anyway recommended to study the uC DataSheet before
//  beginning a microchip project and writing the software, isn't it?)
//
void AVRInstructionCheck()
  {
  switch(ins_group)
    {
    case  2: // break spm ijmp icall eicall eijmp
     if (StrCmpI(fopcd, "break") == 0)    StoreMissingInstr(_BREAK_);
     else if (StrCmpI(fopcd, "spm") == 0) StoreMissingInstr(_SPM_);
     else if (StrCmpI(fopcd, "ijmp") == 0   || StrCmpI(fopcd, "icall") == 0) StoreMissingInstr(_IJMPCALL_);
     else if (StrCmpI(fopcd, "eicall") == 0 || StrCmpI(fopcd, "eijmp") == 0) StoreMissingInstr(_EIJMPCALL_);
     break;

    case  4: // MUL
     if (StrCmpI(fopcd, "mul") == 0) StoreMissingInstr(_MUL_);
     break;

    case  8: // FMUL FMULS FMULSU MULSU
    case 20: // MULS
     StoreMissingInstr(_MUL_);
     break;
    
    // Special devices (AVRrc reduced core): Missing PUSH POP   
    // The stack is replaced by a 3-level hardware stack, 
    // and the PUSH/POP instructions are deleted.
    case 11: 
     StoreMissingInstr(_PUSHPOP_);
     break;

    case 10: // LAC LAS LAT XCH
     StoreMissingInstr(_XCHLAx_);
     break;

    case 12: // ADIW SBIW
     StoreMissingInstr(_ADIWSBIW_);
     break;
    case 24: // MOVW
     StoreMissingInstr(_MOVW_);
     break;

    case 21: // DES
     StoreMissingInstr(_DES_);
     break;

    case 22: // JMP CALL
     StoreMissingInstr(_JMPCALL_);
     break;

    case 28: // LD
     StoreMissingInstr(_LD_);
     break;

    case 29: // LDD
    case 31: // STD
     StoreMissingInstr(_LDDSTD_);
     break;

    case 32: // LPM / ELPM
     if (StrCmpI(fopcd, "elpm") == 0) StoreMissingInstr(_ELPM_);
     if (StrCmpI(fopcd, "lpm") == 0) StoreMissingInstr(_LPM_);
     break;

    default:
      break;
    } // end switch 
  } // AVRInstructionCheck

//------------------------------------------------------------------------------
//
//                       InstructionInfo
//
// Example (to be displayed on console and appended to listing:
//
// Info - Used instructions below are missing in some AVR MicroChips.
//        Consult the specific uC Manual to comfirm the instructions. 
//        ---------   ---------   ---------   ---------   ---------  
//        LPM Rd,Z+   FMULSU      ELPM        LDD         XCH
//        DES         ADIW        EIJMP       SPM Z+      BREAK
//        PUSH        POP
//        ---------   ---------   ---------   ---------   ---------
// 
void InstructionInfo()
  {
  char* infoHdr1 = " Info - Used instructions below are missing in some AVR MicroChips.\n";
  char* infoHdr2 = "        See the specific uC Data Sheet to confirm the instructions.";
  char* infoHdr3 = "\n        ---------   ---------   ---------   ---------   ---------";
  char* infoHdr4 = "\n        ";

  // Skip info if all instructions are supported
  if (missInstrBuf[0] == 0) return;          

  printf(infoHdr1);                          // Display also on console
  printf(infoHdr2);
  printf(infoHdr3); 

  LstFile.write(infoHdr1, strlen(infoHdr1)); // Emit to listfile
  LstFile.write(infoHdr2, strlen(infoHdr2));
  LstFile.write(infoHdr3, strlen(infoHdr3));

  // Apped a formatted instruction table in listing 
  _i=0; _k=0;
  while (missInstrBuf[_i] != 0 && _i<(MISS_INSTR_BUFLEN))
    {
    if (_k % 5 == 0)
      {
      printf(infoHdr4);                      // Display also on console
      LstFile.write(infoHdr4, strlen(infoHdr4));
      }
    printf("%s", &missInstrBuf[_i]);         // Display also on console
    LstFile.write(&missInstrBuf[_i], strlen(&missInstrBuf[_i]));

    // Tabulating
    for (_j=strlen(&missInstrBuf[_i])-1; _j<MISS_INSTR_LENGTH+1; _j++)
      {                                      // Display also on console
      printf(" ");                           
      LstFile.write(" ", 1);                 // Emit into listfile
      }

    _i += MISS_INSTR_LENGTH; _k++;
    } // end while

  printf(infoHdr3); printf("\n");            // Display also on console
  LstFile.write(infoHdr3, strlen(infoHdr3)); // Emit to listfile
  newli(2);                                  // CRLF before "ASSEMBLY COMPLETE.." in listing
  } // InstructionInfo
 
//-----------------------------------------------------------------------------
//
//                      WriteDeviceInfo
//
// Console command: "XASMAVR /d"
// Display a formatted list of all supported AVR devices on console.
//
void WriteDeviceInfo(LPDEVICEAVR deviceStruc)
  {
  int _FLASHsize, _EEPROMsize, _SRAMsize;
  
  printf("DeviceName\tFLASH Start  Size   SRAM Start  Size   EEPROM Start  Size\n");
  printf("-------------------------------------------------------------------------\n");

  while (deviceStruc->deviceName != NULL)
    {
    _FLASHsize  = deviceStruc->FLASHSize/1024;
    _SRAMsize   = deviceStruc->SRAMSize;
    _EEPROMsize = deviceStruc->EEPROMSize;

    if (strlen(deviceStruc->deviceName) <= 7)
      printf("%s\t\tFLASH=0x%04X (%dK) ", deviceStruc->deviceName,
                                          deviceStruc->FLASHStart,
                                          _FLASHsize);
    else
      printf("%s\tFLASH=0x%04X (%dK) ", deviceStruc->deviceName,
                                        deviceStruc->FLASHStart,
                                        _FLASHsize);
    if (_FLASHsize < 10) printf("  ");
    else if (_FLASHsize >= 10 && _FLASHsize < 100 ) printf(" ");

    printf("SRAM=0x%04X ", deviceStruc->SRAMStart);
    if (_SRAMsize < 1024) printf("(%03d)  ", _SRAMsize);

    else if (_SRAMsize%1024 == 0 &&
             _SRAMsize/1024 >= 1 &&
             _SRAMsize/1024 < 10)
      printf("(%dK)   ", _SRAMsize/1024);

    else if (_SRAMsize%1024 ==  0 &&
             _SRAMsize/1024 >= 10 &&
             _SRAMsize/1024 < 100)
      printf("(%dK)  ", _SRAMsize/1024);

    else printf("(%d) ", _SRAMsize);
      
    printf("EEPROM=0x%04X ", deviceStruc->EEPROMStart);
    if (_EEPROMsize >= 1024)  printf("(%dK)\n", _EEPROMsize/1024);
    else printf("(%03d)\n", _EEPROMsize);
    deviceStruc++;
    } // end while
  } // WriteDeviceInfo

//-----------------------------------------------------------------------------
//
//                      ListDeviceAVR
//
// Console command: "XASMAVR /d"
// Display a formatted list of all supported AVR devices on console.
//
void ListDeviceAVR()
  {
  WriteDeviceInfo(deviceATmegaPtr);
  printf("\n");
  WriteDeviceInfo(deviceTinyPtr);
  printf("\n");
  WriteDeviceInfo(deviceAVRDxPtr);
  printf("\n");
  WriteDeviceInfo(deviceXMEGAPtr);
  printf("\n");
  WriteDeviceInfo(deviceAutomotivePtr);

  ListDeviceAVRNoInstructions();
  } // ListDeviceAVR

//-----------------------------------------------------------------------------
//
//                      WriteDeviceInfoInstr
//
// Console command: "XASMAVR /d"
// Display a formatted list of all supported AVR devices on console.
//
// typedef struct tagDEVICEAVR {
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
// #define _ALL_       0xFFFF // Reduced instruction set
// #define _NONE_      0x0000 // Full instruction set support
// #define _MUL_       0x0001 // missing: FMUL:FMULS:FMULSU:MUL:MULS:MULSU 
// #define _DES_       0x0002 // missing: DES
// #define _LD_        0x0004 // missing: LD:LD X:LD Y:LD -Z:LD Z+ 
// #define _LDDSTD_    0x0008 // missing: LDD:STD 
// #define _JMPCALL_   0x0010 // missing: CALL:JMP 
// #define _EIJMPCALL_ 0x0020 // missing: EICALL:EIJMP 
// #define _IJMPCALL_  0x0040 // missing: IJMP:ICALL 
// #define _ADSBIMOW_  0x0080 // missing: ADIW:MOVW:SBIW                    
// #define _XCHLAx_    0x0100 // missing: LAC:LAS:LAT:XCH 
// #define _LPM_       0x0200 // missing: LPM:LPM Rd,Z:LPM Rd,Z+  
// #define _ELPM_      0x0400 // missing: ELPM
// #define _SPM_       0x0800 // missing: SPM:SPM Z+ 
// #define _BREAK_     0x1000 // missing: BREAK
// 
// ?? PUSHPOP: The Note below was found in Atmel/Microchip documentation:     ??
//    "This instruction is not available on all devices. Refer to Appendix A."
// ?? (which is table above) ??
// #define _PUSHPOP_   0x2000 // missing: PUSH:POP (always (or sometimes not) supported ??)
//
// Example:
// -------------------------------------------------------------------------
// ATtiny102       INSTRUCTIONS_NOT_SUPPORTED :DES:LAC:LAS:LAT:XCH
//                         :FMUL:FMULS:FMULSU:MUL:MULSU
//                         :ADIW:MOVW:SBIW
//                         :LD:LDD:STD:LPM:ELPM:SPM:BREAK
//                         :CALL:JMP:EICALL:EIJMP:IJMP:ICALL
//
void WriteDeviceInfoInstr(LPDEVICEAVR deviceStruc)
  {
  char* _tab = "\t\t\t\t\t   "; // formatting stuff

  char* _none      = "Full instruction set support";
  char* _mul       = "FMUL:FMULS:FMULSU:MUL:MULSU";
  char* _des       = "DES";  
  char* _ld        = "LD";   
  char* _lddstd    = "LDD:STD";                    
  char* _jmpcall   = "CALL:JMP"; 
  char* _eijmpcall = "EICALL:EIJMP";                  
  char* _ijmpcall  = "ICALL:IJMP";                    
  char* _adiwsbiw  = "ADIW:SBIW";                  
  char* _movw      = "MOVW";                   
  char* _xchlax    = "LAC:LAS:LAT:XCH";                 
  char* _lpm       = "LPM";         
  char* _elpm      = "ELPM";
  char* _spm       = "SPM";     
  char* _break     = "BREAK";          
  char* _pushpop   = "PUSH:POP";

  while (deviceStruc->deviceName != NULL)
    {
    if (deviceStruc->missingInst == _NONE_)            
      printf("\n%s\t%s\n", deviceStruc->deviceName, _none);

    else if (strlen(deviceStruc->deviceName) <= 7)
      printf("\n%s\t\tINSTRUCTIONS_NOT_SUPPORTED ",
             deviceStruc->deviceName, deviceStruc->missingInst);
    else
      printf("\n%s\tINSTRUCTIONS_NOT_SUPPORTED ",
             deviceStruc->deviceName, deviceStruc->missingInst);

    int _k=-1;  // formatting stuff

    // INSTRUCTIONS_NOT_SUPPORTED
    // :DES
    if (deviceStruc->missingInst & _DES_)       { printf(":%s", _des); _k=1; }
    // :LAC:LAS:LAT:XCH
    if (deviceStruc->missingInst & _XCHLAx_)    { printf(":%s\n", _xchlax); _k+=4; }
    if (_k==1) printf("\n");
    // :FMUL:FMULS:FMULSU:MUL:MULSU
    if (deviceStruc->missingInst & _MUL_)       { printf("%s:%s\n", _tab, _mul); _k=0; }
    // :ADIW:SBIW
    if (deviceStruc->missingInst & _ADIWSBIW_)  { printf("%s:%s", _tab, _adiwsbiw);_k=1; }
    // :MOVW
    if (_k!=0 && deviceStruc->missingInst & _MOVW_) { printf(":%s\n", _movw);_k=0; }
    else if (deviceStruc->missingInst & _MOVW_)     { printf("%s:%s\n", _tab, _movw);_k=0; }
    // :LD:LDD:STD
    if (deviceStruc->missingInst & _LD_)        { printf("%s:%s", _tab, _ld);_k=0; }
    if (deviceStruc->missingInst & _LDDSTD_)    { printf(":%s\n", _lddstd); _k=0; }
    // :PUSH:POP
    if (_k!=-1 && deviceStruc->missingInst & _PUSHPOP_) { printf("%s:%s\n", _tab, _pushpop);_k=0; }
    else if (deviceStruc->missingInst & _PUSHPOP_)      { printf(":%s\n", _pushpop);_k=0; }
    // :ICALL:IJMP
    if (deviceStruc->missingInst & _IJMPCALL_)  { printf("%s:%s\n", _tab, _ijmpcall); _k=0; }
    // :EICALL:EIJMP
    if (deviceStruc->missingInst & _EIJMPCALL_) { printf("%s:%s\n", _tab, _eijmpcall); _k=2; }  // suppress CRLF
    // :CALL:JMP
    if (deviceStruc->missingInst & _JMPCALL_)   { printf("%s:%s\n", _tab, _jmpcall); _k=0; }
    // :LPM:
    if (deviceStruc->missingInst & _LPM_)       { printf("%s:%s", _tab, _lpm); _k=1; }
    // ELPM:SPM
    if (_k==1 && deviceStruc->missingInst & _ELPM_)  { printf(":%s", _elpm); _k=0; }
    else if (deviceStruc->missingInst & _ELPM_)      { printf("%s:%s", _tab, _elpm); _k=0; }
    // :SPM
    if (deviceStruc->missingInst & _SPM_)       { printf(":%s\n", _spm); _k=1; }
    // :BREAK
    if (_k!=0 && deviceStruc->missingInst & _BREAK_) { printf("%s:%s\n", _tab, _break);_k=1; }
    else if (deviceStruc->missingInst & _BREAK_)     { printf("\n%s:%s\n", _tab, _break); _k=1; }

    if (_k==0) printf("\n");
    deviceStruc++;
    } // end while
  } // WriteDeviceInfoInstr

//-----------------------------------------------------------------------------
//
//                      ListDeviceAVRNoInstructions
//
// Console command: "XASMAVR /d"
// Display a formatted list of all supported AVR devices on console.
//
void ListDeviceAVRNoInstructions()
  {
  char* separatorLine = "-------------------------------------------------------------------------";
  char* instrNoSupport = "DeviceName\tINSTRUCTIONS_NOT_SUPPORTED";  
  
  printf("%s\n", separatorLine);
  printf("%s\n\t\t'megaAVR(R) Devices'\n", instrNoSupport);
  printf(separatorLine);
  WriteDeviceInfoInstr(deviceATmegaPtr);
  printf("\n");

  printf("%s\n", separatorLine);
  printf("%s\n\t\t'tinyAVR(R) Devices'\n", instrNoSupport);
  printf(separatorLine);
  WriteDeviceInfoInstr(deviceTinyPtr);
  printf("\n");

  printf("%s\n", separatorLine);
  printf("%s\n\t\t'AVR(R) Dx Devices'\n", instrNoSupport);
  printf(separatorLine);
  WriteDeviceInfoInstr(deviceAVRDxPtr);
  printf("\n");
  
  printf("%s\n", separatorLine);
  printf("%s\n\t\t'XMEGA(R) Devices'\n", instrNoSupport);
  printf(separatorLine);
  WriteDeviceInfoInstr(deviceXMEGAPtr);
  printf("\n");
  
  printf("%s\n", separatorLine);
  printf("%s (for details see uC Datasheet)\n\t\t'Automotive Devices'\n", instrNoSupport);
  printf(separatorLine);
  WriteDeviceInfoInstr(deviceAutomotivePtr);
  } // ListDeviceAVRNoInstructions

//------------------------------------------------------------------------------

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha////if (_debugSwPass == _PASS1)                                         //--DEBUG--
//ha//{
//ha//printf("partNameAVR=%s  devicePtr=%08X  devicePtr->missingInst=%04X",
//ha//        partNameAVR,    devicePtr,      devicePtr->missingInst);
//ha//DebugStop(02, "EvalDeviceAVR()", __FILE__);                           //--DEBUG--
//ha//}                                                                     //--DEBUG--
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//--------------------------end-of-c++-module-----------------------------------




































































