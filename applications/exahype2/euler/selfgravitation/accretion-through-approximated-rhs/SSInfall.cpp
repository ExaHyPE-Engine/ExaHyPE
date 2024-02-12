#include "../../selfgravitation/accretion-through-approximated-rhs/SSInfall.h"

#include "Constants.h"
#include "exahype2/RefinementControl.h"

#include "tarch/multicore/Lock.h"
#include "tarch/NonCriticalAssertions.h"


tarch::logging::Log   applications::exahype2::euler::sphericalaccretion::SSInfall::_log( "applications::exahype2::euler::sphericalaccretion::SSInfall" );


const tarch::la::Vector<Dimensions,double>  applications::exahype2::euler::sphericalaccretion::SSInfall::OverDensityCentre = {0.0, 0.0, 0.0};


#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
double applications::exahype2::euler::sphericalaccretion::SSInfall::_rMax;
double applications::exahype2::euler::sphericalaccretion::SSInfall::_rhoMax;
double applications::exahype2::euler::sphericalaccretion::SSInfall::_mTotMax;
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif


void applications::exahype2::euler::sphericalaccretion::SSInfall::startTimeStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
){
  AbstractSSInfall::startTimeStep(globalMinTimeStamp, globalMaxTimeStamp, globalMinTimeStepSize, globalMaxTimeStepSize);
  constexpr double pi = M_PI;
  if (AbstractSSInfall::isFirstGridSweepOfTimeStep()){
    for (int i=0;i<sample_number;i++) {
      m_tot_copy[i]      = global_m_tot[i];
      global_m_tot[i]    = 0;
      global_cell_tot[i] = 0;
      m_tot[i]           = 0;
      cell_tot[i]        = 0;
    }
    double inter_time=_minTimeStamp+getAdmissibleTimeStepSize()/2.0;
    #ifdef useTable
    int time_position=0;
    //logInfo( "startTImeStep()", "current min time=" << globalMinTimeStamp<<" current min timestep=" << globalMinTimeStepSize<<" inter time=" << inter_time);
    //logInfo( "startTImeStep()", "current max time=" << globalMaxTimeStamp<<" current max timestep=" << globalMaxTimeStepSize);
    //logInfo( "startTImeStep()", "current time=" << _minTimeStamp<<" current timestep=" << getAdmissibleTimeStepSize());

    if (tarch::la::equals(inter_time,0.0) ){
      interpolated_a=scale_factor_points[0];
      //logInfo( "startTImeStep()", "reset scale a to " << interpolated_a << " because t=0.0");
    }else{
      for(int i=0; i<table_point_number;i++){
        if (tarch::la::smallerEquals(inter_time,code_time_points[i]) ){time_position=i; break;}
        time_position=i;
      }
      //logInfo( "startTImeStep()", inter_time << " is between time point " << code_time_points[time_position-1] << " and " << code_time_points[time_position]);
      interpolated_a=scale_factor_points[time_position-1]*(code_time_points[time_position]-inter_time)/(code_time_points[time_position]-code_time_points[time_position-1])+scale_factor_points[time_position]*(inter_time-code_time_points[time_position-1])/(code_time_points[time_position]-code_time_points[time_position-1]);
      //logInfo( "startTImeStep()", "interpolated to " << interpolated_a << " from " << scale_factor_points[time_position-1] << " and " << scale_factor_points[time_position]);
    }
    #endif

    if (MassCal==1){
      for(int i=0;i<sample_number;i++){
        m_tot_copy[i]=std::max(0.0,(4/3)*pi*pow(r_s[0],3)*((rho_0+rho_x[0])/2-1));
      }
      for(int i=1;i<sample_number;i++){
        double m_layer=(4/3)*pi*(pow(r_s[i],3)-pow(r_s[i-1],3))*((rho_x[i]+rho_x[i-1])/2-1);
        for(int j=i;j<sample_number;j++){
          m_tot_copy[j]+=std::max(0.0,m_layer);
        }
      }
    }
  }
}


