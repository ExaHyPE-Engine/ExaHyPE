#include "Particle.h"



#include <sstream>
#include <algorithm>



examples::particles::globaldata::Particle::Particle(tarch::la::Vector<Dimensions,double>  __debugX, tarch::la::Vector<Dimensions,double>  __debugH, tarch::la::Vector<Dimensions,double>  __x, MoveState  __MoveState, ParallelState  __ParallelState, NewParallelState  __NewParallelState, double  __cutOffRadius, tarch::la::Vector<Dimensions,double>  __x_prev, tarch::la::Vector<Dimensions,double>  __v, tarch::la::Vector<Dimensions,double>  __a, tarch::la::Vector<Dimensions,double>  __v_full, tarch::la::Vector<Dimensions,double>  __v_prev, tarch::la::Vector<Dimensions,double>  __a_prev_step, double  __mass, double  __density, double  __pressure, double  __u, double  __u_full, double  __uDot, double  __uDot_prev_step, double  __smoothingLength, int  __Ncount, double  __wcount, double  __wcount_dh, double  __soundSpeed, double  __Omega, double  __rho_dh, double  __hDot, double  __rot_v, double  __div_v, double  __v_sig_AV, double  __balsara, double  __time, double  __timeStepSize, double  __residual, int  __iterCount, bool  __hasBeenKicked, bool  __hasConverged, double  __smoothingLengthPrevious):
    _debugX(__debugX)
  , _debugH(__debugH)
  , _x(__x)
  , _MoveState(__MoveState)
  , _ParallelState(__ParallelState)
  , _NewParallelState(__NewParallelState)
  , _cutOffRadius(__cutOffRadius)
  , _x_prev(__x_prev)
  , _v(__v)
  , _a(__a)
  , _v_full(__v_full)
  , _v_prev(__v_prev)
  , _a_prev_step(__a_prev_step)
  , _mass(__mass)
  , _density(__density)
  , _pressure(__pressure)
  , _u(__u)
  , _u_full(__u_full)
  , _uDot(__uDot)
  , _uDot_prev_step(__uDot_prev_step)
  , _smoothingLength(__smoothingLength)
  , _Ncount(__Ncount)
  , _wcount(__wcount)
  , _wcount_dh(__wcount_dh)
  , _soundSpeed(__soundSpeed)
  , _Omega(__Omega)
  , _rho_dh(__rho_dh)
  , _hDot(__hDot)
  , _rot_v(__rot_v)
  , _div_v(__div_v)
  , _v_sig_AV(__v_sig_AV)
  , _balsara(__balsara)
  , _time(__time)
  , _timeStepSize(__timeStepSize)
  , _residual(__residual)
  , _iterCount(__iterCount)
  , _hasBeenKicked(__hasBeenKicked)
  , _hasConverged(__hasConverged)
  , _smoothingLengthPrevious(__smoothingLengthPrevious)
  {}



