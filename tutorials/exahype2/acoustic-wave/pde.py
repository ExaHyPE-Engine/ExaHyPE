import sympy

import exahype2

from abc import ABC, abstractmethod

class PDE(exahype2.symhype.FirstOrderConservativePDEFormulation, ABC):
  def __init__(self, unknowns, auxiliary_variables, dimensions):
    super().__init__(unknowns=unknowns, auxiliary_variables=auxiliary_variables, dimensions=dimensions)

    self.p = self.name_Q_entry(0, "p")
    self.v = self.name_Q_entries(1, dimensions, "v")

    self.rho = sympy.symbols("rho")
    self.c = sympy.symbols("c")
    self.K0 = self.rho * self.c * self.c

    #
    # Define the equation system
    #
    # Flux [unknowns, dimensions]
    if self.dimensions == 2:
      self.F[0, 0] = self.K0 * self.v[0]
      self.F[1, 0] = 1.0 / self.rho * self.p
      self.F[2, 0] = 0

      self.F[0, 1] = self.K0 * self.v[1]
      self.F[1, 1] = 0
      self.F[2, 1] = 1.0 / self.rho * self.p
    else:
      self.F[0, 0] = self.K0 * self.v[0]
      self.F[1, 0] = 1.0 / self.rho * self.p
      self.F[2, 0] = 0
      self.F[3, 0] = 0

      self.F[0, 1] = self.K0 * self.v[1]
      self.F[1, 1] = 0
      self.F[2, 1] = 1.0 / self.rho * self.p
      self.F[3, 1] = 0

      self.F[0, 2] = self.K0 * self.v[2]
      self.F[1, 2] = 0
      self.F[2, 2] = 0
      self.F[3, 2] = 1.0 / self.rho * self.p

    # Eigenvalues [unknowns, dimensions]
    if self.dimensions == 2:
      self.eigenvalues[0] = -self.c
      self.eigenvalues[1] = 0
      self.eigenvalues[2] = self.c
    else:
      self.eigenvalues[0] = -self.c
      self.eigenvalues[1] = 0
      self.eigenvalues[2] = 0
      self.eigenvalues[3] = self.c
