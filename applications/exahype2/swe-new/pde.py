import sympy

import exahype2

from abc import ABC, abstractmethod

class PDE(exahype2.symhype.FirstOrderConservativePDEFormulation, ABC):
  @abstractmethod
  def __init__(self, dimensions=2, auxiliary_variables=1):
    super().__init__(unknowns=dimensions+1, auxiliary_variables=auxiliary_variables, dimensions=dimensions)

    self.h = self.name_Q_entry(0, "h")
    self.hu = self.name_Q_entry(1, "hu")
    self.hv = self.name_Q_entry(2, "hv")
    self.b = self.name_Q_entry(dimensions + auxiliary_variables, "b")

    # gravitational constant
    self.g = sympy.symbols('g')

    # speed in x direction
    self.u = self.hu / self.h

    # speed in y direction
    self.v = self.hv / self.h

    # Flux [unknowns, dimensions]
    self.F[0, 0] = self.hu
    self.F[1, 0] = self.hu * self.u + (self.g * self.h**2)/2
    self.F[2, 0] = self.h * self.u * self.v

    self.F[0, 1] = self.hv
    self.F[1, 1] = self.h * self.u * self.v
    self.F[2, 1] = self.hv * self.v + (self.g * self.h**2)/2

    # Eigenvalues [unkowns, dimensions]
    self.eigenvalues[0, 0] = self.u
    self.eigenvalues[0, 1] = self.v

    self.eigenvalues[1, 0] = self.u + sympy.sqrt(self.g * self.h) # maybe no sympy.sqrt(self.g * self.h) as done in exahype2/applications/aderdg/SWE/w_bathimetry/SWE.CPP:77
    self.eigenvalues[1, 1] = self.v + sympy.sqrt(self.g * self.h)

    self.eigenvalues[2, 0] = self.u - sympy.sqrt(self.g * self.h)
    self.eigenvalues[2, 1] = self.v - sympy.sqrt(self.g * self.h)

  def implementation_of_reflective_BC(self):
    return """
    Qoutside[0] = Qinside[0];
    Qoutside[1] = -1.f * Qinside[1];
    Qoutside[2] = -1.f * Qinside[2];
    Qoutside[3] = Qinside[3];
    """