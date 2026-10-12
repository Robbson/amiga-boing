/*
 * mathtrans.c -- free (redistributable) reimplementation of the AmigaOS
 * mathtrans.library v1 API, for the amiga-boing demo.
 *
 * ---------------------------------------------------------------------------
 * PROVENANCE / LICENSE
 * ---------------------------------------------------------------------------
 * The FFP transcendental algorithms below are transcribed from the AROS
 * project's mathtrans.library sources (workbench/libs/mathtrans/*.c),
 * Copyright (C) 1995-2006, The AROS Development Team, which are distributed
 * under the AROS Public License (APL) v1.1. This file is therefore a
 * derivative work and is distributed under the same license; see LICENSE
 * next to this file. It exists so that a built Boing ADF can be shared
 * *without* embedding Commodore/Hyperion's copyrighted mathtrans.library.
 *
 * The AmigaOS calling glue (register ABI, library base, vectors) is original
 * to this repository and lives in mathtrans_lib.s.
 *
 * ---------------------------------------------------------------------------
 * IMPLEMENTATION NOTES
 * ---------------------------------------------------------------------------
 * Freestanding C (no libc). All FFP arithmetic is delegated to the real,
 * ROM-resident AmigaOS mathffp.library via the mffp_1()/mffp_2() trampolines
 * in mathtrans_lib.s. Those trampolines put the first C argument in D0 and
 * the second in D1.
 *
 * AmigaOS mathffp.library uses the natural "D0 op D1" order, but the AROS
 * sources use the swapped C-level convention inherited from the original FFP
 * library. Verified against the demo's own compiled code (globe.s:411
 * "D0 = (sin*band)/D5", main.s:1563 "D0 = 192.0 - _fy", main.s:1551
 * "compare _fy : +96.0"):
 *
 *     AROS C call          AmigaOS LVO (D0 op D1)     ->  trampoline args
 *     SPAdd(a,b) = a+b     SPAdd(D0,D1) = D0+D1           (a, b)
 *     SPMul(a,b) = a*b     SPMul(D0,D1) = D0*D1           (a, b)
 *     SPSub(a,b) = b-a     SPSub(D0,D1) = D0-D1           (b, a)  swapped
 *     SPDiv(a,b) = b/a     SPDiv(D0,D1) = D0/D1           (b, a)  swapped
 *     SPCmp(a,b) = sgn(a-b) SPCmp(D0,D1) = sgn(D0-D1)     (a, b)
 *     SPFloor(a) = floor(a) SPFloor(D0)                    (a)
 *
 * SetSR() (setting the CCR for the caller) is a no-op here: the Boing demo
 * ignores the FFP condition codes, and skipping the privileged SR write keeps
 * this library safe to call from user mode.
 */

typedef long           LONG;
typedef unsigned long  ULONG;
typedef signed char    BYTE;

/* signed-char cast, matching the reference implementation's intent */
typedef signed char    SCH;

#define TRUE  1
#define FALSE 0

/* mathffp.library delegate trampolines (asm) */
extern LONG mffp_2(LONG lvo, LONG a, LONG b);
extern LONG mffp_1(LONG lvo, LONG a);

#define SPAdd(a,b) mffp_2(-66, (LONG)(a), (LONG)(b))
#define SPSub(a,b) mffp_2(-72, (LONG)(b), (LONG)(a))   /* swapped: b-a */
#define SPMul(a,b) mffp_2(-78, (LONG)(a), (LONG)(b))
#define SPDiv(a,b) mffp_2(-84, (LONG)(b), (LONG)(a))   /* swapped: b/a */
#define SPCmp(a,b) mffp_2(-42, (LONG)(a), (LONG)(b))
#define SPFloor(a) mffp_1(-90, (LONG)(a))

static void SetSR(LONG bits, LONG mask)
{
    (void)bits;
    (void)mask;
}

/* FFP field masks */
#define FFPMantisse_Mask 0xFFFFFF00
#define FFPExponent_Mask 0x0000007F
#define FFPSign_Mask     0x00000080

