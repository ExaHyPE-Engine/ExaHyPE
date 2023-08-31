import sympy

import exahype2

from abc import ABC, abstractmethod

class PDE(exahype2.symhype.FirstOrderConservativePDEFormulation, ABC):
  @abstractmethod
  def __init__(self, dimensions=2, unknowns=3, auxiliary_variables=1):
    super().__init__(unknowns=unknowns, auxiliary_variables=auxiliary_variables, dimensions=2)

    self.h = self.name_Q_entry(0, "h")
    self.hu = self.name_Q_entry(1, "hu")
    self.hv = self.name_Q_entry(2, "hv")
    self.b = self.name_auxiliary_variable(0, "b")

    # gravitational constant
    self.g = sympy.symbols('g')

    # speed in x direction
    self.u = sympy.Piecewise(
                              (self.hu / self.h, self.h > 0.0), 
                              (0.0, True)
      )

    # speed in y direction
    self.v = sympy.Piecewise(
                              (self.hv / self.h, self.h > 0.0), 
                              (0.0, True)
      )

    self.definiton_of_flux()

    self.definition_of_eigenvalues()

  def definiton_of_flux(self):
    # Flux [unknowns, dimensions]
    self.F[0, 0] = sympy.Piecewise(
                                    (self.hu, self.h > 0.0), 
                                    (0.0, True)
                                  )
    self.F[1, 0] = sympy.Piecewise(
                                    (self.hu * self.u + (self.g * self.h**2)/2, self.h > 0.0),
                                    (0.0, True)
                                  )
    self.F[2, 0] = sympy.Piecewise(
                                    (self.h * self.u * self.v, self.h > 0.0),
                                    (0.0, True)
                                  )

    self.F[0, 1] = sympy.Piecewise(
                                    (self.hv, self.h > 0.0),
                                    (0.0, True)
                                  )
    self.F[1, 1] = sympy.Piecewise(
                                    (self.h * self.u * self.v, self.h > 0.0),
                                    (0.0, True)
                                  )
    self.F[2, 1] = sympy.Piecewise(
                                    (self.hv * self.v + (self.g * self.h**2)/2, self.h > 0.0),
                                    (0.0, True)
                                  )

  def definition_of_eigenvalues(self):
    # Eigenvalues [unknowns, dimensions]
    self.eigenvalues[0, 0] = self.u
    self.eigenvalues[0, 1] = self.v

    self.eigenvalues[1, 0] = sympy.Piecewise(
                                              (self.u + sympy.sqrt(self.g * self.h), self.h > 0.0),
                                              (0.0, True)
                                            )
    self.eigenvalues[1, 1] = sympy.Piecewise(
                                              (self.v + sympy.sqrt(self.g * self.h), self.h > 0.0),
                                              (0.0, True)
                                            )

    self.eigenvalues[2, 0] = sympy.Piecewise(
                                              (self.u - sympy.sqrt(self.g * self.h), self.h > 0.0),
                                              (0.0, True)
                                            )
    self.eigenvalues[2, 1] = sympy.Piecewise(
                                              (self.v - sympy.sqrt(self.g * self.h), self.h > 0.0),
                                              (0.0, True)
                                            )

  def outflow_boundary_conditions(self):
    # outflow boundary conditions
    self.boundary_values[0] = self.h
    self.boundary_values[1] = self.hu
    self.boundary_values[2] = self.hv
    self.boundary_values[3] = self.b

  def reflective_boundary_conditions(self):
    # reflective boundary conditions
    self.boundary_values[0] = self.h
    self.boundary_values[1] = -self.hu
    self.boundary_values[2] = -self.hv
    self.boundary_values[3] = self.b