# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .GenerateKernels import *

from exahype2.solvers.fd.SingleSweep import SingleSweep
from exahype2.solvers.fd.FixedTimeSteppingCodeSnippets import (
    FixedTimeSteppingCodeSnippets,
)


class GlobalFixedTimeStep(SingleSweep):
    def __init__(
        self,
        name,
        patch_size,
        order,
        unknowns,
        auxiliary_variables,
        normalised_time_step_size,
        solvers_before_in_coupling=[],
    ):
        super(GlobalFixedTimeStep, self).__init__(
            name=name,
            patch_size=patch_size,
            order=order,
            unknowns=unknowns,
            auxiliary_variables=auxiliary_variables,
            solvers_before_in_coupling=solvers_before_in_coupling,
        )

        self._compute_eigenvalue = False
        self._normalised_time_step_size = normalised_time_step_size
        self._time_stepping_code_snippets = FixedTimeSteppingCodeSnippets(
            self._normalised_time_step_size
        )

    def _generate_kernels(self, output):
        generate_kernels(
            solver=self,
            output=output,
        )
