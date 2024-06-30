# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4.solversteps


class AbstractADERDGActionSet(peano4.solversteps.ActionSet):
    def __init__(self, solver):
        super(AbstractADERDGActionSet, self).__init__(
            descend_invocation_order=1, parallel=False
        )
        self._solver = solver

    def get_action_set_name(self):
        """
        You should replicate this function in each subclass, so you get
        meaningful action set names (otherwise, it will be
        AbstractADERDGActionSet0,1,2,...).
        """
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        return (
            """
#include <functional>
#include "exahype2/dg/DGUtils.h"
"""
            + self._solver._get_default_includes()
            + self._solver.get_user_action_set_includes()
        )
