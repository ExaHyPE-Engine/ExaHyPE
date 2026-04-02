# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4

max_depth = 5
dimension = 3
build_mode = peano4.output.CompileMode.Release

project = peano4.Project(
    ["tests", "peano4", "grid"], "Spacetree", executable="Spacetree"
)

min_h = 1.1 / (3.0**max_depth)
project.constants.export_constexpr_with_type("MinH", str(min_h), "double")

project.output.makefile.parse_configuration_outcome()
project.output.makefile.set_dimension(dimension)
project.output.makefile.set_mode(build_mode)
project.output.makefile.add_cpp_file("Spacetree-main.cpp")
project.output.makefile.add_cpp_file("CreateGridObserver.cpp")
project.output.makefile.add_cpp_file("EmptyTraversalObserver.cpp")

project.build()
