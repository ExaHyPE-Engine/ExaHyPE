#include "tarch/Assertions.h"
#include "tarch/tarch.h"
#include "tarch/services/ServiceRepository.h"

#include <sstream>
#include <cstdlib>
#include <chrono>

#include "Rank.h"
#include "tarch/compiler/CompilerSpecificSettings.h"
#include "tarch/multicore/multicore.h"

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


namespace {
  int tagCounter = 0;
}



tarch::logging::Log tarch::mpi::Rank::_log("tarch::mpi::Rank");


tarch::mpi::Rank  tarch::mpi::Rank::_singleton;



void tarch::mpi::Rank::releaseTag(int tag) {
  if (tag==tagCounter-1) {
    tagCounter--;
  }
}


int tarch::mpi::Rank::reserveFreeTag(const std::string& fullQualifiedMessageName, int numberOfTags) {
  assertion2(numberOfTags>=1,fullQualifiedMessageName,numberOfTags);
  const int result = tagCounter;
  tagCounter += numberOfTags;

  // I protect the tag manually (not via log filter), as many tags are actually
  // grabbed before most applications initialise their log filters properly.
  //
  // We may not use isGlobalMaster() as this query checks whether the code is
  // properly initialised. Please note rank is -1 as long as MPI is not properly
  // initialised, i.e. any tag booking prior to the MPI initialisation is not
  // logged properly.
  #if PeanoDebug>0
  if ( getInstance()._rank==getGlobalMasterRank() ) {
    std::cout << "rank " << getInstance()._rank << ": assigned message " << fullQualifiedMessageName
              << " the free tag " << result << " (" << numberOfTags << " consecutive tags reserved)" << std::endl;
  }
  #endif
  return result;
}


bool tarch::mpi::Rank::isInitialised() const {
  return _initIsCalled;
}


void tarch::mpi::Rank::ensureThatMessageQueuesAreEmpty( int fromRank, int tag ) {
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
    _log.error("plotMessageQueues()", "there are no messages from any sender in MPI queue");
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


void tarch::mpi::Rank::triggerDeadlockTimeOut(
  const std::string&  className,
  const std::string&  methodName,
  int                 communicationPartnerRank,
  int                 tag,
  int                 numberOfExpectedMessages,
  const std::string&  comment
) {
  std::ostringstream out;
  out << "operation " << className << "::" << methodName << " on node "
      << getRank() << " had to wait more than " << std::to_string(_deadlockTimeOut.count())
      << " seconds for " << numberOfExpectedMessages
      << " message(s) from node " << communicationPartnerRank << " with tag " << tag
      << ". Timeout. " << comment;
  _log.error( "triggerDeadlockTimeOut(...)", out.str() );

  plotMessageQueues();

  exit(DEADLOCK_EXIT_CODE);
}


void tarch::mpi::Rank::writeTimeOutWarning(
  const std::string&  className,
  const std::string&  methodName,
  int                 communicationPartnerRank,
  int                 tag,
  int                 numberOfExpectedMessages
) {
  logWarning(
    "writeTimeOutWarning(...)",
    "operation " << className << "::" << methodName << " on node "
    << getRank() << " had to wait more than " << std::to_string(_timeOutWarning.count())
    << " seconds for " << numberOfExpectedMessages
    << " message(s) from node " << communicationPartnerRank << " with tag " << tag
  );
  if (_deadlockTimeOut>std::chrono::seconds(0)) {
    logWarning(
      "writeTimeOutWarning(...)",
  	  "application will terminate after " << std::to_string(_deadlockTimeOut.count()) << " seconds because of a deadlock"
	  );
  }
  else {
    logWarning(
      "writeTimeOutWarning(...)",
      "no deadlock treshold set, i.e. application will not terminate because of any deadlock"
    );
  }
}


std::chrono::system_clock::time_point tarch::mpi::Rank::getDeadlockWarningTimeStamp() const {
  return std::chrono::system_clock::now() + _timeOutWarning;
}


std::chrono::system_clock::time_point tarch::mpi::Rank::getDeadlockTimeOutTimeStamp() const {
  return std::chrono::system_clock::now() + _deadlockTimeOut;
}


bool tarch::mpi::Rank::isTimeOutDeadlockEnabled() const {
  return _areTimeoutsEnabled and _deadlockTimeOut > std::chrono::seconds(0);
}


bool tarch::mpi::Rank::isTimeOutWarningEnabled() const {
  return _areTimeoutsEnabled and _timeOutWarning > std::chrono::seconds(0);
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
  _areTimeoutsEnabled(true) {
}
#else
tarch::mpi::Rank::Rank():
  _initIsCalled(false),
  _rank(0),
  _numberOfProcessors(1),
  _timeOutWarning(0),
  _deadlockTimeOut(0),
  _areTimeoutsEnabled(true) {
}
#endif


tarch::mpi::Rank::~Rank() {
}


void tarch::mpi::Rank::barrier(std::function<void()> waitor) {
  #ifdef Parallel
  logTraceIn( "barrier()" );

  if (getNumberOfRanks()>1) {

  MPI_Request request;
  MPI_Ibarrier( getCommunicator(), &request );

  auto timeOutWarning          = tarch::mpi::Rank::getInstance().getDeadlockWarningTimeStamp();
  auto timeOutShutdown         = tarch::mpi::Rank::getInstance().getDeadlockTimeOutTimeStamp();
  bool triggeredTimeoutWarning = false;
  int flag                     = 0;
  while (not flag) {
    if (
      tarch::mpi::Rank::getInstance().isTimeOutWarningEnabled() &&
      (std::chrono::system_clock::now()>timeOutWarning) &&
      (!triggeredTimeoutWarning)
    ) {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "tarch::mpi::Rank", "barrier()", -1, -1 );
      triggeredTimeoutWarning = true;
    }
    if (
      tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
      (std::chrono::system_clock::now()>timeOutShutdown)
    ) {
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "tarch::mpi::Rank", "barrier()", -1, -1 );
    }
    waitor();
    tarch::multicore::yield();
    MPI_Test( &request, &flag, MPI_STATUS_IGNORE );
  }

  }

  logTraceOut( "barrier()" );
  #endif
}


