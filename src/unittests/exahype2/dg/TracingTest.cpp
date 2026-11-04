// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/dg/Tracing.h"

#include "exahype2/dg/Basis.h"
#include "exahype2/particles/Forces.h"
#include "exahype2/particles/ParticleSet.h"
#include "peano4/CellConnectivity.h"
#include "peano4/Loop.h"
#include "peano4/Spacetree.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"
#include "tarch/la/Vector.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/peano4/TestGrids.h"

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

  GPU_CALLABLE_METHOD double getLinearField(const int unknown, const tarch::la::Vector<DIMENSIONS, double>& x) {
    double value = unknown + 1.0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      value += (axis + 2.0) * x(axis);
    }
    return value;
  }

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
  struct FillNodeVelocity final {
    peano4::CellConnectivity cellConnectivity{};
    CellPool::View           solution{};
    double                   velocity{0.0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int entry) const {
      double* const Q = solution.getChunk(cellConnectivity.getCells()[entry]);
      for (int node = 0; node < NodesPerCell; node++) {
        for (int unknown = 0; unknown < NumberOfData; unknown++) {
          Q[node * NumberOfData + unknown] = unknown == 0 ? velocity : 0.0;
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
   * A regular grid with a solution of constant velocity along the first axis.
   */
  struct Grid final {
    const peano4::CellConnectivity&    cellConnectivity;
    CellPool                           solution{ValuesPerCell};
    tarch::accelerator::Buffer<double> quadratureNodes{tarch::MemoryLocation::Device};

    explicit Grid(const double velocity) :
      cellConnectivity(buildGrid()) {
      solution.reserve(cellConnectivity.getNumberOfSlots());
      quadratureNodes.setValues(std::vector<double>(QuadratureNodes1D, QuadratureNodes1D + NodesPerAxis));
      tarch::accelerator::parallelFor(
        tarch::accelerator::Range(cellConnectivity.getCells().size),
        FillNodeVelocity{cellConnectivity.onDevice(), solution.getView(), velocity}
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
    peano4::CellConnectivity cellConnectivity{};
    FacePool::View           faceStates{};
    const double*            quadratureNodes{nullptr};
    double                   upperShift{0.0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int face) const {
      const int     normal = cellConnectivity.getFaceNormal(face);
      double* const lower  = faceStates.getChunk(face);
      double* const upper  = lower + static_cast<std::ptrdiff_t>(NodesPerFace) * NumberOfData;
      for (int node = 0; node < NodesPerFace; node++) {
        const auto                            nodeIndex = peano4::dDelinearisedExclusive(node, NodesPerAxis, normal);
        tarch::la::Vector<DIMENSIONS, double> position;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          position(axis) = cellConnectivity.getFaceCentre(face, axis);
          if (axis != normal) {
            position(axis) += (quadratureNodes[nodeIndex(axis)] - 0.5) * cellConnectivity.getFaceWidth(face, axis);
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

  void fillFaceStates(const Grid& grid, FacePool& faceStates, const double upperShift) {
    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(grid.cellConnectivity.getNumberOfFaces()),
      FillFaceStates{grid.cellConnectivity.onDevice(), faceStates.getView(), grid.quadratureNodes.getData(), upperShift}
    );
  }

  /**
   * Returns the forces of a tracer that moves with the fluid velocity.
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
   * Inserts into the set one particle at 0.1 along every axis whose attributes all equal 42.
   */
  template <typename SampleSet>
  void insertParticleWithAttributes42(const Grid& grid, SampleSet& particles) {
    std::vector<double> values(SampleSet::ValuesPerParticle, 42.0);
    std::fill_n(values.begin(), DIMENSIONS, 0.1);
    particles.insert(grid.cellConnectivity, values);
  }

  /**
   * Samples the first unknown of the fluid into the first attribute of every particle of the set.
   * The particles keep their positions.
   */
  template <typename SampleSet>
  void sampleIntoTheFirstAttribute(const Grid& grid, SampleSet& particles) {
    constexpr int               Attributes = SampleSet::ValuesPerParticle - DIMENSIONS;
    exahype2::particles::Forces forces;
    forces.interpolationScheme = exahype2::particles::InterpolationScheme::PiecewiseLinear;
    exahype2::dg::traceParticles<NodesPerAxis, NumberOfData, Attributes, 0>(
      grid.cellConnectivity,
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

UNIT_TEST("The interpolation writes unknown k into the k-th named entry and leaves the other entries unchanged") {
  GIVEN("A cell of the linear field around 0.5 of width 0.4, the position 0.37 and a particle with zero entries") {
    const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> width(0.4);
    const tarch::la::Vector<DIMENSIONS, double> position(0.37);
    const std::vector<double>                   Q = getLinearCell(centre, width);

    double                        values[Set::ValuesPerParticle] = {};
    exahype2::particles::Particle particle;
    particle.values          = values;
    particle.numberOfData    = NumberOfParticleData;
    const double* const data = values + DIMENSIONS;

    WHEN("The piecewise linear interpolation writes the unknowns zero and one into the entries three and one") {
      exahype2::dg::interpolateFields<NodesPerAxis, NumberOfData, double, 3, 1>(
        exahype2::particles::InterpolationScheme::PiecewiseLinear,
        centre,
        width,
        QuadratureNodes1D,
        Q.data(),
        position,
        particle
      );

      THEN("The entries three and one store their unknown at the position and the entries zero and two stay zero") {
        VALIDATE_NUMERICAL_EQUALS(data[3], getLinearField(0, position));
        VALIDATE_NUMERICAL_EQUALS(data[1], getLinearField(1, position));
        VALIDATE_NUMERICAL_EQUALS(data[0], 0.0);
        VALIDATE_NUMERICAL_EQUALS(data[2], 0.0);
      }
    }

    WHEN("An interpolation without scheme names the entry zero") {
      exahype2::dg::interpolateFields<NodesPerAxis, NumberOfData, double, 0>(
        exahype2::particles::InterpolationScheme::None,
        centre,
        width,
        QuadratureNodes1D,
        Q.data(),
        position,
        particle
      );

      THEN("The entry zero stays zero") { VALIDATE_NUMERICAL_EQUALS(data[0], 0.0); }
    }
  }
}

UNIT_TEST("A tracer takes and follows the fluid velocity, wraps through a periodic wall and reflects at a solid one") {
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
      exahype2::dg::traceParticles<
        NodesPerAxis,
        NumberOfData,
        NumberOfParticleData,
        exahype2::particles::ParticleVelocityAttribute,
        exahype2::particles::ParticleVelocityAttribute + DIMENSIONS - 1
      >(grid.cellConnectivity,
        grid.solution,
        grid.quadratureNodes.getData(),
        TimeStepSize,
        particles,
        getFluidTracer(),
        DomainOffset,
        DomainWidth,
        0);
      const int tracedParticles = peano4::unittests::getGlobalSumOf(particles.getSize());
      exahype2::dg::traceParticles<
        NodesPerAxis,
        NumberOfData,
        NumberOfParticleData,
        exahype2::particles::ParticleVelocityAttribute,
        exahype2::particles::ParticleVelocityAttribute + DIMENSIONS - 1
      >(grid.cellConnectivity,
        grid.solution,
        grid.quadratureNodes.getData(),
        TimeStepSize,
        wrapped,
        getFluidTracer(),
        DomainOffset,
        DomainWidth,
        FirstAxisPeriodic);
      const int wrappedParticles = peano4::unittests::getGlobalSumOf(wrapped.getSize());
      exahype2::dg::traceParticles<
        NodesPerAxis,
        NumberOfData,
        NumberOfParticleData,
        exahype2::particles::ParticleVelocityAttribute,
        exahype2::particles::ParticleVelocityAttribute + DIMENSIONS - 1
      >(grid.cellConnectivity,
        grid.solution,
        grid.quadratureNodes.getData(),
        TimeStepSize,
        reflected,
        getFluidTracer(),
        DomainOffset,
        DomainWidth,
        0);
      const int reflectedParticles = peano4::unittests::getGlobalSumOf(reflected.getSize());
      particles.download();
      wrapped.download();
      reflected.download();

      THEN("Every set keeps its tracer and every tracer moves, wraps or reflects and samples the fluid velocity") {
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

UNIT_TEST("Static sampling writes only its named attributes, also into a set without motion attributes") {
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

UNIT_TEST("A particle on a face plane takes the mean of the two traces and, for the named field, their jump") {
  GIVEN(
    "A grid whose face states store the linear field below and the field plus ten above every face and one "
    "particle on and one off the first cell boundary"
  ) {
    constexpr double UpperShift = 10.0;
    constexpr double Plane      = 1.0 / peano4::SubdivisionFactor;
    const Grid       grid(0.0);
    FacePool         faceStates{ValuesPerFace};
    faceStates.reserve(grid.cellConnectivity.getNumberOfFaces());
    fillFaceStates(grid, faceStates, UpperShift);
    Set particles;
    Set away;
    particles.insert(grid.cellConnectivity, getTracerAt(Plane));
    away.insert(grid.cellConnectivity, getTracerAt(0.5 * Plane));
    const int insertedParticles = peano4::unittests::getGlobalSumOf(particles.getSize());

    WHEN("The faces along the first axis trace into both particles") {
      exahype2::dg::traceFaceValues<NodesPerAxis, NumberOfData, NumberOfParticleData, 1>(
        grid.cellConnectivity,
        faceStates,
        grid.quadratureNodes.getData(),
        particles,
        0,
        Plane
      );
      particles.download();
      exahype2::dg::traceFaceValues<NodesPerAxis, NumberOfData, NumberOfParticleData, 1>(
        grid.cellConnectivity,
        faceStates,
        grid.quadratureNodes.getData(),
        away,
        0,
        Plane
      );
      away.download();

      THEN("The particle on the boundary stores the mean and the jump ten and the particle off it stays zero") {
        VALIDATE_EQUALS(insertedParticles, 1);
        for (int index = 0; index < particles.getSize(); index++) {
          const auto position = particles.getPosition(index);
          VALIDATE_NUMERICAL_EQUALS(particles.getData(index)[0], getLinearField(0, position) + 0.5 * UpperShift);
          VALIDATE_NUMERICAL_EQUALS(particles.getData(index)[1], UpperShift);
        }
        for (int index = 0; index < away.getSize(); index++) {
          VALIDATE_NUMERICAL_EQUALS(away.getData(index)[0], 0.0);
          VALIDATE_NUMERICAL_EQUALS(away.getData(index)[1], 0.0);
        }
      }
    }
    faceStates.release();
  }
}
