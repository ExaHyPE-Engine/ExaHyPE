// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/ErrorMeasurement.h"
#include "exahype2/particles/ParticleSet.h"
#include "tarch/CompilerSpecificSettings.h"

namespace exahype2::particles {
  template <typename SolverType, int NumberOfData>
  INLINE_METHOD std::array<double, ErrorMeasurement::NumberOfNorms> measurePositionErrors(
    ParticleSet<NumberOfData>& particles,
    double                     timeStamp,
    double                     timeStepSize,
    ErrorMeasurement&          measurement
  );
} // namespace exahype2::particles

#include "exahype2/particles/ParticleErrorMeasurement.cpph"
