# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .GenerateKernels import *

from exahype2.solvers.fv.SingleSweep import SingleSweep
from exahype2.solvers.fv.FixedTimeSteppingCodeSnippets import (
    FixedTimeSteppingCodeSnippets,
)

class GlobalFixedTimeStep(SingleSweep):
    def __init__(
        self,
        name,
        patch_size,
        unknowns,
        auxiliary_variables,
        normalised_time_step_size,
        overlap=1,
        fuse_compute_kernels=False,
    ):
        super(GlobalFixedTimeStep, self).__init__(
            name=name,
            patch_size=patch_size,
            overlap=overlap,
            unknowns=unknowns,
            auxiliary_variables=auxiliary_variables,
            fuse_compute_kernels=fuse_compute_kernels,
        )

        self._compute_eigenvalue = False
        self._time_stepping_code_snippets = FixedTimeSteppingCodeSnippets(
            normalised_time_step_size
        )

    def _generate_kernels(self, output):
        generate_kernels(
            solver=self,
            output=output,
        )
        if self._fuse_compute_kernels:
            generate_fused_kernels(
                solver=self,
                output=output,
            )
