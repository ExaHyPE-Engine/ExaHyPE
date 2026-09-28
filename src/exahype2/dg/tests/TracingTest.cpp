// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TracingTest.h"

#include "exahype2/dg/Basis.h"
#include "exahype2/dg/Tracing.h"
#include "exahype2/particles/Forces.h"
#include "exahype2/particles/ParticleSet.h"
#include "peano4/Loop.h"
#include "peano4/Partition.h"
#include "peano4/Spacetree.h"
#include "peano4/tests/TestGrids.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"
#include "tarch/la/Vector.h"

namespace {
  TARGET_DECLARE
  constexpr int Depth                = 1;
  constexpr int NodesPerAxis         = 2;
  constexpr int NumberOfData         = DIMENSIONS;
  constexpr int NodesPerCell         = exahype2::dg::getNodesPerCell(NodesPerAxis);
  constexpr int ValuesPerCell        = NodesPerCell * NumberOfData;
  constexpr int NumberOfParticleData = exahype2::particles::ParticleMassAttribute + 1;
  TARGET_DECLARE_END

  constexpr double QuadratureNodes1D[NodesPerAxis] = {0.21132486540518713, 0.7886751345948129};

  const tarch::la::Vector<DIMENSIONS, double> DomainOffset(0.0);
  const tarch::la::Vector<DIMENSIONS, double> DomainWidth(1.0);

  using CellPool = tarch::accelerator::ChunkedMemoryPool<double>;
  using Set      = exahype2::particles::ParticleSet<NumberOfParticleData>;

