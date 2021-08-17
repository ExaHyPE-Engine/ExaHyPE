# # Shallow water equations with ExaHyPE 2 and GeoClaw
#
# A solver of the SWE equations that relies on ClawPack's point-wise SWE solver and GeoClaw.

# To run this code, you need a working ClawPack installation with GeoClaw in addition to your Peano/ExaHyPE installation. Installation instructions are available on the [ClawPack website](http://www.clawpack.org/installing.html).


import os
import peano4
import exahype2
import exahype2.sympy
import exahype2.postprocessing


clawpack_path = "/ddn/data/cvxs42/clawpack_src/clawpack-v5-8-0"

peano_path = "/ddn/data/cvxs42/Peano"


output_files = [
    f
    for f in os.listdir(".")
    if f.endswith(".peano-patch-file") or f.endswith(".vtu") or f.startswith("output")
]
for f in output_files:
    os.remove(f)

os.system("make clean")


# The first thing to do in any ExaHyPE 2 application is to create an ExaHyPE project. We have to tell it exactly what namespace we want to use and we have to give it a name. There are a few more options that we omit here, i.e. we use the defaults.

project = exahype2.Project(["examples", "exahype2", "swe"], "exaclaw")

# ## The simplest ExaClaw Finite Volumes solver
#
# Our first step is to use a Finite Volumes solver. We refer them as ExaClaw solvers. ExaClaw is the research project, where we promised to marry ExaHyPE and ClawPack's Riemann solvers. The actual softare is still ExaHyPE + ClawPack. The ClawPack solver is similar to the Euler one. Instead of a (generic) flux and eigenvalue, it needs the name of the ClawPack Riemann solver plus the Fortran file with the implementation. The latter actually can be a set of Fortran files for more complex solvers.

my_patch_size = 40
my_unknowns = 3  # height and two velocities
my_auxiliary_variables = 1  # bathymetry
my_time_step_size = 0.0001 # keep this in, even though we do adaptive time stepping, for snapshot frequency
my_max_h = 1.0 / 9.0
my_min_h = 1.0 / 27.0

end_time = 0.001

my_solver = exahype2.solvers.fv.PointWiseClawPackAdaptiveTimeStepSize(
    name="SWE",
    patch_size=my_patch_size,
    unknowns=my_unknowns,
    auxiliary_variables=my_auxiliary_variables,
    min_h=my_min_h,
    max_h=my_max_h,
    #  time_step_size=my_time_step_size,
    clawpack_Riemann_solver="rpn2",
    Riemann_solver_implementation_files=["rpn2_ptwise_geoclaw.f90"],
    discriminate_normal=True,
)
project.add_solver(my_solver)


# The two fundamental pieces of info we still have to feed into the solver are proper boundary and initial conditions. I use some factory/symbolic mechanisms here for the boundary conditions. For the initial condition, I could just omit the parameter. I'd then get a file Acoustic.cpp with an empty function that I have to implement. As this function will be close to trivial once more, I directly inject the C++ code here. So I don't have to touch any C++ code while I run the Juptyer notebook.

pde = exahype2.sympy.PDE(unknowns=3, auxiliary_variables=1, dimensions=2)

my_solver.set_implementation(
    boundary_conditions=pde.implementation_of_homogeneous_Neumann_BC(),
    refinement_criterion=exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
    initial_conditions="""
      static TopologyParser parser("topology/etopo10min120W60W60S0S.asc", "topology/dtopo_usgs100227.tt3");
      float z;
      float d;

      z = parser.sampletopology(volumeCentre(0), volumeCentre(1));
      d = parser.sampledisplacement(volumeCentre(0), volumeCentre(1), 1.0);
  
      //if(z < -200){z = -200;}
  
      if(z < 0){
        Q[0] = -z + d;
       //Q[0] = volumeCentre(0) > 0.3 && volumeCentre(0) < 0.4 ? 20-z : -z;
       //Q[0] = (volumeCentre(0)-0.5)*(volumeCentre(0)-0.5) + (volumeCentre(1)-0.5)*(volumeCentre(1)-0.5) < 0.1*0.1 ? 10-z : -z;
      }else{
        Q[0] = 0.0;
      }
      Q[1] = 0.0;
      Q[2] = 0.0;
      Q[3] = z; // bathymetry
    """,
)

