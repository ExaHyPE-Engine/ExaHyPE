// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/VertexMarker.h"
#include "peano4/particles/internal.h"
#include "peano4/particles/particles.h"
#include "tarch/la/VectorOperations.h"

namespace peano4::particles {
  using ParticleReassociationInstruction = int;

  constexpr int ParticleReassociationInstructionKeep          = -1;
  constexpr int ParticleReassociationInstructionSieveGlobally = -2;

  template <typename Particle>
  ParticleReassociationInstruction liftParticleAssociatedWithVertex(
    const Particle&                particle,
    peano4::dataview::VertexMarker marker
  );

  ParticleReassociationInstruction liftParticleAssociatedWithVertex(
    bool                                         isLocal,
    double                                       searchRadius,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    peano4::dataview::VertexMarker               marker
  );

  template <typename Particle>
  bool sieveParticle(const Particle& particle, const peano4::dataview::VertexMarker& marker);

  bool sieveParticle(
    double                                       searchRadius,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const peano4::dataview::VertexMarker&        marker
  );

  template <typename Particle>
  bool dropParticle(const Particle& particle, const peano4::dataview::VertexMarker& marker);

  bool dropParticle(
    double                                       searchRadius,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const peano4::dataview::VertexMarker&        marker
  );

  template <typename Particle>
  bool particleWillBeDroppedFurther(const Particle& particle, const peano4::dataview::CellMarker& marker);

  bool particleWillBeDroppedFurther(double searchRadius, const peano4::dataview::CellMarker& marker);

  template <typename Particle>
  bool particleWillBeDroppedFurther(const Particle& particle, const peano4::dataview::VertexMarker& marker);

  bool particleWillBeDroppedFurther(double searchRadius, const peano4::dataview::VertexMarker& marker);

  double relativeSpatialOwnershipTolerance(const peano4::dataview::CellMarker& marker);

  template <typename Particle>
  ParticleReassociationInstruction getParticleReassociationInstructionWithinCellWithIntraCellReassignment(
    const Particle&                     particle,
    const peano4::dataview::CellMarker& marker,
    int                                 numberOfVertexWithinCell
  );

  ParticleReassociationInstruction getParticleReassociationInstructionWithinCellWithIntraCellReassignment(
    bool                                         isLocal,
    double                                       searchRadius,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const peano4::dataview::CellMarker&          marker,
    int                                          numberOfVertexWithinCell
  );
} // namespace peano4::particles

#include "peano4/particles/MultiscaleTransitions.cpph"
