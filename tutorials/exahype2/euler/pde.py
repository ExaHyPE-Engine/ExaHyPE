import sympy

import exahype2

from abc import ABC, abstractmethod

class PDE(exahype2.symhype.FirstOrderConservativePDEFormulation, ABC):
  def __init__(self, unknowns, auxiliary_variables, dimensions):
    super().__init__(unknowns=unknowns, auxiliary_variables=auxiliary_variables, dimensions=dimensions)

    #
    # Give entries in input vector symbolic names. We first declare the constant
    # gamma. Then we tell the solver how we would like to name the Q entries.
    #
    self.gamma = sympy.symbols("gamma")
    # First scalar is density
    self.rho   = self.name_Q_entry(0, "rho")
    # Momentum: Entries 1-dimensions (C counting style) holds j vector
    # (Note: To get the velocities we need to divide with the density)
    self.j     = self.name_Q_entries(1, dimensions, "j")
    # Energy
    self.E     = self.name_Q_entry(dimensions + 1, "E")
    # Pressure
    self.p = (self.gamma - 1) * (self.E-1/2 * exahype2.symhype.dot(self.j, self.j) / self.rho)

    #
    # Define the equation system
    #
    # Flux [unknowns, dimensions]
    self.F[0, :] = self.j
    self.F[1:self.dimensions + 1, :] = 1 / self.rho * exahype2.symhype.outer(self.j, self.j) + self.p * sympy.eye(dimensions)
    self.F[self.dimensions + 1, :]   = 1 / self.rho * self.j * (self.E + self.p)

    # Eigenvalues [unknowns, dimensions]
    c = sympy.sqrt(self.gamma * self.p /self.rho)
    u = self.j / self.rho # Velocities
    if self.dimensions == 3:
      self.eigenvalues[0] = [u[0] - c, u[1] - c, u[2] - c]
      self.eigenvalues[1] = [u[0],     u[1],     u[2]]
      self.eigenvalues[2] = [u[0],     u[1],     u[2]]
      self.eigenvalues[3] = [u[0],     u[1],     u[2]]
      self.eigenvalues[4] = [u[0] + c, u[1] + c, u[2] + c]
    else: # 2D
      self.eigenvalues[0] = [u[0] - c, u[1] - c]
      self.eigenvalues[1] = [u[0],     u[1]]
      self.eigenvalues[2] = [u[0],     u[1]]
      self.eigenvalues[3] = [u[0] + c, u[1] + c]
