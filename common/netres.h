/* Hey, Emacs, this a -*-C++-*- file !
 *
 * Copyright distributed.net 1997-2003 - All Rights Reserved
 * For use in distributed.net projects only.
 * Any other distribution or use of this source violates copyright.
*/

#ifndef __NETRES_H__
#define __NETRES_H__ "@(#)$Id: netres.h,v 1.8 2008/12/19 11:10:59 andreasb Exp $"

int NetResolve( const char *host, int port, int resauto,
                u32 *addrlist, unsigned int addrlistcount,
                char *resolve_hostname, unsigned int resolve_hostname_sz);

#endif /* __NETRES_H__ */
