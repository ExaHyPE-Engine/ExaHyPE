#include "tarch/Assertions.h"
#include "tarch/tarch.h"
#include "tarch/services/ServiceRepository.h"

#include <sstream>
#include <cstdlib>
#include <chrono>

#include "Rank.h"
#include "tarch/compiler/CompilerSpecificSettings.h"
#include "tarch/multicore/multicore.h"

#include "tarch/mpi/DoubleMessage.h"
#include "tarch/mpi/IntegerMessage.h"
#include "tarch/mpi/StringMessage.h"
#include "tarch/mpi/BooleanSemaphore.h"

/**
 * For the machine name. If it doesn't work, switch it off in the file
 * CompilerSpecificSettings.h.
 */
#ifdef CompilerHasUTSName
#include <sys/utsname.h>
#endif

tarch::logging::Log tarch::mpi::Rank::_log("tarch::mpi::Rank");
int     tarch::mpi::Rank::_maxTags       = -1;
int     tarch::mpi::Rank::_tagCounter    =  0;

tarch::mpi::Rank  tarch::mpi::Rank::_singleton;

void tarch::mpi::Rank::releaseTag(int tag) {
  if (tag==_tagCounter-1) {
    _tagCounter--;
  }
}


int tarch::mpi::Rank::reserveFreeTag([[maybe_unused]] const std::string& fullQualifiedMessageName, [[maybe_unused]] int numberOfTags) {
  assertion2(numberOfTags>=1,fullQualifiedMessageName,numberOfTags);
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
  #if PeanoDebug>0
  if ( getInstance()._rank==getGlobalMasterRank() ) {
    std::cout << "assigned message " << fullQualifiedMessageName
              << " the free tag " << result << " (" << numberOfTags << " consecutive tags reserved)" << std::endl;
  }
  #endif
  */

  validateMaxTagIsSupported();

  return result;
}


bool tarch::mpi::Rank::isInitialised() const {
  return _initIsCalled;
}


void tarch::mpi::Rank::ensureThatMessageQueuesAreEmpty( [[maybe_unused]] int fromRank, [[maybe_unused]] int tag ) {
  #ifdef Parallel
  int          flag;
  MPI_Iprobe(fromRank, tag, _communicator, &flag, MPI_STATUS_IGNORE);
  if (flag!=0) {
    plotMessageQueues();
  }
  assertion3( flag==0, fromRank, tag, getRank() );
  #endif
}


void tarch::mpi::Rank::plotMessageQueues() {
  #ifdef Parallel
  int          flag;
  MPI_Status   status;
  MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, _communicator, &flag, &status);
  if (flag==0) {
    logError("plotMessageQueues()", "there are no messages from any sender in MPI queue");
  }
  else {
    logError(
      "plotMessageQueues()",
      "there is still a message in queue "
      " from rank " << status.MPI_SOURCE <<
      " with tag " << status.MPI_TAG
    );
  }
  #endif
}


bool tarch::mpi::Rank::exceededTimeOutWarningThreshold() const {
  return _areTimeoutsEnabled
      and
      _timeOutWarning>std::chrono::seconds(0)
      and
      std::chrono::system_clock::now() > _globalTimeOutDeadlock;
}


bool tarch::mpi::Rank::exceededDeadlockThreshold() const {
  return     _areTimeoutsEnabled
    and
    _deadlockTimeOut>std::chrono::seconds(0)
    and
    std::chrono::system_clock::now() > _globalTimeOutDeadlock;
}


void tarch::mpi::Rank::triggerDeadlockTimeOut(
  const std::string&  className,
  const std::string&  methodName,
  int                 communicationPartnerRank,
  int                 tag,
  int                 numberOfExpectedMessages,
  const std::string&  comment
) {
  if ( exceededDeadlockThreshold() ) {
    std::ostringstream out;
    out << "operation " << className << "::" << methodName << " on node "
        << getRank() << " had to wait more than " << std::to_string(_deadlockTimeOut.count())
        << " seconds for " << numberOfExpectedMessages
        << " message(s) from node " << communicationPartnerRank << " with tag " << tag
        << ". Timeout. " << comment;
    logError( "triggerDeadlockTimeOut(...)", out.str() );

    plotMessageQueues();

    abort(DEADLOCK_EXIT_CODE);
  }
}


