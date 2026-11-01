// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/fv/Tracing.h"

#include "exahype2/fv/FiniteVolumeScheme.h"
#include "exahype2/particles/Forces.h"
#include "exahype2/particles/ParticleSet.h"
#include "peano4/CellConnectivity.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "peano4/Spacetree.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"
#include "tarch/la/Vector.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/peano4/TestGrids.h"

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
    peano4::CellConnectivity cellConnectivity{};
    CellPool::View           solution{};
    double                   velocity{0.0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int entry) const {
      double* const Q = solution.getChunk(cellConnectivity.getCells()[entry]);
      for (int volume = 0; volume < VolumesPerPatch; volume++) {
        for (int unknown = 0; unknown < NumberOfData; unknown++) {
          Q[volume * NumberOfData + unknown] = unknown == 0 ? velocity : 0.0;
        }
      }
    }
  };
  TARGET_DECLARE_END

  const peano4::CellConnectivity& buildGrid() {
    peano4::unittests::buildRegularGrid(Depth);
    return peano4::Spacetree::getInstance().getCellConnectivity();
  }

  /**
   * A regular grid with a constant velocity along the first axis.
   */
  struct Grid final {
    const peano4::CellConnectivity& cellConnectivity;
    CellPool                        solution{ValuesPerPatch};

    explicit Grid(const double velocity) :
      cellConnectivity(buildGrid()) {
      solution.reserve(cellConnectivity.getNumberOfSlots());
      tarch::accelerator::parallelFor(
        tarch::accelerator::Range(cellConnectivity.getCells().size),
        FillVelocity{cellConnectivity.onDevice(), solution.getView(), velocity}
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

  /**
   * Inserts into the set one particle whose coordinates are 0.1 and whose attributes are 42.
   */
  template <typename SampleSet>
  void insertParticleWithAttributes42(const Grid& grid, SampleSet& particles) {
    std::vector<double> values(SampleSet::ValuesPerParticle, 42.0);
    std::fill_n(values.begin(), DIMENSIONS, 0.1);
    particles.insert(grid.cellConnectivity, values);
  }

  /**
   * Samples the first unknown of the fluid into the first attribute of every particle of the set without moving it.
   */
  template <typename SampleSet>
  void sampleIntoTheFirstAttribute(const Grid& grid, SampleSet& particles) {
    constexpr int               Attributes = SampleSet::ValuesPerParticle - DIMENSIONS;
    exahype2::particles::Forces forces;
    forces.interpolationScheme = exahype2::particles::InterpolationScheme::PiecewiseLinear;
    exahype2::fv::traceParticles<PatchSize, NumberOfData, Attributes, 0>(
      grid.cellConnectivity,
      grid.solution,
      0.25,
      particles,
      forces,
      DomainOffset,
      DomainWidth,
      0
    );
    particles.download();
  }

  /**
   * Checks that every particle of the set stores the velocity two in its first attribute and keeps its other attributes
   * and its position.
   */
  template <typename SampleSet>
  void validateSampledParticles(const SampleSet& particles) {
    constexpr int Attributes = SampleSet::ValuesPerParticle - DIMENSIONS;
    for (int index = 0; index < particles.getSize(); index++) {
      VALIDATE_NUMERICAL_EQUALS(particles.getData(index)[0], 2.0);
      for (int attribute = 1; attribute < Attributes; attribute++) {
        VALIDATE_EQUALS(particles.getData(index)[attribute], 42.0);
      }
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        VALIDATE_EQUALS(particles.getPosition(index)(axis), 0.1);
      }
    }
  }
} // namespace

UNIT_TEST("The piecewise-constant interpolation reads the volume containing the position or the last one beyond it") {
  GIVEN("A patch of width 0.3 around 0.5 with the linear field, a position in its middle volume and one beyond it") {
    const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> width(0.3);
    const auto                                  patch = getLinearPatch(centre, width);

    tarch::la::Vector<DIMENSIONS, double> position(0.5);
    position(0) = 0.5 - 0.15 + 0.15;
    tarch::la::Vector<DIMENSIONS, double> beyondThePatch(0.5);
    beyondThePatch(0) = 2.0;

    WHEN("The interpolation reads every unknown at the position and the first unknown beyond the patch") {
      std::vector<double> valueAtThePosition;
      for (int unknown = 0; unknown < NumberOfData; unknown++) {
        valueAtThePosition.push_back(
          exahype2::fv::interpolatePiecewiseConstant<PatchSize, NumberOfData>(
            centre,
            width,
            patch.data(),
            position,
            unknown
          )
        );
      }
      const double valueBeyondThePatch = exahype2::fv::interpolatePiecewiseConstant<PatchSize, NumberOfData>(
        centre,
        width,
        patch.data(),
        beyondThePatch,
        0
      );

      THEN("Every unknown equals the field at the middle volume centre and beyond the patch at the last centre") {
        tarch::la::Vector<DIMENSIONS, int> volume(1);
        volume(0) = 1;
        tarch::la::Vector<DIMENSIONS, double> volumeCentre;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          volumeCentre(axis) = centre(axis) - 0.5 * width(axis) + (volume(axis) + 0.5) * 0.1;
        }
        for (int unknown = 0; unknown < NumberOfData; unknown++) {
          VALIDATE_NUMERICAL_EQUALS(valueAtThePosition[unknown], getLinearField(unknown, volumeCentre), unknown);
        }

        tarch::la::Vector<DIMENSIONS, double> lastVolumeCentre = volumeCentre;
        lastVolumeCentre(0)                                    = centre(0) - 0.5 * width(0) + 2.5 * 0.1;
        VALIDATE_NUMERICAL_EQUALS(valueBeyondThePatch, getLinearField(0, lastVolumeCentre));
      }
    }
  }
}

