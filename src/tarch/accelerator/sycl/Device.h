// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(GPUOffloadingSYCL)

namespace tarch::accelerator {
  /**
   * Get a SYCL queue
   *
   * The Device singleton instantiates all SYCL queues. You can query the
   * singleton for the total number of (logical) devices. Per device,
   * we hold one SYCL queue persistently. You can gain access to this
   * one queue via this getter.
   *
   * number is a logical number, i.e. it is a number from 0 to
   * getNumberOfSYCLQueues(). The actual device number might be totally
   * different.
   *
   * If you use SYCL on the host, too, you should not access this queue
   * through he accelerator namespace, but use the tarch::multicore
   * namespace.
   *
   * @see tarch::multicore::getHostSYCLQueue()
   */
  void* getSYCLQueue(int number);
} // namespace tarch::accelerator

#endif
