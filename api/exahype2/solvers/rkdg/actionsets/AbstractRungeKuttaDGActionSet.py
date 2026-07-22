# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from peano4.solversteps.ActionSet import ActionSet


class AbstractRungeKuttaDGActionSet(ActionSet):
    def __init__(self, solver):
        """
        Construct the action set.

        :: Arguments

        solver: RungeKuttaDG
          Reference to creating class.
        """
        super(AbstractRungeKuttaDGActionSet, self).__init__(descend_invocation_order=1)
        self._solver = solver

    def get_action_set_name(self):
        """
        Return the name under which the action set is generated.

        Every subclass should replicate this routine, so each one yields a
        meaningful name. A subclass that does not inherits the name derived
        from this module, and the generated action sets become hard to tell
        apart.
        """
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        return (
            """
#include "exahype2/dg/DGUtils.h"
#include "exahype2/dg/CellIntegral.h"
#include "exahype2/dg/Riemann.h"
#include "exahype2/dg/BoundaryConditions.h"
"""
            + self._solver._get_default_includes()
            + self._solver.user_action_set_includes
        )