#define IEEESPMantisse_Mask 0x007FFFFF
#define IEEESPExponent_Mask 0x7F800000
#define IEEESPSign_Mask     0x80000000

#define Zero_Bit     0x00000004
#define Negative_Bit 0x00000008
#define Overflow_Bit 0x00000002

#define FFP_zero   0x00000000
#define FFP_Pinfty 0xffffff7f
#define FFP_Ninfty 0xffffffff
#define FFP_NAN    0xffffffff

#define InvLde   0xb1721840
#define InvLd10  0x9a209b3f
#define sqrtonehalf 0xb504f340

#define one      0x80000041
#define minusone 0x800000c1
#define two      0x80000042
#define onehalf  0x80000040
#define three    0xc0000042

#define pi       0xc90fdb42
#define pio2     0xc90fdb41
#define pio4     0xc90fdb40

#define sinf1    one
#define sinf2    0xaaaaabbe
#define sinf3    0x8888893a
#define sinf4    0xd00d01b4
#define sinf5    0xb8ef1d2e
#define sinf6    0xd7322ba7

#define cosf1    one
#define cosf2    0x800000c0
#define cosf3    0xaaaaab3c
#define cosf4    0xb60b61b7
#define cosf5    0xd00d0131
#define cosf6    0x93f27eab

#define pS0      0xaaaaaa3e
#define pS1      0xa6b090bf
#define pS2      0xce0aa83e
#define pS3      0xa41146bc
#define pS4      0xcf7f0436
#define pS5      0x91ef0932

#define qS1      0x99d138c2
#define qS2      0x81572c42
#define qS3      0xb03361c0
#define qS4      0x9dc62e3d

/* internal helpers */
static LONG intern_SPisodd(ULONG fnum);

/* forward declarations of the library entry points */
LONG SPAtan(LONG fnum1);
LONG SPSin(LONG fnum1);
LONG SPCos(LONG fnum1);
LONG SPTan(LONG fnum1);
LONG SPSincos(LONG *pfnum2, LONG fnum1);
LONG SPSinh(LONG fnum1);
LONG SPCosh(LONG fnum1);
LONG SPTanh(LONG fnum1);
LONG SPExp(LONG fnum1);
LONG SPLog(LONG fnum1);
LONG SPPow(LONG fnum1, LONG fnum2);
LONG SPSqrt(LONG fnum1);
LONG SPTieee(LONG fnum);
LONG SPSFieee(LONG ieeenum);
LONG SPAsin(LONG fnum1);
LONG SPAcos(LONG fnum1);
LONG SPLog10(LONG fnum1);

/* ------------------------------------------------------------------ */
/* intern_SPisodd: is FFP value an odd integer?                        */
/* ------------------------------------------------------------------ */
static LONG intern_SPisodd(ULONG fnum)
{
    signed char Exponent = ((signed char)(fnum & FFPExponent_Mask)) - 0x41;
    ULONG Mask;

    /* Replicate m68k shift semantics (count is taken modulo 64; a 32-bit
       right shift by >= 32 yields 0) without relying on C UB. */
    if ((ULONG)Exponent < 32)
        Mask = 0x80000000UL >> (ULONG)Exponent;
    else
        Mask = 0;

    if ((fnum & Mask) != 0) return TRUE;
    else                    return FALSE;
}

/* ------------------------------------------------------------------ */
/* intern_SPLd: log (base 2) of the mantissa                           */
/* ------------------------------------------------------------------ */
static LONG intern_SPLd(ULONG fnum)
{
    ULONG i = 0, Res = 0, c;
    ULONG Mask = 0x80000000;

    if (onehalf == fnum) return minusone;

    while (Res == 0 && i <= 23)
    {
        if (sqrtonehalf < fnum)
        {
            i++;
            fnum = SPMul(fnum, fnum);
        }
        else
        {
            Res = Mask;
            Mask >>= 1;
            fnum = SPMul(fnum, fnum);
            fnum++;
        }
    }

    c = i;

    while ((signed char)Mask != 0x40 && c < 24)
    {
        if (sqrtonehalf < fnum)
        {
            fnum = SPMul(fnum, fnum);
        }
        else
        {
            Res |= Mask;
            fnum = SPMul(fnum, fnum);
            fnum++;
        }
        c++;
        Mask >>= 1;
    }

    /* for precision */
    if ((signed char)Res < 0)
    {
        Res += 0x100;
        Res &= FFPMantisse_Mask;
    }

    return (Res | (0x40 - i) | FFPSign_Mask);
}

