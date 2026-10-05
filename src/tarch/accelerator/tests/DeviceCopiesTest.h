// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::accelerator::tests {
  class DeviceCopiesTest;
} // namespace tarch::accelerator::tests

class tarch::accelerator::tests::DeviceCopiesTest final : public tarch::tests::TestCase {
public:
  DeviceCopiesTest();
  DeviceCopiesTest(const DeviceCopiesTest& other)            = delete;
  DeviceCopiesTest(DeviceCopiesTest&& other)                 = delete;
  DeviceCopiesTest& operator=(const DeviceCopiesTest& other) = delete;
  DeviceCopiesTest& operator=(DeviceCopiesTest&& other)      = delete;
  ~DeviceCopiesTest() override                               = default;

  void run() override;

private:
  void testTheKernelsOfTwoTranslationUnitsReadTheLastUpload();
  void testHostCodeAndAKernelReadTheSameConstants();
};
