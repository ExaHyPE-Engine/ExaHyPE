# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .scenario import Scenario

import os, sys
import jinja2

sys.path.insert(0, os.path.abspath("../equations"))
from equations import SWE_W_Bathymetry


class SWERestingLake(Scenario):
    """
    Resting lake scenario for the shallow water equations.

    The free surface h + b of the water column h and the bathymetry b is
    constant over the domain. The exact solution stays at rest. The scheme
    evaluates the sum h' + b' of the derivatives. The rounding errors of this
    sum start spurious waves. The scenario measures the size of these waves
    and whether the scheme damps them.
    """

    _offset = -0.5
    _domain_size = 1.0
    _periodic_bc = True
    _dimensions = 2
    _equation = SWE_W_Bathymetry()
    _end_time = 1.0

    def __init__(
        self,
        initial_conditions="sinusoidal",
    ):
        self._scenario = initial_conditions

    def _initial_conditions(self, Q):
        d = {"VARIABLE": Q}
        return {
            "bilinear": jinja2.Template("""
    {{VARIABLE}}[Shortcuts::hv+0]  = 0;
    {{VARIABLE}}[Shortcuts::hv+1]  = 0;
    {{VARIABLE}}[Shortcuts::b]    = 1 - std::abs(x[0]) - std::abs(x[Shortcuts::hv+0]);
    {{VARIABLE}}[Shortcuts::h]    = 2 - {{VARIABLE}}[Shortcuts::b];
""").render(**d),
            "sinusoidal": jinja2.Template("""
    {{VARIABLE}}[Shortcuts::hv+0]  = 0;
    {{VARIABLE}}[Shortcuts::hv+1]  = 0;
    {{VARIABLE}}[Shortcuts::b]    = sin( 2*M_PI * (x[0]+x[Shortcuts::hv+0]) );
    {{VARIABLE}}[Shortcuts::h]    = 2 - {{VARIABLE}}[Shortcuts::b];
""").render(**d),
            "constant": jinja2.Template("""
    {{VARIABLE}}[Shortcuts::hv+0]  = 0;
    {{VARIABLE}}[Shortcuts::hv+1]  = 0;
    {{VARIABLE}}[Shortcuts::b]    = 1;
    {{VARIABLE}}[Shortcuts::h]    = 2 - {{VARIABLE}}[Shortcuts::b];
""").render(**d),
        }

    def initial_conditions(self):
        return self._initial_conditions("Q")[self._scenario]

    def analytical_solution(self):
        return self._initial_conditions("solution")[self._scenario]
