# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.codegen.helper import KeepUndefined
from exahype2.codegen.helper import render_jinja2_template

from .action_set import ActionSet


class SolverActionSet(ActionSet):
    """An action set of one solver; its C++ renders against the solver's cached dictionary.

    A subclass names the calls it emits per traversal event through ``operation_body`` and
    adds its own entries through ``render``.
    """

    def __init__(self, solver, descend_invocation_order=1):
        super().__init__(descend_invocation_order)
        self._solver = solver

    @property
    def solver(self):
        return self._solver

    def includes(self):
        return [
            "peano4/Loop.h",
            "repositories/SolverRepository.h",
        ] + self._solver.action_set_includes()

    def render(self, template, **entries):
        """Renders a Jinja2 template against the solver dictionary and ``entries``."""
        dictionary = dict(self._solver.dictionary)
        dictionary.update(entries)
        return render_jinja2_template(template, dictionary, KeepUndefined)
