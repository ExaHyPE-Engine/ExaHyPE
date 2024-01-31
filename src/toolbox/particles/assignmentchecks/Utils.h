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