std::string examples::particles::globaldata::Particle::toString() const {
  std::ostringstream out;
  out << "(";
  out << "debugX=" << _debugX;
  out << ","; 
  out << "debugH=" << _debugH;
  out << ","; 
  out << "x=" << _x;
  out << ","; 
  out << "MoveState=" << (_MoveState==MoveState::New? "New" : "")  << (_MoveState==MoveState::NotMovedYet? "NotMovedYet" : "")  << (_MoveState==MoveState::Moved? "Moved" : "") ;
  out << ","; 
  out << "ParallelState=" << (_ParallelState==ParallelState::Local? "Local" : "")  << (_ParallelState==ParallelState::Halo? "Halo" : "")  << (_ParallelState==ParallelState::Virtual? "Virtual" : "") ;
  out << ","; 
  out << "NewParallelState=" << (_NewParallelState==NewParallelState::Local? "Local" : "")  << (_NewParallelState==NewParallelState::Halo? "Halo" : "")  << (_NewParallelState==NewParallelState::Virtual? "Virtual" : "") ;
  out << ","; 
  out << "cutOffRadius=" << _cutOffRadius;
  out << ","; 
  out << "x_prev=" << _x_prev;
  out << ","; 
  out << "v=" << _v;
  out << ","; 
  out << "a=" << _a;
  out << ","; 
  out << "v_full=" << _v_full;
  out << ","; 
  out << "v_prev=" << _v_prev;
  out << ","; 
  out << "a_prev_step=" << _a_prev_step;
  out << ","; 
  out << "mass=" << _mass;
  out << ","; 
  out << "density=" << _density;
  out << ","; 
  out << "pressure=" << _pressure;
  out << ","; 
  out << "u=" << _u;
  out << ","; 
  out << "u_full=" << _u_full;
  out << ","; 
  out << "uDot=" << _uDot;
  out << ","; 
  out << "uDot_prev_step=" << _uDot_prev_step;
  out << ","; 
  out << "smoothingLength=" << _smoothingLength;
  out << ","; 
  out << "Ncount=" << _Ncount;
  out << ","; 
  out << "wcount=" << _wcount;
  out << ","; 
  out << "wcount_dh=" << _wcount_dh;
  out << ","; 
  out << "soundSpeed=" << _soundSpeed;
  out << ","; 
  out << "Omega=" << _Omega;
  out << ","; 
  out << "rho_dh=" << _rho_dh;
  out << ","; 
  out << "hDot=" << _hDot;
  out << ","; 
  out << "rot_v=" << _rot_v;
  out << ","; 
  out << "div_v=" << _div_v;
  out << ","; 
  out << "v_sig_AV=" << _v_sig_AV;
  out << ","; 
  out << "balsara=" << _balsara;
  out << ","; 
  out << "time=" << _time;
  out << ","; 
  out << "timeStepSize=" << _timeStepSize;
  out << ","; 
  out << "residual=" << _residual;
  out << ","; 
  out << "iterCount=" << _iterCount;
  out << ","; 
  out << "hasBeenKicked=" << _hasBeenKicked;
  out << ","; 
  out << "hasConverged=" << _hasConverged;
  out << ","; 
  out << "smoothingLengthPrevious=" << _smoothingLengthPrevious;
  out << ")";
  return out.str();
}


tarch::la::Vector<Dimensions,double>   examples::particles::globaldata::Particle::getDebugX() const {
  return   _debugX;
}


void   examples::particles::globaldata::Particle::setDebugX(const tarch::la::Vector<Dimensions,double>& value) {
  _debugX = value;
}


double   examples::particles::globaldata::Particle::getDebugX(int index) const {
  return   _debugX(index);
}


void   examples::particles::globaldata::Particle::setDebugX(int index, double value) {
  _debugX(index) = value;
}


tarch::la::Vector<Dimensions,double>   examples::particles::globaldata::Particle::getDebugH() const {
  return   _debugH;
}


void   examples::particles::globaldata::Particle::setDebugH(const tarch::la::Vector<Dimensions,double>& value) {
  _debugH = value;
}


double   examples::particles::globaldata::Particle::getDebugH(int index) const {
  return   _debugH(index);
}


void   examples::particles::globaldata::Particle::setDebugH(int index, double value) {
  _debugH(index) = value;
}


tarch::la::Vector<Dimensions,double>   examples::particles::globaldata::Particle::getX() const {
  return   _x;
}


void   examples::particles::globaldata::Particle::setX(const tarch::la::Vector<Dimensions,double>& value) {
  _x = value;
}


double   examples::particles::globaldata::Particle::getX(int index) const {
  return   _x(index);
}


void   examples::particles::globaldata::Particle::setX(int index, double value) {
  _x(index) = value;
}


examples::particles::globaldata::Particle::MoveState   examples::particles::globaldata::Particle::getMoveState() const {
  return _MoveState;
}


void   examples::particles::globaldata::Particle::setMoveState(MoveState value) {
  _MoveState = value;
}


examples::particles::globaldata::Particle::ParallelState   examples::particles::globaldata::Particle::getParallelState() const {
  return _ParallelState;
}


void   examples::particles::globaldata::Particle::setParallelState(ParallelState value) {
  _ParallelState = value;
}


examples::particles::globaldata::Particle::NewParallelState   examples::particles::globaldata::Particle::getNewParallelState() const {
  return _NewParallelState;
}


void   examples::particles::globaldata::Particle::setNewParallelState(NewParallelState value) {
  _NewParallelState = value;
}


double   examples::particles::globaldata::Particle::getCutOffRadius() const {
  return _cutOffRadius;
}


void   examples::particles::globaldata::Particle::setCutOffRadius(double value) {
  _cutOffRadius = value;
}


tarch::la::Vector<Dimensions,double>   examples::particles::globaldata::Particle::getX_prev() const {
  return   _x_prev;
}


void   examples::particles::globaldata::Particle::setX_prev(const tarch::la::Vector<Dimensions,double>& value) {
  _x_prev = value;
}


