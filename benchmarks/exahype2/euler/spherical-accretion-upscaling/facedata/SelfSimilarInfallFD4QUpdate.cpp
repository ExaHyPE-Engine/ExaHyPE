#include "SelfSimilarInfallFD4QUpdate.h"

#include "tarch/accelerator/accelerator.h"


benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::SelfSimilarInfallFD4QUpdate():
  _dataSmartPointer(
    static_cast< double* >( allocateMemory(Cardinality, tarch::MemoryLocation::Heap) )
  ) {
  value = &( _dataSmartPointer[0] );
}


benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::SelfSimilarInfallFD4QUpdate(const SelfSimilarInfallFD4QUpdate& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  _dataSmartPointer = other._dataSmartPointer;
  value = &( _dataSmartPointer[0] );
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::clone(const SelfSimilarInfallFD4QUpdate& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  #if Dimensions==2
  std::copy_n( other.value, 150, value );
  #else
  std::copy_n( other.value, 750, value );
  #endif
}


benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate& benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::operator=(const SelfSimilarInfallFD4QUpdate& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  _dataSmartPointer = other._dataSmartPointer;
  value = &( _dataSmartPointer[0] );
  
  return *this;
}


std::string benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::toString() const {
  std::ostringstream result;
  result << "(";
  #if PeanoDebug>=1
  result << "x=" << _debugX;
  result << ",";
  result << "h=" << _debugH;
  #endif
  result << ")";
  return result.str();
}


#if PeanoDebug>=1

void benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::setDebugX( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugX = data;
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::setDebugH( const tarch::la::Vector<Dimensions,double>& data ) {
  // At the moment, we only use square/cubic elements
  assertion2( data(0)>0.0, data, toString() );
  assertion2( data(0)==data(1), data, toString() );
  #if Dimensions==3
  assertion2( data(1)==data(2), data, toString() );
  #endif
  _debugH = data;
}


tarch::la::Vector<Dimensions,double> benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::getDebugX() const {
  return _debugX;
}


tarch::la::Vector<Dimensions,double> benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::getDebugH() const {
  return _debugH;
}
#endif


void benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::merge(peano4::grid::TraversalObserver::SendReceiveContext context, const SelfSimilarInfallFD4QUpdate& neighbour, const peano4::datamanagement::FaceMarker& marker) {
  
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::send(
  const peano4::datamanagement::FaceMarker& marker
  
) const {
  return false;
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::receiveAndMerge(
  const peano4::datamanagement::FaceMarker& marker
  
) const {
  return false;
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::loadPersistently(
  const peano4::datamanagement::FaceMarker& marker
  
) {
  return false;
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::storePersistently(
  const peano4::datamanagement::FaceMarker& marker
  
) const {
  return false;
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::compute(
  const peano4::datamanagement::FaceMarker& marker
  
) const {
  return true;
}
#ifdef Parallel
void benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::initDatatype() {
  int errorCode = MPI_Type_vector(
    1,           // number of blocks (count)
    Cardinality, // number of entries per block (blocklength),
    0,           // no stride as we work with a continuous datatype,
    MPI_DOUBLE,
    &Datatype
  );
  
  errorCode += MPI_Type_commit( &Datatype );

  if (errorCode) std::cerr << "error constructing MPI datatype in " << __FILE__ << ":" << __LINE__ << std::endl;
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}
  

MPI_Datatype   benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::Datatype;


MPI_Datatype  benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::getForkDatatype() {
  return Datatype;
}


MPI_Datatype  benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::getJoinDatatype() {
  return Datatype;
}


MPI_Datatype  benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::getBoundaryExchangeDatatype() {
  return Datatype;
}


MPI_Datatype  benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::getMultiscaleDataExchangeDatatype() {
  return Datatype;
}


MPI_Datatype  benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFD4QUpdate::getGlobalCommunciationDatatype() {
  return Datatype;
}

#endif