// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"

TARGET_DECLARE
namespace exahype2::dg::ader {
  /**
   * Returned by spaceTimePredictor() when the sweep limit comes before the tolerance.
   */
  inline constexpr int UnconvergedPicardIteration = -2;

  /**
   * Returned by spaceTimePredictor() when an iterate is not finite or leaves the divergence bound.
   */
  inline constexpr int DivergedPicardIteration = -1;
} // namespace exahype2::dg::ader
TARGET_DECLARE_END
