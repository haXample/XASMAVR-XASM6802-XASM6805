// haXASM - Cross-Assembler for 8bit Microprocessors
// extern.h - C++ Developer source file.
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
//              haXASM externals (public in workst.cpp) 
//
extern void edqh(char*, unsigned long long);

extern int swpass, swp1err, _debugSwPass, _hexFileEdit;
extern int _i, _j, _k, error, srcEOF, iclEOF, MotorolaFlag, AtmelFlag, IntelFlag;

extern char binPadByte;

extern char lastCharAL, lastCharAH, lastCharAX, _exprInfo, _PCsymbol;
extern char *symbl, *signon, *mssge, *_pszfile;
extern char *datpos_ptr, *pagpos_ptr, *lhtxt_ptr, *lhtxt16, *lhtxt32, *symtx;
extern char *errtx_file, *errtx_sys, *errtx_err, *err_alloc, *errtx;
extern char *qstr, *dstr;
 
extern int ercnt, errStatus, errline1, lastErr;
extern int pagel, pge, pws, linel, linct, _lstline, _curline, swmodel; 
extern int cmdmodel, nosym, cmdnosym;

extern int swoverlap, swstx, swsym, swlst, swlab, swcom, swicl, swicleof, swhxrec02;
extern int swdevice, swpragma, swdefmacro, swexpmacro, swlistmac, macroBegin;
extern int p1defmacroFlag, p2defmacroFlag;
extern int swif, swifdef, swelseif, swCif, swCifdef, swCelseif;//, swdqFlag;
extern int ifCount, p1Flagifndef, CifCount, Cp1ifdefCnt;  

extern int _errCSEG, _errDSEG, _errESEG;

extern int p1erc, erno, warno;

extern int SegType, _radix, macroCount, _ActivityMonitorCount, swActivityMonitor;

extern UINT value, RomPage, SRamStart, EEPromStart, EEPromPage, EEPromSize;
extern ULONG pcValue, pcc, pccw, pcd, pce, lvalue, strlenDB, RomStart, RomSize, SRamSize, BinFilesize;

extern unsigned __int64 qvalue;

extern int lpLOC, lpOBJ, lpSRC, lpLINE, lpMARK, lpMACR, lpXSEG;

extern char* pszSrcFilebuf;
extern char* pszSrcFilebuf0;

extern char* pszBinFilebuf;
extern char* pszBinbuf;      
extern char* pszBinEEbuf;    
extern char* pszHexFilebuf;  
extern char* pszHexFileEEbuf;
extern char* pszBinbuf0;      
extern char* pszBinEEbuf0;    
extern char* pszHexFilebuf0;  
extern char* pszHexFileEEbuf0;
extern char* pszScratchbuf; 
extern char* pszScratchbuf0;
extern char* pszErrtextbuf;
extern char* pszWarntextbuf;

extern UINT hxpc, eehxpc;

extern UCHAR ins_group, ilen, nopAlign, chksum, SymType;
extern UCHAR insv[], sline[];

extern char flabl[], fopcd[], oper1[], oper2[], oper3[], oper4[];
extern char oper5[], oper6[], oper7[], oper8[], oper9[], oper10[];
extern char oper11[], oper12[], oper13[], oper14[], oper15[], oper16[];
extern char lhbuf[], lh_date[], lh_time[], lhtit[], lhsubttl[], txerr[];
extern char partNameAVR[], p1Symifndef[];

extern char  hxbuf[],   hxfbuf[],   eehxbuf[],   eehxfbuf[], hxrec02[];
extern char *hxbuf_ptr,*hxfbuf_ptr,*eehxbuf_ptr,*eehxfbuf_ptr,*hxeof, *srstart;

extern char  srcbuf[],   iclbuf[],   szListBuf[], missInstrBuf[];
extern char *srcbuf_ptr,*iclbuf_ptr,*inbuf_ptr,*pszListBuf, *missInstrBuf_ptr;

extern char inbuf[];
extern char* inbuf_ptr;

extern char  symUndefBuf[];
extern char *symUndefBuf_ptr;

extern char *errSymbol_ptr, *warnSymbol_ptr;

extern char* warnText[], *errText[];  // pointer arrays

extern CIFDEF preprocessStack[];
extern LPCIFDEF preprocessStack_ptr;
extern int CifdefCnt, CifndefCnt, CifCnt, CelseCnt, CendifCnt, p12CSkipFlag;

//ha//extern SYMBOLBUF   symboltab[];
//ha//extern LPSYMBOLBUF symboltab_ptr, symboltab_top;  // !!! DONT USE 'symboltab_top' NOT RELIABLE  !!!
extern LPSYMBOLBUF symboltab;
extern LPSYMBOLBUF symboltab_ptr;

extern LPMACROBUF macrotab;

extern char* pszDefMacroBuf;          
extern char* pszDefMacroBuf0;         
extern char* pszExpMacroBuf;
extern char* pszExpMacroBuf0;
extern char* pszExpLabMacroBuf;
extern char* pszExpLabMacroBuf0;

//extern SPLITFIELD   srcLine;      // not used
//extern LPSPLITFIELD srcLine_ptr;

extern P1ERRTBL   p1errTbl[];    
extern LPP1ERRTBL p1errTbl_ptr; 

extern OPERXBUF   p1undef[]; 
extern LPOPERXBUF p1undef_ptr;

extern INCFILES   icfStack[];
extern LPINCFILES icfStack_ptr;

extern ORGSEG csegLayout[];            
extern LPORGSEG csegLayout_ptr; 

extern ORGSEG dsegLayout[];            
extern LPORGSEG dsegLayout_ptr; 

extern ORGSEG esegLayout[];            
extern LPORGSEG esegLayout_ptr; 

extern MACCASCADE macrocascade[];
extern LPMACCASCADE macrocascade_ptr;

extern LPDEVICEAVR devicePtr;
extern LPDEVICEAVR deviceUnknownPtr;

//------------------------------------------------------------------------------

