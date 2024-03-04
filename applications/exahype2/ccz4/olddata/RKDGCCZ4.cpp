#include "RKDGCCZ4.h"
#include "exahype2/RefinementControl.h"
#include <algorithm>

#include "Constants.h"

#include <limits>

#include <stdio.h>
#include <string.h>
#include "tarch/NonCriticalAssertions.h"

#ifdef IncludeTwoPunctures
TP::TwoPunctures* _tp = new TP::TwoPunctures();
#endif

tarch::logging::Log   applications::exahype2::ccz4::RKDGCCZ4::_log( "applications::exahype2::ccz4::RKDGCCZ4" );

#ifdef IncludeTwoPunctures
//pre-process, solve the puncture equations
void applications::exahype2::ccz4::RKDGCCZ4::prepare(TP::TwoPunctures* tp){
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

applications::exahype2::ccz4::RKDGCCZ4::RKDGCCZ4() {
  if ( Scenario==0 || Scenario==1 ) {
    const char* name = "GaugeWave";//nothing to do here for now
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

::exahype2::RefinementCommand applications::exahype2::ccz4::RKDGCCZ4::refinementCriterion(
  const double * __restrict__ Q,               // Q[59+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  double                                       t
) {
  logTraceInWith3Arguments( "refinementCriterion(...)", x, h, t );
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  // see comments in header file

  logTraceOutWith1Argument( "refinementCriterion(...)", ::toString(result) );
  return result;
}




void applications::exahype2::ccz4::RKDGCCZ4::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
  bool                                         gridIsConstructed
) {
  logTraceInWith2Arguments( "initialCondition(...)", x, gridIsConstructed );

  if ( Scenario==0 ) {
    applications::exahype2::ccz4::gaugeWave(Q, x, 0);
  }
  else if ( Scenario==3 ) {
    applications::exahype2::ccz4::diagonal_gaugeWave(Q, x, 0);
  }
  else if ( Scenario==1 ) {
    applications::exahype2::ccz4::linearWave(Q, x, 0);
  }
  #ifdef IncludeTwoPunctures
  else if ( Scenario==2 ) {

   // We use the bool to trigger the hgh res interpolation once the grid is constructed
    applications::exahype2::ccz4::ApplyTwoPunctures(Q, x, 0, _tp, not gridIsConstructed); //we interpolate for real IC here.
  }
  #endif
  else {
    logError( "initialCondition(...)", "initial scenario " << Scenario << " is not supported" );
  }

  for (int i=0; i<NumberOfUnknowns; i++) {
    assertion2( std::isfinite(Q[i]), x, i );
  }

  for (int i=NumberOfUnknowns; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  logTraceOut( "initialCondition(...)" );
}





void applications::exahype2::ccz4::RKDGCCZ4::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[59+0]
  double * __restrict__                        Qoutside, // Qoutside[59+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  int                                          normal
) {
  logTraceInWith3Arguments( "boundaryConditions(...)", x, t, normal );
  for(int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    assertion4( Qinside[i]==Qinside[i], x, t, normal, i );
    Qoutside[i]=Qinside[i];
  }
  logTraceOut( "boundaryConditions(...)" );
}



double ::applications::exahype2::ccz4::RKDGCCZ4::maxEigenvalue(
  const double * __restrict__ Q, // Q[59+0],
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  double                                       dt,
  int                                          normal
)  {
  logTraceInWith4Arguments( "maxEigenvalue(...)", x, t, dt, normal );
  const double qmin = std::min({Q[0],Q[3],Q[5]});
  const double alpha = std::max({1.0, Q[16]}) * std::max({1.0, Q[54]}) / std::sqrt(qmin);

  constexpr double sqrtwo = 1.4142135623730951;
  const double tempA = alpha * std::max({sqrtwo, CCZ4e, CCZ4ds, CCZ4GLMc/alpha, CCZ4GLMd/alpha});
  const double tempB = Q[17+normal];//DOT_PRODUCT(Q(18:20),nv(:))

  logTraceOut( "maxEigenvalue(...)" );
  return std::max({1.0, std::abs(-tempA-tempB), std::abs(tempA-tempB)});
}




void ::applications::exahype2::ccz4::RKDGCCZ4::nonconservativeProduct(
  const double * __restrict__                  Q,    // Q[59+0],
  const double * __restrict__                  dQdx, // [59+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__                        BgradQ // BgradQ[59]
)  {
  logTraceInWith4Arguments( "nonconservativeProduct(...)", x, t, dt, normal );
  double dQdxSerialised[NumberOfUnknowns*3];
  for (int i=0; i<NumberOfUnknowns; i++) {
    dQdxSerialised[i+0*NumberOfUnknowns] = 0.0;
    dQdxSerialised[i+1*NumberOfUnknowns] = 0.0;
    dQdxSerialised[i+2*NumberOfUnknowns] = 0.0;

    dQdxSerialised[i+normal*NumberOfUnknowns] = dQdx[i];
  }
  ncp(BgradQ, Q, dQdxSerialised, normal, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4mu);
  logTraceOut( "nonconservativeProduct(...)" );
}




void ::applications::exahype2::ccz4::RKDGCCZ4::sourceTerm(
  const double * __restrict__                  Q, // Q[59+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S  // S[59]
) {
  logTraceInWith3Arguments( "sourceTerm(...)", x, t, dt );

  memset(S, 0, NumberOfUnknowns*sizeof(double));
  source(S,Q, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4itau, CCZ4eta, CCZ4k1, CCZ4k2, CCZ4k3);
  
  logTraceOut( "sourceTerm(...)" );
}


















