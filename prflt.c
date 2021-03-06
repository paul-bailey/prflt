/*
 * Copyright (c) 2022, Paul Bailey
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

static double
dmantissa_(unsigned long mantissa, int shift)
{
        unsigned int msb = 1u << (shift-1);
        long double ret = 1.0L;
        long double retcmp = 0.5L;
        int i;
        for (i = 0; i < shift; i++) {
                if (!!(mantissa & msb))
                        ret += retcmp;
                mantissa <<= 1;
                retcmp /= 2.0L;
        }
        return (double)ret;
}


static double
fmantissa(unsigned int mantissa)
{
        return dmantissa_((unsigned long)mantissa, 23);
}

static double
dmantissa(unsigned long mantissa)
{
        return dmantissa_(mantissa, 52);
}

static void
prflt(float v)
{
        int exp;
        char sign;
        union {
                uint32_t u32;
                int32_t i32;
                float f;
        } x;
        unsigned int mantissa;
        x.f = v;
        exp = (x.u32 >> 23 & 0xffu) - 0x7f;
        sign = x.i32 < 0 ? '-' : '+';
        mantissa = x.u32 & ((1u << 23) - 1);
        printf("Single:\n");
        printf("\t0x%08X\n", x.u32);
        printf("\t%c%.8f * 2^%d\n", sign, fmantissa(mantissa), exp);
}

static void
prdbl(double v)
{
        enum {
                DMSIZE = 52,
                DMSIZE32 = DMSIZE - 32,
                DMMASK = (1ul << DMSIZE32) - 1,
        };
        int exp;
        char sign;
        uint32_t tmp;
        union {
                uint32_t u32[2];
                uint64_t u64;
                int64_t i64;
                double d;
        } x;
        unsigned long mantissa;

        if (sizeof(long) < 8) {
                fprintf(stderr, "Cannot process double precision\n");
                return;
        }

        x.d = v;
        sign = x.i64 < 0 ? '-' : '+';
        /* Apparently endianness only applies to four-byte chunks */
        exp = ((x.u32[1] >> DMSIZE32) & 0x7ffu) - 0x3ff;
        mantissa = (uint64_t)x.u32[1] | ((uint64_t)(x.u32[0] & DMMASK) << 32);
        printf("Double:\n");
        printf("\t0x%016lX\n", x.u64);
        printf("\t%c%.17f * 2^%d\n", sign, dmantissa(mantissa), exp);
}

int main(int argc, char **argv)
{
        float f;
        double d;
        char *endptr;
        if (argc < 2) {
                fprintf(stderr, "Expected: floating point real number\n");
                return 1;
        }
        errno = 0;
        f = strtof(argv[1], &endptr);
        if (endptr == argv[1] || errno || !isfinite(f)) {
                fprintf(stderr, "Single:\n\t%s bad or out of range\n",
                        argv[1]);
        } else {
                prflt(f);
        }

        errno = 0;
        d = strtod(argv[1], &endptr);
        if (endptr == argv[1] || errno || !isfinite(d)) {
                fprintf(stderr, "Double:\n\t%s bad or out of range\n",
                        argv[1]);
        } else {
                prdbl(d);
        }
        return 0;
}

