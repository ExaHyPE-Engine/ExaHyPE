// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Constants.h"
#include "peano4/dataview/VertexMarker.h"
#include "peano4/particles/MultiscaleTransitions.h"
#include "peano4/particles/assignmentchecks/TracingAPI.h"
#include "peano4/particles/particles.h"
#include "peano4/sfc/ActiveCurve.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Termination.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/la/Vector.h"

namespace exahype2::particles {
  struct LiftDropStatistics;

  template <typename ParticleSet, typename ParentVertices>
  INLINE_METHOD void dropParticles(
    const peano4::dataview::VertexMarker& marker,
    ParticleSet&                          fineGridVertexParticles,
    const ParentVertices&                 coarseGridVertexParticles,
    const std::string&                    particleName,
    int                                   treeId,
    LiftDropStatistics&                   statistics
  );

  template <typename ParticleSet>
  INLINE_METHOD void sieveParticlesIntoVertex(
    const peano4::dataview::VertexMarker& marker,
    ParticleSet&                          fineGridVertexParticles,
    int                                   treeId,
    LiftDropStatistics&                   statistics
  );

  template <typename ParticleSet, typename ParentVertices>
  INLINE_METHOD void liftOrReassignParticles(
    const peano4::dataview::VertexMarker&        marker,
    ParticleSet&                                 fineGridVertexParticles,
    const ParentVertices&                        coarseGridVertexParticles,
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainSize,
    const std::bitset<DIMENSIONS>&               periodicBoundaries,
    const std::string&                           particleName,
    int                                          treeId,
    LiftDropStatistics&                          statistics
  );

  template <typename ParticleSet, typename ParentVertices>
  INLINE_METHOD void liftAllParticles(
    const peano4::dataview::VertexMarker& marker,
    ParticleSet&                          fineGridVertexParticles,
    const ParentVertices&                 coarseGridVertexParticles,
    const std::string&                    particleName,
    int                                   treeId,
    LiftDropStatistics&                   statistics
  );
} // namespace exahype2::particles

/** Counts the particle movements between mesh levels that one tree performs in one traversal. */
struct exahype2::particles::LiftDropStatistics final {
public:
  int numberOfLifts{0};
  int numberOfDrops{0};
  int numberOfLiftsIntoSieveSet{0};
  int numberOfDropsFromSieveSet{0};
  int numberOfDropsIntoHorizontalTreeDecomposition{0};

  LiftDropStatistics()                                     = default;
  LiftDropStatistics(const LiftDropStatistics&)            = default;
  LiftDropStatistics(LiftDropStatistics&&)                 = default;
  LiftDropStatistics& operator=(const LiftDropStatistics&) = default;
  LiftDropStatistics& operator=(LiftDropStatistics&&)      = default;
  ~LiftDropStatistics()                                    = default;

  INLINE_METHOD void clear();

  template <typename ParticleSet>
  INLINE_METHOD void record() const;
};

#include "exahype2/particles/GridAssociation.cpph"
