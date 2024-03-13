from .Equations import Equations
from exahype2.solvers.PDETerms import PDETerms

class Elastic(Equations):
  def __init__(
      self,
      dimensions
  ):
    self.dimensions = dimensions
    self.num_unknowns = 5 if dimensions==2 else 9
    self.num_auxiliary_variables = 3
    self.is_linear = True

  def eigenvalues(self):
    return """
#if Dimensions==2
  const double cp = Q[6];
  const double cs = Q[7];
#else
  const double cp = Q[10];
  const double cs = Q[11];
#endif
  return std::max(std::abs(cp), std::abs(cs));
"""

  def flux(self):
    return ("""
  //Lamé parameters
    const double iRho   = 1.0/Q[5];
    const double Mu     = Q[5] * Q[7] * Q[7];             //rho*cs*cs
    const double Lambda = Q[5] * Q[6] * Q[6] - 2.0 * Mu;  //rho*cp*cp-2*Mu

    switch (normal) {
    case 0:
      F[0] = -iRho * Q[2];
      F[1] = -iRho * Q[4];
      F[2] = -(2.0 * Mu + Lambda) * Q[0];
      F[3] = -Lambda * Q[0];
      F[4] = -Mu * Q[1];
      break;
    case 1:
      F[0] = -iRho * Q[4];
      F[1] = -iRho * Q[3];
      F[2] = -Lambda * Q[1];
      F[3] = -(2.0 * Mu + Lambda) * Q[1];
      F[4] = -Mu * Q[0];
      break;
    }
""" if self.dimensions==2 else """
  //Lamé parameters
  const double iRho   = 1.0/Q[9];
  const double Mu     = Q[9] * Q[11] * Q[11];             //rho*cs*cs
  const double Lambda = Q[9] * Q[10] * Q[10] - 2.0 * Mu;  //rho*cp*cp - 2 Mu

  /*
  vars in order are: 
  vx, vy, vz, sxx, syy, szz, sxy, sxz, syz, rho, cp, cs
  */
  switch(normal) {
    case 0:
      F[0] = - iRho*Q[3]; // -sigma_xx/rho
      F[1] = - iRho*Q[6]; // -sigma_xy/rho
      F[2] = - iRho*Q[7]; // -sigma_xz/rho
      F[3] = -(Lambda + 2*Mu) * Q[0]; // -vx * (l+2m)
      F[4] = - Lambda * Q[0];         // -vy * l
      F[5] = - Lambda * Q[0];         // -vz * l
      F[6] = - Mu * Q[1];             // -vy * m
      F[7] = - Mu * Q[2];             // -vz * m
      F[8] =   0.0;                   //   0.
      break;
    case 1:
      F[0] = - iRho*Q[6]; // -sigma_xy/rho
      F[1] = - iRho*Q[4]; // -sigma_yy/rho
      F[2] = - iRho*Q[8]; // -sigma_yz/rho
      F[3] = - Lambda * Q[1];         // -vx * l
      F[4] = -(Lambda + 2*Mu) * Q[1]; // -vy * (l+2m)
      F[5] = - Lambda * Q[1];         // -vz * l
      F[6] = - Mu * Q[0];             // -vx * m
      F[7] =   0.0;                   //   0.
      F[8] = - Mu * Q[2];             // -vz * m
      break;
    case 2:
      F[0] = - iRho*Q[7]; // -sigma_xz/rho
      F[1] = - iRho*Q[8]; // -sigma_yz/rho
      F[2] = - iRho*Q[5]; // -sigma_zz/rho
      F[3] = - Lambda * Q[2];         // -vx * l
      F[4] = - Lambda * Q[2];         // -vy * l
      F[5] = -(Lambda + 2*Mu) * Q[2]; // -vz * (l+2m)
      F[6] =   0.0;                   //   0.
      F[7] = - Mu * Q[0];             // -vx * m
      F[8] = - Mu * Q[1];             // -vy * m
  }
""")
