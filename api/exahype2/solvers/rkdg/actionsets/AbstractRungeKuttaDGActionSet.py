# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from peano4.solversteps.ActionSet import ActionSet


class AbstractRungeKuttaDGActionSet(ActionSet):
    def __init__(self, solver):
        """Constructs the action set of the RungeKuttaDG solver `solver`."""
        super(AbstractRungeKuttaDGActionSet, self).__init__(descend_invocation_order=1)
        self._solver = solver

    def get_action_set_name(self):
        """
        Returns the name under which the action set is generated.

        A subclass repeats this routine, so its name comes from its own module.
        """
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        return (
            """
#include "exahype2/dg/DGUtils.h"
"""
            + self._solver._get_default_includes()
            + self._solver.user_action_set_includes
        )
