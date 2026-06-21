# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import os
import peano4.output
from typing import Union, Dict, List


class VariableShortcuts:
    def __init__(
        self,
        name: str,
        unknowns: Union[int, Dict[str, int]],
        auxiliary_variables: Union[int, Dict[str, int]],
    ):
        """
        Initialises the variable shortcuts and maps.

        Args:
            unknowns: Either an integer count or a dict {name: count}
            auxiliary_variables: Either an integer count or a dict {name: count}
        """
        self._name = name
        self._variable_names: List[str] = []
        self._variable_pos: List[int] = [0]
        self._plot_description: Dict[str, int] = {}
        self._generate_shortcut_file: bool = False
        self._unknowns_count = self._process_input(unknowns)
        self._auxiliary_variables_count = self._process_input(auxiliary_variables)

    def _process_input(self, data: Union[int, Dict[str, int]]) -> int:
        if isinstance(data, int):
            return data

        if isinstance(data, dict):
            # Check for zero values
            for name, count in data.items():
                if count <= 0:
                    raise ValueError(
                        f"Variable '{name}' has an invalid count of {count}. "
                        "All variable component counts must be greater than zero."
                    )

            self._generate_shortcut_file = True
            self._variable_names.extend(data.keys())
            self._plot_description.update(data)
            for count in data.values():
                self._variable_pos.append(self._variable_pos[-1] + count)
            return sum(data.values())

        raise TypeError(
            f"Invalid type '{type(data).__name__}'. "
            "Parameter must be an int or a dictionary."
        )

    def generate_shortcut_file(self, dictionary, namespace, output):
        if not self._generate_shortcut_file:
            return

        dictionary["USE_VARIABLE_SHORTCUTS"] = True
        dictionary["VARIABLE_NAMES"] = self._variable_names
        dictionary["VARIABLE_POSITIONS"] = self._variable_pos

        generated_shortcut_file = peano4.output.Jinja2TemplatedHeaderFile(
            os.path.dirname(os.path.realpath(__file__))
            + "/"
            + "VariableShortcuts.template.h",
            "VariableShortcuts" + self._name,
            namespace,
            ".",
            dictionary,
            True,
        )
        output.add(generated_shortcut_file)
        output.makefile.add_h_file(
            "VariableShortcuts" + self._name + ".h", generated=True
        )

    @property
    def unknowns(self) -> int:
        return self._unknowns_count

    @property
    def auxiliary_variables(self) -> int:
        return self._auxiliary_variables_count

    @property
    def names(self) -> List[str]:
        return self._variable_names

    @property
    def positions(self) -> List[int]:
        return self._variable_pos

    @property
    def plot_description(self) -> Dict[str, int]:
        return self._plot_description
