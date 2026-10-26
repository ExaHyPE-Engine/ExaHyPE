// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/mpi/Diagnostics.h"

#include "tarch/testing/UnitTest.h"

namespace {
  /**
   * Switches the pending-request table on for one test and restores the previous configuration in
   * its destructor. Both thresholds lie beyond the run time of the test.
   */
  class ScopedTracking final {
  public:
    ScopedTracking() {
      auto& diagnostics = tarch::mpi::Diagnostics::getInstance();
      diagnostics.setTimeoutWarning(3600.0);
      diagnostics.setDeadlockTimeout(3600.0);
      diagnostics.setRequestTracking(true);
    }
    ScopedTracking(const ScopedTracking& other)            = delete;
    ScopedTracking(ScopedTracking&& other)                 = delete;
    ScopedTracking& operator=(const ScopedTracking& other) = delete;
    ScopedTracking& operator=(ScopedTracking&& other)      = delete;
    ~ScopedTracking() {
      auto& diagnostics = tarch::mpi::Diagnostics::getInstance();
      diagnostics.setRequestTracking(_requestTrackingWasEnabled);
      diagnostics.setDeadlockTimeout(_deadlockTimeout);
      diagnostics.setTimeoutWarning(_timeoutWarning);
    }

  private:
    const double _timeoutWarning{tarch::mpi::Diagnostics::getInstance().getTimeoutWarning().count()};
    const double _deadlockTimeout{tarch::mpi::Diagnostics::getInstance().getDeadlockTimeout().count()};
    const bool   _requestTrackingWasEnabled{tarch::mpi::Diagnostics::getInstance().isRequestTrackingEnabled()};
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

UNIT_TEST("The pending-request table finds a registered request and misses a cleared one") {
  GIVEN("The pending-request table switched on and a fabricated request handle") {
    const ScopedTracking tracking;
    auto&                diagnostics       = tarch::mpi::Diagnostics::getInstance();
    const MPI_Request    registeredRequest = getFabricatedHandle<MPI_Request>();

    WHEN("The table registers the request, reports it, clears it and reports it again") {
      diagnostics.registerRequest(&registeredRequest, "MPI_Isend", 3, 7, std::source_location::current());
      const bool foundAfterTheRegistration =
        diagnostics.reportRequest(&registeredRequest, "MPI_Wait", std::source_location::current());
      diagnostics.clearRequest(&registeredRequest);
      const bool foundAfterTheClear =
        diagnostics.reportRequest(&registeredRequest, "MPI_Wait", std::source_location::current());

      THEN("The first report finds the request and the second one misses it") {
        VALIDATE(foundAfterTheRegistration);
        VALIDATE(not foundAfterTheClear);
      }
    }
  }
}

UNIT_TEST("The table ends empty after two threads register, report and clear their requests concurrently") {
  GIVEN("The pending-request table switched on and 64 fabricated requests for each of two threads") {
    constexpr int RequestsPerThread = 64;

    const ScopedTracking                       tracking;
    auto&                                      diagnostics = tarch::mpi::Diagnostics::getInstance();
    std::array<MPI_Request, RequestsPerThread> mainThreadRequests;
    std::array<MPI_Request, RequestsPerThread> partnerThreadRequests;
    mainThreadRequests.fill(getFabricatedHandle<MPI_Request>());
    partnerThreadRequests.fill(getFabricatedHandle<MPI_Request>());

    WHEN("The main thread and a partner thread each register, report and clear their disjoint requests 500 times") {
      constexpr int Rounds               = 500;
      const auto    exerciseRequestTable = [&diagnostics](const std::array<MPI_Request, RequestsPerThread>& requests) {
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

      std::thread partnerThread(exerciseRequestTable, std::ref(partnerThreadRequests));
      exerciseRequestTable(mainThreadRequests);
      partnerThread.join();

      THEN("Every report of a request of either thread returns false") {
        for (const auto& request : mainThreadRequests) {
          VALIDATE(not diagnostics.reportRequest(&request, "MPI_Wait", std::source_location::current()));
        }
        for (const auto& request : partnerThreadRequests) {
          VALIDATE(not diagnostics.reportRequest(&request, "MPI_Wait", std::source_location::current()));
        }
      }
    }
  }
}