void tarch::mpi::Rank::writeTimeOutWarning(
  const std::string&  className,
  const std::string&  methodName,
  int                 communicationPartnerRank,
  int                 tag,
  int                 numberOfExpectedMessages
) {
  if ( exceededTimeOutWarningThreshold() ) {
    logWarning(
      "writeTimeOutWarning(...)",
      "operation " << className << "::" << methodName << " on node "
      << getRank() << " had to wait more than " << std::to_string(_timeOutWarning.count())
      << " seconds for " << numberOfExpectedMessages
      << " message(s) from node " << communicationPartnerRank << " with tag " << tag
    );

    if ( _deadlockTimeOut.count()>0 ) {
      logWarning(
        "writeTimeOutWarning(...)",
        "application will terminate after " << std::to_string(_deadlockTimeOut.count()) << " seconds because of a deadlock"
      );
    } else {
      logWarning(
        "writeTimeOutWarning(...)",
        "deadlock detection switched off as deadlock time out is set to " << _deadlockTimeOut.count() << ", i.e. you will continue to get warnings, but code will not shut down"
      );
    }

    if (
      _timeOutWarning<_deadlockTimeOut/2
      or
      _deadlockTimeOut.count()<=0
    ) {
      std::chrono::seconds newTimeOutWarning = _timeOutWarning*2;
      logWarning(
        "writeTimeOutWarning(...)",
        "increase time out warning threshold from " << std::to_string(_timeOutWarning.count()) << " seconds to " << std::to_string(newTimeOutWarning.count()) << " seconds to avoid flood of warning messages"
      );
      _timeOutWarning       = newTimeOutWarning;
      _globalTimeOutWarning = _globalTimeOutWarning+_timeOutWarning;
    }
  }
}


void tarch::mpi::Rank::setDeadlockWarningTimeStamp() {
  _globalTimeOutWarning = std::chrono::system_clock::now() + _timeOutWarning;
}


void tarch::mpi::Rank::setDeadlockTimeOutTimeStamp() {
  _globalTimeOutDeadlock = std::chrono::system_clock::now() + _deadlockTimeOut;
}


void tarch::mpi::Rank::suspendTimeouts( bool timeoutsDisabled ) {
  _areTimeoutsEnabled = !timeoutsDisabled;
}


#ifdef Parallel
std::string tarch::mpi::MPIReturnValueToString( int result ) {
  std::ostringstream out;

  int   resultlen;
  char* string = new char[MPI_MAX_ERROR_STRING];  // (char *)malloc(MPI_MAX_ERROR_STRING * sizeof(char));
  MPI_Error_string(result, string, &resultlen);

  int   errorclass;
  MPI_Error_class(result, &errorclass);

  out << "mpi error class: " << errorclass << "="
      << ", mpi error text: " << string;

  switch ( errorclass ) {
    case MPI_SUCCESS:      out << "MPI_SUCCESS [no error]"; break;
    case MPI_ERR_BUFFER:   out << "MPI_ERR_BUFFER [invalid buffer pointer]"; break;
    case MPI_ERR_COUNT:    out << "MPI_ERR_COUNT [invalid count argument]"; break;
    case MPI_ERR_TYPE:     out << "MPI_ERR_TYPE [invalid datatype]"; break;
    case MPI_ERR_TAG:      out << "MPI_ERR_TAG [invalid tag]"; break;
    case MPI_ERR_COMM:     out << "MPI_ERR_COMM [invalid communicator]"; break;
    case MPI_ERR_RANK:     out << "MPI_ERR_RANK [invalid rank]"; break;
    case MPI_ERR_REQUEST:  out << "MPI_ERR_REQUEST [invalid request handle]"; break;
    case MPI_ERR_ROOT:     out << "MPI_ERR_ROOT [invalid root argument]"; break;
    case MPI_ERR_GROUP:    out << "MPI_ERR_GROUP [invalid group]"; break;
    case MPI_ERR_OP:       out << "MPI_ERR_OP [invalid operation]"; break;
    case MPI_ERR_TOPOLOGY: out << "MPI_ERR_TOPOLOGY [invalid topology]"; break;
    case MPI_ERR_DIMS:     out << "MPI_ERR_DIMS [invalid dimensions]"; break;
    case MPI_ERR_ARG:      out << "MPI_ERR_ARG [invalid argument]"; break;
    case MPI_ERR_UNKNOWN:  out << "MPI_ERR_UNKNOWN [unknown error]"; break;
    case MPI_ERR_TRUNCATE: out << "MPI_ERR_TRUNCATE [message has been truncated by receiver]"; break;
    case MPI_ERR_OTHER:    out << "MPI_ERR_OTHER [other unknown error]"; break;
    case MPI_ERR_INTERN:   out << "MPI_ERR_INTERN [internal mpi error]"; break;
    default: out << "unknown"; break;
  }

  delete[] string;

  return out.str();
}