bool tarch::mpi::Rank::isMessageInQueue(int tag) const {
  #if Parallel
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

  _log.info( "logStatus()", statusMessage.str() );
}


bool tarch::mpi::Rank::init(int* argc, char*** argv) {
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

  if (result!=MPI_SUCCESS) {
    std::cerr << "init(int*,char***)\t initialisation failed: " + MPIReturnValueToString(result) + " (no logging available yet)";
    return false;
  }

  result = MPI_Comm_size( MPI_COMM_WORLD, &_numberOfProcessors );
  if (result!=MPI_SUCCESS) {
    std::cerr << "init(int*,char***)\t initialisation failed: " + MPIReturnValueToString(result) + " (no logging available yet)";
    return false;
  }

  result = MPI_Comm_rank( MPI_COMM_WORLD, &_rank );
  if (result!=MPI_SUCCESS) {
    std::cerr << "init(int*,char***)\t initialisation failed: " + MPIReturnValueToString(result) + " (no logging available yet)";
    return false;
  }

  if (_rank==0) {
    #if defined( SharedMemoryParallelisation )
    logDebug( "init(...)", "initialised MPI with MPI_THREAD_MULTIPLE" );
    #else
    logDebug( "init(...)", "initialised MPI with default mode, i.e. with MPI_Init(...), as no shared memory required" );
    #endif
  }
    
  IntegerMessage::initDatatype();
  #endif

  _initIsCalled = true;
  return true;
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
}


#ifdef Parallel
void tarch::mpi::Rank::setCommunicator( MPI_Comm communicator ) {
  _communicator = communicator;
}
#endif


void tarch::mpi::Rank::abort(int errorCode) {
  std::cout.flush();
  std::cerr.flush();
  #if Parallel
  MPI_Abort(MPI_COMM_WORLD,errorCode);
  #else
  exit(errorCode);
  #endif
}