void applications::exahype2::euler::sphericalaccretion::SSInfall::finishTimeStep(){
  AbstractSSInfall::finishTimeStep();

  if (AbstractSSInfall::isLastGridSweepOfTimeStep()){
    #ifdef Parallel
    tarch::mpi::Rank::getInstance().allReduce(
        m_tot,
        global_m_tot,
        sample_number, MPI_DOUBLE,
        MPI_SUM,
        [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
    );
    tarch::mpi::Rank::getInstance().allReduce(
        cell_tot,
        global_cell_tot,
        sample_number, MPI_DOUBLE,
        MPI_SUM,
        [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
    );
    #else
    for (int i=0;i<sample_number;i++) {
      global_m_tot[i]=m_tot[i];
      global_cell_tot[i]=cell_tot[i];
    }

    // @Han is this ok?
    _rMax     = r_s[sample_number-1];
    _rhoMax   = rho_x[sample_number-1];
    _mTotMax  = m_tot_copy[sample_number-1];
    #endif
    plotGlobalMTot(false);
  }
}


std::string applications::exahype2::euler::sphericalaccretion::SSInfall::plotRs() const {
  std::ostringstream msg;
  msg << "r_s=(";
  for (int i=0;i<sample_number;i++) {
    if (i!=0) msg << ",";
    msg << r_s[i];
  }
  msg << ")";
  return msg.str();
}


std::string applications::exahype2::euler::sphericalaccretion::SSInfall::plotGlobalMTot(bool plotCopy) const {
  std::ostringstream msg;
  if (plotCopy) {
    msg << "m_tot_copy=(";
    for (int i=0;i<sample_number;i++) {
      if (i!=0) msg << ",";
      msg << m_tot_copy[i];
    }
    msg << ")";
  }
  else {
    msg << "global_m_tot=(";
    for (int i=0;i<sample_number;i++) {
      if (i!=0) msg << ",";
      msg << global_m_tot[i];
    }
    msg << ")";
  }
  return msg.str();
}


::exahype2::RefinementCommand applications::exahype2::euler::sphericalaccretion::SSInfall::refinementCriterion(
  const double * __restrict__ Q, // Q[5+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {

  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;
  double radius=volumeX(0)*volumeX(0)+volumeX(1)*volumeX(1)+volumeX(2)*volumeX(2); radius=pow(radius,0.5);
  if (ReSwi==1){ //radius based
    constexpr int NumberOfRefinementLayers = 2;
    double Radius[NumberOfRefinementLayers] = {0.7,0.5};
    double MaxH[NumberOfRefinementLayers]   = {0.02,0.006};
    //constexpr int NumberOfRefinementLayers = 1;
    //double Radius[NumberOfRefinementLayers] = {0.5};
    //double MaxH[NumberOfRefinementLayers]   = {0.02};
    for (int i=0; i<NumberOfRefinementLayers; i++) {
      if (radius<Radius[i] and tarch::la::max(volumeH)>MaxH[i]) {
        result=::exahype2::RefinementCommand::Refine;
      }
    }
  }
  if (ReSwi==2){ //radius based
    constexpr int NumberOfRefinementLayers = 5;
    double Radius[NumberOfRefinementLayers] = {1.5,0.5,0.3,0.1,0.05};
    double MaxH[NumberOfRefinementLayers]   = {0.05,0.02,0.006,0.002,0.0007};
    for (int i=0; i<NumberOfRefinementLayers; i++) {
      if (radius<Radius[i] and tarch::la::max(volumeH)>MaxH[i]) {
        result=::exahype2::RefinementCommand::Refine;
      }
    }
  }
  if (ReSwi==3){ //radius based, for test
    if (radius<0.1) {result=::exahype2::RefinementCommand::Refine;}
  }
  if (ReSwi==4){ //dynamic one
    double t_i=pow(a_i,1.5)*2.057*1e17;
    double t_used=t;
    double Mpc=3.086*1e19; //in Km;
    double t_real=pow( (pow((Mpc/150),(-1.0/3.0))*pow(a_i,(-0.5))-t_used*pow(150,(4.0/3.0))/300/pow(Mpc,(1.0/3.0))),-3);
    double a_scale=pow((150/Mpc),(2.0/3.0))*pow( (pow((Mpc/150),(-1.0/3.0))*pow(a_i,(-0.5))-t_used*pow(150,(4.0/3.0))/300/pow(Mpc,(1.0/3.0))),-2);
    double R_ta_code=(a_i/a_scale)*pow((3.0*delta_m)/(4.0*M_PI*tilde_rho_ini),(1.0/3.0))*pow((4.0/3.0/M_PI),(8.0/9.0))*pow((t_real/t_i),(8.0/9.0));
    double delta_R=std::abs(radius-R_ta_code*0.55);
    //double delta_R=std::abs(radius-0.0875);

    constexpr int NumberOfRefinementLayers = 3;
    double distance[NumberOfRefinementLayers+1] = {1.5,0.6,0.3,0};
    double MaxH[NumberOfRefinementLayers+1]   = {0.05,0.018,0.006,0};

    /*if (delta_R>distance[0]) {result=::exahype2::RefinementCommand::Erase;}
    else if (delta_R<distance[NumberOfRefinementLayers-1]) {result=::exahype2::RefinementCommand::Refine;}
    else{
      for (int i=0; i<(NumberOfRefinementLayers-1); i++) {
        if (delta_R<distance[i] and delta_R>distance[i+1]) {
          if (tarch::la::max(volumeH)>MaxH[i])   {result=::exahype2::RefinementCommand::Refine;}
          if (tarch::la::max(volumeH)<MaxH[i+1]) {result=::exahype2::RefinementCommand::Erase;}
        }
      }
    }*/
    for (int i=0; i<(NumberOfRefinementLayers); i++) {
      if (delta_R<distance[i] and delta_R>distance[i+1]) {
        if (tarch::la::max(volumeH)>MaxH[i])   {result=::exahype2::RefinementCommand::Refine;}
        //if (tarch::la::max(volumeH)<MaxH[i+1]) {result=::exahype2::RefinementCommand::Erase;}
      }
    }
  }  
  if (ReSwi==5){ //erase test
    double delta_R=radius;

    constexpr int NumberOfRefinementLayers = 1;
    double distance[NumberOfRefinementLayers] = {0.1};
    double MaxH[NumberOfRefinementLayers]   = {0.01};

    if (delta_R>distance[0]) {result=::exahype2::RefinementCommand::Erase;}
    else if (delta_R<distance[NumberOfRefinementLayers-1]) {result=::exahype2::RefinementCommand::Refine;}
    else{
      for (int i=0; i<(NumberOfRefinementLayers-1); i++) {
        if (delta_R<distance[i] and delta_R>distance[i+1]) {
          if (tarch::la::max(volumeH)>MaxH[i])   {result=::exahype2::RefinementCommand::Refine;}
          if (tarch::la::max(volumeH)<MaxH[i+1]) {result=::exahype2::RefinementCommand::Erase;}
        }
      }
    }
  }
  return result;
}



void applications::exahype2::euler::sphericalaccretion::SSInfall::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructed
) {
  logDebug( "initialCondition(...)", "init volume at " << volumeCentre << "x" << volumeH << " (grid constructed=" << gridIsConstructed << ")" );

  constexpr double pi = M_PI;
  double x=volumeCentre(0)-OverDensityCentre(0);
  double y=volumeCentre(1)-OverDensityCentre(1);
  double z=volumeCentre(2)-OverDensityCentre(2);

  bool isInTheSphere = ( (x*x+y*y+z*z) < r_ini*r_ini );//overdensity region
  double r_coor=x*x+y*y+z*z;
  r_coor=pow(r_coor,0.5);

  //double H_i=2/(3*t_ini); //Hubble constant
  //double rho_ini=1/(6*pi*G*t_ini*t_ini);

  double rho_ini=tilde_rho_ini;
  //constexpr double gamma = 5.0/3.0;
  if (iseed==0) {
    Q[0] = isInTheSphere ? rho_ini*(1+delta_rho) : rho_ini;
  }  // rho
  if (iseed==1)
  	{Q[0] = rho_ini;}
  if (v_scale==0)
		{Q[1] = 0; Q[2] = 0; Q[3] = 0;} // velocities
  else
    {if (r_coor<r_point) 
       {Q[1]=-v_scale*x*1.5*Omega_m*a_i*delta_m/4/pi/pow(r_point,3)*Q[0];
        Q[2]=-v_scale*y*1.5*Omega_m*a_i*delta_m/4/pi/pow(r_point,3)*Q[0];
        Q[3]=-v_scale*z*1.5*Omega_m*a_i*delta_m/4/pi/pow(r_point,3)*Q[0];}
     else
       {Q[1]=-v_scale*x*1.5*Omega_m*a_i*delta_m/4/pi/pow(r_coor,3)*Q[0];
        Q[2]=-v_scale*y*1.5*Omega_m*a_i*delta_m/4/pi/pow(r_coor,3)*Q[0];
        Q[3]=-v_scale*z*1.5*Omega_m*a_i*delta_m/4/pi/pow(r_coor,3)*Q[0];}
    }
  Q[4] = 0.5*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3])/Q[0]+tilde_P_ini/(gamma-1); // inner energy

  for (int i=NumberOfUnknowns; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  const double irho = 1./Q[0];
  #if Dimensions==3
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
  #else
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
  #endif

  Q[0] = rho_ini;
  nonCriticalAssertion6( p>=0.0, Q[0], Q[1], Q[2], Q[3], Q[4], volumeH );
  assertion5( Q[0]>1e-12, Q[1], Q[2], Q[3], Q[4], volumeH );
  // initial conditions
/*
  Q[0] = isInTheSphere ? rho_ini*(1+delta_rho) : rho_ini;  // rho
  Q[1] = Q[0]*H_i*x;    // velocities
  Q[2] = Q[0]*H_i*y;
  Q[3] = Q[0]*H_i*z;
  Q[4] = 0.5*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3])/Q[0]+initial_internal_energy; // inner energy
*/
  tarch::la::Vector<Dimensions,double> circleCentre = {0,0,0};
/*
  // initial conditions
  bool isInTheCentre = ( tarch::la::norm2( volumeCentre-circleCentre ) < 0.2 );
  Q[0] = 0.1;  // rho
  Q[1] = 0;    // velocities
  Q[2] = 0;
  Q[3] = 0;
  Q[4] = isInTheCentre ? 1.0 : 0.0; // inner energy
*/
  if ( Q[0]<1e-12 ){
    ::tarch::triggerNonCriticalAssertion( __FILE__, __LINE__, "Q[0]>0", "density become 0 at volume: ["+std::to_string(volumeCentre(0))+", "+std::to_string(volumeCentre(1))+", "+std::to_string(volumeCentre(2))+"] Q[] array: "+std::to_string(Q[0])+" "+std::to_string(Q[1])+" "+std::to_string(Q[2])+" "+std::to_string(Q[3])+" "+std::to_string(Q[4])+".");
  }

  for (int i=5; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }
}


