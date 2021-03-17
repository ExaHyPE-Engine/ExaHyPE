//
// ExaHyPE2 CCZ4 implementation
//
#ifndef _examples_exahype2_ccz4_CCZ4_Kernels_H_
#define _examples_exahype2_ccz4_CCZ4_Kernels_H_


namespace examples {
  namespace exahype2 {
    namespace ccz4 {
      #if defined(OpenMPGPUOffloading)
      #pragma omp declare target
      #endif
      void ncp(double* BgradQ, const double* const Q, const double* const gradQSerialised, const int normal);

      void source(double* S, const double* const Q);
	  
      void eigenvalues(double* lambda, const double* const Q, const int normal);
	
      void enforceCCZ4constraints(double * luh);
      #if defined(OpenMPGPUOffloading)
      #pragma omp end declare target
      #endif
    }
  }
}



#endif

