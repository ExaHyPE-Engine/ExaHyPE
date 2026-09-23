// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Constants.h"
#include "peano4/dataview/Enumerator.h"

namespace peano4::dataview {
  /**
   * Provides indexed access to the vertices adjacent to a cell.
   *
   * Vertices use Peano's lexicographic bit ordering, starting with the lower
   * vertex along every coordinate axis.
   *
   * @image html VertexEnumerator.png
   * @see peano4::dataview::FaceEnumerator
   */
  template <class Vertex>
  using VertexEnumerator = peano4::dataview::Enumerator<Vertex, peano4::VerticesPerCell>;
} // namespace peano4::dataview
