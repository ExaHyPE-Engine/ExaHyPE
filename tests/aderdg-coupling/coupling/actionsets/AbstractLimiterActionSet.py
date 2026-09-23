# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from exahype2.actionsets import ActionSet


class AbstractLimiterActionSet(ActionSet):
    """The base of the action sets that a static coupling adds to a grid sweep."""

    def __init__(self, solver, guard):
        """``solver`` is the coupling that creates this action set and ``guard`` the
        condition under which the emitted C++ runs."""
        super(AbstractLimiterActionSet, self).__init__()
        self._solver = solver
        self.guard = guard

    @property
    def name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def is_user_template(self):
        return False

    def render(self, template, **entries):
        """Renders a Jinja2 template against the coupling's dictionary and ``entries``."""
        dictionary = dict(self._solver.dictionary)
        dictionary["PREDICATE"] = self.guard
        dictionary.update(entries)
        return jinja2.Template(template, undefined=jinja2.DebugUndefined).render(
            **dictionary
        )

    def includes(self):
        return self._solver.action_set_includes()
