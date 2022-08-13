#ifndef __APPLICATIONS_EXAHYPE2_EULER_SPHERICAL_ACCRETION_GRAVITY_MODEL_H__
#define __APPLICATIONS_EXAHYPE2_EULER_SPHERICAL_ACCRETION_GRAVITY_MODEL_H__


#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/logging/Log.h"
#include "tarch/la/Vector.h"
#include <vector>


namespace applications {
  namespace exahype2 {
    namespace euler {
      namespace sphericalaccretion {
        /**
         * Set the initial overdensity profile as top hat.
         */
        void initialiseOverdensity_topHat(
          double * __restrict__                        Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       topHatRadius,
          double                                       additionalMass,
          double                                       baselineDensity,
          double                                       baselinePressure,
          double                                       gamma
        );


        void initialiseOverdensity_Gaussian(
          double * __restrict__                        Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       topHatRadius,
          double                                       additionalMass,
          double                                       baselineDensity,
          double                                       baselinePressure,
          double                                       gamma
        );


        void initialiseOverdensity_bumpFunction(
          double * __restrict__                        Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       topHatRadius,
          double                                       additionalMass,
          double                                       baselineDensity,
          double                                       baselinePressure,
          double                                       gamma
        );


        void initialiseOverdensity_hyperbolicSecant(
          double * __restrict__                        Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       topHatRadius,
          double                                       additionalMass,
          double                                       baselineDensity,
          double                                       baselinePressure,
          double                                       gamma
        );


        void addGravitationalSource_AlphaCDM(
          double * __restrict__                        S,
          const tarch::la::Vector<Dimensions,double>&  x,
          const double * __restrict__                  Q,
          double                                       mass,
          double                                       aInitial,
          double                                       t
        );
      }
    }
  }
}


#endif