  /**
   * Returns the linear field of unknown k at x, which is k + 1 plus the sum over the axes of (axis + 2) x(axis).
   */
  GPU_CALLABLE_METHOD double getLinearField(const int unknown, const tarch::la::Vector<DIMENSIONS, double>& x) {
    double value = unknown + 1.0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      value += (axis + 2.0) * x(axis);
    }
    return value;
  }

  /**
   * Returns the values of the linear field at the nodes of a cell.
   */
  std::vector<double> getLinearCell(
    const tarch::la::Vector<DIMENSIONS, double>& centre,
    const tarch::la::Vector<DIMENSIONS, double>& width
  ) {
    std::vector<double> Q(ValuesPerCell, 0.0);
    peano4::dForEach(NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
      const auto position = exahype2::dg::getQuadraturePoint(centre, width, node, QuadratureNodes1D);
      for (int unknown = 0; unknown < NumberOfData; unknown++) {
        Q[static_cast<std::size_t>(peano4::dLinearised(node, NodesPerAxis)) * NumberOfData + unknown] =
          getLinearField(unknown, position);
      }
    });
    return Q;
  }

  TARGET_DECLARE
  /**
   * Writes velocity into the first unknown of every node of a cell and zero into its other unknowns.
   */
  struct FillNodeVelocity final {
    peano4::Partition partition{};
    CellPool::View    solution{};
    double            velocity{0.0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int entry) const {
      double* const Q = solution.getChunk(partition.getCells()[entry]);
      for (int node = 0; node < NodesPerCell; node++) {
        for (int unknown = 0; unknown < NumberOfData; unknown++) {
          Q[node * NumberOfData + unknown] = unknown == 0 ? velocity : 0.0;
        }
      }
    }
  };
  TARGET_DECLARE_END

  /**
   * Builds the regular grid of Depth levels partitioned over the ranks and returns the partition of this rank.
   */
  const peano4::Partition& buildGrid() {
    peano4::tests::buildRegularGrid(Depth);
    return peano4::Spacetree::getInstance().getPartition();
  }

  /**
   * Holds the partition of a regular grid partitioned over the ranks, a solution pool with a
   * constant velocity along the first axis and the quadrature nodes in the memory the dispatches
   * read.
   */
  struct Grid final {
    const peano4::Partition&           partition;
    CellPool                           solution{ValuesPerCell};
    tarch::accelerator::Buffer<double> quadratureNodes{tarch::MemoryLocation::Device};

    explicit Grid(const double velocity) :
      partition(buildGrid()) {
      solution.reserve(partition.getNumberOfSlots());
      quadratureNodes.setValues(std::vector<double>(QuadratureNodes1D, QuadratureNodes1D + NodesPerAxis));
      tarch::accelerator::parallelFor(
        tarch::accelerator::Range(partition.getCells().size),
        FillNodeVelocity{partition.onDevice(), solution.getView(), velocity}
      );
    }
  };

  TARGET_DECLARE
  constexpr int NodesPerFace  = exahype2::dg::getNodesPerFace(NodesPerAxis);
  constexpr int ValuesPerFace = 2 * NodesPerFace * NumberOfData;
  TARGET_DECLARE_END

  using FacePool = tarch::accelerator::ChunkedMemoryPool<double>;

  TARGET_DECLARE
  /**
   * Writes the linear field into the lower half of the nodes of one face and the field plus
   * upperShift into its upper half.
   */
  struct FillFaceStates final {
    peano4::Partition partition{};
    FacePool::View    faceStates{};
    const double*     quadratureNodes{nullptr};
    double            upperShift{0.0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int face) const {
      const int     normal = partition.getFaceNormal(face);
      double* const lower  = faceStates.getChunk(face);
      double* const upper  = lower + static_cast<std::ptrdiff_t>(NodesPerFace) * NumberOfData;
      for (int node = 0; node < NodesPerFace; node++) {
        const auto                            nodeIndex = peano4::dDelinearisedExclusive(node, NodesPerAxis, normal);
        tarch::la::Vector<DIMENSIONS, double> position;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          position(axis) = partition.getFaceCentre(face, axis);
          if (axis != normal) {
            position(axis) += (quadratureNodes[nodeIndex(axis)] - 0.5) * partition.getFaceWidth(face, axis);
          }
        }
        for (int unknown = 0; unknown < NumberOfData; unknown++) {
          lower[node * NumberOfData + unknown] = getLinearField(unknown, position);
          upper[node * NumberOfData + unknown] = getLinearField(unknown, position) + upperShift;
        }
      }
    }
  };
  TARGET_DECLARE_END

  /**
   * Writes the linear field into the lower half of every face node and the field plus upperShift into its
   * upper half.
   */
  void fillFaceStates(const Grid& grid, FacePool& faceStates, const double upperShift) {
    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(grid.partition.getNumberOfFaces()),
      FillFaceStates{grid.partition.onDevice(), faceStates.getView(), grid.quadratureNodes.getData(), upperShift}
    );
  }

  /**
   * The forces of a tracer that copies the fluid velocity into its own velocity and moves with it.
   */
  exahype2::particles::Forces getFluidTracer() {
    exahype2::particles::Forces forces;
    forces.interpolationScheme = exahype2::particles::InterpolationScheme::PiecewiseLinear;
    forces.integrationScheme   = exahype2::particles::IntegrationScheme::ExplicitEuler;
    forces.forceMethod         = 0;
    return forces;
  }

  std::vector<double> getTracerAt(const double first) {
    std::vector<double> values(Set::ValuesPerParticle, 0.5);
    values[0] = first;
    for (int entry = 0; entry < NumberOfParticleData; entry++) {
      values[static_cast<std::size_t>(DIMENSIONS + entry)] = 0.0;
    }
    return values;
  }
} // namespace

exahype2::dg::tests::TracingTest::TracingTest() :
  tarch::tests::TestCase("exahype2::dg::tests::TracingTest") {}

void
exahype2::dg::tests::TracingTest::run() {
  testMethod(testInterpolateFieldsWritesThePolynomialIntoTheNamedEntries);
  testMethod(testTracingCarriesAParticleWithTheFluid);
  testMethod(testStaticSamplingPreservesParticleData);
  testMethod(testFaceTracingWritesTheMeanAndTheJumpOfTheTraces);
}

/**
 * Static sampling writes only its named attributes, also into a set without motion attributes.
 */
