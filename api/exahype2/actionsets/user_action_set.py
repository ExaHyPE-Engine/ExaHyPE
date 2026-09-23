# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .action_set import ActionSet


class UserActionSet(ActionSet):
    def __init__(self, name, descend_invocation_order=0):
        super().__init__(descend_invocation_order)
        self._name = name

    def is_user_template(self):
        return True

    def get_constructor_body(self):
        return "  // @todo Please implement\n"

    def get_destructor_body(self):
        return "  // @todo Please implement\n"

    def grid_control_events_body(self):
        return "  // @todo Please implement\n  return std::vector<peano4::grid::GridControlEvent>();\n"

    def operation_body(self, operation_name):
        return """
  // @todo Please implement
"""

    @property
    def name(self):
        return self._name

    def attributes(self):
        return "  // @todo Please implement\n"

    def includes(self):
        return []