std::string tarch::mpi::MPIStatusToString( const MPI_Status& status ) {
  std::ostringstream out;
  out << "status flag:"
      << " MPI_ERROR=" << status.MPI_ERROR
      << " (" << MPIReturnValueToString(status.MPI_ERROR)
      << ") ,MPI_SOURCE=" << status.MPI_SOURCE
      << ",MPI_TAG=" << status.MPI_TAG;
  return out.str();
}
#endif


#ifdef Parallel
tarch::mpi::Rank::Rank():
  _initIsCalled(false),
  _rank(-1),
  _numberOfProcessors(-1),
  _communicator( MPI_COMM_WORLD),
  _timeOutWarning(0),
  _deadlockTimeOut(0),
  _areTimeoutsEnabled(true) {}
#else
tarch::mpi::Rank::Rank():
  _initIsCalled(false),
  _rank(0),
  _numberOfProcessors(1),
  _timeOutWarning(0),
  _deadlockTimeOut(0),
  _areTimeoutsEnabled(true) {}
#endif

tarch::mpi::Rank::~Rank() {}

#ifdef Parallel
void tarch::mpi::Rank::allReduce(
  const void *sendbuf, void *recvbuf, int count,
  MPI_Datatype datatype,
  MPI_Op op,
  std::function<void()> waitor
) {
  logTraceIn( "allReduce()" );

//  if (getNumberOfRanks()>1) {
    MPI_Request* request = new MPI_Request();
    MPI_Iallreduce(sendbuf, recvbuf, count, datatype, op, getCommunicator(), request );

    tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
    tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();

    int flag                     = 0;
    while (not flag) {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "tarch::mpi::Rank", "allReduce()", -1, -1 );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "tarch::mpi::Rank", "allReduce()", -1, -1 );

      waitor();

      // leads to deadlock/starve situations
      //tarch::multicore::yield();
      MPI_Test( request, &flag, MPI_STATUS_IGNORE );
    }

    delete request;
//  }

  logTraceOut( "allReduce()" );
}


void tarch::mpi::Rank::reduce(
  const void *sendbuf, void *recvbuf, int count,
  MPI_Datatype datatype,
  MPI_Op op, int root,
  std::function<void()> waitor
) {
  logTraceIn( "reduce()" );

//  if (getNumberOfRanks()>1) {
    MPI_Request* request = new MPI_Request();
    MPI_Ireduce(sendbuf, recvbuf, count, datatype, op, root, getCommunicator(), request );

    tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
    tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    int flag                     = 0;
    while (not flag) {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "tarch::mpi::Rank", "reduce()", -1, -1 );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "tarch::mpi::Rank", "reduce()", -1, -1 );

      waitor();

      // leads to deadlock/starve situations
      //tarch::multicore::yield();
      MPI_Test( request, &flag, MPI_STATUS_IGNORE );
    }

    delete request;
