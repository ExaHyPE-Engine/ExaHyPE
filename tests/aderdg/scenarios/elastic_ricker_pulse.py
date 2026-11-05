# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .scenario import Scenario

import os
import sys

sys.path.insert(0, os.path.abspath("../equations"))
from equations import Elastic


class ElasticRickerPulse(Scenario):
    """
    Ricker pulse after Käser, Herrmann & de la Puente, https://doi.org/10.1111/j.1365-246X.2008.03781.x,
    section 2.2, p. 992.

    The quadrature measures the error of the analytical solution inaccurately when the signal lies
    away from a cell centre. A time-frequency misfit of a tracer record, for example with ObsPy
    (https://docs.obspy.org/tutorial/code_snippets/time_frequency_misfit.html), measures it accurately.
    """

    _offset = 0.0
    _domain_size = 30.0
    _periodic_bc = True
    _dimensions = 2

    def __init__(self, iterations=1):
        self._end_time = self._domain_size * iterations
        self._equation = Elastic(dimensions=2)

    def initial_conditions(self):
        return """
  constexpr auto ShearModulus       = 1;
  constexpr auto FirstLameParameter = 2;

  constexpr auto Density    = 1;
  constexpr auto PWaveSpeed = 2;
  constexpr auto SWaveSpeed = 1;

  // The pulse is centred in the leftmost cell. It reaches the left edge of the
  // domain. The periodic boundary places part of it on the right edge.
  const auto tau  = 1.25*std::numbers::pi*(x[0]-h[0]/2);
  const auto tau2 = 1.25*std::numbers::pi*(x[0]-DomainSize[0]-h[0]/2);
  const auto amplitudeX = (1-2*tau*tau)*std::exp(-tau*tau) + (1-2*tau2*tau2)*std::exp(-tau2*tau2);
  constexpr auto AmplitudeY = 0.;

  Q.sigma(0, 0) = -amplitudeX*(FirstLameParameter+2*ShearModulus) + AmplitudeY*FirstLameParameter;
  Q.sigma(1, 1) = -amplitudeX*FirstLameParameter + AmplitudeY*(FirstLameParameter+2*ShearModulus);
  Q.sigma(0, 1) = 0;

  Q.v(0) = amplitudeX*PWaveSpeed;
  Q.v(1) = AmplitudeY*PWaveSpeed;

  Q.rho = Density;
  Q.cp  = PWaveSpeed;
  Q.cs  = SWaveSpeed;
"""

    def analytical_solution(self):
        return """
  constexpr auto PWaveSpeed = 2;

  // The loop divides h down to the width of a cell on the finest level. The pulse starts at the
  // centre of the leftmost cell.
  double h = DomainSize[0];
  for (int level = 0; level < parameters.maxGridLevels; level++) {
    h /= peano4::SubdivisionFactor;
  }

  // The P-wave moves with speed PWaveSpeed. The pulse lies in the periodic image closest to x.
  auto distance = x[0] - PWaveSpeed*t - h/2;
  distance -= DomainSize[0] * std::round(distance / DomainSize[0]);
  const auto tau  = 1.25*std::numbers::pi*(distance);
  const auto amplitudeX = (1-2*tau*tau)*std::exp(-tau*tau);

  solution.sigma = Q.sigma;
  solution.v(0)  = amplitudeX*PWaveSpeed;
  solution.v(1)  = Q.v(1);
"""
