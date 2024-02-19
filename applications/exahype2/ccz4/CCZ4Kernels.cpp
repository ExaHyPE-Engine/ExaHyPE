#include "tarch/multicore/multicore.h"
#include "Constants.h"

#include <limits>
#include <stdio.h>
#include <string.h>
#include <iostream>

#include "CCZ4Kernels.h"


void applications::exahype2::ccz4::enforceCCZ4constraints(
  const double* __restrict__  oldQ,
  double* __restrict__        dQdt,
  double                      timeStepSize
) {
  double newQ[59];
  for (int i=0; i<59; i++) {
    newQ[i] = oldQ[i] + timeStepSize * dQdt[i];
  }
  enforceCCZ4constraints(newQ);
  for (int i=0; i<59; i++) {
    dQdt[i] = (newQ[i] - oldQ[i]) / timeStepSize;
  }
}


#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
void applications::exahype2::ccz4::enforceCCZ4constraints(double * luh)
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

    //new constraints: a minimum for phi(chi) and alpha
    luh[16]=std::fmax(1e-16,luh[16]);
    luh[54]=std::fmax(1e-16,luh[54]);

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
    luh[38] = DD[0][1][1];
    luh[39] = DD[0][1][2];
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
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
void applications::exahype2::ccz4::admconstraints(double* constraints, const double* const Q, const double* const gradQSerialised)
{
    constexpr int nVar(59);
    double gradQin[59][3] ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    // De-serialise input data and fill static array
    for (int normal=0; normal<3; normal++)
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

    const double dAex[3][3][3] = {
        {{gradQin[6][0],gradQin[7][0],gradQin[8][0]}, {gradQin[7][0], gradQin[9][0], gradQin[10][0]},  {gradQin[8][0], gradQin[10][0], gradQin[11][0]}},
        {{gradQin[6][1],gradQin[7][1],gradQin[8][1]}, {gradQin[7][1], gradQin[9][1], gradQin[10][1]},  {gradQin[8][1], gradQin[10][1], gradQin[11][1]}},
        {{gradQin[6][2],gradQin[7][2],gradQin[8][2]}, {gradQin[7][2], gradQin[9][2], gradQin[10][2]},  {gradQin[8][2], gradQin[10][2], gradQin[11][2]}}
    };

    const double traceK = Q[53];
    const double dtraceK[3] = {gradQin[53][0], gradQin[53][1], gradQin[53][2]};

    const double phi = std::fmax(1e-2,Q[54]); 
    const double phi2 = phi*phi;
    const double PP[3] = {Q[55], Q[56], Q[57]};

    const double dPP[3][3] = {
        {gradQin[55][0],gradQin[56][0],gradQin[57][0]},
        {gradQin[55][1],gradQin[56][1],gradQin[57][1]},
        {gradQin[55][2],gradQin[56][2],gradQin[57][2]}
    };

    const double DD[3][3][3] = {
        {{Q[35], Q[36], Q[37]}, {Q[36], Q[38], Q[39]}, {Q[37], Q[39], Q[40]}},
        {{Q[41], Q[42], Q[43]}, {Q[42], Q[44], Q[45]}, {Q[43], Q[45], Q[46]}},
        {{Q[47], Q[48], Q[49]}, {Q[48], Q[50], Q[51]}, {Q[49], Q[51], Q[52]}}
    };
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

    double dgup[3][3][3] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    for (int k = 0; k < 3; k++)
    for (int m = 0; m < 3; m++)
    for (int l = 0; l < 3; l++)
    for (int n = 0; n < 3; n++)
    for (int j = 0; j < 3; j++) dgup[k][m][l] -= g_contr[m][n]*g_contr[j][l]*2*DD[k][n][j];

    double Kex[3][3]={ 0 };
    for (int i=0;i<3;i++)
    for (int j=0;j<3;j++) Kex[i][j]=Aex[i][j]/phi2 + (1./3)*traceK*g_cov[i][j]/phi2;
    double Kmix[3][3]={0,0,0,0,0,0,0,0,0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int u = 0; u < 3; u++) Kmix[i][j] += phi2*g_contr[i][u] * Kex[u][j];
    double Kup[3][3]={0,0,0,0,0,0,0,0,0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int u = 0; u < 3; u++) Kup[i][j] += phi2*g_contr[i][u] * Kmix[j][u]; // Note the transposition is in the indices

    double Christoffel[3][3][3]       = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    double Christoffel_tilde[3][3][3]       = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++)
    for (int l = 0; l < 3; l++) {
        Christoffel_tilde[i][j][k] += g_contr[k][l] * ( DD[i][j][l] + DD[j][i][l] - DD[l][i][j] );
        Christoffel[i][j][k]  += g_contr[k][l] * ( DD[i][j][l] + DD[j][i][l] - DD[l][i][j] ) - g_contr[k][l]/phi * ( g_cov[j][l] * PP[i] + g_cov[i][l] * PP[j] - g_cov[i][j] * PP[l] );
    }

    double dChristoffel[3][3][3][3] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    double dChristoffel_tilde[3][3][3][3] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
 
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int m = 0; m < 3; m++)
    for (int k = 0; k < 3; k++)
    for (int l = 0; l < 3; l++)
    {
        dChristoffel[k][i][j][m] += 0.5*g_contr[m][l] * ( //npc part
            dDD[k][i][j][l] + dDD[i][k][j][l] + dDD[k][j][i][l] + dDD[j][k][i][l] - dDD[k][l][i][j] - dDD[l][k][i][j]
            - g_cov[j][l]*(dPP[k][i] + dPP[i][k])/phi - g_cov[i][l]*(dPP[k][j]+dPP[j][k])/phi +  g_cov[i][j]*(dPP[k][l]+dPP[l][k])/phi )
            +dgup[k][m][l]*(DD[i][j][l]+DD[j][i][l]-DD[l][i][j])  //src part
            -dgup[k][m][l]*(g_cov[j][l]*PP[i]+g_cov[i][l]*PP[j]-g_cov[i][j]*PP[l])/phi
            -2*g_contr[m][l]*(DD[k][j][l]*PP[i]+DD[k][i][l]*PP[j]-DD[k][i][j]*PP[l])/phi
            +g_contr[m][l]*(g_cov[j][l]*PP[i]*PP[k]+g_cov[i][l]*PP[j]*PP[k]-g_cov[i][j]*PP[k]*PP[l])/phi2;

        dChristoffel_tilde[k][i][j][m] += dgup[k][m][l]*(DD[i][j][l]+DD[j][i][l]-DD[l][i][j])
            +0.5*g_contr[m][l]*(dDD[k][i][j][l] + dDD[i][k][j][l] + dDD[k][j][i][l] + dDD[j][k][i][l] - dDD[k][l][i][j] - dDD[l][k][i][j]);
    }

    double Riemann[3][3][3][3] = {0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int m = 0; m < 3; m++)
    for (int k = 0; k < 3; k++){
        Riemann[i][k][j][m] = dChristoffel[k][i][j][m] - dChristoffel[j][i][k][m];
        for (int l = 0; l < 3; l++){
            Riemann[i][k][j][m] += Christoffel[i][j][l]*Christoffel[l][k][m]-Christoffel[i][k][l]*Christoffel[l][j][m];
        }
    }

    double Ricci[3][3] = {0,0,0,0,0,0,0,0,0};
    for (int m = 0; m < 3; m++)
    for (int n = 0; n < 3; n++)
    for (int l = 0; l < 3; l++) Ricci[m][n] += Riemann[m][l][n][l];

    double R=0;
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) R += phi2*g_contr[i][j]*Ricci[i][j];
    double Kupdown=0;
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) Kupdown += Kex[i][j]*Kup[i][j];

