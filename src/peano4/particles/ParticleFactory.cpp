// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParticleFactory.h"

/**
 * Draws a thread-local coordinate perturbation from [-0.5, 0.5).
 */
double peano4::particles::drawParticlePositionNoise() {
  thread_local std::mt19937                           randomEngine(std::random_device{}());
  thread_local std::uniform_real_distribution<double> distribution(0.0, 1.0);
  return distribution(randomEngine) - 0.5;
}

namespace {
  struct StubParticle final {
    enum class ParallelState { Local, Virtual };

#if PEANO_DEBUG
    void setDebugX(const tarch::la::Vector<DIMENSIONS, double>& position) { _debugPosition = position; }

    void setDebugH(const double cellSize) { _debugCellSize = cellSize; }
#endif

    void setX(const tarch::la::Vector<DIMENSIONS, double>& position) { _position = position; }

    void setSearchRadius(const double searchRadius) { _searchRadius = searchRadius; }

    const std::array<double, 1>& getData() const { return _data; }

    void setData(const int index, const double value) { _data[index] = value; }

    void setParallelState(const ParallelState parallelState) { _parallelState = parallelState; }

  private:
#if PEANO_DEBUG
    tarch::la::Vector<DIMENSIONS, double> _debugPosition{0.0};
    double                                _debugCellSize{0.0};
#endif
    tarch::la::Vector<DIMENSIONS, double> _position{0.0};
    double                                _searchRadius{0.0};
    std::array<double, 1>                 _data{};
    ParallelState                         _parallelState{ParallelState::Virtual};
  };

  // Taking these addresses compiles every particle-factory template in the core library.
  [[maybe_unused]] void instantiateParticleFactoryTemplates() {
    using Initialise = void (*)(
      StubParticle&                                particle,
      const tarch::la::Vector<DIMENSIONS, double>& position,
      const double                                 searchRadius
    );
    using InitialiseWithData = void (*)(
      StubParticle&                                particle,
      const tarch::la::Vector<DIMENSIONS, double>& position,
      const double                                 searchRadius,
      const std::vector<double>&                   initialData
    );
    using CreateCellLocal = std::vector<StubParticle*> (*)(
      const double                                 spacing,
      const tarch::la::Vector<DIMENSIONS, double>& voxelCentre,
      const tarch::la::Vector<DIMENSIONS, double>& voxelSize,
      const bool                                   addNoise,
      const double                                 noiseScale
    );
    using CreateDomainAligned = std::vector<StubParticle*> (*)(
      const double                                 spacing,
      const tarch::la::Vector<DIMENSIONS, double>& voxelCentre,
      const tarch::la::Vector<DIMENSIONS, double>& voxelSize,
      const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
      const tarch::la::Vector<DIMENSIONS, double>& domainSize,
      const bool                                   addNoise,
      const double                                 noiseScale
    );

    [[maybe_unused]] const Initialise         initialise         = &peano4::particles::init<StubParticle>;
    [[maybe_unused]] const InitialiseWithData initialiseWithData = &peano4::particles::initWithData<StubParticle>;
    [[maybe_unused]] const CreateCellLocal    createCellLocal =
      &peano4::particles::createEquallySpacedParticles<StubParticle>;
    [[maybe_unused]] const CreateDomainAligned createDomainAligned =
      &peano4::particles::createParticlesAlignedWithGlobalCartesianMesh<StubParticle>;
  }
} // namespace
