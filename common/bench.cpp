/* 
 * Copyright distributed.net 1997-2002 - All Rights Reserved
 * For use in distributed.net projects only.
 * Any other distribution or use of this source violates copyright.
*/
const char *bench_cpp(void) {
return "@(#)$Id: bench.cpp,v 1.27.2.62 2002/03/28 01:07:37 andreasb Exp $"; }

//#define TRACE

#include "cputypes.h"  // CLIENT_OS, CLIENT_CPU
#include "baseincs.h"  // general includes
#include "client.h"    // contest enum
#include "problem.h"   // Problem class
#include "triggers.h"  // CheckExitRequestTriggerNoIO()
#include "clitime.h"   // CliGetTimeString()
#include "clicdata.h"  // GetContestNameFromID()
#include "selcore.h"   // selcoreGet[SelectedCoreForContest|DisplayName]()
#include "pollsys.h"   // NonPolledUSleep()
#include "logstuff.h"  // LogScreen()
#include "clievent.h"  // event post etc.
#include "bench.h"     // ourselves
#include "util.h"      // TRACE_OUT

#define TBENCHMARK_CALIBRATION 0x80

#if (CONTEST_COUNT != 4)
  #error static initializer expects CONTEST_COUNT == 4
#endif
unsigned long bestrate_tab[CONTEST_COUNT] = {0,0,0,0};

/* -------------------------------------------------------------------- */

/* BenchGetBestRate() is always per-processor */
unsigned long BenchGetBestRate(unsigned int contestid)
{
  if (contestid < CONTEST_COUNT)
  { 
    if (bestrate_tab[contestid] == 0)
    { 
      // This may trigger a mini-benchmark, which will get the speed
      // we need and not waste time.
      selcoreGetSelectedCoreForContest( contestid );
    }
    if (bestrate_tab[contestid] == 0)
    { 
      TBenchmark(contestid, 2, 
                 TBENCHMARK_CALIBRATION|TBENCHMARK_QUIET|TBENCHMARK_IGNBRK);
    }
    return bestrate_tab[contestid];
  }
  return 0;
}

/* -------------------------------------------------------------------- */

static inline void __BenchSetBestRate(unsigned int contestid, unsigned long rate)
{
  if (contestid < CONTEST_COUNT)
  { 
    if (rate > bestrate_tab[contestid])
      bestrate_tab[contestid] = rate;
  }
  return;
}

/* -------------------------------------------------------------------- */

