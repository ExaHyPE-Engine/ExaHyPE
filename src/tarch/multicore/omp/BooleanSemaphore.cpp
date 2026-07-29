// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/multicore/multicore.h"

#if defined(PEANO_WITH_OPENMP)
#include "tarch/instrumentation/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Core.h"

tarch::multicore::BooleanSemaphore::BooleanSemaphore() { omp_init_lock(&_lock); }

tarch::multicore::BooleanSemaphore::~BooleanSemaphore() {}

void tarch::multicore::BooleanSemaphore::enterCriticalSection() { omp_set_lock(&_lock); }

void tarch::multicore::BooleanSemaphore::leaveCriticalSection() { omp_unset_lock(&_lock); }

bool tarch::multicore::BooleanSemaphore::tryEnterCriticalSection() { return omp_test_lock(&_lock); }

#endif