/* ------------------------------------------------------------------ */
/* SPAtan                                                              */
/* ------------------------------------------------------------------ */
LONG SPAtan(LONG fnum1)
{
    LONG fnumabs = fnum1 & (FFPMantisse_Mask | FFPExponent_Mask);
    LONG fnumsquared, fnumcubed;

    if ((LONG)FFP_Pinfty == fnumabs) return (pio2 | (fnum1 & FFPSign_Mask));

    if (0 == fnumabs)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }

    if ((BYTE)fnumabs >= 0x48)
    {
        if (fnumabs == fnum1)
        {
            SetSR(0, Zero_Bit | Negative_Bit | Overflow_Bit);
            return SPSub(SPDiv(fnumabs, one), pio2);
        }
        else
        {
            LONG res;
            SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
            res = SPSub(SPDiv(fnumabs, one), pio2);
            return res | FFPSign_Mask;
        }
    }

    fnumsquared = SPMul(fnumabs, fnumabs);

    if ((BYTE)fnumabs >= 0x47)
    {
        fnumcubed = SPMul(fnumabs, fnumsquared);

        if (fnumabs == fnum1)
        {
            LONG res;
            SetSR(0, Zero_Bit | Negative_Bit | Overflow_Bit);
            res = SPMul(three, fnumsquared);
            return SPAdd(pio2, SPDiv(SPMul(three, fnumcubed),
                                     SPAdd(res | FFPSign_Mask, one)));
        }
        else
        {
            LONG res;
            SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
            res = SPMul(three, fnumsquared);
            res = SPAdd(pio2, SPDiv(SPMul(three, fnumcubed),
                                    SPAdd(res | FFPSign_Mask, one)));
            return res | FFPSign_Mask;
        }
    }

    return SPAsin(SPDiv(SPSqrt(SPAdd(one, fnumsquared)), fnum1));
}

/* ------------------------------------------------------------------ */
/* SPSin                                                               */
/* ------------------------------------------------------------------ */
LONG SPSin(LONG fnum1)
{
    LONG z, Res, ysquared, yabs, tmp;
    yabs = fnum1 & (FFPMantisse_Mask + FFPExponent_Mask);

    if ((LONG)FFP_Pinfty == yabs)
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return FFP_NAN;
    }

    z = SPFloor(SPDiv(pi, yabs));
    tmp = SPMul(z, pi);
    tmp |= FFPSign_Mask; /* tmp = -tmp; */
    yabs = SPAdd(yabs, tmp);

    if ((SCH)yabs > (SCH)pio2 && (yabs & FFPMantisse_Mask) > (pio2 & FFPMantisse_Mask))
    {
        yabs |= FFPSign_Mask;
        yabs = SPAdd(pi, yabs);
    }
    ysquared = SPMul(yabs, yabs);
    Res = SPMul(yabs,
          SPAdd(sinf1,
          SPMul(ysquared,
          SPAdd(sinf2,
          SPMul(ysquared,
          SPAdd(sinf3,
          SPMul(ysquared,
          SPAdd(sinf4,
          SPMul(ysquared,
          SPAdd(sinf5,
          SPMul(ysquared, sinf6)))))))))));

    if ((SCH)fnum1 < 0) Res ^= FFPSign_Mask;

    if (TRUE == intern_SPisodd(z)) Res ^= FFPSign_Mask;

    if (0 == Res)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }

    if ((SCH)Res < 0)
    {
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    }

    return Res;
}

