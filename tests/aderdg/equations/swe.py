# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .equation import Equation


class SWE_W_Bathymetry(Equation):
    def __init__(self, dimensions=2):
        if dimensions != 2:
            raise Exception("SWE must be in 2 dimensions")
        self.dimensions = 2
        self.num_unknowns = {"h": 1, "hv": 2, "b": 1}
        self.num_auxiliary_variables = 0

    def eigenvalues(self):
        return """
    constexpr auto grav = 9.81;

    const auto u = Q[Shortcuts::hv + normal] / Q[Shortcuts::h];
    const auto c = std::sqrt(grav * Q[Shortcuts::h]);

    return std::fmax(std::abs(u + c), std::abs(u - c));
"""

    def flux(self):
        return """
    const auto ih = 1 / Q[Shortcuts::h];

    F[Shortcuts::h] = Q[Shortcuts::hv + normal];
    F[Shortcuts::hv+0] = Q[Shortcuts::hv + normal] * Q[Shortcuts::hv+0] * ih;
    F[Shortcuts::hv+1] = Q[Shortcuts::hv + normal] * Q[Shortcuts::hv+1] * ih;
    F[Shortcuts::b] = 0;
"""

    def ncp(self):
        return """
    constexpr auto grav = 9.81;
    BTimesDeltaQ[Shortcuts::h] = 0;
    switch (normal) {
    case 0:
      BTimesDeltaQ[Shortcuts::hv+0] = grav * Q[Shortcuts::h] * (deltaQ[Shortcuts::h] + deltaQ[Shortcuts::b]);
      BTimesDeltaQ[Shortcuts::hv+1] = 0;
      break;
    case 1:
      BTimesDeltaQ[Shortcuts::hv+0] = 0;
      BTimesDeltaQ[Shortcuts::hv+1] = grav * Q[Shortcuts::h] * (deltaQ[Shortcuts::h] + deltaQ[Shortcuts::b]);
      break;
    }
    BTimesDeltaQ[Shortcuts::b] = 0;
"""

    def riemann_solver(self):
        return """
  double smax = 0;
  constexpr auto grav = 9.81;
  for (int xy = 0; xy < DGOrder + 1; xy++) {
    smax = std::max(smax, maxEigenvalue(&QL[xy*4], x, h, t, dt, direction));
    smax = std::max(smax, maxEigenvalue(&QR[xy*4], x, h, t, dt, direction));
  }

  for (int xy = 0; xy < DGOrder + 1; xy++) {
    FL[xy*4+Shortcuts::h] = 0.5*(
        FL[xy*4+Shortcuts::h]+FR[xy*4+Shortcuts::h]
      + smax*(QL[xy*4+Shortcuts::h]+QL[xy*4+Shortcuts::b]-QR[xy*4+Shortcuts::h]-QR[xy*4+Shortcuts::b])
    );
    FL[xy*4+Shortcuts::hv+0] = 0.5*(
        FL[xy*4+Shortcuts::hv+0]+FR[xy*4+Shortcuts::hv+0]
      + smax*(QL[xy*4+Shortcuts::hv+0]-QR[xy*4+Shortcuts::hv+0])
    );
    FL[xy*4+Shortcuts::hv+1] = 0.5*(
        FL[xy*4+Shortcuts::hv+1]+FR[xy*4+Shortcuts::hv+1]
      + smax*(QL[xy*4+Shortcuts::hv+1]-QR[xy*4+Shortcuts::hv+1])
    );
    FL[xy*4+Shortcuts::b] = 0;

    FR[xy*4+Shortcuts::h]   = FL[xy*4+Shortcuts::h];
    FR[xy*4+Shortcuts::hv+0] = FL[xy*4+Shortcuts::hv+0];
    FR[xy*4+Shortcuts::hv+1] = FL[xy*4+Shortcuts::hv+1];
    FR[xy*4+Shortcuts::b]   = 0;

    const auto ncp = 0.5*grav*0.5
      *(QL[xy*4+Shortcuts::h]+QR[xy*4+Shortcuts::h])
      *(QR[xy*4+Shortcuts::b]+QR[xy*4+Shortcuts::h]-QL[xy*4+Shortcuts::b]-QL[xy*4+Shortcuts::h]);
    FL[xy*4+Shortcuts::hv+direction] += ncp;
    FR[xy*4+Shortcuts::hv+direction] -= ncp;
  }
"""
