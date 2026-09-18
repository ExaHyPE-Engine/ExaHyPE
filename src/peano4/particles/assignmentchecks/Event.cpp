// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Event.h"

#include "tarch/Assertions.h"

/**
 * Constructs an event from every field, the mesh sweep index included.
 */
peano4::particles::assignmentchecks::Event::Event(
  Type                                         type_,
  bool                                         isLocal_,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX_,
  const tarch::la::Vector<DIMENSIONS, double>& particleX_,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH_,
  int                                          treeId_,
  const std::string&                           trace_,
  const int                                    meshSweepIndex_
) :
  type(type_),
  isLocal(isLocal_),
  vertexX(vertexX_),
  previousParticleX(particleX_),
  vertexH(vertexH_),
  treeId(treeId_),
  trace(trace_),
  meshSweepIndex(meshSweepIndex_) {}

/**
 * Constructs an event that assigns a particle to a vertex or removes it from a
 * vertex.
 */
peano4::particles::assignmentchecks::Event::Event(
  Type                                         type_,
  bool                                         isLocal_,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX_,
  const tarch::la::Vector<DIMENSIONS, double>& particleX_,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH_,
  int                                          treeId_,
  const std::string&                           trace_
) :
  type(type_),
  isLocal(isLocal_),
  vertexX(vertexX_),
  previousParticleX(particleX_),
  vertexH(vertexH_),
  treeId(treeId_),
  trace(trace_) {
  assertion(type == Type::AssignToVertex or type == Type::DetachFromVertex);
}

/**
 * Constructs an event that assigns a particle to the sieve set or erases it.
 */
peano4::particles::assignmentchecks::Event::Event(Type type_, bool isLocal_, int treeId_, const std::string& trace_) :
  type(type_),
  isLocal(isLocal_),
  vertexX(tarch::la::Vector<DIMENSIONS, double>(0.0)),
  previousParticleX(0.0),
  vertexH(tarch::la::Vector<DIMENSIONS, double>(0.0)),
  treeId(treeId_),
  trace(trace_) {
  assertion(type == Type::AssignToSieveSet or type == Type::Erase);
}

/**
 * Constructs an event of the type NotFound, which the database returns when it
 * holds no history for a particle.
 */
peano4::particles::assignmentchecks::Event::Event(Type type_) :
  type(type_),
  vertexX(tarch::la::Vector<DIMENSIONS, double>(0.0)),
  previousParticleX(0.0),
  vertexH(tarch::la::Vector<DIMENSIONS, double>(0.0)),
  trace("no-trace") {
  assertion(type == Type::NotFound);
}

/**
 * Constructs an event that moves a particle which stays at its vertex.
 * previousParticleX_ holds the position from before the movement.
 */
peano4::particles::assignmentchecks::Event::Event(
  Type                                         type_,
  const tarch::la::Vector<DIMENSIONS, double>& vertexX_,
  const tarch::la::Vector<DIMENSIONS, double>& previousParticleX_,
  const tarch::la::Vector<DIMENSIONS, double>& vertexH_,
  int                                          treeId_,
  const std::string&                           trace_
) :
  type(type_),
  isLocal(true),
  vertexX(vertexX_),
  previousParticleX(previousParticleX_),
  vertexH(vertexH_),
  treeId(treeId_),
  trace(trace_) {
  assertion(type == Type::MoveWhileAssociatedToVertex);
}

std::string peano4::particles::assignmentchecks::Event::toString() const {
  std::ostringstream msg;

  msg << "(";
  switch (type) {
  case Type::AssignToSieveSet:
    msg << "assign-to-sieve-set"
        << ",local=" << isLocal << ",tree=" << treeId << ",trace=" << trace;
    break;
  case Type::AssignToVertex:
    msg << "assign-to-vertex"
        << ",local=" << isLocal << ",x=" << vertexX << ",h=" << vertexH << ",tree=" << treeId << ",trace=" << trace;
    break;
  case Type::Erase:
    msg << "erase"
        << ",local=" << isLocal << ",tree=" << treeId << ",trace=" << trace;
    break;
  case Type::DetachFromVertex:
    msg << "detach-from-vertex"
        << ",local=" << isLocal << ",x=" << vertexX << ",h=" << vertexH << ",tree=" << treeId << ",trace=" << trace;
    break;
  case Type::NotFound:
    msg << "not-found";
    break;
  case Type::MoveWhileAssociatedToVertex:
    msg << "moved-while-associated-to-vertex"
        << "," << previousParticleX << "->x_new,tree=" << treeId << ",trace=" << trace;
    break;
  case Type::ConsecutiveMoveWhileAssociatedToVertex:
    msg << "consecutive-moved-while-associated-to-vertex"
        << "," << previousParticleX << "->x_new,tree=" << treeId << ",trace=" << trace;
    break;
  }
  msg << ")";

  return msg.str();
}
