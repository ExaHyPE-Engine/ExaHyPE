#include "GPRDR.h"
#include "exahype2/RefinementControl.h"
#include "Constants.h"


#include "InitialData.h"
#include "PDE.h"


#include <algorithm>


tarch::logging::Log   examples::exahype2::gprdr::GPRDR::_log( "examples::exahype2::gprdr::GPRDR" );


void examples::exahype2::gprdr::GPRDR::adjustSolution(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  logTraceInWith3Arguments( "adjustSolution(...)", volumeX, volumeH, t );
  if (tarch::la::equals(t,0.0) ) {
    if (tarch::la::equals(t,0.0)) {
      int md = 0;
      double cms = getMaxMeshSize();
      const int order = _NumberOfFiniteVolumesPerAxisPerPatch;

      std::fill_n(Q,NumberOfUnknowns,0.0);

      double x_3[3];
      #if Dimensions==2
      x_3[0] = volumeX(0);
      x_3[1] = volumeX(1);
      x_3[2] = 0.0;
      #else
      x_3[0] = volumeX(0);
      x_3[1] = volumeX(1);
      x_3[2] = volumeX(2);
      #endif

      initialdata_(x_3, &t, Q);
    }

 //    dynamicrupture_(x,&t,Q);//Duo April 10

    for(int i = 0; i< NumberOfUnknowns; i++){
      assert(std::isfinite(Q[i]));
    }
  }
  else {
    // other stuff
  }
  logTraceOut( "adjustSolution(...)" );
}




double examples::exahype2::gprdr::GPRDR::maxEigenvalue(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "eigenvalues(...)", faceCentre, volumeH, t, normal );

  // Tip: You find documentation for this method in header file "GPRDR::GPRDRSolver_FV.h".
  // Tip: See header file "GPRDR::AbstractGPRDRSolver_FV.h" for toolkit generated compile-time
  //      constants such as PatchSize, NumberOfVariables, and NumberOfParameters.

  // @todo Please implement/augment if required
  double nv[3] = {0.0};
  nv[normal] = 1.0;
  double lambda[NumberOfUnknowns];
  pdeeigenvalues_(lambda, Q, nv);

  double result = 0.0;
  for (int i=0; i<NumberOfUnknowns; i++) {
    result = std::max( result, std::abs(lambda[i]) );
  }

  logTraceOutWith1Argument( "eigenvalues(...)", result );
  return result;
}



void examples::exahype2::gprdr::GPRDR::flux(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ F
) {
  logTraceInWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );

  double F_tmp[3][NumberOfUnknowns];
  pdeflux_(F_tmp[0], F_tmp[1], F_tmp[2], Q);

  for (int i=0; i<NumberOfUnknowns; i++) {
    F[i] = F_tmp[normal][i];
  }

  logTraceOut( "flux(...)" );
}




void examples::exahype2::gprdr::GPRDR::nonconservativeProduct(
  double * __restrict__ Q,
  double                                       gradQ[27+3][Dimensions],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ BgradQ // BgradQ[27]
) {
  logTraceInWith4Arguments( "nonconservativeProduct(...)", faceCentre, volumeH, t, normal );

  double gradQ_tmp[(NumberOfMaterialParameters+NumberOfUnknowns)*3];

  for (int i=0; i<NumberOfMaterialParameters+NumberOfUnknowns; i++) {
    gradQ_tmp[i+0*(NumberOfMaterialParameters+NumberOfUnknowns)] = gradQ[i][0];
    gradQ_tmp[i+1*(NumberOfMaterialParameters+NumberOfUnknowns)] = gradQ[i][1];
    #if Dimensions==2
    gradQ_tmp[i+2*(NumberOfMaterialParameters+NumberOfUnknowns)] = 0.0;
    #else
    gradQ_tmp[i+2*(NumberOfMaterialParameters+NumberOfUnknowns)] = gradQ[i][2];
    #endif
  }

  pdencp_(BgradQ, Q, gradQ_tmp);

  logTraceOut( "nonconservativeProduct(...)" );
}

