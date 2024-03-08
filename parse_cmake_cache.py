#!/usr/bin/env python

import os
import sys

def populate_predefined_strings():
    defined_build_types = [
        "Debug",
        "Release",
        "Asserts",
        "Trace",
        "Stats"
    ]
    predefined_strings = []
    for build_type in defined_build_types:
        capitalized_build_type = build_type.upper()
        predefined_strings += [
            "PEANO_CXX_FLAGS_" + capitalized_build_type,
            "PEANO_CUDA_FLAGS_" + capitalized_build_type,
            "PEANO_" + capitalized_build_type + "_DEFINITIONS",
        ]
    predefined_strings += [
        "PEANO_DEFINITIONS",
        "USE_COMPONENT_DEFINITIONS",
        "ENABLE_PARTICLES",
        "ENABLE_EXAHYPE",
        "ENABLE_FINITEELEMENTS",
        "ENABLE_BLOCKSTRUCTURED",
        "ENABLE_LOADBALANCING",
        "ENABLE_PARTICLES",
        "ENABLE_SWIFT",
        "PEANO_INCLUDES",
        "PEANO_DEPENDS",
        "PEANO_FLAGS",
        "PEANO_LINK_FLAGS",
        "_PROJECT_OPTIONS_INTERFACE_LINK_OPTIONS",
        "_PROJECT_OPTIONS_INTERFACE_COMPILE_OPTIONS",
        "_PROJECT_OPTIONS_INTERFACE_COMPILE_DEFINITIONS",
        "_PROJECT_OPTIONS_LINK_OPTIONS",
        "_PROJECT_OPTIONS_COMPILE_OPTIONS",
        "_PROJECT_OPTIONS_COMPILE_DEFINITIONS",
    ]

    return predefined_strings

predefined_strings = populate_predefined_strings()


def search_and_copy_strings(input_file_path, predefined_strings, output_directory):
    # Check if the input file exists
    if not os.path.exists(input_file_path):
        print("Input file does not exist.")
        return

    with open(input_file_path, 'r') as file:
        with open(output_file_path, 'w') as output_file:
            for line in file:
                for string in predefined_strings:
                    if string in line:
                        if "MPI_CXX_COMPILER" in line or "WITH_GPU_ARCH" in line \
                            or "WITH_GPU" in line:
                            output_file.write("#" + line)
                        elif not line.startswith("//"):
                            tokens = line.split("=", 1)
                            var_name_and_type = tokens[0]
                            # Replace ; with " " and also do not consider the last character which is
                            # a new line
                            value = tokens[1].replace(";", " ").replace("\n", "")
                            var_name = var_name_and_type.split(":")[0]
                            line_as_set_command = f'set({var_name} {value} CACHE INTERNAL "")\n'
                            output_file.write(line_as_set_command)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python script.py input_file_path")
        sys.exit(1)

    input_file_path = sys.argv[1]
    predefined_strings = populate_predefined_strings()
    output_file_path = os.path.join(os.path.dirname(input_file_path), "Config.cmake")
    search_and_copy_strings(input_file_path, predefined_strings, output_file_path)
