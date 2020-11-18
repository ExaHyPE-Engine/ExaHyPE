#include "LOH1Q.h"



#include "peano4/utils/Loop.h" 

#include "observers/SolverRepository.h" 
#include "peano4/utils/Loop.h" 



examples::exahype2::loh1::facedata::LOH1Q::LOH1Q() {
}


examples::exahype2::loh1::facedata::LOH1Q::LOH1Q(const LOH1Q& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  #if Dimensions==2
  std::copy(other.value, other.value+130,value);
  #else
  std::copy(other.value, other.value+650,value);
  #endif
}


examples::exahype2::loh1::facedata::LOH1Q& examples::exahype2::loh1::facedata::LOH1Q::operator=(const LOH1Q& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif
  
  #if Dimensions==2
  std::copy(other.value, other.value+130,value);
  #else
  std::copy(other.value, other.value+650,value);
  #endif
  return *this;
}


std::string examples::exahype2::loh1::facedata::LOH1Q::toString() const {
  std::string result = std::string("()");
  return result;
}


#if PeanoDebug>=1

void examples::exahype2::loh1::facedata::LOH1Q::setDebugX( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugX = data;
}


void examples::exahype2::loh1::facedata::LOH1Q::setDebugH( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugH = data;
}


tarch::la::Vector<Dimensions,double> examples::exahype2::loh1::facedata::LOH1Q::getDebugX() const {
  return _debugX;
}


tarch::la::Vector<Dimensions,double> examples::exahype2::loh1::facedata::LOH1Q::getDebugH() const {
  return _debugH;
}
#endif


void examples::exahype2::loh1::facedata::LOH1Q::merge(const LOH1Q& neighbour, const peano4::datamanagement::FaceMarker& marker) {
  
  auto serialisePatchIndex = [](tarch::la::Vector<Dimensions,int> overlapCell, int normal) {
    int base   = 1;
    int result = 0;
    for (int d=0; d<Dimensions; d++) {
      result += overlapCell(d) * base;
      if (d==normal) {
        base *= 1.0*2;
      }
      else {
        base *= 5;
      }
    }
    return result;
  };
  
  //
  // The face merges are always called from a cell. So it is one particular
  // cell that loads a particular face for the first time and thus 
  //
  //
  #if PeanoDebug>0
  tarch::logging::Log _log( "Q" );
  logDebug( "merge()", "merge at x=" << marker.toString() << " with an outer normal of " << marker.outerNormal() ); 
  #endif
  
  const int faceNormal = marker.getSelectedFaceNumber() % Dimensions;
  dfore(i,5,faceNormal,0) {
    for (int j=0; j<1.0; j++) {
      tarch::la::Vector<Dimensions,int> volume = i;
      volume(faceNormal) += marker.outerNormal()(faceNormal)>0 ? j + 1.0 : j;
      
      int volumeSerialised = serialisePatchIndex(volume, faceNormal);
      for (int k=0; k<13; k++) {
        value[volumeSerialised*13+k] = neighbour.value[volumeSerialised*13+k];
      }
      
      // This should be non-critical assertion, but this assertion is only
      // generically available in ExaHyPE 2, so I comment the assertion out
      // here.
      // assertion(value[volumeSerialised]==value[volumeSerialised]);
    }
  }

}


bool examples::exahype2::loh1::facedata::LOH1Q::send(const peano4::datamanagement::FaceMarker& marker) {
  return observers::InstanceOfLOH1.getSolverState()==LOH1::SolverState::GridInitialisation;
}


bool examples::exahype2::loh1::facedata::LOH1Q::receiveAndMerge(const peano4::datamanagement::FaceMarker& marker) {
  return observers::InstanceOfLOH1.getSolverState()==LOH1::SolverState::PlottingInitialCondition or observers::InstanceOfLOH1.getSolverState()==LOH1::SolverState::PrimaryAfterGridInitialisation;
}


bool examples::exahype2::loh1::facedata::LOH1Q::storePersistently(const peano4::datamanagement::FaceMarker& marker) {
  return true;
}


bool examples::exahype2::loh1::facedata::LOH1Q::loadPersistently(const peano4::datamanagement::FaceMarker& marker) {
  return true;
}
#ifdef Parallel
void examples::exahype2::loh1::facedata::LOH1Q::initDatatype() {
  examples::exahype2::loh1::facedata::LOH1Q  instances[2];
 
  #if PeanoDebug>=1
    MPI_Datatype subtypes[] = { MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE };

    #if Dimensions==2
    int blocklen[] = { Dimensions, Dimensions, 130 };
    #else
    int blocklen[] = { Dimensions, Dimensions, 650 };
    #endif

    const int NumberOfAttributes = 3;
  #else   
    MPI_Datatype subtypes[] = { MPI_DOUBLE };
    
    #if Dimensions==2
    int blocklen[] = { 130 };
    #else
    int blocklen[] = { 650 };
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


void examples::exahype2::loh1::facedata::LOH1Q::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}
  

MPI_Datatype   examples::exahype2::loh1::facedata::LOH1Q::Datatype;
#endif