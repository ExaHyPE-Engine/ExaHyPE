# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4.output


class ParticleToData:
    def __init__(self, data):
        self.data = data

    def get_stack_container(self):
        return "::peano4::grid::Stack< " + self.data.get_full_qualified_type() + " >"

    def get_header_file_include(self):
        return (
            """
#include "peano4/grid/Stack.h"
#include \""""
            + self.data.namespace[-1]
            + "/"
            + self.data.name
            + """.h"
"""
        )

    def _get_dictionary_for_output(self):
        qualified_name = self.data.get_full_qualified_type()
        layout_attributes = sorted(
            self.data.attributes,
            key=lambda attribute: (attribute.layout_priority, attribute.name),
        )
        return {
            "ENUM_DECLARATIONS": "\n".join(
                declaration
                for declaration in (
                    attribute.get_enum_declaration()
                    for attribute in self.data.attributes
                )
                if declaration is not None
            ),
            "ATTRIBUTES": "\n".join(
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
            "ATTRIBUTE_METHOD_DECLARATIONS": "\n".join(
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
        headerfile_template = (
            os.path.dirname(os.path.realpath(__file__)) + "/Particle.template.h"
        )
        cppfile_template = (
            os.path.dirname(os.path.realpath(__file__)) + "/Particle.template.cpp"
        )
        cpphfile_template = (
            os.path.dirname(os.path.realpath(__file__)) + "/Particle.template.cpph"
        )
        generated_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            headerfile_template,
            cppfile_template,
            self.data.name,
            self.data.namespace,
            self.data.namespace[-1],
            template_data,
        )
        output.add(generated_files)
        output.add(
            peano4.output.Jinja2TemplatedHeaderFile(
                cpphfile_template,
                self.data.name,
                self.data.namespace,
                self.data.namespace[-1],
                template_data,
            )
        )
