# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .SingleSweep import SingleSweep
from .FixedTimeSteppingCodeSnippets import FixedTimeSteppingCodeSnippets


class GlobalFixedTimeStep(SingleSweep):
    def __init__(
        self,
        name,
        order,
        unknowns,
        auxiliary_variables,
        min_cell_h,
        max_cell_h,
        normalised_time_step_size,
        use_enclave_tasking=False,
        number_of_enclave_tasks=1,
        plot_grid_properties=False,
    ):
        super(GlobalFixedTimeStep, self).__init__(
            name=name,
            order=order,
            unknowns=unknowns,
            auxiliary_variables=auxiliary_variables,
            min_cell_h=min_cell_h,
            max_cell_h=max_cell_h,
            use_enclave_tasking=use_enclave_tasking,
            number_of_enclave_tasks=number_of_enclave_tasks,
            plot_grid_properties=plot_grid_properties,
        )

        self._time_stepping_code_snippets = FixedTimeSteppingCodeSnippets(normalised_time_step_size)