//  }

  logTraceOut( "reduce()" );
}
#endif


void tarch::mpi::Rank::barrier([[maybe_unused]] std::function<void()> waitor) {
  #ifdef Parallel
  logTraceIn( "barrier()" );

  if (getNumberOfRanks()>1) {
    MPI_Request* request = new MPI_Request();
    MPI_Ibarrier( getCommunicator(), request );

    tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
    tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    int flag                     = 0;
    while (not flag) {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "tarch::mpi::Rank", "barrier()", -1, -1 );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "tarch::mpi::Rank", "barrier()", -1, -1 );

      waitor();

      // leads to deadlock/starve situations
      //tarch::multicore::yield();
      MPI_Test( request, &flag, MPI_STATUS_IGNORE );
    }

    delete request;
  }

  logTraceOut( "barrier()" );
  #endif
}


bool tarch::mpi::Rank::isMessageInQueue([[maybe_unused]] int tag) const {
  #ifdef Parallel
  int  flag        = 0;
  MPI_Iprobe(
    MPI_ANY_SOURCE, tag,
    getCommunicator(), &flag, MPI_STATUS_IGNORE
  );
  return flag;
  #else
  return false;
  #endif
}

void tarch::mpi::Rank::shutdown() {
  logTraceIn( "shutdown()" );
  #ifdef Parallel
  assertion( _rank!=-1 );

  IntegerMessage::shutdownDatatype();

#ifdef UseTargetDART
  finalizeTargetDART();
#endif

  int errorCode = MPI_Finalize();
  if (errorCode) {
    logError( "shutdown()", MPIReturnValueToString(errorCode) );
  }

  _communicator = MPI_COMM_WORLD;
  #endif

  _rank         = -1;
  logTraceOut( "shutdown()" );
}


int tarch::mpi::Rank::getGlobalMasterRank() {
  return 0;
}


bool tarch::mpi::Rank::isGlobalMaster() const {
  #ifdef Parallel
  assertion(_initIsCalled);
  return getRank() == getGlobalMasterRank();
  #else
  return true;
  #endif
}


void tarch::mpi::Rank::logStatus() const {
  std::ostringstream statusMessage;
  statusMessage << "MPI status:";

  #ifdef CompilerHasUTSName
  utsname* utsdata = new utsname();
  assertion( utsdata!=NULL );
  uname(utsdata);
  statusMessage << " nodename=" << utsdata->nodename;
  delete utsdata;
  #else
  statusMessage << " nodename=undef";
  #endif

  statusMessage << ", rank=" << _rank;
  #ifdef Parallel
  statusMessage << ", communicator=" << _communicator;
  #endif
  statusMessage << ", #processors=" << _numberOfProcessors;

  logInfo( "logStatus()", statusMessage.str() );
}


bool tarch::mpi::Rank::validateMaxTagIsSupported() {
  if (
    _maxTags <= 0
    or
    _tagCounter < _maxTags
    ) {
    return true;
  }
  else {
    logWarning( "validateMaxTagIsSupported()", "maximum tag value is " << _maxTags << " though we would need " << _tagCounter << " tags. Code will likely crash" );
    return false;
  }
}

#ifdef UseTargetDART
static void targetDartTextPointer() {}
#endif

