from .Equations import Equations

class SWE_W_Bathimetry(Equations):
  def __init__(
      self,
      dimensions=2
  ):
    if dimensions!=2: raise Exception("SWE must be in 2 dimensions")
    self.dimensions               = 2
    self.num_unknowns             = 4
    self.num_auxiliary_variables  = 0

  def eigenvalues(self):
    return """
    constexpr double grav = 9.81;

    const double u = Q[1 + normal] / Q[0];
    const double c = std::sqrt(grav * Q[0]);

    return std::max(std::abs(u + c), std::abs(u - c));
"""

  def flux(self):
    return """
    double ih = 1.0 / Q[0];

    F[0] = Q[1 + normal];
    F[1] = Q[1 + normal] * Q[1] * ih;
    F[2] = Q[1 + normal] * Q[2] * ih;
    F[3] = 0.0;
"""

  def ncp(self):
    return """
    constexpr double grav = 9.81;
    BTimesDeltaQ[0] = 0.0;
    switch (normal) {
    case 0:
      BTimesDeltaQ[1] = grav * Q[0] * (deltaQ[0] + deltaQ[3]);
      BTimesDeltaQ[2] = 0.0;
      break;
    case 1:
      BTimesDeltaQ[1] = 0.0;
      BTimesDeltaQ[2] = grav * Q[0] * (deltaQ[0] + deltaQ[3]);
      break;
    }
    BTimesDeltaQ[3] = 0.0;
"""


class SWE_WO_Bathimetry(Equations):
  def __init__(
      self,
      dimensions=2
  ):
    if dimensions!=2: raise Exception("SWE must be in 2 dimensions")
    self.dimensions               = 2
    self.num_unknowns             = 3
    self.num_auxiliary_variables  = 0

  def eigenvalues(self):
    return """
    constexpr double grav = 9.81;

    const double u = Q[1 + normal] / Q[0];
    const double c = std::sqrt(grav * Q[0]);

    return std::max(std::abs(u + c), std::abs(u - c));
"""

  def flux(self):
    return """
    constexpr double grav = 9.81;
    double ih = 1.0 / Q[0];

    F[0] = Q[1 + normal];
    F[1] = Q[1 + normal] * Q[1] * ih;
    F[2] = Q[1 + normal] * Q[2] * ih;

    F[normal+1] += 0.5*grav*Q[0]*Q[0];
"""