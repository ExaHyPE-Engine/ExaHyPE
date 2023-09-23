// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace exahype2 {
  enum class PrecisionCommand {
    Long_double = sizeof(long double),
    Double      = sizeof(double),
    Single      = sizeof(float),
    Float       = Single,
    Half        = sizeof(float) / 2,
    Float16_t   = Half,
    FP16        = Half,
    Bfloat      = -1,
    Bfloat16_t  = Bfloat,
    BF16        = Bfloat,
    Custom
  };

  /**
   * The default is double as this is the highest precision available per default.
   */
  PrecisionCommand getDefaultPrecisionCommand();
} // namespace exahype2
