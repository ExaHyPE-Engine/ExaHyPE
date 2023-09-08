import sympy

import exahype2

from abc import ABC, abstractmethod

class PDE(exahype2.symhype.FirstOrderConservativePDEFormulation, ABC):
  def __init__(self, unknowns, auxiliary_variables, dimensions):
    super().__init__(unknowns=unknowns, auxiliary_variables=auxiliary_variables, dimensions=dimensions)

    self.v = self.name_Q_entries(0, dimensions, "v")

    self.c = sympy.symbols("c")

    #
    # Define the equation system
    #
    # Flux [unknowns, dimensions]
    if self.dimensions == 2:
      self.F[0, 0] = self.v[0]
      self.F[1, 0] = 0

      self.F[0, 1] = 0
      self.F[1, 1] = self.v[1]
    else:
      self.F[0, 0] = self.v[0]
      self.F[1, 0] = 0
      self.F[2, 0] = 0

      self.F[0, 1] = 0
      self.F[1, 1] = self.v[1]
      self.F[2, 1] = 0

      self.F[0, 2] = 0
      self.F[1, 2] = 0
      self.F[2, 2] = self.v[2]

    # Eigenvalues [unknowns, dimensions]
    if self.dimensions == 2:
      self.eigenvalues[0] = self.c
      self.eigenvalues[1] = self.c
    else:
      self.eigenvalues[0] = self.c
      self.eigenvalues[1] = self.c
      self.eigenvalues[2] = self.c
