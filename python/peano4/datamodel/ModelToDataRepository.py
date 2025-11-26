# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4.output


class ModelToDataRepository(object):
    def __init__(self, model):
        self.model = model
        self.d = {}

    def __build_up_dictionary_for_one_data_set(self, i):
        self.d["DATA_CONTAINER_INCLUDES"] += i.generator.get_header_file_include()
        self.d["DATA_CONTAINER_INCLUDES"] += "\n"

        self.d["DATA_CONTAINER_DECLARATION"] += (
            "static ::peano4::stacks::TreeToStacks<"
            + i.generator.get_stack_container()
            + "> _"
            + i.get_logical_type_name()
            + "Stack;\n"
        )
        self.d["DATA_CONTAINER_INSTANTIATION"] += (
            "::peano4::stacks::TreeToStacks<"
            + i.generator.get_stack_container()
            + "> "
            + self.d["FULL_QUALIFIED_CLASSNAME"]
            + "::_"
            + i.get_logical_type_name()
            + "Stack;\n"
        )

        self.d["DATA_CONTAINERS"].append(
            {"name": i.get_logical_type_name(), "fq": i.get_full_qualified_type()}
        )

    def __parse_data_declarations_in_model(self, n):
        subdirectory = self.model.subdirectories[n]
        self.d["DATA_CONTAINER_INCLUDES"] = ""
        self.d["DATA_CONTAINER_DECLARATION"] = ""
        self.d["DATA_CONTAINER_INSTANTIATION"] = ""
        self.d["DATA_CONTAINERS"] = []
        self.d["FULL_QUALIFIED_DATA_CONTAINERS"] = []

        self.d["FULL_QUALIFIED_CLASSNAME"] = ""
        for i in self.__get_full_namespace(n):
            self.d["FULL_QUALIFIED_CLASSNAME"] += i
            self.d["FULL_QUALIFIED_CLASSNAME"] += "::"
        self.d["FULL_QUALIFIED_CLASSNAME"] += self.__get_class_name()

        for i in self.model.cell_data:
            if i.subdirectory == subdirectory:
                self.__build_up_dictionary_for_one_data_set(i)

        for i in self.model.face_data:
            if i.subdirectory == subdirectory:
                self.__build_up_dictionary_for_one_data_set(i)

        for i in self.model.vertex_data:
            if i.subdirectory == subdirectory:
                self.__build_up_dictionary_for_one_data_set(i)

    def __get_full_namespace(self, i):
        return self.model.namespaces[i] + ["repositories"]

    def __get_class_name(self):
        return "DataRepository"

    def construct_output(self, output):
        for i in range(len(self.model.namespaces)):
            self.__parse_data_declarations_in_model(i)

            namespace = self.__get_full_namespace(i)
            class_name = self.__get_class_name()
            output.makefile.add_cpp_file(
                self.model.subdirectories[i]
                + namespace[-1]
                + "/"
                + class_name
                + ".cpp",
                generated=True,
            )
            headerfile_template = (
                os.path.dirname(os.path.realpath(__file__))
                + "/DataRepository.template.h"
            )
            cppfile_template = (
                os.path.dirname(os.path.realpath(__file__))
                + "/DataRepository.template.cpp"
            )
            generated_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                headfile_template=headerfile_template,
                cppfile_template=cppfile_template,
                classname=class_name,
                namespace=namespace,
                subdirectory=self.model.subdirectories[i] + namespace[-1],
                dictionary=self.d,
                default_overwrite=True,
                apply_iteratively=True,
            )
            output.add(generated_files)
