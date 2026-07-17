// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Log.h"

#define logWarning(logMacroMessageStream) \
  do { \
    if (tarch::instrumentation::Log::isWarningOutputEnabled()) { \
      std::ostringstream conv; \
      conv << logMacroMessageStream; \
      tarch::instrumentation::Log::warningWithScopeName(TARCH_INSTRUMENTATION_SCOPE_NAME, conv.str()); \
    } \
  } while (false)
