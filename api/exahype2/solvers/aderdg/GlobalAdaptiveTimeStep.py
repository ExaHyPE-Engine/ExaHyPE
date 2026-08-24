# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .SingleSweep import SingleSweep
from exahype2.solvers import AdaptiveTimeSteppingCodeSnippets


class GlobalAdaptiveTimeStep(SingleSweep):
    def __init__(
        self,
        name,
        order,
        unknowns,
        auxiliary_variables,
        time_step_relaxation,
    ):
        super(GlobalAdaptiveTimeStep, self).__init__(
            name=name,
            order=order,
            unknowns=unknowns,
            auxiliary_variables=auxiliary_variables,
        )

        self._cflSafetyFactor = time_step_relaxation
        self._time_stepping_code_snippets = (
            AdaptiveTimeSteppingCodeSnippets.for_aderdg()
        )
