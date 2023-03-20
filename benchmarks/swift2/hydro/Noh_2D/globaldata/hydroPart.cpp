#include "hydroPart.h"



#include <sstream>
#include <algorithm>



benchmarks::swift2::noh::globaldata::hydroPart::hydroPart(tarch::la::Vector<Dimensions,double>  __debugX, tarch::la::Vector<Dimensions,double>  __debugH, tarch::la::Vector<Dimensions,double>  __x, double  __cutOffRadius, ParallelState  __ParallelState, NewParallelState  __NewParallelState, MoveState  __MoveState, bool  __CellHasUpdatedParticle, double  __mass, tarch::la::Vector<Dimensions,double>  __v, tarch::la::Vector<Dimensions,double>  __a, double  __density, double  __pressure, double  __smoothingLength, double  __u, double  __uDot, double  __f, double  __wcount_dh, double  __rho_dh, double  __hLeft, double  __hRight, double  __Omega, double  __smoothingLengthPrevious, double  __wcount, double  __hDot, double  __residual, int  __iterCount, bool  __hasConverged, double  __balsara, double  __rot_v, double  __div_v, double  __v_sig_AV, double  __soundSpeed, tarch::la::Vector<Dimensions,double>  __v_full, double  __u_full, bool  __hasNoNeighbours, bool  __isBoundaryParticle, int  __Ncount, int  __partid){
#if PeanoDebug>0
setDebugX( __debugX);
#endif 
#if PeanoDebug>0
setDebugH( __debugH);
#endif 
setX( __x);
setCutOffRadius( __cutOffRadius);
setParallelState( __ParallelState);
setNewParallelState( __NewParallelState);
setMoveState( __MoveState);
setCellHasUpdatedParticle( __CellHasUpdatedParticle);
setMass( __mass);
setV( __v);
setA( __a);
setDensity( __density);
setPressure( __pressure);
setSmoothingLength( __smoothingLength);
setU( __u);
setUDot( __uDot);
setF( __f);
setWcount_dh( __wcount_dh);
setRho_dh( __rho_dh);
setHLeft( __hLeft);
setHRight( __hRight);
setOmega( __Omega);
setSmoothingLengthPrevious( __smoothingLengthPrevious);
setWcount( __wcount);
setHDot( __hDot);
setResidual( __residual);
setIterCount( __iterCount);
setHasConverged( __hasConverged);
setBalsara( __balsara);
setRot_v( __rot_v);
setDiv_v( __div_v);
setV_sig_AV( __v_sig_AV);
setSoundSpeed( __soundSpeed);
setV_full( __v_full);
setU_full( __u_full);
setHasNoNeighbours( __hasNoNeighbours);
setIsBoundaryParticle( __isBoundaryParticle);
setNcount( __Ncount);
setPartid( __partid);
}



benchmarks::swift2::noh::globaldata::hydroPart::hydroPart( const hydroPart& copy ) {
#if PeanoDebug>0
  setDebugX( copy.getDebugX() );
#endif 
#if PeanoDebug>0
  setDebugH( copy.getDebugH() );
#endif 
  setX( copy.getX() );
  setCutOffRadius( copy.getCutOffRadius() );
  setParallelState( copy.getParallelState() );
  setNewParallelState( copy.getNewParallelState() );
  setMoveState( copy.getMoveState() );
  setCellHasUpdatedParticle( copy.getCellHasUpdatedParticle() );
  setMass( copy.getMass() );
  setV( copy.getV() );
  setA( copy.getA() );
  setDensity( copy.getDensity() );
  setPressure( copy.getPressure() );
  setSmoothingLength( copy.getSmoothingLength() );
  setU( copy.getU() );
  setUDot( copy.getUDot() );
  setF( copy.getF() );
  setWcount_dh( copy.getWcount_dh() );
  setRho_dh( copy.getRho_dh() );
  setHLeft( copy.getHLeft() );
  setHRight( copy.getHRight() );
  setOmega( copy.getOmega() );
  setSmoothingLengthPrevious( copy.getSmoothingLengthPrevious() );
  setWcount( copy.getWcount() );
  setHDot( copy.getHDot() );
  setResidual( copy.getResidual() );
  setIterCount( copy.getIterCount() );
  setHasConverged( copy.getHasConverged() );
  setBalsara( copy.getBalsara() );
  setRot_v( copy.getRot_v() );
  setDiv_v( copy.getDiv_v() );
  setV_sig_AV( copy.getV_sig_AV() );
  setSoundSpeed( copy.getSoundSpeed() );
  setV_full( copy.getV_full() );
  setU_full( copy.getU_full() );
  setHasNoNeighbours( copy.getHasNoNeighbours() );
  setIsBoundaryParticle( copy.getIsBoundaryParticle() );
  setNcount( copy.getNcount() );
  setPartid( copy.getPartid() );
}





