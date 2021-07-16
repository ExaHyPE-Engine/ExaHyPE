#include "FiniteVolumeCCZ4.h"
#include "exahype2/RefinementControl.h"
#include "exahype2/NonCriticalAssertions.h"

#include "InitialValues.h"
#include "CCZ4Kernels.h"

#include <algorithm>

#include "Constants.h"

#include <limits>

#include <stdio.h>
#include <string.h>

#ifdef IncludeTwoPunctures
TP::TwoPunctures* _tp = new TP::TwoPunctures();
#endif

tarch::logging::Log   examples::exahype2::ccz4::FiniteVolumeCCZ4::_log( "examples::exahype2::ccz4::FiniteVolumeCCZ4" );
examples::exahype2::ccz4::FiniteVolumeCCZ4::FiniteVolumeCCZ4() {
  if ( Scenario==0 || Scenario==1 ) {
    const char* name = "GaugeWave";
    int length = strlen(name);
    //initparameters_(&length, name);
  }
  #ifdef IncludeTwoPunctures
  if ( Scenario==2 ) {
    prepare(_tp);//we solve the puncture equation here.
    //exit(0);
  }
  #endif
  else {
    std::cerr << "initial scenario " << Scenario << " is not supported" << std::endl << std::endl << std::endl;
  }
}

#ifdef IncludeTwoPunctures
//pre-process, solve the puncture equations
void examples::exahype2::ccz4::FiniteVolumeCCZ4::prepare(TP::TwoPunctures* tp){
    //first we set the parameter. TODO:find a way to read parameter from python script
    //int swi=0;//0--single black hole, 1--BBH hoc, 2--BBH rotation, 3--GW150914
    
	if (CCZ4swi==0){
		tp->par_b=1.0;
		tp->center_offset[0]=-1.0; tp->center_offset[1]=0.0; tp->center_offset[2]=0.0;
		tp->target_M_plus=1.0;//adm mass
		tp->par_P_plus[0]=0.0; tp->par_P_plus[1]=0.0; tp->par_P_plus[2]=0.0;//linear momentum
		tp->par_S_plus[0]=0.0; tp->par_S_plus[1]=0.0; tp->par_S_plus[2]=0.0;//spin
		tp->target_M_minus=0.0;//adm mass
		tp->par_P_minus[0]=0.0; tp->par_P_minus[1]=0.0; tp->par_P_minus[2]=0.0;//linear momentum
		tp->par_S_minus[0]=0.0; tp->par_S_minus[1]=0.0; tp->par_S_minus[2]=0.0; //spin		
		tp->grid_setup_method="evaluation"; //evaluation or Taylor expansion
		tp->TP_epsilon=1e-6;}
		
	if (CCZ4swi==1){
		tp->par_b=4.0;
		tp->center_offset[0]=0.0; tp->center_offset[1]=0.0; tp->center_offset[2]=0.0;
		tp->target_M_plus=1.0;//adm mass
		tp->par_P_plus[0]=0.0; tp->par_P_plus[1]=0.0; tp->par_P_plus[2]=0.0;//linear momentum
		tp->par_S_plus[0]=0.0; tp->par_S_plus[1]=0.0; tp->par_S_plus[2]=0.0;//spin
		tp->target_M_minus=1.0;//adm mass
		tp->par_P_minus[0]=0.0; tp->par_P_minus[1]=0.0; tp->par_P_minus[2]=0.0;//linear momentum
		tp->par_S_minus[0]=0.0; tp->par_S_minus[1]=0.0; tp->par_S_minus[2]=0.0; //spin		
		tp->grid_setup_method="evaluation"; //evaluation or Taylor expansion
		tp->TP_epsilon=1e-6;}
	
	if (CCZ4swi==2){
		tp->par_b=4.251;
		tp->center_offset[0]=0.0; tp->center_offset[1]=0.0; tp->center_offset[2]=0.0;
		tp->give_bare_mass=true;//use puncture mass instead of adm mass
		tp->par_m_plus=0.494; tp->par_m_minus=0.494;
		//tp->target_M_plus=999;//adm mass
		tp->par_P_plus[0]=0.0; tp->par_P_plus[1]=0.1091; tp->par_P_plus[2]=0.0;//linear momentum
		tp->par_S_plus[0]=0.0; tp->par_S_plus[1]=0.0; tp->par_S_plus[2]=0.0;//spin
		//tp->target_M_minus=999;//adm mass
		tp->par_P_minus[0]=0.0; tp->par_P_minus[1]=-0.1091; tp->par_P_minus[2]=0.0;//linear momentum
		tp->par_S_minus[0]=0.0; tp->par_S_minus[1]=0.0; tp->par_S_minus[2]=0.0; //spin		
		tp->grid_setup_method="evaluation"; //evaluation or Taylor expansion
		tp->TP_epsilon=1e-6;}

	if (CCZ4swi==3){
		double D=10.0, q=36.0/29.0, chip=0.31, chim=-0.46, M=1.0;
		double Pr=-0.00084541526517121, Pphi=0.09530152296974252;
		double mp=M*q/(1+q), mm=M*1/(1+q);
		tp->par_b=5.0;
		tp->center_offset[0]=D*mm-D/2; tp->center_offset[1]=0.0; tp->center_offset[2]=0.0;
		tp->target_M_plus=mp;//adm mass
		tp->par_P_plus[0]=Pr; tp->par_P_plus[1]=Pphi; tp->par_P_plus[2]=0.0;//linear momentum
		tp->par_S_plus[0]=0.0; tp->par_S_plus[1]=0.0; tp->par_S_plus[2]=chip*mp*mp;//spin
		tp->target_M_minus=1/(1+q);//adm mass
		tp->par_P_minus[0]=-Pr; tp->par_P_minus[1]=-Pphi; tp->par_P_minus[2]=0.0;//linear momentum
		tp->par_S_minus[0]=0.0; tp->par_S_minus[1]=0.0; tp->par_S_minus[2]=chim*mm*mm; //spin		
		tp->grid_setup_method="evaluation"; //evaluation or Taylor expansion
		tp->TP_epsilon=1e-6;}
		tp->PrintParameters();
		
		//then solve the equation
	tp->Run();
}
#endif

