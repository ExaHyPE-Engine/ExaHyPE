""" This file is part of the multigrid project within Peano 4. For conditions of distribution and 
 use, please see the copyright notice at www.peano-framework.org
"""
import mghype.matrixfree.solvers.api.actionsets

from .CollocatedDLinearDiscretisationWithPointJacobi                import CollocatedDLinearDiscretisationWithPointJacobi
from .ContinuousGalerkinDiscretisationGaussLobattoWithBlockJacobi   import ContinuousGalerkinDiscretisationGaussLobattoWithBlockJacobi

from .DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver import DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver
from .DiscontinuousGalerkinWithoutFusion                        import DiscontinuousGalerkinWithoutFusion
from .DGTaskingSolver                                           import DGTaskingSolver
from .DGPointWiseWithSmartPointerPatches                        import DGPointWiseWithSmartPointerPatches
from .DiscontinuousGalerkinWithFusionUnshifted                  import DiscontinuousGalerkinWithFusionUnshifted
from .CollocatedMG                                              import CollocatedMG
from .CollocatedMGSingleThreaded                                import CollocatedMGSingleThreaded