UNIT_TEST("The piecewise-linear interpolation reproduces a linear field and keeps the last value beyond the centres") {
  GIVEN("A patch of width 0.3 around 0.5 with the linear field and positions near the middle and the last centre") {
    const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> width(0.3);
    const auto                                  patch = getLinearPatch(centre, width);

    tarch::la::Vector<DIMENSIONS, double> position(0.5);
    position(0) = 0.53;
    tarch::la::Vector<DIMENSIONS, double> belowTheCentre(0.5);
    belowTheCentre(0) = 0.47;
    tarch::la::Vector<DIMENSIONS, double> beyondTheLastCentre(0.5);
    beyondTheLastCentre(0) = 0.5 + 0.15 - 0.01;
    tarch::la::Vector<DIMENSIONS, double> lastCentre(0.5);
    lastCentre(0) = 0.5 + 0.15 - 0.05;

    WHEN("The tracer reads every unknown above the middle centre and the first one below it and past the last centre") {
      std::vector<double> valueAtThePosition;
      for (int unknown = 0; unknown < NumberOfData; unknown++) {
        valueAtThePosition.push_back(
          exahype2::fv::interpolatePiecewiseLinear<PatchSize, NumberOfData>(
            centre,
            width,
            patch.data(),
            position,
            unknown
          )
        );
      }
      const double valueBelowTheCentre = exahype2::fv::interpolatePiecewiseLinear<PatchSize, NumberOfData>(
        centre,
        width,
        patch.data(),
        belowTheCentre,
        0
      );
      const double valueBeyondTheLastCentre = exahype2::fv::interpolatePiecewiseLinear<PatchSize, NumberOfData>(
        centre,
        width,
        patch.data(),
        beyondTheLastCentre,
        0
      );

      THEN("Both sides of the middle centre match the field and past the last centre the value is the field there") {
        for (int unknown = 0; unknown < NumberOfData; unknown++) {
          VALIDATE_NUMERICAL_EQUALS(valueAtThePosition[unknown], getLinearField(unknown, position), unknown);
        }
        VALIDATE_NUMERICAL_EQUALS(valueBelowTheCentre, getLinearField(0, belowTheCentre));
        VALIDATE_NUMERICAL_EQUALS(valueBeyondTheLastCentre, getLinearField(0, lastCentre));
      }
    }
  }
}

