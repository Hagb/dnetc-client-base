/*
 * Copyright distributed.net 1999-2002 - All Rights Reserved
 * For use in distributed.net projects only.
 * Any other distribution or use of this source violates copyright.
 *
*/
#ifndef __OGR_H__
#define __OGR_H__ "@(#)$Id: ogr.h,v 1.2.4.6 2003/08/25 08:37:59 mweiser Exp $"

#ifndef u16
#include "cputypes.h"
#endif

#include <limits.h>
#if (UINT_MAX < 0xfffffffful)
  #error "ogr needs an int thats not less than 32bits"
#elif (UINT_MAX == 0xffffffff)
  #define OGR_INT_SIZE 4
#elif (UINT_MAX == 0xffffffffffffffff)
  #define OGR_INT_SIZE 8
#else
  #error "What's up Doc?"
#endif

#if (CLIENT_CPU == CPU_POWERPC) && (defined(__GCC__) || defined(__GNUC__)) && \
    (defined(__VEC__) || defined(__ALTIVEC__)) && (!defined(__APPLE_CC__))
  #include <altivec.h>
#endif

/* ===================================================================== */

/*
 * Constants for return values from all the below functions.
 * Those starting with CORE_S are success codes, and those starting
 * with CORE_E are error codes.
 */
#define CORE_S_OK       0
#define CORE_S_CONTINUE 1
#define CORE_S_SUCCESS  2
#define CORE_E_MEMORY   (-1)
#define CORE_E_IO       (-2)
#define CORE_E_FORMAT   (-3)
#define CORE_E_STOPPED  (-4)
#define CORE_E_STUB     (-5)

#include "pack1.h"

/*
 * Dispatch table structure. A pointer to one of these should be returned
 * from an exported function in the module.
 *
 * The result callback is weird, it implies that only one client in the
 * entire process will use this core (but that client can have multiple
 * threads). Oh well.
 */
typedef struct {
  /*
   * Initialize the core, called once for all threads.
   */
  int (*init)(void);

  /*
   * Create a new work unit, called once for each thread.
   * The format of input is defined by the core.
   */
  int (*create)(void *input, int inputlen, void *state, int statelen);

  /*
   * Continue working, return CORE_S_OK if no more work to do, or
   * CORE_S_CONTINUE if things need to keep going.
   * On input, nodes should contain the number of algorithm iterations
   * to do. On output, nodes will contain the actual number of iterations
   * done.
   *
   * If with_time_constraints is 0, the OGR cruncher uses the nodeslimit 
   * merely as a hint when to leave the cruncher rather than adhering to it 
   * exactly. There is a slight speed increase (negligible in a preemptive 
   * non-realtime environment) when applying the hint scheme, but cannot 
   * be used in a cruncher that is bound by time constraints (eg 
   * non-preemptive or real-time environments) because it can happen 
   * that a nodeslimit request of 1000 will end up with 32000 nodes done.
  */
  int (*cycle)(void *state, int *steps, int with_time_constraints);

  /*
   * If cycle returns CORE_S_SUCCESS, call getresult to get the successful
   * result. If called at other times, returns the current state of the 
   * search.
   */
  int (*getresult)(void *state, void *result, int resultlen);

  /*
   * Clean up state structure.
   */
  int (*destroy)(void *state);

#if 0
  /*
   * Return the number of bytes needed to serialize this state.
   */
  int (*count)(void *state);

  /*
   * Serialize the state into a flat data structure suitable for
   * persistent storage.
   * buflen must be at least as large as the number of bytes returned
   * by count().
   * Does not destroy the state structure.
   */
  int (*save)(void *state, void *buffer, int buflen);

  /*
   * Load the state from persistent storage buffer.
   */
  int (*load)(void *buffer, int buflen, void **state);
#endif

  /*
   * Clean up anything allocated in init().
   */
  int (*cleanup)(void);

} DNETC_PACKED CoreDispatchTable;

#if (!defined(__GNUC__) || (__GNUC__ < 2) || \
     ((__GNUC__ == 2) && (__GNUC_MINOR__ < 91))) && \
    !defined(MIPSpro) && !defined(__SUNPRO_CC)
# pragma pack()
#endif

/* ===================================================================== */

// define this to enable LOGGING code
#undef OGR_DEBUG
//#define OGR_PROFILE
// OGR_WINDOW is used to test register windowing in the core
//#define OGR_WINDOW 10

