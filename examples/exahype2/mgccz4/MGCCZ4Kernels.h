//
// ExaHyPE2 MGCCZ4 implementation
//
#ifndef _examples_exahype2_mgccz4_MGCCZ4_Kernels_H_
#define _examples_exahype2_mgccz4_MGCCZ4_Kernels_H_


namespace examples {
  namespace exahype2 {
    namespace mgccz4 {
      #if defined(OpenMPGPUOffloading)
      #pragma omp declare target
      #endif
      void ncp(double* BgradQ, const double* const Q, const double* const gradQSerialised, const int normal,
        const int MGCCZ4LapseType,
        const double MGCCZ4ds,
        const double MGCCZ4c,
        const double MGCCZ4e,
        const double MGCCZ4f,
        const double MGCCZ4bs,
        const double MGCCZ4sk,
        const double MGCCZ4xi,
        const double MGCCZ4mu);
      #if defined(OpenMPGPUOffloading)
      #pragma omp end declare target
      #endif

      #if defined(OpenMPGPUOffloading)
      #pragma omp declare target
      #endif
      void source(double* S, const double* const Q,
        const int MGCCZ4LapseType,
        const double MGCCZ4ds,
        const double MGCCZ4c,
        const double MGCCZ4e,
        const double MGCCZ4f,
        const double MGCCZ4bs,
        const double MGCCZ4sk,
        const double MGCCZ4xi,
        const double MGCCZ4itau,
        const double MGCCZ4eta,
        const double MGCCZ4k1,
        const double MGCCZ4k2,
        const double MGCCZ4k3);
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
      void enforceMGCCZ4constraints(double * luh);
      #if defined(OpenMPGPUOffloading)
      #pragma omp end declare target
      #endif
    }
  }
}



#endif

