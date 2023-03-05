#include "euler2DQEstimates.h"


examples::exahype2::euler2D::facedata::euler2DQEstimates::euler2DQEstimates() {
}


examples::exahype2::euler2D::facedata::euler2DQEstimates::euler2DQEstimates(const euler2DQEstimates& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  #if Dimensions==2
  std::copy(other.value, other.value+32,value);
  #else
  std::copy(other.value, other.value+128,value);
  #endif
}


examples::exahype2::euler2D::facedata::euler2DQEstimates& examples::exahype2::euler2D::facedata::euler2DQEstimates::operator=(const euler2DQEstimates& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif
  
  #if Dimensions==2
  std::copy(other.value, other.value+32,value);
  #else
  std::copy(other.value, other.value+128,value);
  #endif
  return *this;
}


std::string examples::exahype2::euler2D::facedata::euler2DQEstimates::toString() const {
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

void examples::exahype2::euler2D::facedata::euler2DQEstimates::setDebugX( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugX = data;
}


void examples::exahype2::euler2D::facedata::euler2DQEstimates::setDebugH( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugH = data;
}


tarch::la::Vector<Dimensions,double> examples::exahype2::euler2D::facedata::euler2DQEstimates::getDebugX() const {
  return _debugX;
}


tarch::la::Vector<Dimensions,double> examples::exahype2::euler2D::facedata::euler2DQEstimates::getDebugH() const {
  return _debugH;
}
#endif


void examples::exahype2::euler2D::facedata::euler2DQEstimates::merge(peano4::grid::TraversalObserver::SendReceiveContext context, const euler2DQEstimates& neighbour, const peano4::datamanagement::FaceMarker& marker) {
  
  
  #if Dimensions==2
  constexpr int spaceFaceSize = 4*4; //Order+1
  #else
  constexpr int spaceFaceSize = 4*4*4; //(Order+1)^2
  #endif
  
  const int faceDimension = marker.getSelectedFaceNumber()%Dimensions;
  //if the outer normal is positive, the normal points to the right so the face is on the right
  const int faceLR = (marker.outerNormal()[faceDimension]>0.0 ? 0 : 1);

  //  std::copy_n(from, how_many, to);
  std::copy_n(
      &neighbour.value[(1-faceLR)*spaceFaceSize],
      spaceFaceSize,
      &value[(1-faceLR)*spaceFaceSize]
     );
  

}


bool examples::exahype2::euler2D::facedata::euler2DQEstimates::send(
  const peano4::datamanagement::FaceMarker& marker
  
) const {
  return repositories::InstanceOfeuler2D.getSolverState()==euler2D::SolverState::Prediction;
}


bool examples::exahype2::euler2D::facedata::euler2DQEstimates::receiveAndMerge(
  const peano4::datamanagement::FaceMarker& marker
  
) const {
  return repositories::InstanceOfeuler2D.getSolverState()==euler2D::SolverState::Correction;
}


bool examples::exahype2::euler2D::facedata::euler2DQEstimates::loadPersistently(
  const peano4::datamanagement::FaceMarker& marker
  
) {
  return not marker.hasBeenRefined() and repositories::InstanceOfeuler2D.getSolverState()!=euler2D::SolverState::GridConstruction and repositories::InstanceOfeuler2D.getSolverState()!=euler2D::SolverState::GridInitialisation;
}


bool examples::exahype2::euler2D::facedata::euler2DQEstimates::storePersistently(
  const peano4::datamanagement::FaceMarker& marker
  
) const {
  return not marker.willBeRefined() and repositories::InstanceOfeuler2D.getSolverState()!=euler2D::SolverState::GridConstruction;
}


bool examples::exahype2::euler2D::facedata::euler2DQEstimates::compute(
  const peano4::datamanagement::FaceMarker& marker
  
) const {
  return true;
}
#ifdef Parallel
void examples::exahype2::euler2D::facedata::euler2DQEstimates::initDatatype() {
  examples::exahype2::euler2D::facedata::euler2DQEstimates  instances[2];
 
  #if PeanoDebug>=1
    MPI_Datatype subtypes[] = { MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE };

    #if Dimensions==2
    int blocklen[] = { Dimensions, Dimensions, 32 };
    #else
    int blocklen[] = { Dimensions, Dimensions, 128 };
    #endif

    const int NumberOfAttributes = 3;
  #else   
    MPI_Datatype subtypes[] = { MPI_DOUBLE };
    
    #if Dimensions==2
    int blocklen[] = { 32 };
    #else
    int blocklen[] = { 128 };
    #endif
  
    const int NumberOfAttributes = 1;  
  #endif

  MPI_Aint  baseFirstInstance;
  MPI_Aint  baseSecondInstance;
  MPI_Get_address( &instances[0], &baseFirstInstance );
  MPI_Get_address( &instances[1], &baseSecondInstance );
  MPI_Aint  disp[ NumberOfAttributes ];
  int       currentAddress = 0;
  #if PeanoDebug>=1
  MPI_Get_address( &(instances[0]._debugX.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._debugH.data()[0]), &disp[currentAddress] );
  currentAddress++;
  #endif
  MPI_Get_address( &(instances[0].value), &disp[currentAddress] );
  currentAddress++;

  MPI_Aint offset = disp[0] - baseFirstInstance;
  MPI_Aint extent = baseSecondInstance - baseFirstInstance - offset;
  for (int i=NumberOfAttributes-1; i>=0; i--) {
    disp[i] = disp[i] - disp[0];
  }

  int errorCode = 0; 
  MPI_Datatype tmpType; 
  errorCode += MPI_Type_create_struct( NumberOfAttributes, blocklen, disp, subtypes, &tmpType );
  errorCode += MPI_Type_create_resized( tmpType, offset, extent, &Datatype );
  errorCode += MPI_Type_commit( &Datatype );
  errorCode += MPI_Type_free( &tmpType );
  if (errorCode) std::cerr << "error constructing MPI datatype in " << __FILE__ << ":" << __LINE__ << std::endl;
}


void examples::exahype2::euler2D::facedata::euler2DQEstimates::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}
  

MPI_Datatype   examples::exahype2::euler2D::facedata::euler2DQEstimates::Datatype;


MPI_Datatype  examples::exahype2::euler2D::facedata::euler2DQEstimates::getForkDatatype() {
  return Datatype;
}


MPI_Datatype  examples::exahype2::euler2D::facedata::euler2DQEstimates::getJoinDatatype() {
  return Datatype;
}


MPI_Datatype  examples::exahype2::euler2D::facedata::euler2DQEstimates::getBoundaryExchangeDatatype() {
  return Datatype;
}


MPI_Datatype  examples::exahype2::euler2D::facedata::euler2DQEstimates::getMultiscaleDataExchangeDatatype() {
  return Datatype;
}


MPI_Datatype  examples::exahype2::euler2D::facedata::euler2DQEstimates::getGlobalCommunciationDatatype() {
  return Datatype;
}

#endif