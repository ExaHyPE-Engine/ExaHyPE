#include "mpi.h"


#include "tarch/mpi/Rank.h"
#include "tarch/services/ServiceRepository.h"


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


void tarch::mpi::wait(MPI_Request& request, const std::string& fullQualifiedClassName, const std::string& functionName) {
  int         flag = 0;
  MPI_Test( &request, &flag, MPI_STATUS_IGNORE );
  tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
  tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
  while (!flag) {
    tarch::mpi::Rank::getInstance().writeTimeOutWarning(    fullQualifiedClassName, functionName, -1, -1 );
    tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( fullQualifiedClassName, functionName, -1, -1 );
    tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    MPI_Test( &request, &flag, MPI_STATUS_IGNORE );
  }
}
#endif
