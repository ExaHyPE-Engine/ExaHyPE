// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ReconstructPatch.h"

namespace {
  // Library-side instantiation catches template errors before application generation.
  [[maybe_unused]] void instantiateReconstructPatch() {
    constexpr int NumberOfDofsPerAxis = 8;
    constexpr int NumberOfUnknowns    = 5;

    const float* const       inputPatchFloat              = nullptr;
    const double* const      inputPatchDouble             = nullptr;
    const long double* const inputPatchLongDouble         = nullptr;
    float* const             facesFloat[TWO_TIMES_D]      = {};
    double* const            facesDouble[TWO_TIMES_D]     = {};
    long double* const       facesLongDouble[TWO_TIMES_D] = {};
    float* const             reconstructedPatchFloat      = nullptr;
    double* const            reconstructedPatchDouble     = nullptr;
    long double* const       reconstructedPatchLongDouble = nullptr;

    exahype2::reconstructPatch<NumberOfDofsPerAxis, 1, NumberOfUnknowns, float>(
      inputPatchFloat,
      facesFloat,
      reconstructedPatchFloat
    );
    exahype2::reconstructPatch<NumberOfDofsPerAxis, 2, NumberOfUnknowns, float>(
      inputPatchFloat,
      facesFloat,
      reconstructedPatchFloat
    );
    exahype2::reconstructPatch<NumberOfDofsPerAxis, 1, NumberOfUnknowns, double>(
      inputPatchDouble,
      facesDouble,
      reconstructedPatchDouble
    );
    exahype2::reconstructPatch<NumberOfDofsPerAxis, 2, NumberOfUnknowns, double>(
      inputPatchDouble,
      facesDouble,
      reconstructedPatchDouble
    );
    exahype2::reconstructPatch<NumberOfDofsPerAxis, 1, NumberOfUnknowns, long double>(
      inputPatchLongDouble,
      facesLongDouble,
      reconstructedPatchLongDouble
    );
    exahype2::reconstructPatch<NumberOfDofsPerAxis, 2, NumberOfUnknowns, long double>(
      inputPatchLongDouble,
      facesLongDouble,
      reconstructedPatchLongDouble
    );
  }
} // namespace
