# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .equation import Equation


class Elastic(Equation):
    def __init__(self, dimensions):
        self.dimensions = dimensions
        self.num_unknowns = {"v": dimensions, "sigma": (3 if dimensions == 2 else 6)}
        5 if dimensions == 2 else 9
        self.num_auxiliary_variables = {"rho": 1, "cp": 1, "cs": 1}
        self.is_linear = True

    def eigenvalues(self):
        return """
  return std::fmax(
    std::abs(Q[Shortcuts::cp]), 
    std::abs(Q[Shortcuts::cs])
  );
"""

    def flux(self):
        return (
            """
    // Lamé parameters
    const auto iRho   = 1/Q[Shortcuts::rho];
    const auto Mu     = Q[Shortcuts::rho] * Q[Shortcuts::cs] * Q[Shortcuts::cs];          //rho*cs*cs
    const auto Lambda = Q[Shortcuts::rho] * Q[Shortcuts::cp] * Q[Shortcuts::cp] - 2 * Mu; //rho*cp*cp - 2 Mu

    switch (normal) {
    case 0:
      F[Shortcuts::v+0] = -iRho * Q[Shortcuts::sigma+0];
      F[Shortcuts::v+1] = -iRho * Q[Shortcuts::sigma+2];
      F[Shortcuts::sigma+0] = -(2 * Mu + Lambda) * Q[Shortcuts::v+0];
      F[Shortcuts::sigma+1] = -Lambda * Q[Shortcuts::v+0];
      F[Shortcuts::sigma+2] = -Mu * Q[Shortcuts::v+1];
      break;
    case 1:
      F[Shortcuts::v+0] = -iRho * Q[Shortcuts::sigma+2];
      F[Shortcuts::v+1] = -iRho * Q[Shortcuts::sigma+1];
      F[Shortcuts::sigma+0] = -Lambda * Q[Shortcuts::v+1];
      F[Shortcuts::sigma+1] = -(2 * Mu + Lambda) * Q[Shortcuts::v+1];
      F[Shortcuts::sigma+2] = -Mu * Q[Shortcuts::v+0];
      break;
    }
"""
            if self.dimensions == 2
            else """
  // Lamé parameters
  const auto iRho   = 1/Q[Shortcuts::rho];
  const auto Mu     = Q[Shortcuts::rho] * Q[Shortcuts::cs] * Q[Shortcuts::cs];          //rho*cs*cs
  const auto Lambda = Q[Shortcuts::rho] * Q[Shortcuts::cp] * Q[Shortcuts::cp] - 2 * Mu; //rho*cp*cp - 2 Mu

  /*
  vars in order are:
  vx, vy, vz, sxx, syy, szz, sxy, sxz, syz, rho, cp, cs
  */
  switch(normal) {
    case 0:
      F[Shortcuts::v+0] = - iRho*Q[Shortcuts::sigma+0]; // -sigma_xx/rho
      F[Shortcuts::v+1] = - iRho*Q[Shortcuts::sigma+3]; // -sigma_xy/rho
      F[Shortcuts::v+2] = - iRho*Q[Shortcuts::sigma+4]; // -sigma_xz/rho
      F[Shortcuts::sigma+0] = -(Lambda + 2*Mu) * Q[Shortcuts::v+0]; // -vx * (l+2m)
      F[Shortcuts::sigma+1] = - Lambda * Q[Shortcuts::v+0];         // -vy * l
      F[Shortcuts::sigma+2] = - Lambda * Q[Shortcuts::v+0];         // -vz * l
      F[Shortcuts::sigma+3] = - Mu * Q[Shortcuts::v+1];             // -vy * m
      F[Shortcuts::sigma+4] = - Mu * Q[Shortcuts::v+2];             // -vz * m
      F[Shortcuts::sigma+5] =   0;                                  //   0.
      break;
    case 1:
      F[Shortcuts::v+0] = - iRho*Q[Shortcuts::sigma+3]; // -sigma_xy/rho
      F[Shortcuts::v+1] = - iRho*Q[Shortcuts::sigma+1]; // -sigma_yy/rho
      F[Shortcuts::v+2] = - iRho*Q[Shortcuts::sigma+5]; // -sigma_yz/rho
      F[Shortcuts::sigma+0] = - Lambda * Q[Shortcuts::v+1];         // -vx * l
      F[Shortcuts::sigma+1] = -(Lambda + 2*Mu) * Q[Shortcuts::v+1]; // -vy * (l+2m)
      F[Shortcuts::sigma+2] = - Lambda * Q[Shortcuts::v+1];         // -vz * l
      F[Shortcuts::sigma+3] = - Mu * Q[Shortcuts::v+0];             // -vx * m
      F[Shortcuts::sigma+4] =   0;                                  //   0.
      F[Shortcuts::sigma+5] = - Mu * Q[Shortcuts::v+2];             // -vz * m
      break;
    case 2:
      F[Shortcuts::v+0] = - iRho*Q[Shortcuts::sigma+4]; // -sigma_xz/rho
      F[Shortcuts::v+1] = - iRho*Q[Shortcuts::sigma+5]; // -sigma_yz/rho
      F[Shortcuts::v+2] = - iRho*Q[Shortcuts::sigma+2]; // -sigma_zz/rho
      F[Shortcuts::sigma+0] = - Lambda * Q[Shortcuts::v+2];         // -vx * l
      F[Shortcuts::sigma+1] = - Lambda * Q[Shortcuts::v+2];         // -vy * l
      F[Shortcuts::sigma+2] = -(Lambda + 2*Mu) * Q[Shortcuts::v+2]; // -vz * (l+2m)
      F[Shortcuts::sigma+3] =   0;                                  //   0.
      F[Shortcuts::sigma+4] = - Mu * Q[Shortcuts::v+0];             // -vx * m
      F[Shortcuts::sigma+5] = - Mu * Q[Shortcuts::v+1];             // -vy * m
  }
"""
        )
