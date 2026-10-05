// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TracingTest.h"

#include "exahype2/fv/FiniteVolumeScheme.h"
#include "exahype2/fv/Tracing.h"
#include "exahype2/particles/Forces.h"
#include "exahype2/particles/ParticleSet.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "peano4/Partition.h"
#include "peano4/Spacetree.h"
#include "peano4/tests/TestGrids.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"
#include "tarch/la/Vector.h"

namespace {
  TARGET_DECLARE
  constexpr int Depth                = 1;
  constexpr int PatchSize            = 3;
  constexpr int NumberOfData         = DIMENSIONS;
  constexpr int VolumesPerPatch      = peano4::power(PatchSize, DIMENSIONS);
  constexpr int ValuesPerPatch       = VolumesPerPatch * NumberOfData;
  constexpr int NumberOfParticleData = exahype2::particles::ParticleMassAttribute + 1;
  TARGET_DECLARE_END

  const tarch::la::Vector<DIMENSIONS, double> DomainOffset(0.0);
  const tarch::la::Vector<DIMENSIONS, double> DomainWidth(1.0);

  using CellPool = tarch::accelerator::ChunkedMemoryPool<double>;
  using Set      = exahype2::particles::ParticleSet<NumberOfParticleData>;

  double getLinearField(const int unknown, const tarch::la::Vector<DIMENSIONS, double>& x) {
    double value = unknown + 1.0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      value += (axis + 2.0) * x(axis);
    }
    return value;
  }

  std::vector<double> getLinearPatch(
    const tarch::la::Vector<DIMENSIONS, double>& centre,
    const tarch::la::Vector<DIMENSIONS, double>& width
  ) {
    std::vector<double> Q(ValuesPerPatch, 0.0);
    peano4::dForEach(PatchSize, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
      const auto volumeCentre = exahype2::fv::getVolumeCentre(centre, width, PatchSize, volume);
      for (int unknown = 0; unknown < NumberOfData; unknown++) {
        Q[static_cast<std::size_t>(peano4::dLinearised(volume, PatchSize)) * NumberOfData + unknown] =
          getLinearField(unknown, volumeCentre);
      }
    });
    return Q;
  }

  TARGET_DECLARE
  struct FillVelocity final {
    peano4::Partition partition{};
    CellPool::View    solution{};
    double            velocity{0.0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int entry) const {
      double* const Q = solution.getChunk(partition.getCells()[entry]);
      for (int volume = 0; volume < VolumesPerPatch; volume++) {
        for (int unknown = 0; unknown < NumberOfData; unknown++) {
          Q[volume * NumberOfData + unknown] = unknown == 0 ? velocity : 0.0;
        }
      }
    }
  };
  TARGET_DECLARE_END

  const peano4::Partition& buildGrid() {
    peano4::tests::buildRegularGrid(Depth);
    return peano4::Spacetree::getInstance().getPartition();
  }

  /**
   * Regular grid with a constant velocity along the first axis.
   */
  struct Grid final {
    const peano4::Partition& partition;
    CellPool                 solution{ValuesPerPatch};

    explicit Grid(const double velocity) :
      partition(buildGrid()) {
      solution.reserve(partition.getNumberOfSlots());
      tarch::accelerator::parallelFor(
        tarch::accelerator::Range(partition.getCells().size),
        FillVelocity{partition.onDevice(), solution.getView(), velocity}
      );
    }
  };

  /**
   * Returns the forces of a tracer that takes the fluid velocity and moves with it.
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

exahype2::fv::tests::TracingTest::TracingTest() :
  tarch::tests::TestCase("exahype2::fv::tests::TracingTest") {}

void
exahype2::fv::tests::TracingTest::run() {
  testMethod(testPiecewiseConstantReadsTheVolumeThatContainsThePosition);
  testMethod(testPiecewiseLinearReproducesALinearField);
  testMethod(testInterpolationOnRectangularPatches);
  testMethod(testClampIntoPatchProjectsOntoTheVolumeCentres);
  testMethod(testTracingCarriesAParticleWithTheFluid);
  testMethod(testStaticSamplingPreservesParticleData);
}

void
exahype2::fv::tests::TracingTest::testInterpolationOnRectangularPatches() {
  const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
  tarch::la::Vector<DIMENSIONS, double>       width;
  tarch::la::Vector<DIMENSIONS, double>       position;
  tarch::la::Vector<DIMENSIONS, double>       volumeCentre;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    width(axis)        = 0.3 * (axis + 1);
    position(axis)     = centre(axis) + 0.2 * width(axis);
    volumeCentre(axis) = centre(axis) + width(axis) / 3.0;
  }
  const auto values = getLinearPatch(centre, width);
  for (int unknown = 0; unknown < NumberOfData; unknown++) {
    validateNumericalEquals(
      (exahype2::fv::interpolatePiecewiseConstant<PatchSize, NumberOfData>(
        centre,
        width,
        values.data(),
        position,
        unknown
      )),
      getLinearField(unknown, volumeCentre)
    );
    validateNumericalEquals(
      (exahype2::fv::interpolatePiecewiseLinear<PatchSize, NumberOfData>(
        centre,
        width,
        values.data(),
        position,
        unknown
      )),
      getLinearField(unknown, position)
    );
  }
}

/**
 * Static sampling writes only the named attributes, also in a set without motion attributes.
 */
