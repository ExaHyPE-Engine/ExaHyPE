#include "Rank.h"

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/mpi/DoubleMessage.h"
#include "tarch/mpi/IntegerMessage.h"
#include "tarch/mpi/StringMessage.h"
#include "tarch/mpi/mpi.h"
#include "tarch/multicore/multicore.h"
#include "tarch/services/ServiceRepository.h"
#include "tarch/tarch.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sstream>

/**
 * For the machine name. If it doesn't work, switch it off in the file
 * CompilerSpecificSettings.h.
 */
#ifdef COMPILER_HAS_UTS_NAME
#include <sys/utsname.h>
#endif

int tarch::mpi::Rank::_maxTags    = -1;
int tarch::mpi::Rank::_tagCounter = 0;

tarch::mpi::Rank tarch::mpi::Rank::_singleton;

void tarch::mpi::Rank::releaseTag(int tag) {
  if (tag == _tagCounter - 1) {
    _tagCounter--;
  }
}

int tarch::mpi::Rank::reserveFreeTag(
  [[maybe_unused]] const std::string& fullQualifiedMessageName,
  [[maybe_unused]] int                numberOfTags
) {
  assertion(numberOfTags >= 1, fullQualifiedMessageName, numberOfTags);
  const int result = _tagCounter;
  _tagCounter += numberOfTags;

  // I protect the tag manually (not via log filter), as many tags are actually
  // grabbed before most applications initialise their log filters properly.
  //
  // We may not use isGlobalMaster() as this query checks whether the code is
  // properly initialised. Please note rank is -1 as long as MPI is not properly
  // initialised, i.e. any tag booking prior to the MPI initialisation is not
  // logged properly.
  //
  // We also may not use the instance, as the instance might not be up yet. I
  // also saw other stuff with cout, so better ignore it
  /*
  #if PEANO_DEBUG
  if ( getInstance()._rank==getGlobalMasterRank() ) {
    std::cout << "assigned message " << fullQualifiedMessageName
              << " the free tag " << result << " (" << numberOfTags << " consecutive tags reserved)"
  << std::endl;
  }
  #endif
  */

  validateMaxTagIsSupported();

  return result;
}

bool tarch::mpi::Rank::isInitialised() const { return _initIsCalled; }

void tarch::mpi::Rank::ensureThatMessageQueuesAreEmpty([[maybe_unused]] int fromRank, [[maybe_unused]] int tag) {
#ifdef PEANO_WITH_MPI
  int flag;
  tarch::mpi::iprobe(fromRank, tag, _communicator, &flag, MPI_STATUS_IGNORE);
  if (flag != 0) {
    plotMessageQueues();
  }
  assertion(flag == 0, fromRank, tag, getRank());
#endif
}

void tarch::mpi::Rank::plotMessageQueues() {
#ifdef PEANO_WITH_MPI
  int        flag;
  MPI_Status status;
  tarch::mpi::iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, _communicator, &flag, &status);
  if (flag == 0) {
    logError("there are no messages from any sender in MPI queue");
  } else {
    logError(
      "there is still a message in queue "
      " from rank "
      << status.MPI_SOURCE << " with tag " << status.MPI_TAG
    );
  }
#endif
}

bool tarch::mpi::Rank::exceededTimeOutWarningThreshold() const {
  return _areTimeoutsEnabled and _timeOutWarning > std::chrono::seconds(0) and
         std::chrono::system_clock::now() > _globalTimeOutWarning;
}

bool tarch::mpi::Rank::exceededDeadlockThreshold() const {
  return _areTimeoutsEnabled and _deadlockTimeOut > std::chrono::seconds(0) and
         std::chrono::system_clock::now() > _globalTimeOutDeadlock;
}

void tarch::mpi::Rank::triggerDeadlockTimeOut(
  const std::string& className,
  const std::string& methodName,
  int                communicationPartnerRank,
  int                tag,
  int                numberOfExpectedMessages,
  const std::string& comment
) {
  if (exceededDeadlockThreshold()) {
    std::ostringstream out;
    out << "operation " << className << "::" << methodName << " on node " << getRank() << " had to wait more than "
        << _deadlockTimeOut.count() << " seconds for " << numberOfExpectedMessages << " message(s) from node "
        << communicationPartnerRank << " with tag " << tag << ". Timeout. " << comment;
    logError(out.str());

    plotMessageQueues();

    abort(DEADLOCK_EXIT_CODE);
  }
}

