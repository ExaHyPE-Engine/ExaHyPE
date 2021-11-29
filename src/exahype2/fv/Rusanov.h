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
      double*  reco; int* id; double* x0; double* h0; double* x1; double*
        h1; double* t; double* dt; int npatches; double * result;
    } ;

    struct patchData3d
    {
      double*  reco; int* id; double* x0; double* h0; double* x1; double*
        h1; double* x2; double* h2; double* t; double* dt; int npatches;
      double * result;
    } ;



    template<
      int                                          numVPAIP, // numberofVolumesPerAxisInPatch
      int                                          unknowns,
      int                                          auxiliaryVariables,
      typename SOLVER,
      bool                                         skipFluxEvaluation,
      bool                                         skipNCPEvaluation,
      bool                                         skipSourceTerm
      >
    void Fusanov_2D(
      const int       haloSize,
      patchData2d     pV,
      const int       sourcePatchSize,
      const int       destPatchSize,
      int             targetDevice
    );



    /**
     * Fused kernel that supports execution on CPUs. The kernel tries to collapse all
     * the loops where only possible. If you work with OpenMP, the kernel tries to use
     * aggressive SIMD, too.
     *
     */
    template<
      int      numVPAIP, // numberofVolumesPerAxisInPatch
      int      unknowns,
      int      auxiliaryVariables,
      typename SOLVER,
      bool     skipFluxEvaluation,
      bool     skipNCPEvaluation,
      bool     skipSourceTerm
      >
    void Fusanov_2D(
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
      bool                                         skipSourceTerm
    >
    void Fusanov_3D(
      int                                          haloSize,
      patchData3d                            pV,
      int                                          sourcePatchSize,
      int                                          destPatchSize,
      int             targetDevice
    );


    /**
     * Variant of the 3d optimised, fused kernel which is tailored to the CPU.
     * The kernel tries to collapse all loops and makes aggressive use of OpenMP
     * SIMDsation.
     */
    template<
      int                                          numVPAIP, //numberofVolumesPerAxisInPatch,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      typename SOLVER,
      bool                                         skipFluxEvaluation,
      bool                                         skipNCPEvaluation,
      bool                                         skipSourceTerm
    >
    void Fusanov_3D(
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
