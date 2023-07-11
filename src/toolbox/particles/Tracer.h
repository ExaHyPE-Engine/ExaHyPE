// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/datamanagement/CellMarker.h"
#include "peano4/utils/Globals.h"

// #include <concepts>

namespace toolbox {
  namespace particles {
    /**
     * Leave the tracer particles where they are.
     *
     * Essentially, this is a complicated way to write down a
     * ``not an operation''  (nope). The routine simply does nothing with
     * the particle passed.
     */
    template <typename Particle>
    void staticPosition(const Particle& particle, double timeStepSize) {}

    /**
     *
     */
    template <typename ParticleAccessor>
    concept ParticleTimeSteppingAccessor = requires { typename ParticleAccessor::Particle; };

    /*

     //(ParticleAccessor accessor, ) {

          accessor.x(Particle&)       -> tarch::la::Vector< Dimensions, double>&;
          accessor.x(const Particle&) -> const tarch::la::Vector< Dimensions, double>&;

          accessor.v(Particle&)       -> tarch::la::Vector< Dimensions, double>&;
          accessor.v(const Particle&) -> const tarch::la::Vector< Dimensions, double>&;
    */
    //    };

    template <typename Particle>
    struct ParticleAccessorIdentity {
      static tarch::la::Vector<Dimensions, double>& x(Particle& particle) { return particle.x(); }

      static const tarch::la::Vector<Dimensions, double>& x(const Particle& particle) { return particle.x(); }

      static tarch::la::Vector<Dimensions, double>& v(Particle& particle) { return particle.v(); }

      static const tarch::la::Vector<Dimensions, double>& v(const Particle& particle) { return particle.v(); }
    };

    /**
     * Simple explicit Euler
     *
     * We realise the explicit Euler through a policy template technique: The
     * routine is generic as it accepts a template type as particle. However,
     * it does not expect the template argument to have a particular velocity
     * attribute. Instead, it takes a second template argument which takes
     * the particle and provides setters and getters.
     *
     * While the construct seems to be complicated, it allows us to write the
     * Euler once, but to use it for various particles. Some of them might
     * have a field called velocity, others might not have such a field and
     * get their data from somewhere else.
     *
     * The term policy is not uniquely determined in C++ and thus this name
     * might not be optimal.
     *
     * ## Template argument
     *
     * ParticleAccesor has to be a class (or struct) which defines a typedef
     * called Particle.
     *
     * It has to to provide the routines
     *
     * - v()
     * - v() const
     * - x()
     * - x() const
     *
     * which return a copy or provide access to the velocity or position,
     * respectively.
     */
    template <typename Particle>
    void explicitEuler(Particle& particle, double timeStepSize);
  } // namespace particles
} // namespace toolbox

// #include "Tracer.cpph"