UNIT_TEST(
  "The piecewise-linear interpolation of a mirrored or swapped patch at the mirrored or swapped position keeps the bits"
) {
  GIVEN("A patch of distinct values, its mirror along the first axis and its copy with the first two axes swapped") {
    const tarch::la::Vector<DIMENSIONS, double> centre(0.0);
    const tarch::la::Vector<DIMENSIONS, double> width(0.3);
    std::vector<double>                         original(ValuesPerPatch, 0.0);
    std::vector<double>                         mirrored(ValuesPerPatch, 0.0);
    std::vector<double>                         swapped(ValuesPerPatch, 0.0);
    peano4::dForEach(PatchSize, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
      tarch::la::Vector<DIMENSIONS, int> mirroredVolume = volume;
      mirroredVolume(0)                                 = PatchSize - 1 - volume(0);
      tarch::la::Vector<DIMENSIONS, int> swappedVolume  = volume;
      swappedVolume(0)                                  = volume(1);
      swappedVolume(1)                                  = volume(0);
      for (int unknown = 0; unknown < NumberOfData; unknown++) {
        const double value = std::sin(1.0 + 0.7 * peano4::dLinearised(volume, PatchSize) + 0.3 * unknown);
        original[static_cast<std::size_t>(peano4::dLinearised(volume, PatchSize)) * NumberOfData + unknown] = value;
        mirrored[static_cast<std::size_t>(peano4::dLinearised(mirroredVolume, PatchSize)) * NumberOfData + unknown] =
          value;
        swapped[static_cast<std::size_t>(peano4::dLinearised(swappedVolume, PatchSize)) * NumberOfData + unknown] =
          value;
      }
    });
    const auto interpolate = [&centre, &width](
                               const std::vector<double>&                   patch,
                               const tarch::la::Vector<DIMENSIONS, double>& position,
                               const int                                    unknown
                             ) {
      return exahype2::fv::interpolatePiecewiseLinear<PatchSize, NumberOfData>(
        centre,
        width,
        patch.data(),
        position,
        unknown
      );
    };

    WHEN("The interpolation runs at 49 positions in the patch and at their mirrors and swaps in the other patches") {
      static constexpr double Samples[] = {-0.13, -0.071, -0.02, 0.0, 0.033, 0.09, 0.149};
      std::vector<double>     values;
      std::vector<double>     mirroredValues;
      std::vector<double>     swappedValues;
      for (const double first : Samples) {
        for (const double second : Samples) {
          tarch::la::Vector<DIMENSIONS, double> position(0.01);
          position(0)                                            = first;
          position(1)                                            = second;
          tarch::la::Vector<DIMENSIONS, double> mirroredPosition = position;
          mirroredPosition(0)                                    = -first;
          tarch::la::Vector<DIMENSIONS, double> swappedPosition  = position;
          swappedPosition(0)                                     = second;
          swappedPosition(1)                                     = first;
          for (int unknown = 0; unknown < NumberOfData; unknown++) {
            values.push_back(interpolate(original, position, unknown));
            mirroredValues.push_back(interpolate(mirrored, mirroredPosition, unknown));
            swappedValues.push_back(interpolate(swapped, swappedPosition, unknown));
          }
        }
      }

      THEN("The mirrored and the swapped patch return the bits of the patch at every position and every unknown") {
        for (std::size_t sample = 0; sample < values.size(); sample++) {
          VALIDATE_EQUALS(mirroredValues[sample], values[sample], sample);
          VALIDATE_EQUALS(swappedValues[sample], values[sample], sample);
        }
      }
    }
  }
}

UNIT_TEST("Every scheme interpolates a patch of one volume into the particle and keeps the particle position") {
  GIVEN("A patch of one volume of width one around 0.5 that stores the float values 3 and -2") {
    const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> width(1.0);
    const float                                 values[2]{3.0F, -2.0F};

    WHEN("Every scheme writes the second and the first value into particles that store 42 and 43 at five positions") {
      std::vector<double> firstData;
      std::vector<double> secondData;
      std::vector<double> expectedFirstData;
      std::vector<double> expectedSecondData;
      int                 movedCoordinates = 0;
      for (const auto scheme :
           {exahype2::particles::InterpolationScheme::None,
            exahype2::particles::InterpolationScheme::PiecewiseConstant,
            exahype2::particles::InterpolationScheme::PiecewiseLinear}) {
        for (const double coordinate : {-0.5, 0.0, 0.5, 1.0, 1.5}) {
          double                                      particleValues[DIMENSIONS + 2]{};
          exahype2::particles::Particle               particle{particleValues, 2};
          const tarch::la::Vector<DIMENSIONS, double> position(coordinate);
          particle.setX(position);
          particle.setData(0, 42.0);
          particle.setData(1, 43.0);
          exahype2::fv::interpolateFields<1, 2, float, 1, 0>(scheme, centre, width, values, position, particle);
          const bool samples = scheme != exahype2::particles::InterpolationScheme::None;
          firstData.push_back(particle.getData(0));
          secondData.push_back(particle.getData(1));
          expectedFirstData.push_back(samples ? -2.0 : 42.0);
          expectedSecondData.push_back(samples ? 3.0 : 43.0);
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            movedCoordinates += particle.getX(axis) == coordinate ? 0 : 1;
          }
        }
      }

      THEN("The sampling schemes write both values, None keeps 42 and 43 and every particle keeps its position") {
        for (std::size_t sample = 0; sample < firstData.size(); sample++) {
          VALIDATE_EQUALS(firstData[sample], expectedFirstData[sample], sample);
          VALIDATE_EQUALS(secondData[sample], expectedSecondData[sample], sample);
        }
        VALIDATE_EQUALS(movedCoordinates, 0);
      }
    }
  }
}

