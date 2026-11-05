# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

import exahype2.codegen


class SymmetricMatrix:
    """Declares a variable as a symmetric ``size`` x ``size`` matrix. The state stores its diagonal
    first and then its upper triangle row by row."""

    def __init__(self, size):
        if isinstance(size, bool) or not isinstance(size, int) or size <= 0:
            raise ValueError(
                "A symmetric matrix expects a positive int size (got {!r}).".format(
                    size
                )
            )
        self.size = size

    @property
    def entries(self):
        return self.size * (self.size + 1) // 2

    def __repr__(self):
        return "SymmetricMatrix({})".format(self.size)


def _is_positive_int(value):
    return isinstance(value, int) and not isinstance(value, bool) and value > 0


def _matrix_of(shape):
    """Returns the C++ type, the number of values and the header of the matrix ``shape``. Returns
    None if ``shape`` is a scalar or a vector."""
    if isinstance(shape, SymmetricMatrix):
        return (
            "tarch::la::SymmetricMatrix<{}, Value>".format(shape.size),
            shape.entries,
            "tarch/la/SymmetricMatrix.h",
        )
    if (
        isinstance(shape, tuple)
        and len(shape) == 2
        and all(_is_positive_int(extent) for extent in shape)
    ):
        return (
            "tarch::la::Matrix<{}, {}, Value>".format(shape[0], shape[1]),
            shape[0] * shape[1],
            "tarch/la/Matrix.h",
        )
    return None


def _member(name, shape):
    """Returns the data member of the variable ``name`` with ``shape``. An int gives a scalar or a
    vector. A matrix shape gives a matrix. A pair of an int and a matrix shape gives a vector of
    matrices."""
    matrix = _matrix_of(shape)
    if matrix is not None:
        return {
            "name": name,
            "declaration": "{} {}".format(matrix[0], name),
            "entries": matrix[1],
            "scalar": False,
            "blocks": 0,
            "includes": [matrix[2]],
        }
    if (
        isinstance(shape, tuple)
        and len(shape) == 2
        and _is_positive_int(shape[0])
        and _matrix_of(shape[1]) is not None
    ):
        block = _matrix_of(shape[1])
        return {
            "name": name,
            "declaration": "tarch::la::Vector<{}, {}> {}".format(
                shape[0], block[0], name
            ),
            "entries": shape[0] * block[1],
            "scalar": False,
            "blocks": shape[0],
            "block_entries": block[1],
            "includes": [block[2], "tarch/la/Vector.h"],
        }
    if _is_positive_int(shape):
        return {
            "name": name,
            "declaration": (
                "Value " + name
                if shape == 1
                else "tarch::la::Vector<{}, Value> {}".format(shape, name)
            ),
            "entries": shape,
            "scalar": shape == 1,
            "blocks": 0,
            "includes": ["tarch/la/Vector.h"],
        }
    raise ValueError(
        "The variable '{}' expects a positive int, a pair of positive ints, an "
        "exahype2.SymmetricMatrix or a pair of a positive int and one of these matrix shapes "
        "(got {!r}).".format(name, shape)
    )


class State:
    """Describes the unknowns and auxiliary variables of a solver.

    Every variable becomes a data member of ``<Name>Unknowns`` or ``<Name>State``. The solver
    exposes these structs as ``Unknowns`` and ``State``. An unnamed group becomes the vector
    member ``unknowns`` or ``auxiliaryVariables``.
    """

    RESERVED_NAMES = ("unknowns", "auxiliaryVariables")

    def __init__(self, name, unknowns, auxiliary_variables):
        self._name = name
        self._variables = []
        self._plot_description = {}
        self._unknown_members = []
        self._auxiliary_members = []
        self._unknowns = self._count(unknowns, "unknowns", self._unknown_members, 0)
        self._auxiliary_variables = self._count(
            auxiliary_variables,
            "auxiliaryVariables",
            self._auxiliary_members,
            self._unknowns,
        )

    def _count(self, data, group, members, position):
        """Returns the number of values of ``data`` and appends its members to ``members``. The
        values start at ``position`` of the state."""
        if isinstance(data, bool):
            raise TypeError(
                "A variable count expects an int or a dictionary (got a bool)."
            )
        if isinstance(data, int):
            if data < 0:
                raise ValueError("A variable count expects a non-negative int.")
            if data > 0:
                members.append(
                    {
                        "name": group,
                        "declaration": "tarch::la::Vector<{}, Value> {}".format(
                            data, group
                        ),
                        "entries": data,
                        "scalar": False,
                        "blocks": 0,
                        "includes": ["tarch/la/Vector.h"],
                    }
                )
            return data
        if isinstance(data, dict):
            for name, shape in data.items():
                if not name.isidentifier() or name in State.RESERVED_NAMES:
                    raise ValueError(
                        "A variable name expects a C++ identifier other than {} (got '{}').".format(
                            " and ".join(State.RESERVED_NAMES), name
                        )
                    )
                member = _member(name, shape)
                members.append(member)
                self._variables.append(
                    {"name": name, "position": position, "count": member["entries"]}
                )
                self._plot_description[name] = member["entries"]
                position += member["entries"]
            return sum(member["entries"] for member in members)
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
        return [variable["name"] for variable in self._variables]

    @property
    def plot_description(self):
        return self._plot_description

    @property
    def has_names(self):
        return bool(self._variables)

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
                variables = [
                    variable for variable in self._variables if variable["name"] == item
                ]
                if not variables:
                    raise ValueError(
                        "The measured unknowns accept the variable names {} only (got '{}').".format(
                            self.names, item
                        )
                    )
                indices.extend(
                    range(
                        variables[0]["position"],
                        variables[0]["position"] + variables[0]["count"],
                    )
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

    def generate_state(self, dictionary, namespace, output):
        """Writes ``<Name>State.h`` and ``<Name>State.cpph`` with the structs ``<Name>Unknowns``
        and ``<Name>State`` and their arithmetic."""
        dictionary["STATE_UNKNOWN_MEMBERS"] = self._unknown_members
        dictionary["STATE_AUXILIARY_MEMBERS"] = self._auxiliary_members
        dictionary["STATE_INCLUDES"] = sorted(
            {
                include
                for member in self._unknown_members + self._auxiliary_members
                for include in member["includes"]
            }
            | {"tarch/la/Vector.h"}
        )
        template_directory = os.path.dirname(os.path.realpath(__file__))
        output.add(
            exahype2.codegen.TemplatedFile(
                os.path.join(template_directory, "State.h.jinja2"),
                None,
                self._name + "State",
                namespace,
                "",
                dictionary,
            )
        )
        output.add(
            exahype2.codegen.TemplatedFile(
                os.path.join(template_directory, "State.cpph.jinja2"),
                None,
                self._name + "State",
                namespace,
                "",
                dictionary,
            )
        )
        output.makefile.add_h_file(self._name + "State.h", generated=True)
        output.makefile.add_h_file(self._name + "State.cpph", generated=True)
