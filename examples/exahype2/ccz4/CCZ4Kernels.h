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
      void ncp(double* BgradQ, const double* const Q, const double* const gradQSerialised, const int normal,
        const int CCZ4LapseType,
        const double CCZ4ds,
        const double CCZ4c,
        const double CCZ4e,
        const double CCZ4f,
        const double CCZ4bs,
        const double CCZ4sk,
        const double CCZ4xi,
        const double CCZ4mu);
      #if defined(OpenMPGPUOffloading)
      #pragma omp end declare target
      #endif

      #if defined(OpenMPGPUOffloading)
      #pragma omp declare target
      #endif
      void source(double* S, const double* const Q,
        const int CCZ4LapseType,
        const double CCZ4ds,
        const double CCZ4c,
        const double CCZ4e,
        const double CCZ4f,
        const double CCZ4bs,
        const double CCZ4sk,
        const double CCZ4xi,
        const double CCZ4itau,
        const double CCZ4eta,
        const double CCZ4k1,
        const double CCZ4k2,
        const double CCZ4k3);
      #if defined(OpenMPGPUOffloading)
      #pragma omp end declare target
      #endif

      #if defined(OpenMPGPUOffloading)
      #pragma omp declare target
      #endif
      void admconstraints(double* constraints, const double* const Q, const double* const gradQSerialised);
      #if defined(OpenMPGPUOffloading)
      #pragma omp end declare target
      #endif

      //#if defined(OpenMPGPUOffloading)
      //#pragma omp declare target
      //#endif
      //void eigenvalues(double* lambda, const double* const Q, const int normal);
      //#if defined(OpenMPGPUOffloading)
      //#pragma omp end declare target
      //#endif
	
      #if defined(OpenMPGPUOffloading)
      #pragma omp declare target
      #endif
      void enforceCCZ4constraints(double * luh);
      #if defined(OpenMPGPUOffloading)
      #pragma omp end declare target
      #endif
    }
  }
}



#endif