UNIT_TEST("Both interpolations use the spacing of each axis of a rectangular patch") {
  GIVEN("A rectangular patch around 0.5 with the linear field, a position in its last volume and the volume centre") {
    const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
    tarch::la::Vector<DIMENSIONS, double>       width;
    tarch::la::Vector<DIMENSIONS, double>       position;
    tarch::la::Vector<DIMENSIONS, double>       volumeCentre;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      width(axis)        = 0.3 * (axis + 1);
      position(axis)     = centre(axis) + 0.2 * width(axis);
      volumeCentre(axis) = centre(axis) + width(axis) / 3.0;
    }
    const auto patch = getLinearPatch(centre, width);

    WHEN("Both interpolations read every unknown at the position") {
      std::vector<double> piecewiseConstantValue;
      std::vector<double> piecewiseLinearValue;
      for (int unknown = 0; unknown < NumberOfData; unknown++) {
        piecewiseConstantValue.push_back(
          exahype2::fv::interpolatePiecewiseConstant<PatchSize, NumberOfData>(
            centre,
            width,
            patch.data(),
            position,
            unknown
          )
        );
        piecewiseLinearValue.push_back(
          exahype2::fv::interpolatePiecewiseLinear<PatchSize, NumberOfData>(
            centre,
            width,
            patch.data(),
            position,
            unknown
          )
        );
      }

      THEN("The piecewise-constant values match the field at the volume centre and the linear ones at the position") {
        for (int unknown = 0; unknown < NumberOfData; unknown++) {
          VALIDATE_NUMERICAL_EQUALS(piecewiseConstantValue[unknown], getLinearField(unknown, volumeCentre));
          VALIDATE_NUMERICAL_EQUALS(piecewiseLinearValue[unknown], getLinearField(unknown, position));
        }
      }
    }
  }
}

UNIT_TEST("Clamping keeps a position between the volume centres and moves one beyond them onto the last centre") {
  GIVEN("A patch of width 0.3 around 0.5, a position inside it and one beyond it along the first axis") {
    const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> width(0.3);
    const tarch::la::Vector<DIMENSIONS, double> inside(0.4);
    tarch::la::Vector<DIMENSIONS, double>       outside(0.5);
    outside(0) = 0.9;

    WHEN("The tracer clamps both positions into the patch") {
      const auto clampedInside  = exahype2::fv::clampIntoPatch<PatchSize>(centre, width, inside);
      const auto clampedOutside = exahype2::fv::clampIntoPatch<PatchSize>(centre, width, outside);

      THEN("The inside position stays and the other one moves onto the last centre along the first axis only") {
        VALIDATE_NUMERICAL_EQUALS(clampedInside(0), 0.4);
        VALIDATE_NUMERICAL_EQUALS(clampedOutside(0), 0.5 + 0.15 - 0.05);
        VALIDATE_NUMERICAL_EQUALS(clampedOutside(DIMENSIONS - 1), 0.5);
      }
    }
  }
}