void
exahype2::dg::tests::TracingTest::testStaticSamplingPreservesParticleData() {
  const Grid grid(2.0);
  const auto sample = [this, &grid](auto& particles) {
    using SampleSet                = std::remove_reference_t<decltype(particles)>;
    constexpr int       Attributes = SampleSet::ValuesPerParticle - DIMENSIONS;
    std::vector<double> values(SampleSet::ValuesPerParticle, 42.0);
    std::fill_n(values.begin(), DIMENSIONS, 0.1);
    particles.insert(grid.partition, values);
    exahype2::particles::Forces forces;
    forces.interpolationScheme = exahype2::particles::InterpolationScheme::PiecewiseLinear;
    exahype2::dg::traceParticles<NodesPerAxis, NumberOfData, Attributes, 0>(
      grid.partition,
      grid.solution,
      grid.quadratureNodes.getData(),
      0.25,
      particles,
      forces,
      DomainOffset,
      DomainWidth,
      0
    );
    particles.download();
    validateEquals(peano4::tests::getGlobalSumOf(particles.getSize()), 1);
    for (int index = 0; index < particles.getSize(); index++) {
      validateNumericalEquals(particles.getData(index)[0], 2.0);
      for (int attribute = 1; attribute < Attributes; attribute++) {
        validateEquals(particles.getData(index)[attribute], 42.0);
      }
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        validateEquals(particles.getPosition(index)(axis), 0.1);
      }
    }
  };
  exahype2::particles::ParticleSet<1> small;
  Set                                 full;
  sample(small);
  sample(full);
}

/**
 * A particle on the plane of the first cell boundary takes the mean of the two traces of the
 * face polynomial at its position and, for the named field, the upper trace minus the lower one.
 */
void
exahype2::dg::tests::TracingTest::testFaceTracingWritesTheMeanAndTheJumpOfTheTraces() {
  constexpr double UpperShift = 10.0;
  const Grid       grid(0.0);
  FacePool         faceStates{ValuesPerFace};
  faceStates.reserve(grid.partition.getNumberOfFaces());
  fillFaceStates(grid, faceStates, UpperShift);

  constexpr double Plane = 1.0 / peano4::SubdivisionFactor;
  Set              particles;
  particles.insert(grid.partition, getTracerAt(Plane));
  validateEquals(peano4::tests::getGlobalSumOf(particles.getSize()), 1);
  exahype2::dg::traceFaceValues<NodesPerAxis, NumberOfData, NumberOfParticleData, 1>(
    grid.partition,
    faceStates,
    grid.quadratureNodes.getData(),
    particles,
    0,
    Plane
  );
  particles.download();
  for (int index = 0; index < particles.getSize(); index++) {
    const auto position = particles.getPosition(index);
    validateNumericalEquals(particles.getData(index)[0], getLinearField(0, position) + 0.5 * UpperShift);
    validateNumericalEquals(particles.getData(index)[1], UpperShift);
  }

  Set away;
  away.insert(grid.partition, getTracerAt(0.5 * Plane));
  exahype2::dg::traceFaceValues<NodesPerAxis, NumberOfData, NumberOfParticleData, 1>(
    grid.partition,
    faceStates,
    grid.quadratureNodes.getData(),
    away,
    0,
    Plane
  );
  away.download();
  for (int index = 0; index < away.getSize(); index++) {
    validateNumericalEquals(away.getData(index)[0], 0.0);
    validateNumericalEquals(away.getData(index)[1], 0.0);
  }
  faceStates.release();
}

/**
 * The interpolation writes unknown k into the k-th named entry and leaves the other entries alone.
 */
void
exahype2::dg::tests::TracingTest::testInterpolateFieldsWritesThePolynomialIntoTheNamedEntries() {
  const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> width(0.4);
  const tarch::la::Vector<DIMENSIONS, double> position(0.37);
  const std::vector<double>                   Q = getLinearCell(centre, width);

  double                        values[Set::ValuesPerParticle] = {};
  exahype2::particles::Particle particle;
  particle.values          = values;
  particle.numberOfData    = NumberOfParticleData;
  const double* const data = values + DIMENSIONS;

  exahype2::dg::interpolateFields<NodesPerAxis, NumberOfData, double, 3, 1>(
    exahype2::particles::InterpolationScheme::PiecewiseLinear,
    centre,
    width,
    QuadratureNodes1D,
    Q.data(),
    position,
    particle
  );
  validateNumericalEquals(data[3], getLinearField(0, position));
  validateNumericalEquals(data[1], getLinearField(1, position));
  validateNumericalEquals(data[0], 0.0);
  validateNumericalEquals(data[2], 0.0);

  exahype2::dg::interpolateFields<NodesPerAxis, NumberOfData, double, 0>(
    exahype2::particles::InterpolationScheme::None,
    centre,
    width,
    QuadratureNodes1D,
    Q.data(),
    position,
    particle
  );
  validateNumericalEquals(data[0], 0.0);
}

