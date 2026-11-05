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
    constexpr auto Gravity = 9.81;

    const auto u = Q.hv(normal) / Q.h;
    const auto c = std::sqrt(Gravity * Q.h);

    return std::fmax(std::abs(u + c), std::abs(u - c));
"""

    def flux(self):
        return """
    const auto inverseHeight = 1 / Q.h;

    F.h = Q.hv(normal);
    for (int axis = 0; axis < 2; axis++) {
      F.hv(axis) = Q.hv(normal) * Q.hv(axis) * inverseHeight;
    }
    F.b = 0;
"""

    def ncp(self):
        return """
    constexpr auto Gravity = 9.81;
    BTimesDeltaQ            = {};
    BTimesDeltaQ.hv(normal) = Gravity * Q.h * (deltaQ.h + deltaQ.b);
"""

    def riemann_solver(self):
        return """
  double smax = 0;
  constexpr auto Gravity = 9.81;
  for (int node = 0; node < DGOrder + 1; node++) {
    smax = std::max(smax, maxEigenvalue(QL[node], x, h, t, dt, normal, parameters));
    smax = std::max(smax, maxEigenvalue(QR[node], x, h, t, dt, normal, parameters));
  }

  for (int node = 0; node < DGOrder + 1; node++) {
    FL[node].h = 0.5*(
        FL[node].h+FR[node].h
      + smax*((QL[node].h+QL[node].b)-(QR[node].h+QR[node].b))
    );
    for (int axis = 0; axis < 2; axis++) {
      FL[node].hv(axis) = 0.5*(
          FL[node].hv(axis)+FR[node].hv(axis)
        + smax*(QL[node].hv(axis)-QR[node].hv(axis))
      );
    }
    FL[node].b = 0;
    FR[node]   = FL[node];

    const auto ncp = 0.5*Gravity*0.5
      *(QL[node].h+QR[node].h)
      *((QR[node].b+QR[node].h)-(QL[node].b+QL[node].h));
    FL[node].hv(normal) += ncp;
    FR[node].hv(normal) -= ncp;
  }
  return smax;
"""