double applications::exahype2::euler::sphericalaccretion::SSInfall::getInitialMIn( double r_coor ) const {
  const double rho_ini=tilde_rho_ini;
  if (iseed==0) {//not using
    if (r_coor<r_ini) {
      return rho_ini*delta_rho*pow(r_coor,3)/3;
    }
    else {
      return rho_ini*delta_rho*pow(r_ini,3)/3;
    }
  }
  if (iseed==1) {//tophat
    if (r_coor<r_point){
      return delta_m*pow(r_coor/r_point,3)/4/tarch::la::PI;
    }
    else {
      return delta_m/4/tarch::la::PI;
    }
  }
  return 0.0;
}


double applications::exahype2::euler::sphericalaccretion::SSInfall::getForceDensityNorm(double r_coor,double m_in, double t, double density, double a_input) {
  double a=0.0287*pow((-t/11.8+0.1694*pow(a_i,-0.5)),-2);//when code time ~ 2*(a_i^(-0.5)-1), a~1
  #ifdef useTable
  a=a_input; //if we use a table to interpolate a (for LCDM), we replace it here.
  #endif
  double force_density_norm=density*G*m_in/pow(r_coor,3)*Omega_m*a*1.5;
  #ifdef DGP
  //double y3_over_Delta=(4.0/3.0)*tarch::la::PI*pow(r_coor,3)/delta_m;
  double l3_over_tildeM=(4.0/3.0)*tarch::la::PI*pow(r_coor,3)/m_in;
  double modifier=0.0;
  if (tarch::la::equals(DGP_zeta,0.0)){modifier=1.0/3.0/DGP_beta;}
  else{modifier=(27*DGP_beta/16/DGP_zeta/DGP_zeta)*l3_over_tildeM*(pow(1+(32*DGP_zeta*DGP_zeta/81/DGP_beta/DGP_beta/l3_over_tildeM),0.5)-1);}
  //logInfo( "source()", "modifier" << modifier );
  force_density_norm=force_density_norm*(1.0+modifier);
  #endif
  return force_density_norm;
}


