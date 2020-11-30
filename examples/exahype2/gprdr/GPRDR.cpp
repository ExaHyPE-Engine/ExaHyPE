#include "GPRDR.h"
#include "Constants.h"
#include "InitialData.h"
#include "PDE.h"

#include "exahype2/RefinementControl.h"
#include "exahype2/NonCriticalAssertions.h"

#include "peano4/utils/Loop.h"




#include <algorithm>


tarch::logging::Log   examples::exahype2::gprdr::GPRDR::_log( "examples::exahype2::gprdr::GPRDR" );



examples::exahype2::gprdr::GPRDR::GPRDR() {
  const int length=Scenario.length();
  initparameters_(&length,&Scenario[0]);

  /*
  if (constants.isValueValidString("cgfile")) {
  std::string cgfile = constants.getValueAsString("cgfile");
  if (constants.isValueValidDouble("cx") && constants.isValueValidDouble("cy") && constants.isValueValidInt("isbinary") ) {
    const double cx = constants.getValueAsDouble("cx");
    const double cy = constants.getValueAsDouble("cy");
    const int isbinary = constants.getValueAsInt("isbinary");
    const int lengthcg=cgfile.length();

    logInfo("init(...)","CG File:"<<cgfile);
    logInfo("init(...)","CG center:("<<cx<<","<<cy<<")");
    logInfo("init(...)","CG binary:"<<isbinary);
    std::cout << cgfile << std::endl;
    //loadcgfile_(&_domainOffset[0],&_domainSize[0],&lengthcg,&cgfile[0],&cx,&cy,&isbinary);
    loadcgfile_(&_domainOffset[0],&_domainSize[0],&cx,&cy,&isbinary);
  }else{
    logInfo("init(...)","CG data CX is not correct.");
    std::abort();
  }
}
*/
}



// @todo I think this is underspecified, and we need dt here on the long run, too
void examples::exahype2::gprdr::GPRDR::adjustSolution(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  logTraceInWith3Arguments( "adjustSolution(...)", volumeX, volumeH, t );
  if (tarch::la::equals(t,0.0) ) {
    //
    // Initial conditions
    //
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
  else {
    //
    // Point-source
    //
    double source[NumberOfUnknowns];
    pdesource_(source, Q);
    for (int i=0; i<NumberOfUnknowns; i++) {
      Q[i] += getMinTimeStepSize() * source[i];
    }

    //
    // Guess this is the actual rupture part
    //
    dfor(k,_NumberOfFiniteVolumesPerAxisPerPatch) {
      int cellIndex = peano4::utils::dLinearised(k,_NumberOfFiniteVolumesPerAxisPerPatch);
      double* Q_cell = Q + cellIndex * NumberOfUnknowns;

      //dynamicrupture_(x,&t,&luh_cell[0],&slp);   //Duo April 28
    }
  }
/*
    GPRDR::AbstractGPRDRSolver_FV::solutionUpdate(luh, cellCenter, cellSize, t, dt, maxAdmissibleDt);
    constexpr int patchSize          = GPRDR::GPRDRSolver_FV::PatchSize;
    constexpr int ghostLayerWidth    = GPRDR::GPRDRSolver_FV::GhostLayerWidth;
    constexpr int patchBegin         = ghostLayerWidth; // patchBegin cell is inside domain
    constexpr int patchEnd           = patchBegin+patchSize; // patchEnd cell is outside domain
    constexpr int wholePatchSize      = patchSize+2*ghostLayerWidth;
    double x[3];
    double slp_p[3],slp_m[3];
    double slp;
    slp = 0.0;
    slp_p[2] = 0.0;
    slp_m[2] = 0.0;
    x[2] = 0.0;

  #if DIMENSIONS==3
    kernels::idx4 idx(wholePatchSize,wholePatchSize,wholePatchSize,NumberOfVariables);
    for (int k = patchBegin; k < patchEnd; k++) {
  #else
      kernels::idx3 idx(wholePatchSize,wholePatchSize,NumberOfVariables);
  #endif
      for (int j = patchBegin; j < patchEnd; j++) {
        for (int i = patchBegin; i < patchEnd; i++) {
  #if DIMENSIONS==3
    double* luh_cell = luh + idx(k,j,i,0);
  #else
    double* luh_cell = luh + idx(j,i,0);
      double* luh_m = luh+idx(patchEnd-j,i,0);

      slp = std::abs( (luh_cell[24]-luh_m[24]) );
      slp = std::abs(2*luh_cell[24]) ;
  #endif
    double luh_cell_new[NumberOfVariables];

        x[0] = cellSize[0]/patchSize*(0.5+i-patchBegin)+cellCenter[0]-0.5*cellSize[0];
        x[1] = cellSize[1]/patchSize*(0.5+j-patchBegin)+cellCenter[1]-0.5*cellSize[1];
        dynamicrupture_(x,&t,&luh_cell[0],&slp);   //Duo April 28


    updatesolutionode_(&luh_cell_new[0],luh_cell,&dt);
    std::copy_n(luh_cell_new,NumberOfVariables,luh_cell);
*/


  //
  // Sanity check
  //
  for(int i = 0; i< NumberOfUnknowns; i++){
    // If check fails, we write one snapshot and then terminate. A real assertion
    // would abbort immediately.
    nonCriticalAssertion(std::isfinite(Q[i]));
  }

  logTraceOut( "adjustSolution(...)" );
}


void examples::exahype2::gprdr::GPRDR::solveRiemannProblem(
   double * __restrict__ QL,
   double * __restrict__ QR,
   const tarch::la::Vector<Dimensions,double>&  faceCentre,
   double                                       volumeH,
   double                                       t,
   double                                       dt,
   int                                          normal,
   double * __restrict__ FL,
   double * __restrict__ FR
 ) {
  hllemfluxfv_(FL, FR, QL, QR, &normal);
}


/*double examples::exahype2::gprdr::GPRDR::maxEigenvalue(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "eigenvalues(...)", faceCentre, volumeH, t, normal );

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

  double gradQ_tmp[NumberOfUnknowns*3];

  for (int i=0; i<NumberOfUnknowns; i++) {
    gradQ_tmp[i+0*NumberOfUnknowns] = gradQ[i][0];
    gradQ_tmp[i+1*NumberOfUnknowns] = gradQ[i][1];
    #if Dimensions==2
    gradQ_tmp[i+2*NumberOfUnknowns] = 0.0;
    #else
    gradQ_tmp[i+2*NumberOfUnknowns] = gradQ[i][2];
    #endif
  }

  pdencp_(BgradQ, Q, gradQ_tmp);

  logTraceOut( "nonconservativeProduct(...)" );
}*/

