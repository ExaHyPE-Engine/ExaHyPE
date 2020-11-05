#ifndef __INITIAL_DATA_ADAPTER_CPP_FORTRAN_MHD__
#define __INITIAL_DATA_ADAPTER_CPP_FORTRAN_MHD__


extern "C" {

// FORTRAN functions called by C
//void initialdata_(const double* x, const double* const t, double* Q, int* md, double * cms, const int* const order);
  void initialdata_(const double* x, const double* const t, double* Q);
void initparameters_(const int*  length, const char* parsetup);


// only initialdata_ is used, no more.
void pdelimitervalue_(int* limiter_value, const double* xx,const int* const numberOfObservables, const double* const observablesMin,const double* const observablesMax);
}/* extern "C" */
#endif /* __INITIAL_DATA_ADAPTER_CPP_FORTRAN_MHD__ */