void applications::exahype2::euler::sphericalaccretion::SSInfall::sourceTerm(
  const double * __restrict__                  Q, // Q[5+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S  // S[5
) {
  logTraceInWith4Arguments( "sourceTerm(...)", volumeX, volumeH, t, dt );

  nonCriticalAssertion4( not std::isnan(Q[0]), volumeX, volumeH, t, dt );

  assertion4( not std::isnan(Q[0]), volumeX, volumeH, t, dt );
  assertion4( not std::isnan(Q[1]), volumeX, volumeH, t, dt );
  assertion4( not std::isnan(Q[2]), volumeX, volumeH, t, dt );
  assertion4( not std::isnan(Q[3]), volumeX, volumeH, t, dt );
  assertion4( not std::isnan(Q[4]), volumeX, volumeH, t, dt );
  assertion4( not std::isinf(Q[0]), volumeX, volumeH, t, dt );
  assertion4( not std::isinf(Q[1]), volumeX, volumeH, t, dt );
  assertion4( not std::isinf(Q[2]), volumeX, volumeH, t, dt );
  assertion4( not std::isinf(Q[3]), volumeX, volumeH, t, dt );
  assertion4( not std::isinf(Q[4]), volumeX, volumeH, t, dt );

  nonCriticalAssertion4(
    not std::isnan(Q[0]) or t<0.5,
    volumeX, volumeH, t, dt
  );

  double x=volumeX(0)-OverDensityCentre(0);
  double y=volumeX(1)-OverDensityCentre(1);
  double z=volumeX(2)-OverDensityCentre(2);

  double r_coor=x*x+y*y+z*z;
  r_coor=pow(r_coor,0.5);
  double m_in=0;
  
  if (tarch::la::equals(t,0)){//we know the mass distri at the beginning
    m_in = getInitialMIn( r_coor );
  } 
  else {
    if (iseed==0){
      m_in=mass_interpolate(r_coor,MassCal)/4.0/tarch::la::PI; //remove the overall 4\pi coefficient.
      nonCriticalAssertion4( not std::isnan(m_in), volumeX, volumeH, t, dt );
      nonCriticalAssertion4( m_in<1e100, volumeX, volumeH, t, dt );
    }
    if (iseed==1){
      if (r_coor<r_point){
        m_in=(mass_interpolate(r_coor, MassCal)+delta_m*pow(r_coor/r_point,3))/4.0/tarch::la::PI;
        nonCriticalAssertion4( not std::isnan(m_in), volumeX, volumeH, t, dt );
        nonCriticalAssertion4( m_in<1e100, volumeX, volumeH, t, dt );
      }
      else {
        assertion3( isOutsideOfLargestRadius(volumeX, volumeH), volumeX, volumeH, _rMax );
        m_in=(mass_interpolate(r_coor, MassCal)+delta_m)/4.0/tarch::la::PI;
        nonCriticalAssertion4( not std::isnan(m_in), volumeX, volumeH, t, dt );
        nonCriticalAssertion4( m_in<1e100, volumeX, volumeH, t, dt );
      }
    }
  }

  double force_density_norm = getForceDensityNorm(
    r_coor,
    m_in,
    t, Q[0],interpolated_a
  );

  S[0] = 0;  // rho
  S[1] = -force_density_norm*x;    // velocities
  S[2] = -force_density_norm*y;
  S[3] = -force_density_norm*z;
  S[4] = -force_density_norm*(Q[1]*x+Q[2]*y+Q[3]*z)/Q[0];

  for (int i=5; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    S[i] = 0.0;
  }

  logTraceOut( "sourceTerm(...)" );
}


#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
void applications::exahype2::euler::sphericalaccretion::SSInfall::sourceTerm(
  const double * __restrict__                  Q, // Q[5+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S, // S[5
  Offloadable
) {
  double r_coor         = tarch::la::norm2(volumeX - OverDensityCentre);

  double m_in = (4.0/3.0)*tarch::la::PI*(pow(r_coor,3)-pow(_rMax,3))*(_rMax-1.0) + _mTotMax;

  double force_density_norm = getForceDensityNorm(
    r_coor,
    m_in,
    t, Q[0],1.0
  );

  double x = volumeX(0)-OverDensityCentre(0);
  double y = volumeX(1)-OverDensityCentre(1);
  double z = volumeX(2)-OverDensityCentre(2);

  S[0] = 0.0;  // rho
  S[1] = -force_density_norm*x;    // velocities
  S[2] = -force_density_norm*y;
  S[3] = -force_density_norm*z;
  S[4] = -force_density_norm*(Q[1]*x+Q[2]*y+Q[3]*z)/Q[0];

  for (int i=5; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    S[i] = 0.0;
  }
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif


bool applications::exahype2::euler::sphericalaccretion::SSInfall::isOutsideOfLargestRadius(
  const tarch::la::Vector<Dimensions,double>&  patchCentre,
  const tarch::la::Vector<Dimensions,double>&  patchH
) const {
  double r_coor         = tarch::la::norm2(patchCentre - OverDensityCentre);
  double safetyDistance = std::sqrt(3) * tarch::la::max(patchH) / 2.0;
  return  r_coor + safetyDistance > _rMax;
}


bool applications::exahype2::euler::sphericalaccretion::SSInfall::patchCanUseStatelessPDETerms(
  const tarch::la::Vector<Dimensions,double>&  patchCentre,
  const tarch::la::Vector<Dimensions,double>&  patchH,
  double                                       t,
  double                                       dt
) const {
  return tarch::la::greater(t,0.0) and isOutsideOfLargestRadius(patchCentre,patchH);
}


void applications::exahype2::euler::sphericalaccretion::SSInfall::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[5+0]
  double * __restrict__                        Qoutside, // Qoutside[5+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "boundaryConditions(...)", faceCentre, volumeH, t, normal );

  nonCriticalAssertion4( Qinside[0]==Qinside[0], faceCentre, volumeH, t, normal );
  nonCriticalAssertion4( Qinside[1]==Qinside[1], faceCentre, volumeH, t, normal );
  nonCriticalAssertion4( Qinside[2]==Qinside[2], faceCentre, volumeH, t, normal );
  nonCriticalAssertion4( Qinside[3]==Qinside[3], faceCentre, volumeH, t, normal );
  nonCriticalAssertion4( Qinside[4]==Qinside[4], faceCentre, volumeH, t, normal );

  nonCriticalAssertion5( Qinside[0]>1e-12, faceCentre, volumeH, t, normal, Qinside[0] );
/*
  assertion5( Qinside[0]>1e-12, faceCentre, volumeH, t, normal, Qinside[0] );
  if ( Qinside[0]<1e-12 ){
    ::tarch::triggerNonCriticalAssertion( __FILE__, __LINE__, "Qinside[0]>0", "density become 0 at face: ["+std::to_string(faceCentre(0))+", "+std::to_string(faceCentre(1))+", "+std::to_string(faceCentre(2))+"] Q[] array: "+std::to_string(Qinside[0])+" "+std::to_string(Qinside[1])+" "+std::to_string(Qinside[2])+" "+std::to_string(Qinside[3])+" "+std::to_string(Qinside[4])+".");
  }
*/

  if (extrapolate_bc==0){
    Qoutside[0] = Qinside[0];
    Qoutside[1] = Qinside[1];
    Qoutside[2] = Qinside[2];
    Qoutside[3] = Qinside[3];
    Qoutside[4] = Qinside[4];
  }
  else if (extrapolate_bc==1)
  {
    Qoutside[0] = Qinside[0];
    Qoutside[4] = Qinside[4];
    for (int i=0; i<5; i++){
      if (normal<3) {
        Qoutside[i]=Qinside[i]+Qinside[5+i*3+normal]*(-volumeH(normal));
      }
      else if (normal>=3) {
        Qoutside[i]=Qinside[i]+Qinside[5+i*3+normal-3]*(volumeH(normal-3));
      } 
    }
    for (int i=NumberOfUnknowns; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    	Qoutside[i]=0;
    }
  }    
  else if (extrapolate_bc==2)
  {
    nonCriticalAssertion7(
      not std::isnan(Qinside[0]),
      normal, faceCentre, Qinside[0], Qinside[1], Qinside[2], Qinside[3], Qinside[4]
    );
    for (int i=NumberOfUnknowns; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    	Qoutside[i]=0;
    }  
    for (int i=0; i<5; i++){
      if (normal<3) {
        Qoutside[i]=Qinside[i]+Qinside[5+i*3+normal]*(-volumeH(normal));
      }
      else if (normal>=3) {
        Qoutside[i]=Qinside[i]+Qinside[5+i*3+normal-3]*(volumeH(normal-3));
      } 
    }  
    if (Qinside[0]<tilde_rho_ini){
      for (int i=0; i<5; i++){Qoutside[i] = Qinside[i];}
    }
  }  
  //add more constraints here
  //for (int j=1; j<=3; j++){
  //  if (Qoutside[j]*Qinside[j]<0) {Qoutside[j]=0;}
  //}
  const double p = (gamma-1) * (Qoutside[4] - 0.5*(Qoutside[1]*Qoutside[1]+Qoutside[2]*Qoutside[2]+Qoutside[3]*Qoutside[3])/Qoutside[0]); 
  if (p<0){
    Qoutside[4]=0.5*(Qoutside[1]*Qoutside[1]+Qoutside[2]*Qoutside[2]+Qoutside[3]*Qoutside[3])/Qoutside[0]+1e-10;
  }   
  nonCriticalAssertion7(
    not std::isnan(Qinside[0]),
    normal, faceCentre, Qinside[0], Qinside[1], Qinside[2], Qinside[3], Qinside[4]
  );

  for (int i=5; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    Qoutside[i]=0;
  }

  logTraceOut( "boundaryConditions(...)" );
}


