// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::plotter::tests {
  class ParticlePlotterTest;
} // namespace exahype2::plotter::tests

class exahype2::plotter::tests::ParticlePlotterTest final : public tarch::tests::TestCase {
public:
  ParticlePlotterTest();
  ParticlePlotterTest(const ParticlePlotterTest&)            = delete;
  ParticlePlotterTest(ParticlePlotterTest&&)                 = delete;
  ParticlePlotterTest& operator=(const ParticlePlotterTest&) = delete;
  ParticlePlotterTest& operator=(ParticlePlotterTest&&)      = delete;
  ~ParticlePlotterTest() override                            = default;

  void run() override;

private:
  void testPlotsEveryLocalParticleOfTheVertex();
  void testSkipsTheVertexOnAnUpperPeriodicWall();
};
