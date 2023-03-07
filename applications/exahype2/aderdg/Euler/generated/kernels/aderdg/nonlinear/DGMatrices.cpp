
#include <mm_malloc.h> //g++

#include "./DGMatrices.h"

//DGMatrices
double* generated::kernels::AderDG::nonlinear::Kxi;
double* generated::kernels::AderDG::nonlinear::Kxi_T;
double* generated::kernels::AderDG::nonlinear::iK1_T;
double* generated::kernels::AderDG::nonlinear::dudx;
double* generated::kernels::AderDG::nonlinear::dudx_T;
double* generated::kernels::AderDG::nonlinear::FLCoeff;
double* generated::kernels::AderDG::nonlinear::FRCoeff;
double** generated::kernels::AderDG::nonlinear::fineGridProjector1d;
double** generated::kernels::AderDG::nonlinear::fineGridProjector1d_T_weighted;


void generated::kernels::AderDG::nonlinear::freeDGMatrices() {
  _mm_free(FLCoeff);
  _mm_free(FRCoeff);
  _mm_free(dudx);
  _mm_free(dudx_T);
  _mm_free(iK1_T);
  _mm_free(Kxi);
  _mm_free(Kxi_T);
  
  _mm_free(fineGridProjector1d[0]);
  _mm_free(fineGridProjector1d[1]);
  _mm_free(fineGridProjector1d[2]);
  delete [] fineGridProjector1d;
  
  _mm_free(fineGridProjector1d_T_weighted[0]);
  _mm_free(fineGridProjector1d_T_weighted[1]);
  _mm_free(fineGridProjector1d_T_weighted[2]);
  delete [] fineGridProjector1d_T_weighted;
}


