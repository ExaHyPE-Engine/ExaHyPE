#include "AcousticsQ.h"



#include "peano4/utils/Loop.h"
#include "observers/SolverRepository.h" 



examples::exahype2::acoustics::facedata::AcousticsQ::AcousticsQ() {
}


examples::exahype2::acoustics::facedata::AcousticsQ::AcousticsQ(const AcousticsQ& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  #if Dimensions==2
  std::copy(other.value, other.value+88,value);
  #else
  std::copy(other.value, other.value+968,value);
  #endif
}


examples::exahype2::acoustics::facedata::AcousticsQ& examples::exahype2::acoustics::facedata::AcousticsQ::operator=(const AcousticsQ& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif
  
  #if Dimensions==2
  std::copy(other.value, other.value+88,value);
  #else
  std::copy(other.value, other.value+968,value);
  #endif
  return *this;
}


std::string examples::exahype2::acoustics::facedata::AcousticsQ::toString() const {
  std::string result = std::string("()");
  return result;
}


#if PeanoDebug>=1

void examples::exahype2::acoustics::facedata::AcousticsQ::setDebugX( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugX = data;
}


void examples::exahype2::acoustics::facedata::AcousticsQ::setDebugH( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugH = data;
}


tarch::la::Vector<Dimensions,double> examples::exahype2::acoustics::facedata::AcousticsQ::getDebugX() const {
  return _debugX;
}


tarch::la::Vector<Dimensions,double> examples::exahype2::acoustics::facedata::AcousticsQ::getDebugH() const {
  return _debugH;
}
#endif


void examples::exahype2::acoustics::facedata::AcousticsQ::merge(const AcousticsQ& neighbour, const peano4::datamanagement::FaceMarker& marker) {
  
  auto serialisePatchIndex = [](tarch::la::Vector<Dimensions,int> overlapCell, int normal) {
    int base   = 1;
    int result = 0;
    for (int d=0; d<Dimensions; d++) {
      result += overlapCell(d) * base;
      if (d==normal) {
        base *= 1.0*2;
      }
      else {
        base *= 11;
      }
    }
    return result;
  };
  

  const int faceNormal = marker.getSelectedFaceNumber() % Dimensions;
  dfore(i,11,faceNormal,0) {
    for (int j=0; j<1.0; j++) {
      tarch::la::Vector<Dimensions,int> volume = i;
      volume(faceNormal) += marker.outerNormal()(faceNormal)>0 ? j + 1.0 : j;
      
      int volumeSerialised = serialisePatchIndex(volume, faceNormal);
      for (int k=0; k<4; k++) {
        assertion4( 
          neighbour.value[volumeSerialised*4+k]==neighbour.value[volumeSerialised*4+k],
          k, 4,
          volume,
          marker.toString()
        );
      
        value[volumeSerialised*4+k] = neighbour.value[volumeSerialised*4+k];
      
      }
      
      // This should be non-critical assertion, but this assertion is only
      // generically available in ExaHyPE 2, so I comment the assertion out
      // here.
      // assertion(value[volumeSerialised]==value[volumeSerialised]);
    }
  }

}


bool examples::exahype2::acoustics::facedata::AcousticsQ::send(const peano4::datamanagement::FaceMarker& marker) {
  return not marker.isRefined() and observers::InstanceOfAcoustics.getSolverState()!=Acoustics::SolverState::GridConstruction;
}


bool examples::exahype2::acoustics::facedata::AcousticsQ::receiveAndMerge(const peano4::datamanagement::FaceMarker& marker) {
  return not marker.isRefined() and observers::InstanceOfAcoustics.getSolverState()!=Acoustics::SolverState::GridConstruction and observers::InstanceOfAcoustics.getSolverState()!=Acoustics::SolverState::GridInitialisation;
}


bool examples::exahype2::acoustics::facedata::AcousticsQ::storePersistently(const peano4::datamanagement::FaceMarker& marker) {
  return true;
}


bool examples::exahype2::acoustics::facedata::AcousticsQ::loadPersistently(const peano4::datamanagement::FaceMarker& marker) {
  return true;
}
#ifdef Parallel
void examples::exahype2::acoustics::facedata::AcousticsQ::initDatatype() {
  examples::exahype2::acoustics::facedata::AcousticsQ  instances[2];
 
  #if PeanoDebug>=1
    MPI_Datatype subtypes[] = { MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE };

    #if Dimensions==2
    int blocklen[] = { Dimensions, Dimensions, 88 };
    #else
    int blocklen[] = { Dimensions, Dimensions, 968 };
    #endif

    const int NumberOfAttributes = 3;
  #else   
    MPI_Datatype subtypes[] = { MPI_DOUBLE };
    
    #if Dimensions==2
    int blocklen[] = { 88 };
    #else
    int blocklen[] = { 968 };
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


void examples::exahype2::acoustics::facedata::AcousticsQ::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}
  

MPI_Datatype   examples::exahype2::acoustics::facedata::AcousticsQ::Datatype;
#endif