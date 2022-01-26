// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "config.h"


#if !defined(_TARCH_MULTICORE_SMART_SCHEDULER_H_) and defined(UseSmartMPI)
#define _TARCH_MULTICORE_SMART_SCHEDULER_H_

#include "smartmpi.h"

namespace tarch {
  namespace multicore {
    /**
     * Register a new smart task
     *
     * The usage is as follows:
     *
     * - Each smart task has a static field storing the task's unique task type id (an
     *   integer).
     * - This value is passed to the SmartMPI constructor.
     * - The id is also handed into the present routine subject to a well-suited
     *   callback.
     *
     * As the routine pipes through the number argument (input=output), we can set up
     * the whole system in one static initialisation:
     *
     * @param task number This is the key, i.e. unique number of the task type. It has
     *   to be the same that is later uesd within the SmartMPI constructor.
     *
     * @return task number (unmodified). This allows us to use the routine within static
     *   initialisers.
     */
    int registerSmartMPITask(int taskTypeNumber, smartmpi::Receiver functor);
  }
}

#endif
