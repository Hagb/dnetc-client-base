/* -*-C++-*-
 *
 * Copyright distributed.net 1997-2002 - All Rights Reserved
 * For use in distributed.net projects only.
 * Any other distribution or use of this source violates copyright.
 *
 * ** header is included from cores, so guard against c++ constructs **
 */

#ifndef __PROBLEM_H__
#define __PROBLEM_H__ "@(#)$Id: problem.h,v 1.94 2002/10/17 15:17:48 andreasb Exp $"

#include "cputypes.h" /* u32 */
#include "ccoreio.h"  /* Crypto core stuff (including RESULT_* enum members) */
#include "projdata.h" /* RC5, DES, ..., CONTEST_COUNT */
#include "selcore.h"
#if defined(HAVE_OGR_CORES)
#include "ogr.h"      /* OGR core stuff */
#endif

#if 0
// this moved to projdata.h
enum {
  RC5, // RC5-64, http://www.rsa.com/rsalabs/97challenge/
  DES, // http://www.rsa.com/rsalabs/des3/index.html
  OGR, // http://members.aol.com/golomb20/
  CSC, // http://www.cie-signaux.fr/security/index.htm
  OGR_NEXTGEN_SOMEDAY,
  RC5_72 // http://www.rsasecurity.com/rsalabs/challenges/secretkey/
};
#define CONTEST_COUNT       6  /* RC5,DES,OGR,CSC,OGR_NEXTGEN,RC5_72 */
#endif

/* ---------------------------------------------------------------------- */

#if defined(HAVE_RC5_64_CORES) || defined(HAVE_DES_CORES) \
    || defined(HAVE_CSC_CORES)
  #define HAVE_CRYPTO_V1
#endif
#if defined(HAVE_RC5_72_CORES)
  #define HAVE_CRYPTO_V2
#endif

/* ---------------------------------------------------------------------- */

#undef MAX_MEM_REQUIRED_BY_CORE
#define MAX_MEM_REQUIRED_BY_CORE  8  //64 bits
// Problem->core_membuffer should be aligned to 2^CORE_MEM_ALIGNMENT
#define CORE_MEM_ALIGNMENT 3

#if defined(HAVE_DES_CORES) && defined(MMX_BITSLICER)
  #if MAX_MEM_REQUIRED_BY_CORE < (17*1024)
     #undef MAX_MEM_REQUIRED_BY_CORE
     #define MAX_MEM_REQUIRED_BY_CORE (17*1024)
  #endif
#endif
#if defined(HAVE_CSC_CORES)
  #if MAX_MEM_REQUIRED_BY_CORE < (17*1024)
     #undef MAX_MEM_REQUIRED_BY_CORE
     #define MAX_MEM_REQUIRED_BY_CORE (17*1024)
  #endif
  // CSC membuffer should be aligned to a 16-byte boundary
  #if CORE_MEM_ALIGNMENT < 4
     #undef CORE_MEM_ALIGNMENT
     #define CORE_MEM_ALIGNMENT 4
  #endif
#endif
#if defined(HAVE_OGR_CORES)
  #if MAX_MEM_REQUIRED_BY_CORE < OGR_PROBLEM_SIZE
     #undef MAX_MEM_REQUIRED_BY_CORE
     #define MAX_MEM_REQUIRED_BY_CORE OGR_PROBLEM_SIZE
  #endif
  // OGR membuffer should be aligned to a 8-byte boundary
  // (essential for non-x86 CPUs)
  #if __VEC__ /* We might use AltiVec */
     #if CORE_MEM_ALIGNMENT < 4
       #undef CORE_MEM_ALIGNMENT
       #define CORE_MEM_ALIGNMENT 4
     #endif
  #else
     #if CORE_MEM_ALIGNMENT < 3
       #undef CORE_MEM_ALIGNMENT
       #define CORE_MEM_ALIGNMENT 3
     #endif
  #endif
#endif
//#if 0
//PROJECT_NOT_HANDLED("enter your core mem alignment here");
//#endif

/* ---------------------------------------------------------------------- */

#ifndef MIPSpro
#pragma pack(1)
#endif

