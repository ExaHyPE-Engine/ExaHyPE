// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace peano4::particles::assignmentchecks {
  /**
   * Stores one event of the particle-grid association.
   *
   * An event assigns a particle to a vertex or to the sieve set, detaches it
   * from a vertex, erases it, or moves it in space.
   */
  struct Event {
    enum class Type {
      NotFound,
      AssignToSieveSet,
      AssignToVertex,
      Erase,
      DetachFromVertex,
      MoveWhileAssociatedToVertex,
      ConsecutiveMoveWhileAssociatedToVertex
    };

    Type                                  type{Type::NotFound};
    bool                                  isLocal{false};
    tarch::la::Vector<DIMENSIONS, double> vertexX{};
    tarch::la::Vector<DIMENSIONS, double> previousParticleX{};
    tarch::la::Vector<DIMENSIONS, double> vertexH{};
    int                                   treeId{-1};
    std::string                           trace{};
    int                                   meshSweepIndex{-1};

    Event(
      Type                                         type_,
      bool                                         isLocal_,
      const tarch::la::Vector<DIMENSIONS, double>& vertexX_,
      const tarch::la::Vector<DIMENSIONS, double>& particleX,
      const tarch::la::Vector<DIMENSIONS, double>& vertexH_,
      int                                          treeId_,
      const std::string&                           trace_,
      int                                          meshSweepIndex_
    );

    Event(
      Type                                         type_,
      bool                                         isLocal_,
      const tarch::la::Vector<DIMENSIONS, double>& vertexX_,
      const tarch::la::Vector<DIMENSIONS, double>& particleX,
      const tarch::la::Vector<DIMENSIONS, double>& vertexH_,
      int                                          treeId_,
      const std::string&                           trace_
    );

    Event(
      Type                                         type_,
      const tarch::la::Vector<DIMENSIONS, double>& vertexX_,
      const tarch::la::Vector<DIMENSIONS, double>& previousParticleX_,
      const tarch::la::Vector<DIMENSIONS, double>& vertexH_,
      int                                          treeId_,
      const std::string&                           trace_
    );

    Event(Type type_, bool isLocal_, int treeId_, const std::string& trace_);

    Event(Type type_);

    [[nodiscard]] std::string toString() const;
  };
} // namespace peano4::particles::assignmentchecks
