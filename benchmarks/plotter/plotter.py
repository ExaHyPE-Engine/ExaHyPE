# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import argparse
import os
import subprocess

import peano4

parser = argparse.ArgumentParser(description="Build the plotter I/O benchmark.")
parser.add_argument("-d", "--dimensions", type=int, default=3, choices=[2, 3])
parser.add_argument(
    "-m",
    "--build-mode",
    default="Release",
    choices=["Release", "Debug", "Asserts", "Trace"],
)
args = parser.parse_args()

makefile = peano4.output.Makefile()
makefile.parse_configuration_outcome()
makefile.set_dimension(args.dimensions)
makefile.set_mode(peano4.output.string_to_mode(args.build_mode))
makefile.set_executable_name("PlotterBenchmark")
makefile.add_cpp_file("PlotterBenchmark.cpp")
makefile.d["BUILD_SUBDIR"] = "PlotterBenchmark.build"
makefile.generate(".")

subprocess.check_call(["make", "-j" + str(os.cpu_count() or 1)])
