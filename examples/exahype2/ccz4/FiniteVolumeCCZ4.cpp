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
    TP_bindding::prepare(_tp);//we solve the puncture equation here.
    //exit(0);
  }
  #endif
  else {
    std::cerr << "initial scenario " << Scenario << " is not supported" << std::endl << std::endl << std::endl;
  }
}


void examples::exahype2::ccz4::FiniteVolumeCCZ4::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructred
) {
  logTraceInWith3Arguments( "boundaryCondition(...)", volumeX, volumeH, gridIsConstructred );

  if ( Scenario==0 ) {
    examples::exahype2::ccz4::gaugeWave(Q, volumeX, 0);
  }
  else if ( Scenario==1 ) {
    examples::exahype2::ccz4::linearWave(Q, volumeX, 0);
  }
  #ifdef IncludeTwoPunctures
  else if ( Scenario==2 ) {
    examples::exahype2::ccz4::ApplyTwoPunctures(Q, volumeX, 0, _tp); //we interpolate for real IC here.
  }
  #endif
  else {
    logError( "adjustSolution(...)", "initial scenario " << Scenario << " is not supported" );
  }

  for (int i=0; i<NumberOfUnknowns; i++) {
    assertion3( std::isfinite(Q[i]), volumeX, t, i );
  }

  for (int i=NumberOfUnknowns; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }


/*
  else {
    enforceCCZ4constraints(Q);
  }
*/
  logTraceOut( "boundaryCondition(...)" );
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
  double radius=volumeCentre(0)*volumeCentre(0)+volumeCentre(1)*volumeCentre(1)+volumeCentre(2)*volumeCentre(2);
  radius=pow(radius,0.5);
  if (radius<5) {result=::exahype2::RefinementCommand::Refine;}
  return result;
}



