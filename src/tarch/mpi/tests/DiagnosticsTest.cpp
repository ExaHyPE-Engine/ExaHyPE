// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DiagnosticsTest.h"

#include "tarch/mpi/Diagnostics.h"

namespace {
  /**
   * Switches the pending-request table on for one test and restores the previous configuration in
   * its destructor. Both thresholds lie beyond the run time of the test, so the test neither warns
   * nor aborts.
   */
  class ScopedTracking {
  public:
    ScopedTracking() :
      _timeoutWarning(tarch::mpi::Diagnostics::getInstance().getTimeoutWarning().count()),
      _deadlockTimeout(tarch::mpi::Diagnostics::getInstance().getDeadlockTimeout().count()),
      _requestTrackingWasEnabled(tarch::mpi::Diagnostics::getInstance().isRequestTrackingEnabled()) {
      auto& diagnostics = tarch::mpi::Diagnostics::getInstance();
      diagnostics.setTimeoutWarning(3600.0);
      diagnostics.setDeadlockTimeout(3600.0);
      diagnostics.setRequestTracking(true);
    }
    ScopedTracking(const ScopedTracking&)            = delete;
    ScopedTracking(ScopedTracking&&)                 = delete;
    ScopedTracking& operator=(const ScopedTracking&) = delete;
    ScopedTracking& operator=(ScopedTracking&&)      = delete;
    ~ScopedTracking() {
      auto& diagnostics = tarch::mpi::Diagnostics::getInstance();
      diagnostics.setRequestTracking(_requestTrackingWasEnabled);
      diagnostics.setDeadlockTimeout(_deadlockTimeout);
      diagnostics.setTimeoutWarning(_timeoutWarning);
    }

  private:
    const double _timeoutWarning;
    const double _deadlockTimeout;
    const bool   _requestTrackingWasEnabled;
  };

  /**
   * Returns a handle other than MPI_REQUEST_NULL. The table keys on the address of the handle and
   * never passes the handle to MPI.
   */
  template <typename Request>
  Request getFabricatedHandle() {
    if constexpr (std::is_pointer_v<Request>) {
      return reinterpret_cast<Request>(static_cast<std::uintptr_t>(0x1));
    } else {
      return static_cast<Request>(1);
    }
  }
} // namespace

tarch::mpi::tests::DiagnosticsTest::DiagnosticsTest() :
  tarch::tests::TestCase("tarch::mpi::tests::DiagnosticsTest") {}

void
tarch::mpi::tests::DiagnosticsTest::run() {
  testMethod(testRegisterReportClear);
  testMethod(testTableSurvivesConcurrentChurn);
}

/** A registered request is found and named, and a cleared one is gone. */
void
tarch::mpi::tests::DiagnosticsTest::testRegisterReportClear() {
  const ScopedTracking tracking;
  auto&                diagnostics = tarch::mpi::Diagnostics::getInstance();

  const MPI_Request registeredRequest = getFabricatedHandle<MPI_Request>();
  diagnostics.registerRequest(&registeredRequest, "MPI_Isend", 3, 7, std::source_location::current());
  validate(diagnostics.reportRequest(&registeredRequest, "MPI_Wait", std::source_location::current()));

  diagnostics.clearRequest(&registeredRequest);
  validate(not diagnostics.reportRequest(&registeredRequest, "MPI_Wait", std::source_location::current()));
}

/** The table loses, duplicates or keeps no entry while two threads register, report and clear disjoint requests. */
void
tarch::mpi::tests::DiagnosticsTest::testTableSurvivesConcurrentChurn() {
  const ScopedTracking tracking;
  auto&                diagnostics = tarch::mpi::Diagnostics::getInstance();

  constexpr int RequestsPerThread = 64;
  constexpr int Rounds            = 500;

  const auto exerciseRequestTable = [&diagnostics](const std::array<MPI_Request, RequestsPerThread>& requests) {
    for (int round = 0; round < Rounds; round++) {
      for (const auto& request : requests) {
        diagnostics.registerRequest(&request, "MPI_Irecv", 1, 2, std::source_location::current());
      }
      for (const auto& request : requests) {
        diagnostics.reportRequest(&request, "MPI_Wait", std::source_location::current());
      }
      for (const auto& request : requests) {
        diagnostics.clearRequest(&request);
      }
    }
  };

  std::array<MPI_Request, RequestsPerThread> mainThreadRequests;
  std::array<MPI_Request, RequestsPerThread> partnerThreadRequests;
  mainThreadRequests.fill(getFabricatedHandle<MPI_Request>());
  partnerThreadRequests.fill(getFabricatedHandle<MPI_Request>());

  std::thread partnerThread(exerciseRequestTable, std::ref(partnerThreadRequests));
  exerciseRequestTable(mainThreadRequests);
  partnerThread.join();

  for (const auto& request : mainThreadRequests) {
    validate(not diagnostics.reportRequest(&request, "MPI_Wait", std::source_location::current()));
  }
  for (const auto& request : partnerThreadRequests) {
    validate(not diagnostics.reportRequest(&request, "MPI_Wait", std::source_location::current()));
  }
}
