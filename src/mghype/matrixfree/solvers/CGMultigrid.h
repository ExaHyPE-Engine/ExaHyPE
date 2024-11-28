// This file is part of the Peano's MGHyPE extension. For conditions of
// distribution and use, please see the copyright notice at
// www.peano-framework.org
#pragma once

#include "tarch/la/Matrix.h"
#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"
#include "tarch/logging/Log.h"
#include "tarch/la/VectorSlice.h"

namespace mghype{
namespace matrixfree{
namespace solvers{
namespace cgmultigrid{
  /**
   * Restrict data from fine grid vertices onto the coarse level. We aim to take the
   * residual we calculated in the final stage of the solution on the final level
   * and write it into the right hand side of the next coarser level.
   * 
   * This method works for any number of unknowns on the vertex. We handle each one
   * after another in a for loop.
   * 
   * @param restrictionMatrix - Matrix that casts values from the fine grid vertices 
   *  onto the coarse grid vertices. Dimensions are FourPowerD * TwoPowerD since we
   *  ultimately project residual values from FourPowerD fine vertices into TwoPowerD
   *  coarse vertices. We only have one of these fine vertices in scope when we
   *  call this function during touchVertexLastTime. Hence we use the vertex marker to
   *  determine the position we occupy within the father cell.
   * 
   * @param coarseGridVertices - Vector containing TwoPowerD coarse vertices that we
   *  write into. We take the residual and put it into the rhs.
   * 
   * @param fineGridVertex - Fine vertex that we operate on. We take the residual and
   *  write its contributions into each of the TwoPowerD coarse vertices in scope
   * 
   * @param vertexMarker - vertexMarker to determine the position of this vertex within
   *  the coarse cell, which we linearise into a number between 0 and FourPowerD. Plays
   *  no other role, and could in theory be passed in as an integer instead of the
   *  whole marker.
   */
  template<class CGSolver>
  void restrictToNextLevel(
    const tarch::la::Matrix< TwoPowerD, FourPowerD, double >& restrictionMatrix,
    auto& coarseGridVertices,
    auto& fineGridVertex,
    const auto& vertexMarker
  );

  /**
   * Prolongate data from coarse vertices down onto fine vertices. Before this routine,
   * we should set "delta" on the coarse vertex, to reflect changes that have occurred
   * during post-smoothing (ie the difference that we see on the other side of the
   * v-cycle). Then, we project this onto the fine grid vertices.
   * 
   * @param prolongationMatrix - Matrix that casts values from the coarse grid vertices 
   *  onto the fine grid vertices. Dimensions are TwoPowerD * FourPowerD since we
   *  ultimately project residual values from TwoPowerD coarse vertices into FourPowerD
   *  coarse vertices. We only have one of these fine vertices in scope when we
   *  call this function during touchVertexLastTime. Hence we use the vertex marker to
   *  determine the position we occupy within the father cell.
   * 
   * @param coarseGridVertices - Vector containing TwoPowerD coarse vertices that we
   *  read "delta" from.
   * 
   * @param fineGridVertex - fine level vertex that we write into. We update the
   * solution, U.
   * 
   * @param vertexMarker - vertexMarker to determine the position of this vertex within
   *  the coarse cell, which we linearise into a number between 0 and FourPowerD. Plays
   *  no other role, and could in theory be passed in as an integer instead of the
   *  whole marker.
   * 
   */
  template<class CGSolver>
  void prolongate(
    const tarch::la::Matrix< FourPowerD, TwoPowerD, double >& prolongationMatrix,
    auto& coarseGridVertices,
    auto& fineGridVertex,
    const auto& vertexMarker
  );

  extern tarch::logging::Log  _log;

} // namespace cgmultigrid
 }// namespace solvers
} // namespace matrixfree
} // namespace mghype

#include "CGMultigrid.cpph"
