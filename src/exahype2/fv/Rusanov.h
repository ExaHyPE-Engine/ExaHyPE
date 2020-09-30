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
      std::function< double(
        double                                       Q[],
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal
      ) >   eigenvalues,
      double QL[],
      double QR[],
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       dx,
      double                                       t,
      double                                       dt,
      int                                          normal,
      int                                          unknowns,
      int                                          auxiliaryVariables,
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
          std::function< double(
                  double                                       Q[],
                  const tarch::la::Vector<Dimensions,double>&  faceCentre,
                  const tarch::la::Vector<Dimensions,double>&  volumeH,
                  double                                       t,
                  double                                       dt,
                  int                                          normal
          ) >   eigenvalues, 
          double QL[],
          double QR[],
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       dx,
          double                                       t,
          double                                       dt,
          int                                          normal,
          int                                          unknowns,
          int                                          auxiliaryVariables,
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
      template< typename Flux, typename NCP, typename MaxEigenvalue>
      void splitRusanov1d(
        Flux flux,
        NCP  nonconservativeProduct,
        MaxEigenvalue maxEigenvalue,
        double QL[],
        double QR[],
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       dx,
        double                                       t,
        double                                       dt,
        int                                          normal,
        int                                          unknowns,
        int                                          auxiliaryVariables,
        double                                       FL[],
        double                                       FR[]
      );
      #if defined(GPUOffloading)
      #pragma omp end declare target
      #endif
  }
}


//#include "Rusanov.cpph"

#endif
