# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

import exahype2.codegen


class State:
    """The unknowns and auxiliary variables of a solver and the positions of named variables.

    The positions of named variables become the constants of ``<Name>VariableShortcuts``,
    which the solver aliases as ``Shortcuts``.
    """

    def __init__(self, name, unknowns, auxiliary_variables):
        self._name = name
        self._names = []
        self._positions = [0]
        self._plot_description = {}
        self._unknowns = self._count(unknowns)
        self._auxiliary_variables = self._count(auxiliary_variables)

    def _count(self, data):
        if isinstance(data, bool):
            raise TypeError(
                "A variable count expects an int or a dictionary (got a bool)."
            )
        if isinstance(data, int):
            if data < 0:
                raise ValueError("A variable count expects a non-negative int.")
            return data
        if isinstance(data, dict):
            for name, count in data.items():
                if count <= 0:
                    raise ValueError(
                        "The variable '{}' needs a positive component count "
                        "(got {}).".format(name, count)
                    )
            self._names.extend(data.keys())
            self._plot_description.update(data)
            for count in data.values():
                self._positions.append(self._positions[-1] + count)
            return sum(data.values())
        raise TypeError(
            "A variable count expects an int or a dictionary (got '{}').".format(
                type(data).__name__
            )
        )

    @property
    def unknowns(self):
        return self._unknowns

    @property
    def auxiliary_variables(self):
        return self._auxiliary_variables

    @property
    def number_of_values(self):
        return self._unknowns + self._auxiliary_variables

    @property
    def names(self):
        return self._names

    @property
    def plot_description(self):
        return self._plot_description

    @property
    def has_names(self):
        return bool(self._names)

    def indices_of(self, selection):
        """Returns the indices of the unknowns in ``selection``.

        ``selection`` is None for every unknown, one index or one variable name, or an
        iterable of indices and variable names.
        """
        if selection is None:
            return list(range(self._unknowns))
        if isinstance(selection, (int, str)):
            selection = [selection]
        indices = []
        for item in selection:
            if isinstance(item, bool):
                raise TypeError(
                    "A measured unknown expects an int or a str (got a bool)."
                )
            if isinstance(item, str):
                if item not in self._names:
                    raise ValueError(
                        "The measured unknowns accept the variable names {} only (got '{}').".format(
                            self._names, item
                        )
                    )
                position = self._names.index(item)
                indices.extend(
                    range(self._positions[position], self._positions[position + 1])
                )
            elif isinstance(item, int):
                indices.append(item)
            else:
                raise TypeError(
                    "A measured unknown expects an int or a str (got {}).".format(
                        type(item).__name__
                    )
                )
        result = []
        for index in indices:
            if index < 0 or index >= self._unknowns:
                raise ValueError(
                    "A measured unknown expects an index below {} (got {}).".format(
                        self._unknowns, index
                    )
                )
            if index not in result:
                result.append(index)
        if not result:
            raise ValueError(
                "The selection of measured unknowns needs at least one unknown."
            )
        return result

    def generate_shortcuts(self, dictionary, namespace, output):
        """Adds ``<Name>VariableShortcuts.h`` to the output when the variables have names."""
        if not self.has_names:
            return
        dictionary["USE_VARIABLE_SHORTCUTS"] = True
        dictionary["VARIABLE_NAMES"] = self._names
        dictionary["VARIABLE_POSITIONS"] = self._positions
        output.add(
            exahype2.codegen.TemplatedFile(
                os.path.join(
                    os.path.dirname(os.path.realpath(__file__)),
                    "VariableShortcuts.h.jinja2",
                ),
                None,
                self._name + "VariableShortcuts",
                namespace,
                "",
                dictionary,
                True,
            )
        )
        output.makefile.add_h_file(self._name + "VariableShortcuts.h", generated=True)
