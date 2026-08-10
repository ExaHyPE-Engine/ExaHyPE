# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import subprocess
import argparse
import jinja2

import peano4
import exahype2

from CCZ4Solver import CCZ4Solver_FV_GlobalAdaptiveTimeStepWithEnclaveTasking

parser = argparse.ArgumentParser(
    description="ExaHyPE 2 - CCZ4 Finite Volume Rusanov Kernel Benchmarking Script"
)
parser.add_argument(
    "-m",
    "--build-mode",
    choices=peano4.output.CompileModes,
    default=peano4.output.CompileModes[0], # Release
    help="|".join(peano4.output.CompileModes),
)
parser.add_argument(
    "-t",
    "--num-threads",
    default=1,
    type=int,
    help="Number of launching threads",
)
parser.add_argument(
    "-ps",
    "--patch-size",
    default=16,
    type=int,
    help="Number of finite volumes per axis (dimension) per patch.",
)
parser.add_argument(
    "-p",
    "--num-patches",
    nargs="+",
    default=[32, 64, 128, 256, 512, 1024, 2048],
    type=int,
    help="Number of patches to study",
)
parser.add_argument(
    "-samples",
    "--samples",
    default=10,
    type=int,
    help="Number of samples per measurement",
)
parser.add_argument(
    "-a",
    "--accuracy",
    default=0.0,
    type=float,
    help="Floating point accuracy to which the different kernel variants have to match (absolute). Pass in 0 to disable correctness check. Pass in values < 0 to use machine epsilon (default).",
)
parser.add_argument(
    "-e",
    "--eval-eig",
    action="store_false",
    help="Evaluate max. eigenvalue",
)
parser.add_argument(
    "-cpu",
    "--cpu",
    action="store_true",
    help="Assess host kernels",
)
parser.add_argument(
    "-gpu",
    "--gpu",
    action="store_true",
    help="Assess device kernels",
)
parser.add_argument(
    "-mlirgpu",
    "--mlir-gpu",
    action="store_true",
    dest="mlir_gpu",
    help="Assess MLIR GPU kernels",
)
parser.add_argument(
    "-serial",
    "--serial",
    action="store_true",
    help="Assess serial kernels",
)
parser.add_argument(
    "-parallel",
    "--parallel",
    action="store_true",
    help="Assess parallel kernels",
)
parser.add_argument(
    "-fpe",
    "--fpe",
    action="store_true",
    help="Enable a floating-point exception handler.",
)
parser.add_argument(
    "-d",
    "--dimensions",
    default=3,
    choices = [2, 3],
    type=int,
    help="Number of dimensions to use: 2 or 3",
)
parser.add_argument(
    "--enable-mlir",
    dest="enable_mlir",
    action="store_true",
    help="Enable MLIR kernel compilation support",
)
parser.add_argument(
    "--enable-cpp",
    dest="enable_cpp",
    action="store_true",
    help="Enable CPP kernels",
)
parser.add_argument(
    "-j",
    default=-1,
    type=int,
    help="Number of parallel builds to use",
)
args = parser.parse_args()

fv_rusanov_solver = CCZ4Solver_FV_GlobalAdaptiveTimeStepWithEnclaveTasking(
    name="FVRusanov",
    patch_size=args.patch_size,
    min_volume_h=0.001,  # max_cell_size -> arbitrary value
    max_volume_h=0.001,  # min_cell_size -> arbitrary value
    pde_terms_without_state=True,
)

project = exahype2.Project(
    namespace=["benchmarks", "exahype2", "kernelbenchmarks"],
    project_name="KernelBenchmarksFVRusanovDSL",
    directory=".",
    executable=f"KernelBenchmarksFVRusanovDSL.{args.build_mode}",
)
fv_rusanov_solver.add_all_solver_constants()
project.add_solver(fv_rusanov_solver)

project.set_global_simulation_parameters(
    dimensions=args.dimensions,
    size=[1.0] * args.dimensions,
    offset=[0.0] * args.dimensions,
    min_end_time=0.1,
    max_end_time=0.1,
    first_plot_time_stamp=0.0,
    time_in_between_plots=0.0,
    periodic_BC=[False] * args.dimensions
)

project.set_Peano4_installation("../../../../", mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=True)

accuracy = args.accuracy
if accuracy < 0:
    import numpy
    accuracy = default = numpy.finfo(float).eps
project.constants.export_constexpr_with_type("Accuracy", str(accuracy), "double")

project.constants.export_constexpr_with_type("NumberOfSamples", str(args.samples), "int")

formatted_num_patches = "{{{}}}".format(", ".join(str(val) for val in args.num_patches))
project.constants.export_const_with_type("NumberOfPatchesToStudy", str(formatted_num_patches), "tarch::la::Vector<%s, int>" % len(args.num_patches))

project.constants.export_constexpr_with_type("NumberOfLaunchingThreads", str(args.num_threads), "int")

if args.fpe:
    project.constants.export_boolean("EnableFPE", True)
else:
    project.constants.export_boolean("EnableFPE", False)

if args.cpu == False and args.gpu == False and args.mlir_gpu == False:
    project.constants.export_boolean("AssessHostKernels", True)
    project.constants.export_boolean("AssessDeviceKernels", True)
    project.constants.export_boolean("AssessMLIRDeviceKernels", False)
