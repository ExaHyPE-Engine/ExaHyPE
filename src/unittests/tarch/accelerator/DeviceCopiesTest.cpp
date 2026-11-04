// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/DeviceCopies.h"

#include "tarch/Assertions.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"
#include "tarch/la/Vector.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/tarch/accelerator/DeviceCopiesTestData.h"

namespace {
  TARGET_DECLARE
  DEVICE_VARIABLE constexpr tarch::la::Vector<2, double> ConstantVector{1.5, -2.5};
  DEVICE_VARIABLE constexpr double                       ConstantScalar = 4.0;

  struct ReadValue final {
    double* target{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()([[maybe_unused]] const int workItem) const {
      *target = tarch::accelerator::getDeviceCopy<double>();
    }
  };

  INLINE_METHOD GPU_CALLABLE_METHOD double getConstantValue(const int index) {
    ASSERTION(index >= 0 and index < 2, index);
    return ConstantVector[index] + std::max(static_cast<double>(index), ConstantScalar);
  }

  struct ReadConstants final {
    double* target{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int workItem) const {
      target[workItem] = getConstantValue(workItem);
    }
  };
  TARGET_DECLARE_END

  double getValueOfAKernel() {
    tarch::accelerator::Buffer<double> target(1, tarch::MemoryLocation::Device);
    tarch::accelerator::parallelFor(tarch::accelerator::Range(1), ReadValue{target.getData()});
    double result{0.0};
    target.copyToHost(&result, 1);
    return result;
  }
} // namespace

UNIT_TEST("The kernels of two translation units read the host value of the last upload from the device copy") {
  GIVEN("The values 42.5 and -3") {
    constexpr double FirstValue  = 42.5;
    constexpr double SecondValue = -3.0;

    WHEN("The host value takes each value in turn and a kernel of each translation unit reads it after each upload") {
      tarch::accelerator::unittests::hostValue = FirstValue;
      tarch::accelerator::uploadDeviceCopies();
      const double valuesAfterTheFirstUpload[2]{
        getValueOfAKernel(),
        tarch::accelerator::unittests::getValueOfAKernelOfTheTestData()
      };

      tarch::accelerator::unittests::hostValue = SecondValue;
      tarch::accelerator::uploadDeviceCopies();
      const double valuesAfterTheSecondUpload[2]{
        getValueOfAKernel(),
        tarch::accelerator::unittests::getValueOfAKernelOfTheTestData()
      };

      THEN("The device copy of the host value exists and both kernels read 42.5 first and -3 afterwards") {
        VALIDATE(deviceValueIsAdded);
        for (int translationUnit = 0; translationUnit < 2; translationUnit++) {
          VALIDATE_NUMERICAL_EQUALS(valuesAfterTheFirstUpload[translationUnit], 42.5, translationUnit);
          VALIDATE_NUMERICAL_EQUALS(valuesAfterTheSecondUpload[translationUnit], -3.0, translationUnit);
        }
      }
    }
  }
}

UNIT_TEST("Host code and a kernel read the same values from constexpr device variables") {
  GIVEN("A device buffer of two entries") {
    tarch::accelerator::Buffer<double> target(2, tarch::MemoryLocation::Device);

    WHEN("A kernel writes the constant values of the indices zero and one into the buffer and host code reads them") {
      tarch::accelerator::parallelFor(tarch::accelerator::Range(2), ReadConstants{target.getData()});
      double valuesOfAKernel[2]{0.0, 0.0};
      target.copyToHost(valuesOfAKernel, 2);
      const double valuesOfHostCode[2]{getConstantValue(0), getConstantValue(1)};

      THEN("Host code reads 5.5 and 1.5 and the kernel reads the same values") {
        VALIDATE_NUMERICAL_EQUALS(valuesOfHostCode[0], 5.5);
        VALIDATE_NUMERICAL_EQUALS(valuesOfHostCode[1], 1.5);
        for (int index = 0; index < 2; index++) {
          VALIDATE_NUMERICAL_EQUALS(valuesOfAKernel[index], valuesOfHostCode[index], index);
        }
      }
    }
  }
}
