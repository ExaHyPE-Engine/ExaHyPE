# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .equation import Equation


class SWE_W_Bathymetry(Equation):
    def __init__(self, dimensions=2):
        if dimensions != 2:
            raise Exception("SWE must be in 2 dimensions")
        self.dimensions = 2
        self.num_unknowns = 4
        self.num_auxiliary_variables = 0

    def eigenvalues(self):
        return """
    constexpr double grav = 9.81;

    const double u = Q[1 + normal] / Q[0];
    const double c = std::sqrt(grav * Q[0]);

    return std::fmax(std::abs(u + c), std::abs(u - c));
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

    def riemann_solver(self):
        return """
  //Compute max eigenvalue over face
  double smax = 0.0;
  for(int xy=0; xy<Order+1; xy++){
    smax = std::max(smax, maxEigenvalue(&QL[xy*4], x, h, t, dt, direction));
    smax = std::max(smax, maxEigenvalue(&QR[xy*4], x, h, t, dt, direction));
  }

  for(int xy=0; xy<Order+1; xy++){

    // h gets added term from bathimetry, u and v are regular Rusanov, b does not change
    FL[xy*4+0] = 0.5*(FL[xy*4+0]+FR[xy*4+0] + smax*(QL[xy*4+0]+QL[xy*4+3]-QR[xy*4+0]-QR[xy*4+3]) );
    FL[xy*4+1] = 0.5*(FL[xy*4+1]+FR[xy*4+1] + smax*(QL[xy*4+1]-QR[xy*4+1]) );
    FL[xy*4+2] = 0.5*(FL[xy*4+2]+FR[xy*4+2] + smax*(QL[xy*4+2]-QR[xy*4+2]) );
    FL[xy*4+3] = 0.0;

    FR[xy*4+0] = FL[xy*4+0];
    FR[xy*4+1] = FL[xy*4+1];
    FR[xy*4+2] = FL[xy*4+2];
    FR[xy*4+3] = 0.0;

    //Contribution from NCP
    FL[xy*4+direction+1] += 0.5*9.81*0.5*(QL[xy*4+0]+QR[xy*4+0])*(QR[xy*4+3]+QR[xy*4+0]-QL[xy*4+3]-QL[xy*4+0]);
    FR[xy*4+direction+1] -= 0.5*9.81*0.5*(QL[xy*4+0]+QR[xy*4+0])*(QR[xy*4+3]+QR[xy*4+0]-QL[xy*4+3]-QL[xy*4+0]);
  }
"""


class SWE_WO_Bathymetry(Equation):
    def __init__(self, dimensions=2):
        if dimensions != 2:
            raise Exception("SWE must be in 2 dimensions")
        self.dimensions = 2
        self.num_unknowns = 3
        self.num_auxiliary_variables = 0

    def eigenvalues(self):
        return """
    constexpr double grav = 9.81;

    const double u = Q[1 + normal] / Q[0];
    const double c = std::sqrt(grav * Q[0]);

    return std::fmax(std::abs(u + c), std::abs(u - c));
"""

    def flux(self):
        return """
    constexpr double grav = 9.81;
    double ih = 1.0 / Q[0];

    F[0] = Q[1 + normal];
    F[1] = Q[1 + normal] * Q[1] * ih;
    F[2] = Q[1 + normal] * Q[2] * ih;

    F[normal+1] += 0.5 * grav * Q[0] * Q[0];
"""
