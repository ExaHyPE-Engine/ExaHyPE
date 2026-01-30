// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "VertexType.h"
#include "peano4/Globals.h"
#include "tarch/la/Vector.h"

namespace peano4::grid {
  enum class FaceType : std::uint8_t { New, Hanging, Persistent, Delete };

  std::string toString(FaceType type);

  /**
   * Identify type of face.
   *
   * Find out what type a face has, i.e. is it a new one, one that is to
   * be deleted, or is it a persistent one. The outcome will decide where
   * to take a face from and where to store it. It also will determine
   * which cascade of events we will invoke.
   *
   * <h2> Implementation </h2>
   *
   * Different to getVertexType(), we can work with the the fine grid's
   * vertices here already. The implementation might not be the most
   * elegant one: We loop over all four or eight vertices, respectively,
   * but we use the normal of the faceNumber to alter the vertex index
   * such that we eventually study only half of the vertices; those adjacent
   * to the face.
   *
   * There are four potential outcomes: new, hanging, persistent, delete.
   * They derive as follows:
   *
   * - Hanging: A face is hanging if all adjacent vertices on the same
   *   level are hanging (red in sketch above).
   * - Delete: A face is delete if all adjacent vertices are either
   *   hanging or delete and the face is not hanging.
   * - New: A face is new if all adjacent vertices are either
   *   hanging or new and the face is not hanging.
   * - Persistent: If nothing else applies.
   */
  FaceType getFaceType(const VertexType vertexTypes[TWO_POWER_D], int faceNumber);
} // namespace peano4::grid

std::ostream& operator<<(std::ostream& out, const peano4::grid::FaceType& type);
