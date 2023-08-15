#pragma once


#include <vector>

#include "tarch/la/Vector.h"
#include "tarch/logging/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"


namespace applications {
  namespace exahype2 {
    namespace euler {
      namespace sphericalaccretion {
        /**
         * Set the initial overdensity profile as top hat.
         *
         * Within the radius of topHatRadius, we add some overdensity.
         * Everywhere else, no overdensity is applied, i.e. we stick to
         * baselineDensity. The overdensity is chosen such that the total
         * mass of the overdensity equals additionalMass but if and only
         * if we integrate it exactly. The very moment you discretise the
         * domain, you will get an under- or overapproximation.
         *
         * This initial configuration usually works fine for Finite
         * Volume-type methods which can resolve discontinuities. It
         * usually breaks down for higher order methods where the discontinuity
         * in the initial condition tends to induce oscillations. In this case,
         * you have to approximate the top hat with something smooth. We
         * usually use the Gaussian.
         *
         * @see initialiseOverdensity_Gaussian()
         */
        void initialiseOverdensity_topHat(
          double* __restrict__ Q,
          const tarch::la::Vector<Dimensions, double>& x,
          double                                       topHatRadius,
          double                                       additionalMass,
          double                                       baselineDensity,
          double                                       baselinePressure,
          double                                       gamma
        );


        /**
         * Baseline configuration where no overdensity is applied
         */
        void initialiseHomogeneousDensity(
          double* __restrict__ Q, double baselineDensity, double baselinePressure, double gamma
        );


        /**
         * Initialise overdensity according to Gaussian
         *
         * This routine computes the overdensity given the additionalMass and
         * then distributes it according to the Gaussian. Different to the top
         * hat function that is implemented in initialiseOverdensity_topHat(),
         * we have an extremely smooth and global mass distribution.
         *
         * To switch to a smooth global mass overdistribution is reasonable in
         * the context of higher order methods, where discontinuous initial
         * conditions yield oscillations in the absense of a limiter. A big
         * disadvantage of a non-local overdensity is that the mass integration
         * cannot be localised either.
         *
         * We recognise that our MNRAS paper discretises the shells, i.e. the
         * integration range, and that the underlying code in MassAccumulator
         * identifies which shells do make a significant contribution. Shells
         * far away from the centre of the overdensity are ignored. This
         * corresponds to a localisation of the Gaussian, i.e. its support is
         * again localised. The disadvantage of this truncation is that the
         * total mass is not equal to additionalMass anymore. Therefore, it
         * makes sense to slightly scale the mass distribution to preserve the
         * total mass again. I found a scaling of around 1.5 reasonable for
         * many experiments, but guess that a proper scaling in practice
         * depends on the size of the shells that you use, the domain size, the
         * integration order and the mesh width. So there might be some trial
         * and error involved.
         */
        void initialiseOverdensity_Gaussian(
          double* __restrict__ Q,
          const tarch::la::Vector<Dimensions, double>& x,
          double                                       topHatRadius,
          double                                       additionalMass,
          double                                       baselineDensity,
          double                                       baselinePressure,
          double                                       gamma
        );


        /**
         * Alternative to Gaussian
         *
         *
         * @see initialiseOverdensity_Gaussian()
         */
        void initialiseOverdensity_bumpFunction(
          double* __restrict__ Q,
          const tarch::la::Vector<Dimensions, double>& x,
          double                                       topHatRadius,
          double                                       additionalMass,
          double                                       baselineDensity,
          double                                       baselinePressure,
          double                                       gamma
        );


        /**
         * Alternative to Gaussian
         *
         *
         * @see initialiseOverdensity_Gaussian()
         */
        void initialiseOverdensity_hyperbolicSecant(
          double* __restrict__ Q,
          const tarch::la::Vector<Dimensions, double>& x,
          double                                       topHatRadius,
          double                                       additionalMass,
          double                                       baselineDensity,
          double                                       baselinePressure,
          double                                       gamma
        );


        void addGravitationalSource_AlphaCDM(
          double* __restrict__ S,
          const tarch::la::Vector<Dimensions, double>& x,
          const double* __restrict__ Q,
          double mass,
          double aInitial,
          double t
        );
      } // namespace sphericalaccretion
    }   // namespace euler
  }     // namespace exahype2
} // namespace applications
