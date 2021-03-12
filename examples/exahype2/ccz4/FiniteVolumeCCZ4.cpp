#include "FiniteVolumeCCZ4.h"
#include "exahype2/RefinementControl.h"


#include "exahype2/NonCriticalAssertions.h"

/** it is for aderdg solver with signatures changed from the one with fv.
 *  * I manually include this header which in turn declared all the
 *   * routines written in Fortran.
 *    */
#include "PDE.h"
#include "Properties.h"
#include "InitialValue.h"

/**
 *  * This file is automatically created by Peano. I need it to interact with
 *   * the Python API, i.e. to read out data set there.
 *    */
#include "Constants.h"
using namespace examples::exahype2::ccz4;

#include <limits>

#include <stdio.h>
#include <string.h>



tarch::logging::Log   examples::exahype2::ccz4::FiniteVolumeCCZ4::_log( "examples::exahype2::ccz4::FiniteVolumeCCZ4" );



examples::exahype2::ccz4::FiniteVolumeCCZ4::FiniteVolumeCCZ4() {
  if ( Scenario=="gaugewave-c++" ) {
    const char* name = "GaugeWave";
    int length = strlen(name);
    initparameters_(&length, name);
  }
  else {
    std::cerr << "initial scenario " << Scenario << " is not supported" << std::endl << std::endl << std::endl;
  }
}


