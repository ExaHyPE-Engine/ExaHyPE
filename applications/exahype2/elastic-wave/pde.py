import sympy

import exahype2

class PDE(exahype2.symhype.FirstOrderConservativePDEFormulation):
  def __init__(self, dimensions, auxiliary_variables):
    super(PDE, self).__init__(unknowns=dimensions + 3, auxiliary_variables=auxiliary_variables, dimensions=dimensions)

    self.v = self.name_Q_entries(0, dimensions, "v")
    self.sigma = self.name_Q_entries(dimensions, 3, "sigma")

    self.initial_values[0] = 0
    self.initial_values[1] = 0
    self.initial_values[2] = 0
    self.initial_values[3] = 0
    self.initial_values[4] = 0

    self.boundary_values[0] = 0
    self.boundary_values[1] = 0
    self.boundary_values[2] = 0
    self.boundary_values[3] = 0
    self.boundary_values[4] = 0

    self.rho = sympy.symbols("rho")
    self.cp = sympy.symbols("cp")
    self.cs = sympy.symbols("cs")

    # Lamé parameters
    mu = self.rho * self.cs * self.cs
    lamb = self.rho * self.cp * self.cp - 2.0 * mu

    # Flux [unknowns, dimensions]
    self.F[0, 0] = -1.0/self.rho*self.sigma[0]
    self.F[1, 0] = -1.0/self.rho*self.sigma[2]
    self.F[2, 0] = -(2.0*mu*lamb) * self.v[0]
    self.F[3, 0] = -lamb * self.v[0]
    self.F[4, 0] = -mu * self.v[1]

    self.F[0, 1] = -1.0/self.rho*self.sigma[2]
    self.F[1, 1] = -1.0/self.rho*self.sigma[1]
    self.F[2, 1] = -lamb * self.v[1]
    self.F[3, 1] = -(2.0*mu*lamb) * self.v[1]
    self.F[4, 1] = -mu * self.v[0]

    self.eigenvalues[0] = self.cp
    self.eigenvalues[1] = self.cs
    self.eigenvalues[2] = 0
    self.eigenvalues[3] = -self.cs
    self.eigenvalues[4] = -self.cp

    self.t0 = sympy.symbols("t0")
    self.M0 = sympy.symbols("M0")
    self.t = sympy.symbols("t")
    force = self.M0 * self.t / (self.t0 * self.t0) * (sympy.exp(-self.t/self.t0))

    self.sources[0] = 0
    self.sources[1] = 0
    self.sources[2] = 0
    self.sources[3] = 0
    self.sources[4] = force
