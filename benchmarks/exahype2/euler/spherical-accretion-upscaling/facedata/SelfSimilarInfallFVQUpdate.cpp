#include "SelfSimilarInfallFVQUpdate.h"

#include "tarch/accelerator/accelerator.h"


benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate::SelfSimilarInfallFVQUpdate():
  _dataSmartPointer(
    static_cast< double* >( allocateMemory(Cardinality, tarch::MemoryLocation::Heap) )
  ) {
  value = &( _dataSmartPointer[0] );
}


benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate::SelfSimilarInfallFVQUpdate(const SelfSimilarInfallFVQUpdate& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  _dataSmartPointer = other._dataSmartPointer;
  value = &( _dataSmartPointer[0] );
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate::clone(const SelfSimilarInfallFVQUpdate& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  #if Dimensions==2
  std::copy_n( other.value, 40, value );
  #else
  std::copy_n( other.value, 160, value );
  #endif
}


benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate& benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate::operator=(const SelfSimilarInfallFVQUpdate& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  _dataSmartPointer = other._dataSmartPointer;
  value = &( _dataSmartPointer[0] );
  
  return *this;
}


std::string benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate::toString() const {
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

void benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate::setDebugX( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugX = data;
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate::setDebugH( const tarch::la::Vector<Dimensions,double>& data ) {
  // At the moment, we only use square/cubic elements
  assertion2( data(0)>0.0, data, toString() );
  assertion2( data(0)==data(1), data, toString() );
  #if Dimensions==3
  assertion2( data(1)==data(2), data, toString() );
  #endif
  _debugH = data;
}


tarch::la::Vector<Dimensions,double> benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate::getDebugX() const {
  return _debugX;
}


tarch::la::Vector<Dimensions,double> benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate::getDebugH() const {
  return _debugH;
}
#endif


void benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate::merge(peano4::grid::TraversalObserver::SendReceiveContext context, const SelfSimilarInfallFVQUpdate& neighbour, const peano4::datamanagement::FaceMarker& marker) {
  
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate::send(
  const peano4::datamanagement::FaceMarker& marker
  
) const {
  return false;
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate::receiveAndMerge(
  const peano4::datamanagement::FaceMarker& marker
  
) const {
  return false;
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate::loadPersistently(
  const peano4::datamanagement::FaceMarker& marker
  
) {
  return false;
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate::storePersistently(
  const peano4::datamanagement::FaceMarker& marker
  
) const {
  return false;
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate::compute(
  const peano4::datamanagement::FaceMarker& marker
  
) const {
  return true;
}
#ifdef Parallel
void benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate::initDatatype() {
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}
  

MPI_Datatype   benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate::Datatype;


MPI_Datatype  benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate::getForkDatatype() {
  return Datatype;
}


MPI_Datatype  benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate::getJoinDatatype() {
  return Datatype;
}


MPI_Datatype  benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate::getBoundaryExchangeDatatype() {
  return Datatype;
}


MPI_Datatype  benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate::getMultiscaleDataExchangeDatatype() {
  return Datatype;
}


MPI_Datatype  benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQUpdate::getGlobalCommunciationDatatype() {
  return Datatype;
}

#endif