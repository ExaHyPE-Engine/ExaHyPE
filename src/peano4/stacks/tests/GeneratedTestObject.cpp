#include "GeneratedTestObject.h"



#include <sstream>
#include <algorithm>



peano4::stacks::tests::GeneratedTestObject::GeneratedTestObject(int  __id, tarch::la::Vector<256,double>  __data){
setId( __id);
setData( __data);
}



std::string peano4::stacks::tests::GeneratedTestObject::toString() const {
  std::ostringstream out;
  out << "(";
  out << "id=" << _id;
  out << ","; 
  out << "data=" << _data.toVector();
  out << ")";
  return out.str();
}





int   peano4::stacks::tests::GeneratedTestObject::getId() const {
  return _id;
}


void   peano4::stacks::tests::GeneratedTestObject::setId(int value) {
  _id = value;
}


tarch::la::Vector<256,double>   peano4::stacks::tests::GeneratedTestObject::getData() const {
  return   _data.toVector();
}


void   peano4::stacks::tests::GeneratedTestObject::setData(const tarch::la::Vector<256,double>& value) {
  _data = value;
}


double   peano4::stacks::tests::GeneratedTestObject::getData(int index) const {
  return   _data(index);
}


void   peano4::stacks::tests::GeneratedTestObject::setData(int index, double value) {
  _data(index) = value;
}


tarch::la::SmartPointerVector<256,double>   peano4::stacks::tests::GeneratedTestObject::getDataSmartPointer() {
  return   _data;
}






#ifdef Parallel
void peano4::stacks::tests::GeneratedTestObject::freeForkDatatype() {
  FlattenedType::freeForkDatatype();
}

void peano4::stacks::tests::GeneratedTestObject::freeGlobalCommunciationDatatype() {
  FlattenedType::freeGlobalCommunciationDatatype();
}

void peano4::stacks::tests::GeneratedTestObject::freeJoinDatatype() {
  FlattenedType::freeJoinDatatype();
}

void peano4::stacks::tests::GeneratedTestObject::freeBoundaryExchangeDatatype() {
  FlattenedType::freeBoundaryExchangeDatatype();
}

void peano4::stacks::tests::GeneratedTestObject::freeMultiscaleDataExchangeDatatype() {
  FlattenedType::freeMultiscaleDataExchangeDatatype();
}

int peano4::stacks::tests::GeneratedTestObject::getSenderRank() const {
  return _senderDestinationRank;
}

void peano4::stacks::tests::GeneratedTestObject::initDatatype() {
  FlattenedType::initDatatype();
}

void peano4::stacks::tests::GeneratedTestObject::shutdownDatatype() {
  FlattenedType::shutdownDatatype();
}

void peano4::stacks::tests::GeneratedTestObject::send(const SmartType& buffer, int destination, int tag, MPI_Comm communicator ) {
  FlattenedType sendReceiveObject( flatten(buffer) );
  FlattenedType::send( sendReceiveObject, destination, tag, communicator );
}

void peano4::stacks::tests::GeneratedTestObject::receive(SmartType& buffer, int source, int tag, MPI_Comm communicator ) {
  FlattenedType sendReceiveObject;
  FlattenedType::receive(sendReceiveObject, source, tag, communicator);
  buffer = unflatten( sendReceiveObject );
  buffer._senderDestinationRank = sendReceiveObject.getSenderRank();
}

void peano4::stacks::tests::GeneratedTestObject::send(
  const SmartType& buffer,
  int destination,
  int tag,
  std::function<void()> startCommunicationFunctor,
  std::function<void()> waitFunctor,
  MPI_Comm communicator
) {
  FlattenedType sendReceiveObject( flatten(buffer) );
  FlattenedType::send( sendReceiveObject, destination, tag, startCommunicationFunctor, waitFunctor, communicator );
}

void peano4::stacks::tests::GeneratedTestObject::receive(
  SmartType& buffer,
  int source,
  int tag,
  std::function<void()> startCommunicationFunctor,
  std::function<void()> waitFunctor,
  MPI_Comm communicator
) {
  FlattenedType sendReceiveObject;
  FlattenedType::receive(sendReceiveObject, source, tag, startCommunicationFunctor, waitFunctor, communicator);
  buffer = unflatten( sendReceiveObject );
  buffer._senderDestinationRank = sendReceiveObject.getSenderRank();
}
#endif

peano4::stacks::tests::GeneratedTestObject::FlattenedType peano4::stacks::tests::GeneratedTestObject::flatten(const SmartType& copy ) {
  FlattenedType result;
  
result.setId( static_cast<decltype(result.getId())>( copy.getId() ) );

result.setData( static_cast<decltype(result.getData())>( copy.getData() ) );

  return result;
}

peano4::stacks::tests::GeneratedTestObject::SmartType peano4::stacks::tests::GeneratedTestObject::unflatten(const FlattenedType& copy ) {
  SmartType result;
  
result.setId( static_cast<decltype(result.getId())>( copy.getId() ) );

result.setData( static_cast<decltype(result.getData())>( copy.getData() ) );

  return result;
}

#ifdef Parallel
void peano4::stacks::tests::GeneratedTestObject::sendAndPollDanglingMessages(const peano4::stacks::tests::GeneratedTestObject& message, int destination, int tag, MPI_Comm communicator ) {
  peano4::stacks::tests::GeneratedTestObject::send(
    message, destination, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::stacks::tests::GeneratedTestObject", "sendAndPollDanglingMessages()",destination, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::stacks::tests::GeneratedTestObject", "sendAndPollDanglingMessages()", destination, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}


void peano4::stacks::tests::GeneratedTestObject::receiveAndPollDanglingMessages(peano4::stacks::tests::GeneratedTestObject& message, int source, int tag, MPI_Comm communicator ) {
  peano4::stacks::tests::GeneratedTestObject::receive(
    message, source, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::stacks::tests::GeneratedTestObject", "receiveAndPollDanglingMessages()", source, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::stacks::tests::GeneratedTestObject", "receiveAndPollDanglingMessages()", source, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}
#endif
    