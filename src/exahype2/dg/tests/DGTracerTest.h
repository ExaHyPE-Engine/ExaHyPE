// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::tests {
  class DGTracerTest;
} // namespace exahype2::dg::tests

class exahype2::dg::tests::DGTracerTest final : public tarch::tests::TestCase {
public:
  DGTracerTest();
  DGTracerTest(const DGTracerTest&)            = delete;
  DGTracerTest(DGTracerTest&&)                 = delete;
  DGTracerTest& operator=(const DGTracerTest&) = delete;
  DGTracerTest& operator=(DGTracerTest&&)      = delete;
  ~DGTracerTest() override                     = default;

  void run() override;

private:
  void testProjectValueOntoParticle();
  void testProjectAllValuesOntoParticle();
};
