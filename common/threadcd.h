/* Hey, Emacs, this a -*-C++-*- file !
 *
 * Copyright distributed.net 1997-2003 - All Rights Reserved
 * For use in distributed.net projects only.
 * Any other distribution or use of this source violates copyright.
 *
 * -----------------------------------------------------------------------
 * Once portable thread creation/destruction is guaranteed, it
 * isn't a big step to add functionality to the client - For example:
 * 1. a 'pipe' proxy (perhaps with IPX/NetBios support).
 * 2. a 'lurk' mechanism running asynchronously.
 * -----------------------------------------------------------------------
*/
#ifndef __THREADCD_H__
#define __THREADCD_H__ "@(#)$Id: threadcd.h,v 1.16 2003/09/12 22:29:26 mweiser Exp $"

#include "cputypes.h" /* THREADID typedef */

/* create a thread (blocks till running) - returns threadid or NULL if error */
extern THREADID CliCreateThread( register void (*proc)(void *), void *param );

/* destroy a thread (blocks till dead) */
extern int CliDestroyThread( THREADID cliThreadID );

#endif /* ifndef __CLITHREAD_H__ */
