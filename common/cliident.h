/* Hey, Emacs, this a -*-C++-*- file !
 *
 * Copyright distributed.net 1997-2002 - All Rights Reserved
 * For use in distributed.net projects only.
 * Any other distribution or use of this source violates copyright.
*/ 
#ifndef __CLIIDENT_H__
#define __CLIIDENT_H__ "@(#)$Id: cliident.h,v 1.10 2002/09/02 01:15:45 andreasb Exp $"

extern void CliIdentifyModules(void);
extern time_t CliGetNewestModuleTime(void);
extern int CliIsDevelVersion(void);
extern const char *CliGetFullVersionDescriptor(void);

#endif /* __CLIIDENT_H__ */
