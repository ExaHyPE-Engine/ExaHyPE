// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/dataview/VertexMarker.h"
#include "peano4/particles/assignmentchecks/TracingAPI.h"
#include "peano4/particles/particles.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/instrumentation/Debug.h"

namespace exahype2::particles {
  struct ParallelStateStatistics;

  template <typename ParticleSet>
  INLINE_METHOD void updateParallelStateOfParticles(
    const peano4::dataview::VertexMarker& marker,
    ParticleSet&                          fineGridVertexParticles,
    const std::string&                    particleName,
    int                                   treeId,
    ParallelStateStatistics&              statistics
  );

  template <typename ParticleSet>
  INLINE_METHOD void deleteVirtualParticles(
    const peano4::dataview::VertexMarker& marker,
    ParticleSet&                          fineGridVertexParticles,
    const std::string&                    particleName,
    int                                   treeId,
    ParallelStateStatistics&              statistics
  );
} // namespace exahype2::particles

/**
 * Counts the local and the expired particles that one tree sees in one traversal.
 *
 * Every vertex copy of a local particle adds to the counter, while the set of particle numbers
 * holds one entry per particle.
 */
struct exahype2::particles::ParallelStateStatistics final {
public:
  int                          numberOfRemainingLocalParticles{0};
  std::set<std::array<int, 2>> remainingLocalParticleNumbers{};
  int                          numberOfExpiredHaloParticles{0};
  int                          numberOfParticlesThatHaveLeftTheirDomain{0};

  ParallelStateStatistics()                                          = default;
  ParallelStateStatistics(const ParallelStateStatistics&)            = default;
  ParallelStateStatistics(ParallelStateStatistics&&)                 = default;
  ParallelStateStatistics& operator=(const ParallelStateStatistics&) = default;
  ParallelStateStatistics& operator=(ParallelStateStatistics&&)      = default;
  ~ParallelStateStatistics()                                         = default;

  INLINE_METHOD void clear();

  template <typename ParticleSet>
  INLINE_METHOD void record() const;
};

#include "exahype2/particles/ParallelState.cpph"
