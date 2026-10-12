;==============================================================================
; mathtrans_lib.s -- AmigaOS shared-library shell for the free
;                    (redistributable) mathtrans.library
;==============================================================================
; Provides the library base, RomTag/auto-init, the standard Open/Close/Expunge
; vectors and the 17 mathtrans v1 function vectors (LVO -30 .. -126). The actual
; FFP algorithms live in mathtrans.c (AROS-derived, APL); this file only bridges
; the AmigaOS register-based library ABI to the C stack-based ABI and delegates
; all FFP arithmetic to the ROM-resident AmigaOS mathffp.library.
;
; Vector layout (LVO, name):
;   -30 SPAtan  -36 SPSin   -42 SPCos   -48 SPTan   -54 SPSincos -60 SPSinh
;   -66 SPCosh  -72 SPTanh  -78 SPExp   -84 SPLog   -90 SPPow    -96 SPSqrt
;  -102 SPTieee -108 SPSFieee -114 SPAsin -120 SPAcos  -126 SPLog10
;
; Licensed under the AROS Public License (APL) v1.1; see LICENSE.
;==============================================================================

        section .text

; --- standard exec/library offsets (exec/libraries.i) ----------------------
LIB_FLAGS       equ     14
LIB_NEGSIZE     equ     16
LIB_POSSIZE     equ     18
LIB_VERSION     equ     20
LIB_REVISION    equ     22
LIB_IDSTRING    equ     24
LIB_OPENCNT     equ     32
LIB_SIZE        equ     34

RTF_AUTOINIT    equ     $80
NT_LIBRARY      equ     9
RTC_MATCHWORD   equ     $4AFC

;-----------------------------------------------------------------------------
; Resident / RomTag -- MUST be the first thing in the first hunk.
;-----------------------------------------------------------------------------
Resident:
        dc.w    RTC_MATCHWORD
        dc.l    Resident
        dc.l    ResidentEnd
        dc.b    RTF_AUTOINIT
        dc.b    37                      ; rt_Version: the demo opens with version 31,
                                        ; the original library reports 37 (0x25)
        dc.b    NT_LIBRARY              ; rt_Type
        dc.b    0                       ; rt_Pri
        dc.l    LibName
        dc.l    LibIdString
        dc.l    AutoInit
ResidentEnd:

; auto-init structure: { dataSize, vectorTable, structInit, initFunc }
AutoInit:
        dc.l    LIB_SIZE
        dc.l    FuncTab
        dc.l    InitTable
        dc.l    LibInit

; vector table: index 0 -> LVO -6 (Open), 4 -> LVO -30 (SPAtan), ... , -1 ends
FuncTab:
        dc.l    LibOpen
        dc.l    LibClose
        dc.l    LibExpunge
        dc.l    LibReserved
        dc.l    SPAtan_e
        dc.l    SPSin_e
        dc.l    SPCos_e
        dc.l    SPTan_e
        dc.l    SPSincos_e
        dc.l    SPSinh_e
        dc.l    SPCosh_e
        dc.l    SPTanh_e
        dc.l    SPExp_e
        dc.l    SPLog_e
        dc.l    SPPow_e
        dc.l    SPSqrt_e
        dc.l    SPTieee_e
        dc.l    SPSFieee_e
        dc.l    SPAsin_e
        dc.l    SPAcos_e
        dc.l    SPLog10_e
        dc.l    -1

;-----------------------------------------------------------------------------
; Standard library vectors
;-----------------------------------------------------------------------------
LibOpen:
        addq.w  #1,LIB_OPENCNT(a6)
        move.l  a6,d0
        rts

LibClose:
        subq.w  #1,LIB_OPENCNT(a6)
        move.l  a6,d0
        rts

LibExpunge:
        moveq   #0,d0
        rts

LibReserved:
        moveq   #0,d0
        rts

; Init: D0=library base, A0=segList, A6=SysBase. Returns D0=base, or 0 on fail.
LibInit:
        movem.l d1/a0-a1,-(sp)
        move.l  d0,-(sp)                ; save library base
        lea     MathFFPName(pc),a1
        moveq   #0,d0
        jsr     -552(a6)                ; OpenLibrary("mathffp.library", 0)
        move.l  d0,MathBase
        move.l  (sp)+,d0                ; restore library base
        movem.l (sp)+,d1/a0-a1
        tst.l   d0
        beq.s   LibInitFail
        tst.l   MathBase
        beq.s   LibInitFail
        rts
LibInitFail:
        moveq   #0,d0
        rts

;-----------------------------------------------------------------------------
; Function vectors -> C entry points
;   single arg: value in D0
;   two args:   first in D0, second in D1 (AmigaOS mathffp convention)
;-----------------------------------------------------------------------------
SPAtan_e:
        move.l  d0,-(sp)
        jsr     SPAtan
        addq.l  #4,sp
        move.l  d0,d0
        rts

SPSin_e:
        move.l  d0,-(sp)
        jsr     SPSin
        addq.l  #4,sp
        move.l  d0,d0
        rts

SPCos_e:
        move.l  d0,-(sp)
        jsr     SPCos
        addq.l  #4,sp
        move.l  d0,d0
        rts

