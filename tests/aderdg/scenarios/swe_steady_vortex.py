# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .scenario import Scenario

import os, sys
import jinja2

sys.path.insert(0, os.path.abspath("../equations"))
from equations import SWE_W_Bathymetry


class SWESteadyVortex(Scenario):
    """
    Steady state vortex with varying bathymetry, from Clain & Figueiredo
    (https://doi.org/10.1016/j.compfluid.2016.11.013, section 4.4), used
    also in e.g. https://arxiv.org/pdf/2508.20596
    """

    _offset = -3.0
    _domain_size = 6.0
    _periodic_bc = False
    _dimensions = 2
    _equation = SWE_W_Bathymetry()
    _end_time = 1.0

    def _initial_conditions(self, Q):
        d = {"VARIABLE": Q}
        return jinja2.Template("""
    constexpr double inv_g_quarter = 1/(4*9.81);
    const double r = (1-x[0]*x[0]-x[1]*x[1]);

    {{VARIABLE}}.b     = std::exp(r/2)/5;
    {{VARIABLE}}.h     = 1 - inv_g_quarter*std::exp(2*r) - {{VARIABLE}}.b;
    {{VARIABLE}}.hv(0) =  x[1]*std::exp(r)*{{VARIABLE}}.h;
    {{VARIABLE}}.hv(1) = -x[0]*std::exp(r)*{{VARIABLE}}.h;
""").render(**d)

    def initial_conditions(self):
        return self._initial_conditions("Q")

    def boundary_conditions(self):
        return self._initial_conditions("Qoutside")

    def analytical_solution(self):
        return self._initial_conditions("solution")
