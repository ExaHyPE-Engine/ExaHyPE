from .Equations import Equations
from exahype2.solvers.PDETerms import PDETerms

class Advection(Equations):
  def __init__(
      self,
      dimensions,
      adv_speed = 1.0
  ):
    self.dimensions = dimensions
    self.num_unknowns = 2 if dimensions==2 else 3
    self.num_auxiliary_variables = 0
    self.adv_speed = adv_speed
    self.is_linear = True

  def eigenvalues(self):
    return """
  constexpr double v = """ + str(self.adv_speed) + """;
  return v;
"""

  def flux(self):
    return """

    F[0] = 0.0;
    F[1] = 0.0;
#if Dimensions == 3
    F[2] = 0.0;
#endif

    F[normal] = """ + str(self.adv_speed) + """ * Q[normal];

"""
