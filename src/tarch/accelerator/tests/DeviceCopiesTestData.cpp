// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DeviceCopiesTestData.h"

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/DeviceCopies.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"

namespace {
  TARGET_DECLARE
  struct ReadValueOfTheTestData final {
    double* target{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()([[maybe_unused]] const int workItem) const {
      *target = tarch::accelerator::getDeviceCopy<double>();
    }
  };
  TARGET_DECLARE_END
} // namespace

double
tarch::accelerator::tests::getValueOfAKernelOfTheTestData() {
  tarch::accelerator::Buffer<double> target(1, tarch::MemoryLocation::Device);
  tarch::accelerator::parallelFor(tarch::accelerator::Range(1), ReadValueOfTheTestData{target.getData()});
  double result{0.0};
  target.copyToHost(&result, 1);
  return result;
}
