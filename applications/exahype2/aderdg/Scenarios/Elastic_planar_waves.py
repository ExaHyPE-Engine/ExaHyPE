from .Scenario import Scenario

import os
import sys
sys.path.insert(0, os.path.abspath("../Equations"))
from Equations import Elastic

class Elastic_planar_waves(Scenario):
    
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
        self._end_time    = iterations
        self._equation    = Elastic(dimensions)


    def initial_conditions(self):
        return """
  //Lamé parameters
  constexpr double mu     =  4.0;
  constexpr double lambda = -4.0;

  //Constant auxiliary parameters
  constexpr double rho  = 1.0;
  constexpr double cp   = 2.0;
  constexpr double cs   = 2.0;

  const double val_x = cos( -std::numbers::pi*x[0]);
  const double val_y = cos( -std::numbers::pi*x[1]);
""" + ("""
  const double val_z = cos( -std::numbers::pi*x[2]);

  //stresses
  Q[3] = val_x*(lambda+2*mu)  + val_y*lambda        + val_z*lambda;         //xx
  Q[4] = val_x*lambda         + val_y*(lambda+2*mu) + val_z*lambda;         //yy
  Q[5] = val_x*lambda         + val_y*lambda        + val_z*(lambda+2*mu);  //zz
  Q[6] = val_x*mu; //xy
  Q[7] = val_z*mu; //xz
  Q[8] = val_y*mu; //yz

  //velocities
  Q[0] = val_x*cp + val_z*cs; //vx
  Q[1] = val_y*cp + val_x*cs; //vy
  Q[2] = val_z*cp + val_y*cs; //vz

  //auxiliary variables
  Q[9]  = rho; //rho
  Q[10]   = cp; //cp
  Q[11]   = cs; //cs
""" if self._dimensions==3 else """

  //stresses
  Q[2] = val_x*(lambda+2*mu)  + val_y*lambda;         //xx
  Q[3] = val_x*lambda         + val_y*(lambda+2*mu);  //yy
  Q[4] = val_x*mu + val_y*mu;                         //xy

  //velocities
  Q[0] = val_x*cp + val_y*cs; //vx
  Q[1] = val_y*cp + val_x*cs; //vy

  //auxiliary variables
  Q[5]  = rho; //rho
  Q[6]   = cp; //cp
  Q[7]   = cs; //cs
""")

    def analytical_solution(self):
        return """
  //Lamé parameters
  constexpr double mu     =  4.0;
  constexpr double lambda = -4.0;

  //Constant auxiliary parameters
  constexpr double rho  = 1.0;
  constexpr double cp   = 2.0;
  constexpr double cs   = 2.0;

  constexpr double w = -2*M_PI;

  const double val_x = cos( w*t -std::numbers::pi*x[0]);
  const double val_y = cos( w*t -std::numbers::pi*x[1]);
""" + ("""
  const double val_z = cos( -std::numbers::pi*x[2]);

  //stresses
  sol[3] = val_x*(lambda+2*mu)  + val_y*lambda        + val_z*lambda;         //xx
  sol[4] = val_x*lambda         + val_y*(lambda+2*mu) + val_z*lambda;         //yy
  sol[5] = val_x*lambda         + val_y*lambda        + val_z*(lambda+2*mu);  //zz
  sol[6] = val_x*mu; //xy
  sol[7] = val_z*mu; //xz
  sol[8] = val_y*mu; //yz

  //velocities
  sol[0] = val_x*cp + val_z*cs; //vx
  sol[1] = val_y*cp + val_x*cs; //vy
  sol[2] = val_z*cp + val_y*cs; //vz
""" if self._dimensions==3 else """

  //stresses
  sol[2] = val_x*(lambda+2*mu)  + val_y*lambda;         //xx
  sol[3] = val_x*lambda         + val_y*(lambda+2*mu);  //yy
  sol[4] = val_x*mu + val_y*mu;                         //xy

  //velocities
  sol[0] = val_x*cp + val_y*cs; //vx
  sol[1] = val_y*cp + val_x*cs; //vy
""")