double   examples::particles::globaldata::Particle::getX_prev(int index) const {
  return   _x_prev(index);
}


void   examples::particles::globaldata::Particle::setX_prev(int index, double value) {
  _x_prev(index) = value;
}


tarch::la::Vector<Dimensions,double>   examples::particles::globaldata::Particle::getV() const {
  return   _v;
}


void   examples::particles::globaldata::Particle::setV(const tarch::la::Vector<Dimensions,double>& value) {
  _v = value;
}


double   examples::particles::globaldata::Particle::getV(int index) const {
  return   _v(index);
}


void   examples::particles::globaldata::Particle::setV(int index, double value) {
  _v(index) = value;
}


tarch::la::Vector<Dimensions,double>   examples::particles::globaldata::Particle::getA() const {
  return   _a;
}


void   examples::particles::globaldata::Particle::setA(const tarch::la::Vector<Dimensions,double>& value) {
  _a = value;
}


double   examples::particles::globaldata::Particle::getA(int index) const {
  return   _a(index);
}


void   examples::particles::globaldata::Particle::setA(int index, double value) {
  _a(index) = value;
}


tarch::la::Vector<Dimensions,double>   examples::particles::globaldata::Particle::getV_full() const {
  return   _v_full;
}


void   examples::particles::globaldata::Particle::setV_full(const tarch::la::Vector<Dimensions,double>& value) {
  _v_full = value;
}


double   examples::particles::globaldata::Particle::getV_full(int index) const {
  return   _v_full(index);
}


void   examples::particles::globaldata::Particle::setV_full(int index, double value) {
  _v_full(index) = value;
}


tarch::la::Vector<Dimensions,double>   examples::particles::globaldata::Particle::getV_prev() const {
  return   _v_prev;
}


void   examples::particles::globaldata::Particle::setV_prev(const tarch::la::Vector<Dimensions,double>& value) {
  _v_prev = value;
}


double   examples::particles::globaldata::Particle::getV_prev(int index) const {
  return   _v_prev(index);
}


void   examples::particles::globaldata::Particle::setV_prev(int index, double value) {
  _v_prev(index) = value;
}


tarch::la::Vector<Dimensions,double>   examples::particles::globaldata::Particle::getA_prev_step() const {
  return   _a_prev_step;
}


void   examples::particles::globaldata::Particle::setA_prev_step(const tarch::la::Vector<Dimensions,double>& value) {
  _a_prev_step = value;
}


double   examples::particles::globaldata::Particle::getA_prev_step(int index) const {
  return   _a_prev_step(index);
}


void   examples::particles::globaldata::Particle::setA_prev_step(int index, double value) {
  _a_prev_step(index) = value;
}


double   examples::particles::globaldata::Particle::getMass() const {
  return _mass;
}


void   examples::particles::globaldata::Particle::setMass(double value) {
  _mass = value;
}


double   examples::particles::globaldata::Particle::getDensity() const {
  return _density;
}


void   examples::particles::globaldata::Particle::setDensity(double value) {
  _density = value;
}


double   examples::particles::globaldata::Particle::getPressure() const {
  return _pressure;
}


void   examples::particles::globaldata::Particle::setPressure(double value) {
  _pressure = value;
}


double   examples::particles::globaldata::Particle::getU() const {
  return _u;
}


void   examples::particles::globaldata::Particle::setU(double value) {
  _u = value;
}


double   examples::particles::globaldata::Particle::getU_full() const {
  return _u_full;
}


void   examples::particles::globaldata::Particle::setU_full(double value) {
  _u_full = value;
}


double   examples::particles::globaldata::Particle::getUDot() const {
  return _uDot;
}


void   examples::particles::globaldata::Particle::setUDot(double value) {
  _uDot = value;
}


double   examples::particles::globaldata::Particle::getUDot_prev_step() const {
  return _uDot_prev_step;
}


void   examples::particles::globaldata::Particle::setUDot_prev_step(double value) {
  _uDot_prev_step = value;
}


double   examples::particles::globaldata::Particle::getSmoothingLength() const {
  return _smoothingLength;
}


void   examples::particles::globaldata::Particle::setSmoothingLength(double value) {
  _smoothingLength = value;
}


int   examples::particles::globaldata::Particle::getNcount() const {
  return _Ncount;
}


void   examples::particles::globaldata::Particle::setNcount(int value) {
  _Ncount = value;
}


