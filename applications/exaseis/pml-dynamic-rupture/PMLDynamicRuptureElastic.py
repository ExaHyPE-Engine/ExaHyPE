# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


def algebraic_source():
    return """
  // The "phd" tag of ExaSeis adds this term in line 468 of PMLSlipWeakening.cpp. Its Zenodo release
  // omits it.
  S.u = Q.v;
"""


def userIncludes():
    return [
        "applications/exaseis/riemannSolvers/DynamicRuptureRiemannSolver.h",
        "exahype2/Curvi.h",
        "exahype2/dg/Basis.h",
        "peano4/sfc/ActiveCurve.h",
        "tarch/reader/NetCDFFieldSampler.h",
    ]