/* ------------------------------------------------------------------ */
/* SPCos                                                               */
/* ------------------------------------------------------------------ */
LONG SPCos(LONG fnum1)
{
    LONG z, Res, ysquared, yabs, tmp;
    yabs = fnum1 & (FFPMantisse_Mask + FFPExponent_Mask);

    if ((LONG)FFP_Pinfty == yabs)
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return FFP_NAN;
    }

    z = SPFloor(SPDiv(pi, yabs));
    tmp = SPMul(z, pi);
    tmp |= FFPSign_Mask; /* tmp = -tmp; */
    yabs = SPAdd(yabs, tmp);

    if ((SCH)yabs > (SCH)pio2 && (yabs & FFPMantisse_Mask) > (pio2 & FFPMantisse_Mask))
    {
        yabs |= FFPSign_Mask;
        yabs = SPAdd(pi, yabs);
        tmp = TRUE;
    }
    else
    {
        tmp = FALSE;
    }

    ysquared = SPMul(yabs, yabs);
    Res = SPAdd(cosf1,
          SPMul(ysquared,
          SPAdd(cosf2,
          SPMul(ysquared,
          SPAdd(cosf3,
          SPMul(ysquared,
          SPAdd(cosf4,
          SPMul(ysquared,
          SPAdd(cosf5,
          SPMul(ysquared, cosf6))))))))));

    if (0 == Res)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }

    if (TRUE == intern_SPisodd(z)) Res ^= FFPSign_Mask;

    if (TRUE == tmp) Res ^= FFPSign_Mask;

    if ((SCH)Res < 0) SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);

    return Res;
}

/* ------------------------------------------------------------------ */
/* SPTan                                                               */
/* ------------------------------------------------------------------ */
LONG SPTan(LONG fnum1)
{
    LONG z, SIN, Res, ysquared, yabs, tmp;
    yabs = fnum1 & (FFPMantisse_Mask + FFPExponent_Mask);

    if ((LONG)FFP_Pinfty == yabs)
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return FFP_NAN;
    }

    z = SPFloor(SPDiv(pi, yabs));
    tmp = SPMul(z, pi);
    tmp |= FFPSign_Mask; /* tmp = -tmp; */
    yabs = SPAdd(yabs, tmp);

    if ((SCH)yabs > (SCH)pio2 && (yabs & FFPMantisse_Mask) > (pio2 & FFPMantisse_Mask))
    {
        yabs |= FFPSign_Mask;
        yabs = SPAdd(pi, yabs);
        tmp = TRUE;
    }
    else
    {
        tmp = FALSE;
    }

    ysquared = SPMul(yabs, yabs);
    SIN = SPMul(yabs,
          SPAdd(sinf1,
          SPMul(ysquared,
          SPAdd(sinf2,
          SPMul(ysquared,
          SPAdd(sinf3,
          SPMul(ysquared,
          SPAdd(sinf4,
          SPMul(ysquared,
          SPAdd(sinf5,
          SPMul(ysquared, sinf6)))))))))));
    z = SPAdd(cosf1,
        SPMul(ysquared,
        SPAdd(cosf2,
        SPMul(ysquared,
        SPAdd(cosf3,
        SPMul(ysquared,
        SPAdd(cosf4,
        SPMul(ysquared,
        SPAdd(cosf5,
        SPMul(ysquared, cosf6))))))))));

    Res = SPDiv(z, SIN);

    if ((SCH)fnum1 < 0) Res ^= FFPSign_Mask;
    if (TRUE == tmp)    Res ^= FFPSign_Mask;

    if (0 == Res)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }

    if ((SCH)Res < 0)
    {
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    }

    return Res;
}

/* ------------------------------------------------------------------ */
/* SPSincos                                                            */
/* ------------------------------------------------------------------ */
LONG SPSincos(LONG *pfnum2, LONG fnum1)
{
    *pfnum2 = SPCos(fnum1);
    return SPSin(fnum1);
}

