# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import textwrap

import exahype2.codegen

from .dof import DoF


def _dedented(blocks):
    """Returns the C++ blocks one per line without their common indentation, so a template
    indents them."""
    return textwrap.dedent("\n".join(blocks)).strip()


class ParticleGenerator:
    """Writes the data class of a particle from its attribute list."""

    def __init__(self, data):
        self.data = data

    def get_stack_container(self):
        return "::peano4::grid::Stack<" + self.data.get_full_qualified_type() + ">"

    def _get_dictionary_for_output(self):
        qualified_name = self.data.get_full_qualified_type()
        layout_attributes = sorted(
            self.data.attributes,
            key=lambda attribute: (attribute.layout_priority, attribute.name),
        )
        return {
            "ENUM_DECLARATIONS": _dedented(
                declaration
                for declaration in (
                    attribute.get_enum_declaration()
                    for attribute in self.data.attributes
                )
                if declaration is not None
            ),
            "ATTRIBUTES": _dedented(
                declaration
                for declaration in (
                    attribute.get_member_declaration()
                    for attribute in layout_attributes
                )
                if declaration
            ),
            "ATTRIBUTE_TO_STRING_PAIRS": [
                {
                    "name": attribute.name,
                    "expression": attribute.get_to_string_expression(),
                }
                for attribute in self.data.attributes
            ],
            "ATTRIBUTE_METHOD_DECLARATIONS": _dedented(
                "\n".join(attribute.get_method_declarations())
                for attribute in self.data.attributes
            ),
            "ATTRIBUTE_METHOD_DEFINITIONS": "\n\n".join(
                attribute.get_method_definitions(qualified_name)
                for attribute in self.data.attributes
            ),
            "INCLUDES": self.data.includes,
        }

    def construct_output(self, output):
        template_data = self._get_dictionary_for_output()
        output.makefile.add_cpp_file(
            self.data.namespace[-1] + "/" + self.data.name + ".cpp",
            generated=True,
        )
        template_directory = os.path.dirname(os.path.realpath(__file__))
        output.add(
            exahype2.codegen.TemplatedFile(
                os.path.join(template_directory, "Particle.h.jinja2"),
                os.path.join(template_directory, "Particle.cpp.jinja2"),
                self.data.name,
                self.data.namespace,
                self.data.namespace[-1],
                template_data,
            )
        )
        output.add(
            exahype2.codegen.TemplatedFile(
                os.path.join(template_directory, "Particle.cpph.jinja2"),
                None,
                self.data.name,
                self.data.namespace,
                self.data.namespace[-1],
                template_data,
            )
        )


class Particle(DoF):
    """A record of named attributes a particle carries through the grid."""

    def __init__(self, name):
        super().__init__(name)
        self.attributes = []
        self.includes = []
        self.generator = ParticleGenerator(self)

    def add_attribute(self, attribute):
        self.attributes.append(attribute)
