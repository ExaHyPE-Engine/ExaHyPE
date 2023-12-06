// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "AoSLexicographicEnumerator.h"
#include "AoSoALexicographicEnumerator.h"
#include "FaceAoSLexicographicEnumerator.h"
#include "SingleDoFEnumerator.h"
#include "SoALexicographicEnumerator.h"

namespace exahype2::enumerator {
  /**
   * @page exahype2_enumerator ExaHyPE 2 enumerators
   *
   * ## Finite Volumes
   *
   * If you use an enumerator in a Finite Volume context, then a cell equals
   * a patch, as each cell hosts exactly one of them. In this case,
   * _numberOfDoFsPerAxisInCell equals the number of volumes per axis, i.e.,
   * if your patch is of size @f$ p \times p \times p @f$, then
   * _numberOfDoFsPerAxisInCell equals p.
   *
   * I assume that each patch is supplemented by a halo layer. It is embedded
   * into this halo. The index you hand into the enumerator can have negative
   * entries, and then you access elements of the halo layer. So the intention
   * is that (0,0,0) is always the left bottom ``active'' volume no matter how
   * big the halo layer is.
   *
   * In most ExaHyPE 2 codes, I have patches supplemented with halo layers and
   * (output) data without a halo. I can use different enumerator objects of
   * the same type for both of them: In one case, I set the alo layer properly,
   * and in the latter case I just set it to zero.
   *
   * ## Discontinuous Galerkin
   *
   * If you use this class in a Discontinuous Galerkin context, then
   * _numberOfDoFsPerAxisInCell equals the number of quadrature points per
   * axis. Consequently, this value is equal to the order plus one.
   *
   * Halos have no real meaning for DG, so you can safely set this value to
   * zero.
   *
   *
   * ## Multipatch enumeration
   *
   * If you work with many cells, enumerators can help you to access the individual
   * cell's data if all the data is stored within one big chunk of memory. Yet,
   * if each cell has its own memory block, while the
   * blocks are scattered over the heap, then you should pass in 1 as number
   * of cells.
   *
   */
} // namespace exahype2::enumerator
