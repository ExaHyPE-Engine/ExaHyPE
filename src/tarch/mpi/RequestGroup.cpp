// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RequestGroup.h"

#include "tarch/mpi/Diagnostics.h"
#include "tarch/mpi/PollPacing.h"
#include "tarch/mpi/Request.h"

void tarch::mpi::RequestGroup::add(MPI_Request* request) { _requests.push_back(request); }

bool tarch::mpi::RequestGroup::testAll(std::source_location location) {
  bool allFinished = true;
  for (MPI_Request* const request : _requests) {
    int flag = 0;
    tarch::mpi::test(request, &flag, MPI_STATUS_IGNORE, location);
    allFinished &= (flag != 0);
  }
  return allFinished;
}

void tarch::mpi::RequestGroup::waitAll(
  const std::string&   className,
  const std::string&   methodName,
  std::source_location location
) {
  auto& diagnostics = tarch::mpi::Diagnostics::getInstance();
  diagnostics.setDeadlockWarningTimeStamp();
  diagnostics.setDeadlockTimeoutTimeStamp();

  tarch::mpi::PollPacing pacing;
  while (not testAll(location)) {
    if (pacing.reportProgressNow()) {
      diagnostics.writeTimeoutWarning(className, methodName, -1, -1, static_cast<int>(_requests.size()));
      diagnostics.triggerDeadlockTimeout(className, methodName, -1, -1, static_cast<int>(_requests.size()));
    }
  }
}

void tarch::mpi::RequestGroup::clear() { _requests.clear(); }
