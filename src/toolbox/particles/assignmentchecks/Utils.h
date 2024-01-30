// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <map>
#include <vector>

#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/VertexEnumerator.h"
#include "tarch/la/Vector.h"


namespace toolbox {
  namespace particles {
    namespace assignmentchecks {
      using ParticlePosition = tarch::la::Vector<Dimensions, double>;

      template <typename ParticleContainer>
      std::vector< ParticlePosition > recordParticlePositions(const ParticleContainer& container);

      /**
       * Record the particle positions
       *
       * This routine runs through the particles in container and invokes
       * moveParticle() for each and every particle which has moved. That is,
       * we assume that recordedPositions has been obtained through
       * recordParticlePositions() before, and both recordedPositions and
       * container hold the same particles in the same order (even though they
       * might have moved in-between).
       *
       * We only record particles that have moved, but actually the comparison
       * here is only an optimisation, as the moveParticle itself will once
       * again check that the particle has actually changed its position.
       */
      template <typename ParticleContainer>
      void traceParticleMovements(
        const ParticleContainer&                container,
        const std::vector< ParticlePosition >&  recordedPositions,
        int                                     spacetreeId
      );
    } // namespace assignmentchecks
  }   // namespace particles
} // namespace toolbox


#include "Utils.cpph"

