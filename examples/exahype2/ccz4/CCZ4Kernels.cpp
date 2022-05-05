#include "tarch/multicore/multicore.h"
#include "Constants.h"

#include <limits>
#include <stdio.h>
#include <string.h>


#include "CCZ4Kernels.h"

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
void examples::exahype2::ccz4::enforceCCZ4constraints(double * luh)
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
void examples::exahype2::ccz4::admconstraints(double* constraints, const double* const Q, const double* const gradQSerialised)
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

    const double phi = std::exp(std::fmax(-20., std::fmin(20.,Q[54])));
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
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++)
    for (int l = 0; l < 3; l++) Christoffel[i][j][k]  += g_contr[k][l] * ( DD[i][j][l] + DD[j][i][l] - DD[l][i][j] ) - g_contr[k][l] * ( g_cov[j][l] * PP[i] + g_cov[i][l] * PP[j] - g_cov[i][j] * PP[l] );

    double dChristoffel[3][3][3][3] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int m = 0; m < 3; m++)
    for (int k = 0; k < 3; k++)
    for (int l = 0; l < 3; l++)
    {
        dChristoffel[k][i][j][m] += 0.5*g_contr[m][l] * (
            dDD[k][i][j][l] + dDD[i][k][j][l] + dDD[k][j][i][l] + dDD[j][k][i][l] - dDD[k][l][i][j] - dDD[l][k][i][j]
            - g_cov[j][l]*(dPP[k][i] + dPP[i][k]) - g_cov[i][l]*(dPP[k][j]+dPP[j][k]) +  g_cov[i][j]*(dPP[k][l]+dPP[l][k]) )
            +dgup[k][m][l]*(DD[i][j][l]+DD[j][i][l]-DD[l][i][j])
            -dgup[k][m][l]*(g_cov[j][l]*PP[i]+g_cov[i][l]*PP[j]-g_cov[i][j]*PP[l])
            -2*g_contr[m][l]*(DD[k][j][l]*PP[i]+DD[k][i][l]*PP[j]-DD[k][i][j]*PP[l]);
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

    double Ham=0;
    Ham = R-Kupdown+traceK*traceK;

    double dKex[3][3][3]={0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int k = 0; k < 3; k++) dKex[k][i][j]= (1.0/phi2)*(
            dAex[k][i][j]-2*Aex[i][j]*PP[k]+(1.0/3)*dtraceK[k]*g_cov[i][j]
            +(2.0/3)*traceK*DD[k][i][j]-(2.0/3)*traceK*g_cov[i][j]*PP[k]);

    double Mom[3]={0,0,0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int l = 0; l < 3; l++){
        Mom[i] += phi2*g_contr[j][l]*(dKex[l][i][j]-dKex[i][j][l]);
        for (int m = 0; m < 3; m++){
            Mom[i] += phi2*g_contr[j][l]*(-Christoffel[j][l][m]*Kex[m][i]+Christoffel[j][i][m]*Kex[m][l]);
        }
    }

    memset(constraints, 0, 6*sizeof(double));
    constraints[0]  =   Ham;
    constraints[1]  =   Mom[0];
    constraints[2]  =   Mom[1];
    constraints[3]  =   Mom[2];
    constraints[4]  =   1.0/invdet-1.0;
    constraints[5]  =   traceA;
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
void examples::exahype2::ccz4::Psi4Calc(double* Psi4, const double* const Q, const double* const gradQSerialised, double* coor)
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

    const double phi = std::exp(std::fmax(-20., std::fmin(20.,Q[54])));
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
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++)
    for (int l = 0; l < 3; l++) Christoffel[i][j][k]  += g_contr[k][l] * ( DD[i][j][l] + DD[j][i][l] - DD[l][i][j] ) - g_contr[k][l] * ( g_cov[j][l] * PP[i] + g_cov[i][l] * PP[j] - g_cov[i][j] * PP[l] );

    double dChristoffel[3][3][3][3] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
    for (int m = 0; m < 3; m++)
    for (int k = 0; k < 3; k++)
    for (int l = 0; l < 3; l++)
    {
        dChristoffel[k][i][j][m] += 0.5*g_contr[m][l] * (
            dDD[k][i][j][l] + dDD[i][k][j][l] + dDD[k][j][i][l] + dDD[j][k][i][l] - dDD[k][l][i][j] - dDD[l][k][i][j]
            - g_cov[j][l]*(dPP[k][i] + dPP[i][k]) - g_cov[i][l]*(dPP[k][j]+dPP[j][k]) +  g_cov[i][j]*(dPP[k][l]+dPP[l][k]) )
            +dgup[k][m][l]*(DD[i][j][l]+DD[j][i][l]-DD[l][i][j])
            -dgup[k][m][l]*(g_cov[j][l]*PP[i]+g_cov[i][l]*PP[j]-g_cov[i][j]*PP[l])
            -2*g_contr[m][l]*(DD[k][j][l]*PP[i]+DD[k][i][l]*PP[j]-DD[k][i][j]*PP[l]);
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
    for (int k = 0; k < 3; k++) dKex[k][i][j]  = (1.0/phi2)*(dAex[k][i][j]+(1./3)*g_cov[i][j]*dtraceK[k]+(2./3)*traceK*DD[k][i][j])-2*Kex[i][j]*PP[k];
    
    double Cov_dKex[3][3][3]   = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};  //covariant derivative \nabla_i K_jk 
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++)
    for (int m = 0; m < 3; m++) Cov_dKex[i][j][k] += dKex[i][j][k]-Christoffel[i][k][m]*Kex[j][m]-Christoffel[i][j][m]*Kex[m][k];
    
	const double detgd=(1.0/(phi2*phi2*phi2))*( Q[0]*Q[3]*Q[5] - Q[0]*Q[4]*Q[4] - Q[1]*Q[1]*Q[5] + 2*Q[1]*Q[2]*Q[4] -Q[2]*Q[2]*Q[3]); //determinant of \gamma_ij
	
	double eps_lc_u[3][3][3]   = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //Levi-Civita tensor
	eps_lc_u[0][1][2] = 1/pow(detgd,0.5); eps_lc_u[1][2][0] = 1/pow(detgd,0.5); eps_lc_u[2][0][1] = 1/pow(detgd,0.5);
	eps_lc_u[2][1][0] =-1/pow(detgd,0.5); eps_lc_u[0][2][1] =-1/pow(detgd,0.5); eps_lc_u[1][0][2] =-1/pow(detgd,0.5);
	
	//calculate the orthonormal basis
	if ((coor[0]*coor[0]+coor[1]*coor[1])<1e-10) {coor[0]+=1e-10;}
	double v_vec[3]={coor[0], coor[1], coor[2]};//r-direction
	double u_vec[3]={-coor[2], coor[1], 0.0}; //phi-direction
	double w_vec[3]={coor[0]*coor[2],coor[1]*coor[2],-coor[0]*coor[0]-coor[1]*coor[1]};//theta-direction
	
	double dotp1=0;
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++) dotp1 += g_cov[i][j]*v_vec[i]*v_vec[j]/phi2;
	for (int a = 0; a < 3; a++) v_vec[a] = v_vec[a] / pow(dotp1,0.5);
	
	dotp1=0;
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++) dotp1 += g_cov[i][j]*v_vec[i]*w_vec[j]/phi2;
	for (int a = 0; a < 3; a++) w_vec[a] = w_vec[a] - dotp1* v_vec[a];	
	
	dotp1=0;
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++) dotp1 += g_cov[i][j]*w_vec[i]*w_vec[j]/phi2;
	for (int a = 0; a < 3; a++) w_vec[a] = w_vec[a] / pow(dotp1,0.5);
	
	dotp1=0;
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++) dotp1 += g_cov[i][j]*v_vec[i]*u_vec[j]/phi2;	
	double dotp2=0;
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++) dotp2 += g_cov[i][j]*w_vec[i]*u_vec[j]/phi2;
    
	for (int a = 0; a < 3; a++) u_vec[a] = u_vec[a] - dotp1 * v_vec[a] - dotp2 * w_vec[a];		
		
	dotp1=0;
    for (int j = 0; j < 3; j++)
    for (int i = 0; i < 3; i++) dotp1 += g_cov[i][j]*u_vec[i]*u_vec[j]/phi2;
	for (int a = 0; a < 3; a++) u_vec[a] = u_vec[a] / pow(dotp1,0.5);
	
	//EB part of Weyl
	double elec[3][3] = {0,0,0,0,0,0,0,0,0};
	double mag[3][3] = {0,0,0,0,0,0,0,0,0};
    for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++){
    	elec[i][j]=Ricci[i][j];
		for (int m = 0; m < 3; m++)
		for (int k = 0; k < 3; k++){
			elec[i][j] += phi2*g_contr[m][k]*(Kex[i][j]*Kex[m][k]-Kex[i][m]*Kex[k][j]);
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
		elec[i][j] -= elec[i][j] - g_cov[i][j]*electr / (3*phi2);
		mag[i][j]  -= mag[i][j]  - g_cov[i][j]*magtr  / (3*phi2);
	}

	for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++){
    	//Real part for Psi4
		Psi4[0] += 0.5*elec[i][j]*(w_vec[i]*w_vec[j]-u_vec[i]*u_vec[j])-0.5*mag[i][j]*(u_vec[i]*w_vec[j]+u_vec[j]*w_vec[i]);
		//imaginary part for Psi4
		Psi4[0] += -0.5*elec[i][j]*(u_vec[i]*w_vec[j]+u_vec[j]*w_vec[j])-0.5*mag[i][j]*(w_vec[i]*w_vec[j]-u_vec[i]*u_vec[i]);
	}
	
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif


