
#include <mm_malloc.h> //g++

#include "./Quadrature.h"

// Use Gauss-Legendre quadrature
double* generated::kernels::AderDG::nonlinear::weights1;
double* generated::kernels::AderDG::nonlinear::weights2;
double* generated::kernels::AderDG::nonlinear::weights3;
double* generated::kernels::AderDG::nonlinear::weights4;
double* generated::kernels::AderDG::nonlinear::iweights1;
double* generated::kernels::AderDG::nonlinear::iweights3;
double* generated::kernels::AderDG::nonlinear::nodes;

void generated::kernels::AderDG::nonlinear::freeQuadratureNodesAndWeights() {
  _mm_free(weights1);
  _mm_free(weights2);
  _mm_free(weights3);
  _mm_free(weights4);
  _mm_free(iweights1);
  _mm_free(iweights3);
  _mm_free(nodes);
}

void generated::kernels::AderDG::nonlinear::initQuadratureNodesAndWeights() {
  weights1  = (double *) _mm_malloc(sizeof(double)*4, 16); //nDofPad
  weights2  = (double *) _mm_malloc(sizeof(double)*4, 16); //2D: nDofPad (==weight1), 3D: (nDof*nDof)Pad (== w1[i]*w1[j])
  weights3  = (double *) _mm_malloc(sizeof(double)*16, 16); //2D: (nDof*nDof)Pad (== w1[i]*w1[j]), 3D: (nDof*nDof*nDof)Pad (== w1[i]*w1[j]*w1[k])
  weights4  = (double *) _mm_malloc(sizeof(double)*64, 16); //2D: (nDof*nDof*nDof)Pad (== w1[i]*w1[j]*w1[k]), 3D: (nDof*nDof*nDof*nDof)Pad (== w1[i]*w1[j]*w1[k]*w1[l])
  iweights1 = (double *) _mm_malloc(sizeof(double)*4, 16); //nDofPad
  iweights3 = (double *) _mm_malloc(sizeof(double)*16, 16); //2D: (nDof*nDof)Pad (== w1[i]*w1[j]), 3D: (nDof*nDof*nDof)Pad (== w1[i]*w1[j]*w1[k])
  nodes     = (double *) _mm_malloc(sizeof(double)*4, 16);
  
  weights1[0]  = 1.739274225687269e-01;
  weights1[1]  = 3.260725774312730e-01;
  weights1[2]  = 3.260725774312730e-01;
  weights1[3]  = 1.739274225687269e-01;

  weights2[0]  = 1.739274225687269e-01;
  weights2[1]  = 3.260725774312730e-01;
  weights2[2]  = 3.260725774312730e-01;
  weights2[3]  = 1.739274225687269e-01;

  weights3[0]  = 3.025074832140050e-02;
  weights3[1]  = 5.671296296296296e-02;
  weights3[2]  = 5.671296296296296e-02;
  weights3[3]  = 3.025074832140050e-02;
  weights3[4]  = 5.671296296296296e-02;
  weights3[5]  = 1.063233257526736e-01;
  weights3[6]  = 1.063233257526736e-01;
  weights3[7]  = 5.671296296296296e-02;
  weights3[8]  = 5.671296296296296e-02;
  weights3[9]  = 1.063233257526736e-01;
  weights3[10]  = 1.063233257526736e-01;
  weights3[11]  = 5.671296296296296e-02;
  weights3[12]  = 3.025074832140050e-02;
  weights3[13]  = 5.671296296296296e-02;
  weights3[14]  = 5.671296296296296e-02;
  weights3[15]  = 3.025074832140050e-02;

  weights4[0]  = 5.261434686316431e-03;
  weights4[1]  = 9.863939474383817e-03;
  weights4[2]  = 9.863939474383817e-03;
  weights4[3]  = 5.261434686316431e-03;
  weights4[4]  = 9.863939474383817e-03;
  weights4[5]  = 1.849254200709766e-02;
  weights4[6]  = 1.849254200709766e-02;
  weights4[7]  = 9.863939474383817e-03;
  weights4[8]  = 9.863939474383817e-03;
  weights4[9]  = 1.849254200709766e-02;
  weights4[10]  = 1.849254200709766e-02;
  weights4[11]  = 9.863939474383817e-03;
  weights4[12]  = 5.261434686316431e-03;
  weights4[13]  = 9.863939474383817e-03;
  weights4[14]  = 9.863939474383817e-03;
  weights4[15]  = 5.261434686316431e-03;
  weights4[16]  = 9.863939474383817e-03;
  weights4[17]  = 1.849254200709766e-02;
  weights4[18]  = 1.849254200709766e-02;
  weights4[19]  = 9.863939474383817e-03;
  weights4[20]  = 1.849254200709766e-02;
  weights4[21]  = 3.466912086923912e-02;
  weights4[22]  = 3.466912086923912e-02;
  weights4[23]  = 1.849254200709766e-02;
  weights4[24]  = 1.849254200709766e-02;
  weights4[25]  = 3.466912086923912e-02;
  weights4[26]  = 3.466912086923912e-02;
  weights4[27]  = 1.849254200709766e-02;
  weights4[28]  = 9.863939474383817e-03;
  weights4[29]  = 1.849254200709766e-02;
  weights4[30]  = 1.849254200709766e-02;
  weights4[31]  = 9.863939474383817e-03;
  weights4[32]  = 9.863939474383817e-03;
  weights4[33]  = 1.849254200709766e-02;
  weights4[34]  = 1.849254200709766e-02;
  weights4[35]  = 9.863939474383817e-03;
  weights4[36]  = 1.849254200709766e-02;
  weights4[37]  = 3.466912086923912e-02;
  weights4[38]  = 3.466912086923912e-02;
  weights4[39]  = 1.849254200709766e-02;
  weights4[40]  = 1.849254200709766e-02;
  weights4[41]  = 3.466912086923912e-02;
  weights4[42]  = 3.466912086923912e-02;
  weights4[43]  = 1.849254200709766e-02;
  weights4[44]  = 9.863939474383817e-03;
  weights4[45]  = 1.849254200709766e-02;
  weights4[46]  = 1.849254200709766e-02;
  weights4[47]  = 9.863939474383817e-03;
  weights4[48]  = 5.261434686316431e-03;
  weights4[49]  = 9.863939474383817e-03;
  weights4[50]  = 9.863939474383817e-03;
  weights4[51]  = 5.261434686316431e-03;
  weights4[52]  = 9.863939474383817e-03;
  weights4[53]  = 1.849254200709766e-02;
  weights4[54]  = 1.849254200709766e-02;
  weights4[55]  = 9.863939474383817e-03;
  weights4[56]  = 9.863939474383817e-03;
  weights4[57]  = 1.849254200709766e-02;
  weights4[58]  = 1.849254200709766e-02;
  weights4[59]  = 9.863939474383817e-03;
  weights4[60]  = 5.261434686316431e-03;
  weights4[61]  = 9.863939474383817e-03;
  weights4[62]  = 9.863939474383817e-03;
  weights4[63]  = 5.261434686316431e-03;

  iweights1[0] = 5.749524630624896e+00;
  iweights1[1] = 3.066801899987349e+00;
  iweights1[2] = 3.066801899987349e+00;
  iweights1[3] = 5.749524630624896e+00;

  iweights3[0] = 3.305703347816235e+01;
  iweights3[1] = 1.763265306122449e+01;
  iweights3[2] = 1.763265306122449e+01;
  iweights3[3] = 3.305703347816235e+01;
  iweights3[4] = 1.763265306122449e+01;
  iweights3[5] = 9.405273893766010e+00;
  iweights3[6] = 9.405273893766010e+00;
  iweights3[7] = 1.763265306122449e+01;
  iweights3[8] = 1.763265306122449e+01;
  iweights3[9] = 9.405273893766010e+00;
  iweights3[10] = 9.405273893766010e+00;
  iweights3[11] = 1.763265306122449e+01;
  iweights3[12] = 3.305703347816235e+01;
  iweights3[13] = 1.763265306122449e+01;
  iweights3[14] = 1.763265306122449e+01;
  iweights3[15] = 3.305703347816235e+01;

  nodes[0]     = 6.943184420297371e-02;
  nodes[1]     = 3.300094782075719e-01;
  nodes[2]     = 6.699905217924281e-01;
  nodes[3]     = 9.305681557970263e-01;


}