double applications::exahype2::euler::sphericalaccretion::SSInfall::maxEigenvalue(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal
)  {
  assertion(normal>=0);
  assertion(normal<Dimensions);
  assertion( Q[0]>0.0 );

  assertion4( not std::isnan(Q[0]), faceCentre, volumeH, t, normal );
  assertion4( not std::isnan(Q[1]), faceCentre, volumeH, t, normal );
  assertion4( not std::isnan(Q[2]), faceCentre, volumeH, t, normal );
  assertion4( not std::isnan(Q[3]), faceCentre, volumeH, t, normal );
  assertion4( not std::isnan(Q[4]), faceCentre, volumeH, t, normal );
  assertion4( not std::isinf(Q[0]), faceCentre, volumeH, t, normal );
  assertion4( not std::isinf(Q[1]), faceCentre, volumeH, t, normal );
  assertion4( not std::isinf(Q[2]), faceCentre, volumeH, t, normal );
  assertion4( not std::isinf(Q[3]), faceCentre, volumeH, t, normal );
  assertion4( not std::isinf(Q[4]), faceCentre, volumeH, t, normal );

  double result = maxEigenvalue(Q,faceCentre,volumeH,t,dt,normal,Offloadable::Yes);

  nonCriticalAssertion10(
    result<10000,
    result, Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal
  );
  //if (t>1.0 and result<1e-8) {
  //  logInfo( "maxEigenvalue()", "max eigenvalue too small: " << result );
  //  std::abort();
  //}
  return result;
}