void tarch::mpi::Rank::writeTimeOutWarning(
  const std::string& className,
  const std::string& methodName,
  int                communicationPartnerRank,
  int                tag,
  int                numberOfExpectedMessages
) {
  if (exceededTimeOutWarningThreshold()) {
    logWarning(
      "operation " << className << "::" << methodName << " on node " << getRank() << " had to wait more than "
                   << _timeOutWarning.count() << " seconds for " << numberOfExpectedMessages << " message(s) from node "
                   << communicationPartnerRank << " with tag " << tag
    );

    if (_deadlockTimeOut.count() > 0.0) {
      logWarning("application will terminate after " << _deadlockTimeOut.count() << " seconds because of a deadlock");
    } else {
      logWarning(
        "deadlock detection switched off as deadlock time out is set to "
        << _deadlockTimeOut.count() << ", i.e. you will continue to get warnings, but code will not shut down"
      );
    }

    if (_timeOutWarning < _deadlockTimeOut / 2 or _deadlockTimeOut.count() <= 0.0) {
      const std::chrono::duration<double> newTimeOutWarning = _timeOutWarning * 2;
      logWarning(
        "increase time out warning threshold from " << _timeOutWarning.count() << " seconds to "
                                                    << newTimeOutWarning.count()
                                                    << " seconds to avoid flood of warning messages"
      );
      _timeOutWarning       = newTimeOutWarning;
      _globalTimeOutWarning = _globalTimeOutWarning + _timeOutWarning;
    }
  }
}

void tarch::mpi::Rank::setDeadlockWarningTimeStamp() {
  _globalTimeOutWarning = std::chrono::system_clock::now() + _timeOutWarning;
}

void tarch::mpi::Rank::setDeadlockTimeOutTimeStamp() {
  _globalTimeOutDeadlock = std::chrono::system_clock::now() + _deadlockTimeOut;
}

void tarch::mpi::Rank::suspendTimeouts(bool timeoutsDisabled) { _areTimeoutsEnabled = !timeoutsDisabled; }

bool tarch::mpi::Rank::areTimeoutsEnabled() const { return _areTimeoutsEnabled; }

std::chrono::duration<double> tarch::mpi::Rank::getTimeOutWarning() const { return _timeOutWarning; }

std::chrono::duration<double> tarch::mpi::Rank::getDeadlockTimeOut() const { return _deadlockTimeOut; }

#ifdef PEANO_WITH_MPI
tarch::mpi::Rank::Rank() :
  _initIsCalled(false),
  _rank(-1),
  _numberOfProcessors(-1),
  _providedThreadLevelSupport(-1),
  _communicator(MPI_COMM_WORLD),
  _timeOutWarning(0),
  _deadlockTimeOut(0),
  _areTimeoutsEnabled(true),
  _deadlockDetectionEnabled(false) {}
#else
tarch::mpi::Rank::Rank() :
  _initIsCalled(false),
  _rank(0),
  _numberOfProcessors(1),
  _timeOutWarning(0),
  _deadlockTimeOut(0),
  _areTimeoutsEnabled(true),
  _deadlockDetectionEnabled(false) {}
#endif

tarch::mpi::Rank::~Rank() {}

bool tarch::mpi::Rank::isMessageInQueue([[maybe_unused]] int tag) const {
#ifdef PEANO_WITH_MPI
  int flag = 0;
  tarch::mpi::iprobe(MPI_ANY_SOURCE, tag, getCommunicator(), &flag, MPI_STATUS_IGNORE);
  return flag;
#else
  return false;
#endif
}

void tarch::mpi::Rank::shutdown() {

#ifdef PEANO_WITH_MPI
  assertion(_rank != -1);

  DoubleMessage::shutdownDatatype();
  IntegerMessage::shutdownDatatype();

  int errorCode = MPI_Finalize();
  if (errorCode) {
    logError(MPIReturnValueToString(errorCode));
  }
#endif

  _rank = -1;
}

int tarch::mpi::Rank::getGlobalMasterRank() { return 0; }

bool tarch::mpi::Rank::isGlobalMaster() const {
#ifdef PEANO_WITH_MPI
  assertion(_initIsCalled);
  return getRank() == getGlobalMasterRank();
#else
  return true;
#endif
}

void tarch::mpi::Rank::logStatus() const {
  std::ostringstream statusMessage;
  statusMessage << "MPI status:";

#ifdef COMPILER_HAS_UTS_NAME
  utsname* utsdata = new utsname();
  assertion(utsdata != NULL);
  uname(utsdata);
  statusMessage << " nodename=" << utsdata->nodename;
  delete utsdata;
#else
  statusMessage << " nodename=undef";
#endif

  statusMessage << ", rank=" << _rank;
#ifdef PEANO_WITH_MPI
  statusMessage << ", communicator=" << _communicator;
#endif
  statusMessage << ", #processors=" << _numberOfProcessors;

  logInfo(statusMessage.str());
}

