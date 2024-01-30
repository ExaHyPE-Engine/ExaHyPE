# This file is part of the Swift2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2


class Inflow(ActionSet):
    def __init__(
        self,
        particle_set,
    ):
        """!

        Add inflow boundary conditions


        """
        super(Inflow, self).__init__()