void examples::exahype2::ccz4::FiniteVolumeCCZ4::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructred
) {
  logTraceInWith3Arguments( "initialCondition(...)", volumeX, volumeH, gridIsConstructred );

  if ( Scenario==0 ) {
    examples::exahype2::ccz4::gaugeWave(Q, volumeX, 0);
  }
  else if ( Scenario==1 ) {
    examples::exahype2::ccz4::linearWave(Q, volumeX, 0);
  }
  #ifdef IncludeTwoPunctures
  else if ( Scenario==2 ) {

   // We use the bool to trigger the hgh res interpolation once the grid is constructed
    examples::exahype2::ccz4::ApplyTwoPunctures(Q, volumeX, 0, _tp, not gridIsConstructred); //we interpolate for real IC here.
  }
  #endif
  else {
    logError( "initialCondition(...)", "initial scenario " << Scenario << " is not supported" );
  }

  for (int i=0; i<NumberOfUnknowns; i++) {
    assertion2( std::isfinite(Q[i]), volumeX, i );
  }

  for (int i=NumberOfUnknowns; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }


/*
  else {
    enforceCCZ4constraints(Q);
  }
*/
  logTraceOut( "initialCondition(...)" );
}


void examples::exahype2::ccz4::FiniteVolumeCCZ4::sourceTerm(
  const double * __restrict__                  Q, // Q[59+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S  // S[59
) {
  logTraceInWith4Arguments( "sourceTerm(...)", volumeX, volumeH, t, dt );
  for(int i=0; i<NumberOfUnknowns; i++){
    assertion3( std::isfinite(Q[i]), i, volumeX, t );
  }

  memset(S, 0, NumberOfUnknowns*sizeof(double));
  //pdesource_(S,Q);    //  S(Q)
  source(S,Q, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4itau, CCZ4eta, CCZ4k1, CCZ4k2, CCZ4k3);
  for(int i=0; i<NumberOfUnknowns; i++){
    nonCriticalAssertion3( std::isfinite(S[i]), i, volumeX, t );
  }
  logTraceOut( "sourceTerm(...)" );
}



