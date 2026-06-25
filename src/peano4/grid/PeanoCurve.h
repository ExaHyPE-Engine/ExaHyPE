// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Loop.h"

namespace peano4::grid {
  class PeanoCurve;
  struct AutomatonState;
} // namespace peano4::grid

// TODO: Put in utils namespace
class peano4::grid::PeanoCurve {
public:
  static bool isTraversePositiveAlongAxis(const AutomatonState& state, int axis);

  /**
   * Holds a set bit for each dimension along which the traversal is
   * positive.
   */
  static peano4::LoopDirection getLoopDirection(const AutomatonState& state);

  static void setExitFace(AutomatonState& cell, int axis);
  static void setEntryFace(AutomatonState& cell, int axis);
  static void invertEvenFlag(AutomatonState& cell, int axis);
  static void removeFaceAccessNumber(AutomatonState& cell, int face);
  static void setFaceAccessNumber(AutomatonState& cell, int face, int value);

  /**
   * Looks into a cell of the spacetree and gives the index of the first
   * local vertex. The local enumeration obviously depends on the
   * orientation of the curve. If you iterate over all vertices, an xor
   * with the integer counter and this result makes the iterate a cell-local
   * traversal. So a typical loop over all vertices of a cell along the Peano
   * SFC reads as
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * const std::bitset<DIMENSIONS> coordinates = PeanoCurve::getFirstVertexIndex(state);
   * for (int i=0; i<TWO_POWER_D; i++) {
   *   std::bitset<DIMENSIONS> currentLocalVertexIndex( coordinates ^ std::bitset<DIMENSIONS>(i) );
   *   logDebug( "handle vertex " << currentLocalVertexIndex );
   * }
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * We heavily exploit bitsets here. They allow us, for example, to invert
   * the whole direction by a simple flip.
   */
  static std::bitset<DIMENSIONS> getFirstVertexIndex(const AutomatonState& cell);

  /**
   *
   * ## Implementation details
   *
   * As the access numbers already are inverted in the new code where all
   * access flags are created on the fly, we don't need the inversion flag
   * here anymore. The only situation where we need it is to toggle input
   * and output stacks.
   *
   * @return 0 or 1 if it is about in/out stacks
   */
  static int getVertexReadStackNumber(const AutomatonState& state, const std::bitset<DIMENSIONS>& vertex);

  /**
   * @return 0 or 1 if it is about in/out stacks
   */
  static int getVertexWriteStackNumber(const AutomatonState& state, const std::bitset<DIMENSIONS>& vertex);

  /**
   * Faces are enumerated following the same paradigm as the vertices. Face 0
   * is the face with a x_0 as normal which runs through the origin (left
   * bottom point), face 1 is the face with x_1 as normal, ... Face 0+d is the
   * face parallel to face 0 but on the opposite side of the cell. In the SFC
   * context, we obviously need a different enumeration scheme. With vertices,
   * deriving this scheme is simple: you get the first vertex and then you xor
   * the vertex numbers. Here, this is not possible, i.e. for faces users have
   * to go through this routine.
   *
   * Constructing a face enumeration is in principle similar to the vertex
   * numbers. We start with our normalised enumeration as sketched above. Then
   * we look at the even flags of the cell. For every even flag not set (odd
   * numbers), we invert the enumerations along all other normals. If we run
   * through the grid forward, we have, as pointed out above, first the face
   * with normal 0 running through the origin, then the face with normal 1
   * running through the origin, ... If we invert the traversal, we first have
   * to handle the face with normal d not running through the origin, then
   * the one with normal d-1, and so forth.
   */
  static int getFaceNumberAlongCurve(const AutomatonState& state, int logicalFaceNumber);

  /**
   * It is important to get the input/output stack ordering per stack type
   * consistent among all grid entities. That is, in principle it does not
   * matter whether we stream 1 to 2 and then back or 2 to 1 and then back.
   * But if vertices stream from 1 to 2 first, then faces should do so as
   * well. This allows the stack administration in the parallel case to map
   * all stacks consistently (it doesn't have to search which input/output
   * stack is full).
   *
   * @see getInputStackNumber()
   * @see getOutputStackNumber()
   */
  static int getFaceReadStackNumber(const AutomatonState& state, int face);
  static int getFaceWriteStackNumber(const AutomatonState& state, int face);

  static int getCellReadStackNumber(const AutomatonState& state);
  static int getCellWriteStackNumber(const AutomatonState& state);

  static int getInputStackNumber(const AutomatonState& state);
  static int getOutputStackNumber(const AutomatonState& state);
};