void generated::kernels::AderDG::nonlinear::initDGMatrices() {
  
  FLCoeff = (double *) _mm_malloc(sizeof(double)*4, 16);
  FRCoeff = (double *) _mm_malloc(sizeof(double)*4, 16);
  //note: FLCoeff is also F0
  
  dudx    = (double *) _mm_malloc(sizeof(double)*16, 16);
  dudx_T  = (double *) _mm_malloc(sizeof(double)*16, 16);
  iK1_T   = (double *) _mm_malloc(sizeof(double)*16, 16);
  Kxi     = (double *) _mm_malloc(sizeof(double)*16, 16);
  Kxi_T   = (double *) _mm_malloc(sizeof(double)*16, 16);

  fineGridProjector1d            = new double* [3];
  fineGridProjector1d_T_weighted = new double* [3];
  for(int i=0; i<3; i++) {
    fineGridProjector1d[i]            = (double *) _mm_malloc(sizeof(double)*16, 16);
    fineGridProjector1d_T_weighted[i] = (double *) _mm_malloc(sizeof(double)*16, 16);
  }
  
  FLCoeff[0] = 1.526788125457266e+00;
  FLCoeff[1] = -8.136324494869271e-01;
  FLCoeff[2] = 4.007615203116503e-01;
  FLCoeff[3] = -1.139171962819899e-01;

  FRCoeff[0] = -1.139171962819898e-01;
  FRCoeff[1] = 4.007615203116500e-01;
  FRCoeff[2] = -8.136324494869263e-01;
  FRCoeff[3] = 1.526788125457266e+00;

  dudx[0] = -6.664000472704562e+00;
  dudx[1] = -1.515115229598468e+00;
  dudx[2] = 6.573964485165484e-01;
  dudx[3] = -1.161256338324529e+00;
  dudx[4] = 9.720308831370394e+00;
  dudx[5] = -7.688287844464170e-01;
  dudx[6] = -2.941340462561434e+00;
  dudx[7] = 4.217564696990359e+00;
  dudx[8] = -4.217564696990357e+00;
  dudx[9] = 2.941340462561433e+00;
  dudx[10] = 7.688287844464181e-01;
  dudx[11] = -9.720308831370390e+00;
  dudx[12] = 1.161256338324528e+00;
  dudx[13] = -6.573964485165482e-01;
  dudx[14] = 1.515115229598467e+00;
  dudx[15] = 6.664000472704561e+00;

  dudx_T[0] = -6.664000472704562e+00;
  dudx_T[1] = 9.720308831370394e+00;
  dudx_T[2] = -4.217564696990357e+00;
  dudx_T[3] = 1.161256338324528e+00;
  dudx_T[4] = -1.515115229598468e+00;
  dudx_T[5] = -7.688287844464170e-01;
  dudx_T[6] = 2.941340462561433e+00;
  dudx_T[7] = -6.573964485165482e-01;
  dudx_T[8] = 6.573964485165484e-01;
  dudx_T[9] = -2.941340462561434e+00;
  dudx_T[10] = 7.688287844464181e-01;
  dudx_T[11] = 1.515115229598467e+00;
  dudx_T[12] = -1.161256338324529e+00;
  dudx_T[13] = 4.217564696990359e+00;
  dudx_T[14] = -9.720308831370390e+00;
  dudx_T[15] = 6.664000472704561e+00;

  iK1_T[0] = 5.464353624196442e-01;
  iK1_T[1] = -1.443261832932572e-01;
  iK1_T[2] = 1.014623598288626e-01;
  iK1_T[3] = -6.687578310368546e-02;
  iK1_T[4] = 1.018853316771303e+00;
  iK1_T[5] = 5.847599728573240e-01;
  iK1_T[6] = -1.702637242678426e-01;
  iK1_T[7] = 1.014623598288636e-01;
  iK1_T[8] = 1.024010506693092e+00;
  iK1_T[9] = 1.000743778553195e+00;
  iK1_T[10] = 5.847599728573233e-01;
  iK1_T[11] = -1.443261832932576e-01;
  iK1_T[12] = 9.740050582643962e-01;
  iK1_T[13] = 1.024010506693091e+00;
  iK1_T[14] = 1.018853316771303e+00;
  iK1_T[15] = 5.464353624196443e-01;

  Kxi[0] = -1.159052426214283e+00;
  Kxi[1] = 1.690628261612287e+00;
  Kxi[2] = -7.335501572643867e-01;
  Kxi[3] = 2.019743218663828e-01;
  Kxi[4] = -4.940375280205473e-01;
  Kxi[5] = -2.506939833477959e-01;
  Kxi[6] = 9.590904657302993e-01;
  Kxi[7] = -2.143589543619561e-01;
  Kxi[8] = 2.143589543619561e-01;
  Kxi[9] = -9.590904657302998e-01;
  Kxi[10] = 2.506939833477962e-01;
  Kxi[11] = 4.940375280205472e-01;
  Kxi[12] = -2.019743218663829e-01;
  Kxi[13] = 7.335501572643869e-01;
  Kxi[14] = -1.690628261612286e+00;
  Kxi[15] = 1.159052426214282e+00;

  Kxi_T[0] = -1.159052426214283e+00;
  Kxi_T[1] = -4.940375280205473e-01;
  Kxi_T[2] = 2.143589543619561e-01;
  Kxi_T[3] = -2.019743218663829e-01;
  Kxi_T[4] = 1.690628261612287e+00;
  Kxi_T[5] = -2.506939833477959e-01;
  Kxi_T[6] = -9.590904657302998e-01;
  Kxi_T[7] = 7.335501572643869e-01;
  Kxi_T[8] = -7.335501572643867e-01;
  Kxi_T[9] = 9.590904657302993e-01;
  Kxi_T[10] = 2.506939833477962e-01;
  Kxi_T[11] = -1.690628261612286e+00;
  Kxi_T[12] = 2.019743218663828e-01;
  Kxi_T[13] = -2.143589543619561e-01;
  Kxi_T[14] = 4.940375280205472e-01;
  Kxi_T[15] = 1.159052426214282e+00;

  fineGridProjector1d[0][0] = 1.336580943538414e+00;
  fineGridProjector1d[0][1] = -5.106599509271045e-01;
  fineGridProjector1d[0][2] = 2.422582771987365e-01;
  fineGridProjector1d[0][3] = -6.817926981004674e-02;
  fineGridProjector1d[0][4] = 7.501737820555416e-01;
  fineGridProjector1d[0][5] = 3.503991256637167e-01;
  fineGridProjector1d[0][6] = -1.376638598070825e-01;
  fineGridProjector1d[0][7] = 3.709095208782415e-02;
  fineGridProjector1d[0][8] = 2.500683135132718e-01;
  fineGridProjector1d[0][9] = 9.137546431442426e-01;
  fineGridProjector1d[0][10] = -2.182390043731605e-01;
  fineGridProjector1d[0][11] = 5.441604771564604e-02;
  fineGridProjector1d[0][12] = 3.282922721362738e-02;
  fineGridProjector1d[0][13] = 1.010071394721412e+00;
  fineGridProjector1d[0][14] = -5.564103858754978e-02;
  fineGridProjector1d[0][15] = 1.274041665251057e-02;
  fineGridProjector1d[1][0] = -3.535004259641938e-02;
  fineGridProjector1d[1][1] = 9.710461986760909e-01;
  fineGridProjector1d[1][2] = 8.197886521853832e-02;
  fineGridProjector1d[1][3] = -1.767502129820969e-02;
  fineGridProjector1d[1][4] = -9.286838847674270e-02;
  fineGridProjector1d[1][5] = 7.760907833371602e-01;
  fineGridProjector1d[1][6] = 3.880453916685798e-01;
  fineGridProjector1d[1][7] = -7.126778652899733e-02;
  fineGridProjector1d[1][8] = -7.126778652899739e-02;
  fineGridProjector1d[1][9] = 3.880453916685800e-01;
  fineGridProjector1d[1][10] = 7.760907833371599e-01;
  fineGridProjector1d[1][11] = -9.286838847674268e-02;
  fineGridProjector1d[1][12] = -1.767502129820965e-02;
  fineGridProjector1d[1][13] = 8.197886521853816e-02;
  fineGridProjector1d[1][14] = 9.710461986760909e-01;
  fineGridProjector1d[1][15] = -3.535004259641930e-02;
  fineGridProjector1d[2][0] = 1.274041665251054e-02;
  fineGridProjector1d[2][1] = -5.564103858754964e-02;
  fineGridProjector1d[2][2] = 1.010071394721412e+00;
  fineGridProjector1d[2][3] = 3.282922721362726e-02;
  fineGridProjector1d[2][4] = 5.441604771564604e-02;
  fineGridProjector1d[2][5] = -2.182390043731605e-01;
  fineGridProjector1d[2][6] = 9.137546431442430e-01;
  fineGridProjector1d[2][7] = 2.500683135132715e-01;
  fineGridProjector1d[2][8] = 3.709095208782417e-02;
  fineGridProjector1d[2][9] = -1.376638598070826e-01;
  fineGridProjector1d[2][10] = 3.503991256637167e-01;
  fineGridProjector1d[2][11] = 7.501737820555415e-01;
  fineGridProjector1d[2][12] = -6.817926981004679e-02;
  fineGridProjector1d[2][13] = 2.422582771987366e-01;
  fineGridProjector1d[2][14] = -5.106599509271045e-01;
  fineGridProjector1d[2][15] = 1.336580943538415e+00;

  fineGridProjector1d_T_weighted[0][0] = 4.455269811794714e-01;
  fineGridProjector1d_T_weighted[0][1] = 4.687991788443810e-01;
  fineGridProjector1d_T_weighted[0][2] = 1.562728834761402e-01;
  fineGridProjector1d_T_weighted[0][3] = 1.094307573787579e-02;
  fineGridProjector1d_T_weighted[0][4] = -9.079550098274686e-02;
  fineGridProjector1d_T_weighted[0][5] = 1.167997085545722e-01;
  fineGridProjector1d_T_weighted[0][6] = 3.045848810480808e-01;
  fineGridProjector1d_T_weighted[0][7] = 1.795910138352788e-01;
  fineGridProjector1d_T_weighted[0][8] = 4.307359840054563e-02;
  fineGridProjector1d_T_weighted[0][9] = -4.588795326902750e-02;
  fineGridProjector1d_T_weighted[0][10] = -7.274633479105348e-02;
  fineGridProjector1d_T_weighted[0][11] = -9.892994280411245e-03;
  fineGridProjector1d_T_weighted[0][12] = -2.272642327001558e-02;
  fineGridProjector1d_T_weighted[0][13] = 2.317890640443743e-02;
  fineGridProjector1d_T_weighted[0][14] = 3.400571853517907e-02;
  fineGridProjector1d_T_weighted[0][15] = 4.246805550836856e-03;
  fineGridProjector1d_T_weighted[1][0] = -1.178334753213979e-02;
  fineGridProjector1d_T_weighted[1][1] = -5.803538499999801e-02;
  fineGridProjector1d_T_weighted[1][2] = -4.453672016009882e-02;
  fineGridProjector1d_T_weighted[1][3] = -5.891673766069885e-03;
  fineGridProjector1d_T_weighted[1][4] = 1.726523216205243e-01;
  fineGridProjector1d_T_weighted[1][5] = 2.586969277790534e-01;
  fineGridProjector1d_T_weighted[1][6] = 1.293484638895267e-01;
  fineGridProjector1d_T_weighted[1][7] = 1.457586819565721e-02;
  fineGridProjector1d_T_weighted[1][8] = 1.457586819565724e-02;
  fineGridProjector1d_T_weighted[1][9] = 1.293484638895266e-01;
  fineGridProjector1d_T_weighted[1][10] = 2.586969277790533e-01;
  fineGridProjector1d_T_weighted[1][11] = 1.726523216205243e-01;
  fineGridProjector1d_T_weighted[1][12] = -5.891673766069896e-03;
  fineGridProjector1d_T_weighted[1][13] = -4.453672016009878e-02;
  fineGridProjector1d_T_weighted[1][14] = -5.803538499999800e-02;
  fineGridProjector1d_T_weighted[1][15] = -1.178334753213977e-02;
  fineGridProjector1d_T_weighted[2][0] = 4.246805550836847e-03;
  fineGridProjector1d_T_weighted[2][1] = 3.400571853517908e-02;
  fineGridProjector1d_T_weighted[2][2] = 2.317890640443744e-02;
  fineGridProjector1d_T_weighted[2][3] = -2.272642327001560e-02;
  fineGridProjector1d_T_weighted[2][4] = -9.892994280411221e-03;
  fineGridProjector1d_T_weighted[2][5] = -7.274633479105348e-02;
  fineGridProjector1d_T_weighted[2][6] = -4.588795326902754e-02;
  fineGridProjector1d_T_weighted[2][7] = 4.307359840054565e-02;
  fineGridProjector1d_T_weighted[2][8] = 1.795910138352788e-01;
  fineGridProjector1d_T_weighted[2][9] = 3.045848810480810e-01;
  fineGridProjector1d_T_weighted[2][10] = 1.167997085545722e-01;
  fineGridProjector1d_T_weighted[2][11] = -9.079550098274686e-02;
  fineGridProjector1d_T_weighted[2][12] = 1.094307573787575e-02;
  fineGridProjector1d_T_weighted[2][13] = 1.562728834761400e-01;
  fineGridProjector1d_T_weighted[2][14] = 4.687991788443810e-01;
  fineGridProjector1d_T_weighted[2][15] = 4.455269811794715e-01;

}