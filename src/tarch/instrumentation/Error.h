// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/instrumentation/Log.h"

#define logError(logMacroMessageStream) \
  do { \
    std::ostringstream conv; \
    conv << logMacroMessageStream; \
    tarch::instrumentation::Log::errorWithScopeName(TARCH_INSTRUMENTATION_SCOPE_NAME, conv.str()); \
  } while (false)