// following is added to calculate nabla_i Z^i
    double Gtilde[3] = {0,0,0};
    for (int l = 0; l < 3; l++)
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++) Gtilde[i] += g_contr[j][l] * Christoffel_tilde[j][l][i];

    const double Ghat[3] = {Q[13], Q[14], Q[15]};

    const double dGhat[3][3] = {
        {gradQin[13][0],gradQin[14][0],gradQin[15][0]},
        {gradQin[13][1],gradQin[14][1],gradQin[15][1]},
        {gradQin[13][2],gradQin[14][2],gradQin[15][2]}
    };

    double dGtilde[3][3] = {0,0,0,0,0,0,0,0,0};
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++)
    for (int j = 0; j < 3; j++)
    for (int l = 0; l < 3; l++) dGtilde[k][i] += dgup[k][j][l]*Christoffel_tilde[j][l][i]+g_contr[j][l]*dChristoffel_tilde[k][j][l][i];

    double Z[3] = {0,0,0}; // Matrix vector multiplications
    for (int i=0;i<3;i++)
    for (int j=0;j<3;j++) Z[i] += 0.5* g_cov[i][j]* (Ghat[j] - Gtilde[j]);//needed here

    double dZ[3][3] = {0,0,0,0,0,0,0,0,0};
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++) dZ[k][i] += DD[k][i][j]*(Ghat[j]-Gtilde[j])+0.5*g_cov[i][j]*(dGhat[k][j]-dGtilde[k][j]);

    double nablaZ[3][3] = {0,0,0,0,0,0,0,0,0};
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    {
      nablaZ[i][j] = dZ[i][j];
      for (int k = 0; k < 3; k++) nablaZ[i][j] -= Christoffel[i][j][k]*Z[k];
    }

    double nablaZcontracted=0;
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) nablaZcontracted += g_contr[i][j]*nablaZ[i][j]; // TODO fuse these steps
    nablaZcontracted*=phi2;

