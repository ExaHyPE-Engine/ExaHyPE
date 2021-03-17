//
// ExaHyPE2 CCZ4 implementation
//
#ifndef _examples_exahype2_ccz4_CCZ4_Kernels_H_
#define _examples_exahype2_ccz4_CCZ4_Kernels_H_


namespace examples {
  namespace exahype2 {
    namespace ccz4 {
      #pragma omp declare target
      void ncp(double* BgradQ, const double* const Q, const double* const gradQSerialised, const int normal);
      #pragma omp end declare target
	
      #pragma omp declare target
      void source(double* S, const double* const Q);
      #pragma omp end declare target
	  
      #pragma omp declare target
      void eigenvalues(double* lambda, const double* const Q, const int normal);
      #pragma omp end declare target
	
      #pragma omp declare target
      void enforceCCZ4constraints(double * luh);
      #pragma omp end declare target
    }
  }
}



#endif

