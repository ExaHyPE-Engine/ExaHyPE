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
        min_cell_h,
        max_cell_h,
        time_step_relaxation,
        plot_grid_properties=False,
    ):
        super(GlobalAdaptiveTimeStep, self).__init__(
            name=name,
            order=order,
            unknowns=unknowns,
            auxiliary_variables=auxiliary_variables,
            min_cell_h=min_cell_h,
            max_cell_h=max_cell_h,
            plot_grid_properties=plot_grid_properties,
        )

        self._cflSafetyFactor = time_step_relaxation
        self._time_stepping_code_snippets = AdaptiveTimeSteppingCodeSnippets()
