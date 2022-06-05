#pragma once

    struct patchData2d
    {
      double*  reco;
      int*     id;
      double* x0; double* h0; double* x1; double* h1;
      double* t; double* dt;
      int      npatches, nsrc, ndst;
      double*  result;
      // out
      double*  maxEigenvalue;
    } ;

    struct patchData3d
    {
      double*  reco;
      int*     id;
      double* x0; double* h0; double* x1; double* h1; double* x2; double* h2;
      double* t; double* dt;
      int      npatches, nsrc, ndst;
      double*  result;
      // out
      double*  maxEigenvalue;
    } ;
