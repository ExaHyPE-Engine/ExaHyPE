// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

#include <mpi.h>

namespace tarch::mpi {
  class RequestGroup;
} // namespace tarch::mpi

/**
 * Completes a set of outstanding requests under one watchdog.
 *
 * A caller that posts several non-blocking operations and needs them all done
 * adds their handles and calls waitAll(), which polls the whole set, warns
 * once the timeout-warning threshold passes and aborts once the deadlock
 * threshold passes. The group holds pointers to the caller's request handles,
 * so the handles have to outlive the wait; the group never owns them.
 */
class tarch::mpi::RequestGroup final {
public:
  void add(MPI_Request* request);

  /**
   * One poll over the set. Returns whether every request has completed. A
   * completed request stays in the set, where testing it again is free, so a
   * caller may poll interleaved with work of its own.
   */
  bool testAll(std::source_location location = std::source_location::current());

  void waitAll(
    const std::string&   className,
    const std::string&   methodName,
    std::source_location location = std::source_location::current()
  );

  void clear();

private:
  std::vector<MPI_Request*> _requests;
};
