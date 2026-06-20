# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4.output


class MetaDataToData(object):
    def __init__(self, data):
        self.data = data

    def get_stack_container(self):
        return "::peano4::stacks::Stack< " + self.data.get_full_qualified_type() + " >"

    def get_header_file_include(self):
        return (
            """
#include "peano4/stacks/Stack.h"
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
                    attribute.get_enum_declaration() for attribute in self.data.attributes
                )
                if declaration is not None
            ),
            "ATTRIBUTES": "\n".join(
                declaration
                for declaration in (
                    attribute.get_private_declaration()
                    for attribute in layout_attributes
                )
                if declaration
            ),
            "STATIC_ATTRIBUTES": "\n".join(
                declaration
                for declaration in (
                    attribute.get_static_declaration()
                    for attribute in self.data.attributes
                )
                if declaration
            ),
            "ATTRIBUTE_METHOD_DECLARATIONS": "\n".join(
                "\n".join(attribute.get_public_declarations())
                for attribute in self.data.attributes
            ),
            "ATTRIBUTE_METHOD_DEFINITIONS": "\n".join(
                attribute.get_definitions(qualified_name)
                for attribute in self.data.attributes
            ),
            "ATTRIBUTE_TO_STRING_PAIRS": [
                {
                    "name": attribute.name,
                    "expression": attribute.get_to_string_expression(),
                }
                for attribute in self.data.attributes
            ],
            "CONSTRUCTOR_INITIALISERS": ",\n  ".join(
                f"{attribute.field_name}({attribute.get_initialiser()})"
                for attribute in layout_attributes
                if attribute.get_initialiser() is not None
            ),
            "STATIC_ATTRIBUTE_DEFINITIONS": "\n".join(
                definition
                for definition in (
                    attribute.get_static_definition(qualified_name)
                    for attribute in self.data.attributes
                )
                if definition
            ),
            "INCLUDES": self.data.includes,
            "DATA_ASSOCIATION": int(self.data.association),
            "MERGE_METHOD_DEFINITION": self.data.merge_method_definition,
            "SEND_CONDITION": self.data.send_condition,
            "RECEIVE_AND_MERGE_CONDITION": self.data.receive_and_merge_condition,
        }

    def construct_output(self, output):
        d = self._get_dictionary_for_output()
        output.makefile.add_cpp_file(
            self.data.subdirectory
            + self.data.namespace[-1]
            + "/"
            + self.data.name
            + ".cpp",
            generated=True,
        )
        headerfile_template = (
            os.path.dirname(os.path.realpath(__file__)) + "/MetaData.template.h"
        )
        cppfile_template = (
            os.path.dirname(os.path.realpath(__file__)) + "/MetaData.template.cpp"
        )
        generated_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            headerfile_template,
            cppfile_template,
            self.data.name,
            self.data.namespace,
            self.data.subdirectory + self.data.namespace[-1],
            d,
            True,
        )
        output.add(generated_files)
