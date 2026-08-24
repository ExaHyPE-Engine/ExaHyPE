# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from . import (
    configure_compute_kernel_call,
    validate_overlap,
)

from exahype2.solvers import FixedTimeSteppingCodeSnippets
from exahype2.solvers.fv.SingleSweep import SingleSweep


class GlobalFixedTimeStep(SingleSweep):
    def __init__(
        self,
        name,
        patch_size,
        unknowns,
        auxiliary_variables,
        normalised_time_step_size,
        overlap=1,
    ):
        validate_overlap(overlap)
        super().__init__(
            name=name,
            patch_size=patch_size,
            overlap=overlap,
            unknowns=unknowns,
            auxiliary_variables=auxiliary_variables,
        )

        self._compute_eigenvalue = False
        self._time_stepping_code_snippets = (
            FixedTimeSteppingCodeSnippets.for_finite_volume(normalised_time_step_size)
        )

    def _generate_kernels(self, output):
        configure_compute_kernel_call(self, output)
