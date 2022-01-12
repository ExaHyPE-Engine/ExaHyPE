// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FV_RUSANOV_H_
#define _EXAHYPE2_FV_RUSANOV_H_



#include "../PatchUtils.h"
#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"
#include "tarch/multicore/multicore.h"


#include <functional>
#include <algorithm>

namespace exahype2 {
  namespace fv {

    struct patchData2d
    {
      double*  reco;
      int*     id;
      double* x0; double* h0; double* x1; double* h1;
      double* t; double* dt;
      int      npatches;
      double*  result;
      // out
      double*  newDt;
    } ;

    struct patchData3d
    {
      double*  reco;
      int*     id;
      double* x0; double* h0; double* x1; double* h1; double* x2; double* h2;
      double* t; double* dt;
      int      npatches;
      double*  result;
      // out
      double*  newDt;
    } ;



    template<
      int                                          numVPAIP, // numberofVolumesPerAxisInPatch
      int                                          unknowns,
      int                                          auxiliaryVariables,
      typename SOLVER,
      bool                                         skipFluxEvaluation,
      bool                                         skipNCPEvaluation,
      bool                                         skipSourceTerm,
      bool                                         skipNewTimeStepComputation
      >
    void Fusanov_2D_with_OpenMP_offloading(
      const int       haloSize,
      patchData2d     pV,
      const int       sourcePatchSize,
      const int       destPatchSize,
      int             targetDevice
    );



    /**
     * @see Fusanov_3D_with_inlining()
     */
    template<
      int      numVPAIP, // numberofVolumesPerAxisInPatch
      int      unknowns,
      int      auxiliaryVariables,
      typename SOLVER,
      bool     skipFluxEvaluation,
      bool     skipNCPEvaluation,
      bool                                         skipSourceTerm,
      bool                                         skipNewTimeStepComputation
      >
    void Fusanov_2D_with_batching(
      const int       haloSize,
      patchData2d     pV,
      const int       sourcePatchSize,
      const int       destPatchSize
    );


    /**
     * Fused implementation of the Rusanov solver, i.e. one kernel over
     * multiple patches. This variant of the code is optimised towards
     * OpenMP target offloading.
     *
     * @todo Holger can you please add more docu and remove the CCZ4-specific
     * data offloads?
     */
    template<
      int                                          numVPAIP, //numberofVolumesPerAxisInPatch,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      typename SOLVER,
      bool                                         skipFluxEvaluation,
      bool                                         skipNCPEvaluation,
      bool                                         skipSourceTerm,
      bool                                         skipNewTimeStepComputation
    >
    void Fusanov_3D_with_OpenMP_offloading(
      int                                          haloSize,
      patchData3d                                  pV,
      int                                          sourcePatchSize,
      int                                          destPatchSize,
      int             targetDevice
    );


    /**
     * Variant of the 3d optimised, fused kernel which is tailored to the CPU.
     * The kernel tries to collapse all loops. There's limited SIMD concurrency
     * here, but at least some core routines are vectorised.
     *
     * The kernel tries to collapse loops and uses SIMD where possible. However,
     * it does not use any parallel for. We found that such nested parallelism slows down the code massively or even introduces
     * deadlocks. It also makes no sense, as we always issue fused tasks within OpenMP
     * tasks, and as OpenMP tasks so far cannot span multiple cores/threads. So we
     * may rely on vectorisation but do not introduce any SIMT-like parallelism
     * for this fused variant, while the GPU variant operates exactly the other
     * way round.
     *
     * ## Copy solution
     *
     * This part is simple. Indeed, we can collapse all five nested loops. It is
     * important to collapse, as the collapsing allows the compiler to reorder
     * the loop ranges and, hence, to ensure that all data accesses are continuous.
     *
     * ## Source term
     *
     * This part is skipped completely if there is no source term.
     * @todo
     *
     * ## Flux terms
     *
     * We assume that we always have a conservative flux or an ncp term. Therefore,
     * we always run through this loop and make the flux vs ncp decision within the
     * loop.
     *
     * We found that there's no way to collapse the flux loops. We have played
     * around quite a lot with instructions alike
     *
     *       #pragma omp simd collapse(4) safelen(numVPAIP*numVPAIP)
     *
     * None of them gave us a vectorised code. Even once we had eliminated the key
     * challenge prohibiting vectorisation:
     *
     * If we vectorise over the faces, we have gather data from the left and right
     * cell of a face. Left and right are likely not continuous in memory. So we
     * have a gather here. That seems
     *
     * ## Optimisation aspect within user solvers
     *
     * All solver calls are explicitly inlined, though the corresponding Clang
     * pragma seems to be widely ignored. We found that the 2022 versions of Intel's
     * Clang tools require the flux, ncp, source and eigenvalue functions to be
     * marked with the attribute
     *
     *      __attribute__((force_inline))
     *
     * If this attribute is missing, the compiler does not manage to inline. For
     * whatever reason. Might be that we need some fancy ipo magic, but then this is
     * weird as we work within templates and with header files here.
     *
     * @see Fusanov_2D() for details on the CPU optimisation.
     */
    template<
      int                                          numVPAIP, //numberofVolumesPerAxisInPatch,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      typename SOLVER,
      bool                                         skipFluxEvaluation,
      bool                                         skipNCPEvaluation,
      bool                                         skipSourceTerm,
      bool                                         skipNewTimeStepComputation
    >
    void Fusanov_3D_with_batching(
      int                                          haloSize,
      patchData3d                                  pV,
      int                                          sourcePatchSize,
      int                                          destPatchSize
    );