std::string benchmarks::swift2::noh::globaldata::hydroPart::toString() const {
  std::ostringstream out;
  out << "(";
#if PeanoDebug>0
  out << "debugX=" << _debugX;
#endif 
#if PeanoDebug>0
  out << ","; 
  out << "debugH=" << _debugH;
#endif 
  out << ","; 
  out << "x=" << _x;
  out << ","; 
  out << "cutOffRadius=" << _cutOffRadius;
  out << ","; 
  out << "ParallelState=" << (_ParallelState==ParallelState::Local? "Local" : "")  << (_ParallelState==ParallelState::Virtual? "Virtual" : "") ;
  out << ","; 
  out << "NewParallelState=" << (_NewParallelState==NewParallelState::Local? "Local" : "")  << (_NewParallelState==NewParallelState::Virtual? "Virtual" : "") ;
  out << ","; 
  out << "MoveState=" << (_MoveState==MoveState::New? "New" : "")  << (_MoveState==MoveState::NotMovedYet? "NotMovedYet" : "")  << (_MoveState==MoveState::Moved? "Moved" : "") ;
  out << ","; 
  out << "CellHasUpdatedParticle=" << _CellHasUpdatedParticle;
  out << ","; 
  out << "mass=" << _mass;
  out << ","; 
  out << "v=" << getV();
  out << ","; 
  out << "a=" << getA();
  out << ","; 
  out << "density=" << _density;
  out << ","; 
  out << "pressure=" << _pressure;
  out << ","; 
  out << "smoothingLength=" << _smoothingLength;
  out << ","; 
  out << "u=" << _u;
  out << ","; 
  out << "uDot=" << _uDot;
  out << ","; 
  out << "f=" << _f;
  out << ","; 
  out << "wcount_dh=" << _wcount_dh;
  out << ","; 
  out << "rho_dh=" << _rho_dh;
  out << ","; 
  out << "hLeft=" << _hLeft;
  out << ","; 
  out << "hRight=" << _hRight;
  out << ","; 
  out << "Omega=" << _Omega;
  out << ","; 
  out << "smoothingLengthPrevious=" << _smoothingLengthPrevious;
  out << ","; 
  out << "wcount=" << _wcount;
  out << ","; 
  out << "hDot=" << _hDot;
  out << ","; 
  out << "residual=" << _residual;
  out << ","; 
  out << "iterCount=" << _iterCount;
  out << ","; 
  out << "hasConverged=" << _hasConverged;
  out << ","; 
  out << "balsara=" << _balsara;
  out << ","; 
  out << "rot_v=" << _rot_v;
  out << ","; 
  out << "div_v=" << _div_v;
  out << ","; 
  out << "v_sig_AV=" << _v_sig_AV;
  out << ","; 
  out << "soundSpeed=" << _soundSpeed;
  out << ","; 
  out << "v_full=" << getV_full();
  out << ","; 
  out << "u_full=" << _u_full;
  out << ","; 
  out << "hasNoNeighbours=" << _hasNoNeighbours;
  out << ","; 
  out << "isBoundaryParticle=" << _isBoundaryParticle;
  out << ","; 
  out << "Ncount=" << _Ncount;
  out << ","; 
  out << "partid=" << _partid;
  out << ")";
  return out.str();
}