#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
double applications::exahype2::euler::sphericalaccretion::SSInfall::maxEigenvalue(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  Offloadable
)  {
  //constexpr double gamma = 5.0/3.0;
  const double irho = 1./Q[0];
  #if Dimensions==3
  double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
  #else
  double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
  #endif

  if ( p<0 ) {
    p=0;
    //::tarch::triggerNonCriticalAssertion( __FILE__, __LINE__, "p>=0", "negative pressure "+std::to_string(p)+" detected at t=" + std::to_string(t) + " at face position ["+std::to_string(faceCentre(0))+", "+std::to_string(faceCentre(1))+", "+std::to_string(faceCentre(2))+"] Q[] array: "+std::to_string(Q[0])+" "+std::to_string(Q[1])+" "+std::to_string(Q[2])+" "+std::to_string(Q[3])+" "+std::to_string(Q[4])+".");
  }

  // nonCriticalAssertion9( p>=0.0, Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
  const double c   = std::sqrt(gamma * p * irho);

  const double u_n = Q[normal + 1] * irho;
  double result = std::max( std::abs(u_n - c), std::abs(u_n + c)); //result=1;
  //nonCriticalAssertion14( result>0.0, result, p, u_n, irho, c, Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
  result=result*(1+C_1*exp(-C_2*t));

  return result;
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif


void applications::exahype2::euler::sphericalaccretion::SSInfall::flux(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ F // F[5]
)  {
  logTraceInWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );

  assertion4( Q[0]>0.0, faceCentre, volumeH, t, normal );
  assertion4( not std::isnan(Q[0]), faceCentre, volumeH, t, normal );
  assertion4( not std::isnan(Q[1]), faceCentre, volumeH, t, normal );
  assertion4( not std::isnan(Q[2]), faceCentre, volumeH, t, normal );
  assertion4( not std::isnan(Q[3]), faceCentre, volumeH, t, normal );
  assertion4( not std::isnan(Q[4]), faceCentre, volumeH, t, normal );
  assertion4( not std::isinf(Q[0]), faceCentre, volumeH, t, normal );
  assertion4( not std::isinf(Q[1]), faceCentre, volumeH, t, normal );
  assertion4( not std::isinf(Q[2]), faceCentre, volumeH, t, normal );
  assertion4( not std::isinf(Q[3]), faceCentre, volumeH, t, normal );
  assertion4( not std::isinf(Q[4]), faceCentre, volumeH, t, normal );

  flux(Q,faceCentre,volumeH,t,dt,normal,F,Offloadable::Yes);

  assertion4( not std::isnan(F[0]), faceCentre, volumeH, t, normal );
  assertion4( not std::isnan(F[1]), faceCentre, volumeH, t, normal );
  assertion4( not std::isnan(F[2]), faceCentre, volumeH, t, normal );
  assertion4( not std::isnan(F[3]), faceCentre, volumeH, t, normal );
  assertion4( not std::isnan(F[4]), faceCentre, volumeH, t, normal );
  assertion4( not std::isinf(F[0]), faceCentre, volumeH, t, normal );
  assertion4( not std::isinf(F[1]), faceCentre, volumeH, t, normal );
  assertion4( not std::isinf(F[2]), faceCentre, volumeH, t, normal );
  assertion4( not std::isinf(F[3]), faceCentre, volumeH, t, normal );
  assertion4( not std::isinf(F[4]), faceCentre, volumeH, t, normal );

  logTraceOutWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );
}


