# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .GenerateKernels import *

from exahype2.solvers.fd.SingleSweep import SingleSweep
from exahype2.solvers.fd.AdaptiveTimeSteppingCodeSnippets import (
    AdaptiveTimeSteppingCodeSnippets,
)


class GlobalAdaptiveTimeStep(SingleSweep):
    def __init__(
        self,
        name,
        patch_size,
        order,
        unknowns,
        auxiliary_variables,
        time_step_relaxation,
        solvers_before_in_coupling=[],
    ):
        super(GlobalAdaptiveTimeStep, self).__init__(
            name=name,
            patch_size=patch_size,
            order=order,
            unknowns=unknowns,
            auxiliary_variables=auxiliary_variables,
            solvers_before_in_coupling=solvers_before_in_coupling,
        )

        self._compute_eigenvalue = True
        self._time_step_relaxation = time_step_relaxation
        self._time_stepping_code_snippets = AdaptiveTimeSteppingCodeSnippets(
            self._time_step_relaxation
        )

    def _generate_kernels(self, output, subdirectory):
        generate_kernels(
            solver=self,
            output=output,
            subdirectory=subdirectory,
        )
