import sympy

import exahype2

from abc import ABC, abstractmethod

class PDE(exahype2.symhype.FirstOrderConservativePDEFormulation, ABC):
  def __init__(self, unknowns, auxiliary_variables, dimensions):
    super().__init__(unknowns=unknowns, auxiliary_variables=auxiliary_variables, dimensions=dimensions)

    self.v = self.name_Q_entries(0, dimensions, "v")
    self.sigma = self.name_Q_entries(dimensions, 3, "sigma")

    self.rho = sympy.symbols("rho")
    self.cp = sympy.symbols("cp")
    self.cs = sympy.symbols("cs")

    # Lamé parameters
    mu = self.rho * self.cs * self.cs
    lamb = self.rho * self.cp * self.cp - 2.0 * mu

    #
    # Define the equation system
    #
    # Flux [unknowns, dimensions]
    self.F[0, 0] = -1.0 / self.rho * self.sigma[0]
    self.F[1, 0] = -1.0 / self.rho * self.sigma[2]
    self.F[2, 0] = -(2.0 * mu + lamb) * self.v[0]
    self.F[3, 0] = -lamb * self.v[0]
    self.F[4, 0] = -mu * self.v[1]

    self.F[0, 1] = -1.0 / self.rho * self.sigma[2]
    self.F[1, 1] = -1.0 / self.rho * self.sigma[1]
    self.F[2, 1] = -lamb * self.v[1]
    self.F[3, 1] = -(2.0 * mu + lamb) * self.v[1]
    self.F[4, 1] = -mu * self.v[0]

    # Eigenvalues [unknowns, dimensions]
    self.eigenvalues[0] = self.cp
    self.eigenvalues[1] = self.cs
    self.eigenvalues[2] = 0
    self.eigenvalues[3] = -self.cs
    self.eigenvalues[4] = -self.cp
