from Scenario import Scenario

import os
import sys
sys.path.insert(0, os.path.abspath("../Equations"))
from Equations import Acoustic

from math import sqrt

class Acoustic_planar_waves(Scenario):

    _plot_dt     = 0.0
    _offset      = -1.0
    _domain_size = 2.0
    _periodic_bc = True

    def __init__(
        self,
        dimensions,
        iterations=2
    ):
        self._dimensions  = dimensions
        self._end_time    = iterations*sqrt(dimensions)
        self._K0  = 4.0
        self._rho = 1.0
        self._equation    = Acoustic(dimensions, rho=self._rho, K0=self._K0)


    def initial_conditions(self):
        return """
  // simple translation in positive diagonal direction
  const double val = cos( - std::numbers::pi*( 
      x[0] + x[1]
#if Dimensions==3
      + x[2]
#endif
  ));

  Q[1] = val;
  Q[2] = val;
#if Dimensions==3
  Q[3] = val;
#endif

  constexpr double K0  = """ + str(self._K0) + """;
  constexpr double rho = """ + str(self._rho) + """;

  //These are defined by the eigenvector of the plane wave operator
#if Dimensions==3
  constexpr double kr = K0*std::sqrt(rho/(3.0*K0)) + 2*std::sqrt(K0*rho/3.0);
#else
  constexpr double kr = std::sqrt(2*K0*rho);
#endif

  Q[0] = kr*val;
"""

    def analytical_solution(self):
        return """
  constexpr double w = 2*std::sqrt(Dimensions)*M_PI;

  const double val = cos( w*t - std::numbers::pi*( 
      x[0] + x[1]
#if Dimensions==3
    + x[2]
#endif
  ));

  sol[1] = val;
  sol[2] = val;

  constexpr double K0  = """ + str(self._K0) + """;
  constexpr double rho = """ + str(self._rho) + """;

#if Dimensions==3
  sol[3] = val;
  constexpr double kr = K0*std::sqrt(rho/(3.0*K0)) + 2*std::sqrt(K0*rho/3.0);
#else
  constexpr double kr = std::sqrt(2*K0*rho);
#endif

  sol[0] = kr*val;
"""
