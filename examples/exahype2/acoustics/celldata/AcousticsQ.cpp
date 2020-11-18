#include "AcousticsQ.h"





examples::exahype2::acoustics::celldata::AcousticsQ::AcousticsQ() {
}


examples::exahype2::acoustics::celldata::AcousticsQ::AcousticsQ(const AcousticsQ& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  #if Dimensions==2
  std::copy(other.value, other.value+484,value);
  #else
  std::copy(other.value, other.value+5324,value);
  #endif
}


examples::exahype2::acoustics::celldata::AcousticsQ& examples::exahype2::acoustics::celldata::AcousticsQ::operator=(const AcousticsQ& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif
  
  #if Dimensions==2
  std::copy(other.value, other.value+484,value);
  #else
  std::copy(other.value, other.value+5324,value);
  #endif
  return *this;
}


std::string examples::exahype2::acoustics::celldata::AcousticsQ::toString() const {
  std::string result = std::string("()");
  return result;
}


#if PeanoDebug>=1

void examples::exahype2::acoustics::celldata::AcousticsQ::setDebugX( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugX = data;
}


void examples::exahype2::acoustics::celldata::AcousticsQ::setDebugH( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugH = data;
}


tarch::la::Vector<Dimensions,double> examples::exahype2::acoustics::celldata::AcousticsQ::getDebugX() const {
  return _debugX;
}


tarch::la::Vector<Dimensions,double> examples::exahype2::acoustics::celldata::AcousticsQ::getDebugH() const {
  return _debugH;
}
#endif


void examples::exahype2::acoustics::celldata::AcousticsQ::merge(const AcousticsQ& neighbour, const peano4::datamanagement::CellMarker& marker) {
  
}


bool examples::exahype2::acoustics::celldata::AcousticsQ::send(const peano4::datamanagement::CellMarker& marker) {
  return true;
}


bool examples::exahype2::acoustics::celldata::AcousticsQ::receiveAndMerge(const peano4::datamanagement::CellMarker& marker) {
  return true;
}


bool examples::exahype2::acoustics::celldata::AcousticsQ::storePersistently(const peano4::datamanagement::CellMarker& marker) {
  return true;
}


bool examples::exahype2::acoustics::celldata::AcousticsQ::loadPersistently(const peano4::datamanagement::CellMarker& marker) {
  return true;
}
#ifdef Parallel
void examples::exahype2::acoustics::celldata::AcousticsQ::initDatatype() {
  examples::exahype2::acoustics::celldata::AcousticsQ  instances[2];
 
  #if PeanoDebug>=1
    MPI_Datatype subtypes[] = { MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE };

    #if Dimensions==2
    int blocklen[] = { Dimensions, Dimensions, 484 };
    #else
    int blocklen[] = { Dimensions, Dimensions, 5324 };
    #endif

    const int NumberOfAttributes = 3;
  #else   
    MPI_Datatype subtypes[] = { MPI_DOUBLE };
    
    #if Dimensions==2
    int blocklen[] = { 484 };
    #else
    int blocklen[] = { 5324 };
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


void examples::exahype2::acoustics::celldata::AcousticsQ::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}
  

MPI_Datatype   examples::exahype2::acoustics::celldata::AcousticsQ::Datatype;
#endif