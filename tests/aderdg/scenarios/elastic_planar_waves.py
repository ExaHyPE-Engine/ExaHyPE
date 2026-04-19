# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .scenario import Scenario

import os
import sys

sys.path.insert(0, os.path.abspath("../equations"))
from equations import Elastic


class ElasticPlanarWaves(Scenario):
    """
    Scenario reproduced from Dumbser & Käser, https://doi.org/10.1111/j.1365-246X.2006.03120.x (Chapter 5, p. 328)
    """

    _offset = -1.0
    _domain_size = 2.0
    _periodic_bc = True

    def __init__(self, dimensions, iterations=2):
        self._dimensions = dimensions
        self._end_time = iterations
        self._equation = Elastic(dimensions)

    def initial_conditions(self):
        return """
  // Lamé parameters
  constexpr auto mu     =  4;
  constexpr auto lambda = -4;

  // Constant auxiliary parameters
  constexpr auto rho  = 1;
  constexpr auto cp   = 2;
  constexpr auto cs   = 2;

  const double val_x = cos( -std::numbers::pi*x[0]);
  const double val_y = cos( -std::numbers::pi*x[1]);
""" + (
            """
  const double val_z = cos( -std::numbers::pi*x[2]);

  // Stresses
  Q[Shortcuts::sigma+0] = val_x*(lambda+2*mu)  + val_y*lambda        + val_z*lambda;         //xx
  Q[Shortcuts::sigma+1] = val_x*lambda         + val_y*(lambda+2*mu) + val_z*lambda;         //yy
  Q[Shortcuts::sigma+2] = val_x*lambda         + val_y*lambda        + val_z*(lambda+2*mu);  //zz
  Q[Shortcuts::sigma+3] = val_x*mu; //xy
  Q[Shortcuts::sigma+4] = val_z*mu; //xz
  Q[Shortcuts::sigma+5] = val_y*mu; //yz

  // Velocities
  Q[Shortcuts::v+0] = val_x*cp + val_z*cs; //vx
  Q[Shortcuts::v+1] = val_y*cp + val_x*cs; //vy
  Q[Shortcuts::v+2] = val_z*cp + val_y*cs; //vz

  // Auxiliary variables
  Q[Shortcuts::rho] = rho;  //rho
  Q[Shortcuts::cp]  = cp;   //cp
  Q[Shortcuts::cs]  = cs;   //cs
"""
            if self._dimensions == 3
            else """
  // Stresses
  Q[Shortcuts::sigma+0] = val_x*(lambda+2*mu)  + val_y*lambda;         //xx
  Q[Shortcuts::sigma+1] = val_x*lambda         + val_y*(lambda+2*mu);  //yy
  Q[Shortcuts::sigma+2] = val_x*mu + val_y*mu;                         //xy

  // Velocities
  Q[Shortcuts::v+0] = val_x*cp + val_y*cs; //vx
  Q[Shortcuts::v+1] = val_y*cp + val_x*cs; //vy

  // Auxiliary variables
  Q[Shortcuts::rho] = rho;  //rho
  Q[Shortcuts::cp]  = cp;   //cp
  Q[Shortcuts::cs]  = cs;   //cs
"""
        )

    def analytical_solution(self):
        return """
  // Lamé parameters
  constexpr auto mu     =  4;
  constexpr auto lambda = -4;

  // Constant auxiliary parameters
  constexpr auto rho  = 1;
  constexpr auto cp   = 2;
  constexpr auto cs   = 2;

  constexpr double w = -2*M_PI;

  const double val_x = cos( w*t -std::numbers::pi*x[0]);
  const double val_y = cos( w*t -std::numbers::pi*x[1]);
""" + (
            """
  const double val_z = cos( -std::numbers::pi*x[2]);

  // Stresses
  solution[Shortcuts::sigma+0] = val_x*(lambda+2*mu)  + val_y*lambda        + val_z*lambda;         //xx
  solution[Shortcuts::sigma+1] = val_x*lambda         + val_y*(lambda+2*mu) + val_z*lambda;         //yy
  solution[Shortcuts::sigma+2] = val_x*lambda         + val_y*lambda        + val_z*(lambda+2*mu);  //zz
  solution[Shortcuts::sigma+3] = val_x*mu; //xy
  solution[Shortcuts::sigma+4] = val_z*mu; //xz
  solution[Shortcuts::sigma+5] = val_y*mu; //yz

  // Velocities
  solution[Shortcuts::v+0] = val_x*cp + val_z*cs; //vx
  solution[Shortcuts::v+1] = val_y*cp + val_x*cs; //vy
  solution[Shortcuts::v+2] = val_z*cp + val_y*cs; //vz
"""
            if self._dimensions == 3
            else """
  // Stresses
  solution[Shortcuts::sigma+0] = val_x*(lambda+2*mu)  + val_y*lambda;         //xx
  solution[Shortcuts::sigma+1] = val_x*lambda         + val_y*(lambda+2*mu);  //yy
  solution[Shortcuts::sigma+2] = val_x*mu + val_y*mu;                         //xy

  // Velocities
  solution[Shortcuts::v+0] = val_x*cp + val_y*cs; //vx
  solution[Shortcuts::v+1] = val_y*cp + val_x*cs; //vy
"""
        )
