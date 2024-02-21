from .Equations import Equations
from exahype2.solvers.PDETerms import PDETerms

class Euler(Equations):
  def __init__(
      self,
      dimensions,
      gamma=1.4
  ):
    self.dimensions = dimensions
    self.num_unknowns = 4 if dimensions==2 else 5
    self.num_auxiliary_variables = 0
    self.gamma = gamma

  def eigenvalues(self):
    return """
  const double irho = 1.0/Q[0];
  constexpr double gamma = """ + str(self.gamma) + """;
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));

  const double c   = std::sqrt(gamma * p * irho);
  const double u = Q[normal+1]*irho;

  return std::max(std::abs(u-c), std::abs(u+c));
""" if self.dimensions==3 else """
  const double irho = 1.0/Q[0];
  constexpr double gamma = """ + str(self.gamma) + """;
  const double p = (gamma-1) * (Q[3] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));

  const double c   = std::sqrt(gamma * p * irho);
  const double u = Q[normal+1]*irho;

  return std::max(std::abs(u-c), std::abs(u+c));
"""

  def flux(self):
    return ("""
  const double irho = 1.0 / Q[0];
  constexpr double gamma = """ + str(self.gamma) + """;
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
  
  F[0] = Q[normal+1];
  F[1] = Q[normal+1]*Q[1]*irho;
  F[2] = Q[normal+1]*Q[2]*irho;
  F[3] = Q[normal+1]*Q[3]*irho;
  F[4] = Q[normal+1]*irho*(Q[4]+p);

  F[normal+1] += p;
""" if self.dimensions==3 else """
  const double irho = 1.0 / Q[0];
  constexpr double gamma = """ + str(self.gamma) + """;
  const double p = (gamma-1) * (Q[3] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
  
  F[0] = Q[normal+1];
  F[1] = Q[normal+1]*Q[1]*irho;
  F[2] = Q[normal+1]*Q[2]*irho;
  F[3] = Q[normal+1]*irho*(Q[3]+p);

  F[normal+1] += p;
""")
