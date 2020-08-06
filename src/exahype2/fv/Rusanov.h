// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FV_RUSANOV_H_
#define _EXAHYPE2_FV_RUSANOV_H_



#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"
#include "tarch/multicore/multicore.h"


#include <functional>


namespace exahype2 {
  namespace fv {
    /**
     * Apply simple Rusanov with dimensional splitting
     *
     * We run over the mesh and compute the fluxes through every single face as
     * a 1d problem according to the Rusanov flux
     * @f$
         F(x) = F( \frac{1}{2} (Q_L+Q_R) ) - \lambda_{\mbox{max}} (Q_R - Q_L)
       @f$
     *
     * where @f$ \lambda_{\mbox{max}} @f$  is the maximum over the eigenvalues both of @f$ Q_R @f$  and @f$ Q_L @f$.
     *
     */
    void applyRusanovToPatch_FaceLoops(
      std::function< void(
        double                                       Q[],
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double                                       F[]
      ) >   flux,
      std::function< void(
        double                                       Q[],
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double                                       lambdas[]
      ) >   eigenvalues,
      const tarch::la::Vector<Dimensions,double>&  patchCentre,
      const tarch::la::Vector<Dimensions,double>&  patchSize,
      double                                       t,
      double                                       dt,
      int                                          numberOfVolumesPerAxisInPatch,
      int                                          unknowns,
      double                                       Qin[],
      double                                       Qout[]
    );


    /**
     * This variant of Rusanov does support non-conversative (ncp) products
     */
    void applyRusanovToPatch_FaceLoops(
      std::function< void(
        double                                       Q[],
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double                                       F[]
      ) >   flux,
      std::function< void(
        double                                       Q[],
        double                                       gradQ[][Dimensions],
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double                                       BgradQ[]
      ) >   nonconservativeProduct,
      std::function< void(
        double                                       Q[],
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double                                       lambdas[]
      ) >   eigenvalues,
      const tarch::la::Vector<Dimensions,double>&  patchCentre,
      const tarch::la::Vector<Dimensions,double>&  patchSize,
      double                                       t,
      double                                       dt,
      int                                          numberOfVolumesPerAxisInPatch,
      int                                          unknowns,
      double                                       Qin[],
      double                                       Qout[]
    );


    namespace gpu {
      #if defined(GPUOffloading)
      #pragma omp declare target
      #endif
      template <typename Flux, typename Eigenvalues>
      void applyRusanovToPatch_FaceLoops(
        Flux                                         flux,
        Eigenvalues                                  eigenvalues,
        const tarch::la::Vector<Dimensions,double>&  patchCentre,
        const tarch::la::Vector<Dimensions,double>&  patchSize,
        double                                       t,
        double                                       dt,
        int                                          numberOfVolumesPerAxisInPatch,
        int                                          unknowns,
        double                                       Qin[],
        double                                       Qout[]
      );


      template <typename Flux, typename NCP, typename Eigenvalues>
      void applyRusanovToPatch_FaceLoops(
        Flux                                         flux,
        NCP                                          nonconservativeProduct,
        Eigenvalues                                  eigenvalues,
        const tarch::la::Vector<Dimensions,double>&  patchCentre,
        const tarch::la::Vector<Dimensions,double>&  patchSize,
        double                                       t,
        double                                       dt,
        int                                          numberOfVolumesPerAxisInPatch,
        int                                          unknowns,
        double                                       Qin[],
        double                                       Qout[]
      );
       #if defined(GPUOffloading)
       #pragma omp end declare target
       #endif
    }

    namespace internal {
      /**
       * 1d Riemann accepting a flux and eigenvalue function.
       */
      void splitRusanov1d(
        std::function< void(
              double                                       Q[],
              const tarch::la::Vector<Dimensions,double>&  faceCentre,
              const tarch::la::Vector<Dimensions,double>&  volumeH,
              double                                       t,
              double                                       dt,
              int                                          normal,
              double                                       F[]
        ) >   flux,
        std::function< void(
              double                                       Q[],
              const tarch::la::Vector<Dimensions,double>&  faceCentre,
              const tarch::la::Vector<Dimensions,double>&  volumeH,
              double                                       t,
              double                                       dt,
              int                                          normal,
              double                                       lambdas[]
        ) >   eigenvalues,
        double QL[],
        double QR[],
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       dx,
        double                                       t,
        double                                       dt,
        int                                          normal,
        int                                          unknowns,
        double                                       FL[],
        double                                       FR[]
      );

      /**
       * Extension of standard Rusanov1d. This one also supports non-conservative fluxes.
       */
      void splitRusanov1d(
          std::function< void(
                  double                                       Q[],
                  const tarch::la::Vector<Dimensions,double>&  faceCentre,
                  const tarch::la::Vector<Dimensions,double>&  volumeH,
                  double                                       t,
                  double                                       dt,
                  int                                          normal,
                  double                                       F[]
          ) >   flux,
          std::function< void(
              double                                       Q[],
              double                                       gradQ[][Dimensions],
              const tarch::la::Vector<Dimensions,double>&  faceCentre,
              const tarch::la::Vector<Dimensions,double>&  volumeH,
              double                                       t,
              double                                       dt,
              int                                          normal,
              double                                       BgradQ[]
            ) >   nonconservativeProduct,
          std::function< void(
                  double                                       Q[],
                  const tarch::la::Vector<Dimensions,double>&  faceCentre,
                  const tarch::la::Vector<Dimensions,double>&  volumeH,
                  double                                       t,
                  double                                       dt,
                  int                                          normal,
                  double                                       lambdas[]
          ) >   eigenvalues, 
          double QL[],
          double QR[],
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       dx,
          double                                       t,
          double                                       dt,
          int                                          normal,
          int                                          unknowns,
          double                                       FL[],
          double                                       FR[]
      );
    }

    namespace gpu {
	    // @todo docu: Would like to have it in the cpp file. Docu in 
	    // report that lambdas improves template thing and now we are 
	    // back
	    //
	    // Big summary: Codes becomes uglier as all ends up in headerfiles
      /**
	     * Do this one only for the most generic variant. Doesn't matter, 
	     * as it is a template, so compiler will filter out.
	     *
	     *
	     * @todo Tons of docu that solver routines have to be stateless
	     * different to original
       */
      #if defined(GPUOffloading)
      #pragma omp declare target
      #endif
      template< typename Flux, typename NCP, typename Eigenvalues>
      void splitRusanov1d(
        Flux flux,
        NCP  nonconservativeProduct,
        Eigenvalues eigenvalues,
        double QL[],
        double QR[],
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       dx,
        double                                       t,
        double                                       dt,
        int                                          normal,
        int                                          unknowns,
        double                                       FL[],
        double                                       FR[]
      );
      #if defined(GPUOffloading)
      #pragma omp end declare target
      #endif
    }
  }
}


#include "Rusanov.cpph"

#endif
