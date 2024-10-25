# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.PDETerms import PDETerms

from .GlobalFixedTimeStep import GlobalFixedTimeStep
from .GlobalAdaptiveTimeStep import GlobalAdaptiveTimeStep
from .GlobalAdaptiveTimeStepWithEnclaveTasking import (
    GlobalAdaptiveTimeStepWithEnclaveTasking,
)


from .kernels import SolverVariant
from .kernels import KernelVariant

from .amr import switch_to_FD4_tensor_product_interpolation
from .amr import switch_to_FD4_tensor_product_restriction
from .amr import switch_to_FD4_matrix_interpolation
from .amr import switch_to_FD4_matrix_restriction
from .amr import switch_to_FD4_second_order_interpolation
from .amr import switch_to_FD4_second_order_restriction
from .amr import switch_to_FD4_third_order_interpolation
from .amr import switch_to_FD4_third_order_restriction