// specifies the number of ruler diffs can be represented.
// Warning: increasing this will cause all structures based
// on workunit_t in packets.h to change, possibly breaking
// network and buffer structure operations.
#define STUB_MAX 10

struct Stub { /* size is 24 */
  u16 marks;           /* N-mark ruler to which this stub applies */
  u16 length;          /* number of valid elements in the stub[] array */
  u16 diffs[STUB_MAX]; /* first <length> differences in ruler */
} DNETC_PACKED;

struct WorkStub { /* size is 28 */
  Stub stub;           /* stub we're working on */
  u32 worklength;      /* depth of current state */
} DNETC_PACKED;

#include "pack0.h"

// Internal stuff that's not part of the interface but we need for
// declaring the problem work area size.

// I have to reserve memory for all possible OGR cruncher setups because
// memory reservation happens inside problem.h/.c and I cannot know what
// cruncher is going to get used :(

#define BITMAPS     5       /* need to change macros when changing this */
#define MAXDEPTH   40

typedef u32 U;

struct Level {
  /* If AltiVec is possible we must reserve memory, just in case */
  #ifdef __VEC__   // unused if OGROPT_ALTERNATE_CYCLE == 0 || == 1
  vector unsigned int listV0, listV1, compV0, compV1;
  #endif
  U list[BITMAPS]; // unused if OGROPT_ALTERNATE_CYCLE == 2
  U dist[BITMAPS]; // unused if OGROPT_ALTERNATE_CYCLE == 1 || 2
  U comp[BITMAPS]; // unused if OGROPT_ALTERNATE_CYCLE == 2
  int cnt1;        // unused if OGROPT_ALTERNATE_CYCLE == 1 || == 2
  int cnt2;        // always needed
  int limit;       // always needed
};

#define OGR_LEVEL_SIZE ((128*4)+((4*BITMAPS)*3)+(OGR_INT_SIZE*3))

struct State {
  #if 0 /* unused - see notes for ogr_cycle() above */
  struct { U hi,lo; } Nodes;      /* counts "tree branches" */
  //double Nodes;                 /* counts "tree branches" */  
  #endif
  int max;                        /* maximum length of ruler */
  int maxdepth;                   /* maximum number of marks in ruler */
  int maxdepthm1;                 /* maxdepth-1 */
  int half_length;                /* half of max */
  int half_depth;                 /* half of maxdepth */
  int half_depth2;                /* half of maxdepth, adjusted for 2nd mark */
  int marks[MAXDEPTH+1];          /* current length */
  int startdepth;
  int depth;
  int limit;
  #ifdef OGR_DEBUG
    int LOGGING;
  #endif
  #ifdef  OGR_WINDOW /* used by OGRtestbench */
    int wind;                     /* depth window base */
    int turn;                     /* window turn counter */
  #endif
  #ifdef OGR_PROFILE /* used by OGRtestbench */
    struct {
      long hd;                    /* Half depth */
      long hd2;                   /* Half depth 2 */
      long ghd;                   /* Greater than Half depth */
      long lt16;                  /* shift <16 */
      long lt32;                  /* shift < 32 */
      long ge32;                  /* shift >= 32 */
      long fo;                    /* found one? */
      long push;                  /* Go deeper */
   } prof;
  #endif
  /* If AltiVec is possible we must reserve memory, just in case */
  #ifdef __VEC__     /* only used by OGROPT_ALTERNATE_CYCLE == 2 */
    vector unsigned int distV0, distV1;
  #endif
  U dist[BITMAPS];   /* only used by OGROPT_ALTERNATE_CYCLE == 1 */
  struct Level Levels[MAXDEPTH];
};

#define OGR_PROBLEM_SIZE (/*16+*/(6*OGR_INT_SIZE)+(OGR_INT_SIZE*(MAXDEPTH+1))+ \
                         (4*OGR_INT_SIZE)+(128*2)+(OGR_INT_SIZE*BITMAPS)+ \
                         (OGR_LEVEL_SIZE*MAXDEPTH)+64)
                         /* sizeof(struct State) */

unsigned long ogr_nodecount(const struct Stub *);
const char *ogr_stubstr_r(const struct Stub *stub,
                          char *buffer, unsigned int bufflen,
                          int worklength);
const char *ogr_stubstr(const struct Stub *stub);

#endif /* __OGR_H__ */

