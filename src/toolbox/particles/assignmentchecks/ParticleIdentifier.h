// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#pragma once

#include "tarch/la/Vector.h"

namespace toolbox {
  namespace particles {
    namespace assignmentchecks {
      namespace internal {

        /**
         * @struct ParticleIdentifier is used as a key to the 'database' map
         * to keep the particles' history as the map's values.
         *
         * By default, particles' are ID'd by their unique integer ID and
         * type name. However, in certain situations, that may not be enough.
         * For example, if a particle is close to a periodic boundary, it will
         * be replicated and its coordinates shifted appropriately on the other
         * boundary. There will however exist 2 particles with the same unique
         * integer ID and type name, which will cause issues with the assignment
         * checks, as it will have an inappropriate history. Therefore, we also
         * need to take into account the particles' positions to ID them.
         *
         * Luckily, we don't need an exact coordinate match, but we can allow
         * for big tolerances (order of the simulation box size) and thusly
         * sidestep many floating point inaccuracies. This tolerance needs to be
         * specified when constructing a ParticleIdentifier. The recommendation
         * is to use the 'size' of the vertex (`vertexH`) the particle is
         * currently assigned to.
         *
         * A minor complication lies in the fact that a std::map (which is used
         * as the "database") needs strict comparisons. So we define two very
         * similar structs: a ParticleIdentifier, which is used as the key in
         * the map, and a ParticleSearchIdentifier, which is used to search for
         * a particle in the map and permits for a fuzzy search using the
         * tolerance mentioned above.
         */
        struct ParticleIdentifier;

        /**
         * @struct ParticleSearchIdentifier defines a struct to be used when
         * querying the database map for a particle history. See the
         * documentation of ParticleIdentifier for more details on the
         * differences between them.
         */
        struct ParticleSearchIdentifier;

        struct ParticleIdentifier {

          // don't make these functions below virtual: I need to be able to pass
          // &ParticleSearchIdentifier and have it be treated like
          // &ParticleIdentifier in function arguments.

          std::string                           particleName;
          tarch::la::Vector<Dimensions, double> particleX;
          int                                   particleID;

          std::string toString() const;

          bool numericalEquals(const ParticleIdentifier& rhs) const;
          bool numericalSmaller(const ParticleIdentifier& rhs) const;
          bool numericalSmallerWithTolerance(
            const ParticleIdentifier& rhs,
            const double              tolerance
          ) const;

          // bool operator==(const ParticleIdentifier& rhs) const;
          // bool operator!=(const ParticleIdentifier& rhs) const;
          bool operator<(const ParticleIdentifier& rhs) const;
          bool operator<(const ParticleSearchIdentifier& rhs) const;


          ParticleIdentifier(
            const std::string&                           particleName__,
            const tarch::la::Vector<Dimensions, double>& particleX__,
            const int                                    particleID__
          );
        };


        struct ParticleSearchIdentifier: ParticleIdentifier {

          static constexpr double shiftTolerance = 0.3;

          double positionTolerance;

          bool operator<(const ParticleIdentifier& rhs) const;

          ParticleSearchIdentifier(
            const std::string&                           particleName__,
            const tarch::la::Vector<Dimensions, double>& particleX__,
            const int                                    particleID__,
            const double                                 positionTolerance__
          );
        };

      } // namespace internal
    }   // namespace assignmentchecks
  }     // namespace particles
} // namespace toolbox


/**
 * Comparison operator
 *
 * Some older variants of C++ require an external operator (not embedded
 * into class or namespace) to compare two objects. We have seen issues
 * with Intel 2023, e.g., while 2024 was absolutely fine. Delegates to
 * member function.
 */
bool operator==(
  const toolbox::particles::assignmentchecks::internal::ParticleIdentifier& lhs,
  const toolbox::particles::assignmentchecks::internal::ParticleIdentifier& rhs
);

bool operator!=(
  const toolbox::particles::assignmentchecks::internal::ParticleIdentifier& lhs,
  const toolbox::particles::assignmentchecks::internal::ParticleIdentifier& rhs
);
