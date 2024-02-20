#include "tarch/NonCriticalAssertions.h"

#include "tarch/logging/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Lock.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/IntegerMessage.h"
#include "tarch/Assertions.h"


#include "tarch/compiler/CompilerSpecificSettings.h"


namespace {
  /**
   * This is the rank-local variable to store the number of the
   * failing rank.
   */
  int  rankWhichHasSetNonCriticalAssertion = -1;

  /**
   * There's only one variable per rank, so we have to protect
   * access through many threads.
   */
  tarch::multicore::BooleanSemaphore   assertionSemaphore;

  #if defined(Parallel)
  /**
   * An assertion is triggered just by sending a message to
   * rank 0 on an error tag.
   */
  int   assertionExchangeTag;
  #endif

  bool  useNonCriticalAssertions = true;
}


void tarch::enableNonCriticalAssertions(bool value) {
  useNonCriticalAssertions = value;
}


void tarch::shutdownNonCriticalAssertionEnvironment() {
  #if defined(Parallel)
  tarch::mpi::Rank::releaseTag( assertionExchangeTag );
  #endif
}


void tarch::initNonCriticalAssertionEnvironment() {
  #if defined(Parallel)
  assertionExchangeTag = tarch::mpi::Rank::reserveFreeTag( "mpi::noncriticalassertion" );
  #endif
}


void tarch::triggerNonCriticalAssertion( std::string file, int line, std::string expression, std::string parameterValuePairs ) {
  static tarch::logging::Log _log("tarch");

  tarch::multicore::Lock lock( assertionSemaphore );

  if (rankWhichHasSetNonCriticalAssertion>=0 and useNonCriticalAssertions) {
    logDebug( "triggerNonCriticalAssertion(...)", "noncritical assertion " << expression << " failed in (" << file << ":" << line << ")" );
    //logError( "triggerNonCriticalAssertion(...)", parameterValuePairs );
    logDebug( "triggerNonCriticalAssertion(...)", "there has been a non-critical assertion before, so node should already be in shutdown mode" );
  }
  if (rankWhichHasSetNonCriticalAssertion<0 and useNonCriticalAssertions) {
    logError( "triggerNonCriticalAssertion(...)", "noncritical assertion " << expression << " failed in (" << file << ":" << line << ")" );
    logError( "triggerNonCriticalAssertion(...)", parameterValuePairs );
    logError( "triggerNonCriticalAssertion(...)", "inform rank 0 to dump solution and to shutdown application" );

    rankWhichHasSetNonCriticalAssertion = tarch::mpi::Rank::getInstance().getRank();

    if (not tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      #if defined(Parallel)
      int rank = tarch::mpi::Rank::getInstance().getRank();
      tarch::mpi::IntegerMessage message( rank );
      tarch::mpi::IntegerMessage::send(
        message,
        tarch::mpi::Rank::getGlobalMasterRank(),
        assertionExchangeTag,
        tarch::mpi::Rank::getInstance().getCommunicator()
      );
      #endif
    }
  }
}


bool tarch::hasNonCriticalAssertionBeenViolated() {
  #if defined(Parallel)
  static tarch::logging::Log _log("tarch");

  if (rankWhichHasSetNonCriticalAssertion<0) {
    int         flag = 0;
    MPI_Status  status;
    MPI_Iprobe(MPI_ANY_SOURCE, assertionExchangeTag, tarch::mpi::Rank::getInstance().getCommunicator(), &flag, &status );
    if (flag) {
      tarch::mpi::IntegerMessage message;
      tarch::mpi::IntegerMessage::receive(
        message,
        status.MPI_SOURCE,
        assertionExchangeTag,
        tarch::mpi::Rank::getInstance().getCommunicator()
      );
      rankWhichHasSetNonCriticalAssertion = message.getValue();
      logError( "triggerNonCriticalAssertion(...)", "received non-critical assertion from rank " << status.MPI_SOURCE );
    }
  }
  #endif

  return rankWhichHasSetNonCriticalAssertion>=0;
}