bool tarch::mpi::Rank::init([[maybe_unused]] int* argc, [[maybe_unused]] char*** argv) {
  #ifdef Parallel
  int result = MPI_SUCCESS;

  #if defined( SharedMemoryParallelisation )
  int initThreadProvidedThreadLevelSupport;
  result = MPI_Init_thread( argc, argv, MPI_THREAD_MULTIPLE, &initThreadProvidedThreadLevelSupport );
  if (initThreadProvidedThreadLevelSupport!=MPI_THREAD_MULTIPLE ) {
    std::cerr << "warning: MPI implementation does not support MPI_THREAD_MULTIPLE. Support multithreading level is "
              << initThreadProvidedThreadLevelSupport << " instead of " << MPI_THREAD_MULTIPLE
              << ". Disable MultipleThreadsMayTriggerMPICalls in the compiler-specific settings or via -DnoMultipleThreadsMayTriggerMPICalls."<< std::endl;
  }
  #else
  result = MPI_Init( argc, argv );
  #endif

#ifdef UseTargetDART
  initTargetDART(reinterpret_cast<void*>(&targetDartTextPointer));
#endif

  if (result!=MPI_SUCCESS) {
    std::cerr << "init(int*,char***)\t initialisation failed: " + MPIReturnValueToString(result) + " (no logging available yet)" << std::endl;
    return false;
  }

  result = MPI_Comm_size( MPI_COMM_WORLD, &_numberOfProcessors );
  if (result!=MPI_SUCCESS) {
    std::cerr << "init(int*,char***)\t initialisation failed: " + MPIReturnValueToString(result) + " (no logging available yet)" << std::endl;
    return false;
  }

  result = MPI_Comm_rank( MPI_COMM_WORLD, &_rank );
  if (result!=MPI_SUCCESS) {
    std::cerr << "init(int*,char***)\t initialisation failed: " + MPIReturnValueToString(result) + " (no logging available yet)" << std::endl;
    return false;
  }

  int   answerFlag;
  void* rawMaxTag;
  MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_TAG_UB, &rawMaxTag, &answerFlag);
  if (answerFlag) {
    _maxTags = *(int*)rawMaxTag;
  }
  else {
    std::cerr << "init(int*,char***)\t was not able to query what the maximum tag value is" << std::endl;
    return false;
  }

  DoubleMessage::initDatatype();
  IntegerMessage::initDatatype();
  #endif

  _initIsCalled = true;
  return _initIsCalled;
}


int tarch::mpi::Rank::getRank() const {
/*
  #ifdef Parallel
  assertion(_initIsCalled);
  #endif
*/
  return _rank;
}


tarch::mpi::Rank& tarch::mpi::Rank::getInstance() {
  return _singleton;
}


#ifdef Parallel
MPI_Comm tarch::mpi::Rank::getCommunicator() const {
  assertion(_initIsCalled);
  return _communicator;
}
#endif


int tarch::mpi::Rank::getNumberOfRanks() const {
  #ifdef Parallel
  assertion(_initIsCalled);
  #endif
  return _numberOfProcessors;
}


void tarch::mpi::Rank::setTimeOutWarning( int value ) {
  assertion( value>=0 );
  _timeOutWarning = std::chrono::seconds(value);
}


void tarch::mpi::Rank::setDeadlockTimeOut( int value ) {
  assertion( value>=0 );
  _deadlockTimeOut = std::chrono::seconds(value);
  if (value==0) {
    logInfo( "setDeadlockTimeOut(int)", "set deadlock timeout to " << _deadlockTimeOut.count() << " and hence disabled timeout checks" );
  }
}


#ifdef Parallel
void tarch::mpi::Rank::setCommunicator( MPI_Comm communicator, [[maybe_unused]] bool recomputeRankAndWorld ) {
  _communicator = communicator;

  int result = MPI_Comm_size( _communicator, &_numberOfProcessors );
  if (result!=MPI_SUCCESS) {
    logError( "setCommunicator(...)", "initialisation failed: " + MPIReturnValueToString(result) );
  }

  result = MPI_Comm_rank( _communicator, &_rank );
  if (result!=MPI_SUCCESS) {
    logError( "setCommunicator(...)", "initialisation failed: " + MPIReturnValueToString(result) );
  }
}
#endif


void tarch::mpi::Rank::abort([[maybe_unused]] int errorCode) {
  std::cout.flush();
  std::cerr.flush();
  #ifdef Parallel
  MPI_Abort(MPI_COMM_WORLD,errorCode);
  #else
  std::abort();
  #endif
}