#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
void applications::exahype2::euler::sphericalaccretion::SSInfall::flux(
  const double * __restrict__                  Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__                        F, // F[5]
  Offloadable
)  {
  //constexpr double gamma = 5.0/3.0;
  const double irho = 1./Q[0];
  #if Dimensions==3
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
  #else
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
  #endif

  const double coeff = irho*Q[normal+1];
  F[0] = coeff*Q[0];
  F[1] = coeff*Q[1];
  F[2] = coeff*Q[2];
  F[3] = coeff*Q[3];
  F[4] = coeff*Q[4];
  F[normal+1] += p;
  F[4]        += coeff*p;
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif


void applications::exahype2::euler::sphericalaccretion::SSInfall::add_mass(
  const double r_coor,
  const double rho,
  const double size
) {
  static tarch::multicore::BooleanSemaphore _mySemaphore;

  //double m=(rho-1)*pow(size,3); //notice here we use overdensity
  double m=(rho-1)*pow(size,3);
  if (m<0){m=0.0;}
  //m=1;
  tarch::multicore::Lock myLock( _mySemaphore );
  for (int i=0;i<sample_number;i++){
    if ((r_coor+size/2)<r_s[i]) {
      m_tot[i]+=m;
      cell_tot[i]+=1; 
      global_m_tot[i]+=m; 
      global_cell_tot[i]+=1;
    }
    else if ((r_coor-size/2)>r_s[i]) {
      m_tot[i]+=0;
    }
    else {
      m_tot[i]+=m*std::max(0.0,pow((r_s[i]-r_coor+size/2),3))/pow(size,3);cell_tot[i]+=1;
      global_m_tot[i]+=m*std::max(0.0,pow((r_s[i]-r_coor+size/2),3))/pow(size,3); global_cell_tot[i]+=1;
    }
  }
}


double applications::exahype2::euler::sphericalaccretion::SSInfall::mass_interpolate(
  const double r_coor,
  const int MassCal
) {
  double a,b;
  double m_a,m_b;
  double m_result;

  if (MassCal==0){ //which means we use cell counting
    bool IsCenter=false;    // means that we are right in the centre
    bool IsOutSkirt=false;  // means that we are outside of the accreditation area

    if ( tarch::la::smallerEquals( r_coor,r_s[0] ) ) {
      a=0; b=r_s[0];
      m_a=0;
      m_b=m_tot_copy[0];
      IsCenter=true;
    }
    else if ( tarch::la::greater( r_coor, r_s[sample_number-1] ) ) {
      a=r_s[sample_number-2]; b=r_s[sample_number-1];
      m_a=m_tot_copy[sample_number-2];
      m_b=m_tot_copy[sample_number-1];
      IsOutSkirt=true;
    }
    else{
      for (int i=1;i<sample_number;i++){
        if ( tarch::la::greater(r_coor, r_s[i-1]) and tarch::la::smallerEquals(r_coor,r_s[i])) {
          a=r_s[i-1]; b=r_s[i];
          m_a=m_tot_copy[i-1];
          m_b=m_tot_copy[i];
        }
      }
    }

    if (IsCenter){
      m_result=m_b*pow((r_coor),3)/pow(b,3);
      assertion10( m_result>=0.0,  m_result, r_coor, MassCal, m_a, m_b, a, b, IsOutSkirt, plotGlobalMTot(false), plotGlobalMTot(true) );
      assertion10( m_result<1e100, m_result, r_coor, MassCal, m_a, m_b, a, b, IsOutSkirt, plotGlobalMTot(false), plotGlobalMTot(true) );
    }
    else if (IsOutSkirt){
      double vol_tem=(4.0/3.0)*tarch::la::PI*(pow(b,3)-pow(a,3));
      double rho_tem=(m_b-m_a)/vol_tem;
      double vol_out=(4.0/3.0)*tarch::la::PI*(pow(r_coor,3)-pow(b,3));
      m_result=m_b+rho_tem*vol_out;
      assertion14( m_result>=0.0,  m_result, r_coor, MassCal, m_a, m_b, a, b, IsOutSkirt, IsCenter, plotRs(), plotGlobalMTot(false), plotGlobalMTot(true), rho_tem, vol_out );
      assertion14( m_result<1e100, m_result, r_coor, MassCal, m_a, m_b, a, b, IsOutSkirt, IsCenter, plotRs(), plotGlobalMTot(false), plotGlobalMTot(true), rho_tem, vol_out );
    }
    else {  //linear interpolation
      m_result=m_a*(b-r_coor)/(b-a)+m_b*(r_coor-a)/(b-a);
    //try to use a more precise mass calculation scheme here
    /*double vol_tem=(4/3)*pi*(pow(b,3)-pow(a,3));
    double rho_tem=(m_b-m_a)/vol_tem;
    double vol_in=(4/3)*pi*(pow(r_coor,3)-pow(a,3));
    m_result=m_a+rho_tem*vol_in;*/
    //if (not m_b==0){    
      assertion11( m_result>=0.0,  m_result, r_coor, MassCal, m_a, m_b, a, b, IsOutSkirt, plotGlobalMTot(false), plotGlobalMTot(true), plotRs() );
      assertion11( m_result<1e100, m_result, r_coor, MassCal, m_a, m_b, a, b, IsOutSkirt, plotGlobalMTot(false), plotGlobalMTot(true), plotRs() );
    }
  }
  else if (MassCal==1) { //which means we use rho interpolation
    if (r_coor<r_s[0]) {
      double rho_currentpos=rho_0+(rho_x[0]-rho_0)*r_coor/r_s[0];
      m_result=(4.0/3.0)*tarch::la::PI*pow(r_coor,3)*((rho_0+rho_currentpos)/2-1);
      assertion9( m_result>=0.0,  m_result, r_coor, MassCal, m_a, m_b, a, b, plotGlobalMTot(false), plotGlobalMTot(true) );
      assertion9( m_result<1e100, m_result, r_coor, MassCal, m_a, m_b, a, b, plotGlobalMTot(false), plotGlobalMTot(true) );
    }
    else{
      for (int i=1;i<sample_number;i++) {
        if ((r_coor>r_s[i-1]) and (r_coor<r_s[i])) {
          double rho_currentpos=rho_x[i-1]*(r_s[i]-r_coor)/(r_s[i]-r_s[i-1])+rho_x[i]*(r_coor-r_s[i-1])/(r_s[i]-r_s[i-1]);
          m_result=(4.0/3.0)*tarch::la::PI*(pow(r_coor,3)-pow(r_s[i-1],3))*((rho_x[i-1]+rho_currentpos)/2-1);
          m_result+=m_tot_copy[i-1];
          assertion10( m_result>=0.0,  m_result, r_coor, MassCal, m_a, m_b, a, b, i, plotGlobalMTot(false), plotGlobalMTot(true) );
          assertion10( m_result<1e100, m_result, r_coor, MassCal, m_a, m_b, a, b, i, plotGlobalMTot(false), plotGlobalMTot(true) );
        }
      }
    }
    if (r_coor>r_s[sample_number-1]){
      m_result=(4.0/3.0)*tarch::la::PI*(pow(r_coor,3)-pow(r_s[sample_number-1],3))*(rho_x[sample_number-1]-1);
      m_result+=m_tot_copy[sample_number-1];
      assertion10( m_result>=0.0,  m_result, r_coor, MassCal, m_a, m_b, a, b, r_s[sample_number-1], plotGlobalMTot(false), plotGlobalMTot(true) );
      assertion10( m_result<1e100, m_result, r_coor, MassCal, m_a, m_b, a, b, r_s[sample_number-1], plotGlobalMTot(false), plotGlobalMTot(true) );
    }
  }

  return m_result;
}