void examples::exahype2::ccz4::FiniteVolumeCCZ4::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[59+0]
  double * __restrict__                        Qoutside, // Qoutside[59+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "boundaryConditions(...)", faceCentre, volumeH, t, normal );
  for(int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    assertion4( Qinside[i]==Qinside[i], faceCentre, t, normal, i );
    Qoutside[i]=Qinside[i];
  }
  logTraceOut( "boundaryConditions(...)" );
}


double examples::exahype2::ccz4::FiniteVolumeCCZ4::maxEigenvalue(
  const double * __restrict__ Q, // Q[59+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
)
{
#if defined(CCZ4EINSTEIN)
  const double qmin = std::min({Q[0],Q[3],Q[5]});
  const double alpha = std::max({1.0, std::exp(Q[16])}) * std::max({1.0, std::exp(Q[54])}) / std::sqrt(qmin);
#else
  const double alpha = 1.0;
#endif

  constexpr double sqrtwo = 1.4142135623730951;
  // NOTE parameters are stored in Constants.h
  const double tempA = alpha * std::max({sqrtwo, CCZ4e, CCZ4ds, CCZ4GLMc/alpha, CCZ4GLMd/alpha});
  const double tempB = Q[17+normal];//DOT_PRODUCT(Q(18:20),nv(:))
  double tem = std::max({1.0, std::abs(-tempA-tempB), std::abs(tempA-tempB)});
  bool flag = false;
  for(int i=0; i<(NumberOfUnknowns-1); i++){
    if ( std::abs(Q[i]) > 2.0) { flag = true; break; }
  }
  //return flag? 2*tem: tem;
  return tem;
  //// we are only interested in the maximum eigenvalue
  //return std::max({1.0, std::abs(-tempA-tempB), std::abs(tempA-tempB)});

  //logTraceInWith4Arguments( "maxEigenvalue(...)", faceCentre, volumeH, t, normal );
  //constexpr int Unknowns = 59;
  //double lambda[Unknowns];
  //for (int i=0; i<Unknowns; i++) {
    //nonCriticalAssertion4( std::isfinite(Q[i]), i, x, t, normal );
    //lambda[i] = 1.0;
  //}

  //// routine requires explicit normal vector
  //double normalVector[3];
  //normalVector[0] = normal % 3 == 0 ? 1.0 : 0.0;
  //normalVector[1] = normal % 3 == 1 ? 1.0 : 0.0;
  //normalVector[2] = normal % 3 == 2 ? 1.0 : 0.0;

  //// actual method invocation
  //pdeeigenvalues_(lambda, Q, normalVector);

  //// we are only interested in the maximum eigenvalue
  //double result = 0.0;
  //for (int i=0; i<Unknowns; i++) {
    //result = std::max(result,std::abs(lambda[i]));
  //}
  //logTraceOut( "maxEigenvalue(...)" );
  //printf("%f vs %f diff: %f alpha: %f tempA: %f\n\n", result, result2, result-result2, alpha, tempA/alpha);
  //return result;
}


