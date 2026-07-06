# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


class Limiter:
    superbee = """
    // Superbee
    const {{COMPUTE_PRECISION}} a = QR[unknown] - Q[unknown];
    const {{COMPUTE_PRECISION}} b = Q[unknown] - QL[unknown];
    if (tarch::la::equals(b, 0.0)) {
      return 0.0;
    } else {
      const double r = a / b;
      const double phi = tarch::la::max(0.0, std::min(2.0 * r, 1.0), std::min(r, 2.0));
      return phi * b;
    }
"""

    minmod = """
    // Minmod
    const {{COMPUTE_PRECISION}} a = QR[unknown] - Q[unknown];
    const {{COMPUTE_PRECISION}} b = Q[unknown] - QL[unknown];
    if (a * b <= 0.0) {
      return 0.0;
    } else {
      if (tarch::la::smaller(tarch::la::abs(a), tarch::la::abs(b))) {
        return a;
      } else {
        return b;
      }
    }
"""

    zero = """
    // Zero slopes
    return 0.0;
"""

    average = """
    // Central differences
    const {{COMPUTE_PRECISION}} a = QR[unknown] - Q[unknown];
    const {{COMPUTE_PRECISION}} b = Q[unknown] - QL[unknown];
    return (a + b) / 2;
"""

    vanalbada = """
    // Van Albada
    const {{COMPUTE_PRECISION}} a = QR[unknown] - Q[unknown];
    const {{COMPUTE_PRECISION}} b = Q[unknown] - QL[unknown];
    if (tarch::la::equals(b, 0.0)) {
      return 0.0;
    } else {
      const double r = a / b;
      const double rSquared = r * r;
      const double phi = (rSquared + r) / (rSquared + 1.0);
      return phi * b;
    }
"""

    mclim = """
    // MCLim
    const {{COMPUTE_PRECISION}} a = QR[unknown] - Q[unknown];
    const {{COMPUTE_PRECISION}} b = Q[unknown] - QL[unknown];
    if (tarch::la::equals(b, 0.0)) {
      return 0.0;
    } else {
      const double r = a / b;
      const double phi = std::max(0.0, tarch::la::min((1 + r) / 2, 2.0, 2.0 * r));
      return phi * b;
    }
"""
