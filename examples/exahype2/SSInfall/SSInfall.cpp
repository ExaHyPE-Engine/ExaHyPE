#include "SSInfall.h"
#include "exahype2/RefinementControl.h"

#include "Constants.h"
#include "exahype2/NonCriticalAssertions.h"
#include "tarch/multicore/Lock.h"

tarch::logging::Log   examples::exahype2::SSInfall::SSInfall::_log( "examples::exahype2::SSInfall::SSInfall" );

void examples::exahype2::SSInfall::SSInfall::startTimeStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
){
  AbstractSSInfall::startTimeStep(globalMinTimeStamp, globalMaxTimeStamp, globalMinTimeStepSize, globalMaxTimeStepSize);
  constexpr double pi = M_PI;
  if ( isFirstGridSweepOfTimeStep() ){
  for (int i=0;i<sample_number;i++) {
    m_tot_copy[i]=global_m_tot[i];
    //m_tot_copy[i]=m_tot[i];
    //if (MassCal==0) {std::cout << std::setprecision (16) << m_tot_copy[i] << " " <<global_cell_tot[i] <<" "<< r_s[i] << std::endl;}
    global_m_tot[i]=0; global_cell_tot[i]=0; m_tot[i]=0; cell_tot[i]=0;
    //std::cout << rho_x[i] <<" "<< r_s[i] << std::endl;
  }
  //std::cout << rho_0 << std::endl;
  if (MassCal==1){
    for(int i=0;i<sample_number;i++){
      m_tot_copy[i]=std::max(0.0,(4/3)*pi*pow(r_s[0],3)*((rho_0+rho_x[0])/2-1));
    }
    for(int i=1;i<sample_number;i++){
      double m_layer=(4/3)*pi*(pow(r_s[i],3)-pow(r_s[i-1],3))*((rho_x[i]+rho_x[i-1])/2-1);
      //std::cout << rho_x[i] <<" "<< rho_x[i-1]<<" " << m_layer << std::endl;
      for(int j=i;j<sample_number;j++){
        m_tot_copy[j]+=std::max(0.0,m_layer);
      }
    }
    //for (int i=0;i<sample_number;i++) {std::cout << m_tot_copy[i] <<" "<< (4/3)*pi*pow(r_s[i],3) <<" "<< r_s[i] << std::endl;}
  }
  }
}

void examples::exahype2::SSInfall::SSInfall::finishTimeStep(){
  AbstractSSInfall::finishTimeStep();
  //std::cout << "add mass together here!" << std::endl;
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
    std::cout << global_m_tot[i] <<" "<< m_tot[i] << std::endl;
  }
  #endif
}

::exahype2::RefinementCommand examples::exahype2::SSInfall::SSInfall::refinementCriterion(
  const double * __restrict__ Q, // Q[5+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {

  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;
  double radius=volumeX(0)*volumeX(0)+volumeX(1)*volumeX(1)+volumeX(2)*volumeX(2); radius=pow(radius,0.5);
  if (ReSwi==1){ //radius based
    constexpr int NumberOfRefinementLayers = 2;
    double Radius[NumberOfRefinementLayers] = {1.4,0.4};
    double MaxH[NumberOfRefinementLayers]   = {0.03,0.008};
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
  if (ReSwi==3){ //radius based
    if (radius<0.3) {result=::exahype2::RefinementCommand::Refine;}
  }
  return result;
}



void examples::exahype2::SSInfall::SSInfall::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructred
) {
  logDebug( "initialCondition(...)", "init volume at " << volumeCentre << "x" << volumeH << " (grid constructed=" << gridIsConstructred << ")" );
  constexpr double pi = M_PI;
  double x=volumeCentre(0)-center(0);
  double y=volumeCentre(1)-center(1);
  double z=volumeCentre(2)-center(2);

  bool isInTheSphere = ( (x*x+y*y+z*z) < r_ini*r_ini );//overdensity region
  double r_coor=x*x+y*y+z*z;
  r_coor=pow(r_coor,0.5);

  //double H_i=2/(3*t_ini); //Hubble constant
  //double rho_ini=1/(6*pi*G*t_ini*t_ini);

  double rho_ini=tilde_rho_ini;
  //constexpr double gamma = 5.0/3.0;
  if (iseed==0)
  	{Q[0] = isInTheSphere ? rho_ini*(1+delta_rho) : rho_ini;}  // rho
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

  const double irho = 1./Q[0];
  #if Dimensions==3
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
  #else
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
  #endif

  nonCriticalAssertion6( p>=0.0, Q[0], Q[1], Q[2], Q[3], Q[4], volumeH );
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
}


