// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Log.h"

#include <sstream>
#include <string>

#if PEANO_DEBUG
#define logDebug(logMacroMessageStream) \
  do { \
    tarch::instrumentation::Log:: \
      debugIfEnabled(TARCH_LOGGING_FUNCTION_SIGNATURE, TARCH_LOGGING_FUNCTION_NAME, __FILE__, [&]() -> std::string { \
        std::ostringstream conv; \
        conv << logMacroMessageStream; \
        return conv.str(); \
      }); \
  } while (false)
#else
#define logDebug(logMacroMessageStream) \
  do { \
  } while (false)
#endif