my_solver.set_postprocess_updated_patch_kernel_point_wisely(
   """
     if(Q[3] > 0){
       Q[0] = 0.0;
       Q[1] = 0.0;
       Q[2] = 0.0;
      }
   """
)


# ## Configure the global domain
#
# We next configure our global domain, i.e. we specify the dimensions of the computational domain $\Omega $, we specify how long the simulation shall run and how often we want it to dump its data. The dumps will later be used to produce screenshots of the data (or to extract all kinds of properties).

# build_mode = peano4.output.CompileMode.Trace
# build_mode = peano4.output.CompileMode.Asserts
build_mode = peano4.output.CompileMode.Release

dimensions = 2

project.set_global_simulation_parameters(
    dimensions,  # dimensions
    [0.0, 0.0],
    [1.0, 1.0],
    end_time,  # end time
    0.0,
    my_time_step_size,  # snapshots
)


# ## Generate a Peano 4 project
#
# This is exactly the same as for Euler: We ask the ExaHyPE 2 frontend to give us a Peano project.

project.set_Peano4_installation("../../..", build_mode)

peano4_project = project.generate_Peano4_project()


# A "big" difference to Euler is that we now have some external Fortran files. Usually, we take the default settings when we invoke the Fortran compiler. Default is the stuff given to us by Peano's configure. There's however the opportunity to add further flags manually. I use this here to inject debug info.


peano4_project.output.makefile.add_Fortran_flag("-g")
#peano4_project.output.makefile.add_library("gfortran")
peano4_project.output.makefile.add_library("ifcore")
peano4_project.output.makefile.add_library("stdc++")
peano4_project.output.makefile.add_library("m")
peano4_project.output.makefile.add_Fortran_module(
    clawpack_path + "/geoclaw/src/2d/shallow/utility_module.f90"
)
peano4_project.output.makefile.add_Fortran_module(
    clawpack_path + "/amrclaw/src/2d/amr_module.f90"
)
peano4_project.output.makefile.add_Fortran_module(
    clawpack_path + "/geoclaw/src/2d/shallow/geoclaw_module.f90"
)
peano4_project.output.makefile.add_header_search_path(
    clawpack_path + "/amrclaw/src/2d/"
)
peano4_project.output.makefile.add_header_search_path(
    clawpack_path + "/geoclaw/src/2d/shallow/"
)
peano4_project.output.makefile.add_Fortran_file(
    clawpack_path + "/riemann/src/geoclaw_riemann_utils.f"
)
peano4_project.output.makefile.add_Fortran_file(
    clawpack_path + "/amrclaw/src/2d/opendatafile.f"
)
peano4_project.output.makefile.add_cpp_file("TopologyParser.cpp")
peano4_project.generate(throw_away_data_after_generation=False)


parallel_builds = (
    6
)  # Without any arguments, the build process will grab all of your cores.
peano4_project.build(make_clean_first=True, number_of_parallel_builds=parallel_builds)

# os.system('make')


# ## Run the code 
#
# Once we have the Peano 4 project set up and built, we obtain an executable which is standalone. It comprises all the parameters we have set above. So we can either invoke this file on the command line or we can call it through the notebook. Depending on your setup you might want to call the code with a prefix. If you configured Peano with MPI, e.g., you might have to use the mpirun prefix.
#
# Please note that we pipe the terminal output into a file. This way, we can postprocess the data in the next step.


#peano4_project.run(
#    [], prefix=["mpirun", "-n", "4"], pipefile="output.txt", rebuild_if_required=False
#)





