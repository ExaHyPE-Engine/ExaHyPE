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
         * Using particle positions as floats, however, turned out to be a bad
         * idea, even when allowing for fuzzy searches within a reasonable
         * interval.
         *
         * Instead: TODO MLADEN: WRITE DOCU
         */
        struct ParticleIdentifier {

          std::string                              particleName;
          tarch::la::Vector<Dimensions, double>    particleX;
          tarch::la::Vector<Dimensions, long long> vertexInd;
          int                                      vertexDepth;
          int                                      particleID;
          int                                      treeID;

          std::string toString() const;

          bool numericalEquals(const ParticleIdentifier& rhs) const;
          bool numericalSmaller(const ParticleIdentifier& rhs) const;

          bool operator<(const ParticleIdentifier& rhs) const;

          ParticleIdentifier(
            const std::string&                              particleName__,
            const tarch::la::Vector<Dimensions, double>&    particleX__,
            const tarch::la::Vector<Dimensions, double>&    vertexX,
            const tarch::la::Vector<Dimensions, double>&    vertexH,
            const int                                       particleID__,
            const int                                       treeId__
          );

          ParticleIdentifier(
            const std::string&                              particleName__,
            const tarch::la::Vector<Dimensions, double>&    particleX__,
            const tarch::la::Vector<Dimensions, long long>& vertexInd__,
            const int                                       vertexDepth__,
            const int                                       particleID__,
            const int                                       treeID__
          );

          /**
           * Compute an integer vertex "index" given its position and size
           */
          static tarch::la::Vector<Dimensions, long long> getVertexInd(
            const tarch::la::Vector<Dimensions, double>&    vertexX,
            const tarch::la::Vector<Dimensions, double>&    vertexH
            );

          /**
           * Compute an integer vertex "depth" given its size
           * We currently guess a domain size for a simulation and just
           * estimate the depth given the vertex size. The vertexX is
           * used to raise an error if we're out of the assumed box size.
           */
          static int getVertexDepth(
            const tarch::la::Vector<Dimensions, double>&    vertexX,
            const tarch::la::Vector<Dimensions, double>&    vertexH
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
