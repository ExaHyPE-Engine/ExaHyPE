# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from . import (
    Weighting,
    configure_compute_kernel_call,
    validate_overlap,
)

from exahype2.solvers import AdaptiveTimeSteppingCodeSnippets
from exahype2.solvers.fv.SingleSweep import SingleSweep


class GlobalAdaptiveTimeStep(SingleSweep):
    def __init__(
        self,
        name,
        patch_size,
        unknowns,
        auxiliary_variables,
        time_step_relaxation,
        weighting=Weighting.jiangshu,
        overlap=3,
    ):
        validate_overlap(overlap)
        super().__init__(
            name=name,
            patch_size=patch_size,
            overlap=overlap,
            unknowns=unknowns,
            auxiliary_variables=auxiliary_variables,
        )

        self._compute_eigenvalue = True
        self._time_stepping_code_snippets = (
            AdaptiveTimeSteppingCodeSnippets.for_finite_volume(time_step_relaxation)
        )
        self._weighting = weighting

    def _generate_kernels(self, output):
        configure_compute_kernel_call(self, output, self._weighting)
