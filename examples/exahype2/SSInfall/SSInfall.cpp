#include "SSInfall.h"
#include "exahype2/RefinementControl.h"

#include "Constants.h"
#include "exahype2/NonCriticalAssertions.h"

tarch::logging::Log   examples::exahype2::SSInfall::SSInfall::_log( "examples::exahype2::SSInfall::SSInfall" );

void examples::exahype2::SSInfall::SSInfall::startTimeStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
){
  AbstractSSInfall::startTimeStep(globalMinTimeStamp, globalMaxTimeStamp, globalMinTimeStepSize, globalMaxTimeStepSize);
  for (int i=0;i<sample_number;i++) {
    m_tot_copy[i]=m_tot[i];
    //std::cout << std::setprecision (4) << m_tot[i] << " ";
    m_tot[i]=0;
  }
  //std::cout << std::endl;
}


::exahype2::RefinementCommand examples::exahype2::SSInfall::SSInfall::refinementCriterion(
  const double * __restrict__ Q, // Q[5+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {

  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

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
  //double H_i=2/(3*t_ini); //Hubble constant
  //double rho_ini=1/(6*pi*G*t_ini*t_ini);

  double rho_ini=tilde_rho_ini;
  //constexpr double gamma = 5.0/3.0;
  if (iseed==0)
  	{Q[0] = isInTheSphere ? rho_ini*(1+delta_rho) : rho_ini;}  // rho
  if (iseed==1)
  	{Q[0] = rho_ini;}
  Q[1] = 0;    // velocities
  Q[2] = 0;
  Q[3] = 0;
  Q[4] = (0.5*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3])/Q[0]+tilde_P_ini)/(gamma-1); // inner energy

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
      m_in=mass_interpolate(r_coor)/4/pi; //remove the overall 4\pi coefficient. 
    }
    if (iseed==1){
		  if (r_coor<r_point){m_in=(mass_interpolate(r_coor)+delta_m*pow(r_coor/r_point,3))/4/pi;}
		  else {m_in=(mass_interpolate(r_coor)+delta_m)/4/pi;}
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


  Qoutside[0] = Qinside[0];
  Qoutside[1] = Qinside[1];
  Qoutside[2] = Qinside[2];
  Qoutside[3] = Qinside[3];
  Qoutside[4] = Qinside[4];

  logTraceOut( "boundaryConditions(...)" );
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
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
  #else
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
  #endif

  if ( p<0 ) {
    ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "p>=0", "negative pressure detected at t=" + std::to_string(t) + " at face position ["+std::to_string(faceCentre(0))+", "+std::to_string(faceCentre(1))+", "+std::to_string(faceCentre(2))+"]");
  }

  nonCriticalAssertion9( p>=0.0, Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
  const double c   = std::sqrt(gamma * p * irho);

  const double u_n = Q[normal + 1] * irho;
  double result = std::max( std::abs(u_n - c), std::abs(u_n + c)); //result=1;
  nonCriticalAssertion14( result>0.0, result, p, u_n, irho, c, Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
  result=result*(1+C_1*exp(-C_2*t));
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
  double m=(rho-1)*pow(size,3); //notice here we use overdensity

  for (int i=0;i<sample_number;i++){
    if ((r_coor+size/2)<r_s[i]) {m_tot[i]+=m;}
    else if ((r_coor-size/2)>r_s[i]) {m_tot[i]+=0;}
    else {m_tot[i]+=m*pow((r_s[i]-r_coor+size/2),3)/pow(size,3);}
  }
}

double examples::exahype2::SSInfall::SSInfall::mass_interpolate(
  const double r_coor
) {
  double a,b;
  double m_a,m_b;
  double m_result;

  bool IsCenter=false;
  if (r_coor<r_s[0]) {
    a=0; b=r_s[0];
    m_a=0; m_b=m_tot_copy[0];
    IsCenter=true;
  }
  if (r_coor>r_s[sample_number-1]) {
    a=r_s[sample_number-2]; b=r_s[sample_number-1];
    m_a=m_tot_copy[sample_number-2]; m_b=m_tot_copy[sample_number-1];    
  }
  else{
    for (int i=1;i<sample_number;i++){
      if ((r_coor>r_s[i-1]) and (r_coor<r_s[i])){
        a=r_s[i-1]; b=r_s[i];
        m_a=m_tot_copy[i-1]; m_b=m_tot_copy[i];
        //std::cout << m_tot_copy << std::endl;
      }
    }
  }
  if (IsCenter){
    m_result=m_b*pow((r_coor),3)/pow(b,3);
  }
  else {  //linear interpolation
    m_result=m_a*(b-r_coor)/(b-a)+m_b*(r_coor-a)/(b-a);
  }

  return m_result;
}
















