// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParticleIntegration.h"

/**
 * Time step size used by multi-particle integration.
 */
double exahype2::fv::particles::integration::_timeStepSize = 0.01;

/**
 * Integration-method selector used by multi-particle integration.
 */
int exahype2::fv::particles::integration::_integrationMethod = 1;

/**
 * Sets the process-wide settings read by multi-particle integration.
 */
void exahype2::fv::particles::integration::setGlobalIntegrationParameters(
  const double timeStepSize,
  const int    integrationMethod
) {
  _timeStepSize      = timeStepSize;
  _integrationMethod = integrationMethod;
}

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

  // Taking these addresses makes the core library compile every integration method.
  [[maybe_unused]] void instantiateParticleIntegrationTemplates() {
    using EulerIntegration =
      tarch::la::Vector<DIMENSIONS, double> (*)(StubParticle& particle, const double timeStepSize);
    using Integration = tarch::la::Vector<DIMENSIONS, double> (*)(
      StubParticle&                             particle,
      const peano4::datamanagement::CellMarker& cellMarker,
      const double* const                       cellValues,
      const double                              timeStepSize
    );
    using IntegrationDispatch = tarch::la::Vector<DIMENSIONS, double> (*)(
      StubParticle&                             particle,
      const peano4::datamanagement::CellMarker& cellMarker,
      const double* const                       cellValues,
      const double                              timeStepSize,
      const int                                 integrationMethod
    );

    [[maybe_unused]] EulerIntegration explicitEuler =
      &exahype2::fv::particles::integration::explicitEulerIntegration<StubParticle>;
    [[maybe_unused]] EulerIntegration semiImplicitEuler =
      &exahype2::fv::particles::integration::semiImplicitEulerIntegration<StubParticle>;
    [[maybe_unused]] Integration midpoint = &exahype2::fv::particles::integration::midpointIntegration<0, 1>;
    [[maybe_unused]] Integration rk3      = &exahype2::fv::particles::integration::rk3Integration<0, 1>;
    [[maybe_unused]] Integration rk4      = &exahype2::fv::particles::integration::rk4Integration<0, 1>;
    [[maybe_unused]] Integration velocityVerlet =
      &exahype2::fv::particles::integration::velocityVerletIntegration<0, 1>;
    [[maybe_unused]] IntegrationDispatch apply = &exahype2::fv::particles::integration::applyIntegration<0, 1>;
  }
} // namespace
