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
        min_volume_h,
        max_volume_h,
        normalised_time_step_size,
        use_enclave_tasking=False,
        number_of_enclave_tasks=1,
        plot_grid_properties=False,
        overlap=1,
    ):
        super(GlobalFixedTimeStep, self).__init__(
            name=name,
            patch_size=patch_size,
            overlap=overlap,
            unknowns=unknowns,
            auxiliary_variables=auxiliary_variables,
            min_volume_h=min_volume_h,
            max_volume_h=max_volume_h,
            use_enclave_tasking=use_enclave_tasking,
            number_of_enclave_tasks=number_of_enclave_tasks,
            plot_grid_properties=plot_grid_properties,
        )

        self._compute_eigenvalue = False
        self._time_stepping_code_snippets = FixedTimeSteppingCodeSnippets(normalised_time_step_size)

    def _generate_kernels(self, output, subdirectory):
        generate_kernels(
            solver=self,
            output=output,
            subdirectory=subdirectory,
        )
        if self._use_enclave_tasking and self._can_fuse:
            generate_fused_kernels(
                solver=self,
                output=output,
                subdirectory=subdirectory,
            )
