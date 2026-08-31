// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if PEANO_TRACE
#include "tarch/instrumentation/Log.h"

#define traceEnter(...) \
  do { \
    tarch::instrumentation::Log::enterTimedScope( \
      tarch::instrumentation::Log::makeTraceScopeName( \
        TARCH_LOGGING_FUNCTION_SIGNATURE, \
        TARCH_LOGGING_FUNCTION_NAME, \
        __FILE__ __VA_OPT__(, ) __VA_ARGS__ \
      ) \
    ); \
  } while (false)

#define traceLeave(...) \
  do { \
    tarch::instrumentation::Log::leaveTimedScope( \
      tarch::instrumentation::Log::makeTraceScopeName( \
        TARCH_LOGGING_FUNCTION_SIGNATURE, \
        TARCH_LOGGING_FUNCTION_NAME, \
        __FILE__ __VA_OPT__(, ) __VA_ARGS__ \
      ) \
    ); \
  } while (false)
#else
#define traceEnter(...) \
  do { \
  } while (false)
#define traceLeave(...) \
  do { \
  } while (false)
#endif
