// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include "Event.h"

toolbox::particles::assignmentchecks::internal::Event::Event(
  Type                                         type_,
  bool                                         isLocal_,
  const tarch::la::Vector<Dimensions, double>& vertexX_,
  const tarch::la::Vector<Dimensions, double>& particleX_,
  const tarch::la::Vector<Dimensions, double>& vertexH_,
  int                                          treeId_,
  const std::string&                           trace_,
  const int                                    meshSweepIndex_
):
  type(type_),
  isLocal(isLocal_),
  vertexX(vertexX_),
  previousParticleX(particleX_),
  vertexH(vertexH_),
  treeId(treeId_),
  trace(trace_),
  meshSweepIndex(meshSweepIndex_) {}


toolbox::particles::assignmentchecks::internal::Event::Event(
  Type                                         type_,
  bool                                         isLocal_,
  const tarch::la::Vector<Dimensions, double>& vertexX_,
  const tarch::la::Vector<Dimensions, double>& particleX_,
  const tarch::la::Vector<Dimensions, double>& vertexH_,
  int                                          treeId_,
  const std::string&                           trace_
):
  type(type_),
  isLocal(isLocal_),
  vertexX(vertexX_),
  previousParticleX(particleX_),
  vertexH(vertexH_),
  treeId(treeId_),
  trace(trace_) {
  assertion(type == Type::AssignToVertex or type == Type::DetachFromVertex);
}


toolbox::particles::assignmentchecks::internal::Event::
  Event(Type type_, bool isLocal_, int treeId_, const std::string& trace_):
  type(type_),
  isLocal(isLocal_),
  vertexX(tarch::la::Vector<Dimensions, double>(0.0)),
  previousParticleX(0.0),
  vertexH(tarch::la::Vector<Dimensions, double>(0.0)),
  treeId(treeId_),
  trace(trace_) {
  assertion(type == Type::AssignToSieveSet or type == Type::Erase);
}


toolbox::particles::assignmentchecks::internal::Event::Event(Type type_):
  type(type_),
  isLocal(false),
  vertexX(tarch::la::Vector<Dimensions, double>(0.0)),
  previousParticleX(0.0),
  vertexH(tarch::la::Vector<Dimensions, double>(0.0)),
  treeId(-1),
  trace("no-trace") {
  assertion(type == Type::NotFound);
}


toolbox::particles::assignmentchecks::internal::Event::Event(
  Type                                         type_,
  const tarch::la::Vector<Dimensions, double>& vertexX_,
  const tarch::la::Vector<Dimensions, double>& previousParticleX_,
  const tarch::la::Vector<Dimensions, double>& vertexH_,
  int                                          treeId_,
  const std::string&                           trace_
):
  type(type_),
  isLocal(true),
  vertexX(vertexX_),
  previousParticleX(previousParticleX_),
  vertexH(vertexH_),
  treeId(treeId_),
  trace(trace_) {
  assertion(type == Type::MoveWhileAssociatedToVertex);
}


std::string toolbox::particles::assignmentchecks::internal::Event::toString(
) const {
  std::ostringstream msg;

  msg << "(";
  switch (type) {
  case Type::AssignToSieveSet:
    msg << "assign-to-sieve-set"
        << ",local=" << isLocal << ",tree=" << treeId << ",trace=" << trace;
    break;
  case Type::AssignToVertex:
    msg
      << "assign-to-vertex"
      << ",local=" << isLocal << ",x=" << vertexX << ",h=" << vertexH
      << ",tree=" << treeId << ",trace=" << trace;
    break;
  case Type::Erase:
    msg << "erase"
        << ",local=" << isLocal << ",tree=" << treeId << ",trace=" << trace;
    break;
  case Type::DetachFromVertex:
    msg
      << "detach-from-vertex"
      << ",local=" << isLocal << ",x=" << vertexX << ",h=" << vertexH
      << ",tree=" << treeId << ",trace=" << trace;
    break;
  case Type::NotFound:
    msg << "not-found";
    break;
  case Type::MoveWhileAssociatedToVertex:
    msg
      << "moved-while-associated-to-vertex"
      << "," << previousParticleX << "->x_new,tree=" << treeId
      << ",trace=" << trace;
    break;
  case Type::ConsecutiveMoveWhileAssociatedToVertex:
    msg
      << "consecutive-moved-while-associated-to-vertex"
      << "," << previousParticleX << "->x_new,tree=" << treeId
      << ",trace=" << trace;
    break;
  };
  msg << ")";

  return msg.str();
}