/* ------------------------------------------------------------------ */
/* SPSinh                                                              */
/* ------------------------------------------------------------------ */
LONG SPSinh(LONG fnum1)
{
    ULONG Res;
    LONG tmp;

    Res = SPExp(fnum1 & (FFPMantisse_Mask + FFPExponent_Mask));

    if (FFP_Pinfty == Res)
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return Res;
    }

    tmp = (fnum1 & FFPExponent_Mask) - 0x41;

    if (tmp <= 2 || (tmp == 3 && (fnum1 & FFPMantisse_Mask) < 0x90000000))
    {
        Res = SPAdd(Res, ((ULONG)SPDiv(Res, one) | FFPSign_Mask));
    }

    /* Res = Res / 2  (decrement FFP exponent) */
    tmp = Res & 0xFFFFFF00;
    Res -= 1;
    Res = Res | tmp;

    if (0 == Res || (SCH)Res < 0)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }

    if ((SCH)fnum1 < 0)
    {
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return (Res | FFPSign_Mask);
    }

    return Res;
}

/* ------------------------------------------------------------------ */
/* SPCosh                                                              */
/* ------------------------------------------------------------------ */
LONG SPCosh(LONG fnum1)
{
    ULONG Res;
    LONG tmp;

    fnum1 &= (FFPMantisse_Mask + FFPExponent_Mask);

    Res = SPExp(fnum1);

    if (FFP_Pinfty == Res)
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return Res;
    }

    tmp = (fnum1 & FFPExponent_Mask) - 0x41;

    if (tmp <= 2 || (tmp == 3 && (fnum1 & FFPMantisse_Mask) < 0x90000000))
    {
        Res = SPAdd(Res, SPDiv(Res, one));
    }

    tmp = Res & 0xFFFFFF00;
    Res -= 1;
    Res = tmp | Res;

    if (0 == Res || (SCH)Res < 0)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }

    return Res;
}

/* ------------------------------------------------------------------ */
/* SPTanh                                                              */
/* ------------------------------------------------------------------ */
LONG SPTanh(LONG fnum1)
{
    ULONG Res;
    LONG tmp;

    tmp = (fnum1 & FFPExponent_Mask) - 0x41;

    if (tmp >= 3 && (fnum1 & FFPMantisse_Mask) >= 0x90000000)
    {
        return (one | (fnum1 & FFPSign_Mask));
    }

    Res = SPExp(fnum1 & (FFPMantisse_Mask + FFPExponent_Mask));
    Res = SPDiv(SPAdd(Res, SPDiv(Res, one)),
                SPAdd(Res, (ULONG)SPDiv(Res, one) | FFPSign_Mask));

    if (0 == Res)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }

    if ((SCH)fnum1 < 0)
    {
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return (Res | FFPSign_Mask);
    }

    return Res;
}

/* ------------------------------------------------------------------ */
/* SPExp                                                               */
/* ------------------------------------------------------------------ */
LONG SPExp(LONG fnum1)
{
    static const ULONG ExpTable[] =
    {
        0x8fa1fe6f,  /* e^32  */
        0x87975e58,  /* e^16  */
        0xba4f534c,  /* e^8   */
        0xda648146,  /* e^4   */
        0xec732543,  /* e^2   */
        0xadf85442,  /* e^1   */
        0xD3094C41,  /* e^(1/2) */
        0xA45aF241,  /* e^(1/4) */
        0x910b0241,  /* e^(1/8) */
        0x88415b41,  /* e^(1/16) */
        0x84102b41,  /* e^(1/32) */
        0x82040541,  /* e^(1/64) */
        0x81010141,  /* e^(1/128) */
        0x80804041,  /* e^(1/256) */
        0x80401041,  /* e^(1/512) */
        0x80200441,  /* e^(1/1024) */
        0x80100141,  /* e^(1/2048) */
        0x80080041,  /* e^(1/4096) */
        0x80040041,  /* e^(1/8192) */
        0x80020041,  /* e^(1/16384) */
        0x80010041,  /* e^(1/32768) */
        0x80008041,  /* e^(1/65536) */
        0x80004041,  /* e^(1/131072) */
        0x80002041,  /* e^(1/262144) */
        0x80001041,  /* e^(1/524288) */
        0x80000841,  /* e^(1/1048576) */
        0x80000441,  /* e^(1/2097152) */
        0x80000241,  /* e^(1/4194304) */
        0x80000141   /* e^(1/8388608) */
    };

    ULONG Res, i;
    LONG Mantisse;
    signed char Exponent;

    Exponent = (fnum1 & FFPExponent_Mask) - 0x41;

    if (0 == fnum1 || Exponent <= -24) return one;

    if (Exponent > 5)
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return FFP_Pinfty;
    }

    i = 5 - Exponent;

    Mantisse = (fnum1 & FFPMantisse_Mask);
    Res = ExpTable[i++];
    Mantisse <<= 1;

    while (0 != Mantisse && i <= 28)
    {
        if (Mantisse < 0)
        {
            Res = SPMul(Res, ExpTable[i]);
            if (0 == Res)
            {
                SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
                return Res;
            }
            if (FFP_Pinfty == Res)
            {
                SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
                return Res;
            }
        }
        i++;
        Mantisse <<= 1;
    }

    if ((SCH)fnum1 < 0) return SPDiv(one, Res);

    return Res;
}