//end here

    double Ham=0;
    Ham = R-Kupdown+traceK*traceK;

    double dKex[3][3][3]={0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int k = 0; k < 3; k++) dKex[k][i][j]  = (1.0/phi2)*(dAex[k][i][j]+(1./3)*g_cov[i][j]*dtraceK[k]+(2./3)*traceK*DD[k][i][j])-2*Kex[i][j]*PP[k]/phi;

    double Mom[3]={0,0,0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int l = 0; l < 3; l++){
        Mom[i] += phi2*g_contr[j][l]*(dKex[l][i][j]-dKex[i][j][l]);
        for (int m = 0; m < 3; m++){
            Mom[i] += phi2*g_contr[j][l]*(-Christoffel[j][l][m]*Kex[m][i]+Christoffel[j][i][m]*Kex[m][l]);
        }
    }

    tarch::memset(constraints, 0, 4*sizeof(double));

    constraints[0]  =   Ham;        //59
    constraints[1]  =   Mom[0];     //60
    constraints[2]  =   Mom[1];     //61
    constraints[3]  =   Mom[2];     //62
    //constraints[4]  =   R;          //63
    //constraints[5]  =   Kupdown;    //64
    //constraints[6]  =   nablaZcontracted;   //65
    //constraints[4]  =   1.0/invdet-1.0;
    //constraints[5]  =   traceA;
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
void applications::exahype2::ccz4::Psi4Calc(double* Psi4, const double* const Q, const double* const gradQSerialised, double* coor)
{
    constexpr int nVar(59);
    double gradQin[59][3] ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    // De-serialise input data and fill static array
    for (int normal=0; normal<3; normal++)
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

    const double dAex[3][3][3] = {
        {{gradQin[6][0],gradQin[7][0],gradQin[8][0]}, {gradQin[7][0], gradQin[9][0], gradQin[10][0]},  {gradQin[8][0], gradQin[10][0], gradQin[11][0]}},
        {{gradQin[6][1],gradQin[7][1],gradQin[8][1]}, {gradQin[7][1], gradQin[9][1], gradQin[10][1]},  {gradQin[8][1], gradQin[10][1], gradQin[11][1]}},
        {{gradQin[6][2],gradQin[7][2],gradQin[8][2]}, {gradQin[7][2], gradQin[9][2], gradQin[10][2]},  {gradQin[8][2], gradQin[10][2], gradQin[11][2]}}
    };

    const double traceK = Q[53];
    const double dtraceK[3] = {gradQin[53][0], gradQin[53][1], gradQin[53][2]};

    const double phi = std::fmax(1e-2,Q[54]); ;
    const double phi2 = phi*phi;
    const double PP[3] = {Q[55], Q[56], Q[57]};

    const double dPP[3][3] = {
        {gradQin[55][0],gradQin[56][0],gradQin[57][0]},
        {gradQin[55][1],gradQin[56][1],gradQin[57][1]},
        {gradQin[55][2],gradQin[56][2],gradQin[57][2]}
    };

    const double DD[3][3][3] = {
        {{Q[35], Q[36], Q[37]}, {Q[36], Q[38], Q[39]}, {Q[37], Q[39], Q[40]}},
        {{Q[41], Q[42], Q[43]}, {Q[42], Q[44], Q[45]}, {Q[43], Q[45], Q[46]}},
        {{Q[47], Q[48], Q[49]}, {Q[48], Q[50], Q[51]}, {Q[49], Q[51], Q[52]}}
    };
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

    double dgup[3][3][3] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    for (int k = 0; k < 3; k++)
    for (int m = 0; m < 3; m++)
    for (int l = 0; l < 3; l++)
    for (int n = 0; n < 3; n++)
    for (int j = 0; j < 3; j++) dgup[k][m][l] -= g_contr[m][n]*g_contr[j][l]*2*DD[k][n][j];

    double Kex[3][3]={ 0 };
    for (int i=0;i<3;i++)
    for (int j=0;j<3;j++) Kex[i][j]=Aex[i][j]/phi2 + (1./3)*traceK*g_cov[i][j]/phi2;

    double Christoffel[3][3][3]       = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    double Christoffel_tilde[3][3][3]       = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++)
    for (int l = 0; l < 3; l++) {
        Christoffel_tilde[i][j][k] += g_contr[k][l] * ( DD[i][j][l] + DD[j][i][l] - DD[l][i][j] );
        Christoffel[i][j][k]  += g_contr[k][l] * ( DD[i][j][l] + DD[j][i][l] - DD[l][i][j] ) - g_contr[k][l]/phi * ( g_cov[j][l] * PP[i] + g_cov[i][l] * PP[j] - g_cov[i][j] * PP[l] );
    }

    double dChristoffel[3][3][3][3] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    double dChristoffel_tilde[3][3][3][3] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
 
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int m = 0; m < 3; m++)
    for (int k = 0; k < 3; k++)
    for (int l = 0; l < 3; l++)
    {
        dChristoffel[k][i][j][m] += 0.5*g_contr[m][l] * ( //npc part
            dDD[k][i][j][l] + dDD[i][k][j][l] + dDD[k][j][i][l] + dDD[j][k][i][l] - dDD[k][l][i][j] - dDD[l][k][i][j]
            - g_cov[j][l]*(dPP[k][i] + dPP[i][k])/phi - g_cov[i][l]*(dPP[k][j]+dPP[j][k])/phi +  g_cov[i][j]*(dPP[k][l]+dPP[l][k])/phi )
            +dgup[k][m][l]*(DD[i][j][l]+DD[j][i][l]-DD[l][i][j])  //src part
            -dgup[k][m][l]*(g_cov[j][l]*PP[i]+g_cov[i][l]*PP[j]-g_cov[i][j]*PP[l])/phi
            -2*g_contr[m][l]*(DD[k][j][l]*PP[i]+DD[k][i][l]*PP[j]-DD[k][i][j]*PP[l])/phi
            +g_contr[m][l]*(g_cov[j][l]*PP[i]*PP[k]+g_cov[i][l]*PP[j]*PP[k]-g_cov[i][j]*PP[k]*PP[l])/phi2;

        dChristoffel_tilde[k][i][j][m] += dgup[k][m][l]*(DD[i][j][l]+DD[j][i][l]-DD[l][i][j])
            +0.5*g_contr[m][l]*(dDD[k][i][j][l] + dDD[i][k][j][l] + dDD[k][j][i][l] + dDD[j][k][i][l] - dDD[k][l][i][j] - dDD[l][k][i][j]);
    }

    double Gtilde[3] = {0,0,0};
    for (int l = 0; l < 3; l++)
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++) Gtilde[i] += g_contr[j][l] * Christoffel_tilde[j][l][i];

    const double Theta = Q[12]; // needed later
    const double Ghat[3] = {Q[13], Q[14], Q[15]};

    double Z[3] = {0,0,0}; // Matrix vector multiplications
    for (int i=0;i<3;i++)
    for (int j=0;j<3;j++) Z[i] += 0.5* g_cov[i][j]* (Ghat[j] - Gtilde[j]);

    const double dGhat[3][3] = {
        {gradQin[13][0],gradQin[14][0],gradQin[15][0]},
        {gradQin[13][1],gradQin[14][1],gradQin[15][1]},
        {gradQin[13][2],gradQin[14][2],gradQin[15][2]}
    };

    double dGtilde[3][3] = {0,0,0,0,0,0,0,0,0};
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++)
    for (int j = 0; j < 3; j++)
    for (int l = 0; l < 3; l++) dGtilde[k][i] += dgup[k][j][l]*Christoffel_tilde[j][l][i]+g_contr[j][l]*dChristoffel_tilde[k][j][l][i];

    double dZ[3][3] = {0,0,0,0,0,0,0,0,0};
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++) dZ[k][i] += DD[k][i][j]*(Ghat[j]-Gtilde[j])+0.5*g_cov[i][j]*(dGhat[k][j]-dGtilde[k][j]);

    double nablaZ[3][3] = {0,0,0,0,0,0,0,0,0};
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    {
      nablaZ[i][j] = dZ[i][j];
      for (int k = 0; k < 3; k++) nablaZ[i][j] -= Christoffel[i][j][k]*Z[k];
    }


    double Riemann[3][3][3][3] = {0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int m = 0; m < 3; m++)
    for (int k = 0; k < 3; k++){
        Riemann[i][k][j][m] = dChristoffel[k][i][j][m] - dChristoffel[j][i][k][m];
        for (int l = 0; l < 3; l++){
            Riemann[i][k][j][m] += Christoffel[i][j][l]*Christoffel[l][k][m]-Christoffel[i][k][l]*Christoffel[l][j][m];
        }
    }

    double Ricci[3][3] = {0,0,0,0,0,0,0,0,0};
    for (int m = 0; m < 3; m++)
    for (int n = 0; n < 3; n++)
    for (int l = 0; l < 3; l++) Ricci[m][n] += Riemann[m][l][n][l];
    
    double dKex[3][3][3]   = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};  //derivative  \partial_k K_ij
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++) dKex[k][i][j]  = (1.0/phi2)*(dAex[k][i][j]+(1./3)*g_cov[i][j]*dtraceK[k]+(2./3)*traceK*DD[k][i][j])-2*Kex[i][j]*PP[k]/phi;
    
    double Cov_dKex[3][3][3]   = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};  //covariant derivative \nabla_i K_jk 
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++)
    for (int m = 0; m < 3; m++) Cov_dKex[i][j][k] += dKex[i][j][k]-Christoffel[i][k][m]*Kex[j][m]-Christoffel[i][j][m]*Kex[m][k];
    
	const double detgd=(1.0/(phi2*phi2*phi2))*( Q[0]*Q[3]*Q[5] - Q[0]*Q[4]*Q[4] - Q[1]*Q[1]*Q[5] + 2*Q[1]*Q[2]*Q[4] -Q[2]*Q[2]*Q[3]); //determinant of \gamma_ij
	
	double eps_lc_u[3][3][3]   = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //Levi-Civita tensor
    double eps_lc_symbol[3][3][3]   = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //Levi-Civita symbol
	eps_lc_u[0][1][2] = 1/pow(detgd,0.5); eps_lc_u[1][2][0] = 1/pow(detgd,0.5); eps_lc_u[2][0][1] = 1/pow(detgd,0.5);
	eps_lc_u[2][1][0] =-1/pow(detgd,0.5); eps_lc_u[0][2][1] =-1/pow(detgd,0.5); eps_lc_u[1][0][2] =-1/pow(detgd,0.5);
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++) {
        eps_lc_symbol[i][j][k] = eps_lc_u[i][j][k] * pow(detgd,0.5);
        if (detgd<0){ eps_lc_u[i][j][k]= - eps_lc_u[i][j][k]; }
    }

	
	//calculate the orthonormal basis
    //mainly follow the paper: https://arxiv.org/pdf/gr-qc/0610128v1.pdf
	if ((coor[0]*coor[0]+coor[1]*coor[1])<1e-10) {coor[0]+=1e-10;}
	double u_vec[3]={coor[0], coor[1], coor[2]};//r-direction
	double v_vec[3]={-coor[2], coor[1], 0.0}; //phi-direction
	//double w_vec[3]={coor[0]*coor[2],coor[1]*coor[2],-coor[0]*coor[0]-coor[1]*coor[1]};//theta-direction //??
    double w_vec[3]={0,0,0};
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++)
    for (int l = 0; l < 3; l++) w_vec[i] += phi2*g_contr[i][j]*eps_lc_symbol[j][k][l]*v_vec[k]*u_vec[l];

	
	double dotp1=0;
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++) dotp1 += g_cov[i][j]*v_vec[i]*v_vec[j]/phi2;
	for (int a = 0; a < 3; a++) v_vec[a] = v_vec[a] / pow(dotp1,0.5);
	
	dotp1=0;
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++) dotp1 += g_cov[i][j]*v_vec[i]*u_vec[j]/phi2;
	for (int a = 0; a < 3; a++) u_vec[a] = u_vec[a] - dotp1* v_vec[a];	
	
	dotp1=0;
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++) dotp1 += g_cov[i][j]*u_vec[i]*u_vec[j]/phi2;
	for (int a = 0; a < 3; a++) u_vec[a] = u_vec[a] / pow(dotp1,0.5);
	
	dotp1=0;
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++) dotp1 += g_cov[i][j]*v_vec[i]*w_vec[j]/phi2;	
	double dotp2=0;
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++) dotp2 += g_cov[i][j]*u_vec[i]*w_vec[j]/phi2;
    
	for (int a = 0; a < 3; a++) w_vec[a] = u_vec[a] - dotp1 * v_vec[a] - dotp2 * u_vec[a];		
		
	dotp1=0;
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++) dotp1 += g_cov[i][j]*w_vec[i]*w_vec[j]/phi2;
	for (int a = 0; a < 3; a++) w_vec[a] = w_vec[a] / pow(dotp1,0.5);
	
	//EB part of Weyl
    //reference: https://academic.oup.com/book/9640/chapter/156750056
    //8.3.15 and 8.3.16
	double elec[3][3] = {0,0,0,0,0,0,0,0,0};
	double mag[3][3] = {0,0,0,0,0,0,0,0,0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++){
    	elec[i][j]=Ricci[i][j];
		for (int m = 0; m < 3; m++)
		for (int k = 0; k < 3; k++){
			elec[i][j] += phi2*g_contr[m][k]*(Kex[i][j]*Kex[m][k]-Kex[i][m]*Kex[k][j])-Kex[i][j]*Theta + 0.5*(nablaZ[i][j]+nablaZ[j][i]);
			for (int l = 0; l < 3; l++){
				mag[i][j] += g_cov[i][l] * eps_lc_u[l][k][m] * Cov_dKex[k][m][j] + g_cov[j][l] * eps_lc_u[l][k][m] * Cov_dKex[k][m][i];
			}
		}
		mag[i][j]=mag[i][j]/(2*phi2);
	}
	
	double electr=0, magtr=0;
	for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++){
    	electr += phi2*g_contr[i][j]*elec[i][j];
		magtr  += phi2*g_contr[i][j]*mag[i][j];
	}
	for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++){
		elec[i][j] = elec[i][j] - g_cov[i][j]*electr / (3*phi2);
		mag[i][j]  = mag[i][j]  - g_cov[i][j]*magtr  / (3*phi2);
	}

    //use 8.6.17 and 8.6.18
    //notice we use \bar{m}^i={1 \over \sqrt{2} }(w^i- i v^i)
    Psi4[0]=0.0; Psi4[1]=0.0;
	for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++){
    	//Real part for Psi4
		Psi4[0] += 0.5*elec[i][j]*(w_vec[i]*w_vec[j]-v_vec[i]*v_vec[j])-0.5*mag[i][j]*(v_vec[i]*w_vec[j]+v_vec[j]*w_vec[i]);
		//imaginary part for Psi4
		Psi4[1] += - 0.5*elec[i][j]*(v_vec[i]*w_vec[j]+v_vec[j]*w_vec[i])-0.5*mag[i][j]*(w_vec[i]*w_vec[j]-v_vec[i]*v_vec[j]);
	}
	
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
void applications::exahype2::ccz4::TestingOutput(double* terms, const double* const Q, const double* const gradQSerialised)
{
    constexpr int nVar(59);
    double gradQin[59][3] ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    // De-serialise input data and fill static array
    for (int normal=0; normal<3; normal++)
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

    const double dAex[3][3][3] = {
        {{gradQin[6][0],gradQin[7][0],gradQin[8][0]}, {gradQin[7][0], gradQin[9][0], gradQin[10][0]},  {gradQin[8][0], gradQin[10][0], gradQin[11][0]}},
        {{gradQin[6][1],gradQin[7][1],gradQin[8][1]}, {gradQin[7][1], gradQin[9][1], gradQin[10][1]},  {gradQin[8][1], gradQin[10][1], gradQin[11][1]}},
        {{gradQin[6][2],gradQin[7][2],gradQin[8][2]}, {gradQin[7][2], gradQin[9][2], gradQin[10][2]},  {gradQin[8][2], gradQin[10][2], gradQin[11][2]}}
    };

    const double traceK = Q[53];
    const double dtraceK[3] = {gradQin[53][0], gradQin[53][1], gradQin[53][2]};

    const double phi = std::fmax(1e-2,Q[54]); 
    const double phi2 = phi*phi;
    const double PP[3] = {Q[55], Q[56], Q[57]};

    const double dPP[3][3] = {
        {gradQin[55][0],gradQin[56][0],gradQin[57][0]},
        {gradQin[55][1],gradQin[56][1],gradQin[57][1]},
        {gradQin[55][2],gradQin[56][2],gradQin[57][2]}
    };

    const double DD[3][3][3] = {
        {{Q[35], Q[36], Q[37]}, {Q[36], Q[38], Q[39]}, {Q[37], Q[39], Q[40]}},
        {{Q[41], Q[42], Q[43]}, {Q[42], Q[44], Q[45]}, {Q[43], Q[45], Q[46]}},
        {{Q[47], Q[48], Q[49]}, {Q[48], Q[50], Q[51]}, {Q[49], Q[51], Q[52]}}
    };
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

    double dgup[3][3][3] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    for (int k = 0; k < 3; k++)
    for (int m = 0; m < 3; m++)
    for (int l = 0; l < 3; l++)
    for (int n = 0; n < 3; n++)
    for (int j = 0; j < 3; j++) dgup[k][m][l] -= g_contr[m][n]*g_contr[j][l]*2*DD[k][n][j];

    double Kex[3][3]={ 0 };
    for (int i=0;i<3;i++)
    for (int j=0;j<3;j++) Kex[i][j]=Aex[i][j]/phi2 + (1./3)*traceK*g_cov[i][j]/phi2;
    double Kmix[3][3]={0,0,0,0,0,0,0,0,0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int u = 0; u < 3; u++) Kmix[i][j] += phi2*g_contr[i][u] * Kex[u][j];
    double Kup[3][3]={0,0,0,0,0,0,0,0,0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int u = 0; u < 3; u++) Kup[i][j] += phi2*g_contr[i][u] * Kmix[j][u]; // Note the transposition is in the indices

    double Christoffel[3][3][3]       = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    double Christoffel_tilde[3][3][3]       = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++)
    for (int l = 0; l < 3; l++) {
        Christoffel_tilde[i][j][k] += g_contr[k][l] * ( DD[i][j][l] + DD[j][i][l] - DD[l][i][j] );
        Christoffel[i][j][k]  += g_contr[k][l] * ( DD[i][j][l] + DD[j][i][l] - DD[l][i][j] ) - g_contr[k][l]/phi * ( g_cov[j][l] * PP[i] + g_cov[i][l] * PP[j] - g_cov[i][j] * PP[l] );
    }

    double dChristoffel[3][3][3][3] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    double dChristoffel_tilde[3][3][3][3] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
 
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int m = 0; m < 3; m++)
    for (int k = 0; k < 3; k++)
    for (int l = 0; l < 3; l++)
    {
        dChristoffel[k][i][j][m] += 0.5*g_contr[m][l] * ( //npc part
            dDD[k][i][j][l] + dDD[i][k][j][l] + dDD[k][j][i][l] + dDD[j][k][i][l] - dDD[k][l][i][j] - dDD[l][k][i][j]
            - g_cov[j][l]*(dPP[k][i] + dPP[i][k])/phi - g_cov[i][l]*(dPP[k][j]+dPP[j][k])/phi +  g_cov[i][j]*(dPP[k][l]+dPP[l][k])/phi )
            +dgup[k][m][l]*(DD[i][j][l]+DD[j][i][l]-DD[l][i][j])  //src part
            -dgup[k][m][l]*(g_cov[j][l]*PP[i]+g_cov[i][l]*PP[j]-g_cov[i][j]*PP[l])/phi
            -2*g_contr[m][l]*(DD[k][j][l]*PP[i]+DD[k][i][l]*PP[j]-DD[k][i][j]*PP[l])/phi
            +g_contr[m][l]*(g_cov[j][l]*PP[i]*PP[k]+g_cov[i][l]*PP[j]*PP[k]-g_cov[i][j]*PP[k]*PP[l])/phi2;

        dChristoffel_tilde[k][i][j][m] += dgup[k][m][l]*(DD[i][j][l]+DD[j][i][l]-DD[l][i][j])
            +0.5*g_contr[m][l]*(dDD[k][i][j][l] + dDD[i][k][j][l] + dDD[k][j][i][l] + dDD[j][k][i][l] - dDD[k][l][i][j] - dDD[l][k][i][j]);
    }

    double Riemann[3][3][3][3] = {0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int m = 0; m < 3; m++)
    for (int k = 0; k < 3; k++){
        Riemann[i][k][j][m] = dChristoffel[k][i][j][m] - dChristoffel[j][i][k][m];
        for (int l = 0; l < 3; l++){
            Riemann[i][k][j][m] += Christoffel[i][j][l]*Christoffel[l][k][m]-Christoffel[i][k][l]*Christoffel[l][j][m];
        }
    }

    double Ricci[3][3] = {0,0,0,0,0,0,0,0,0};
    for (int m = 0; m < 3; m++)
    for (int n = 0; n < 3; n++)
    for (int l = 0; l < 3; l++) Ricci[m][n] += Riemann[m][l][n][l];

    double R=0;
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) R += phi2*g_contr[i][j]*Ricci[i][j];
    double Kupdown=0;
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) Kupdown += Kex[i][j]*Kup[i][j];

