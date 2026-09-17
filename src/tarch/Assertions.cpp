// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Assertions.h"

#if PEANO_ASSERTIONS
#include "tarch/Termination.h"

void tarch::triggerAssertion(const std::string& assertionMessage) { tarch::terminateWithError(assertionMessage); }
#endif
