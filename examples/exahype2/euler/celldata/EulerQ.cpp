#include "EulerQ.h"


 #include "observers/SolverRepository.h" 


examples::exahype2::euler::celldata::EulerQ::EulerQ() {
}


examples::exahype2::euler::celldata::EulerQ::EulerQ(const EulerQ& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  #if Dimensions==2
  std::copy(other.value, other.value+245,value);
  #else
  std::copy(other.value, other.value+1715,value);
  #endif
}


examples::exahype2::euler::celldata::EulerQ& examples::exahype2::euler::celldata::EulerQ::operator=(const EulerQ& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif
  
  #if Dimensions==2
  std::copy(other.value, other.value+245,value);
  #else
  std::copy(other.value, other.value+1715,value);
  #endif
  return *this;
}


std::string examples::exahype2::euler::celldata::EulerQ::toString() const {
  std::string result = std::string("()");
  return result;
}


#if PeanoDebug>=1

void examples::exahype2::euler::celldata::EulerQ::setDebugX( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugX = data;
}


void examples::exahype2::euler::celldata::EulerQ::setDebugH( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugH = data;
}


tarch::la::Vector<Dimensions,double> examples::exahype2::euler::celldata::EulerQ::getDebugX() const {
  return _debugX;
}


tarch::la::Vector<Dimensions,double> examples::exahype2::euler::celldata::EulerQ::getDebugH() const {
  return _debugH;
}
#endif


void examples::exahype2::euler::celldata::EulerQ::merge(const EulerQ& neighbour, const peano4::datamanagement::CellMarker& marker) {
  
}


bool examples::exahype2::euler::celldata::EulerQ::send(const peano4::datamanagement::CellMarker& marker) {
  return true;
}


bool examples::exahype2::euler::celldata::EulerQ::receiveAndMerge(const peano4::datamanagement::CellMarker& marker) {
  return true;
}


bool examples::exahype2::euler::celldata::EulerQ::storePersistently(const peano4::datamanagement::CellMarker& marker) {
  return true;
}


bool examples::exahype2::euler::celldata::EulerQ::loadPersistently(const peano4::datamanagement::CellMarker& marker) {
  return true;
}
#ifdef Parallel
void examples::exahype2::euler::celldata::EulerQ::initDatatype() {
  examples::exahype2::euler::celldata::EulerQ  instances[2];
 
  #if PeanoDebug>=1
    MPI_Datatype subtypes[] = { MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE };

    #if Dimensions==2
    int blocklen[] = { Dimensions, Dimensions, 245 };
    #else
    int blocklen[] = { Dimensions, Dimensions, 1715 };
    #endif

    const int NumberOfAttributes = 3;
  #else   
    MPI_Datatype subtypes[] = { MPI_DOUBLE };
    
    #if Dimensions==2
    int blocklen[] = { 245 };
    #else
    int blocklen[] = { 1715 };
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


void examples::exahype2::euler::celldata::EulerQ::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}
  

MPI_Datatype   examples::exahype2::euler::celldata::EulerQ::Datatype;
#endif