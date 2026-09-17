// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Loop.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/VertexEnumerator.h"
#include "peano4/dataview/VertexMarker.h"
#include "peano4/particles/assignmentchecks/TracingAPI.h"
#include "peano4/particles/internal.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/la/Vector.h"
#include "tarch/la/VectorVectorOperations.h"
#include "tarch/mpi/Rank.h"

namespace peano4::particles {
  template <typename Iterator>
  Iterator particleIsDuplicate(const typename Iterator::value_type& particle, Iterator begin, const Iterator& end);

  tarch::la::Vector<DIMENSIONS, double> periodicImageTowards(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& referencePosition,
    const tarch::la::Vector<DIMENSIONS, double>& domainWidth,
    const std::bitset<DIMENSIONS>&               periodicBC
  );

  tarch::la::Vector<DIMENSIONS, double> periodicImageTowards(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& referencePosition
  );

  tarch::la::Vector<DIMENSIONS, double> periodicImageTowardsVertex(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const peano4::dataview::VertexMarker&        marker
  );

  tarch::la::Vector<DIMENSIONS, double> periodicImageTowardsCell(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const peano4::dataview::CellMarker&          marker
  );

  bool domainHasPeriodicAxes();

  std::bitset<TWO_POWER_D> getAdjacentCellsOwningParticle(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const peano4::dataview::VertexMarker&        marker
  );

  template <typename Particle, typename ParticleSet>
  void insertParticlesIntoCell(
    const peano4::dataview::CellMarker&              marker,
    const std::vector<Particle*>&                    newParticles,
    peano4::dataview::VertexEnumerator<ParticleSet>& fineGridVertices,
    int                                              treeId
  );

  template <typename Particle, typename ParticleSet>
  void insertParticleIntoCell(
    const peano4::dataview::CellMarker&              marker,
    Particle*                                        newParticle,
    peano4::dataview::VertexEnumerator<ParticleSet>& fineGridVertices,
    int                                              treeId
  );

  template <typename Particle>
  void updateContainedInAdjacentCellProperty(
    const peano4::dataview::CellMarker& marker,
    const std::bitset<DIMENSIONS>&      assignedVertex,
    Particle&                           particle
  );

  template <typename Particle>
  void updateContainedInAdjacentCellProperty(const peano4::dataview::VertexMarker& marker, Particle& particle);

  template <typename Particle>
  void updateContainedInAdjacentCellProperty(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    Particle&                                    particle
  );

  bool particleAssignedToVertexWillBeLocal(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const peano4::dataview::VertexMarker&        marker
  );

  tarch::la::Vector<DIMENSIONS, double> applyPeriodicBoundaryConditions(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    tarch::la::Vector<DIMENSIONS, double>        domainOffset,
    tarch::la::Vector<DIMENSIONS, double>        domainSize,
    std::bitset<DIMENSIONS>                      periodicBC
  );
} // namespace peano4::particles

#include "peano4/particles/particles.cpph"
