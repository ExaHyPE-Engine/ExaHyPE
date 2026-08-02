def algebraic_source():
    return """
//This term is not present in the Zenodo repository at https://zenodo.org/records/6386282
//however it is present in Leo's "phd" tag at https://gitlab.lrz.de/ExaHyPE-Seismic/ExaSeis/-/blob/leo/phd/Applications/DynamicRupture/PMLSlipWeakening/PMLSlipWeakening.cpp?ref_type=tags#L468
// We are working on the assumption that it should be in there.

  S[Shortcuts::u+0] = -Q[Shortcuts::v+0];
  S[Shortcuts::u+1] = -Q[Shortcuts::v+1];
  S[Shortcuts::u+2] = -Q[Shortcuts::v+2];
"""


def userIncludes():
    return """
#define _TOP 1

#include "DynamicRuptureParameters.h"
#include "../riemannSolvers/riemannsolverPMLDynamicRupture.h"

#include "exahype2/dg/DGUtils.h"
#include "toolbox/blockstructured/Curvi.h"

#include "tarch/reader/NetCDFFieldSampler.h"
"""