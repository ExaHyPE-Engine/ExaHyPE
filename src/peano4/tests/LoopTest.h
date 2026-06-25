// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::tests {
  class LoopTest;
} // namespace peano4::tests

class peano4::tests::LoopTest final: public tarch::tests::TestCase {
private:
  /**
   * dfor(k,max) has to visit every point of [0,max)^d exactly once, with the
   * 0th component running fastest, i.e. in dLinearised order.
   */
  void testDForVisitsAllPointsInLinearOrder();

  /**
   * dfor(k,maxVector) with individual per-axis ranges (including a degenerate
   * axis of extent one, as produced by the blockstructured face loops) has to
   * enumerate the full box exactly once.
   */
  void testDForWithPerAxisRange();

  /**
   * The scalar dInc(counter,int) and the vector dInc(counter,Vector) implement
   * the same operation and must agree for every counter, canonical or not.
   */
  void testDIncOverloadConsistency();

  /**
   * dLinearised(counter,max) maps a counter to sum_d counter(d)*max^d.
   */
  void testDLinearised();

  /**
   * dfore(k,max,fixedDim,0) keeps the fixed dimension at zero and visits every
   * combination of the remaining dimensions exactly once.
   */
  void testDForeKeepsFixedDimensionAndIsComplete();

  /**
   * dCmp returns true iff every component is within range, for both the scalar
   * and the per-axis overload.
   */
  void testDCmp();

  /**
   * dfor2 enumerates the 2^d cube corners exactly once and its scalar counter
   * equals dLinearised(corner,2).
   */
  void testDFor2EnumeratesCornersInLinearisedOrder();

  /**
   * dfor3 enumerates the 3^d stencil exactly once in dLinearised order.
   */
  void testDFor3EnumeratesStencil();

  /**
   * zfor3 is a space-filling (Peano) loop and therefore has to visit every cell
   * of [0,3)^d exactly once for every initial direction.
   */
  void testZFor3IsAHamiltonianPath();

  /**
   * The generic zfor (and the dInc(...,LoopDirection&) behind it) has to be a
   * Hamiltonian path over [0,max)^d for every initial direction and arbitrary
   * max, not just the table-supported max=3.
   */
  void testGenericZForIsAHamiltonianPath();

  /**
   * For max=3 the generic zfor must reproduce the table-driven zfor3 cell for
   * cell - this cross-validates both implementations against each other.
   */
  void testGenericZForMatchesZFor3();

public:
  LoopTest();
  ~LoopTest() override final = default;

  void run() override final;
};