    //*
     //The routine has exactly the same semantics as applySplit1DRiemannToPatch_Overlap1AoS2d()
     //but runs through the faces slightly differently.

     //@image html applySplit1DRiemannToPatch_Overlap1AoS2d_SplitLoop.png

     //As in applySplit1DRiemannToPatch_Overlap1AoS2d(), we have two big
     //blocks. The first one runs through all vertical faces, the second one
     //through all horizontal ones. Each block now runs through the whole data
     //twice though with a spacing of two and an offset of 0 or 1. That is,
     //we run through every second face (faces 0, 2, 4 along the x-axis) and
     //then through the other ones (1,3,5,...).

     //Each face writes to its left and right adjacent volume in Qout. As we
     //skip every second face, we know that all writes are parallel. No race
     //conditions can arise. That is: Though we now run through the data structure
     //twice, we can process the for loops embarassingly parallel.
     //
     //
     //
     // NOTE: we decided to have a dedicated split loop implementation for the
     // Rusanov solver as all attempts to offload the original bit using functors failed.
     // One benefit here is that the helper variables are on the stack.
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    template<
      int                                          numVPAIP, // numberofVolumesPerAxisInPatch
      int                                          unknowns,
      int                                          auxiliaryVariables,
      typename SOLVER>
    void applySplit1DRiemannToPatch_Overlap1AoS2d_SplitLoop_Rusanov(
      const tarch::la::Vector<Dimensions,double>&  thepatchCentre,
      const tarch::la::Vector<Dimensions,double>&  thepatchSize,
      double                                       t,
      double                                       dt,
      const double * __restrict__                  Qin,
      double * __restrict__                        Qout
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif


    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    template<
      int                                          numVPAIP,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      typename SOLVER>
    void applySplit1DRiemannToPatch_Overlap1AoS3d_SplitLoop_Rusanov(
      const tarch::la::Vector<Dimensions,double>&  thepatchCentre,
      const tarch::la::Vector<Dimensions,double>&  thepatchSize,
      double                                       t,
      double                                       dt,
      const double * __restrict__                  Qin,
      double * __restrict__                        Qout
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif

    /**
     * Extension of standard Rusanov1d. This one also supports non-conservative fluxes.
     */
    void splitRusanov1d(
      std::function< void(
        const double * __restrict__ Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__ F
      ) >   flux,
      std::function< void(
        const double * __restrict__                  Q,
        const double * __restrict__     deltaQ,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__ BgradQ
      ) >   nonconservativeProduct,
      std::function< double(
        const double * __restrict__ Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal
      ) >   eigenvalues,
      const double * __restrict__ QL,
      const double * __restrict__ QR,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       dx,
      double                                       t,
      double                                       dt,
      int                                          normal,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      double * __restrict__     FL,
      double * __restrict__     FR,
      bool skipFluxEvaluation   = false,
      bool skipNCPEvaluation    = false
    );
  }
}


#include "Rusanov.cpph"


#endif