typedef union
{
  #if defined(HAVE_CRYPTO_V1)  // used by RC5, DES, CSC
  struct {
    struct {u32 hi,lo;} key;              // starting key
    struct {u32 hi,lo;} iv;               // initialization vector
    struct {u32 hi,lo;} plain;            // plaintext we're searching for
    struct {u32 hi,lo;} cypher;           // cyphertext
    struct {u32 hi,lo;} keysdone;         // iterations done (also current position in block)
    struct {u32 hi,lo;} iterations;       // iterations to do
  } crypto;        /* 48 bytes */
  #endif
  #if defined(HAVE_CRYPTO_V2)
  struct {
    struct {u32 hi,mid,lo;} key;          // starting key
    struct {u32 hi,lo;} iv;               // initialization vector
    struct {u32 hi,lo;} plain;            // plaintext we're searching for
    struct {u32 hi,lo;} cypher;           // cyphertext
    struct {u32 hi,lo;} keysdone;         // iterations done (also current position in block)
    struct {u32 hi,lo;} iterations;       // iterations to do
    u32 randomsubspace;                   // subspace for random generation.
    struct {u32 count; u32 hi,mid,lo;} check;   // keyid of last found counter-measure check.
  } bigcrypto;     /* 68 bytes */
  #endif
  #if defined(HAVE_OGR_CORES)
  struct {
    struct WorkStub workstub;             // stub to work on (28 bytes)
    struct {u32 hi,lo;} nodes;            // nodes completed
  } ogr;           /* 36 bytes */
  #endif
  struct {
    char unused[80];
  } unused;
//  #if 0
//    PROJECT_NOT_HANDLED("in ContestWork");
//  #endif
} ContestWork;

typedef struct
{
  ContestWork work;/* {key,iv,plain,cypher,keysdone,iter} or {stub,pad} */
  u32  resultcode; /* core state: RESULT_WORKING:0|NOTHING:1|FOUND:2 */
  char id[64];     /* d.net id of worker that last used this */
  u32  contest;    /* 0=rc5,1=des,etc.  */
  u32  cpu;        /* CLIENT_CPU */
  u32  os;         /* CLIENT_OS */
  u32  build;      /* CLIENT_VERSION - combined build identifier */
  u32  core;       /* core used to process the packet */
} WorkRecord;

#ifndef MIPSpro
# pragma pack()
#endif /* ! MIPSpro */

/* ---------------------------------------------------------------------- */

struct problem_publics
{
  u32 elapsed_time_sec, elapsed_time_usec; /* wall clock time between
        start/finish, only valid after Run() returned RESULT_NOTHING/_FOUND */
  u32 runtime_sec, runtime_usec; /* ~total user time spent in core */
  u32 last_runtime_sec, last_runtime_usec; /* time spent in core in last run */
  int last_runtime_is_invalid; /* last_runtime was bad (clock change etc) */
  u32 core_run_count; /* used by go_mt and other things */

  struct
  { 
    u32 avg_coretime_usecs;
  } profiling;                   /* -- managed by non-preemptive OSs     */
  struct
  {
    u32 ccounthi, ccountlo;
    u32 ctimehi,  ctimelo;
    u32 utimehi,  utimelo;
    int init;
  } live_rate[2];                /* -- payload for ContestGetLiveRate    */

  u32 startpermille;             /* -,                                   */
  struct {u32 hi,lo;} startkeys;
  unsigned int contest;          /*  |__ assigned in LoadState()         */
  int coresel;                   /*  |                                   */
  int client_cpu;                /*  | effective CLIENT_CPU              */
  u32 tslice;                    /* -' -- adjusted by non-preemptive OSs */
  const char *was_truncated;     /* set (reason msg) if truncated        */
  int was_reset;                 /* set if loadstate reset the block     */
  int is_random;                 /* set if problem was RC5 'random'      */
  int is_benchmark;              /* set if problem is benchmark          */

  int loaderflags; /* used by problem loader (probfill.cpp) */

  unsigned int pipeline_count;
  unit_func_union unit_func;
  int use_generic_proto; /* RC5/DES unit_func prototype is generic form */
  int cruncher_is_asynchronous; /* on a co-processor or similar */
  int cruncher_is_time_constrained; /* non-preemptive or real-time OS */
};

typedef struct
{
  struct problem_publics pub_data;
} Problem;

/* ---------------------------------------------------------------------- */

/*
 * in the following functions that take a __thisprob argument, __thisprob
 * must be a void * to suppress the name mangling for struct Problem.
*/

// Load state into internal structures.
// state is invalid (will generate errors) until this is called.
// expected_[core|cpu|os|buildnum] are those loaded with the workunit
//   and allow LoadState to reset the problem if deemed necessary.
// returns: -1 on error, 0 is OK
// LoadState() and RetrieveState() work in pairs. A LoadState() without
// a previous RetrieveState(,,purge) will fail, and vice-versa.

