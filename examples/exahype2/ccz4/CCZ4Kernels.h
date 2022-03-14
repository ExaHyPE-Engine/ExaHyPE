//
// ExaHyPE2 CCZ4 implementation
//
#ifndef _examples_exahype2_ccz4_CCZ4_Kernels_H_
#define _examples_exahype2_ccz4_CCZ4_Kernels_H_


#include <cmath>


namespace examples {
  namespace exahype2 {
    namespace ccz4 {
/*
      #if defined(OpenMPGPUOffloading)
      #pragma omp declare target
      #endif
*/
      static void ncp(double* BgradQ, const double* const Q, const double* const gradQSerialised, const int normal,
        const int CCZ4LapseType,
        const double CCZ4ds,
        const double CCZ4c,
        const double CCZ4e,
        const double CCZ4f,
        const double CCZ4bs,
        const double CCZ4sk,
        const double CCZ4xi,
        const double CCZ4mu);
/*
      #if defined(OpenMPGPUOffloading)
      #pragma omp end declare target
      #endif

      #if defined(OpenMPGPUOffloading)
      #pragma omp declare target
      #endif
*/

      /**
       * This routine is marked as static, as we provide it within the underlying
       * ccph file. That is, we basically hold in the header. Consequently, we get
       * multiple defined instances errors if we omit the static keyword. If we add
       * it, all works, but we get compiler warnings.
       */
      static void source(double* S, const double* const Q,
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
/*
      #if defined(OpenMPGPUOffloading)
      #pragma omp end declare target
      #endif

      #if defined(OpenMPGPUOffloading)
      #pragma omp declare target
      #endif
*/
      static void admconstraints(double* constraints, const double* const Q, const double* const gradQSerialised);

/*
      #if defined(OpenMPGPUOffloading)
      #pragma omp end declare target
      #endif

      #if defined(OpenMPGPUOffloading)
      #pragma omp declare target
      #endif
      #if defined(InlineCCZ4Kernels)
      static
      #endif
*/
      static void Psi4Calc(double* Psi4, const double* const Q, const double* const gradQSerialised, double* coor);
/*
      #if defined(OpenMPGPUOffloading)
      #pragma omp end declare target
      #endif
*/

      //#if defined(OpenMPGPUOffloading)
      //#pragma omp declare target
      //#endif
      //void eigenvalues(double* lambda, const double* const Q, const int normal);
      //#if defined(OpenMPGPUOffloading)
      //#pragma omp end declare target
      //#endif
	
/*
      #if defined(OpenMPGPUOffloading)
      #pragma omp declare target
      #endif
      #if defined(InlineCCZ4Kernels)
      static
      #endif
*/
      void enforceCCZ4constraints(double * luh);
/*
      #if defined(OpenMPGPUOffloading)
      #pragma omp end declare target
      #endif
*/
    }
  }
}


#include "CCZ4Kernels.cpph"



#endif