#if PeanoDebug>0
tarch::la::Vector<Dimensions,double>   benchmarks::swift2::noh::globaldata::hydroPart::getDebugX() const {
  return   _debugX;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setDebugX(const tarch::la::Vector<Dimensions,double>& value) {
  _debugX = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getDebugX(int index) const {
  return   _debugX(index);
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setDebugX(int index, double value) {
  _debugX(index) = value;
}


#endif 


#if PeanoDebug>0
tarch::la::Vector<Dimensions,double>   benchmarks::swift2::noh::globaldata::hydroPart::getDebugH() const {
  return   _debugH;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setDebugH(const tarch::la::Vector<Dimensions,double>& value) {
  _debugH = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getDebugH(int index) const {
  return   _debugH(index);
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setDebugH(int index, double value) {
  _debugH(index) = value;
}


#endif 


tarch::la::Vector<Dimensions,double>   benchmarks::swift2::noh::globaldata::hydroPart::getX() const {
  return   _x;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setX(const tarch::la::Vector<Dimensions,double>& value) {
  _x = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getX(int index) const {
  return   _x(index);
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setX(int index, double value) {
  _x(index) = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getCutOffRadius() const {
  return _cutOffRadius;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setCutOffRadius(double value) {
  _cutOffRadius = value;
}


benchmarks::swift2::noh::globaldata::hydroPart::ParallelState   benchmarks::swift2::noh::globaldata::hydroPart::getParallelState() const {
  return _ParallelState;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setParallelState(ParallelState value) {
  _ParallelState = value;
}


benchmarks::swift2::noh::globaldata::hydroPart::NewParallelState   benchmarks::swift2::noh::globaldata::hydroPart::getNewParallelState() const {
  return _NewParallelState;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setNewParallelState(NewParallelState value) {
  _NewParallelState = value;
}


benchmarks::swift2::noh::globaldata::hydroPart::MoveState   benchmarks::swift2::noh::globaldata::hydroPart::getMoveState() const {
  return _MoveState;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setMoveState(MoveState value) {
  _MoveState = value;
}


bool   benchmarks::swift2::noh::globaldata::hydroPart::getCellHasUpdatedParticle() const {
  return _CellHasUpdatedParticle;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setCellHasUpdatedParticle(bool value) {
  _CellHasUpdatedParticle = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getMass() const {
  return _mass;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setMass(double value) {
  _mass = value;
}


tarch::la::Vector<Dimensions,double>   benchmarks::swift2::noh::globaldata::hydroPart::getV() const {

  tarch::la::Vector<Dimensions,double> result;
  for( int i=0; i<Dimensions; i++) {
    result(i) =   _v[i];
  }
  return result;
      }


void   benchmarks::swift2::noh::globaldata::hydroPart::setV(const tarch::la::Vector<Dimensions,double>& value) {

  for( int i=0; i<Dimensions; i++) {
      _v[i] = value(i);
  }
      }


double   benchmarks::swift2::noh::globaldata::hydroPart::getV(int index) const {
  return   _v[index];
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setV(int index, double value) {
  _v[index] = value;
}


tarch::la::Vector<Dimensions,double>   benchmarks::swift2::noh::globaldata::hydroPart::getA() const {

  tarch::la::Vector<Dimensions,double> result;
  for( int i=0; i<Dimensions; i++) {
    result(i) =   _a[i];
  }
  return result;
      }


void   benchmarks::swift2::noh::globaldata::hydroPart::setA(const tarch::la::Vector<Dimensions,double>& value) {

  for( int i=0; i<Dimensions; i++) {
      _a[i] = value(i);
  }
      }


double   benchmarks::swift2::noh::globaldata::hydroPart::getA(int index) const {
  return   _a[index];
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setA(int index, double value) {
  _a[index] = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getDensity() const {
  return _density;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setDensity(double value) {
  _density = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getPressure() const {
  return _pressure;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setPressure(double value) {
  _pressure = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getSmoothingLength() const {
  return _smoothingLength;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setSmoothingLength(double value) {
  _smoothingLength = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getU() const {
  return _u;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setU(double value) {
  _u = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getUDot() const {
  return _uDot;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setUDot(double value) {
  _uDot = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getF() const {
  return _f;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setF(double value) {
  _f = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getWcount_dh() const {
  return _wcount_dh;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setWcount_dh(double value) {
  _wcount_dh = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getRho_dh() const {
  return _rho_dh;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setRho_dh(double value) {
  _rho_dh = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getHLeft() const {
  return _hLeft;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setHLeft(double value) {
  _hLeft = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getHRight() const {
  return _hRight;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setHRight(double value) {
  _hRight = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getOmega() const {
  return _Omega;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setOmega(double value) {
  _Omega = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getSmoothingLengthPrevious() const {
  return _smoothingLengthPrevious;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setSmoothingLengthPrevious(double value) {
  _smoothingLengthPrevious = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getWcount() const {
  return _wcount;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setWcount(double value) {
  _wcount = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getHDot() const {
  return _hDot;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setHDot(double value) {
  _hDot = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getResidual() const {
  return _residual;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setResidual(double value) {
  _residual = value;
}


int   benchmarks::swift2::noh::globaldata::hydroPart::getIterCount() const {
  return _iterCount;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setIterCount(int value) {
  _iterCount = value;
}


bool   benchmarks::swift2::noh::globaldata::hydroPart::getHasConverged() const {
  return _hasConverged;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setHasConverged(bool value) {
  _hasConverged = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getBalsara() const {
  return _balsara;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setBalsara(double value) {
  _balsara = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getRot_v() const {
  return _rot_v;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setRot_v(double value) {
  _rot_v = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getDiv_v() const {
  return _div_v;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setDiv_v(double value) {
  _div_v = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getV_sig_AV() const {
  return _v_sig_AV;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setV_sig_AV(double value) {
  _v_sig_AV = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getSoundSpeed() const {
  return _soundSpeed;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setSoundSpeed(double value) {
  _soundSpeed = value;
}


tarch::la::Vector<Dimensions,double>   benchmarks::swift2::noh::globaldata::hydroPart::getV_full() const {

  tarch::la::Vector<Dimensions,double> result;
  for( int i=0; i<Dimensions; i++) {
    result(i) =   _v_full[i];
  }
  return result;
      }


void   benchmarks::swift2::noh::globaldata::hydroPart::setV_full(const tarch::la::Vector<Dimensions,double>& value) {

  for( int i=0; i<Dimensions; i++) {
      _v_full[i] = value(i);
  }
      }


double   benchmarks::swift2::noh::globaldata::hydroPart::getV_full(int index) const {
  return   _v_full[index];
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setV_full(int index, double value) {
  _v_full[index] = value;
}


double   benchmarks::swift2::noh::globaldata::hydroPart::getU_full() const {
  return _u_full;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setU_full(double value) {
  _u_full = value;
}


bool   benchmarks::swift2::noh::globaldata::hydroPart::getHasNoNeighbours() const {
  return _hasNoNeighbours;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setHasNoNeighbours(bool value) {
  _hasNoNeighbours = value;
}


bool   benchmarks::swift2::noh::globaldata::hydroPart::getIsBoundaryParticle() const {
  return _isBoundaryParticle;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setIsBoundaryParticle(bool value) {
  _isBoundaryParticle = value;
}


int   benchmarks::swift2::noh::globaldata::hydroPart::getNcount() const {
  return _Ncount;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setNcount(int value) {
  _Ncount = value;
}


int   benchmarks::swift2::noh::globaldata::hydroPart::getPartid() const {
  return _partid;
}


void   benchmarks::swift2::noh::globaldata::hydroPart::setPartid(int value) {
  _partid = value;
}






#ifdef Parallel

#if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)      
MPI_Datatype benchmarks::swift2::noh::globaldata::hydroPart::Datatype;
#endif


[[clang::map_mpi_datatype]]
MPI_Datatype benchmarks::swift2::noh::globaldata::hydroPart::getForkDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype benchmarks::swift2::noh::globaldata::hydroPart::getGlobalCommunciationDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype benchmarks::swift2::noh::globaldata::hydroPart::getJoinDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype benchmarks::swift2::noh::globaldata::hydroPart::getBoundaryExchangeDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype benchmarks::swift2::noh::globaldata::hydroPart::getMultiscaleDataExchangeDatatype() {
  return Datatype;
}


int benchmarks::swift2::noh::globaldata::hydroPart::getSenderRank() const {
  return _senderDestinationRank;
}


void benchmarks::swift2::noh::globaldata::hydroPart::initDatatype() {
  #if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
  benchmarks::swift2::noh::globaldata::hydroPart  instances[2];

  int NumberOfAttributes = 0;
#if PeanoDebug>0
  NumberOfAttributes++;
#endif 
#if PeanoDebug>0
  NumberOfAttributes++;
#endif 
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;

  MPI_Datatype* subtypes = new MPI_Datatype[NumberOfAttributes];
  int*          blocklen = new int[NumberOfAttributes]; 
  MPI_Aint*     disp     = new MPI_Aint[NumberOfAttributes];
  
  int counter            = 0;
#if PeanoDebug>0
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = Dimensions;
  counter++;
#endif
#if PeanoDebug>0
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = Dimensions;
  counter++;
#endif
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = Dimensions;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_BYTE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = Dimensions;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = Dimensions;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_BYTE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = Dimensions;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_BYTE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_BYTE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  counter++;
   
  MPI_Aint  baseFirstInstance;
  MPI_Aint  baseSecondInstance;
  MPI_Get_address( &instances[0], &baseFirstInstance );
  MPI_Get_address( &instances[1], &baseSecondInstance );
  
  counter = 0;

#if PeanoDebug>0
  MPI_Get_address( &(instances[0]._debugX.data()[0]), &disp[counter] );
  counter++;
#endif

#if PeanoDebug>0
  MPI_Get_address( &(instances[0]._debugH.data()[0]), &disp[counter] );
  counter++;
#endif
  MPI_Get_address( &(instances[0]._x.data()[0]), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._cutOffRadius), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._ParallelState), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._NewParallelState), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._MoveState), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._CellHasUpdatedParticle), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._mass), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._v.data()[0]), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._a.data()[0]), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._density), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._pressure), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._smoothingLength), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._u), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._uDot), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._f), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._wcount_dh), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._rho_dh), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._hLeft), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._hRight), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._Omega), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._smoothingLengthPrevious), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._wcount), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._hDot), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._residual), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._iterCount), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._hasConverged), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._balsara), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._rot_v), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._div_v), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._v_sig_AV), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._soundSpeed), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._v_full.data()[0]), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._u_full), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._hasNoNeighbours), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._isBoundaryParticle), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._Ncount), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._partid), &disp[counter] );
  counter++;

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

  delete[] subtypes;
  delete[] blocklen; 
  delete[] disp;

  #else
  // invoke routine once to trigger lazy initialisation
  getForkDatatype();
  getJoinDatatype();
  getBoundaryExchangeDatatype();
  getMultiscaleDataExchangeDatatype();
  getGlobalCommunciationDatatype();
  #endif
}


void benchmarks::swift2::noh::globaldata::hydroPart::shutdownDatatype() {
  #if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
  MPI_Datatype type;
  type = getForkDatatype();
  MPI_Type_free( &type );
  type = getJoinDatatype();
  MPI_Type_free( &type );
  type = getBoundaryExchangeDatatype();
  MPI_Type_free( &type );
  type = getMultiscaleDataExchangeDatatype();
  MPI_Type_free( &type );
  type = getGlobalCommunciationDatatype();
  MPI_Type_free( &type );
  #else
  MPI_Datatype type = Datatype;
  MPI_Type_free( &type );
  #endif
}


void benchmarks::swift2::noh::globaldata::hydroPart::send(const benchmarks::swift2::noh::globaldata::hydroPart& buffer, int destination, int tag, MPI_Comm communicator ) {
  MPI_Send( &buffer, 1, Datatype, destination, tag, communicator);
}


void benchmarks::swift2::noh::globaldata::hydroPart::receive(benchmarks::swift2::noh::globaldata::hydroPart& buffer, int source, int tag, MPI_Comm communicator ) {
  MPI_Status status;
  MPI_Recv( &buffer, 1, Datatype, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
}


void benchmarks::swift2::noh::globaldata::hydroPart::send(
  const benchmarks::swift2::noh::globaldata::hydroPart& buffer, 
  int destination, 
  int tag, 
  std::function<void()> startCommunicationFunctor, 
  std::function<void()> waitFunctor, 
  MPI_Comm communicator 
) {
  MPI_Request sendRequestHandle; 
  int         flag = 0; 
  MPI_Isend( &buffer, 1, Datatype, destination, tag, communicator, &sendRequestHandle ); 
  MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE );
  startCommunicationFunctor(); 
  while (!flag) { 
    waitFunctor();
    MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE ); 
  }
}


void benchmarks::swift2::noh::globaldata::hydroPart::receive(
  benchmarks::swift2::noh::globaldata::hydroPart& buffer, 
  int source, 
  int tag, 
  std::function<void()> startCommunicationFunctor, 
  std::function<void()> waitFunctor, 
  MPI_Comm communicator 
) {   
  MPI_Status  status;
  MPI_Request receiveRequestHandle; 
  int         flag = 0; 
  MPI_Irecv( &buffer, 1, Datatype, source, tag, communicator, &receiveRequestHandle ); 
  MPI_Test( &receiveRequestHandle, &flag, &status );
  startCommunicationFunctor(); 
  while (!flag) { 
    waitFunctor();
    MPI_Test( &receiveRequestHandle, &flag, &status ); 
  }
  buffer._senderDestinationRank = status.MPI_SOURCE;
}
#endif

#ifdef Parallel
void benchmarks::swift2::noh::globaldata::hydroPart::sendAndPollDanglingMessages(const benchmarks::swift2::noh::globaldata::hydroPart& message, int destination, int tag, MPI_Comm communicator ) {
  benchmarks::swift2::noh::globaldata::hydroPart::send(
    message, destination, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "benchmarks::swift2::noh::globaldata::hydroPart", "sendAndPollDanglingMessages()",destination, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "benchmarks::swift2::noh::globaldata::hydroPart", "sendAndPollDanglingMessages()", destination, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}


void benchmarks::swift2::noh::globaldata::hydroPart::receiveAndPollDanglingMessages(benchmarks::swift2::noh::globaldata::hydroPart& message, int source, int tag, MPI_Comm communicator ) {
  benchmarks::swift2::noh::globaldata::hydroPart::receive(
    message, source, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "benchmarks::swift2::noh::globaldata::hydroPart", "receiveAndPollDanglingMessages()", source, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "benchmarks::swift2::noh::globaldata::hydroPart", "receiveAndPollDanglingMessages()", source, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}
#endif
    
::swift2::ParticleSpecies& benchmarks::swift2::noh::globaldata::hydroPart::getSpecies() {
  static ::swift2::ParticleSpecies species;
  return species;
}
