#include "NonCriticalAssertions.h"
#include "tarch/logging/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Lock.h"
#include "tarch/mpi/Rank.h"
#include "tarch/Assertions.h"


#include "tarch/compiler/CompilerSpecificSettings.h"


namespace {
  int                                  rankWhichHasSetNonCriticalAssertion = -1;
  tarch::multicore::BooleanSemaphore   _assertionSemaphore;
  #if defined(Parallel) and defined(MPISupportsSingleSidedCommunication)
  MPI_Win assertionWindow;
  #endif
  bool                                 useNonCriticalAssertions = true;
}


void exahype2::enableNonCriticalAssertions(bool value) {
  useNonCriticalAssertions = value;
}


void exahype2::shutdownNonCritialAssertionEnvironment() {
  #if defined(Parallel) and defined(MPISupportsSingleSidedCommunication)
  MPI_Win_free( &assertionWindow );
  #endif
}


void exahype2::initNonCritialAssertionEnvironment() {
  #if defined(Parallel) and defined(MPISupportsSingleSidedCommunication)
  MPI_Win_create( &rankWhichHasSetNonCriticalAssertion, sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &assertionWindow );
  #endif
}


void exahype2::triggerNonCriticalAssertion( std::string file, int line, std::string expression, std::string parameterValuePairs ) {
  static tarch::logging::Log _log( "exahype2" );

  tarch::multicore::Lock lock( _assertionSemaphore );

  if (rankWhichHasSetNonCriticalAssertion>=0 and useNonCriticalAssertions) {
    logError( "triggerNonCriticalAssertion(...)", "noncritical assertion " << expression << " failed in (" << file << ":" << line << ")" );
    logError( "triggerNonCriticalAssertion(...)", parameterValuePairs );
    logError( "triggerNonCriticalAssertion(...)", "there has been a non-critical assertion before, so node should already be in shutdown mode" );
  }
  if (rankWhichHasSetNonCriticalAssertion<0 and useNonCriticalAssertions) {
    logError( "triggerNonCriticalAssertion(...)", "noncritical assertion " << expression << " failed in (" << file << ":" << line << ")" );
    logError( "triggerNonCriticalAssertion(...)", parameterValuePairs );
    logError( "triggerNonCriticalAssertion(...)", "inform rank 0 to dump solution and to shutdown application" );

    rankWhichHasSetNonCriticalAssertion = tarch::mpi::Rank::getInstance().getRank();

    if (not tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      #if defined(Parallel) and defined(MPISupportsSingleSidedCommunication)
      int rank = tarch::mpi::Rank::getInstance().getRank();

      /**

From Joachim Protze:



From the mpi_win_free man page:
> It can be invoked by a process only after it has completed its involvement in RMA communications on window win, that is, the process has called MPI_Win_fence, or called MPI_Win_unlock to match a previous call to MPI_Win_lock. When the call returns, the window memory can be freed.
The recommendation from my colleague:
Start a communication epoch with mpi_win_lock,
write the data with mpi_put,
complete the communication on origin and target with mpi_win_flush, and
close the epoch with mpi_win_unlock.Although the flush ensures completion on the target process, the c/c++ semantics don't guarantee that you will see the changed value. Making the value respectively the pointer volatile should solve this.The flush is a blocking call, so the process will be blocked until the target process has received the data. As I understand the use case, this call is not on the critical path, so nothing to worry performance-wise.To communicate with all other processes, you can use win_lock_all and win_flush_all instead of the P2P version.


       */
      MPI_Put( &rank, 1, MPI_INT, 0, 0, 1, MPI_INT, assertionWindow );
      #elif defined(Parallel)
      assertionMsg( false, "Noncritical assertion triggered, but system does not support single-sided communication so program will shut down immediately");
      #endif
    }
  }
}


bool exahype2::hasNonCriticalAssertionBeenViolated() {
  return rankWhichHasSetNonCriticalAssertion>=0;
}
