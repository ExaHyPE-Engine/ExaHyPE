# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

order = 3
max_h = 1.0 / 9.0
min_h = 0.9 / 27.0  # 0.9 / 81.0
plot_interval = 0.0  # 0.01
end_time = 0.1

polynomials = exahype2.solvers.aderdg.Polynomials.Gauss_Legendre

project = exahype2.Project(
    ["tests", "exahype2", "aderdg"], ".", executable="ADERDG-AMR"
)

solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="SWE",
    order=order,
    min_cell_h=min_h,
    max_cell_h=max_h,
    time_step_relaxation=0.8,
    unknowns=4,
    auxiliary_variables=0,
    refinement_criterion=exahype2.solvers.PDETerms.User_Defined_Implementation,
    flux=exahype2.solvers.PDETerms.User_Defined_Implementation,
    ncp=exahype2.solvers.PDETerms.User_Defined_Implementation,
)

# solver._split_sweeps = True
project.add_solver(solver)

project.set_output_path("solutions")

project.set_global_simulation_parameters(
    dimensions=2,
    offset=[0.0, 0.0],
    size=[1.0, 1.0],
    min_end_time=end_time,
    max_end_time=end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=plot_interval,
    periodic_BC=[False, False],
)

project.set_load_balancing(
    "toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates",
    "new ::exahype2::LoadBalancingConfiguration(0.98)",
)
project.set_Peano4_installation("../../../", peano4.output.CompileMode.Release)
project = project.generate_Peano4_project("False")
#project.set_fenv_handler("FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW")
project.output.makefile.add_cpp_file("SWE.cpp")

project.build(make_clean_first=True)

print(solver)
print(project)
