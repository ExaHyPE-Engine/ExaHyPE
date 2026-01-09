# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import peano4
import exahype2

size = [200.0, 200.0, 200.0]
offset = [-100, -100, -100]
end_time = 1.0
plot_interval = 0.0
# periodic_BC = [True, True, True]
periodic_BC = [False, False, False]

floatparams = {
    "GLMc0": 1.5,
    "GLMc": 1.2,
    "GLMd": 2.0,
    "GLMepsA": 1.0,
    "GLMepsP": 1.0,
    "GLMepsD": 1.0,
    "itau": 1.0,
    "k1": 0.1,
    "k2": 0.0,
    "k3": 0.5,
    "eta": 1.0,
    "f": 0.75,
    "g": 0.0,
    "xi": 1.0,
    "e": 1.0,
    "c": 1.0,
    "mu": 0.2,
    "ds": 1.0,
    "sk": 0.0,
    "bs": 0.0,
    "domain_r": 0.5,
    "smoothing": 0.0,
    "KOSigma": 8.0,
}

intparams = {
    "BBHType": 2,
    "LapseType": 0,
    "tp_grid_setup": 0,
    "swi": 99,
    "ReSwi": 1,
    "SO": 0,
}

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="ExaHyPE 2 - CCZ4 Finite Volume Benchmarking Script"
    )
    parser.add_argument(
        "-md",
        "--min-depth",
        type=float,
        default=3,
        help="Determines maximum size of a single cell on each axis.",
    )
    parser.add_argument(
        "-amr",
        "--amr-levels",
        type=int,
        default=0,
        help="Number of AMR grid levels on top of max. size of a cell.",
    )
    parser.add_argument(
        "-ps",
        "--patch-size",
        type=int,
        default=8,
        help="Number of finite volumes per axis (dimension) per patch.",
    )

    for k, v in floatparams.items():
        parser.add_argument(
            "--{}".format(k),
            dest="CCZ4{}".format(k),
            type=float,
            default=v,
            help="default: %(default)s",
        )
    for k, v in intparams.items():
        parser.add_argument(
            "--{}".format(k),
            dest="CCZ4{}".format(k),
            type=int,
            default=v,
            help="default: %(default)s",
        )

    args = parser.parse_args()

    class CCZ4Solver(exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep):
        def __init__(
            self, name, patch_size, min_volume_h, max_volume_h, cfl, domain_r, KOSig
        ):
            unknowns = {
                "G": 6,  # 0-5
                "K": 6,  # 6-11
                "theta": 1,  # 12
                "Z": 3,  # 13-15
                "lapse": 1,  # 16
                "shift": 3,  # 17-19
                "b": 3,  # 20-22
                "dLapse": 3,  # 23-25
                "dxShift": 3,  # 26-28
                "dyShift": 3,  # 29-31
                "dzShift": 3,  # 32-34
                "dxG": 6,  # 35-40
                "dyG": 6,  # 41-46
                "dzG": 6,  # 47-52
                "traceK": 1,  # 53
                "phi": 1,  # 54
                "P": 3,  # 55-57
                "K0": 1,  # 58
            }

            number_of_unknowns = sum(unknowns.values())

            self._my_user_includes = """
#include "../../../applications/ccz4/CCZ4Kernels.h"
"""
            super(CCZ4Solver, self).__init__(
                name=name,
                patch_size=patch_size,
                unknowns=number_of_unknowns,
                auxiliary_variables=0,
                min_volume_h=min_volume_h,
                max_volume_h=max_volume_h,
                time_step_relaxation=cfl,
            )

            self._patch_size = patch_size
            self._domain_r = domain_r

            self.set_implementation(
                boundary_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
                refinement_criterion=exahype2.solvers.PDETerms.User_Defined_Implementation,
                flux=exahype2.solvers.PDETerms.None_Implementation,
                ncp="""
    double gradQSerialised[NumberOfUnknowns * 3];
    for (int i = 0; i < NumberOfUnknowns; i++) {
      gradQSerialised[i + 0 * NumberOfUnknowns]      = 0.0;
      gradQSerialised[i + 1 * NumberOfUnknowns]      = 0.0;
      gradQSerialised[i + 2 * NumberOfUnknowns]      = 0.0;
      gradQSerialised[i + normal * NumberOfUnknowns] = deltaQ[i];
    }
    applications::exahype2::ccz4::ncp(
      BTimesDeltaQ,
      Q,
      gradQSerialised,
      normal,
      CCZ4LapseType,
      CCZ4ds,
      CCZ4c,
      CCZ4e,
      CCZ4f,
      CCZ4bs,
      CCZ4sk,
      CCZ4xi,
      CCZ4mu,
      CCZ4SO
    );
""",
                source_term="""
    std::memset(S, 0, NumberOfUnknowns * sizeof(double));
    applications::exahype2::ccz4::source(
      S,
      Q,
      CCZ4LapseType,
      CCZ4ds,
      CCZ4c,
      CCZ4e,
      CCZ4f,
      CCZ4bs,
      CCZ4sk,
      CCZ4xi,
      CCZ4itau,
      CCZ4eta,
      CCZ4k1,
      CCZ4k2,
      CCZ4k3,
      CCZ4SO
    );
""",
                max_eigenvalue="""
    const double np = 1.0;
#if defined(CCZ4EINSTEIN)
    const double qmin  = std::min({Q[0], Q[3], Q[5]});
    const double alpha = std::max({1.0, Q[16]}) * std::max({1.0, pow(Q[54], 1.0 / np)}) / std::sqrt(qmin);
#else
    const double alpha = 1.0;
#endif

    constexpr double sqrtwo = 1.4142135623730951;
    // NOTE parameters are stored in abstract superclass
    const double tempA = alpha * std::max({sqrtwo, CCZ4e, CCZ4ds, CCZ4GLMc / alpha, CCZ4GLMd / alpha});
    const double tempB = Q[17 + normal]; // DOT_PRODUCT(Q(18:20),nv(:))
    double       tem   = std::max({1.0, std::abs(-tempA - tempB), std::abs(tempA - tempB)});
    bool         flag  = false;
    return tem;
                """,
            )

            self.postprocess_updated_patch = """
    int index = 0;
    for (int i = 0; i < {{NUMBER_OF_VOLUMES_PER_AXIS}} * {{NUMBER_OF_VOLUMES_PER_AXIS}} * {{NUMBER_OF_VOLUMES_PER_AXIS}}; i++) {
      applications::exahype2::ccz4::enforceCCZ4constraints(QOut + index);
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
"""

    project = exahype2.Project(
        namespace=["benchmarks", "exahype2", "ccz4"],
        project_name="GaugeWave",
        directory=".",
        executable="GaugeWaveFV",
    )

    max_h = 1.1 * min(size) / (3.0**args.min_depth)
    min_h = max_h * 3.0 ** (-args.amr_levels)
    my_solver = CCZ4Solver(
        "CCZ4",
        args.patch_size,
        min_h,
        max_h,
        0.1,
        args.CCZ4domain_r,
        args.CCZ4KOSigma,
    )

    for k, v in intparams.items():
        intparams.update({k: eval("args.CCZ4{}".format(k))})
    for k, v in floatparams.items():
        floatparams.update({k: eval("args.CCZ4{}".format(k))})

    solverconstants = ""
    for k, v in floatparams.items():
        solverconstants += "static constexpr double {} = {};\n".format(
            "CCZ4{}".format(k), v
        )
    for k, v in intparams.items():
        solverconstants += "static constexpr int {} = {};\n".format(
            "CCZ4{}".format(k), v
        )
    my_solver.add_solver_constants(solverconstants)
    my_solver.add_user_solver_includes(
        """
    #include "CCZ4Kernels.h"
    #include <cstring>
    """
    )

    project.add_solver(my_solver)
    project.set_global_simulation_parameters(
        dimensions=3,
        size=size,
        offset=offset,
        min_end_time=end_time,
        max_end_time=end_time,
        first_plot_time_stamp=0.0,
        time_in_between_plots=plot_interval,
        periodic_BC=periodic_BC,
    )

    if plot_interval > 0.0:
        project.set_output_path("solutions")
    project.set_build_mode(peano4.output.CompileMode.Release)
    project.set_load_balancer("new ::exahype2::LoadBalancingConfiguration")
    peano4_project = project.generate_Peano4_project(verbose=False)
    peano4_project.output.makefile.add_header_search_path(
        "../../../applications/ccz4/"
    )

    peano4_project.output.makefile.add_CXX_flag("-DCCZ4EINSTEIN")

    peano4_project.output.makefile.add_cpp_file("CCZ4.cpp")
    peano4_project.output.makefile.add_cpp_file(
        "../../../applications/ccz4/CCZ4Kernels.cpp"
    )

    peano4_project.generate(throw_away_data_after_generation=True)
    peano4_project.build(make_clean_first=True)