void examples::exahype2::ccz4::FiniteVolumeCCZ4::adjustSolution(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt
) {
  logTraceInWith4Arguments( "adjustSolution(...)", volumeX, volumeH, t, dt );
  if (tarch::la::equals(t,0.0) ) {
    if ( Scenario=="gaugewave-c++" ) {
      examples::exahype2::ccz4::gaugeWave(Q, volumeX, t);
    }
    else {
      logError( "adjustSolution(...)", "initial scenario " << Scenario << " is not supported" );
    }

    for (int i=0; i<NumberOfUnknowns; i++) {
      assertion3( std::isfinite(Q[i]), x, t, i );
    }

    for (int i=NumberOfUnknowns; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
      Q[i] = 0.0;
    }
  }
  else {
    enforceccz4constraintsholger_(Q);
    //enforceccz4constraints_(Q);
  }
  logTraceOut( "adjustSolution(...)" );
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
    assertion3( std::isfinite(Q[i]), i, x, t );
  }
  memset(S, 0, NumberOfUnknowns*sizeof(double));
  //pdesource_(S,Q);    //  S(Q)
  pdesourceholger_(S,Q);    //  S(Q)
  for(int i=0; i<NumberOfUnknowns; i++){
    nonCriticalAssertion3( std::isfinite(S[i]), i, x, t );
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
    assertion4( Qinside[i]==Qinside[i], x, t, normal, i );
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
  const double test = Q[16]+Q[54];
  double alpha =  1. / std::sqrt(qmin);
  if (test>0) alpha *= std::exp(test);
#else
 const double alpha = 1.0;
#endif
  constexpr double sqrtwo = 1.4142135623730951;
  // NOTE parameters are stored in Constants.h
  const double tempA = alpha * std::max({sqrtwo, CCZ4e, CCZ4ds, CCZ4GLMc/alpha, CCZ4GLMd/alpha});
  const double tempB = Q[17+normal];//DOT_PRODUCT(Q(18:20),nv(:))
  // we are only interested in the maximum eigenvalue
  return std::max({1.0, std::abs(-tempA-tempB), std::abs(tempA-tempB)});
  
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
  logTraceInWith4Arguments( "nonconservativeProduct(...)", faceCentre, volumeH, t, normal );
  assertion( normal>=0 );
  assertion( normal<Dimensions );
  double gradQSerialised[NumberOfUnknowns*3];
  for (int i=0; i<NumberOfUnknowns; i++) {
    gradQSerialised[i+0*NumberOfUnknowns] = 0.0;
    gradQSerialised[i+1*NumberOfUnknowns] = 0.0;
    gradQSerialised[i+2*NumberOfUnknowns] = 0.0;

    gradQSerialised[i+normal*NumberOfUnknowns] = deltaQ[i];
  }
  //pdencp_(BgradQ, Q, gradQSerialised);
  pdencpholger_(BgradQ, Q, gradQSerialised, normal);

  for (int i=0; i<NumberOfUnknowns; i++) {
    nonCriticalAssertion4( std::isfinite(BgradQ[i]), i, x, t, normal );
  }
  logTraceOut( "nonconservativeProduct(...)" );
}

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
void examples::exahype2::ccz4::FiniteVolumeCCZ4::enforceccz4constraintsholger_(double * luh)
{
    double g_cov[3][3] = { {luh[0], luh[1], luh[2]}, {luh[1], luh[3], luh[4]}, {luh[2], luh[4], luh[5]} };
    const double det = luh[0]*luh[3]*luh[5] - luh[0]*luh[4]*luh[4] - luh[1]*luh[1]*luh[5] + 2*luh[1]*luh[2]*luh[4] -luh[2]*luh[2]*luh[3];

    const double phisq = 1./std::cbrt(det);
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) g_cov[i][j] *= phisq;

    const double det2 = g_cov[0][0]*g_cov[1][1]*g_cov[2][2] -
        g_cov[0][0]*g_cov[1][2]*g_cov[2][1] - g_cov[1][0]*g_cov[0][1]*g_cov[2][2] +
        g_cov[1][0]*g_cov[0][2]*g_cov[2][1] + g_cov[2][0]*g_cov[0][1]*g_cov[1][2] -
        g_cov[2][0]*g_cov[0][2]*g_cov[1][1];

    const double invdet = 1./det2;
    const double g_contr[3][3] = {
        { ( g_cov[1][1]*g_cov[2][2]-g_cov[1][2]*g_cov[2][1])*invdet, -(g_cov[0][1]*g_cov[2][2]-g_cov[0][2]*g_cov[2][1])*invdet, -(-g_cov[0][1]*g_cov[1][2]+g_cov[0][2]*g_cov[1][1])*invdet},
        {-( g_cov[1][0]*g_cov[2][2]-g_cov[1][2]*g_cov[2][0])*invdet,  (g_cov[0][0]*g_cov[2][2]-g_cov[0][2]*g_cov[2][0])*invdet, -( g_cov[0][0]*g_cov[1][2]-g_cov[0][2]*g_cov[1][0])*invdet},
        {-(-g_cov[1][0]*g_cov[2][1]+g_cov[1][1]*g_cov[2][0])*invdet, -(g_cov[0][0]*g_cov[2][1]-g_cov[0][1]*g_cov[2][0])*invdet,  ( g_cov[0][0]*g_cov[1][1]-g_cov[0][1]*g_cov[1][0])*invdet}
    };
    double Aex[3][3] = { {luh[6], luh[7], luh[8]}, {luh[7], luh[9], luh[10]}, {luh[8], luh[10], luh[11]} };

    double traceA = 0;
    for (int i=0;i<3;i++)
    for (int j=0;j<3;j++) traceA += g_contr[i][j]*Aex[i][j];

    for (int i=0;i<3;i++)
    for (int j=0;j<3;j++) Aex[i][j] -= 1./3. * traceA * g_cov[i][j];
            
    luh[ 0] = g_cov[0][0];
    luh[ 1] = g_cov[0][1];
    luh[ 2] = g_cov[0][2];
    luh[ 3] = g_cov[1][1];
    luh[ 4] = g_cov[1][2];
    luh[ 5] = g_cov[2][2];
    luh[ 6] =   Aex[0][0];
    luh[ 7] =   Aex[0][1];
    luh[ 8] =   Aex[0][2];
    luh[ 9] =   Aex[1][1];
    luh[10] =   Aex[1][2];
    luh[11] =   Aex[2][2];            
    //
    // As suggested by our PRD referee, we also enforce the algebraic constraint that results from the first spatial derivative of the constraint
    // det \tilde{\gamma}_ij = 0, which leads to
    //
    // \tilde{\gamma}^{ij} D_kij = 0
    //
    // and is thus a condition of trace-freeness on all submatrices D_kij for k=1,2,3.
    //

    double DD[3][3][3] = { 
        {{luh[35], luh[36], luh[37]}, {luh[36], luh[38], luh[39]}, {luh[37], luh[39], luh[40]}},
        {{luh[41], luh[42], luh[43]}, {luh[42], luh[44], luh[45]}, {luh[43], luh[45], luh[46]}},
        {{luh[47], luh[48], luh[49]}, {luh[48], luh[50], luh[51]}, {luh[49], luh[51], luh[52]}}
    };

    for (int l=0;l<3;l++)
    {
        double traceDk = 0;
        for (int i=0;i<3;i++)
        for (int j=0;j<3;j++) traceDk += g_contr[i][j]*DD[l][i][j];
        
        for (int i=0;i<3;i++)
        for (int j=0;j<3;j++) DD[l][i][j] -= 1./3 * g_cov[i][j]*traceDk;
    }
    
    luh[35] = DD[0][0][0];
    luh[36] = DD[0][0][1];
    luh[37] = DD[0][0][2];
    luh[38] = DD[0][2][1];
    luh[39] = DD[0][2][2];
    luh[40] = DD[0][2][2];
    luh[41] = DD[1][0][0];
    luh[42] = DD[1][0][1];
    luh[43] = DD[1][0][2];
    luh[44] = DD[1][1][1];
    luh[45] = DD[1][1][2];
    luh[46] = DD[1][2][2];
    luh[47] = DD[2][0][0];
    luh[48] = DD[2][0][1];
    luh[49] = DD[2][0][2];
    luh[50] = DD[2][1][1];
    luh[51] = DD[2][1][2];
    luh[52] = DD[2][2][2];            
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
void examples::exahype2::ccz4::FiniteVolumeCCZ4::pdesourceholger_(double* S, const double* const Q)
{
    const double alpha = std::exp(std::fmax(-20., std::fmin(20.,Q[16])));
    double fa  = 1.0;
    double faa = 0.0;
    if (CCZ4LapseType==1)
    {
      fa  =  2./alpha;
      faa = -fa/alpha;
    }

    // Note g_cov is symmetric
    const double g_cov[3][3] = { {Q[0], Q[1], Q[2]}, {Q[1], Q[3], Q[4]}, {Q[2], Q[4], Q[5]} };
    const double det = Q[0]*Q[3]*Q[5] - Q[0]*Q[4]*Q[4] - Q[1]*Q[1]*Q[5] + 2*Q[1]*Q[2]*Q[4] -Q[2]*Q[2]*Q[3];
    const double invdet = 1./det;

    const double g_contr[3][3] = {
        { ( Q[3]*Q[5]-Q[4]*Q[4])*invdet, -( Q[1]*Q[5]-Q[2]*Q[4])*invdet, -(-Q[1]*Q[4]+Q[2]*Q[3])*invdet},
        {-( Q[1]*Q[5]-Q[4]*Q[2])*invdet,  ( Q[0]*Q[5]-Q[2]*Q[2])*invdet, -( Q[0]*Q[4]-Q[2]*Q[1])*invdet},
        {-(-Q[1]*Q[4]+Q[3]*Q[2])*invdet, -( Q[0]*Q[4]-Q[1]*Q[2])*invdet,  ( Q[0]*Q[3]-Q[1]*Q[1])*invdet}
    };

    // NOTE Aex is symmetric
    double Aex[3][3] = { {Q[6], Q[7], Q[8]}, {Q[7], Q[9], Q[10]}, {Q[8], Q[10], Q[11]} };

    double traceA = 0;
    for (int i=0;i<3;i++)
    for (int j=0;j<3;j++) traceA += g_contr[i][j]*Aex[i][j];
    
    for (int i=0;i<3;i++)
    for (int j=0;j<3;j++) Aex[i][j] -= 1./3. * traceA * g_cov[i][j];

    // Matrix multiplications Amix = matmul(g_contr, Aex) Aup  = matmul(g_contr, transpose(Amix))
    double Amix[3][3]={0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int u = 0; u < 3; u++) Amix[i][j] += g_contr[i][u] * Aex[u][j];
    double Aup[3][3]={0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int u = 0; u < 3; u++) Aup[i][j] += g_contr[i][u] * Amix[j][u]; // Note the transposition is in the indices

    const double Theta = Q[12]; // needed later

    const double DD[3][3][3] = { 
        {{Q[35], Q[36], Q[37]}, {Q[36], Q[38], Q[39]}, {Q[37], Q[39], Q[40]}},
        {{Q[41], Q[42], Q[43]}, {Q[42], Q[44], Q[45]}, {Q[43], Q[45], Q[46]}},
        {{Q[47], Q[48], Q[49]}, {Q[48], Q[50], Q[51]}, {Q[49], Q[51], Q[52]}}
    };
   
    double dgup[3][3][3] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    for (int k = 0; k < 3; k++)
    for (int m = 0; m < 3; m++)
    for (int l = 0; l < 3; l++) 
    for (int n = 0; n < 3; n++) 
    for (int j = 0; j < 3; j++) dgup[k][m][l] -= 2.0*g_contr[m][n]*g_contr[j][l]*DD[k][n][j];

    const double PP[3] = {Q[55], Q[56], Q[57]};

    double Christoffel[3][3][3]       = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    double Christoffel_tilde[3][3][3] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++)
    for (int l = 0; l < 3; l++)
    {
        Christoffel_tilde[i][j][k] += g_contr[k][l] * ( DD[i][j][l] + DD[j][i][l] - DD[l][i][j] );
        Christoffel[i][j][k]       += g_contr[k][l] * ( DD[i][j][l] + DD[j][i][l] - DD[l][i][j] ) - g_contr[k][l] * ( g_cov[j][l] * PP[i] + g_cov[i][l] * PP[j] - g_cov[i][j] * PP[l] );
    }

    double Gtilde[3] = {0,0,0};
    for (int l = 0; l < 3; l++) 
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++) Gtilde[i] += g_contr[j][l] * Christoffel_tilde[j][l][i];

    const double Ghat[3] = {Q[13], Q[14], Q[15]};

    const double phi = std::exp(std::fmax(-20., std::fmin(20.,Q[54])));
    const double phi2 = phi*phi;

    double Z[3] = {0,0,0}; // Matrix vector multiplications
    for (int i=0;i<3;i++)
    for (int j=0;j<3;j++) Z[i] += 0.5*CCZ4ds*( g_cov[i][j]* (Ghat[j] - Gtilde[j]));
    double Zup[3] = {0,0,0};
    for (int i=0;i<3;i++)
    for (int j=0;j<3;j++) Zup[i] += phi2 * g_contr[i][j] * Z[j];

    double dChristoffelSrc[3][3][3][3] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    double dChristoffel_tildeSrc[3][3][3][3] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    for (int l = 0; l < 3; l++)
    for (int m = 0; m < 3; m++)
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++)
    {
        dChristoffelSrc[k][i][j][m] +=     dgup[k][m][l]*(      DD[i][j][l]+      DD[j][i][l]-DD[l][i][j])
                                      -    dgup[k][m][l]*(g_cov[j][l]*PP[i]+g_cov[i][l]*PP[j]-g_cov[i][j]*PP[l])
                                      -2.0*g_contr[m][l]*(DD[k][j][l]*PP[i]+DD[k][i][l]*PP[j]-DD[k][i][j]*PP[l]);
          
        dChristoffel_tildeSrc[k][i][j][m] += dgup[k][m][l]*(DD[i][j][l]+DD[j][i][l]-DD[l][i][j]);
    }


    double RiemannSrc[3][3][3][3] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    for (int m = 0; m < 3; m++)
    for (int j = 0; j < 3; j++)
    for (int k = 0; k < 3; k++)
    for (int i = 0; i < 3; i++)
    {
      RiemannSrc[i][k][j][m] = dChristoffelSrc[k][i][j][m] - dChristoffelSrc[j][i][k][m];
      for (int l = 0; l < 3; l++) RiemannSrc[i][k][j][m] += Christoffel[i][j][l]*Christoffel[l][k][m] - Christoffel[i][k][l]*Christoffel[l][j][m];
    }

    double RicciSrc[3][3] = {0,0,0,0,0,0,0,0,0};
    for (int l = 0; l < 3; l++)
    for (int n = 0; n < 3; n++)
    for (int m = 0; m < 3; m++) RicciSrc[m][n] += RiemannSrc[m][l][n][l];

    // Here we directly compute the derivative of Gtilde from its original definition as contracted Christoffel symbol,
    // without assuming unit determinant of the conformal metric. Back to the roots, and as few assumptions as possible...
    double dGtildeSrc[3][3] = {0,0,0,0,0,0,0,0,0};
    for (int l = 0; l < 3; l++) 
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++) dGtildeSrc[k][i] += dgup[k][j][l]*Christoffel_tilde[j][l][i] + g_contr[j][l]*dChristoffel_tildeSrc[k][j][l][i];

    double dZSrc[3][3] = {0,0,0,0,0,0,0,0,0};
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++) dZSrc[k][i] += CCZ4ds*(DD[k][i][j]*(Ghat[j]-Gtilde[j]) - 0.5*g_cov[i][j]*dGtildeSrc[k][j]);
    
    double nablaZSrc[3][3] = {0,0,0,0,0,0,0,0,0};
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    {
      nablaZSrc[i][j] = dZSrc[i][j];
      for (int k = 0; k < 3; k++) nablaZSrc[i][j] -= Christoffel[i][j][k]*Z[k];
    }
    
    double RicciPlusNablaZSrc[3][3];
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) RicciPlusNablaZSrc[i][j] = RicciSrc[i][j] + nablaZSrc[i][j] + nablaZSrc[j][i];

    double RPlusTwoNablaZSrc = 0; 
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) RPlusTwoNablaZSrc += g_contr[i][j]*RicciPlusNablaZSrc[i][j]; // TODO fuse these steps
    RPlusTwoNablaZSrc*=phi2;


    const double AA[3] = {Q[23], Q[24], Q[25]};

    double nablaijalphaSrc[3][3];
    for (int j = 0; j < 3; j++) 
    for (int i = 0; i < 3; i++)
    {
      nablaijalphaSrc[i][j] = alpha*AA[j]*AA[i]; 
      for (int k = 0; k < 3; k++) nablaijalphaSrc[i][j] -= alpha*Christoffel[i][j][k]*AA[k];
    }
    
    double nablanablaalphaSrc=0;
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) nablanablaalphaSrc += g_contr[i][j]*nablaijalphaSrc[i][j];
    nablanablaalphaSrc *= phi2;

   
    double SecondOrderTermsSrc[3][3];
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) SecondOrderTermsSrc[i][j] = -nablaijalphaSrc[i][j] + alpha*RicciPlusNablaZSrc[i][j];
    
    double traceSrc = 0;
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) traceSrc += g_contr[i][j]*SecondOrderTermsSrc[i][j];
    
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) SecondOrderTermsSrc[i][j] -= 1./3 * traceSrc * g_cov[i][j];
       
    double dtgamma[3][3];
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) dtgamma[i][j] = -2.0 * alpha * Aex[i][j] - CCZ4itau*(det -1.0)*g_cov[i][j];

    const double BB[3][3] = {
        {Q[26], Q[27], Q[28]}, {Q[29], Q[30], Q[31]}, {Q[32], Q[33], Q[34]}
    };

    double traceB = BB[0][0] + BB[1][1] + BB[2][2];
    const double beta[3] = {Q[17], Q[18], Q[19]};
    
    for (int k = 0; k < 3; k++) 
    for (int j = 0; j < 3; j++) 
    for (int i = 0; i < 3; i++) dtgamma[i][j] += g_cov[k][i]*BB[j][k] + g_cov[k][j]*BB[i][k] - 2./3. *g_cov[i][j]*BB[k][k] + 2.*beta[k]*DD[k][i][j];


    // MATMUL(Aex,Amix)
    double Atemp[3][3]={0,0,0,0,0,0,0,0,0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int u = 0; u < 3; u++) Atemp[i][j] += Aex[i][u] * Amix[u][j];

    const double traceK = Q[53];

    //! Main variables of the CCZ4 system 
    double dtK[3][3];
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) dtK[i][j] = phi2*SecondOrderTermsSrc[i][j] + alpha*Aex[i][j]*(traceK-2.*Theta) - 2.*alpha*Atemp[i][j] - CCZ4itau*g_cov[i][j]*traceA;
    
    for (int j = 0; j < 3; j++) 
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++) dtK[i][j] += Aex[k][i]*BB[j][k] + Aex[k][j]*BB[i][k] - 2./3.*Aex[i][j]*BB[k][k];
   
    const double K0 = Q[58];
    
    double dtTraceK = -nablanablaalphaSrc + alpha*(RPlusTwoNablaZSrc + traceK*traceK - 2.0*CCZ4c*Theta*traceK) -3.0*alpha*CCZ4k1*(1.+CCZ4k2)*Theta;
    double dtphi = beta[0]*PP[0] + beta[1]*PP[1] + beta[2]*PP[2] + 1./3.*(alpha*traceK-traceB);
    double dtalpha = -alpha*fa*(traceK-K0-2.*CCZ4c*Theta)+beta[0]*AA[0]+beta[1]*AA[1]+beta[2]*AA[2];


    double Aupdown = 0;
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) Aupdown += Aex[i][j]*Aup[i][j];

  
    double sumzupaa = 0.0;
    for (int i = 0; i < 3; i++) sumzupaa += Zup[i]*AA[i];
    const double dtTheta = 0.5*alpha*CCZ4e*CCZ4e*(RPlusTwoNablaZSrc - Aupdown + 2./3.*traceK*traceK) - alpha*(CCZ4c*Theta*traceK + sumzupaa + CCZ4k1*(2.+CCZ4k2)*Theta);  // Baojiu


    double dtGhat[3] = {0};
    for (int i = 0; i < 3; i++)
    {
        double temp1=0, temp2=0, temp3=0, temp4=0, temp5=0, temp6=0;
        for (int m = 0; m < 3; m++)
        {
          temp1 += Aup[i][m]*PP[m];
          temp3 += Aup[i][m]*AA[m];
          temp2 += g_contr[m][i]*(-Theta*AA[m] -2./3.*traceK*Z[m]);
          temp4 += g_contr[i][m]*Z[m];
          temp5 += Gtilde[m]*BB[m][i];
          for (int n = 0; n < 3; n++) temp6  += Christoffel_tilde[m][n][i]*Aup[m][n];
        }
        dtGhat[i] += 2.*alpha*(temp6 - 3.*temp1 + temp2 - temp3 - CCZ4k1*temp4) - temp5 + 2./3.*Gtilde[i]*traceB;

        for (int l = 0; l < 3; l++)
        for (int k = 0; k < 3; k++) dtGhat[i] += 2.*CCZ4k3*(2./3.*g_contr[i][l]*Z[l]*BB[k][k] - g_contr[l][k]*Z[l]*BB[k][i]);
    }

    double ov[3];
    for (int k = 0; k < 3; k++)
    {
        double temp=0;
        for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) temp += dgup[k][i][j]*Aex[i][j];
        ov[k] = 2*alpha*temp;
    }

    // matrix vector multiplication in a loop and add result to existing vector
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) dtGhat[i] += CCZ4sk*g_contr[i][j]*ov[j];
    
    const double myb[3] = {Q[20], Q[21], Q[22]};
    
    double dtbb[3];
    for (int i = 0; i < 3; i++) dtbb[i] = CCZ4sk*(CCZ4xi*dtGhat[i] - CCZ4eta*myb[i]);

    double dtbeta[3];
    for (int i = 0; i < 3; i++) dtbeta[i] = CCZ4f*myb[i];
    for (int i = 0; i < 3; i++) dtbeta[i] += CCZ4bs*(beta[0]*BB[0][i] + beta[1]*BB[1][i] + beta[2]*BB[2][i]);
    for (int i = 0; i < 3; i++) dtbeta[i] *= CCZ4sk;


    // Auxiliary variables 
    double dtA[3];
    for (int i = 0; i < 3; i++)
    {
      dtA[i] = -alpha*AA[i]*(fa+alpha*faa)*(traceK - K0 - 2.*CCZ4c*Theta);
      for (int j = 0; j < 3; j++) dtA[i] += BB[i][j]*AA[j];
    }

    for (int k = 0; k < 3; k++)
    {
      double temp = 0;
      for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++) temp+= dgup[k][i][j]*Aex[i][j];
      dtA[k] += -CCZ4sk*alpha*fa*temp;
    }

    double dtB[3][3] ={0,0,0,0,0,0,0,0,0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int u = 0; u < 3; u++) dtB[i][j] += CCZ4sk*(BB[i][u] * BB[u][j]);

    double dtD[3][3][3] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    for (int m = 0; m < 3; m++)
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++)
    for (int n = 0; n < 3; n++) dtD[k][i][j] += 1./3*alpha*g_cov[i][j]*dgup[k][n][m]*Aex[n][m]; // explicitly remove the trace of tilde A again  

    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++)
    {
      dtD[k][i][j] -= alpha*AA[k]*Aex[i][j];
      for (int l = 0; l < 3; l++) dtD[k][i][j] += BB[k][l]*DD[l][i][j] + BB[j][l]*DD[k][l][i] + BB[i][l]*DD[k][l][j] - 2./3.*BB[l][l]*DD[k][i][j];
    }

    double dtP[3] = {0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) dtP[i] += BB[i][j]*PP[j];

    for (int k = 0; k < 3; k++)
    {
      double temp=0;
      for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++) temp += dgup[k][i][j]*Aex[i][j];
      dtP[k] += 1./3.*alpha*(AA[k]*traceK + CCZ4sk*temp);
    }


    S[0]  = dtgamma[0][0];
    S[1]  = dtgamma[0][1];
    S[2]  = dtgamma[0][2];
    S[3]  = dtgamma[1][1];
    S[4]  = dtgamma[1][2];
    S[5]  = dtgamma[2][2];
    S[6]  = dtK[0][0];
    S[7]  = dtK[0][1];
    S[8]  = dtK[0][2];
    S[9]  = dtK[1][1];
    S[10] = dtK[1][2];
    S[11] = dtK[2][2]; 
    S[12] = dtTheta;   
    for (int i = 0; i < 3; i++) S[13+i] = dtGhat[i];
    S[16] = dtalpha;
    for (int i = 0; i < 3; i++) S[17+i] = dtbeta[i];
    for (int i = 0; i < 3; i++) S[20+i] = dtbb[i];
    for (int i = 0; i < 3; i++) S[23+i] = dtA[i];
    S[26] = dtB[0][0];
    S[27] = dtB[1][0];
    S[28] = dtB[2][0];
    S[29] = dtB[0][1];
    S[30] = dtB[1][1];
    S[31] = dtB[2][1];
    S[32] = dtB[0][2];
    S[33] = dtB[1][2];
    S[34] = dtB[2][2];
    S[35] = dtD[0][0][0];
    S[36] = dtD[0][0][1];
    S[37] = dtD[0][0][2];
    S[38] = dtD[0][1][1];
    S[39] = dtD[0][1][2];
    S[40] = dtD[0][2][2];
    S[41] = dtD[1][0][0];
    S[42] = dtD[1][0][1];
    S[43] = dtD[1][0][2];
    S[44] = dtD[1][1][1];
    S[45] = dtD[1][1][2];
    S[46] = dtD[1][2][2];
    S[47] = dtD[2][0][0];
    S[48] = dtD[2][0][1];
    S[49] = dtD[2][0][2];
    S[50] = dtD[2][1][1];
    S[51] = dtD[2][1][2];
    S[52] = dtD[2][2][2];
    S[53] = dtTraceK;
    S[54] = dtphi;
    for (int i = 0; i < 3; i++) S[55+i] = dtP[i];
    S[58] = 0;
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
void examples::exahype2::ccz4::FiniteVolumeCCZ4::pdencpholger_(double* BgradQ, const double* const Q, const double* const gradQSerialised, const int normal)
{
    const double alpha = std::exp(std::fmax(-20., std::fmin(20.,Q[16])));
    const double alpha2 = alpha*alpha;
    double fa  = 1.0;
    double faa = 0.0;
    if (CCZ4LapseType==1)
    {
      fa  =  2./alpha;
      faa = -fa/alpha;
    }

    constexpr int nVar(59);

    // First model parameter ds here (ds == CCZ4ds)
    double gradQin[59][3] ={0};

    // De-serialise input data and fill static array
    // FIXME the use of 2D arrays can be avoided: all terms not in the normal are 0
    for (int i=0; i<nVar; i++) gradQin[i][normal] = gradQSerialised[i+normal*nVar];


    // Note g_cov is symmetric
    const double g_cov[3][3] = { {Q[0], Q[1], Q[2]}, {Q[1], Q[3], Q[4]}, {Q[2], Q[4], Q[5]} };
    const double invdet = 1./( Q[0]*Q[3]*Q[5] - Q[0]*Q[4]*Q[4] - Q[1]*Q[1]*Q[5] + 2*Q[1]*Q[2]*Q[4] -Q[2]*Q[2]*Q[3]);

    const double g_contr[3][3] = {
        { ( Q[3]*Q[5]-Q[4]*Q[4])*invdet, -( Q[1]*Q[5]-Q[2]*Q[4])*invdet, -(-Q[1]*Q[4]+Q[2]*Q[3])*invdet},
        {-( Q[1]*Q[5]-Q[4]*Q[2])*invdet,  ( Q[0]*Q[5]-Q[2]*Q[2])*invdet, -( Q[0]*Q[4]-Q[2]*Q[1])*invdet},
        {-(-Q[1]*Q[4]+Q[3]*Q[2])*invdet, -( Q[0]*Q[4]-Q[1]*Q[2])*invdet,  ( Q[0]*Q[3]-Q[1]*Q[1])*invdet}
    };


    // NOTE Aex is symmetric
    double Aex[3][3] = { {Q[6], Q[7], Q[8]}, {Q[7], Q[9], Q[10]}, {Q[8], Q[10], Q[11]} };

    double traceA = 0;
    for (int i=0;i<3;i++)
    for (int j=0;j<3;j++) traceA+=g_contr[i][j]*Aex[i][j];
    traceA *= 1./3;
    
    for (int i=0;i<3;i++)
    for (int j=0;j<3;j++) Aex[i][j] -= traceA * g_cov[i][j];

    // Matrix multiplications Amix = matmul(g_contr, Aex) Aup  = matmul(g_contr, mytranspose(Amix))
    double Amix[3][3]={0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int u = 0; u < 3; u++) Amix[i][j] += g_contr[i][u] * Aex[u][j];
    double Aup[3][3]={0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int u = 0; u < 3; u++) Aup[i][j] += g_contr[i][u] * Amix[j][u]; // Note the transposition is in the indices

    const double DD[3][3][3] = { 
        {{Q[35], Q[36], Q[37]}, {Q[36], Q[38], Q[39]}, {Q[37], Q[39], Q[40]}},
        {{Q[41], Q[42], Q[43]}, {Q[42], Q[44], Q[45]}, {Q[43], Q[45], Q[46]}},
        {{Q[47], Q[48], Q[49]}, {Q[48], Q[50], Q[51]}, {Q[49], Q[51], Q[52]}}
    };
    double dgup[3][3][3] = {0};
    for (int k = 0; k < 3; k++)
    for (int m = 0; m < 3; m++)
    for (int l = 0; l < 3; l++) 
    for (int n = 0; n < 3; n++) 
    for (int j = 0; j < 3; j++) dgup[k][m][l] -= g_contr[m][n]*g_contr[j][l]*2*DD[k][n][j];

    const double PP[3] = {Q[55], Q[56], Q[57]};

    double Christoffel[3][3][3]       = {0};
    double Christoffel_tilde[3][3][3] = {0};
    double Christoffel_kind1[3][3][3] = {0};
    
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++)
    {
        Christoffel_kind1[i][j][k] = DD[k][i][j] + DD[j][i][k] - DD[i][j][k];
        for (int l = 0; l < 3; l++)
        {
            Christoffel_tilde[i][j][k] += g_contr[k][l] * ( DD[i][j][l] + DD[j][i][l] - DD[l][i][j] );
            Christoffel[i][j][k]       += g_contr[k][l] * ( DD[i][j][l] + DD[j][i][l] - DD[l][i][j] ) - g_contr[k][l] * ( g_cov[j][l] * PP[i] + g_cov[i][l] * PP[j] - g_cov[i][j] * PP[l] );
        }
    }

    double Gtilde[3] = {0};
    for (int l = 0; l < 3; l++) 
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++) Gtilde[i] += g_contr[j][l] * Christoffel_tilde[j][l][i];

    const double Ghat[3] = {Q[13], Q[14], Q[15]};

    const double phi = std::exp(std::fmax(-20., std::fmin(20.,Q[54])));
    const double phi2 = phi*phi;

    double Z[3] = {0};
    for (int i=0;i<3;i++)
    for (int j=0;j<3;j++) Z[i] += ( g_cov[i][j]* (Ghat[j] - Gtilde[j]));
    double Zup[3] = {0};
    for (int i=0;i<3;i++)
    for (int j=0;j<3;j++) Zup[i] += phi2 * g_contr[i][j] * Z[j];


    const double dDD[3][3][3][3] = { 
        {
            {
                {gradQin[35][0],gradQin[36][0],gradQin[37][0]}, {gradQin[36][0],gradQin[38][0],gradQin[39][0]}, {gradQin[37][0],gradQin[39][0],gradQin[40][0]},
            },
            {
                {gradQin[41][0],gradQin[42][0],gradQin[43][0]}, {gradQin[42][0],gradQin[44][0],gradQin[45][0]}, {gradQin[43][0],gradQin[45][0],gradQin[46][0]},
            },
            {
                {gradQin[47][0],gradQin[48][0],gradQin[49][0]}, {gradQin[48][0],gradQin[50][0],gradQin[51][0]}, {gradQin[49][0],gradQin[51][0],gradQin[52][0]}
            }
        },
        {
            {
                {gradQin[35][1],gradQin[36][1],gradQin[37][1]}, {gradQin[36][1],gradQin[38][1],gradQin[39][1]}, {gradQin[37][1],gradQin[39][1],gradQin[40][1]},
            },
            {
                {gradQin[41][1],gradQin[42][1],gradQin[43][1]}, {gradQin[42][1],gradQin[44][1],gradQin[45][1]}, {gradQin[43][1],gradQin[45][1],gradQin[46][1]},
            },
            {
                {gradQin[47][1],gradQin[48][1],gradQin[49][1]}, {gradQin[48][1],gradQin[50][1],gradQin[51][1]}, {gradQin[49][1],gradQin[51][1],gradQin[52][1]}
            }
        },
        {
            {
                {gradQin[35][2],gradQin[36][2],gradQin[37][2]}, {gradQin[36][2],gradQin[38][2],gradQin[39][2]}, {gradQin[37][2],gradQin[39][2],gradQin[40][2]},
            },
            {
                {gradQin[41][2],gradQin[42][2],gradQin[43][2]}, {gradQin[42][2],gradQin[44][2],gradQin[45][2]}, {gradQin[43][2],gradQin[45][2],gradQin[46][2]},
            },
            {
                {gradQin[47][2],gradQin[48][2],gradQin[49][2]}, {gradQin[48][2],gradQin[50][2],gradQin[51][2]}, {gradQin[49][2],gradQin[51][2],gradQin[52][2]}
            }
        }
    };


    const double dPP[3][3] = {
        {gradQin[55][0],gradQin[56][0],gradQin[57][0]},
        {gradQin[55][1],gradQin[56][1],gradQin[57][1]},
        {gradQin[55][2],gradQin[56][2],gradQin[57][2]}
    };



    double dChristoffelNCP[3][3][3][3] = {0};
    double dChristoffel_tildeNCP[3][3][3][3] = {0};
    for (int i = 0; i < 3; i++)
    for (int ip = 0; ip < 3; ip++)
    for (int m = 0; m < 3; m++)
    for (int k = 0; k < 3; k++)
    {
        dChristoffelNCP[k][i][ip][m] = 0;
        dChristoffel_tildeNCP[k][i][ip][m] = 0;
        for (int l = 0; l < 3; l++)
        {
            dChristoffelNCP[k][i][ip][m] +=  0.5*g_contr[m][l] * ( 
                    dDD[k][i][ip][l] + dDD[i][k][ip][l] + dDD[k][ip][i][l] + dDD[ip][k][i][l] - dDD[k][l][i][ip] + dDD[l][k][i][ip] 
                    - g_cov[ip][l]*(dPP[k][i] + dPP[i][k]) - g_cov[i][l]*(dPP[k][ip]+dPP[ip][k]) +  g_cov[i][ip]*(dPP[k][l]+dPP[l][k]) );



            dChristoffel_tildeNCP[k][i][ip][m] += 0.5*g_contr[m][l]*(dDD[k][i][ip][l] + dDD[i][k][ip][l] + dDD[k][ip][i][l] + dDD[ip][k][i][l] - dDD[k][l][i][ip] - dDD[l][k][i][ip]);
            
        }
    }


    double RiemannNCP[3][3][3][3] = {0};
    for (int i = 0; i < 3; i++)
    for (int ip = 0; ip < 3; ip++)
    for (int m = 0; m < 3; m++)
    for (int k = 0; k < 3; k++) RiemannNCP[i][k][ip][m] = dChristoffelNCP[k][i][ip][m] - dChristoffelNCP[ip][i][k][m];

    double RicciNCP[3][3] = {0};
    for (int m = 0; m < 3; m++)
    for (int n = 0; n < 3; n++)
    for (int l = 0; l < 3; l++) RicciNCP[m][n] += RiemannNCP[m][l][n][l];  

    double dGtildeNCP[3][3] = {0};
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++)
    for (int j = 0; j < 3; j++)
    for (int l = 0; l < 3; l++) dGtildeNCP[k][i] += g_contr[j][l]*dChristoffel_tildeNCP[k][j][l][i];

    
    const double dGhat[3][3] = {
        {gradQin[13][0],gradQin[14][0],gradQin[15][0]},
        {gradQin[13][1],gradQin[14][1],gradQin[15][1]},
        {gradQin[13][2],gradQin[14][2],gradQin[15][2]}
    };
    
    double dZNCP[3][3] = {0};
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++) dZNCP[k][i] += CCZ4ds*0.5*g_cov[i][j]*(dGhat[k][j]-dGtildeNCP[k][j]);  
    

    double RicciPlusNablaZNCP[3][3];
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) RicciPlusNablaZNCP[i][j] = RicciNCP[i][j] + dZNCP[i][j] + dZNCP[j][i];

    double RPlusTwoNablaZNCP = 0; 
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) RPlusTwoNablaZNCP += g_contr[i][j]*RicciPlusNablaZNCP[i][j]; // TODO fuse these steps
    RPlusTwoNablaZNCP*=phi2;


    const double AA[3] = {Q[23], Q[24], Q[25]};
    const double dAA[3][3] = {
        {gradQin[23][0],gradQin[24][0],gradQin[25][0]},
        {gradQin[23][1],gradQin[24][1],gradQin[25][1]},
        {gradQin[23][2],gradQin[24][2],gradQin[25][2]}
    };

    double nablaijalphaNCP[3][3];
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) nablaijalphaNCP[i][j] = alpha*0.5*( dAA[i][j] + dAA[j][i] ); 
    
    double nablanablaalphaNCP = 0;
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) nablanablaalphaNCP += g_contr[i][j]*nablaijalphaNCP[i][j]; 
    nablanablaalphaNCP*=phi2;
   
    double SecondOrderTermsNCP[3][3];
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) SecondOrderTermsNCP[i][j] = -nablaijalphaNCP[i][j] + alpha*RicciPlusNablaZNCP[i][j];
    
    double traceNCP = 0;
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) traceNCP += g_contr[i][j]*SecondOrderTermsNCP[i][j];
    
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) SecondOrderTermsNCP[i][j] -= 1./3 * traceNCP * g_cov[i][j];
        
    const double beta[3] = {Q[17], Q[18], Q[19]};

    const double dAex[3][3][3] = { 
        {{gradQin[6][0],gradQin[7][0],gradQin[8][0]}, {gradQin[7][0], gradQin[9][0], gradQin[10][0]},  {gradQin[8][0], gradQin[10][0], gradQin[11][0]}},
        {{gradQin[6][1],gradQin[7][1],gradQin[8][1]}, {gradQin[7][1], gradQin[9][1], gradQin[10][1]},  {gradQin[8][1], gradQin[10][1], gradQin[11][1]}},
        {{gradQin[6][2],gradQin[7][2],gradQin[8][2]}, {gradQin[7][2], gradQin[9][2], gradQin[10][2]},  {gradQin[8][2], gradQin[10][2], gradQin[11][2]}}
    };
    //! Now assemble all this terrible stuff... 
    //!
    //! Main variables of the CCZ4 system 
    double dtK[3][3];
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)  dtK[i][j] = phi2*SecondOrderTermsNCP[i][j] + beta[0] * dAex[0][i][j] + beta[1] * dAex[1][i][j] + beta[2] * dAex[2][i][j]; // extrinsic curvature
   
    const double dtraceK[3] = {gradQin[53][0], gradQin[53][1], gradQin[53][1]};

    double dtTraceK = -nablanablaalphaNCP + alpha*RPlusTwoNablaZNCP + beta[0]*dtraceK[0] + beta[1]*dtraceK[1] + beta[2]*dtraceK[2];

    const double BB[3][3] = {
        {Q[26], Q[27], Q[28]}, {Q[29], Q[30], Q[31]}, {Q[32], Q[33], Q[34]}
    };

    double traceB = BB[0][0] + BB[1][1] + BB[2][2]; // TODO direct from Q! 

    double Aupdown = 0;
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) Aupdown += Aex[i][j]*Aup[i][j];  

    const double dTheta[3] = {gradQin[13][0],gradQin[13][1],gradQin[13][2]};
    const double dtTheta = 0.5*alpha*CCZ4e*CCZ4e*( RPlusTwoNablaZNCP ) + beta[0]*dTheta[0] + beta[1]*dTheta[1] + beta[2]*dTheta[2]; // *** original cleaning *** 

    double divAupNCP[3] = {0};

    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int l = 0; l < 3; l++)
    for (int k = 0; k < 3; k++) divAupNCP[i] += g_contr[i][l]*g_contr[j][k]*dAex[j][l][k];


    const double dBB[3][3][3] = {
        {
            {CCZ4sk*gradQin[26][0],CCZ4sk*gradQin[27][0],CCZ4sk*gradQin[28][0]}, {CCZ4sk*gradQin[29][0],CCZ4sk*gradQin[30][0],CCZ4sk*gradQin[31][0]}, {CCZ4sk*gradQin[32][0],CCZ4sk*gradQin[33][0],CCZ4sk*gradQin[34][0]}
        },
        {
            {CCZ4sk*gradQin[26][1],CCZ4sk*gradQin[27][1],CCZ4sk*gradQin[28][1]}, {CCZ4sk*gradQin[29][1],CCZ4sk*gradQin[30][1],CCZ4sk*gradQin[31][1]}, {CCZ4sk*gradQin[32][1],CCZ4sk*gradQin[33][1],CCZ4sk*gradQin[34][1]}
        },
        {
            {CCZ4sk*gradQin[26][2],CCZ4sk*gradQin[27][2],CCZ4sk*gradQin[28][2]}, {CCZ4sk*gradQin[29][2],CCZ4sk*gradQin[30][2],CCZ4sk*gradQin[31][2]}, {CCZ4sk*gradQin[32][2],CCZ4sk*gradQin[33][2],CCZ4sk*gradQin[34][2]}
        }
    };

    double dtGhat[3];
    for (int i = 0; i < 3; i++)
    {
        double temp=0, temp2=0;
        for (int j = 0; j < 3; j++)
        {
            temp  +=g_contr[i][j]*dtraceK[j];
            temp2 +=g_contr[j][i]*dTheta[j];
        }
        dtGhat[i] = -4./3.*alpha*temp + 2*alpha*temp2 + beta[0]*dGhat[0][i] + beta[1]*dGhat[1][i] + beta[2]*dGhat[2][i];
        for (int l = 0; l < 3; l++)
        for (int k = 0; k < 3; k++) dtGhat[i] += g_contr[k][l]*0.5*(dBB[k][l][i] + dBB[l][k][i]) + 1./3.*g_contr[i][k]*0.5*(dBB[k][l][l] + dBB[l][k][l]);
    }

    double ov[3];
    for (int k = 0; k < 3; k++)
    {
        double temp=0;
        for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) temp += g_contr[i][j]*dAex[k][i][j];
        ov[k] = 2*alpha*temp;
    }
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) dtGhat[i] += CCZ4sk*g_contr[i][j]*ov[j];
    
    double dtbb[3];
    for (int i = 0; i < 3; i++)
    {
        dtbb[i] = CCZ4xi*dtGhat[i] + CCZ4bs * ( beta[0]*gradQin[20+i][0] + beta[1]*gradQin[20+i][1] + beta[2]*gradQin[20+i][2] - beta[0]*gradQin[13+i][0] - beta[1]*gradQin[13+i][1] - beta[2]*gradQin[13+i][2]);
        dtbb[i] *= CCZ4sk;
    }

    // Auxiliary variables 
    double dtA[3];
    double dK0[3] = {0}; // FIXME we just add 0 all the time???
    for (int i = 0; i < 3; i++)
    {
        dtA[i] = -alpha*fa*(dtraceK[i] - dK0[i] - CCZ4c*2*dTheta[i]) + beta[0]*dAA[0][i] + beta[1]*dAA[1][i] + beta[2]*dAA[2][i];
    }

    for (int k = 0; k < 3; k++)
    {
        double temp=0;
        for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) temp += g_contr[i][j]*dAex[k][i][j]; // TODO we computed this quantity  few lines earlier alrady
        dtA[k] -= CCZ4sk*alpha*fa*temp;
    }

    double dtB[3][3] = {
        {CCZ4f*gradQin[20][0],CCZ4f*gradQin[21][0],CCZ4f*gradQin[22][0]},
        {CCZ4f*gradQin[20][1],CCZ4f*gradQin[21][1],CCZ4f*gradQin[22][1]},
        {CCZ4f*gradQin[20][2],CCZ4f*gradQin[21][2],CCZ4f*gradQin[22][2]}
    };


    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++)
    for (int j = 0; j < 3; j++)
    {
        dtB[k][i] += CCZ4mu*alpha2 * g_contr[i][j]*( dPP[k][j] - dPP[j][k]);
        for (int n = 0; n < 3; n++)
        for (int l = 0; l < 3; l++) dtB[k][i] -= CCZ4mu*alpha2 * g_contr[i][j]*g_contr[n][l]*( dDD[k][l][j][n] - dDD[l][k][j][n]);
    }

    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) dtB[i][j] += CCZ4bs*(beta[0]*dBB[0][i][j] + beta[1]*dBB[1][i][j] + beta[2]*dBB[2][i][j]);

    // NOTE 0 value param
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) dtB[i][j] *= CCZ4sk;

    double dtD[3][3][3];
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int k = 0; k < 3; k++) dtD[i][j][k] = -alpha*dAex[i][j][k];
    
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int k = 0; k < 3; k++)
    for (int m = 0; m < 3; m++)
    {
        dtD[k][i][j] += ( 0.25*(g_cov[m][i]*(dBB[k][j][m] + dBB[j][k][m]) + g_cov[m][j]*(dBB[k][i][m]+dBB[i][k][m])) - 1./6.*g_cov[i][j]*(dBB[k][m][m]+dBB[m][k][m]) );
        for (int n = 0; n < 3; n++)
            dtD[k][i][j] += 1./3*alpha*g_cov[i][j]*g_contr[n][m]*dAex[k][n][m]; // explicitly remove the trace of tilde A again  
    }

    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int k = 0; k < 3; k++) dtD[i][j][k] += beta[0]*dDD[0][i][j][k] + beta[1]*dDD[1][i][j][k] + beta[2]*dDD[2][i][j][k];
   
    double dtP[3];
    for (int i = 0; i < 3; i++) dtP[i] = beta[0]*dPP[0][i] + beta[1]*dPP[1][i] + beta[2]*dPP[2][i];

    // NOTE test for non zero params
    for (int k = 0; k < 3; k++)
    {
        double temp=0;
        for (int m = 0; m < 3; m++)
        for (int n = 0; n < 3; n++) temp += g_contr[m][n]*dAex[k][m][n]; // TODO we computed this quantity  few lines earlier alrady
        dtP[k] += 1./3*alpha*(dtraceK[k] + CCZ4sk*temp);
        for (int i = 0; i < 3; i++) dtP[k] -= 1./6*(dBB[k][i][i] + dBB[i][k][i]);
    }

    double dtgamma[3][3] = {0};
    double dtalpha = 0;
    double dtbeta[3] = {0};
    double dtphi = 0;

    BgradQ[0]  = -dtgamma[0][0];
    BgradQ[1]  = -dtgamma[0][1];
    BgradQ[2]  = -dtgamma[0][2];
    BgradQ[3]  = -dtgamma[1][1];
    BgradQ[4]  = -dtgamma[1][2];
    BgradQ[5]  = -dtgamma[2][2];
    BgradQ[6]  = -dtK[0][0];
    BgradQ[7]  = -dtK[0][1];
    BgradQ[8]  = -dtK[0][2];
    BgradQ[9]  = -dtK[1][1];
    BgradQ[10] = -dtK[1][2];
    BgradQ[11] = -dtK[2][2]; // ok
    BgradQ[12] = -dtTheta;   // ok
    for (int i = 0; i < 3; i++) BgradQ[13+i] = -dtGhat[i]; // buggy
    BgradQ[16] = -dtalpha;
    for (int i = 0; i < 3; i++) BgradQ[17+i] = -dtbeta[i];
    for (int i = 0; i < 3; i++) BgradQ[20+i] = -dtbb[i];
    for (int i = 0; i < 3; i++) BgradQ[23+i] = -dtA[i];
    BgradQ[26] = -dtB[0][0]; // note: thes are all 0 for default CCZ4sk=0
    BgradQ[27] = -dtB[1][0];
    BgradQ[28] = -dtB[2][0];
    BgradQ[29] = -dtB[0][1];
    BgradQ[30] = -dtB[1][1];
    BgradQ[31] = -dtB[2][1];
    BgradQ[32] = -dtB[0][2];
    BgradQ[33] = -dtB[1][2];
    BgradQ[34] = -dtB[2][2];
    BgradQ[35] = -dtD[0][0][0];
    BgradQ[36] = -dtD[0][0][1];
    BgradQ[37] = -dtD[0][0][2];
    BgradQ[38] = -dtD[0][1][1];
    BgradQ[39] = -dtD[0][1][2];
    BgradQ[40] = -dtD[0][2][2];
    BgradQ[41] = -dtD[1][0][0];
    BgradQ[42] = -dtD[1][0][1];
    BgradQ[43] = -dtD[1][0][2];
    BgradQ[44] = -dtD[1][1][1];
    BgradQ[45] = -dtD[1][1][2];
    BgradQ[46] = -dtD[1][2][2];
    BgradQ[47] = -dtD[2][0][0];
    BgradQ[48] = -dtD[2][0][1];
    BgradQ[49] = -dtD[2][0][2];
    BgradQ[50] = -dtD[2][1][1];
    BgradQ[51] = -dtD[2][1][2];
    BgradQ[52] = -dtD[2][2][2];
    BgradQ[53] = -dtTraceK;
    BgradQ[54] = -dtphi;
    for (int i = 0; i < 3; i++) BgradQ[55+i] = -dtP[i];
    BgradQ[58] = 0;
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif
