/* Hey, Emacs, this a -*-C++-*- file !
 *
 * Copyright distributed.net 1997-2002 - All Rights Reserved
 * For use in distributed.net projects only.
 * Any other distribution or use of this source violates copyright.
*/ 
#ifndef __CMDLINE_H__
#define __CMDLINE_H__ "@(#)$Id: cmdline.h,v 1.7 2002/09/02 00:35:41 andreasb Exp $"

// runlevel=0 = parse cmdline, >0==exec modes && print messages
// returns !0 if app should be terminated; (retcodeP then has exit code)
int ParseCommandline( Client *client, 
                      int run_level, int argc, const char *argv[], 
                      int *retcodeP, int restarted );

#endif /* __CMDLINE_H__ */
