# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .SingleSweep import SingleSweep
from exahype2.solvers import FixedTimeSteppingCodeSnippets


class GlobalFixedTimeStep(SingleSweep):
    def __init__(
        self,
        name,
        order,
        unknowns,
        auxiliary_variables,
        normalised_time_step_size,
    ):
        super(GlobalFixedTimeStep, self).__init__(
            name=name,
            order=order,
            unknowns=unknowns,
            auxiliary_variables=auxiliary_variables,
        )

        self._time_stepping_code_snippets = FixedTimeSteppingCodeSnippets.for_aderdg(
            normalised_time_step_size
        )