double   examples::particles::globaldata::Particle::getWcount() const {
  return _wcount;
}


void   examples::particles::globaldata::Particle::setWcount(double value) {
  _wcount = value;
}


double   examples::particles::globaldata::Particle::getWcount_dh() const {
  return _wcount_dh;
}


void   examples::particles::globaldata::Particle::setWcount_dh(double value) {
  _wcount_dh = value;
}


double   examples::particles::globaldata::Particle::getSoundSpeed() const {
  return _soundSpeed;
}


void   examples::particles::globaldata::Particle::setSoundSpeed(double value) {
  _soundSpeed = value;
}


double   examples::particles::globaldata::Particle::getOmega() const {
  return _Omega;
}


void   examples::particles::globaldata::Particle::setOmega(double value) {
  _Omega = value;
}


double   examples::particles::globaldata::Particle::getRho_dh() const {
  return _rho_dh;
}


void   examples::particles::globaldata::Particle::setRho_dh(double value) {
  _rho_dh = value;
}


double   examples::particles::globaldata::Particle::getHDot() const {
  return _hDot;
}


void   examples::particles::globaldata::Particle::setHDot(double value) {
  _hDot = value;
}


double   examples::particles::globaldata::Particle::getRot_v() const {
  return _rot_v;
}


void   examples::particles::globaldata::Particle::setRot_v(double value) {
  _rot_v = value;
}


double   examples::particles::globaldata::Particle::getDiv_v() const {
  return _div_v;
}


void   examples::particles::globaldata::Particle::setDiv_v(double value) {
  _div_v = value;
}


double   examples::particles::globaldata::Particle::getV_sig_AV() const {
  return _v_sig_AV;
}


void   examples::particles::globaldata::Particle::setV_sig_AV(double value) {
  _v_sig_AV = value;
}


double   examples::particles::globaldata::Particle::getBalsara() const {
  return _balsara;
}


void   examples::particles::globaldata::Particle::setBalsara(double value) {
  _balsara = value;
}


double   examples::particles::globaldata::Particle::getTime() const {
  return _time;
}


void   examples::particles::globaldata::Particle::setTime(double value) {
  _time = value;
}


double   examples::particles::globaldata::Particle::getTimeStepSize() const {
  return _timeStepSize;
}


void   examples::particles::globaldata::Particle::setTimeStepSize(double value) {
  _timeStepSize = value;
}


double   examples::particles::globaldata::Particle::getResidual() const {
  return _residual;
}


void   examples::particles::globaldata::Particle::setResidual(double value) {
  _residual = value;
}


int   examples::particles::globaldata::Particle::getIterCount() const {
  return _iterCount;
}


void   examples::particles::globaldata::Particle::setIterCount(int value) {
  _iterCount = value;
}


bool   examples::particles::globaldata::Particle::getHasBeenKicked() const {
  return _hasBeenKicked;
}


void   examples::particles::globaldata::Particle::setHasBeenKicked(bool value) {
  _hasBeenKicked = value;
}


bool   examples::particles::globaldata::Particle::getHasConverged() const {
  return _hasConverged;
}


void   examples::particles::globaldata::Particle::setHasConverged(bool value) {
  _hasConverged = value;
}


double   examples::particles::globaldata::Particle::getSmoothingLengthPrevious() const {
  return _smoothingLengthPrevious;
}


void   examples::particles::globaldata::Particle::setSmoothingLengthPrevious(double value) {
  _smoothingLengthPrevious = value;
}






#ifdef Parallel

MPI_Datatype examples::particles::globaldata::Particle::Datatype;

int examples::particles::globaldata::Particle::getSenderRank() const {
  return _senderDestinationRank;
}