void examples::exahype2::SSInfall::SSInfall::sourceTerm(
  const double * __restrict__                  Q, // Q[5+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S  // S[5
) {
  logTraceInWith4Arguments( "sourceTerm(...)", volumeX, volumeH, t, dt );
  constexpr double pi = M_PI;
  logTraceOut( "sourceTerm(...)" );
  double x=volumeX(0)-center(0);
  double y=volumeX(1)-center(1);
  double z=volumeX(2)-center(2);

  double r_coor=x*x+y*y+z*z;
  r_coor=pow(r_coor,0.5);
  //double rho_ini=1/(6*pi*G*t_ini*t_ini);
  double rho_ini=tilde_rho_ini;
  double m_in=0;
  
  if (tarch::la::equals(t,0)){//we know the mass distri at the beginning
    if (iseed==0){
		  if (r_coor<r_ini) { m_in=rho_ini*delta_rho*pow(r_coor,3)/3;}
		  else { m_in=rho_ini*delta_rho*pow(r_ini,3)/3;}
		}
		if (iseed==1){
		  if (r_coor<r_point){m_in=delta_m*pow(r_coor/r_point,3)/4/pi;}
		  else {m_in=delta_m/4/pi;}
		}
  } 
  else {
    if (iseed==0){
      m_in=mass_interpolate(r_coor,MassCal)/4/pi; //remove the overall 4\pi coefficient. 
    }
    if (iseed==1){
		  if (r_coor<r_point){m_in=(mass_interpolate(r_coor, MassCal)+delta_m*pow(r_coor/r_point,3))/4/pi;}
		  else {m_in=(mass_interpolate(r_coor, MassCal)+delta_m)/4/pi;}
		}
  }

  double a=0.0287*pow((-t/11.8+0.1694*pow(a_i,-0.5)),-2);//when code time ~ 2*(a_i^(-0.5)-1), a~1
  double force_density_norm=Q[0]*G*m_in/pow(r_coor,3)*Omega_m*a*1.5;
  //if (r_coor<1e-8) {force_density_norm=0;}//in case we meet explosive force at the grid center

  //force_density_norm=0;

  S[0] = 0;  // rho
  S[1] = -force_density_norm*x;    // velocities
  S[2] = -force_density_norm*y;
  S[3] = -force_density_norm*z;
  S[4] = -force_density_norm*(Q[1]*x+Q[2]*y+Q[3]*z)/Q[0];
  //for (int i=0;i<5;i++){S[i]=S[i]*tarch::la::volume(volumeH);}
/*  S[0] = 0;  // rho
  S[1] = 0;    // velocities
  S[2] = 0;
  S[3] = 0;
  S[4] = 0;*/
}