void
exahype2::fv::tests::TracingTest::testStaticSamplingPreservesParticleData() {
  const Grid grid(2.0);
  const auto sample = [this, &grid](auto& particles) {
    using SampleSet                = std::remove_reference_t<decltype(particles)>;
    constexpr int       Attributes = SampleSet::ValuesPerParticle - DIMENSIONS;
    std::vector<double> values(SampleSet::ValuesPerParticle, 42.0);
    std::fill_n(values.begin(), DIMENSIONS, 0.1);
    particles.insert(grid.partition, values);
    exahype2::particles::Forces forces;
    forces.interpolationScheme = exahype2::particles::InterpolationScheme::PiecewiseLinear;
    exahype2::fv::traceParticles<PatchSize, NumberOfData, Attributes, 0>(
      grid.partition,
      grid.solution,
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

void
exahype2::fv::tests::TracingTest::testPiecewiseConstantReadsTheVolumeThatContainsThePosition() {
  const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> width(0.3);
  const auto                                  Q = getLinearPatch(centre, width);

  tarch::la::Vector<DIMENSIONS, double> position(0.5);
  position(0) = 0.5 - 0.15 + 0.15;
  tarch::la::Vector<DIMENSIONS, int> volume(1);
  volume(0) = 1;
  tarch::la::Vector<DIMENSIONS, double> volumeCentre;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    volumeCentre(axis) = centre(axis) - 0.5 * width(axis) + (volume(axis) + 0.5) * 0.1;
  }
  for (int unknown = 0; unknown < NumberOfData; unknown++) {
    validateNumericalEqualsWithParams1(
      (exahype2::fv::interpolatePiecewiseConstant<PatchSize, NumberOfData>(centre, width, Q.data(), position, unknown)),
      getLinearField(unknown, volumeCentre),
      unknown
    );
  }

  tarch::la::Vector<DIMENSIONS, double> beyondThePatch(0.5);
  beyondThePatch(0)                                      = 2.0;
  tarch::la::Vector<DIMENSIONS, double> lastVolumeCentre = volumeCentre;
  lastVolumeCentre(0)                                    = centre(0) - 0.5 * width(0) + 2.5 * 0.1;
  validateNumericalEquals(
    (exahype2::fv::interpolatePiecewiseConstant<PatchSize, NumberOfData>(centre, width, Q.data(), beyondThePatch, 0)),
    getLinearField(0, lastVolumeCentre)
  );
}

/**
 * The interpolation reproduces a linear field between volume centres and returns the value of the last volume
 * beyond the last centre.
 */
void
exahype2::fv::tests::TracingTest::testPiecewiseLinearReproducesALinearField() {
  const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> width(0.3);
  const auto                                  Q = getLinearPatch(centre, width);

  tarch::la::Vector<DIMENSIONS, double> position(0.5);
  position(0) = 0.53;
  for (int unknown = 0; unknown < NumberOfData; unknown++) {
    validateNumericalEqualsWithParams1(
      (exahype2::fv::interpolatePiecewiseLinear<PatchSize, NumberOfData>(centre, width, Q.data(), position, unknown)),
      getLinearField(unknown, position),
      unknown
    );
  }

  tarch::la::Vector<DIMENSIONS, double> belowTheCentre(0.5);
  belowTheCentre(0) = 0.47;
  validateNumericalEquals(
    (exahype2::fv::interpolatePiecewiseLinear<PatchSize, NumberOfData>(centre, width, Q.data(), belowTheCentre, 0)),
    getLinearField(0, belowTheCentre)
  );

  tarch::la::Vector<DIMENSIONS, double> beyondTheLastCentre(0.5);
  beyondTheLastCentre(0) = 0.5 + 0.15 - 0.01;
  tarch::la::Vector<DIMENSIONS, double> lastCentre(0.5);
  lastCentre(0) = 0.5 + 0.15 - 0.05;
  validateNumericalEquals(
    (
      exahype2::fv::interpolatePiecewiseLinear<PatchSize, NumberOfData>(centre, width, Q.data(), beyondTheLastCentre, 0)
    ),
    getLinearField(0, lastCentre)
  );
}

void
exahype2::fv::tests::TracingTest::testClampIntoPatchProjectsOntoTheVolumeCentres() {
  const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
  const tarch::la::Vector<DIMENSIONS, double> width(0.3);

  const tarch::la::Vector<DIMENSIONS, double> inside(0.4);
  validateNumericalEquals((exahype2::fv::clampIntoPatch<PatchSize>(centre, width, inside))(0), 0.4);

  tarch::la::Vector<DIMENSIONS, double> outside(0.5);
  outside(0)         = 0.9;
  const auto clamped = exahype2::fv::clampIntoPatch<PatchSize>(centre, width, outside);
  validateNumericalEquals(clamped(0), 0.5 + 0.15 - 0.05);
  validateNumericalEquals(clamped(DIMENSIONS - 1), 0.5);
}

/**
 * One explicit Euler step moves the tracer by the fluid velocity times the step size and keeps it
 * in the domain. The tracer samples the fluid velocity at its new position, also after a wall
 * reflected it.
 */
void
exahype2::fv::tests::TracingTest::testTracingCarriesAParticleWithTheFluid() {
  constexpr double Velocity          = 1.0;
  constexpr double TimeStepSize      = 0.5;
  constexpr int    FirstAxisPeriodic = 1;
  const Grid       grid(Velocity);

  Set particles;
  particles.insert(grid.partition, getTracerAt(0.1));
  validateEquals(peano4::tests::getGlobalSumOf(particles.getSize()), 1);
  exahype2::fv::traceParticles<
    PatchSize,
    NumberOfData,
    NumberOfParticleData,
    exahype2::particles::ParticleVelocityAttribute,
    exahype2::particles::ParticleVelocityAttribute + DIMENSIONS - 1
  >(grid.partition, grid.solution, TimeStepSize, particles, getFluidTracer(), DomainOffset, DomainWidth, 0);
  validateEquals(peano4::tests::getGlobalSumOf(particles.getSize()), 1);
  particles.download();
  for (int index = 0; index < particles.getSize(); index++) {
    validateNumericalEquals(particles.getPosition(index)(0), 0.1 + Velocity * TimeStepSize);
    validateNumericalEquals(particles.getPosition(index)(DIMENSIONS - 1), 0.5);
    validateNumericalEquals(particles.getData(index)[exahype2::particles::ParticleVelocityAttribute], Velocity);
    validateEquals(particles.getSlot(index), Set::getSlotAt(grid.partition, particles.getPosition(index)));
  }

  Set wrapped;
  wrapped.insert(grid.partition, getTracerAt(0.9));
  exahype2::fv::traceParticles<
    PatchSize,
    NumberOfData,
    NumberOfParticleData,
    exahype2::particles::ParticleVelocityAttribute,
    exahype2::particles::ParticleVelocityAttribute + DIMENSIONS - 1
  >(grid.partition,
    grid.solution,
    TimeStepSize,
    wrapped,
    getFluidTracer(),
    DomainOffset,
    DomainWidth,
    FirstAxisPeriodic);
  validateEquals(peano4::tests::getGlobalSumOf(wrapped.getSize()), 1);
  wrapped.download();
  for (int index = 0; index < wrapped.getSize(); index++) {
    validateNumericalEquals(wrapped.getPosition(index)(0), 0.9 + Velocity * TimeStepSize - 1.0);
  }

  Set reflected;
  reflected.insert(grid.partition, getTracerAt(0.9));
  exahype2::fv::traceParticles<
    PatchSize,
    NumberOfData,
    NumberOfParticleData,
    exahype2::particles::ParticleVelocityAttribute,
    exahype2::particles::ParticleVelocityAttribute + DIMENSIONS - 1
  >(grid.partition, grid.solution, TimeStepSize, reflected, getFluidTracer(), DomainOffset, DomainWidth, 0);
  validateEquals(peano4::tests::getGlobalSumOf(reflected.getSize()), 1);
  reflected.download();
  for (int index = 0; index < reflected.getSize(); index++) {
    validateNumericalEquals(reflected.getPosition(index)(0), 1.0 - (0.9 + Velocity * TimeStepSize - 1.0));
    validateNumericalEquals(reflected.getData(index)[exahype2::particles::ParticleVelocityAttribute], Velocity);
  }
}
