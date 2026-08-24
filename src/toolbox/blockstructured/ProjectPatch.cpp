// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ProjectPatch.h"

namespace {
  // Library-side instantiation catches template errors before application generation.
  [[maybe_unused]] void instantiateProjectPatch() {
    constexpr int NumberOfDofsPerAxis = 8;
    constexpr int NumberOfUnknowns    = 5;

    const float* const       inputPatchFloat              = nullptr;
    const double* const      inputPatchDouble             = nullptr;
    const long double* const inputPatchLongDouble         = nullptr;
    float* const             facesFloat[TWO_TIMES_D]      = {};
    double* const            facesDouble[TWO_TIMES_D]     = {};
    long double* const       facesLongDouble[TWO_TIMES_D] = {};

    toolbox::blockstructured::projectPatch<NumberOfDofsPerAxis, 1, NumberOfUnknowns, float>(
      inputPatchFloat,
      facesFloat
    );
    toolbox::blockstructured::projectPatch<NumberOfDofsPerAxis, 2, NumberOfUnknowns, float>(
      inputPatchFloat,
      facesFloat
    );
    toolbox::blockstructured::projectPatch<NumberOfDofsPerAxis, 1, NumberOfUnknowns, double>(
      inputPatchDouble,
      facesDouble
    );
    toolbox::blockstructured::projectPatch<NumberOfDofsPerAxis, 2, NumberOfUnknowns, double>(
      inputPatchDouble,
      facesDouble
    );
    toolbox::blockstructured::projectPatch<NumberOfDofsPerAxis, 1, NumberOfUnknowns, long double>(
      inputPatchLongDouble,
      facesLongDouble
    );
    toolbox::blockstructured::projectPatch<NumberOfDofsPerAxis, 2, NumberOfUnknowns, long double>(
      inputPatchLongDouble,
      facesLongDouble
    );
  }
} // namespace
