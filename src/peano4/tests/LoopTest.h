// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::tests {
  class LoopTest;
} // namespace peano4::tests

class peano4::tests::LoopTest final : public tarch::tests::TestCase {
private:
  /**
   * dForEach(max, body) has to visit every point of [0,max)^d exactly once, with
   * the 0th component running fastest, i.e. in dLinearised order.
   */
  void testDForEachVisitsAllPointsInLinearOrder();

  /**
   * dForEach(maxVector, body) with individual per-axis ranges (including a
   * degenerate axis of extent one, as produced by the blockstructured face
   * loops) has to enumerate the full box exactly once.
   */
  void testDForEachWithPerAxisRange();

  /**
   * dStartVector seeds a d-dimensional loop: the plain overload is the zero
   * vector, dStartVector(dim,value) is zero except for the given dimension, and
   * dStartVector(max,direction) seeds the oscillating z-loop with 0 where the
   * direction bit is set and max-1 where it is not.
   */
  void testDStartVector();

  /**
   * The scalar dIncrement(counter,int) and the vector dIncrement(counter,Vector) implement
   * the same operation and must agree for every counter, canonical or not.
   */
  void testDIncrementOverloadConsistency();

  /**
   * dIncrement(counter,max,doNotExamine) is the increment behind dForEachExclusive:
   * it leaves the doNotExamine axis untouched and walks the remaining axes over
   * [0,max), terminating by pushing one free axis to max.
   */
  void testDIncrementExclusive();

  /**
   * dLinearised(counter,max) maps a counter to sum_d counter(d)*max^d.
   */
  void testDLinearised();

  /**
   * dForEachExclusive(max, fixedDim, value, body) keeps the fixed dimension at
   * its start value and visits every combination of the remaining dimensions
   * exactly once.
   */
  void testDForEachExclusiveKeepsFixedDimensionAndIsComplete();

  /**
   * dCompare returns true if every component is within range, for both the scalar
   * and the per-axis overload.
   */
  void testDCompare();

  /**
   * dCompareExclusive returns true if every component except the ignored dimension is
   * within range; the ignored component must not be consulted. This is the
   * comparison behind the exclusive dForEachExclusive loop.
   */
  void testDCompareExclusive();

  /**
   * dForEachCorner enumerates the 2^d cube corners exactly once and its scalar
   * argument equals dLinearised(corner,2).
   */
  void testDForEachCornerEnumeratesCornersInLinearisedOrder();

  /**
   * dForEachStencil enumerates the 3^d stencil exactly once in dLinearised order.
   */
  void testDForEachStencilEnumeratesStencil();

  /**
   * zForEach3 is a space-filling (Peano) loop and therefore has to visit every
   * cell of [0,3)^d exactly once for every initial direction.
   */
  void testZForEach3IsAHamiltonianPath();

  /**
   * The generic zForEach (and the dIncrement(...,LoopDirection&) behind it) has to be
   * a Hamiltonian path over [0,max)^d for every initial direction and arbitrary
   * max, not just the table-supported max=3.
   */
  void testGenericZForEachIsAHamiltonianPath();

  /**
   * For max=3 the generic zForEach must reproduce the table-driven zForEach3
   * cell for cell - this cross-validates both implementations against each other.
   */
  void testGenericZForEachMatchesZForEach3();

public:
  LoopTest();
  ~LoopTest() override = default;

  void run() override;
};
