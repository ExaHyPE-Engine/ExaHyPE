// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/CompileTarget.h"

TARGET_DECLARE
namespace exahype2::dg::ader {
  /**
   * Returned by NonlinearPredictor::predict() when the Picard iteration reaches its sweep limit
   * before the tolerance.
   */
  constexpr int UnconvergedPicardIteration = -2;

  /**
   * Returned by NonlinearPredictor::predict() when an iterate is not finite or leaves the
   * divergence bound.
   */
  constexpr int DivergedPicardIteration = -1;
} // namespace exahype2::dg::ader
TARGET_DECLARE_END
