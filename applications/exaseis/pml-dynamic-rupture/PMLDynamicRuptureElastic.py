# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
def algebraic_source():
    return """
  // From ExaSeis' "phd" tag (PMLSlipWeakening.cpp, line 468); the Zenodo release omits this term.
  S[Shortcuts::u+0] = Q[Shortcuts::v+0];
  S[Shortcuts::u+1] = Q[Shortcuts::v+1];
  S[Shortcuts::u+2] = Q[Shortcuts::v+2];
"""


def userIncludes():
    return [
        "DynamicRuptureParameters.h",
        "../riemannSolvers/riemannsolverPMLDynamicRupture.h",
        "exahype2/Curvi.h",
        "exahype2/dg/DGUtils.h",
        "tarch/reader/NetCDFFieldSampler.h",
    ]