/* TBenchmark() is always per-processor */
long TBenchmark( unsigned int contestid, unsigned int numsecs, int flags )
{
  /* non-preemptive os minimum yields per second */
  struct { int yps, did_adjust; } non_preemptive_os;
  long retvalue = -1L; /* assume error */
  Problem *thisprob; 
  u32 tslice; 

  if (!IsProblemLoadPermitted(-1 /*any thread*/, contestid))
    return 0;

  TRACE_OUT((+1,"TBenchmark(%u, %u, %d)\n",contestid,numsecs,flags));
  
  /* ++++++ determine initial 'timeslice' +++++ */

  tslice = 0; /* zero means 'use calibrated value' */
  non_preemptive_os.yps = 0; /* assume preemptive OS */
  non_preemptive_os.did_adjust = 0;

  #if (CLIENT_OS == OS_NETWARE)
  if ( ( flags & TBENCHMARK_CALIBRATION ) != 0 ) // 2 seconds without yield
    numsecs = ((numsecs > 2) ? (2) : (numsecs)); // ... is acceptable
  else
  {
    non_preemptive_os.yps = 1000/10; /* 10 ms minimum yield rate */ 
    tslice = 0; /* zero means 'use calibrated value' */
  }  
  #elif (CLIENT_OS == OS_MACOS)
  if ( ( flags & TBENCHMARK_CALIBRATION ) != 0 ) // 2 seconds without yield
    numsecs = ((numsecs > 2) ? (2) : (numsecs)); // ... is acceptable
  else
  {    
    non_preemptive_os.yps = 1000/20; /* 20 ms minimum yield rate */
    tslice = 0; /* zero means 'use calibrated value' */
  }
  #elif (CLIENT_OS == OS_WIN16 || CLIENT_OS == OS_WIN32 /* win32s */) 
  if ( ( flags & TBENCHMARK_CALIBRATION ) != 0 ) // 2 seconds without yield
    numsecs = ((numsecs > 2) ? (2) : (numsecs)); // ... is acceptable
  else if (winGetVersion() < 400) /* win16 or win32s */
  {  
    non_preemptive_os.yps = 1000/20; /* 20 ms minimum yield rate */ 
    tslice = 0;  /* zero means 'use calibrated value' */
  } 
  #elif (CLIENT_OS == OS_RISCOS)
  if ( ( flags & TBENCHMARK_CALIBRATION ) != 0 ) // 2 seconds without yield
    numsecs = ((numsecs > 2) ? (2) : (numsecs)); // ... is acceptable
  else if (riscos_check_taskwindow())
  {
    non_preemptive_os.yps = 1000/100; /* 100 ms minimum yield rate */ 
    tslice = 0;  /* zero means 'use calibrated value' */
  }
  #endif

  if (tslice == 0 && ( flags & TBENCHMARK_CALIBRATION ) == 0 )
  {
    long res;
    if ((flags & TBENCHMARK_QUIET) == 0)
    {
      selcoreGetSelectedCoreForContest(contestid); /* let selcore message */
      //LogScreen("Calibrating ... " );
    }  
    res = TBenchmark( contestid, 2, TBENCHMARK_QUIET | TBENCHMARK_IGNBRK | TBENCHMARK_CALIBRATION );
    if ( res == -1 ) /* LoadState failed */
    {
      if ((flags & TBENCHMARK_QUIET) == 0)
      {
      #if ((CLIENT_OS == OS_WIN32) && defined(SMC))
        // HACK! to ignore failed benchmark for x86 rc5 smc core #7 if
        // started from menu and another cruncher is active in background.
        if (contestid == RC5 && selcoreGetSelectedCoreForContest(contestid) == 7)
          LogScreen("\rCan't benchmark core #7 while another cruncher\nis running in the background.\n");
        else
          LogScreen("\rCalibration failed!\n");
      #else
        LogScreen("\rCalibration failed!\n");
      #endif
      }
      TRACE_OUT((-1,"TBenchmark()=-1 (Calibration failed)\n"));
      return -1;
    }  
    tslice = (((u32)res) + 0xFFF) & 0xFFFFF000;
    //if ((flags & TBENCHMARK_QUIET) == 0)
    //  LogScreen("\rCalibrating ... done. (%lu)\n", (unsigned long)tslice );
    if (non_preemptive_os.yps)
      tslice /= non_preemptive_os.yps;
//  else
//    tslice <<= 1; /* try for two second steps */
  }
  if (tslice == 0)
  { 
    tslice = 0x10000;
    if (non_preemptive_os.yps)
      tslice = 4096;
  }

  /* ++++++ run the benchmark +++++ */
  
  thisprob = ProblemAlloc();
  if (thisprob)
  {
    if ( ProblemLoadState( thisprob, CONTESTWORK_MAGIC_BENCHMARK, 
                             contestid, tslice, 0, 0, 0, 0) == 0)
    {
      const char *contname = CliGetContestNameFromID(contestid);
      int silent = 1, run = RESULT_WORKING; u32 bestlo = 0, besthi = 0;
      unsigned long last_permille = 1001;

      //ClientEventSyncPost(CLIEVENT_BENCHMARK_STARTED, (long)thisprob );
      if ((flags & TBENCHMARK_QUIET) == 0)
      {
        silent = 0;
        LogScreen("%s: Benchmarking ... ", contname );
      }

      /* Sleep a bit to a) try to begin the while loop at the top of a 
      ** scheduling quantum, b) Also, per-contest bench can be very stressful 
      ** if there are a lot of cores per contest.
      */
      NonPolledUSleep(50000); /* 50 millisecs */

      while ( run == RESULT_WORKING )
      {
        unsigned long permille; u32 ratehi, ratelo;

        if (non_preemptive_os.yps) /* is this a non-preemptive environment? */
        {
          if (!thisprob->pub_data.last_runtime_is_invalid &&
             non_preemptive_os.did_adjust < 30 /* don't do this too often */
             && thisprob->pub_data.runtime_sec >= ((u32)(2+non_preemptive_os.did_adjust)))
          {
            u32 newtslice;
            ProblemInfo info;
            if (ProblemGetInfo(thisprob, &info, P_INFO_RATE) == -1)
            {
              run = -2;
              break;
            }
            ratelo = info.ratelo;
            newtslice = (u32)(ratelo/((u32)non_preemptive_os.yps));
            if (newtslice > (tslice + (tslice/10)))
            {
              non_preemptive_os.did_adjust++;
              numsecs++; /* bench for a bit more */
            }
            if (newtslice > tslice)
              thisprob->pub_data.tslice = tslice = newtslice;
          }
          #if (CLIENT_OS == OS_WIN16) || (CLIENT_OS == OS_WIN32) /* win32s */
          w32Yield(); /* pump waiting messages */
          #elif (CLIENT_OS == OS_MACOS)
          macosSmartYield(6);
          #elif (CLIENT_OS == OS_RISCOS)
          riscos_upcall_6();
          #elif (CLIENT_OS == OS_NETWARE)
          ThreadSwitchLowPriority();
          #endif
        }
        else /* preemptive environment */ 
             if (!thisprob->pub_data.last_runtime_is_invalid)
        {
          ProblemInfo info;
          if (ProblemGetInfo(thisprob, &info, P_INFO_DCOUNT) == -1)
          {
            run = -2;
            break;
          }
          ProblemComputeRate( contestid, thisprob->pub_data.runtime_sec, 
                                         thisprob->pub_data.runtime_usec,
                                         info.dcounthi, info.dcountlo,
                                         &ratehi, &ratelo, 0, 0 );

          tslice = thisprob->pub_data.tslice;
          if (ratehi > besthi || (ratehi == besthi && ratelo > bestlo))
          {
            bestlo = ratelo; 
            besthi = ratehi;
//printf("\noldtslice=%u, newtslice=%u %s\n", tslice, ratelo, "BEST!");
          }
          else
          { 
            ratehi = besthi;
            ratelo = bestlo;
//printf("\noldtslice=%u, newtslice=%u %s\n", tslice, ratelo, "");
          }
          if (ratehi)
            ratelo = 0x0fffffff;
          if (ratelo > tslice || contestid == OGR)
            tslice = thisprob->pub_data.tslice = ratelo;
        }
        run = ProblemRun(thisprob);
        if ( run < 0 )
        {
          run = -1;
          break;
        }   
        if ((flags & TBENCHMARK_IGNBRK)==0 && CheckExitRequestTriggerNoIO())
        {
          if (!silent)
            LogScreen("\r%s: Benchmarking ... *Break*", contname );
          run = -3;
          break;
        }
        permille = 1000; /* assume finished */
        if (run == RESULT_WORKING) /* not finished */
        {
          permille = ((thisprob->pub_data.runtime_sec  * 1000) + 
                      (thisprob->pub_data.runtime_usec / 1000)) / numsecs;
          if (permille > 1000)
            permille = 1000;
        }
        if (permille == 1000 || last_permille != (permille / 10))
        {    
          if (!silent)
            LogScreen("\r%s: Benchmarking ... %u.%02u%% done", 
                       contname, (unsigned int)(permille/10), 
                               (unsigned int)((permille%10)*10) );
          last_permille = (permille / 10);
          //ClientEventSyncPost(CLIEVENT_BENCHMARK_BENCHING, (long)problem );
        }                                 
        if (permille == 1000) /* time is up or ran out of work */
        {
          char ratebuf[32];
          ProblemInfo info;
          info.rate.ratebuf = ratebuf;
          info.rate.size = sizeof(ratebuf);
          
          if (ProblemGetInfo(thisprob, &info, P_INFO_E_TIME | P_INFO_RATEBUF |
                                              P_INFO_RATE) == -1)
          {
            run = -4;
            break;
          }
          
          if (bestlo || besthi)
          {
            ProblemComputeRate( contestid, 0, 0, besthi, bestlo, &(info.ratehi),
                                &(info.ratelo), info.rate.ratebuf, info.rate.size );
          }
          retvalue = (long)info.ratelo;
          #if (ULONG_MAX > 0xfffffffful)
          retvalue = (long)((info.ratehi << 32)+(info.ratelo));
          #endif
          __BenchSetBestRate(contestid, retvalue);
          if (!silent)
          {
            struct timeval tv; 
            tv.tv_sec = info.elapsed_secs; tv.tv_usec = info.elapsed_usecs;
            LogScreen("\r");
            Log("%s: Benchmark for core #%d (%s)\n%s [%s/sec]\n",
               contname, thisprob->pub_data.coresel, 
               selcoreGetDisplayName(contestid, thisprob->pub_data.coresel),
               CliGetTimeString( &tv, 2 ), info.rate.ratebuf );
          }
          //ClientEventSyncPost(CLIEVENT_BENCHMARK_FINISHED, (long)problem );
          break;
        } /* permille == 1000 */
      } /* while ( run == RESULT_WORKING ) */

      if (!silent)
      {
        if (run < 0) /* error */
        {
          LogScreen("\r");
          if (run != -3) /* break */
            Log("%s: Benchmark failed (error: %d).", contname, run );
        }
      }
      ProblemRetrieveState(thisprob, NULL, NULL, 1, 0); //purge the problem

    } /* if (LoadState() == 0) */

    ProblemFree(thisprob);
  }

  TRACE_OUT((-1,"TBenchmark()=%d\n", retvalue));
  return retvalue;
}  
