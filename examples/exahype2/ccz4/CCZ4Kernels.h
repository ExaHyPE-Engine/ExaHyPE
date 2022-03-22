//
// ExaHyPE2 CCZ4 implementation
//
#ifndef _examples_exahype2_ccz4_CCZ4_Kernels_H_
#define _examples_exahype2_ccz4_CCZ4_Kernels_H_


#include <cmath>
#include "tarch/compiler/CompilerSpecificSettings.h"
#include "tarch/multicore/multicore.h"


namespace examples {
  namespace exahype2 {
    namespace ccz4 {
/*
      #if defined(OpenMPGPUOffloading)
      #pragma omp declare target
      #endif
*/
      /**
       * @see source() for a discussion of annotations.
       */
      #if defined(SharedOMP)
      #pragma omp declare simd
      #endif
      KeywordToAvoidDuplicateSymbolsForInlinedFunctions void ncp(double* BgradQ, const double* const Q, const double* const gradQSerialised, const int normal,
        const int CCZ4LapseType,
        const double CCZ4ds,
        const double CCZ4c,
        const double CCZ4e,
        const double CCZ4f,
        const double CCZ4bs,
        const double CCZ4sk,
        const double CCZ4xi,
        const double CCZ4mu
      ) InlineMethod;


      /**
       * The source term is one out of two terms that we use in our CCZ4
       * formulation. As it is used within the compute kernel, we also
       * want to use it on the GPU, and we want to vectorise over it
       * aggressively. To make this work, we have to inline the function
       * plus ensure that the interna of the function are not vectorised
       * before we inline the routine.
       *
       * So we first declare it as simd and then we also say explicitly
       * that it should be inlined. In my opinion, the inlining should
       * imply that we wanna use it as SIMD, but my guess is that the
       * simd statement ensures that the compiler doesn't prematurely
       * vectorise.
       *
       * @see ncp()
       */
      #if defined(SharedOMP)
      #pragma omp declare simd
      #endif
      KeywordToAvoidDuplicateSymbolsForInlinedFunctions void source(double* S, const double* const Q,
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
        const double CCZ4k3
      ) InlineMethod;


      /**
       * This is a postprocessing routine which ensures a posteriori that the
       * adm constraints are fulfilled. That is, we can read this as a local
       * limiter or post-smoother. It is mainly invoked through the Python
       * scripts, i.e. injected into the post-smoothing phase.
       */
      void admconstraints(double* constraints, const double* const Q, const double* const gradQSerialised);


      /**
       * I have no clue what this function does. @Han, can you write some docu
       * here?
       */
      void Psi4Calc(double* Psi4, const double* const Q, const double* const gradQSerialised, double* coor);


      /**
       * A postprocessing routine which pushes the volume solution back into the
       * area of the CCZ4 constraints. This is a local operation that we can
       * invoke after each time step per volume.
       */
      void enforceCCZ4constraints(double * luh);
    }
  }
}


#include "CCZ4Kernels.cpph"



#endif

