/* Hey, Emacs, this a -*-C++-*- file !
 *
 * Copyright distributed.net 1997-2003 - All Rights Reserved
 * For use in distributed.net projects only.
 * Any other distribution or use of this source violates copyright.
 *
 * ---------------------------------------------------------------
 * Routines to convert DES key # from/to incremental format
 * by Remi Guyomarche
 * ---------------------------------------------------------------
*/ 
#ifndef __CONVDES_H__
#define __CONVDES_H__ "@(#)$Id: convdes.h,v 1.10 2003/09/12 22:29:25 mweiser Exp $"

// odd_parity[n] = (n & 0xFE) | b
// b set so that odd_parity[n] has an odd number of bits
extern const u8 odd_parity[256];

// convert to/from two different key formats
extern void convert_key_from_des_to_inc (u32 *deshi, u32 *deslo);
extern void convert_key_from_inc_to_des (u32 *deshi, u32 *deslo);

#endif /* __CONVDES_H__ */

