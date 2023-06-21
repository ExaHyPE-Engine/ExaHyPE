// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


namespace exahype2 {
  enum class PrecisionCommand {
    Long_double=sizeof(long double),
    Double=sizeof(double),
    Single=sizeof(float),
    Float=Single,
    Half=sizeof(double)/2, 
    Custom
  };

  /**
   * The default is double as this is the highest precision available per default.
   */
  PrecisionCommand getDefaultPrecisionCommand();
}