void examples::exahype2::SSInfall::SSInfall::boundaryConditions(
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

  //nonCriticalAssertion4( Qinside[0]>1e-12, faceCentre, volumeH, t, normal );
  nonCriticalAssertion5( Qinside[0]>1e-12, faceCentre, volumeH, t, normal, Qinside[0] );

  if (extrapolate_bc==0){
    Qoutside[0] = Qinside[0];
    Qoutside[1] = Qinside[1];
    Qoutside[2] = Qinside[2];
    Qoutside[3] = Qinside[3];
    Qoutside[4] = Qinside[4];
    for (int i=5; i<20; i++){
    	Qoutside[i]=0;
    }
  }
  else if (extrapolate_bc==1)
  {
      //std::cout <<  normal << " " << faceCentre(0) << " "<<faceCentre(1)<<" "<<faceCentre(2) << std::endl;
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
    for (int i=5; i<20; i++){
    	Qoutside[i]=0;
    }
  }    
  else if (extrapolate_bc==2)
  {
    nonCriticalAssertion7(
      not std::isnan(Qinside[0]),
      normal, faceCentre, Qinside[0], Qinside[1], Qinside[2], Qinside[3], Qinside[4]
    );
    for (int i=5; i<20; i++){
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
  //if (Qoutside[0]<tilde_rho_ini) {Qoutside[0]=tilde_rho_ini;std::cout << "reset density" << std::endl;}
  //for (int j=1; j<=3; j++){
  //  if (Qoutside[j]*Qinside[j]<0) {Qoutside[j]=0;}
  //}
  const double p = (gamma-1) * (Qoutside[4] - 0.5*(Qoutside[1]*Qoutside[1]+Qoutside[2]*Qoutside[2]+Qoutside[3]*Qoutside[3])/Qoutside[0]); 
  if (p<0){Qoutside[4]=0.5*(Qoutside[1]*Qoutside[1]+Qoutside[2]*Qoutside[2]+Qoutside[3]*Qoutside[3])/Qoutside[0]+1e-10;}   
  logTraceOut( "boundaryConditions(...)" );
  nonCriticalAssertion7(
    not std::isnan(Qinside[0]),
    normal, faceCentre, Qinside[0], Qinside[1], Qinside[2], Qinside[3], Qinside[4]
  );
}


double examples::exahype2::SSInfall::SSInfall::maxEigenvalue(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
)  {
  assertion(normal>=0);
  assertion(normal<Dimensions);
  assertion( Q[0]>0.0 );

  //constexpr double gamma = 5.0/3.0;
  const double irho = 1./Q[0];
  #if Dimensions==3
  double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
  #else
  double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
  #endif

  if ( p<0 ) {
    //std::cout << "what this function saw is " << p << std::endl;
    p=0;
    //::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "p>=0", "negative pressure "+std::to_string(p)+" detected at t=" + std::to_string(t) + " at face position ["+std::to_string(faceCentre(0))+", "+std::to_string(faceCentre(1))+", "+std::to_string(faceCentre(2))+"] Q[] array: "+std::to_string(Q[0])+" "+std::to_string(Q[1])+" "+std::to_string(Q[2])+" "+std::to_string(Q[3])+" "+std::to_string(Q[4])+".");
  }

  nonCriticalAssertion9( p>=0.0, Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
  const double c   = std::sqrt(gamma * p * irho);

  const double u_n = Q[normal + 1] * irho;
  double result = std::max( std::abs(u_n - c), std::abs(u_n + c)); //result=1;
  nonCriticalAssertion14( result>0.0, result, p, u_n, irho, c, Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
  //if (t>0.2 and result>1){
  result=result*(1+C_1*exp(-C_2*t));
    //std::cout <<" u_n "<<u_n<<" c "<< c <<" eigenvale " << result << std::endl;
  //}
  return result;
}



void examples::exahype2::SSInfall::SSInfall::flux(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ F // F[5]
)  {
  logTraceInWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );

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

  logTraceOutWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );
}

void examples::exahype2::SSInfall::SSInfall::add_mass(
  const double r_coor,
  const double rho,
  const double size
) {
  static tarch::multicore::BooleanSemaphore _mySemaphore;

  //double m=(rho-1)*pow(size,3); //notice here we use overdensity
  double m=(rho-1)*pow(size,3);
  if (m<0){m=0.0;}
  //m=1;
  //std::cout << m <<std::endl;
  tarch::multicore::Lock myLock( _mySemaphore );
  for (int i=0;i<sample_number;i++){
    if ((r_coor+size/2)<r_s[i]) {m_tot[i]+=m;cell_tot[i]+=1; global_m_tot[i]+=m; global_cell_tot[i]+=1;}
    else if ((r_coor-size/2)>r_s[i]) {m_tot[i]+=0;}
    else {
      m_tot[i]+=m*std::max(0.0,pow((r_s[i]-r_coor+size/2),3))/pow(size,3);cell_tot[i]+=1;
      global_m_tot[i]+=m*std::max(0.0,pow((r_s[i]-r_coor+size/2),3))/pow(size,3); global_cell_tot[i]+=1;
    }
  }
  //std::cout << m_tot[7] <<std::endl;
  /*for (int i=0;i<sample_number;i++){
    if (r_coor<r_s[i]) {m_tot[i]+=m; cell_tot[i]+=1;}
    else {m_tot[i]+=0;}
  }*/
}

double examples::exahype2::SSInfall::SSInfall::mass_interpolate(
  const double r_coor,
  const int MassCal
) {
  constexpr double pi = M_PI;
  double a,b;
  double m_a,m_b;
  double m_result;
  //for (int i=0;i<sample_number;i++) {std::cout << m_tot_copy[i] << " ";}
  //if (r_coor>0.84) {std::cout << "use it" << std::endl;}
if (MassCal==0){ //which means we use cell counting
  bool IsCenter=false;
  bool IsOutSkirt=false;

  if (r_coor<r_s[0]) {
    a=0; b=r_s[0];
    m_a=0; m_b=m_tot_copy[0];
    IsCenter=true;
  }
  else if (r_coor>r_s[sample_number-1]) {
    a=r_s[sample_number-2]; b=r_s[sample_number-1];
    m_a=m_tot_copy[sample_number-2]; m_b=m_tot_copy[sample_number-1];
    IsOutSkirt=true;    
  }
  else{
    for (int i=1;i<sample_number;i++){
      if ((r_coor>r_s[i-1]) and (r_coor<r_s[i])){
        a=r_s[i-1]; b=r_s[i];
        m_a=m_tot_copy[i-1]; m_b=m_tot_copy[i];
        //std::cout << m_tot_copy[i] << std::endl;
      }
    }
  }

  if (IsCenter){
    m_result=m_b*pow((r_coor),3)/pow(b,3);
  }
  else if (IsOutSkirt){
    double vol_tem=(4/3)*pi*(pow(b,3)-pow(a,3));
    double rho_tem=(m_b-m_a)/vol_tem;
    double vol_out=(4/3)*pi*(pow(r_coor,3)-pow(b,3));
    m_result=m_b+rho_tem*vol_out;
  }
  else {  //linear interpolation
    m_result=m_a*(b-r_coor)/(b-a)+m_b*(r_coor-a)/(b-a);
    //try to use a more precise mass calculation scheme here
    /*double vol_tem=(4/3)*pi*(pow(b,3)-pow(a,3));
    double rho_tem=(m_b-m_a)/vol_tem;
    double vol_in=(4/3)*pi*(pow(r_coor,3)-pow(a,3));
    m_result=m_a+rho_tem*vol_in;*/
    //if (not m_b==0){    
    //std::cout << m_b <<" "<< m_a << std::endl;}
    //std::cout << m_result <<" "<< vol_tem << " "<<rho_tem<<" "<<vol_in << std::endl;
  }
}  
else if (MassCal==1){ //which means we use rho interpolation
  if (r_coor<r_s[0]){
    double rho_currentpos=rho_0+(rho_x[0]-rho_0)*r_coor/r_s[0];
    m_result=(4/3)*pi*pow(r_coor,3)*((rho_0+rho_currentpos)/2-1);
  }
  else{
    for (int i=1;i<sample_number;i++){
      if ((r_coor>r_s[i-1]) and (r_coor<r_s[i])){
        double rho_currentpos=rho_x[i-1]*(r_s[i]-r_coor)/(r_s[i]-r_s[i-1])+rho_x[i]*(r_coor-r_s[i-1])/(r_s[i]-r_s[i-1]);
        m_result=(4/3)*pi*(pow(r_coor,3)-pow(r_s[i-1],3))*((rho_x[i-1]+rho_currentpos)/2-1);
        m_result+=m_tot_copy[i-1];
      }
    }
  }
  if (r_coor>r_s[sample_number-1]){
    m_result=(4/3)*pi*(pow(r_coor,3)-pow(r_s[sample_number-1],3))*(rho_x[sample_number-1]-1);
    m_result+=m_tot_copy[sample_number-1];
  }
//m_result=0.0;

}  
  //if (r_coor>0.84) {std::cout << m_result << std::endl;} 
  //m_result=0.0;

  return m_result;
}
















