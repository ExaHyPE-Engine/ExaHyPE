import sympy

import exahype2

class PDE(exahype2.symhype.FirstOrderConservativePDEFormulation):
  def __init__(self, dimensions, auxiliary_variables):
    super(PDE, self).__init__(unknowns=dimensions + 1, auxiliary_variables=auxiliary_variables, dimensions=dimensions)

    self.p = self.name_Q_entry(0, "p")
    self.v = self.name_Q_entries(1, dimensions, "v")

    self.initial_values[0] = 0
    self.initial_values[1] = 0
    self.initial_values[2] = 0

    self.boundary_values[0] = 0
    self.boundary_values[1] = 0
    self.boundary_values[2] = 0

    self.rho = sympy.symbols("rho")
    self.c = sympy.symbols("c")
    K0 = self.rho * self.c * self.c

    # Flux [unknowns, dimensions]
    self.F[0, 0] = -K0 * self.v[0]
    self.F[1, 0] = -1.0/self.rho * self.p
    self.F[2, 0] = 0

    self.F[0, 1] = -K0 * self.v[1]
    self.F[1, 1] = 0
    self.F[2, 1] = -1.0/self.rho * self.p

    self.eigenvalues[0] = self.c
    self.eigenvalues[1] = -self.c
    self.eigenvalues[2] = 0

    self.sigma = sympy.symbols("sigma")
    self.t0 = sympy.symbols("t0")
    self.M0 = sympy.symbols("M0")
    self.t = sympy.symbols("t")
    force = self.M0 * sympy.exp(-((self.t - self.t0) * (self.t - self.t0)) / (2.0 * self.sigma * self.sigma))

    self.sources[0] = force
    self.sources[1] = 0
    self.sources[2] = 0
