// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DiagnosticsTest.h"

#include "tarch/mpi/Diagnostics.h"

namespace {
  /**
   * Turns the pending-request table on for one test case and puts the previous
   * diagnostics configuration back afterwards. Both thresholds are raised beyond
   * the test case's runtime, so exercising the table neither reports nor aborts.
   */
  class ScopedTracking {
  public:
    ScopedTracking() :
      _timeoutWarning(tarch::mpi::Diagnostics::getInstance().getTimeoutWarning().count()),
      _deadlockTimeout(tarch::mpi::Diagnostics::getInstance().getDeadlockTimeout().count()),
      _requestTracking(tarch::mpi::Diagnostics::getInstance().isRequestTrackingEnabled()) {
      auto& diagnostics = tarch::mpi::Diagnostics::getInstance();
      diagnostics.setTimeoutWarning(3600.0);
      diagnostics.setDeadlockTimeout(3600.0);
      diagnostics.setRequestTracking(true);
    }
    ~ScopedTracking() {
      auto& diagnostics = tarch::mpi::Diagnostics::getInstance();
      diagnostics.setRequestTracking(_requestTracking);
      diagnostics.setDeadlockTimeout(_deadlockTimeout);
      diagnostics.setTimeoutWarning(_timeoutWarning);
    }
    ScopedTracking(const ScopedTracking&)            = delete;
    ScopedTracking& operator=(const ScopedTracking&) = delete;
    ScopedTracking(ScopedTracking&&)                 = delete;
    ScopedTracking& operator=(ScopedTracking&&)      = delete;

  private:
    const double _timeoutWarning;
    const double _deadlockTimeout;
    const bool   _requestTracking;
  };

  /**
   * The table keys on the request handle's address and never hands the handle
   * to MPI, so a fabricated non-null handle value suffices for the test.
   */
  template <typename Request>
  Request fabricatedHandle() {
    if constexpr (std::is_pointer_v<Request>) {
      return reinterpret_cast<Request>(static_cast<std::uintptr_t>(0x1));
    } else {
      return static_cast<Request>(1);
    }
  }
} // namespace

tarch::mpi::tests::DiagnosticsTest::DiagnosticsTest() :
  TestCase("tarch::mpi::tests::DiagnosticsTest") {}

void tarch::mpi::tests::DiagnosticsTest::run() {
  testMethod(testRegisterReportClear);
  testMethod(testTableSurvivesConcurrentChurn);
}

/** A registered request is found and named, and a cleared one is gone. */
void tarch::mpi::tests::DiagnosticsTest::testRegisterReportClear() {
  ScopedTracking tracking;
  auto&          diagnostics = tarch::mpi::Diagnostics::getInstance();

  MPI_Request request = fabricatedHandle<MPI_Request>();
  diagnostics.registerRequest(&request, "MPI_Isend", 3, 7, std::source_location::current());
  validate(diagnostics.reportRequest(&request, "MPI_Wait", std::source_location::current()));

  diagnostics.clearRequest(&request);
  validate(not diagnostics.reportRequest(&request, "MPI_Wait", std::source_location::current()));
}

/**
 * Two threads register, report and clear disjoint request sets concurrently.
 * The slot protocol has no lock, so the test asserts that no entry is lost,
 * duplicated or left behind under churn.
 */
void tarch::mpi::tests::DiagnosticsTest::testTableSurvivesConcurrentChurn() {
  ScopedTracking tracking;
  auto&          diagnostics = tarch::mpi::Diagnostics::getInstance();

  constexpr int RequestsPerThread = 64;
  constexpr int Rounds            = 500;

  auto churn = [&diagnostics](std::array<MPI_Request, RequestsPerThread>& requests) {
    for (int round = 0; round < Rounds; round++) {
      for (auto& request : requests) {
        diagnostics.registerRequest(&request, "MPI_Irecv", 1, 2, std::source_location::current());
      }
      for (auto& request : requests) {
        diagnostics.reportRequest(&request, "MPI_Wait", std::source_location::current());
      }
      for (auto& request : requests) {
        diagnostics.clearRequest(&request);
      }
    }
  };

  std::array<MPI_Request, RequestsPerThread> firstSet;
  std::array<MPI_Request, RequestsPerThread> secondSet;
  firstSet.fill(fabricatedHandle<MPI_Request>());
  secondSet.fill(fabricatedHandle<MPI_Request>());

  std::thread partner(churn, std::ref(secondSet));
  churn(firstSet);
  partner.join();

  for (auto& request : firstSet) {
    validate(not diagnostics.reportRequest(&request, "MPI_Wait", std::source_location::current()));
  }
  for (auto& request : secondSet) {
    validate(not diagnostics.reportRequest(&request, "MPI_Wait", std::source_location::current()));
  }
}
