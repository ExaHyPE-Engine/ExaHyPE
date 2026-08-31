# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4.output


class ModelToDataRepository(object):
    def __init__(self, model):
        self.model = model
        self.d = {}

    def __reserve_container_id(self):
        """
        A container id addresses one container's messages in the aggregated
        boundary exchange.

        Every rank generates this repository from the same model, so a container
        ends up with the same id everywhere and both sides of a rank pair read
        the same tag.
        """
        result = self.__next_container_id
        self.__next_container_id += 1
        return result

    def __build_up_dictionary_for_one_data_set(self, data_set, prefix):
        """:param prefix: Should be "CELL", "FACE" or "VERTEX"."""
        self.d[
            prefix + "_DATA_CONTAINER_INCLUDES"
        ] += data_set.generator.get_header_file_include()
        self.d[prefix + "_DATA_CONTAINER_INCLUDES"] += "\n"

        container = {
            "stack": data_set.generator.get_stack_container(),
            "name": data_set.get_logical_type_name(),
            "fq": data_set.get_full_qualified_type(),
            "container_id": self.__reserve_container_id(),
        }

        self.d[prefix + "_DATA_CONTAINERS"].append(container)

    def __parse_data_declarations_in_model(self, namespace_index):
        self.__next_container_id = 1

        for prefix in ["CELL", "FACE", "VERTEX"]:
            self.d[prefix + "_DATA_CONTAINER_INCLUDES"] = ""
            self.d[prefix + "_DATA_CONTAINERS"] = []

        self.d["FULL_QUALIFIED_CLASSNAME"] = ""
        for i in self.__get_full_namespace(namespace_index):
            self.d["FULL_QUALIFIED_CLASSNAME"] += i
            self.d["FULL_QUALIFIED_CLASSNAME"] += "::"
        self.d["FULL_QUALIFIED_CLASSNAME"] += self.__get_class_name()

        for i in self.model.cell_data:
            self.__build_up_dictionary_for_one_data_set(i, "CELL")

        for i in self.model.face_data:
            self.__build_up_dictionary_for_one_data_set(i, "FACE")

        for i in self.model.vertex_data:
            self.__build_up_dictionary_for_one_data_set(i, "VERTEX")

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
                namespace[-1] + "/" + class_name + ".cpp",
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
                subdirectory=namespace[-1],
                dictionary=self.d,
                apply_iteratively=True,
            )
            output.add(generated_files)
