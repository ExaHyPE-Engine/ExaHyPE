// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/particles/assignmentchecks/Database.h"
#include "tarch/la/Vector.h"

/**
 * @namespace peano4::particles::assignmentchecks Correctness checks of
 * particle-mesh assignment
 *
 * The routines of this namespace record the assignment of a particle to a
 * vertex and compare every new assignment against the recorded history. A
 * PEANO_ASSERTIONS value above zero switches the recording on, and the compile
 * flag -DNO_ASSIGNMENT_CHECKS switches it off. A caller reports the movement of
 * a particle itself and calls startMeshSweep() before each traversal.
 */
namespace peano4::particles::assignmentchecks {
  std::string sweepHistory();

  void startMeshSweep(const std::string& meshSweepName);

  void eraseParticle(
    const std::string&                           particleName,
    const tarch::la::Vector<DIMENSIONS, double>& particleX,
    int                                          particleId,
    bool                                         isLocal,
    const tarch::la::Vector<DIMENSIONS, double>& vertexX,
    const tarch::la::Vector<DIMENSIONS, double>& vertexH,
    int                                          treeId,
    const std::string&                           trace
  );

  void moveParticle(
    const std::string&                           particleName,
    const tarch::la::Vector<DIMENSIONS, double>& oldParticleX,
    const tarch::la::Vector<DIMENSIONS, double>& newParticleX,
    int                                          particleId,
    const tarch::la::Vector<DIMENSIONS, double>& vertexX,
    const tarch::la::Vector<DIMENSIONS, double>& vertexH,
    int                                          treeId,
    const std::string&                           trace
  );

  void assignParticleToVertex(
    const std::string&                           particleName,
    const tarch::la::Vector<DIMENSIONS, double>& particleX,
    int                                          particleId,
    bool                                         isLocal,
    const tarch::la::Vector<DIMENSIONS, double>& vertexX,
    const tarch::la::Vector<DIMENSIONS, double>& vertexH,
    int                                          treeId,
    const std::string&                           trace,
    const tarch::la::Vector<DIMENSIONS, double>& prevVertexX,
    const tarch::la::Vector<DIMENSIONS, double>& prevVertexH,
    bool                                         particleIsNew                      = false,
    bool                                         reassignmentOnSameTreeDepthAllowed = false
  );

  void detachParticleFromVertex(
    const std::string&                           particleName,
    const tarch::la::Vector<DIMENSIONS, double>& particleX,
    int                                          particleId,
    bool                                         isLocal,
    const tarch::la::Vector<DIMENSIONS, double>& vertexX,
    const tarch::la::Vector<DIMENSIONS, double>& vertexH,
    int                                          treeId,
    const std::string&                           trace
  );

  void assignParticleToSieveSet(
    const std::string&                           particleName,
    const tarch::la::Vector<DIMENSIONS, double>& particleX,
    int                                          particleId,
    bool                                         isLocal,
    const tarch::la::Vector<DIMENSIONS, double>& vertexX,
    const tarch::la::Vector<DIMENSIONS, double>& vertexH,
    int                                          treeId,
    const std::string&                           trace
  );

  template <typename Particle>
  std::string pruneTypeName();

  void eliminateExistingParticles();

  void checkDatabaseIsEmpty();

  using ParticlePosition = tarch::la::Vector<DIMENSIONS, double>;

  template <typename ParticleContainer>
  std::vector<ParticlePosition> recordParticlePositions(const ParticleContainer& container);

  template <typename ParticleContainer>
  void traceParticleMovements(
    const ParticleContainer&                     container,
    const std::vector<ParticlePosition>&         recordedPositions,
    const tarch::la::Vector<DIMENSIONS, double>& vertexX,
    const tarch::la::Vector<DIMENSIONS, double>& vertexH,
    int                                          treeId
  );
} // namespace peano4::particles::assignmentchecks

#include "peano4/particles/assignmentchecks/TracingAPI.cpph"