/* ------------------------------------------------------------------ */
/* SPLog                                                               */
/* ------------------------------------------------------------------ */
LONG SPLog(LONG fnum1)
{
    LONG ld_M, Exponent, Mask = 0x40, i, Sign;

    if ((SCH)fnum1 < 0)
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }

    if (fnum1 == 0) return FFP_Ninfty;

    Exponent = (fnum1 & FFPExponent_Mask) - 0x40;
    if (Exponent < 0)
    {
        Exponent = -Exponent;
        Sign = FFPSign_Mask;
    }
    else
    {
        Sign = 0;
    }

    if (Exponent != 0)
    {
        i = 0;
        while ((Mask & Exponent) == 0)
        {
            i++;
            Mask >>= 1;
        }
        Exponent <<= (25 + i);
        Exponent |= (0x47 - i + Sign);
    }

    ld_M = intern_SPLd((fnum1 & FFPMantisse_Mask) | 0x40);

    return SPMul(SPAdd(ld_M, Exponent), InvLde);
}

/* ------------------------------------------------------------------ */
/* SPLog10                                                             */
/* ------------------------------------------------------------------ */
LONG SPLog10(LONG fnum1)
{
    LONG ld_M, Exponent, Mask = 0x40, i, Sign;

    if ((SCH)fnum1 < 0)
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }

    if (0 == fnum1) return 0xde5bd8fe;

    Exponent = (fnum1 & FFPExponent_Mask) - 0x40;
    if (Exponent < 0)
    {
        Exponent = -Exponent;
        Sign = FFPSign_Mask;
    }
    else
    {
        Sign = 0;
    }

    if (Exponent != 0)
    {
        i = 0;
        while ((Mask & Exponent) == 0)
        {
            i++;
            Mask >>= 1;
        }
        Exponent <<= (25 + i);
        Exponent |= (0x47 - i + Sign);
    }

    ld_M = intern_SPLd((fnum1 & FFPMantisse_Mask) | 0x40);

    return SPMul(SPAdd(ld_M, Exponent), InvLd10);
}

/* ------------------------------------------------------------------ */
/* SPPow                                                               */
/* ------------------------------------------------------------------ */
LONG SPPow(LONG fnum1, LONG fnum2)
{
    ULONG Res;

    Res = SPLog(fnum2 & (FFPMantisse_Mask + FFPExponent_Mask));
    Res = SPMul(Res, fnum1);
    Res = SPExp(Res);

    if (0 == Res)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }

    if ((SCH)Res < 0)
    {
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    }

    return Res;
}

