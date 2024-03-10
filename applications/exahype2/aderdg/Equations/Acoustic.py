from .Equations import Equations

class Acoustic(Equations):
  def __init__(
      self,
      dimensions,
      rho = 1.0,
      K0  = 4.0
  ):
    self.dimensions = dimensions
    self.num_unknowns = 3 if dimensions==2 else 4
    self.num_auxiliary_variables = 0
    self.rho = rho
    self.K0  = K0
    self.is_linear = True

  def eigenvalues(self):
    return """
  constexpr double c = std::sqrt(""" + str(self.K0) + "/" + str(self.rho) + """);
  return c;
"""

  def flux(self):
    return """

  constexpr double K0     = """ + str(self.K0) + """;
  constexpr double inv_rho = 1.0 / """ + str(self.rho) + """;

  F[0] = K0 * Q[normal+1];
  F[1] = 0.0;
  F[2] = 0.0;
#if Dimensions==3
  F[3] = 0.0;
#endif

  F[normal+1] = inv_rho * Q[0];

"""