bool tarch::mpi::Rank::validateMaxTagIsSupported() {
  if (_maxTags <= 0 or _tagCounter < _maxTags) {
    return true;
  } else {
    logWarning(
      "maximum tag value is " << _maxTags << " though we would need " << _tagCounter << " tags. Code will likely crash"
    );
    return false;
  }
}

int tarch::mpi::Rank::getProvidedThreadLevelSupport() const { return _providedThreadLevelSupport; }

bool tarch::mpi::Rank::init([[maybe_unused]] int* argc, [[maybe_unused]] char*** argv) {
#ifdef PEANO_WITH_MPI
  int result = MPI_SUCCESS;

#if defined(PEANO_WITH_MULTITHREADING)
  result = MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &_providedThreadLevelSupport);
  if (_providedThreadLevelSupport != MPI_THREAD_MULTIPLE) {
    std::cerr << "warning: MPI implementation does not support MPI_THREAD_MULTIPLE. Support "
                 "multithreading level is "
              << _providedThreadLevelSupport << " instead of " << MPI_THREAD_MULTIPLE
              << ". Disable MultipleThreadsMayTriggerMPICalls in the compiler-specific settings or "
                 "via -DnoMultipleThreadsMayTriggerMPICalls."
              << std::endl;
  }
#else
  result = MPI_Init(argc, argv);
#endif

  if (result != MPI_SUCCESS) {
    std::cerr << "init(int*,char***)\t Initialisation failed: " + MPIReturnValueToString(result) +
                   " (no logging available yet)"
              << std::endl;
    return false;
  }

  result = MPI_Comm_size(MPI_COMM_WORLD, &_numberOfProcessors);
  if (result != MPI_SUCCESS) {
    std::cerr << "init(int*,char***)\t initialisation failed: " + MPIReturnValueToString(result) +
                   " (no logging available yet)"
              << std::endl;
    return false;
  }

  result = MPI_Comm_rank(MPI_COMM_WORLD, &_rank);
  if (result != MPI_SUCCESS) {
    std::cerr << "init(int*,char***)\t initialisation failed: " + MPIReturnValueToString(result) +
                   " (no logging available yet)"
              << std::endl;
    return false;
  }

  int   answerFlag;
  void* rawMaxTag;
  MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_TAG_UB, &rawMaxTag, &answerFlag);
  if (answerFlag) {
    _maxTags = *(int*)rawMaxTag;
  } else {
    std::cerr << "init(int*,char***)\t was not able to query what the maximum tag value is" << std::endl;
    return false;
  }

  MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

  DoubleMessage::initDatatype();
  IntegerMessage::initDatatype();
#endif

  _initIsCalled = true;
  return _initIsCalled;
}

int tarch::mpi::Rank::getRank() const { return _rank; }

tarch::mpi::Rank& tarch::mpi::Rank::getInstance() { return _singleton; }

#ifdef PEANO_WITH_MPI
MPI_Comm tarch::mpi::Rank::getCommunicator() const {
  assertion(_initIsCalled);
  return _communicator;
}
#endif

void tarch::mpi::Rank::setTimeOutWarning(double value) {
  assertion(value >= 0.0);
  _timeOutWarning = std::chrono::duration<double>(value);
}

void tarch::mpi::Rank::setDeadlockTimeOut(double value) {
  assertion(value >= 0.0);
  _deadlockTimeOut = std::chrono::duration<double>(value);
  if (value == 0.0) {
    logInfo("set deadlock timeout to " << _deadlockTimeOut.count() << " and hence disabled timeout checks");
  }
}

void tarch::mpi::Rank::setDeadlockDetection(bool enabled) { _deadlockDetectionEnabled = enabled; }

bool tarch::mpi::Rank::isDeadlockDetectionEnabled() const { return _deadlockDetectionEnabled; }

void tarch::mpi::abort([[maybe_unused]] int errorCode) {
  std::cout.flush();
  std::cerr.flush();
#ifdef PEANO_WITH_MPI
  int mpiInitialised = 0;
  MPI_Initialized(&mpiInitialised);
  if (mpiInitialised) {
    int mpiFinalised = 0;
    MPI_Finalized(&mpiFinalised);
    if (not mpiFinalised) {
      MPI_Abort(MPI_COMM_WORLD, errorCode);
    }
  }
#endif
  std::_Exit(errorCode);
}

void tarch::mpi::Rank::abort(int errorCode) { tarch::mpi::abort(errorCode); }