/* ------------------------------------------------------------------ */
/* SPSqrt                                                              */
/* ------------------------------------------------------------------ */
LONG SPSqrt(LONG fnum1)
{
    ULONG Res, ResSquared, Delta, X, TargetMantisse;
    int z;
    BYTE Exponent;

    if (0 == fnum1)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }

    if ((SCH)fnum1 < 0)
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return fnum1;
    }

    TargetMantisse = ((ULONG)fnum1 & FFPMantisse_Mask);

    if (fnum1 & 1)
    {
        TargetMantisse >>= 1;
        Exponent = ((BYTE)fnum1 >> 1) + 0x21;
    }
    else
    {
        Exponent = ((BYTE)fnum1 >> 1) + 0x20;
    }

    Res = 0;
    ResSquared = 0;
    z = 0;

    while (ResSquared != TargetMantisse && z < 25)
    {
        Delta = (0x80000000 >> z);
        X = ResSquared + (Res >> z) + (Delta >> (z + 1));
        z++;
        if (X <= TargetMantisse)
        {
            Res += Delta;
            ResSquared = X;
        }
    }

    if ((SCH)Res < 0) Res += 0x100;

    Res &= FFPMantisse_Mask;
    Res |= Exponent;

    return Res;
}

/* ------------------------------------------------------------------ */
/* SPTieee : FFP -> IEEE single                                        */
/* ------------------------------------------------------------------ */
LONG SPTieee(LONG fnum)
{
    LONG Res;
    LONG Exponent;

    if (0 == fnum)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }

    Exponent = (fnum & FFPExponent_Mask) - 0x40 + 126;

    Res = (Exponent << (30 - 7));
    Res |= (((ULONG)fnum & 0x7fffff00) >> 8);

    if ((SCH)fnum < 0)
    {
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        Res |= IEEESPSign_Mask;
    }

    return Res;
}

/* ------------------------------------------------------------------ */
/* SPSFieee : IEEE single -> FFP                                       */
/* ------------------------------------------------------------------ */
LONG SPSFieee(LONG ieeenum)
{
    LONG Res;

    if (0 == ieeenum)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }

    Res = (ieeenum & IEEESPExponent_Mask) >> 23;
    Res = Res - 126 + 0x40;

    if ((SCH)Res < 0)
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return Res;
    }

    Res |= (ieeenum << 8) | 0x80000000;

    if (ieeenum < 0)
    {
        Res |= FFPSign_Mask;
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    }

    return Res;
}