UNIT_TEST("One explicit Euler step moves a tracer by the velocity times the step size and keeps it in the domain") {
  GIVEN("A grid of fluid velocity one, the step one half, the periodic first axis and tracers at 0.1, 0.9 and 0.9") {
    constexpr double Velocity          = 1.0;
    constexpr double TimeStepSize      = 0.5;
    constexpr int    FirstAxisPeriodic = 1;
    const Grid       grid(Velocity);
    Set              particles;
    Set              wrapped;
    Set              reflected;
    particles.insert(grid.cellConnectivity, getTracerAt(0.1));
    wrapped.insert(grid.cellConnectivity, getTracerAt(0.9));
    reflected.insert(grid.cellConnectivity, getTracerAt(0.9));
    const int insertedParticles = peano4::unittests::getGlobalSumOf(particles.getSize());

    WHEN("One step traces the tracers, one of those at 0.9 through the periodic first axis") {
      exahype2::fv::traceParticles<
        PatchSize,
        NumberOfData,
        NumberOfParticleData,
        exahype2::particles::ParticleVelocityAttribute,
        exahype2::particles::ParticleVelocityAttribute + DIMENSIONS - 1
      >(grid.cellConnectivity, grid.solution, TimeStepSize, particles, getFluidTracer(), DomainOffset, DomainWidth, 0);
      const int tracedParticles = peano4::unittests::getGlobalSumOf(particles.getSize());
      particles.download();
      exahype2::fv::traceParticles<
        PatchSize,
        NumberOfData,
        NumberOfParticleData,
        exahype2::particles::ParticleVelocityAttribute,
        exahype2::particles::ParticleVelocityAttribute + DIMENSIONS - 1
      >(grid.cellConnectivity,
        grid.solution,
        TimeStepSize,
        wrapped,
        getFluidTracer(),
        DomainOffset,
        DomainWidth,
        FirstAxisPeriodic);
      const int wrappedParticles = peano4::unittests::getGlobalSumOf(wrapped.getSize());
      wrapped.download();
      exahype2::fv::traceParticles<
        PatchSize,
        NumberOfData,
        NumberOfParticleData,
        exahype2::particles::ParticleVelocityAttribute,
        exahype2::particles::ParticleVelocityAttribute + DIMENSIONS - 1
      >(grid.cellConnectivity, grid.solution, TimeStepSize, reflected, getFluidTracer(), DomainOffset, DomainWidth, 0);
      const int reflectedParticles = peano4::unittests::getGlobalSumOf(reflected.getSize());
      reflected.download();

      THEN("Every tracer stays in its set, moves with the fluid, wraps or reflects and samples the fluid velocity") {
        VALIDATE_EQUALS(insertedParticles, 1);
        VALIDATE_EQUALS(tracedParticles, 1);
        VALIDATE_EQUALS(wrappedParticles, 1);
        VALIDATE_EQUALS(reflectedParticles, 1);
        for (int index = 0; index < particles.getSize(); index++) {
          VALIDATE_NUMERICAL_EQUALS(particles.getPosition(index)(0), 0.1 + Velocity * TimeStepSize);
          VALIDATE_NUMERICAL_EQUALS(particles.getPosition(index)(DIMENSIONS - 1), 0.5);
          VALIDATE_NUMERICAL_EQUALS(particles.getData(index)[exahype2::particles::ParticleVelocityAttribute], Velocity);
          VALIDATE_EQUALS(
            particles.getSlot(index),
            Set::getSlotAt(grid.cellConnectivity, particles.getPosition(index))
          );
        }
        for (int index = 0; index < wrapped.getSize(); index++) {
          VALIDATE_NUMERICAL_EQUALS(wrapped.getPosition(index)(0), 0.9 + Velocity * TimeStepSize - 1.0);
        }
        for (int index = 0; index < reflected.getSize(); index++) {
          VALIDATE_NUMERICAL_EQUALS(reflected.getPosition(index)(0), 1.0 - (0.9 + Velocity * TimeStepSize - 1.0));
          VALIDATE_NUMERICAL_EQUALS(reflected.getData(index)[exahype2::particles::ParticleVelocityAttribute], Velocity);
        }
      }
    }
  }
}

UNIT_TEST("Static sampling writes only the named attributes, also in a set without motion attributes") {
  GIVEN("A grid of fluid velocity two and a particle with attributes 42 in a set of one attribute and in a full set") {
    const Grid                          grid(2.0);
    exahype2::particles::ParticleSet<1> small;
    Set                                 full;
    insertParticleWithAttributes42(grid, small);
    insertParticleWithAttributes42(grid, full);

    WHEN("The static sampling interpolates the fluid into the first attribute of both sets") {
      sampleIntoTheFirstAttribute(grid, small);
      sampleIntoTheFirstAttribute(grid, full);
      const int smallParticles = peano4::unittests::getGlobalSumOf(small.getSize());
      const int fullParticles  = peano4::unittests::getGlobalSumOf(full.getSize());

      THEN("Both sets contain one particle with velocity two in its first attribute and every other value unchanged") {
        VALIDATE_EQUALS(smallParticles, 1);
        validateSampledParticles(small);
        VALIDATE_EQUALS(fullParticles, 1);
        validateSampledParticles(full);
      }
    }
  }
}
