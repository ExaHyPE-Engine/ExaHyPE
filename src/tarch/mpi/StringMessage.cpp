#include "StringMessage.h"

#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/mpi.h"
#include "tarch/services/ServiceRepository.h"

#include <algorithm>
#include <sstream>

std::string tarch::mpi::StringMessage::toString() const {
  std::ostringstream out;
  out << "(";
  out << "data=" << getData();
  out << ",rank=" << _senderDestinationRank;
  out << ")";
  return out.str();
}

std::string tarch::mpi::StringMessage::getData() const { return _data; }

void tarch::mpi::StringMessage::setData(const std::string& value) { _data = value; }

#ifdef PEANO_WITH_MPI
void tarch::mpi::StringMessage::sendAndPollDanglingMessages(
  const tarch::mpi::StringMessage& message,
  int                              destination,
  int                              tag
) {
  tarch::mpi::StringMessage::send(message, destination, tag, []() {}, [&]() {
    tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
  }, tarch::mpi::Rank::getInstance().getCommunicator());
}

void tarch::mpi::StringMessage::receiveAndPollDanglingMessages(
  tarch::mpi::StringMessage& message,
  int                        source,
  int                        tag
) {
  tarch::mpi::StringMessage::receive(message, source, tag, []() {}, [&]() {
    tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
  }, tarch::mpi::Rank::getInstance().getCommunicator());
}
#endif

#ifdef PEANO_WITH_MPI

int tarch::mpi::StringMessage::getSenderRank() const { return _senderDestinationRank; }

void tarch::mpi::StringMessage::send(
  const tarch::mpi::StringMessage& buffer,
  int                              destination,
  int                              tag,
  MPI_Comm                         communicator
) {
  tarch::mpi::send(buffer._data.c_str(), buffer._data.size() + 1, MPI_CHAR, destination, tag, communicator);
}

void tarch::mpi::StringMessage::receive(tarch::mpi::StringMessage& buffer, int source, int tag, MPI_Comm communicator) {
  MPI_Message msg;
  MPI_Status  status;
  tarch::mpi::mprobe(source, tag, communicator, &msg, &status);
  int count = 0;
  tarch::mpi::getCount(&status, MPI_CHAR, &count);
  char* tmp = new char[count];
  tarch::mpi::mrecv(tmp, count, MPI_CHAR, &msg, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
  buffer._data                  = tmp;
  delete[] tmp;
}

void tarch::mpi::StringMessage::send(
  const tarch::mpi::StringMessage& buffer,
  int                              destination,
  int                              tag,
  std::function<void()>            startCommunicationFunctor,
  std::function<void()>            waitFunctor,
  MPI_Comm                         communicator
) {
  MPI_Request sendRequestHandle = MPI_REQUEST_NULL;
  tarch::mpi::isend(
    buffer._data.c_str(),
    buffer._data.size() + 1,
    MPI_CHAR,
    destination,
    tag,
    communicator,
    &sendRequestHandle
  );
  startCommunicationFunctor();
  tarch::mpi::wait(&sendRequestHandle, waitFunctor, MPI_STATUS_IGNORE);
}

void tarch::mpi::StringMessage::receive(
  tarch::mpi::StringMessage& buffer,
  int                        source,
  int                        tag,
  std::function<void()>      startCommunicationFunctor,
  std::function<void()>      waitFunctor,
  MPI_Comm                   communicator
) {
  MPI_Message msg;
  MPI_Status  status;
  startCommunicationFunctor();
  tarch::mpi::mprobe(source, tag, communicator, &msg, waitFunctor, &status);
  int count = 0;
  tarch::mpi::getCount(&status, MPI_CHAR, &count);
  char* tmp = new char[count];
  tarch::mpi::mrecv(tmp, count, MPI_CHAR, &msg, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
  buffer._data                  = tmp;
  delete[] tmp;
}

bool tarch::mpi::StringMessage::isMessageInQueue(int tag, MPI_Comm communicator) {
  int flag = 0;
  tarch::mpi::iprobe(MPI_ANY_SOURCE, tag, communicator, &flag, MPI_STATUS_IGNORE);
  return flag;
}

#endif
