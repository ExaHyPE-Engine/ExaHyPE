// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParticleUpdate.h"

namespace {
  struct StubParticle final {
    tarch::la::Vector<DIMENSIONS, double> getX() const { return _position; }

    void setX(const tarch::la::Vector<DIMENSIONS, double>& position) { _position = position; }

    double getData(const int index) const { return _data[index]; }

    void setData(const int index, const double value) { _data[index] = value; }

  private:
    tarch::la::Vector<DIMENSIONS, double> _position{0.0};
    double                                _data[16]{};
  };

  // Taking this address makes the core library compile the particle update.
  [[maybe_unused]] void instantiateParticleUpdateTemplates() {
    using Update = void (*)(
      StubParticle&                             particle,
      const peano4::datamanagement::CellMarker& cellMarker,
      const double* const                       cellValues,
      const double                              timeStepSize,
      const int                                 integrationMethod
    );
    [[maybe_unused]] Update update = &exahype2::fv::particles::updateParticleState<0, 1>;
  }
} // namespace
