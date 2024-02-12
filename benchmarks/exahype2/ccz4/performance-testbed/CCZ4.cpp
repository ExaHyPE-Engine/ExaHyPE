#include "CCZ4.h"
#include "exahype2/RefinementControl.h"
#include "tarch/NonCriticalAssertions.h"
//#include "exahype2/CellAccess.h"

#include <algorithm>

#include "Constants.h"

#include <limits>

#include <stdio.h>
#include <string.h>

#ifdef IncludeTwoPunctures
TP::TwoPunctures* _tp = new TP::TwoPunctures();
#endif

tarch::logging::Log   benchmarks::exahype2::ccz4::CCZ4::_log( "benchmarks::exahype2::ccz4::CCZ4" );

#ifdef IncludeTwoPunctures
//pre-process, solve the puncture equations
void benchmarks::exahype2::ccz4::CCZ4::prepare(TP::TwoPunctures* tp){
    //first we set the parameter. TODO:find a way to read parameter from python script
    //int swi=0;//0--single black hole,  2--BBH hoc, 2--BBH rotation, 3--GW150914
    //bbhtype=0 head on, 1-b=3, 2-b=2
    
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

  if (CCZ4swi==4){
    tp->par_b=1.0;
    tp->center_offset[0]=-3.0; tp->center_offset[1]=-2.0; tp->center_offset[2]=0.0;
    tp->target_M_plus=1.0;//adm mass
    tp->par_P_plus[0]=0.1; tp->par_P_plus[1]=0.1; tp->par_P_plus[2]=0.0;//linear momentum
    tp->par_S_plus[0]=0.0; tp->par_S_plus[1]=0.0; tp->par_S_plus[2]=0.0;//spin
    tp->target_M_minus=0.0;//adm mass
    tp->par_P_minus[0]=0.0; tp->par_P_minus[1]=0.0; tp->par_P_minus[2]=0.0;//linear momentum
    tp->par_S_minus[0]=0.0; tp->par_S_minus[1]=0.0; tp->par_S_minus[2]=0.0; //spin    
    tp->grid_setup_method="evaluation"; //evaluation or Taylor expansion
    tp->TP_epsilon=1e-6;}
		
	if (CCZ4swi==2 and CCZ4BBHType==0){
		tp->par_b=2.0;
		tp->center_offset[0]=0.0; tp->center_offset[1]=0.0; tp->center_offset[2]=0.0;
		tp->target_M_plus=0.5;//adm mass
		tp->par_P_plus[0]=0.0; tp->par_P_plus[1]=0.0; tp->par_P_plus[2]=0.0;//linear momentum
		tp->par_S_plus[0]=0.0; tp->par_S_plus[1]=0.0; tp->par_S_plus[2]=0.0;//spin
		tp->target_M_minus=0.5;//adm mass
		tp->par_P_minus[0]=0.0; tp->par_P_minus[1]=0.0; tp->par_P_minus[2]=0.0;//linear momentum
		tp->par_S_minus[0]=0.0; tp->par_S_minus[1]=0.0; tp->par_S_minus[2]=0.0; //spin		
		tp->grid_setup_method="evaluation"; //evaluation or Taylor expansion
		tp->TP_epsilon=1e-6;}
	
  //following data reference: https://journals.aps.org/prd/pdf/10.1103/PhysRevD.69.024006
	if (CCZ4swi==2 and CCZ4BBHType==1){
		tp->par_b=3.0;
		tp->center_offset[0]=0.0; tp->center_offset[1]=0.0; tp->center_offset[2]=0.0;
		tp->give_bare_mass=true;//use puncture mass instead of adm mass
		tp->par_m_plus=0.47656; tp->par_m_minus=0.47656;
		//tp->target_M_plus=999;//adm mass
		tp->par_P_plus[0]=0.0; tp->par_P_plus[1]=0.13808; tp->par_P_plus[2]=0.0;//linear momentum
		tp->par_S_plus[0]=0.0; tp->par_S_plus[1]=0.0; tp->par_S_plus[2]=0.0;//spin
		//tp->target_M_minus=999;//adm mass
		tp->par_P_minus[0]=0.0; tp->par_P_minus[1]=-0.13808; tp->par_P_minus[2]=0.0;//linear momentum
		tp->par_S_minus[0]=0.0; tp->par_S_minus[1]=0.0; tp->par_S_minus[2]=0.0; //spin		
		tp->grid_setup_method="evaluation"; //evaluation or Taylor expansion
		tp->TP_epsilon=1e-6;}

  if (CCZ4swi==2 and CCZ4BBHType==2){
    tp->par_b=2.0;
    tp->center_offset[0]=0.0; tp->center_offset[1]=0.0; tp->center_offset[2]=0.0;
    tp->give_bare_mass=true;//use puncture mass instead of adm mass
    tp->par_m_plus=0.46477; tp->par_m_minus=0.46477;
    //tp->target_M_plus=999;//adm mass
    tp->par_P_plus[0]=0.0; tp->par_P_plus[1]=0.19243; tp->par_P_plus[2]=0.0;//linear momentum
    tp->par_S_plus[0]=0.0; tp->par_S_plus[1]=0.0; tp->par_S_plus[2]=0.0;//spin
    //tp->target_M_minus=999;//adm mass
    tp->par_P_minus[0]=0.0; tp->par_P_minus[1]=-0.19243; tp->par_P_minus[2]=0.0;//linear momentum
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

benchmarks::exahype2::ccz4::CCZ4::CCZ4() {
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
    //std::cerr << "initial scenario " << Scenario << " is not supported" << std::endl << std::endl << std::endl;
  }
}

void benchmarks::exahype2::ccz4::CCZ4::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructed
) {
  logTraceInWith3Arguments( "initialCondition(...)", volumeX, volumeH, gridIsConstructed );

  if ( Scenario==0 ) {
    applications::exahype2::ccz4::gaugeWave(Q, volumeX, 0);
  }
  else if ( Scenario==3 ) {
    applications::exahype2::ccz4::diagonal_gaugeWave(Q, volumeX, 0);
  }
  else if ( Scenario==1 ) {
    applications::exahype2::ccz4::linearWave(Q, volumeX, 0);
  }
  else if ( Scenario==4 ) {
    applications::exahype2::ccz4::flat(Q, volumeX, 0);
  }
  #ifdef IncludeTwoPunctures
  else if ( Scenario==2 ) {

   // We use the bool to trigger the hgh res interpolation once the grid is constructed
    applications::exahype2::ccz4::ApplyTwoPunctures(Q, volumeX, 0, _tp, not gridIsConstructed); //we interpolate for real IC here.
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


void benchmarks::exahype2::ccz4::CCZ4::sourceTerm(
  const double * __restrict__                  Q, // Q[59+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S  // S[59
) {
#if !defined(GPUOffloadingOMP)
  logTraceInWith4Arguments( "sourceTerm(...)", volumeX, volumeH, t, dt );
  for(int i=0; i<NumberOfUnknowns; i++){
    assertion3( std::isfinite(Q[i]), i, volumeX, t );
  }
#endif
 // for (int i=NumberOfUnknowns; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
  //  S[i] = 0.0;
  //}
  benchmarks::exahype2::ccz4::CCZ4::sourceTerm(Q, volumeX, volumeH, t, dt, S, Offloadable::Yes);
 /*
  if (CCZ4smoothing>0){// then we apply simple laplacian smoothing
    constexpr int NumberOfRefinementLayers = 3;
    double Radius[NumberOfRefinementLayers] = {5.0*1.8, 3.0*1.8, 1.5*1.8};
    double radius=volumeX(0)*volumeX(0)+volumeX(1)*volumeX(1)+volumeX(2)*volumeX(2); radius=pow(radius,0.5);
    ::exahype2::CellAccess access(
      Q,                         // make it initially point to input
      1,                         // halo size, which you know as user
      NumberOfUnknowns,          // defined in superclass
      NumberOfAuxiliaryVariables // defined in superclass
      NumberOfDoFsPerAxisInCell, // defined in superclass
    );
    double laplacian=0.0;
    for (int unknown=0; unknown<NumberOfUnknowns; unknown++) {
      laplacian = -6.0 * access.centre(unknown)
        + 1.0 * access.left(0,unknown)+ 1.0 * access.right(0,unknown)
        + 1.0 * access.left(1,unknown)+ 1.0 * access.right(1,unknown)
        + 1.0 * access.left(2,unknown)+ 1.0 * access.right(2,unknown);
      laplacian /= volumeH(0);
      laplacian /= volumeH(0);
      double coef=CCZ4smoothing*(std::exp(-5.0*std::abs(radius-Radius[0])) + std::exp(-5.0*std::abs(radius-Radius[1])) + std::exp(-5.0*std::abs(radius-Radius[2])) )/3.0;
      coef=CCZ4smoothing;
      S[unknown]+=coef*laplacian;
    }
  }*/

#if !defined(GPUOffloadingOMP)
  for(int i=0; i<NumberOfUnknowns; i++){
    nonCriticalAssertion3( std::isfinite(S[i]), i, volumeX, t );
  }
  logTraceOut( "sourceTerm(...)" );
#endif
}


void benchmarks::exahype2::ccz4::CCZ4::boundaryConditions(
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


double benchmarks::exahype2::ccz4::CCZ4::maxEigenvalue(
  const double * __restrict__ Q, // Q[59+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal
)
{
   return maxEigenvalue(Q, faceCentre, volumeH, t, dt, normal, Offloadable::Yes);
}


void benchmarks::exahype2::ccz4::CCZ4::nonconservativeProduct(
  const double * __restrict__ Q, // Q[59+0],
  const double * __restrict__             deltaQ, // [59+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ BgradQ // BgradQ[59]
)  {
#if !defined(GPUOffloadingOMP)
  logTraceInWith4Arguments( "nonconservativeProduct(...)", faceCentre, volumeH, t, normal );
  assertion( normal>=0 );
  assertion( normal<Dimensions );
#endif
  nonconservativeProduct(Q, deltaQ, faceCentre, volumeH, t, dt, normal, BgradQ, Offloadable::Yes);

#if !defined(GPUOffloadingOMP)
  for (int i=0; i<NumberOfUnknowns; i++) {
    nonCriticalAssertion4( std::isfinite(BgradQ[i]), i, faceCentre, t, normal );
  }
  logTraceOut( "nonconservativeProduct(...)" );
#endif
}

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
void benchmarks::exahype2::ccz4::CCZ4::nonconservativeProduct(
  const double * __restrict__ Q, // Q[59+0],
  const double * __restrict__             deltaQ, // [59+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ BgradQ, // BgradQ[59]
  Offloadable
)
{
  double gradQSerialised[NumberOfUnknowns*3];
  for (int i=0; i<NumberOfUnknowns; i++) {
    gradQSerialised[i+0*NumberOfUnknowns] = 0.0;
    gradQSerialised[i+1*NumberOfUnknowns] = 0.0;
    gradQSerialised[i+2*NumberOfUnknowns] = 0.0;

    gradQSerialised[i+normal*NumberOfUnknowns] = deltaQ[i];
  }
  //for (int i=NumberOfUnknowns; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
  //  BgradQ[i] = 0.0;
  //}
  applications::exahype2::ccz4::ncp(BgradQ, Q, gradQSerialised, normal, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4mu, CCZ4SO);
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

::exahype2::RefinementCommand benchmarks::exahype2::ccz4::CCZ4::refinementCriterion(
  const double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>& volumeCentre,
  const tarch::la::Vector<Dimensions,double>& volumeH,
  double t
) {
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  const double radius = tarch::la::norm2( volumeCentre );
  //
  // see documentation in header file
  //
  if (CCZ4ReSwi==1) { //radius based
    if (radius<0.1) {
      result=::exahype2::RefinementCommand::Refine;
    }
  }
  if (CCZ4ReSwi==2) { //
    if (tarch::la::equals(t,0.0)){  //as we use a quantity calculated in postpocessing, we need to provide criterion at the first timestep 
      constexpr int NumberOfRefinementLayers = 2;
//    current "standard" refinement pattern
      double Radius[NumberOfRefinementLayers] = {4, 2.5};
      double MaxH[NumberOfRefinementLayers]   = {0.15,0.04};
      result = ::exahype2::RefinementCommand::Keep;
      for (int i=0; i<NumberOfRefinementLayers; i++) {
        if (radius<Radius[i] and tarch::la::max(volumeH)>MaxH[i]) {
          result=::exahype2::RefinementCommand::Refine;
        }
      }
    }
  }
  if (CCZ4ReSwi==3) { //binary black holes
    //double radius1=(volumeCentre(0)-4.251)*(volumeCentre(0)-4.251)+volumeCentre(1)*volumeCentre(1)+volumeCentre(2)*volumeCentre(2);
    //double radius2=(volumeCentre(0)+4.251)*(volumeCentre(0)+4.251)+volumeCentre(1)*volumeCentre(1)+volumeCentre(2)*volumeCentre(2);

    tarch::la::Vector<Dimensions,double> leftBH  = {-4.241, 0.0, 0.0};
    tarch::la::Vector<Dimensions,double> rightBH = { 4.241, 0.0, 0.0};

    const double radius1 = tarch::la::norm2( volumeCentre - leftBH  );
    const double radius2 = tarch::la::norm2( volumeCentre - rightBH );

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
  if (CCZ4ReSwi==4){ //single black hole, higher level of amr
    if (tarch::la::equals(t,0.0)){  //as we use a quantity calculated in postpocessing, we need to provide criterion at the first timestep 
      constexpr int NumberOfRefinementLayers = 4;
 //   with some alter
      double Radius[NumberOfRefinementLayers] = {5.0, 3.0, 1.5, 0.5};
      double MaxH[NumberOfRefinementLayers]   = {0.3, 0.15, 0.04, 0.01};

      result = ::exahype2::RefinementCommand::Keep;
      for (int i=0; i<NumberOfRefinementLayers; i++) {
        if (radius<Radius[i] and tarch::la::max(volumeH)>MaxH[i]) {
          result=::exahype2::RefinementCommand::Refine;
        }
      }
    }
  }
  if (CCZ4ReSwi==8){ //single black hole, higher level of amr
    if (tarch::la::equals(t,0.0)){  //as we use a quantity calculated in postpocessing, we need to provide criterion at the first timestep 
      constexpr int NumberOfRefinementLayers = 1;
      double Radius[NumberOfRefinementLayers] = {2};
      double MaxH[NumberOfRefinementLayers]   = {0.04};

      result = ::exahype2::RefinementCommand::Keep;
      for (int i=0; i<NumberOfRefinementLayers; i++) {
        if (radius<Radius[i] and tarch::la::max(volumeH)>MaxH[i]) {
          result=::exahype2::RefinementCommand::Refine;
        }
      }
    }
  }
  if (CCZ4ReSwi==5){ //single black hole, decent refinement
    if (tarch::la::equals(t,0.0)){  //as we use a quantity calculated in postpocessing, we need to provide criterion at the first timestep 
      constexpr int NumberOfRefinementLayers = 3;
//    current "standard" refinement pattern
      double Radius[NumberOfRefinementLayers] = {3.0, 1.0, 0.5};
      double MaxH[NumberOfRefinementLayers]   = {0.2, 0.06, 0.02};
//    with some alter
//      double Radius[NumberOfRefinementLayers] = {5.0, 3.0};
//      double MaxH[NumberOfRefinementLayers]   = {0.3, 0.15};

      result = ::exahype2::RefinementCommand::Keep;
      for (int i=0; i<NumberOfRefinementLayers; i++) {
        if (radius<Radius[i] and tarch::la::max(volumeH)>MaxH[i]) {
          result=::exahype2::RefinementCommand::Refine;
        }
      }
    }
  }
  if (CCZ4ReSwi==6){
    if (tarch::la::equals(t,0.0)){  //as we use a quantity calculated in postpocessing, we need to provide criterion at the first timestep 
      constexpr int NumberOfRefinementLayers = 2;
      double Radius[NumberOfRefinementLayers] = {4, 3};
      double MaxH[NumberOfRefinementLayers]   = {0.15,0.04};

      result = ::exahype2::RefinementCommand::Keep;
      for (int i=0; i<NumberOfRefinementLayers; i++) {
        if (radius<Radius[i] and tarch::la::max(volumeH)>MaxH[i]) {
          result=::exahype2::RefinementCommand::Refine;
        }
      }
    }
  }
  if (CCZ4ReSwi==7){ 
    if (tarch::la::equals(t,0.0)){  //as we use a quantity calculated in postpocessing, we need to provide criterion at the first timestep 
      constexpr int NumberOfRefinementLayers = 2;
      double Radius[NumberOfRefinementLayers] = {3.0, 1.5};
      double MaxH[NumberOfRefinementLayers]   = {0.15,0.04};
      result = ::exahype2::RefinementCommand::Keep;
      for (int i=0; i<NumberOfRefinementLayers; i++) {
        if (radius<Radius[i] and tarch::la::max(volumeH)>MaxH[i]) {
          result=::exahype2::RefinementCommand::Refine;
        }
      }
    }
  }
  return result;
}



