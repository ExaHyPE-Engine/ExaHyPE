import os
import sys

import exahype2

sys.path.insert(0, os.path.abspath("../../../../applications/exahype2/swe"))

from SWE import SWE

if __name__ == "__main__":
    swe = SWE()
    swe._available_solvers = {
        "RusanovGlobalAdaptiveEnclaveFV": exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking,
    }

    my_parser = swe.setup_parser()
    my_parser.add_argument(
        "-t",
        "--num-threads",
        type=int,
        help="Number of launching threads",
    )
    my_parser.add_argument(
        "-p",
        "--num-patches",
        nargs="+",
        type=int,
        help="Number of patches to study",
    )
    my_parser.add_argument(
        "-samples",
        "--samples",
        type=int,
        help="Number of samples per measurement",
    )
    my_parser.add_argument(
        "-a",
        "--accuracy",
        type=float,
        help="Floating point accuracy to which the different kernel variants have to match (absolute). Pass in 0 to disable correctness check. Pass in values < 0 to use machine epsilon (default).",
    )
    my_parser.add_argument(
        "-e",
        "--eval-eig",
        action="store_true",
        help="Evaluate max. eigenvalue",
    )
    my_parser.add_argument(
        "-cpu",
        "--cpu",
        action="store_true",
        help="Assess host kernels",
    )
    my_parser.add_argument(
        "-gpu",
        "--gpu",
        action="store_true",
        help="Assess device kernels",
    )
    my_parser.set_defaults(
        solver="RusanovGlobalAdaptiveEnclaveFV",
        width="[1.0, 1.0]",
        patch_size=16,
        stateless=True,
        number_of_snapshots=0,
        peano_dir="../../../../",
        num_threads=1,
        num_patches=[32, 64, 128, 256, 512, 1024, 2048],
        samples=10,
        accuracy=0.0,
    )
    my_args = my_parser.parse_args()

    my_solver = swe.setup_solver(my_args)
    my_solver.set_implementation(
        initial_conditions="",
        boundary_conditions=exahype2.solvers.PDETerms.Empty_Implementation,
        refinement_criterion=exahype2.solvers.PDETerms.Empty_Implementation,
        flux="::applications::exahype2::swe::flux(Q, faceCentre, volumeH, t, dt, normal, F);",
        ncp="::applications::exahype2::swe::nonconservativeProduct(Q, deltaQ, faceCentre, volumeH, t, dt, normal, BTimesDeltaQ);",
        eigenvalues="""
  double L[3]{0.0};
  eigenvalues(Q, faceCentre, volumeH, t, dt, normal, L);
  return std::max({std::abs(L[0]), std::abs(L[1]), std::abs(L[2])});
""",
        # There is a bug in the code generator: When set to 'None_Implementation', an offloadable function is still generated, also 'Empty_Implementation' does not work.
        # TODO: What is the difference between 'None_Implementation' and 'Empty_Implementation'?
        source_term="",
    )
    my_solver.add_user_solver_includes(
        """
    #include "../../../../applications/exahype2/swe/SWE.h"
    """
    )

    my_project = swe.setup_project(my_args, my_solver)

    accuracy = my_args.accuracy
    if accuracy < 0:
        import numpy

        accuracy = default = numpy.finfo(float).eps
    my_project.constants.export_constexpr_with_type("Accuracy", str(accuracy), "double")
    my_project.constants.export_constexpr_with_type(
        "NumberOfSamples", str(my_args.samples), "int"
    )
    formatted_num_patches = "{{{}}}".format(
        ", ".join(str(val) for val in my_args.num_patches)
    )
    my_project.constants.export_const_with_type(
        "NumberOfPatchesToStudy",
        str(formatted_num_patches),
        "::tarch::la::Vector<%s, int>" % len(my_args.num_patches),
    )
    my_project.constants.export_constexpr_with_type(
        "NumberOfLaunchingThreads", str(my_args.num_threads), "int"
    )
    if my_args.fpe:
        my_project.constants.export_boolean("EnableFPE", True)
    else:
        my_project.constants.export_boolean("EnableFPE", False)
    if my_args.cpu == False and my_args.gpu == False:
        my_project.constants.export_boolean("AssessHostKernels", True)
        my_project.constants.export_boolean("AssessDeviceKernels", True)
    else:
        my_project.constants.export_boolean(
            "AssessHostKernels", True if my_args.cpu else False
        )
        my_project.constants.export_boolean(
            "AssessDeviceKernels", True if my_args.gpu else False
        )
    my_project.constants.export_boolean("EvaluateFlux", True)
    my_project.constants.export_boolean("EvaluateNonconservativeProduct", True)
    my_project.constants.export_boolean("EvaluateSource", False)
    my_project.constants.export_boolean(
        "EvaluateMaximumEigenvalueAfterTimeStep", True if my_args.eval_eig else False
    )  # Reduction

    my_project.output.makefile.add_cpp_file("SWE-main.cpp")
    my_project.output.makefile.add_cpp_file(
        "../../../../applications/exahype2/swe/SWE.cpp"
    )

    swe.build(my_args, my_project)

    print(my_args)
    print(my_solver)
    print(my_project)