void examples::exahype2::ccz4::FiniteVolumeCCZ4::nonconservativeProduct(
  const double * __restrict__ Q, // Q[59+0],
  const double * __restrict__             deltaQ, // [59+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ BgradQ // BgradQ[59]
)  {
#if !defined(OpenMPGPUOffloading)
  logTraceInWith4Arguments( "nonconservativeProduct(...)", faceCentre, volumeH, t, normal );
  assertion( normal>=0 );
  assertion( normal<Dimensions );
#endif
  double gradQSerialised[NumberOfUnknowns*3];
  for (int i=0; i<NumberOfUnknowns; i++) {
    gradQSerialised[i+0*NumberOfUnknowns] = 0.0;
    gradQSerialised[i+1*NumberOfUnknowns] = 0.0;
    gradQSerialised[i+2*NumberOfUnknowns] = 0.0;

    gradQSerialised[i+normal*NumberOfUnknowns] = deltaQ[i];
  }
  ncp(BgradQ, Q, gradQSerialised, normal, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4mu);
  //pdencp_(BgradQ, Q, gradQSerialised);

#if !defined(OpenMPGPUOffloading)
  for (int i=0; i<NumberOfUnknowns; i++) {
    nonCriticalAssertion4( std::isfinite(BgradQ[i]), i, faceCentre, t, normal );
  }
  logTraceOut( "nonconservativeProduct(...)" );
#endif
}

::exahype2::RefinementCommand examples::exahype2::ccz4::FiniteVolumeCCZ4::refinementCriterion(
  const double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>& volumeCentre,
  const tarch::la::Vector<Dimensions,double>& volumeH,
  double t
) {
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;
  //if (volumeCentre(0)>1.2) {result=::exahype2::RefinementCommand::Refine;}
  double radius=volumeCentre(0)*volumeCentre(0)+volumeCentre(1)*volumeCentre(1)+volumeCentre(2)*volumeCentre(2); radius=pow(radius,0.5);
  if (CCZ4ReSwi==1){ //radius based
    if (radius<5) {result=::exahype2::RefinementCommand::Refine;}
  }
  if (CCZ4ReSwi==2){ //single black hole
    if (tarch::la::equals(t,0.0)){  //as we use a quantity calculated in postpocessing, we need to provide criterion at the first timestep 
      if ((radius<5) and (volumeH(0)>1.0)) { result=::exahype2::RefinementCommand::Refine; }
      else if (radius<2.5) { result=::exahype2::RefinementCommand::Refine; }
      else {result = ::exahype2::RefinementCommand::Keep;}
    } /*else {
      if ((Q[65]>0.1) and (volumeH(0)>1.0)) { result=::exahype2::RefinementCommand::Refine; }
      else if (Q[65]>0.2) { result=::exahype2::RefinementCommand::Refine; }
      else {result = ::exahype2::RefinementCommand::Keep;}
    }*/
  }
  if (CCZ4ReSwi==3){ //binary black holes
  double radius1=(volumeCentre(0)-4.251)*(volumeCentre(0)-4.251)+volumeCentre(1)*volumeCentre(1)+volumeCentre(2)*volumeCentre(2); radius=pow(radius,0.5);
  double radius2=(volumeCentre(0)+4.251)*(volumeCentre(0)+4.251)+volumeCentre(1)*volumeCentre(1)+volumeCentre(2)*volumeCentre(2); radius=pow(radius,0.5);
    if (tarch::la::equals(t,0.0)){  //as we use a quantity calculated in postpocessing, we need to provide criterion at the first timestep 
      if ( ((radius1<5) or (radius2<5)) and (volumeH(0)>1.0)) { result=::exahype2::RefinementCommand::Refine; }
      else if ((radius1<2.5) or (radius2<2.5)) { result=::exahype2::RefinementCommand::Refine; }
      else {result = ::exahype2::RefinementCommand::Keep;}
    } else {
      if ((Q[65]>0.1) and (volumeH(0)>1.0)) { result=::exahype2::RefinementCommand::Refine; }
      else if (Q[65]>0.2) { result=::exahype2::RefinementCommand::Refine; }
      else {result = ::exahype2::RefinementCommand::Keep;}
    }
  }
  return result;
}



