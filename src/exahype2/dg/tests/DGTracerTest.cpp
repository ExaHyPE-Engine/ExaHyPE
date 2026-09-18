// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DGTracerTest.h"

#include "exahype2/dg/Tracer.h"
#include "exahype2/dg/tests/TestUtils.h"
#include "peano4/Loop.h"
#include "peano4/dataview/CellMarker.h"

namespace {
  constexpr int Order                = 1;
  constexpr int NodesPerAxis         = Order + 1;
  constexpr int NodesPerCell         = NodesPerAxis * NodesPerAxis * (DIMENSIONS == 3 ? NodesPerAxis : 1);
  constexpr int MaxParticleAttribute = 4;

  /**
   * Holds the position a projection reads and the attributes it writes.
   *
   * The attributes start at zero, so a test tells an attribute a projection wrote from one
   * it left alone.
   */
  struct TracerParticle final {
    tarch::la::Vector<DIMENSIONS, double> x{};
    double                                data[MaxParticleAttribute]{};

    [[nodiscard]] const tarch::la::Vector<DIMENSIONS, double>& getX() const { return x; }

    void setData(const int index, const double value) {
      assertion(index >= 0 and index < MaxParticleAttribute, index);
      data[index] = value;
    }
  };

  /** Returns a cell marker of width 0.2 whose centre lies away from the origin. */
  peano4::dataview::CellMarker createCellMarker() {
    peano4::dataview::CellMarker marker;
    marker.h = tarch::la::Vector<DIMENSIONS, double>(0.2);
#if DIMENSIONS == 2
    marker.centre = {0.4, 0.6};
#else
    marker.centre = {0.4, 0.6, 0.8};
#endif
    return marker;
  }

  /** Returns a particle that sits inside the cell of createCellMarker() and off its nodes. */
  TracerParticle createParticle() {
    TracerParticle particle;
#if DIMENSIONS == 2
    particle.x = {0.45, 0.62};
#else
    particle.x = {0.45, 0.62, 0.83};
#endif
    return particle;
  }

  /**
   * Writes the coordinates of every quadrature node into that node's DIMENSIONS attributes.
   *
   * Q holds NodesPerCell nodes with DIMENSIONS values each, so the polynomial over Q maps
   * a point of the cell onto its own coordinates.
   */
  void fillNodeCoordinates(const peano4::dataview::CellMarker& cellMarker, double* NOALIAS const Q) {
    peano4::dForEach(NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& dof) {
      const int linearisedDoF = peano4::dLinearised(dof, NodesPerAxis);
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        Q[linearisedDoF * DIMENSIONS + axis] =
          cellMarker.getOffset()(axis) + exahype2::dg::tests::QuadratureNodes1DP1[dof(axis)] * cellMarker.getH()(axis);
      }
    });
  }
} // namespace

exahype2::dg::tests::DGTracerTest::DGTracerTest() :
  TestCase("exahype2::dg::tests::DGTracerTest") {}

void exahype2::dg::tests::DGTracerTest::run() {
  testMethod(testProjectValueOntoParticle);
  testMethod(testProjectAllValuesOntoParticle);
}

/** The projection writes the interpolated source attribute into the destination entry alone. */
void exahype2::dg::tests::DGTracerTest::testProjectValueOntoParticle() {
  const peano4::dataview::CellMarker cellMarker = createCellMarker();
  TracerParticle                     particle   = createParticle();

  double Q[NodesPerCell * DIMENSIONS];
  fillNodeCoordinates(cellMarker, Q);

  exahype2::dg::projectValueOntoParticle<TracerParticle, 1, 0>(
    cellMarker,
    Order,
    QuadratureNodes1DP1,
    DIMENSIONS,
    Q,
    particle
  );

  validateNumericalEquals(particle.data[0], particle.x(1));
  validateNumericalEquals(particle.data[1], 0.0);
}

/** The projection writes every interpolated attribute into the particle entry of the same index. */
void exahype2::dg::tests::DGTracerTest::testProjectAllValuesOntoParticle() {
  const peano4::dataview::CellMarker cellMarker = createCellMarker();
  TracerParticle                     particle   = createParticle();

  double Q[NodesPerCell * DIMENSIONS];
  fillNodeCoordinates(cellMarker, Q);

  exahype2::dg::projectAllValuesOntoParticle(cellMarker, Order, QuadratureNodes1DP1, DIMENSIONS, Q, particle);

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    validateNumericalEqualsWithParams1(particle.data[axis], particle.x(axis), axis);
  }
}
