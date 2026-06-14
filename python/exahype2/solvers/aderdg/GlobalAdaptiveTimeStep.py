# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .SingleSweep import SingleSweep
from .AdaptiveTimeSteppingCodeSnippets import AdaptiveTimeSteppingCodeSnippets


class GlobalAdaptiveTimeStep(SingleSweep):
    def __init__(
        self,
        name,
        order,
        unknowns,
        auxiliary_variables,
        time_step_relaxation,
        use_enclave_tasking=False,
        number_of_enclave_tasks=1,
    ):
        super(GlobalAdaptiveTimeStep, self).__init__(
            name=name,
            order=order,
            unknowns=unknowns,
            auxiliary_variables=auxiliary_variables,
            use_enclave_tasking=use_enclave_tasking,
            number_of_enclave_tasks=number_of_enclave_tasks,
        )

        self._cflSafetyFactor = time_step_relaxation
        self._time_stepping_code_snippets = AdaptiveTimeSteppingCodeSnippets()
