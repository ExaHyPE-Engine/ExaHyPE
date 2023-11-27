# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from swift2.particle.SPHLeapfrogFixedSearchRadius import SPHLeapfrogFixedSearchRadius
from swift2.particle.AlgorithmStep import AlgorithmStep

import peano4
import dastgen2
import peano4.dastgen2

from abc import ABC
import copy

class testDensityCalculation(SPHLeapfrogFixedSearchRadius):
    """!
    Test only the density calculation algorithmic step.
    Inherit everything from SPHLeapfrogFixedSearchRadius, just
    modify the algorithm steps.


    """

    def __init__(
        self,
        name,
        dimensions_hydro,
        cfl_factor,
        initial_time_step_size,
        particles_per_cell=5,
        min_h=0.6,
        max_h=0.5,
    ):
        # Init the SPH particle
        super(testDensityCalculation, self).__init__(
            name = name,
            dimensions_hydro = dimensions_hydro,
            cfl_factor = cfl_factor,
            initial_time_step_size=initial_time_step_size,
            particles_per_cell=particles_per_cell,
            min_h=min_h,
            max_h=max_h,
            use_optimised_coalesced_memory_access_kernels=False,
            asserts=False,
        )

        # Now overwrite the Algorithm and Init steps.
        self._setup_algorithm_steps()
        self._setup_initialisation_steps()

        return


    def _setup_algorithm_steps(self):
        """
        Set up the internal list of algorithm steps for this particle.
        We need to maintain an individual instance of this list for cases
        where we modify the algorithm steps later on. This can happen for
        example when adding debugging/dependency checks later on.


        The algorithm steps shall be a list of AlgorithmStep objects to be
        executed in that order.

        Definitions of the algorithm steps stored in the algorithm_steps_dict are
        placed in _setup_algorithm_steps_dict(self). The dict is set up during
        instantiation of this class.

        """

        steps = [
            self.algorithm_steps_dict["Density"],
            self.algorithm_steps_dict["ReduceGlobalQuantities"],
        ]

        self._algorithm_steps = [copy.deepcopy(i) for i in steps]

        return

    def _setup_initialisation_steps(self):
        """
        Define the algorithm steps to be taken during initialization here.
        This follows the same logic as _setup_algorithm_steps. See documentation
        there for more info.

        Make sure self._setup_algorithm_steps_dict() has been called before.
        """
        initsteps = []

        # return deep copies of algorithms steps. Otherwise,
        # modifications down the line (like the dependency checks)
        # will cause trouble.
        self._initialisation_steps = [copy.deepcopy(i) for i in initsteps]

        return


