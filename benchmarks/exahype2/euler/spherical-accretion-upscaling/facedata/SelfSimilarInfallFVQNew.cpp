#include "SelfSimilarInfallFVQNew.h"

#include "tarch/accelerator/accelerator.h"


benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew::SelfSimilarInfallFVQNew():
  _dataSmartPointer(
    static_cast< double* >( allocateMemory(Cardinality, tarch::MemoryLocation::Heap) )
  ) {
  value = &( _dataSmartPointer[0] );
}


benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew::SelfSimilarInfallFVQNew(const SelfSimilarInfallFVQNew& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  _dataSmartPointer = other._dataSmartPointer;
  value = &( _dataSmartPointer[0] );
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew::clone(const SelfSimilarInfallFVQNew& other) {
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


benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew& benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew::operator=(const SelfSimilarInfallFVQNew& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  _dataSmartPointer = other._dataSmartPointer;
  value = &( _dataSmartPointer[0] );
  
  return *this;
}


std::string benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew::toString() const {
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

void benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew::setDebugX( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugX = data;
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew::setDebugH( const tarch::la::Vector<Dimensions,double>& data ) {
  // At the moment, we only use square/cubic elements
  assertion2( data(0)>0.0, data, toString() );
  assertion2( data(0)==data(1), data, toString() );
  #if Dimensions==3
  assertion2( data(1)==data(2), data, toString() );
  #endif
  _debugH = data;
}


tarch::la::Vector<Dimensions,double> benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew::getDebugX() const {
  return _debugX;
}


tarch::la::Vector<Dimensions,double> benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew::getDebugH() const {
  return _debugH;
}
#endif


void benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew::merge(peano4::grid::TraversalObserver::SendReceiveContext context, const SelfSimilarInfallFVQNew& neighbour, const peano4::datamanagement::FaceMarker& marker) {
  
  auto serialisePatchIndex = [](tarch::la::Vector<Dimensions,int> overlapCell, int normal) {
    int base   = 1;
    int result = 0;
    for (int d=0; d<Dimensions; d++) {
      result += overlapCell(d) * base;
      if (d==normal) {
        base *= 1*2;
      }
      else {
        base *= 4;
      }
    }
    return result;
  };
  
  
  switch (context) {
    case ::peano4::grid::TraversalObserver::SendReceiveContext::PeriodicBoundaryDataSwap:
    case ::peano4::grid::TraversalObserver::SendReceiveContext::BoundaryExchange:
      {
        const int faceNormal = marker.getSelectedFaceNumber() % Dimensions;
        dfore(i,4,faceNormal,0) {
          for (int j=0; j<1; j++) {
            tarch::la::Vector<Dimensions,int> volume = i;
            volume(faceNormal) += marker.outerNormal()(faceNormal)>0 ? j + 1 : j;
            
            int volumeSerialised = serialisePatchIndex(volume, faceNormal);
            for (int k=0; k<5; k++) {
              assertion5( 
                neighbour.value[volumeSerialised*5+k]==neighbour.value[volumeSerialised*5+k],
                value[volumeSerialised*5+k],
                k, 5,
                volume,
                marker.toString()
              );
              
              value[volumeSerialised*5+k] = neighbour.value[volumeSerialised*5+k];
             
            }
              
            // This should be non-critical assertion, but this assertion is only
            // generically available in ExaHyPE 2, so I comment the assertion out
            // here.
            // assertion(value[volumeSerialised]==value[volumeSerialised]);
          }
        }
      }
      break;
      
    case ::peano4::grid::TraversalObserver::SendReceiveContext::MultiscaleExchange:
      assertionMsg( false, "not implemented yet" );
      break;

    default:
      assertionMsg( false, "not implemented yet" );
      break;
  }

}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew::send(
  const peano4::datamanagement::FaceMarker& marker
  
) const {
  return (repositories::InstanceOfSelfSimilarInfallFV.getSolverState()==SelfSimilarInfallFV::SolverState::Secondary or repositories::InstanceOfSelfSimilarInfallFV.getSolverState()==SelfSimilarInfallFV::SolverState::GridInitialisation or repositories::InstanceOfSelfSimilarInfallFV.getSolverState()==SelfSimilarInfallFV::SolverState::PlottingInitialCondition or repositories::InstanceOfSelfSimilarInfallFV.getSolverState()==SelfSimilarInfallFV::SolverState::Plotting );
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew::receiveAndMerge(
  const peano4::datamanagement::FaceMarker& marker
  
) const {
  return (repositories::InstanceOfSelfSimilarInfallFV.getSolverState()==SelfSimilarInfallFV::SolverState::Primary or repositories::InstanceOfSelfSimilarInfallFV.getSolverState()==SelfSimilarInfallFV::SolverState::PrimaryAfterGridInitialisation or repositories::InstanceOfSelfSimilarInfallFV.getSolverState()==SelfSimilarInfallFV::SolverState::PlottingInitialCondition or repositories::InstanceOfSelfSimilarInfallFV.getSolverState()==SelfSimilarInfallFV::SolverState::Plotting );
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew::loadPersistently(
  const peano4::datamanagement::FaceMarker& marker
  
) {
  return not marker.hasBeenRefined() and repositories::InstanceOfSelfSimilarInfallFV.getSolverState()!=SelfSimilarInfallFV::SolverState::GridConstruction and repositories::InstanceOfSelfSimilarInfallFV.getSolverState()!=SelfSimilarInfallFV::SolverState::GridInitialisation;
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew::storePersistently(
  const peano4::datamanagement::FaceMarker& marker
  
) const {
  return not marker.willBeRefined() and repositories::InstanceOfSelfSimilarInfallFV.getSolverState()!=SelfSimilarInfallFV::SolverState::GridConstruction;
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew::compute(
  const peano4::datamanagement::FaceMarker& marker
  
) const {
  return true;
}
#ifdef Parallel
void benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew::initDatatype() {
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}
  

MPI_Datatype   benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew::Datatype;


MPI_Datatype  benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew::getForkDatatype() {
  return Datatype;
}


MPI_Datatype  benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew::getJoinDatatype() {
  return Datatype;
}


MPI_Datatype  benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew::getBoundaryExchangeDatatype() {
  return Datatype;
}


MPI_Datatype  benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew::getMultiscaleDataExchangeDatatype() {
  return Datatype;
}


MPI_Datatype  benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::SelfSimilarInfallFVQNew::getGlobalCommunciationDatatype() {
  return Datatype;
}

#endif