// following is added to calculate nabla_i Z^i
    double Gtilde[3] = {0,0,0};
    for (int l = 0; l < 3; l++)
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++) Gtilde[i] += g_contr[j][l] * Christoffel_tilde[j][l][i];

    const double Ghat[3] = {Q[13], Q[14], Q[15]};

    const double dGhat[3][3] = {
        {gradQin[13][0],gradQin[14][0],gradQin[15][0]},
        {gradQin[13][1],gradQin[14][1],gradQin[15][1]},
        {gradQin[13][2],gradQin[14][2],gradQin[15][2]}
    };

    double dGtilde[3][3] = {0,0,0,0,0,0,0,0,0};
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++)
    for (int j = 0; j < 3; j++)
    for (int l = 0; l < 3; l++) dGtilde[k][i] += dgup[k][j][l]*Christoffel_tilde[j][l][i]+g_contr[j][l]*dChristoffel_tilde[k][j][l][i];

    double Z[3] = {0,0,0}; // Matrix vector multiplications
    for (int i=0;i<3;i++)
    for (int j=0;j<3;j++) Z[i] += 0.5* g_cov[i][j]* (Ghat[j] - Gtilde[j]);//needed here

    double dZ[3][3] = {0,0,0,0,0,0,0,0,0};
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++) dZ[k][i] += DD[k][i][j]*(Ghat[j]-Gtilde[j])+0.5*g_cov[i][j]*(dGhat[k][j]-dGtilde[k][j]);

    double nablaZ[3][3] = {0,0,0,0,0,0,0,0,0};
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    {
      nablaZ[i][j] = dZ[i][j];
      for (int k = 0; k < 3; k++) nablaZ[i][j] -= Christoffel[i][j][k]*Z[k];
    }

    double nablaZcontracted=0;
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) nablaZcontracted += g_contr[i][j]*nablaZ[i][j]; // TODO fuse these steps
    nablaZcontracted*=phi2;

