#include "LOH1Q.h"


 #include "observers/SolverRepository.h" 


examples::exahype2::loh1::celldata::LOH1Q::LOH1Q() {
}


examples::exahype2::loh1::celldata::LOH1Q::LOH1Q(const LOH1Q& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  #if Dimensions==2
  std::copy(other.value, other.value+325,value);
  #else
  std::copy(other.value, other.value+1625,value);
  #endif
}


examples::exahype2::loh1::celldata::LOH1Q& examples::exahype2::loh1::celldata::LOH1Q::operator=(const LOH1Q& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif
  
  #if Dimensions==2
  std::copy(other.value, other.value+325,value);
  #else
  std::copy(other.value, other.value+1625,value);
  #endif
  return *this;
}


std::string examples::exahype2::loh1::celldata::LOH1Q::toString() const {
  std::string result = std::string("()");
  return result;
}


#if PeanoDebug>=1

void examples::exahype2::loh1::celldata::LOH1Q::setDebugX( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugX = data;
}


void examples::exahype2::loh1::celldata::LOH1Q::setDebugH( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugH = data;
}


tarch::la::Vector<Dimensions,double> examples::exahype2::loh1::celldata::LOH1Q::getDebugX() const {
  return _debugX;
}


tarch::la::Vector<Dimensions,double> examples::exahype2::loh1::celldata::LOH1Q::getDebugH() const {
  return _debugH;
}
#endif


void examples::exahype2::loh1::celldata::LOH1Q::merge(const LOH1Q& neighbour, const peano4::datamanagement::CellMarker& marker) {
  
}


bool examples::exahype2::loh1::celldata::LOH1Q::send(const peano4::datamanagement::CellMarker& marker) {
  return true;
}


bool examples::exahype2::loh1::celldata::LOH1Q::receiveAndMerge(const peano4::datamanagement::CellMarker& marker) {
  return true;
}


bool examples::exahype2::loh1::celldata::LOH1Q::storePersistently(const peano4::datamanagement::CellMarker& marker) {
  return true;
}


bool examples::exahype2::loh1::celldata::LOH1Q::loadPersistently(const peano4::datamanagement::CellMarker& marker) {
  return true;
}
#ifdef Parallel
void examples::exahype2::loh1::celldata::LOH1Q::initDatatype() {
  examples::exahype2::loh1::celldata::LOH1Q  instances[2];
 
  #if PeanoDebug>=1
    MPI_Datatype subtypes[] = { MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE };

    #if Dimensions==2
    int blocklen[] = { Dimensions, Dimensions, 325 };
    #else
    int blocklen[] = { Dimensions, Dimensions, 1625 };
    #endif

    const int NumberOfAttributes = 3;
  #else   
    MPI_Datatype subtypes[] = { MPI_DOUBLE };
    
    #if Dimensions==2
    int blocklen[] = { 325 };
    #else
    int blocklen[] = { 1625 };
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


void examples::exahype2::loh1::celldata::LOH1Q::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}
  

MPI_Datatype   examples::exahype2::loh1::celldata::LOH1Q::Datatype;
#endif