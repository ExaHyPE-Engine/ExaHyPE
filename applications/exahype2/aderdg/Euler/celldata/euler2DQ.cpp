#include "euler2DQ.h"


examples::exahype2::euler2D::celldata::euler2DQ::euler2DQ() {
}


examples::exahype2::euler2D::celldata::euler2DQ::euler2DQ(const euler2DQ& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  #if Dimensions==2
  std::copy(other.value, other.value+64,value);
  #else
  std::copy(other.value, other.value+256,value);
  #endif
}


examples::exahype2::euler2D::celldata::euler2DQ& examples::exahype2::euler2D::celldata::euler2DQ::operator=(const euler2DQ& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif
  
  #if Dimensions==2
  std::copy(other.value, other.value+64,value);
  #else
  std::copy(other.value, other.value+256,value);
  #endif
  return *this;
}


std::string examples::exahype2::euler2D::celldata::euler2DQ::toString() const {
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

void examples::exahype2::euler2D::celldata::euler2DQ::setDebugX( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugX = data;
}


void examples::exahype2::euler2D::celldata::euler2DQ::setDebugH( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugH = data;
}


tarch::la::Vector<Dimensions,double> examples::exahype2::euler2D::celldata::euler2DQ::getDebugX() const {
  return _debugX;
}


tarch::la::Vector<Dimensions,double> examples::exahype2::euler2D::celldata::euler2DQ::getDebugH() const {
  return _debugH;
}
#endif


void examples::exahype2::euler2D::celldata::euler2DQ::merge(peano4::grid::TraversalObserver::SendReceiveContext context, const euler2DQ& neighbour, const peano4::datamanagement::CellMarker& marker) {
  
}


bool examples::exahype2::euler2D::celldata::euler2DQ::send(
  const peano4::datamanagement::CellMarker& marker
  
) const {
  return true;
}


bool examples::exahype2::euler2D::celldata::euler2DQ::receiveAndMerge(
  const peano4::datamanagement::CellMarker& marker
  
) const {
  return true;
}


bool examples::exahype2::euler2D::celldata::euler2DQ::loadPersistently(
  const peano4::datamanagement::CellMarker& marker
  
) {
  return not marker.hasBeenRefined() and repositories::InstanceOfeuler2D.getSolverState()!=euler2D::SolverState::GridConstruction and repositories::InstanceOfeuler2D.getSolverState()!=euler2D::SolverState::GridInitialisation;
}


bool examples::exahype2::euler2D::celldata::euler2DQ::storePersistently(
  const peano4::datamanagement::CellMarker& marker
  
) const {
  return not marker.willBeRefined() and repositories::InstanceOfeuler2D.getSolverState()!=euler2D::SolverState::GridConstruction;
}


bool examples::exahype2::euler2D::celldata::euler2DQ::compute(
  const peano4::datamanagement::CellMarker& marker
  
) const {
  return true;
}
#ifdef Parallel
void examples::exahype2::euler2D::celldata::euler2DQ::initDatatype() {
  examples::exahype2::euler2D::celldata::euler2DQ  instances[2];
 
  #if PeanoDebug>=1
    MPI_Datatype subtypes[] = { MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE };

    #if Dimensions==2
    int blocklen[] = { Dimensions, Dimensions, 64 };
    #else
    int blocklen[] = { Dimensions, Dimensions, 256 };
    #endif

    const int NumberOfAttributes = 3;
  #else   
    MPI_Datatype subtypes[] = { MPI_DOUBLE };
    
    #if Dimensions==2
    int blocklen[] = { 64 };
    #else
    int blocklen[] = { 256 };
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


void examples::exahype2::euler2D::celldata::euler2DQ::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}
  

MPI_Datatype   examples::exahype2::euler2D::celldata::euler2DQ::Datatype;


MPI_Datatype  examples::exahype2::euler2D::celldata::euler2DQ::getForkDatatype() {
  return Datatype;
}


MPI_Datatype  examples::exahype2::euler2D::celldata::euler2DQ::getJoinDatatype() {
  return Datatype;
}


MPI_Datatype  examples::exahype2::euler2D::celldata::euler2DQ::getBoundaryExchangeDatatype() {
  return Datatype;
}


MPI_Datatype  examples::exahype2::euler2D::celldata::euler2DQ::getMultiscaleDataExchangeDatatype() {
  return Datatype;
}


MPI_Datatype  examples::exahype2::euler2D::celldata::euler2DQ::getGlobalCommunciationDatatype() {
  return Datatype;
}

#endif