//end here

    double Ham=0;
    Ham = R-Kupdown+traceK*traceK;

    double dKex[3][3][3]={0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int k = 0; k < 3; k++) dKex[k][i][j]  = (1.0/phi2)*(dAex[k][i][j]+(1./3)*g_cov[i][j]*dtraceK[k]+(2./3)*traceK*DD[k][i][j])-2*Kex[i][j]*PP[k]/phi;

    double Mom[3]={0,0,0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int l = 0; l < 3; l++){
        Mom[i] += phi2*g_contr[j][l]*(dKex[l][i][j]-dKex[i][j][l]);
        for (int m = 0; m < 3; m++){
            Mom[i] += phi2*g_contr[j][l]*(-Christoffel[j][l][m]*Kex[m][i]+Christoffel[j][i][m]*Kex[m][l]);
        }
    }

    tarch::memset(terms, 0, 15*sizeof(double));
    terms[0]  =   Ham;        //59
    terms[1]  =   Mom[0];     //60
    terms[2]  =   Mom[1];     //61
    terms[3]  =   Mom[2];     //62
    terms[4]  =   R;          //63
    terms[5]  =   Kupdown;    //64
    terms[6]  =   nablaZcontracted;   //65
    //calculate the terms for Px
    const double alpha = std::fmax(1e-4,Q[16]);
    const double dBB[3][3][3] = {
        {
            { gradQin[26][0], gradQin[27][0], gradQin[28][0]}, { gradQin[29][0], gradQin[30][0], gradQin[31][0]}, { gradQin[32][0], gradQin[33][0], gradQin[34][0]}
        },
        {
            { gradQin[26][1], gradQin[27][1], gradQin[28][1]}, { gradQin[29][1], gradQin[30][1], gradQin[31][1]}, { gradQin[32][1], gradQin[33][1], gradQin[34][1]}
        },
        {
            { gradQin[26][2], gradQin[27][2], gradQin[28][2]}, { gradQin[29][2], gradQin[30][2], gradQin[31][2]}, { gradQin[32][2], gradQin[33][2], gradQin[34][2]}
        }
    };
    const double BB[3][3] = {
        {Q[26], Q[27], Q[28]}, {Q[29], Q[30], Q[31]}, {Q[32], Q[33], Q[34]}
    };
    double traceB = BB[0][0] + BB[1][1] + BB[2][2];
    const double AA[3] = {Q[23], Q[24], Q[25]};

    for (int l = 0; l < 3; l++) terms[7]+=BB[0][l]*PP[l]; //66
    terms[8] = (1.0/3.0)*alpha*traceK*PP[0]; //67
    terms[9] = -(1.0/3.0)*traceB*PP[0]; //68
    terms[10]= (1.0/3.0)*phi*alpha*dtraceK[0]; //69
    terms[11]= (1.0/3.0)*phi*traceK*AA[0]; //70
    for (int l = 0; l < 3; l++) terms[12] -= 1./6.*phi*(dBB[0][l][l] + dBB[l][0][l]);  //71
    double temp=0;
    for (int m = 0; m < 3; m++)
    for (int n = 0; n < 3; n++) temp += g_contr[m][n]*dAex[0][m][n];
    terms[13] += 1./3.*alpha*temp; //72
    temp=0;
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) temp += dgup[0][i][j]*Aex[i][j];
    terms[14] += 1./3.*alpha*temp;  //73

    //constraints[4]  =   1.0/invdet-1.0;
    //constraints[5]  =   traceA;
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
void applications::exahype2::ccz4::ThetaOutputNCP(double* NCPterm, const double* const Q, const double* const gradQSerialised, int normal)
{
    const double alpha = std::exp(std::fmax(-20., std::fmin(20.,Q[16])));
    //printf("alpha %f\n",alpha);
    const double alpha2 = alpha*alpha;
    double fa  = 1.0;
    double faa = 0.0;
    fa  =  2./alpha;
    faa = -fa/alpha;
    const double traceK = Q[53];

    constexpr int nVar(59);

    double gradQin[59][3] ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    for (int i=0; i<nVar; i++) {
      gradQin[i][normal] = gradQSerialised[i];
      //if (gradQSerialised[i]>0.0) std::cout<< gradQSerialised[i] << std::endl; 
    }

    // Note g_cov is symmetric
    const double g_cov[3][3] = { {Q[0], Q[1], Q[2]}, {Q[1], Q[3], Q[4]}, {Q[2], Q[4], Q[5]} };
    const double invdet = 1./( Q[0]*Q[3]*Q[5] - Q[0]*Q[4]*Q[4] - Q[1]*Q[1]*Q[5] + 2*Q[1]*Q[2]*Q[4] -Q[2]*Q[2]*Q[3]);

    const double g_contr[3][3] = {
        { ( Q[3]*Q[5]-Q[4]*Q[4])*invdet, -( Q[1]*Q[5]-Q[2]*Q[4])*invdet, -(-Q[1]*Q[4]+Q[2]*Q[3])*invdet},
        {-( Q[1]*Q[5]-Q[4]*Q[2])*invdet,  ( Q[0]*Q[5]-Q[2]*Q[2])*invdet, -( Q[0]*Q[4]-Q[2]*Q[1])*invdet},
        {-(-Q[1]*Q[4]+Q[3]*Q[2])*invdet, -( Q[0]*Q[4]-Q[1]*Q[2])*invdet,  ( Q[0]*Q[3]-Q[1]*Q[1])*invdet}
    };
   const double dPP[3][3] = {
        {gradQin[55][0],gradQin[56][0],gradQin[57][0]},
        {gradQin[55][1],gradQin[56][1],gradQin[57][1]},
        {gradQin[55][2],gradQin[56][2],gradQin[57][2]}
    };

    // NOTE Aex is symmetric
    double Aex[3][3] = { {Q[6], Q[7], Q[8]}, {Q[7], Q[9], Q[10]}, {Q[8], Q[10], Q[11]} };

    double traceA = 0;
    for (int i=0;i<3;i++)
    for (int j=0;j<3;j++) traceA+=g_contr[i][j]*Aex[i][j];
    //traceA *= 1./3;

    for (int i=0;i<3;i++)
    for (int j=0;j<3;j++) Aex[i][j] -= 1./3. * traceA * g_cov[i][j];

    // Matrix multiplications Amix = matmul(g_contr, Aex) Aup  = matmul(g_contr, mytranspose(Amix))
    double Amix[3][3]={0,0,0,0,0,0,0,0,0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int u = 0; u < 3; u++) Amix[i][j] += g_contr[i][u] * Aex[u][j];
    double Aup[3][3]={0,0,0,0,0,0,0,0,0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int u = 0; u < 3; u++) Aup[i][j] += g_contr[i][u] * Amix[j][u]; // Note the transposition is in the indices

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
    for (int j = 0; j < 3; j++) dgup[k][m][l] -= g_contr[m][n]*g_contr[j][l]*2*DD[k][n][j];

    const double PP[3] = {Q[55], Q[56], Q[57]};

    double Christoffel[3][3][3]       = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    double Christoffel_tilde[3][3][3] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    //double Christoffel_kind1[3][3][3] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++)
    {
        //Christoffel_kind1[i][j][k] = DD[k][i][j] + DD[j][i][k] - DD[i][j][k];
        for (int l = 0; l < 3; l++)
        {
            Christoffel_tilde[i][j][k] += g_contr[k][l] * ( DD[i][j][l] + DD[j][i][l] - DD[l][i][j] );
            Christoffel[i][j][k]       += g_contr[k][l] * ( DD[i][j][l] + DD[j][i][l] - DD[l][i][j] ) - g_contr[k][l] * ( g_cov[j][l] * PP[i] + g_cov[i][l] * PP[j] - g_cov[i][j] * PP[l] );
        }
    }

    double Gtilde[3] = {0,0,0};
    for (int l = 0; l < 3; l++)
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++) Gtilde[i] += g_contr[j][l] * Christoffel_tilde[j][l][i];

    const double Ghat[3] = {Q[13], Q[14], Q[15]};

    const double phi = std::exp(std::fmax(-20., std::fmin(20.,Q[54])));
    const double phi2 = phi*phi;

    double Z[3] = {0,0,0};
    for (int i=0;i<3;i++)
    for (int j=0;j<3;j++) Z[i] += 0.5*( g_cov[i][j]* (Ghat[j] - Gtilde[j]));
    double Zup[3] = {0,0,0};
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


    double dChristoffelNCP[3][3][3][3] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    double dChristoffel_tildeNCP[3][3][3][3] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int m = 0; m < 3; m++)
    for (int k = 0; k < 3; k++)
    {
        dChristoffelNCP      [k][i][j][m] = 0;
        dChristoffel_tildeNCP[k][i][j][m] = 0;
        for (int l = 0; l < 3; l++)
        {
            dChristoffelNCP[k][i][j][m] +=  0.5*g_contr[m][l] * (
                    dDD[k][i][j][l] + dDD[i][k][j][l] + dDD[k][j][i][l] + dDD[j][k][i][l] - dDD[k][l][i][j] - dDD[l][k][i][j]
                    - g_cov[j][l]*(dPP[k][i] + dPP[i][k]) - g_cov[i][l]*(dPP[k][j]+dPP[j][k]) +  g_cov[i][j]*(dPP[k][l]+dPP[l][k]) );

            dChristoffel_tildeNCP[k][i][j][m] += 0.5*g_contr[m][l]*(dDD[k][i][j][l] + dDD[i][k][j][l] + dDD[k][j][i][l] + dDD[j][k][i][l] - dDD[k][l][i][j] - dDD[l][k][i][j]);

        }
    }

    double RiemannNCP[3][3][3][3] = {0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int m = 0; m < 3; m++)
    for (int k = 0; k < 3; k++) RiemannNCP[i][k][j][m] = dChristoffelNCP[k][i][j][m] - dChristoffelNCP[j][i][k][m];

    double RicciNCP[3][3] = {0,0,0,0,0,0,0,0,0};
    for (int m = 0; m < 3; m++)
    for (int n = 0; n < 3; n++)
    for (int l = 0; l < 3; l++) RicciNCP[m][n] += RiemannNCP[m][l][n][l];

    double dGtildeNCP[3][3] = {0,0,0,0,0,0,0,0,0};
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++)
    for (int j = 0; j < 3; j++)
    for (int l = 0; l < 3; l++) dGtildeNCP[k][i] += g_contr[j][l]*dChristoffel_tildeNCP[k][j][l][i];


    const double dGhat[3][3] = {
        {gradQin[13][0],gradQin[14][0],gradQin[15][0]},
        {gradQin[13][1],gradQin[14][1],gradQin[15][1]},
        {gradQin[13][2],gradQin[14][2],gradQin[15][2]}
    };

    double dZNCP[3][3] = {0,0,0,0,0,0,0,0,0};
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++) dZNCP[k][i] += 0.5*g_cov[i][j]*(dGhat[k][j]-dGtildeNCP[k][j]);

    double RicciPlusNablaZNCP[3][3];
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) RicciPlusNablaZNCP[i][j] = RicciNCP[i][j] + dZNCP[i][j] + dZNCP[j][i];

    double RPlusTwoNablaZNCP = 0;
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) RPlusTwoNablaZNCP += g_contr[i][j]*RicciPlusNablaZNCP[i][j];
    RPlusTwoNablaZNCP*=phi2;

    NCPterm[0]  =   RPlusTwoNablaZNCP;
    //if (NCPterm>0.0) std::cout<< NCPterm << std::endl; 
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