SPTan_e:
        move.l  d0,-(sp)
        jsr     SPTan
        addq.l  #4,sp
        move.l  d0,d0
        rts

SPSinh_e:
        move.l  d0,-(sp)
        jsr     SPSinh
        addq.l  #4,sp
        move.l  d0,d0
        rts

SPCosh_e:
        move.l  d0,-(sp)
        jsr     SPCosh
        addq.l  #4,sp
        move.l  d0,d0
        rts

SPTanh_e:
        move.l  d0,-(sp)
        jsr     SPTanh
        addq.l  #4,sp
        move.l  d0,d0
        rts

SPExp_e:
        move.l  d0,-(sp)
        jsr     SPExp
        addq.l  #4,sp
        move.l  d0,d0
        rts

SPLog_e:
        move.l  d0,-(sp)
        jsr     SPLog
        addq.l  #4,sp
        move.l  d0,d0
        rts

SPSqrt_e:
        move.l  d0,-(sp)
        jsr     SPSqrt
        addq.l  #4,sp
        move.l  d0,d0
        rts

SPTieee_e:
        move.l  d0,-(sp)
        jsr     SPTieee
        addq.l  #4,sp
        move.l  d0,d0
        rts

SPSFieee_e:
        move.l  d0,-(sp)
        jsr     SPSFieee
        addq.l  #4,sp
        move.l  d0,d0
        rts

SPAsin_e:
        move.l  d0,-(sp)
        jsr     SPAsin
        addq.l  #4,sp
        move.l  d0,d0
        rts

SPAcos_e:
        move.l  d0,-(sp)
        jsr     SPAcos
        addq.l  #4,sp
        move.l  d0,d0
        rts

SPLog10_e:
        move.l  d0,-(sp)
        jsr     SPLog10
        addq.l  #4,sp
        move.l  d0,d0
        rts

; SPPow(exponent=D1, base=D0) -> C SPPow(fnum1, fnum2)
SPPow_e:
        move.l  d0,-(sp)
        move.l  d1,-(sp)
        jsr     SPPow
        addq.l  #8,sp
        move.l  d0,d0
        rts

; SPSincos(cos_out=D1, angle=D0) -> C SPSincos(pfnum2, fnum1)
SPSincos_e:
        move.l  d0,-(sp)
        move.l  d1,-(sp)
        jsr     SPSincos
        addq.l  #8,sp
        move.l  d0,d0
        rts

;-----------------------------------------------------------------------------
; mathffp.library delegates, called from C (stack ABI: lvo, a[, b]).
; They load MathBase into A6, add the LVO offset and jsr, preserving the
; caller's A6 (the C frame pointer).
;-----------------------------------------------------------------------------
        xdef    mffp_2
mffp_2:                                 ; LONG mffp_2(LONG lvo, LONG a, LONG b)
        move.l  a6,-(sp)
        move.l  12(sp),d0               ; a
        move.l  16(sp),d1               ; b
        move.l  8(sp),a0                ; lvo
        move.l  MathBase,a6
        adda.l  a0,a6
        jsr     (a6)
        move.l  (sp)+,a6
        rts

        xdef    mffp_1
mffp_1:                                 ; LONG mffp_1(LONG lvo, LONG a)
        move.l  a6,-(sp)
        move.l  12(sp),d0               ; a
        move.l  8(sp),a0                ; lvo
        move.l  MathBase,a6
        adda.l  a0,a6
        jsr     (a6)
        move.l  (sp)+,a6
        rts

;-----------------------------------------------------------------------------
; Strings
;-----------------------------------------------------------------------------
LibName:
        dc.b    'mathtrans.library',0
        even
LibIdString:
        dc.b    'mathtrans.library 37.1 (AROS-derived reimplementation, APL)',0
        even
MathFFPName:
        dc.b    'mathffp.library',0
        even

; InitStruct table for MakeLibrary. On Kickstart 1.3 the library base fields
; (ln_Type, ln_Name, lib_Version, ...) are NOT copied from the RomTag for a
; disk auto-init library - the init table must set them, or the base ends up
; with lib_Version=0 and OpenLibrary(name,31) is refused. Encodings per
; exec/initializers.i: INITBYTE = $a0,off,val,0 ; INITWORD = $90,off,(word) ;
; INITLONG = $80,off,(long) ; terminator = a single 0 byte.
InitTable:
        dc.b    $a0,8,9,0               ; ln_Type = NT_LIBRARY
        dc.b    $80,10                  ; INITLONG ln_Name
        dc.l    LibName
        dc.b    $90,LIB_REVISION
        dc.w    1                       ; lib_Revision
        dc.b    $90,LIB_VERSION
        dc.w    37                      ; lib_Version (must be >= 31)
        dc.b    $a0,LIB_FLAGS,6,0       ; lib_Flags = LIBF_SUMUSED|LIBF_CHANGED
        dc.b    $80,LIB_IDSTRING        ; INITLONG lib_IdString
        dc.l    LibIdString
        dc.b    $90,LIB_OPENCNT
        dc.w    0                       ; lib_OpenCnt
        dc.b    0                       ; terminator
        even

;-----------------------------------------------------------------------------
        section .data
MathBase:
        dc.l    0
