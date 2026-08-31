// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"
#include "toolbox/particles/assignmentchecks/MeshSweepData.h"
#include "toolbox/particles/assignmentchecks/ParticleIdentifier.h"

#if not defined(ASSIGNMENT_CHECKS) and not defined(NO_ASSIGNMENT_CHECKS) and PEANO_ASSERTIONS
#define ASSIGNMENT_CHECKS
#endif

namespace toolbox::particles::assignmentchecks::internal {
  using ParticleEvents = std::vector<Event>;

  class Database;
} // namespace toolbox::particles::assignmentchecks::internal

/**
 * Stores the assignment history of every particle of one process, indexed by
 * particle and spacetree. Every method except getMeshSweepData() locks _mutex,
 * so several threads call the same object.
 */
class toolbox::particles::assignmentchecks::internal::Database final {
public:
  static constexpr int AnyTree = -1;

  static Database& getInstance();

  Database(const Database&)            = delete;
  Database(Database&&)                 = delete;
  Database& operator=(const Database&) = delete;
  Database& operator=(Database&&)      = delete;
  ~Database()                          = default;

  void           setMaxParticleSnapshotsToKeepTrackOf(size_t maximumNumberOfSnapshots);
  void           startMeshSweep(const std::string& meshSweepName);
  ParticleEvents getParticleHistory(const ParticleIdentifier& identifier) const;
  std::string    particleHistory(const ParticleIdentifier& identifier) const;
  std::string    lastMeshSweepSnapshot() const;
  int            getTotalParticleEntries(const ParticleIdentifier& identifier) const;
  int            getNumberOfSnapshots() const;
  Event          addEvent(ParticleIdentifier identifier, Event& event);
  void           replaceEvent(ParticleIdentifier identifier, Event& oldEvent, Event& newEvent);
  void           shiftIdentifierVertexInd(
              ParticleIdentifier&                       identifier,
              tarch::la::Vector<DIMENSIONS, long long>& newVertexIndex,
              const int                                 newVertexDepth
            );
  std::string                 toString() const;
  void                        reset();
  size_t                      getCurrentMeshSweepIndex() const;
  int                         getNumberOfTracedParticles() const;
  std::vector<MeshSweepData>& getMeshSweepData();
  std::string                 sweepHistory() const;
  void                        eliminateExistingParticles();
  Event getPreviousEvent(const ParticleEvents& particleHistory, int treeId, size_t numberOfFirstEventsToSkip = 0) const;

private:
  Database();

  mutable std::shared_mutex _mutex{};

  std::vector<MeshSweepData> _meshSweepData{};
  int                        _currentMeshSweepIndex{0};

  // The transparent comparison lets a lookup pass a key that is not a ParticleIdentifier.
  std::map<ParticleIdentifier, ParticleEvents, std::less<>> _data{};

  size_t _maxParticleEventsToKeepTrackOf{8};
};
