# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair


class FloatTypes:
    def __init__(self, float_type):
        if float_type == "float":
            self.cpp = "float"
            self.mpi = "MPI_FLOAT"
            self.number_of_mpi_types_per_element = 1
        elif float_type == "double":
            self.cpp = "double"
            self.mpi = "MPI_DOUBLE"
            self.number_of_mpi_types_per_element = 1
        elif float_type == "long double":
            self.cpp = "long double"
            self.mpi = "MPI_LONG_DOUBLE"
            self.number_of_mpi_types_per_element = 1
        elif float_type == "_Float16":
            self.cpp = "_Float16"
            self.mpi = "MPI_BYTE"
            self.number_of_mpi_types_per_element = 2
        elif float_type == "std::float16_t":
            self.cpp = "std::float16_t"
            self.mpi = "MPI_BYTE"
            self.number_of_mpi_types_per_element = 2
        elif float_type == "__bf16":
            self.cpp = "__bf16"
            self.mpi = "MPI_BYTE"
            self.number_of_mpi_types_per_element = 2
        elif float_type == "std::bfloat16_t":
            self.cpp = "std::bfloat16_t"
            self.mpi = "MPI_BYTE"
            self.number_of_mpi_types_per_element = 2
        else:
            raise Exception("Float type " + float_type + " not recognized")


class PatchToData:
    def __init__(self, patch, float_type="double"):
        self.data = patch
        self.merge_method_definition = ""
        self.send_condition = "true"
        self.receive_and_merge_condition = "true"
        self.load_store_compute_flag = "::peano4::grid::LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_StoreToOutputStream"
        self.includes = ""
        self.float_type = FloatTypes(float_type)

    def __str__(self):
        result = (
            """Map to data
  - Send condition:          """
            + self.send_condition
            + """
  - Receive/merge condition: """
            + self.receive_and_merge_condition
            + """
  - Load/store compute flag: """
            + self.load_store_compute_flag
            + """
  - Merge criterion:         """
        )
        if self.merge_method_definition != "":
            result += """User-defined
"""
        else:
            result += """NOP
"""
        return result

    def get_stack_container(self):
        return (
            "::peano4::stacks::StackWithSerialisation< "
            + self.data.get_full_qualified_type()
            + " >"
        )

    def get_header_file_include(self):
        """
        This is the include statement for the data container. It is not something
        directly generated but how to use the generated code.
        """
        return (
            """
#include "peano4/stacks/StackWithSerialisation.h"
#include \""""
            + self.data.namespace[-1]
            + "/"
            + self.data.name
            + """.h"
"""
        )

    def _get_dictionary_for_output(self):
        d = {
            "CARDINALITY_2D": str(
                int(self.data.no_of_unknowns * self.data.dim[0] * self.data.dim[1])
            ),
            "CARDINALITY_3D": str(
                int(
                    self.data.no_of_unknowns
                    * self.data.dim[0]
                    * self.data.dim[1]
                    * self.data.dim[2]
                )
            ),
            "MERGE_METHOD_DECLARATIONS": "",
            "MERGE_METHOD_DEFINITIONS": "",
            "SEND_CONDITION": self.send_condition,
            "RECEIVE_AND_MERGE_CONDITION": self.receive_and_merge_condition,
            "INCLUDES": self.includes,
            "DATA_ASSOCIATION": int(self.data.association),
            "MERGE_METHOD_DEFINITION": self.merge_method_definition,
            "LOAD_STORE_COMPUTE_FLAG": self.load_store_compute_flag,
            "FLOAT_TYPE": self.float_type.cpp,
            "MPI_FLOAT_TYPE": self.float_type.mpi,
            "MPI_ELEMENTS_PER_FLOAT": self.float_type.number_of_mpi_types_per_element,
        }
        return d

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
            os.path.dirname(os.path.realpath(__file__)) + "/Data.template.h"
        )
        cppfile_template = (
            os.path.dirname(os.path.realpath(__file__)) + "/Data.template.cpp"
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