#define CONTESTWORK_MAGIC_RANDOM    ((const ContestWork *)0)
#define CONTESTWORK_MAGIC_BENCHMARK ((const ContestWork *)1)
int ProblemLoadState( void *__thisprob,
                      const ContestWork * work, unsigned int _contest, 
                      u32 _iterations, int expected_cpunum, 
                      int expected_corenum,
                      int expected_os, int expected_buildfrac );

// Retrieve state from internal structures.
// state is invalid (will generate errors) once the state is purged.
// 'dontwait' signifies that the purge need not wait for the cruncher
// to be in a stable state before purging. *not* waiting is necessary
// when the client is aborting (in which case threads may be hung).
// Returns RESULT_* or -1 if error.
// LoadState() and RetrieveState() work in pairs. A LoadState() without
// a previous RetrieveState(,,purge) will fail, and vice-versa.
int ProblemRetrieveState( void *__thisprob,
                          ContestWork * work, unsigned int *contestid, 
                          int dopurge, int dontwait );


// is the problem initialized? (LoadState() successful, no RetrieveState yet)
// returns > 0 if completed (RESULT_xxx), < 0 if still working
int ProblemIsInitialized(void *__thisprob);

// Runs calling unit_func for iterations times...
// Returns RESULT_* or -1 if error.
int ProblemRun(void *__thisprob);

typedef struct ProblemInfo {
  const char *name;                 // name for current contest
  const char *unit;                 // unit name for current contest
  u32 elapsed_secs;                 // elapsed core runtime so far.
  u32 elapsed_usecs;
  u32 swucount;                     // no. of work units problem has loaded
  u32 c_permille;                   // current permille
  u32 s_permille;                   // start   permille
  int is_test_packet;               // RC5: iterations == 0x001000000
  int show_exact_iterations_done;   // OGR: log exact nodecount
  int stats_units_are_integer;
  u32 ratehi, ratelo;               // core rate
  u32 tcounthi, tcountlo;           // total number of iterations (n/a if not finished)
  u32 ccounthi, ccountlo;           // number of iterations done this time
  u32 dcounthi, dcountlo;           // number of iterations done ever
  char sigbuf[32];                  // packet identifier
  char cwpbuf[32];                  // current working position
  struct
  {
    char *ratebuf;
    unsigned int size;
  } rate;
} ProblemInfo;

#define P_INFO_E_TIME      0x00000001
#define P_INFO_SWUCOUNT    0x00000002
#define P_INFO_C_PERMIL    0x00000004
#define P_INFO_S_PERMIL    0x00000008
#define P_INFO_RATE        0x00000010
#define P_INFO_TCOUNT      0x00000020
#define P_INFO_CCOUNT      0x00000040
#define P_INFO_DCOUNT      0x00000080
#define P_INFO_RATEBUF     0x00000100
#define P_INFO_SIGBUF      0x00000200
#define P_INFO_CWPBUF      0x00000400
#define P_INFO_EXACT_PE    0x00000800

// returns RESULT_* or -1 if bad state
int ProblemGetInfo(void *__thisprob, ProblemInfo *info, long flags);

Problem *ProblemAlloc(void);
void ProblemFree(void *__thisprob);

/* Get the number of problems for a particular contest, */
/*  or, if contestid is -1 then the total for all contests */
unsigned int ProblemCountLoaded(int contestid);

/* Get the size of the problem (which is not! sizeof(Problem) */
/* used for IPC, shmem et al */
unsigned int ProblemGetSize(void);

const char *ProblemComputeRate( unsigned int contestid, 
                                u32 secs, u32 usecs, u32 iterhi, u32 iterlo, 
                                u32 *ratehi, u32 *ratelo,
                                char *ratebuf, unsigned int ratebufsz ); 

int WorkGetSWUCount( const ContestWork *work,
                     int rescode, unsigned int contestid,
                     unsigned int *swucount );

// numst_style: -1=unformatted, 0=commas, 1=0+space between magna and number (or at end),
//               2=1+"nodes"/"keys"
char *U64stringify(char *buffer, unsigned int buflen, u32 hi, u32 lo,
                            int numstr_style, const char *numstr_suffix );

/* result depends on #ifdefs, threadsafety issues etc */
int IsProblemLoadPermitted(long prob_index, unsigned int contest_i);

// this is a bit misplaced here, but we need u64-ops
int ProjectSetSpeed(int projectid, u32 speedhi, u32 speedlo);

#endif /* __PROBLEM_H__ */