/**
 * A tracer takes the fluid velocity of the polynomial and moves with it, wraps through a periodic
 * wall and reflects at a solid one.
 */
void
exahype2::dg::tests::TracingTest::testTracingCarriesAParticleWithTheFluid() {
  constexpr double Velocity          = 1.0;
  constexpr double TimeStepSize      = 0.5;
  constexpr int    FirstAxisPeriodic = 1;
  const Grid       grid(Velocity);

  Set particles;
  particles.insert(grid.partition, getTracerAt(0.1));
  validateEquals(peano4::tests::getGlobalSumOf(particles.getSize()), 1);
  exahype2::dg::traceParticles<
    NodesPerAxis,
    NumberOfData,
    NumberOfParticleData,
    exahype2::particles::ParticleVelocityAttribute,
    exahype2::particles::ParticleVelocityAttribute + DIMENSIONS - 1
  >(grid.partition,
    grid.solution,
    grid.quadratureNodes.getData(),
    TimeStepSize,
    particles,
    getFluidTracer(),
    DomainOffset,
    DomainWidth,
    0);
  validateEquals(peano4::tests::getGlobalSumOf(particles.getSize()), 1);
  for (int index = 0; index < particles.getSize(); index++) {
    validateNumericalEquals(particles.getPosition(index)(0), 0.1 + Velocity * TimeStepSize);
    validateNumericalEquals(particles.getPosition(index)(DIMENSIONS - 1), 0.5);
    validateNumericalEquals(particles.getData(index)[exahype2::particles::ParticleVelocityAttribute], Velocity);
    validateEquals(particles.getSlot(index), Set::getSlotAt(grid.partition, particles.getPosition(index)));
  }

  Set wrapped;
  wrapped.insert(grid.partition, getTracerAt(0.9));
  exahype2::dg::traceParticles<
    NodesPerAxis,
    NumberOfData,
    NumberOfParticleData,
    exahype2::particles::ParticleVelocityAttribute,
    exahype2::particles::ParticleVelocityAttribute + DIMENSIONS - 1
  >(grid.partition,
    grid.solution,
    grid.quadratureNodes.getData(),
    TimeStepSize,
    wrapped,
    getFluidTracer(),
    DomainOffset,
    DomainWidth,
    FirstAxisPeriodic);
  validateEquals(peano4::tests::getGlobalSumOf(wrapped.getSize()), 1);
  for (int index = 0; index < wrapped.getSize(); index++) {
    validateNumericalEquals(wrapped.getPosition(index)(0), 0.9 + Velocity * TimeStepSize - 1.0);
  }

  Set reflected;
  reflected.insert(grid.partition, getTracerAt(0.9));
  exahype2::dg::traceParticles<
    NodesPerAxis,
    NumberOfData,
    NumberOfParticleData,
    exahype2::particles::ParticleVelocityAttribute,
    exahype2::particles::ParticleVelocityAttribute + DIMENSIONS - 1
  >(grid.partition,
    grid.solution,
    grid.quadratureNodes.getData(),
    TimeStepSize,
    reflected,
    getFluidTracer(),
    DomainOffset,
    DomainWidth,
    0);
  validateEquals(peano4::tests::getGlobalSumOf(reflected.getSize()), 1);
  for (int index = 0; index < reflected.getSize(); index++) {
    validateNumericalEquals(reflected.getPosition(index)(0), 1.0 - (0.9 + Velocity * TimeStepSize - 1.0));
    validateNumericalEquals(reflected.getData(index)[exahype2::particles::ParticleVelocityAttribute], -0.8 * Velocity);
  }
}
