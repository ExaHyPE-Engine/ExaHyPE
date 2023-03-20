#include "SelfSimilarInfallFD4Q.h"

#include "tarch/accelerator/accelerator.h"


benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::SelfSimilarInfallFD4Q():
  _dataSmartPointer(
    static_cast< double* >( allocateMemory(Cardinality, tarch::MemoryLocation::Heap) )
  ) {
  value = &( _dataSmartPointer[0] );
}


benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::SelfSimilarInfallFD4Q(const SelfSimilarInfallFD4Q& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  _dataSmartPointer = other._dataSmartPointer;
  value = &( _dataSmartPointer[0] );
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::clone(const SelfSimilarInfallFD4Q& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  #if Dimensions==2
  std::copy_n( other.value, 125, value );
  #else
  std::copy_n( other.value, 625, value );
  #endif
}


benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q& benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::operator=(const SelfSimilarInfallFD4Q& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  _dataSmartPointer = other._dataSmartPointer;
  value = &( _dataSmartPointer[0] );
  
  return *this;
}


std::string benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::toString() const {
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

void benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::setDebugX( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugX = data;
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::setDebugH( const tarch::la::Vector<Dimensions,double>& data ) {
  // At the moment, we only use square/cubic elements
  assertion2( data(0)>0.0, data, toString() );
  assertion2( data(0)==data(1), data, toString() );
  #if Dimensions==3
  assertion2( data(1)==data(2), data, toString() );
  #endif
  _debugH = data;
}


tarch::la::Vector<Dimensions,double> benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::getDebugX() const {
  return _debugX;
}


tarch::la::Vector<Dimensions,double> benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::getDebugH() const {
  return _debugH;
}
#endif


void benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::merge(peano4::grid::TraversalObserver::SendReceiveContext context, const SelfSimilarInfallFD4Q& neighbour, const peano4::datamanagement::CellMarker& marker) {
  
  switch (context) {
    case ::peano4::grid::TraversalObserver::SendReceiveContext::BoundaryExchange:
      assertionMsg( false, "should not be called, as cells (on the same level) cannot overlap in Peano" );
      break;
      
    case ::peano4::grid::TraversalObserver::SendReceiveContext::MultiscaleExchange:
      assertionMsg( false, "not implemented yet" );
      break;

    case ::peano4::grid::TraversalObserver::SendReceiveContext::PeriodicBoundaryDataSwap:
      assertionMsg( false, "should never be called" );
      break;
      
    default:
      assertionMsg( false, "not implemented yet" );
      break;
  }

}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::send(
  const peano4::datamanagement::CellMarker& marker
  
) const {
  return true;
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::receiveAndMerge(
  const peano4::datamanagement::CellMarker& marker
  
) const {
  return true;
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::loadPersistently(
  const peano4::datamanagement::CellMarker& marker
  
) {
  return not marker.hasBeenRefined() and repositories::InstanceOfSelfSimilarInfallFD4.getSolverState()!=SelfSimilarInfallFD4::SolverState::GridConstruction and repositories::InstanceOfSelfSimilarInfallFD4.getSolverState()!=SelfSimilarInfallFD4::SolverState::GridInitialisation;
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::storePersistently(
  const peano4::datamanagement::CellMarker& marker
  
) const {
  return not marker.willBeRefined() and repositories::InstanceOfSelfSimilarInfallFD4.getSolverState()!=SelfSimilarInfallFD4::SolverState::GridConstruction;
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::compute(
  const peano4::datamanagement::CellMarker& marker
  
) const {
  return true;
}
#ifdef Parallel
void benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::initDatatype() {
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}
  

MPI_Datatype   benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::Datatype;


MPI_Datatype  benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::getForkDatatype() {
  return Datatype;
}


MPI_Datatype  benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::getJoinDatatype() {
  return Datatype;
}


MPI_Datatype  benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::getBoundaryExchangeDatatype() {
  return Datatype;
}


MPI_Datatype  benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::getMultiscaleDataExchangeDatatype() {
  return Datatype;
}


MPI_Datatype  benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFD4Q::getGlobalCommunciationDatatype() {
  return Datatype;
}

#endif