/* ------------------------------------------------------------------ */
/* SPAsin                                                              */
/* ------------------------------------------------------------------ */
LONG SPAsin(LONG fnum1)
{
    LONG t, w, p, q, c, r, s, ix;

    ix = fnum1 & (FFPMantisse_Mask | FFPExponent_Mask);

    if ((LONG)one == ix)
    {
        return (pio2 | (fnum1 & FFPSign_Mask));
    }

    if (1 == SPCmp(ix, one))
    {
        SetSR(Overflow_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
        return -1;
    }

    if (-1 == SPCmp(ix, onehalf))
    {
        if (-1 == SPCmp(ix, 0xb89b6736))
        {
            return fnum1;
        }

        t = SPMul(fnum1, fnum1);
        p = SPMul(t, SPAdd(pS0,
            SPMul(t, SPAdd(pS1,
            SPMul(t, SPAdd(pS2,
            SPMul(t, SPAdd(pS3,
            SPMul(t, SPAdd(pS4,
            SPMul(t, pS5)))))))))));
        q = SPAdd(one,
            SPMul(t, SPAdd(qS1,
            SPMul(t, SPAdd(qS2,
            SPMul(t, SPAdd(qS3,
            SPMul(t, qS4))))))));
        w = SPDiv(q, p);

        return SPAdd(fnum1, SPMul(fnum1, w));
    }

    w = SPSub(ix, one);
    t = SPMul(w, onehalf);
    p = SPMul(t, SPAdd(pS0,
        SPMul(t, SPAdd(pS1,
        SPMul(t, SPAdd(pS2,
        SPMul(t, SPAdd(pS3,
        SPMul(t, SPAdd(pS4,
        SPMul(t, pS5)))))))))));
    q = SPAdd(one,
        SPMul(t, SPAdd(qS1,
        SPMul(t, SPAdd(qS2,
        SPMul(t, SPAdd(qS3,
        SPMul(t, qS4))))))));
    s = SPSqrt(t);

    if (1 == SPCmp(ix, 0xf9999a40))
    {
        w = SPDiv(q, p);
        t = SPSub(SPMul(two, SPAdd(s, SPMul(s, w))), pio2);
        t = SPAdd(t, 0x8000002b);
    }
    else
    {
        w = s;
        c = SPDiv(SPAdd(s, w), SPSub(SPMul(w, w), t));
        r = SPDiv(q, p);
        p = SPAdd(SPAdd(c, c), SPMul(SPAdd(s, s), r));
        q = SPSub(SPAdd(w, w), pio4);
        t = SPSub(SPSub(q, p), pio4);
    }

    return (t | (fnum1 & FFPSign_Mask));
}

/* ------------------------------------------------------------------ */
/* SPAcos                                                              */
/* ------------------------------------------------------------------ */
LONG SPAcos(LONG fnum1)
{
    LONG z, p, q, r, w, s, c, ix, df;

    ix = fnum1 & (FFPMantisse_Mask | FFPExponent_Mask);

    z = SPCmp(ix, one);

    if (1 == z)
    {
        SetSR(Overflow_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
        return -1;
    }

    if (0 == z)
    {
        if (fnum1 & FFPSign_Mask) return pi;
        SetSR(Zero_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
        return 0;
    }

    if (-1 == SPCmp(ix, onehalf))
    {
        z = SPMul(fnum1, fnum1);
        p = SPMul(z, SPAdd(pS0,
            SPMul(z, SPAdd(pS1,
            SPMul(z, SPAdd(pS2,
            SPMul(z, SPAdd(pS3,
            SPMul(z, SPAdd(pS4,
            SPMul(z, pS5)))))))))));
        q = SPAdd(one,
            SPMul(z, SPAdd(qS1,
            SPMul(z, SPAdd(qS2,
            SPMul(z, SPAdd(qS3,
            SPMul(z, qS4))))))));
        r = SPDiv(q, p);
        return (SPSub(SPAdd(fnum1, SPMul(fnum1, r)), pio2));
    }

    if (fnum1 & FFPSign_Mask)
    {
        z = SPMul(onehalf, SPAdd(one, fnum1));
        p = SPMul(z, SPAdd(pS0,
            SPMul(z, SPAdd(pS1,
            SPMul(z, SPAdd(pS2,
            SPMul(z, SPAdd(pS3,
            SPMul(z, SPAdd(pS4,
            SPMul(z, pS5)))))))))));
        q = SPAdd(one,
            SPMul(z, SPAdd(qS1,
            SPMul(z, SPAdd(qS2,
            SPMul(z, SPAdd(qS3,
            SPMul(z, qS4))))))));
        s = SPSqrt(z);
        r = SPDiv(q, p);
        w = SPMul(r, s);
        return SPSub(SPMul(two, SPAdd(s, w)), pi);
    }

    z = SPMul(onehalf, SPSub(fnum1, one));
    s = SPSqrt(z);
    df = s;
    df = df & 0xfff000ff;
    c = SPDiv(SPAdd(df, s), SPSub(SPMul(df, df), z));
    p = SPMul(z, SPAdd(pS0,
        SPMul(z, SPAdd(pS1,
        SPMul(z, SPAdd(pS2,
        SPMul(z, SPAdd(pS3,
        SPMul(z, SPAdd(pS4,
        SPMul(z, pS5)))))))))));
    q = SPAdd(one,
        SPMul(z, SPAdd(qS1,
        SPMul(z, SPAdd(qS2,
        SPMul(z, SPAdd(qS3,
        SPMul(z, qS4))))))));
    r = SPDiv(q, p);
    w = SPAdd(c, SPMul(r, s));

    return SPAdd(SPMul(two, SPAdd(df, w)), 0x800000a9);
}
