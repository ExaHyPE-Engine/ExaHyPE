def algebraic_source():
    return """
  // From ExaSeis' "phd" tag (PMLSlipWeakening.cpp, line 468); the Zenodo release omits this term.
  S[Shortcuts::u+0] = Q[Shortcuts::v+0];
  S[Shortcuts::u+1] = Q[Shortcuts::v+1];
  S[Shortcuts::u+2] = Q[Shortcuts::v+2];
"""


def userIncludes():
    return """
#define _TOP 1

#include "DynamicRuptureParameters.h"
#include "../riemannSolvers/riemannsolverPMLDynamicRupture.h"

#include "exahype2/Curvi.h"
#include "exahype2/dg/DGUtils.h"

#include "tarch/reader/NetCDFFieldSampler.h"
"""
