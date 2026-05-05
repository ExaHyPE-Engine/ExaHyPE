// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Log.h"

#include <sstream>
#include <string>

#if PEANO_DEBUG
#define logDebug(logMacroMessageStream) \
  do { \
    const std::string logMacroScopeName = TARCH_INSTRUMENTATION_SCOPE_NAME; \
    if (tarch::instrumentation::Log::debugMessagesAreEnabled(logMacroScopeName)) { \
      std::ostringstream conv; \
      conv << logMacroMessageStream; \
      tarch::instrumentation::Log::debugWithScopeName(logMacroScopeName, conv.str()); \
    } \
  } while (false)
#else
#define logDebug(logMacroMessageStream) \
  do { \
  } while (false)
#endif
