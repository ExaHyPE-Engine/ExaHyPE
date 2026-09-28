# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

import exahype2.codegen


class State:
    """Counts the unknowns and auxiliary variables of a solver and names their positions.

    Named variables reach the generated code as the constants of ``<Name>VariableShortcuts``,
    which the solver exposes as ``Shortcuts``.
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
            raise TypeError("A variable count is an int or a dictionary, not a bool.")
        if isinstance(data, int):
            if data < 0:
                raise ValueError("A variable count cannot be negative.")
            return data
        if isinstance(data, dict):
            for name, count in data.items():
                if count <= 0:
                    raise ValueError(
                        "Variable '{}' has an invalid count of {}. Every component count "
                        "is greater than zero.".format(name, count)
                    )
            self._names.extend(data.keys())
            self._plot_description.update(data)
            for count in data.values():
                self._positions.append(self._positions[-1] + count)
            return sum(data.values())
        raise TypeError(
            "Invalid type '{}'. A variable count is an int or a dictionary.".format(
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
        """Returns the unknown indices ``selection`` names.

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
                raise TypeError("A measured unknown is an int or a str, not a bool.")
            if isinstance(item, str):
                if item not in self._names:
                    raise ValueError(
                        "Unknown variable name '{}'. Known names are {}.".format(
                            item, self._names
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
                    "A measured unknown is an int or a str, got {}.".format(
                        type(item).__name__
                    )
                )
        result = []
        for index in indices:
            if index < 0 or index >= self._unknowns:
                raise ValueError(
                    "Unknown index {} is out of range [0, {}).".format(
                        index, self._unknowns
                    )
                )
            if index not in result:
                result.append(index)
        if not result:
            raise ValueError("The selection of measured unknowns is empty.")
        return result

    def generate_shortcuts(self, dictionary, namespace, output):
        """Writes ``<Name>VariableShortcuts.h`` when the variables carry names."""
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
