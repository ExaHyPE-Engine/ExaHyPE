#include "SelfSimilarInfallFVQ.h"

#include "tarch/accelerator/accelerator.h"


benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ::SelfSimilarInfallFVQ():
  _dataSmartPointer(
    static_cast< double* >( allocateMemory(Cardinality, tarch::MemoryLocation::Heap) )
  ) {
  value = &( _dataSmartPointer[0] );
}


benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ::SelfSimilarInfallFVQ(const SelfSimilarInfallFVQ& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  _dataSmartPointer = other._dataSmartPointer;
  value = &( _dataSmartPointer[0] );
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ::clone(const SelfSimilarInfallFVQ& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  #if Dimensions==2
  std::copy_n( other.value, 80, value );
  #else
  std::copy_n( other.value, 320, value );
  #endif
}


benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ& benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ::operator=(const SelfSimilarInfallFVQ& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  _dataSmartPointer = other._dataSmartPointer;
  value = &( _dataSmartPointer[0] );
  
  return *this;
}


std::string benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ::toString() const {
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

void benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ::setDebugX( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugX = data;
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ::setDebugH( const tarch::la::Vector<Dimensions,double>& data ) {
  // At the moment, we only use square/cubic elements
  assertion2( data(0)>0.0, data, toString() );
  assertion2( data(0)==data(1), data, toString() );
  #if Dimensions==3
  assertion2( data(1)==data(2), data, toString() );
  #endif
  _debugH = data;
}


tarch::la::Vector<Dimensions,double> benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ::getDebugX() const {
  return _debugX;
}


tarch::la::Vector<Dimensions,double> benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ::getDebugH() const {
  return _debugH;
}
#endif


void benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ::merge(peano4::grid::TraversalObserver::SendReceiveContext context, const SelfSimilarInfallFVQ& neighbour, const peano4::datamanagement::CellMarker& marker) {
  
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


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ::send(
  const peano4::datamanagement::CellMarker& marker
  
) const {
  return true;
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ::receiveAndMerge(
  const peano4::datamanagement::CellMarker& marker
  
) const {
  return true;
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ::loadPersistently(
  const peano4::datamanagement::CellMarker& marker
  
) {
  return not marker.hasBeenRefined() and repositories::InstanceOfSelfSimilarInfallFV.getSolverState()!=SelfSimilarInfallFV::SolverState::GridConstruction and repositories::InstanceOfSelfSimilarInfallFV.getSolverState()!=SelfSimilarInfallFV::SolverState::GridInitialisation and ((repositories::InstanceOfSelfSimilarInfallFV.getSolverState()==SelfSimilarInfallFV::SolverState::Primary or repositories::InstanceOfSelfSimilarInfallFV.getSolverState()==SelfSimilarInfallFV::SolverState::PrimaryAfterGridInitialisation or repositories::InstanceOfSelfSimilarInfallFV.getSolverState()==SelfSimilarInfallFV::SolverState::PlottingInitialCondition or repositories::InstanceOfSelfSimilarInfallFV.getSolverState()==SelfSimilarInfallFV::SolverState::Plotting ) or marker.hasBeenSkeletonCell());
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ::storePersistently(
  const peano4::datamanagement::CellMarker& marker
  
) const {
  return not marker.willBeRefined() and repositories::InstanceOfSelfSimilarInfallFV.getSolverState()!=SelfSimilarInfallFV::SolverState::GridConstruction and ((repositories::InstanceOfSelfSimilarInfallFV.getSolverState()==SelfSimilarInfallFV::SolverState::Secondary or repositories::InstanceOfSelfSimilarInfallFV.getSolverState()==SelfSimilarInfallFV::SolverState::GridInitialisation or repositories::InstanceOfSelfSimilarInfallFV.getSolverState()==SelfSimilarInfallFV::SolverState::PlottingInitialCondition or repositories::InstanceOfSelfSimilarInfallFV.getSolverState()==SelfSimilarInfallFV::SolverState::Plotting ) or marker.willBeSkeletonCell());
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ::compute(
  const peano4::datamanagement::CellMarker& marker
  
) const {
  return true;
}
#ifdef Parallel
void benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ::initDatatype() {
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}
  

MPI_Datatype   benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ::Datatype;


MPI_Datatype  benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ::getForkDatatype() {
  return Datatype;
}


MPI_Datatype  benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ::getJoinDatatype() {
  return Datatype;
}


MPI_Datatype  benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ::getBoundaryExchangeDatatype() {
  return Datatype;
}


MPI_Datatype  benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ::getMultiscaleDataExchangeDatatype() {
  return Datatype;
}


MPI_Datatype  benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::SelfSimilarInfallFVQ::getGlobalCommunciationDatatype() {
  return Datatype;
}

#endif