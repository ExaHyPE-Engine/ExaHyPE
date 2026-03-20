def algebraic_source():
    return """
//This term is not present in the Zenodo repository at https://zenodo.org/records/6386282
//however it is present in Leo's "phd" tag at https://gitlab.lrz.de/ExaHyPE-Seismic/ExaSeis/-/blob/leo/phd/Applications/DynamicRupture/PMLSlipWeakening/PMLSlipWeakening.cpp?ref_type=tags#L468
// We are working on the assumption that it should be in there.

  S[9]  = -Q[0];
  S[10] = -Q[1];
  S[11] = -Q[2];
"""


def userIncludes():
    return """
#define _CUSTOM_COORDINATES
#define ASAGI_NOMPI
#define _TOP 1

#include "../ExaSeis_core/Curvilinear/ContextDynamicRupture.h"
#include "../ExaSeis_core/Numerics/riemannsolverPMLDynamicRupture.h"

#include "peano4/datamanagement/CellMarker.h"
"""


def abstractDeclarations():
    return """
public:
  double QuadraturePoints1d[Order+1];

  tarch::la::Vector<DIMENSIONS,double> invertProjection(const tarch::la::Vector<DIMENSIONS,double> coordinates);

  static ContextDynamicRupture<VariableShortcutsElasticSolver, Order + 1, NumberOfUnknowns, NumberOfAuxiliaryVariables, {{SOLUTION_STORAGE_PRECISION}}>* context;

protected:
  static constexpr int     pml_cell_width    = 1;
  static constexpr double  pml_alpha_const   = 1.5;
  static constexpr double  pml_alpha_scalar  = 0.0;
  static constexpr double  pml_rel_error     = 0.001;
  static constexpr int     pml_power         = 1;
"""