void examples::particles::globaldata::Particle::initDatatype() {
  examples::particles::globaldata::Particle  instances[2];
    
  MPI_Datatype subtypes[] = { MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_INT, MPI_INT, MPI_INT, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_INT, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_INT, MPI_BYTE, MPI_BYTE, MPI_DOUBLE };
    
  int blocklen[] = { Dimensions, Dimensions, Dimensions, 1, 1, 1, 1, Dimensions, Dimensions, Dimensions, Dimensions, Dimensions, Dimensions, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

  const int NumberOfAttributes = 39;
    
  MPI_Aint  baseFirstInstance;
  MPI_Aint  baseSecondInstance;
  MPI_Get_address( &instances[0], &baseFirstInstance );
  MPI_Get_address( &instances[1], &baseSecondInstance );
  MPI_Aint  disp[ NumberOfAttributes ];
  int       currentAddress = 0;
  MPI_Get_address( &(instances[0]._debugX.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._debugH.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._x.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._MoveState), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._ParallelState), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._NewParallelState), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._cutOffRadius), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._x_prev.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._v.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._a.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._v_full.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._v_prev.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._a_prev_step.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._mass), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._density), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._pressure), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._u), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._u_full), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._uDot), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._uDot_prev_step), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._smoothingLength), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._Ncount), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._wcount), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._wcount_dh), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._soundSpeed), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._Omega), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._rho_dh), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._hDot), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._rot_v), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._div_v), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._v_sig_AV), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._balsara), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._time), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._timeStepSize), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._residual), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._iterCount), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._hasBeenKicked), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._hasConverged), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._smoothingLengthPrevious), &disp[currentAddress] );
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


void examples::particles::globaldata::Particle::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}


void examples::particles::globaldata::Particle::send(const examples::particles::globaldata::Particle& buffer, int destination, int tag, MPI_Comm communicator ) {
  MPI_Send( &buffer, 1, Datatype, destination, tag, communicator);
}


void examples::particles::globaldata::Particle::receive(examples::particles::globaldata::Particle& buffer, int source, int tag, MPI_Comm communicator ) {
  MPI_Status status;
  MPI_Recv( &buffer, 1, Datatype, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
}


void examples::particles::globaldata::Particle::send(const examples::particles::globaldata::Particle& buffer, int destination, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {
  MPI_Request sendRequestHandle; 
  int         flag = 0; 
  MPI_Isend( &buffer, 1, Datatype, destination, tag, communicator, &sendRequestHandle ); 
  MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE ); 
  while (!flag) { 
    waitFunctor();
    MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE ); 
  }
}


void examples::particles::globaldata::Particle::receive(examples::particles::globaldata::Particle& buffer, int source, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {   
  MPI_Status  status;
  MPI_Request receiveRequestHandle; 
  int         flag = 0; 
  MPI_Irecv( &buffer, 1, Datatype, source, tag, communicator, &receiveRequestHandle ); 
  MPI_Test( &receiveRequestHandle, &flag, &status ); 
  while (!flag) { 
    waitFunctor();
    MPI_Test( &receiveRequestHandle, &flag, &status ); 
  }
  buffer._senderDestinationRank = status.MPI_SOURCE;
}

#endif

#ifdef Parallel
void examples::particles::globaldata::Particle::sendAndPollDanglingMessages(const examples::particles::globaldata::Particle& message, int destination, int tag, MPI_Comm communicator ) {
  examples::particles::globaldata::Particle::send(
    message, destination, tag,
    [&]() {
      auto  timeOutWarning   = tarch::mpi::Rank::getInstance().getDeadlockWarningTimeStamp();
      auto timeOutShutdown  = tarch::mpi::Rank::getInstance().getDeadlockTimeOutTimeStamp();
      bool triggeredTimeoutWarning = false;
      if (
        tarch::mpi::Rank::getInstance().isTimeOutWarningEnabled() &&
        (std::chrono::system_clock::now()>timeOutWarning) &&
        (!triggeredTimeoutWarning)
      ) {
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "examples::particles::globaldata::Particle", "sendAndPollDanglingMessages()",destination, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "examples::particles::globaldata::Particle", "sendAndPollDanglingMessages()", destination, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}


void examples::particles::globaldata::Particle::receiveAndPollDanglingMessages(examples::particles::globaldata::Particle& message, int source, int tag, MPI_Comm communicator ) {
  examples::particles::globaldata::Particle::receive(
    message, source, tag,
    [&]() {
      auto timeOutWarning   = tarch::mpi::Rank::getInstance().getDeadlockWarningTimeStamp();
      auto timeOutShutdown  = tarch::mpi::Rank::getInstance().getDeadlockTimeOutTimeStamp();
      bool triggeredTimeoutWarning = false;
      if (
        tarch::mpi::Rank::getInstance().isTimeOutWarningEnabled() &&
        (std::chrono::system_clock::now()>timeOutWarning) &&
        (!triggeredTimeoutWarning)
      ) {
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "examples::particles::globaldata::Particle", "receiveAndPollDanglingMessages()", source, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "examples::particles::globaldata::Particle", "receiveAndPollDanglingMessages()", source, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}
#endif
    