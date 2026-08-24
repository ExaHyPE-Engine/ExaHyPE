// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SubcellProjection.h"

namespace {
  // Taking these addresses makes the core library compile every projection dimension for every supported scalar type.
  template <typename Value>
  void instantiateSubcellProjectionTemplates() {
    [[maybe_unused]] auto* projectLine   = &exahype2::dg::projectTensorProduct<1, 2, 3, 2, Value>;
    [[maybe_unused]] auto* projectPlane  = &exahype2::dg::projectTensorProduct<2, 2, 3, 2, Value>;
    [[maybe_unused]] auto* projectVolume = &exahype2::dg::projectTensorProduct<3, 2, 3, 2, Value>;
  }

  [[maybe_unused]] void instantiateSubcellProjectionTemplates() {
    instantiateSubcellProjectionTemplates<float>();
    instantiateSubcellProjectionTemplates<double>();
    instantiateSubcellProjectionTemplates<long double>();
  }
} // namespace
