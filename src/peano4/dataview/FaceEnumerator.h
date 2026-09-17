// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/dataview/Enumerator.h"

namespace peano4::dataview {
  /**
   * Provides indexed access to the faces adjacent to a cell.
   *
   * The lower face along each coordinate axis precedes the upper face along
   * each coordinate axis.
   *
   * @image html FaceEnumerator.png
   * @see peano4::dataview::VertexEnumerator
   */
  template <class Face>
  using FaceEnumerator = peano4::dataview::Enumerator<Face, TWO_TIMES_D>;
} // namespace peano4::dataview