else:
    project.constants.export_boolean("AssessHostKernels", True if args.cpu else False)
    project.constants.export_boolean("AssessDeviceKernels", True if args.gpu else False)
    project.constants.export_boolean("AssessMLIRDeviceKernels", True if args.mlir_gpu else False)

if args.serial == False and args.parallel == False:
    project.constants.export_boolean("AssessSerialKernels", True)
    project.constants.export_boolean("AssessParallelKernels", True)
else:
    project.constants.export_boolean("AssessSerialKernels", True if args.serial else False)
    project.constants.export_boolean("AssessParallelKernels", True if args.parallel else False)

project.constants.export_boolean("EvaluateFlux", False)
project.constants.export_boolean("EvaluateNonconservativeProduct", True)
project.constants.export_boolean("EvaluateSource", True)
project.constants.export_boolean("EvaluateMaximumEigenvalueAfterTimeStep", True if args.eval_eig else False)  # Reduction

project.constants.define_value("GAMMA", str(1.0))

# MLIR Integration Support
if args.enable_mlir:
    print(f"Enabling MLIR kernel compilation support for {args.dimensions}D...")

    # Set environment variable for dimension detection
    import os
    os.environ['MLIR_DIMENSIONS'] = str(args.dimensions)

    # Add MLIR-specific files and flags
    project.output.makefile.add_cpp_file("MLIRbridge.cpp")
    project.output.makefile.add_CXX_flag("-DUSE_MLIR")
    #project.output.makefile.add_CXX_flag("-DMLIR_KERNEL_ENABLED")
    project.output.makefile.add_CXX_flag("-DSharedOMP")
    
    # NOTE: As we are using LLVM toolings for MLIR, we link against LLVM's OpenMP library  
    project.output.makefile.add_linker_flag("-lomp")   
    project.output.makefile.add_linker_flag("-lomptarget") 
    if args.mlir_gpu:
      print("Enabling MLIR GPU kernel compilation support")
      project.output.makefile.add_cpp_file("MLIRbridgeCUDAfunctions.cpp")
      project.output.makefile.d["MLIR_GPU"] = True
      project.output.makefile.add_linker_flag("-lcuda")
      project.output.makefile.add_linker_flag("-lcudart")
      project.output.makefile.add_linker_flag("-ldl")
      project.output.makefile.add_linker_flag("-pthread")
      project.output.makefile.add_linker_flag("-lmlir_cuda_runtime")
      CUDA_HOME = os.path.dirname(subprocess.check_output("which nvcc", shell=True).decode().strip()) 
      project.output.makefile.add_linker_flag(f"-L{CUDA_HOME}/../targets/sbsa-linux/lib")
      project.output.makefile.add_CXX_flag("-DMLIR_GPU")
      # NOTE: The current MLIR GPU linearization and lowering pipeline uses the base MLIR
      #       but may use the OMP version in future. Update here, as appropriate
      project.output.makefile.d["MLIR_SOURCES_GPU"] = f"rusanov{args.dimensions}d.mlir"
      project.output.makefile.d["MLIR_LL_FILE_GPU"] = f"rusanov{args.dimensions}d_gpu.ll" 
      project.output.makefile.d["MLIR_OBJECT_GPU"] = f"rusanov{args.dimensions}d_mlir_gpu.o"    
        
    project.output.makefile.d["MLIR_ENABLED"] = True
    project.output.makefile.d["SOLVER_NAME"] = "rusanov"
    project.output.makefile.d["MLIR_PHYSICS_FILE"] = "ccz4_physics.mlir"
    project.output.makefile.d["MLIR_SOURCES"] = f"rusanov{args.dimensions}d.mlir"
    project.output.makefile.d["MLIR_LL_FILE"] = f"rusanov{args.dimensions}d.ll" 
    project.output.makefile.d["MLIR_OBJECT"] = f"rusanov{args.dimensions}d_mlir.o"
    project.output.makefile.d["MLIR_SOURCES_OMP"] = f"rusanov{args.dimensions}d_omp.mlir"
    project.output.makefile.d["MLIR_LL_FILE_OMP"] = f"rusanov{args.dimensions}d_omp.ll" 
    project.output.makefile.d["MLIR_OBJECT_OMP"] = f"rusanov{args.dimensions}d_mlir_omp.o"

    # Export MLIR enabled constant for C++ code
    #project.constants.export_boolean("MLIRKernelEnabled", True)
#else:
    #project.constants.export_boolean("MLIRKernelEnabled", False)

if args.enable_cpp:
    project.output.makefile.add_CXX_flag("-DUSE_CPP_KERNELS")


benchmark_main_file = "../../kernel-benchmarks/KernelBenchmarksFVRusanovDSL-main.cpp"
os.system("cp {} KernelBenchmarksFVRusanovDSL-main.cpp".format(benchmark_main_file))

project.output.makefile.add_CXX_flag("-DGRAV=9.81")

fv_rusanov_solver.add_makefile_parameters(
    project, "../../../../applications/exahype2/ccz4"
)
project.generate(throw_away_data_after_generation=False)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True, number_of_parallel_builds=args.j)

print(project)
print(fv_rusanov_solver)

