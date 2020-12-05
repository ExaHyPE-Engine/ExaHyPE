#include "NonCriticalAssertions.h"
#include "tarch/logging/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Lock.h"
#include "tarch/mpi/Rank.h"
#include "tarch/Assertions.h"


// @todo should be called system
#include "tarch/compiler/CompilerSpecificSettings.h"


namespace {
  int                                  rankWhichHasSetNonCriticalAssertion = -1;
  tarch::multicore::BooleanSemaphore   _assertionSemaphore;
  #if defined(Parallel) and defined(MPISupportsSingleSidedCommunication)
  MPI_Win assertionWindow;
  #endif
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

  if (rankWhichHasSetNonCriticalAssertion<0) {
    logError( "triggerNonCriticalAssertion(...)", "noncritical assertion " << expression << " failed in (" << file << ":" << line << ")" );
    logError( "triggerNonCriticalAssertion(...)", parameterValuePairs );
    logError( "triggerNonCriticalAssertion(...)", "inform rank 0 to dump solution and to shutdown application" );

    rankWhichHasSetNonCriticalAssertion = tarch::mpi::Rank::getInstance().getRank();

    #if defined(Parallel) and defined(MPISupportsSingleSidedCommunication)
    if (not tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      int rank = tarch::mpi::Rank::getInstance().getRank();

      MPI_Put( &rank, 1, MPI_INT, 0, 0, 1, MPI_INT, assertionWindow );
    }
    #elif defined(Parallel)
    assertionMsg( false, "Noncritical assertion triggered, but system does not support single-sided communication so program will shut down immediately");
    #endif
  }
}


bool exahype2::hasNonCriticalAssertionBeenViolated() {
  return rankWhichHasSetNonCriticalAssertion>=0;
}
