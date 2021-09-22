# # Shallow water equations with ExaHyPE 2 and GeoClaw
#
# A solver of the SWE equations that relies on ClawPack's point-wise SWE solver and GeoClaw.

# To run this code, you need a working ClawPack installation with GeoClaw in addition to your Peano/ExaHyPE installation. Installation instructions are available on the [ClawPack website](http://www.clawpack.org/installing.html).


import peano4
import exahype2
import exahype2.sympy
import exahype2.postprocessing


def removeOutput():
    import os
    for f in os.listdir("."):
        if f.endswith(".peano-patch-file") or f.endswith(".vtu") or f.startswith("output"):
            os.remove(f)

def checkPath(path):
    import os
    if not os.path.exists(path):
        raise Exception("Path {} does not exist".format(path))

def selectSolver(solver):
    if solver == "rpn2":
        return ["rpn2_ptwise_geoclaw.f90"]
    else:
        raise Exception("Solver {} is not known".format(solver))


if __name__ == "__main__":

    import optparse, os, sys
    op = optparse.OptionParser(usage=__doc__)
    op.add_option("-v", "--debug", dest="DEBUG", action="store_true", default=False, help="Turn on some debug messages")
    op.add_option("-o", dest="OUTDIR", default="tune", help="Output directory (default: %default)")
    op.add_option("-c", dest="CLAWPACK", default=None, help="Clawpack path (default: %default)")
    op.add_option("-e", "--et", dest="ENDTIME", type=float, default=0.01, help="Simulation end time (default: %default)")
    op.add_option("--ps", dest="PATCHSIZE", type=int, default=40, help="Patch size (default: %default)")
    op.add_option("--maxh", dest="MAXH", type=float, default=1./81, help="AMR max h (default: %default)")
    op.add_option("--minh", dest="MINH", type=float, default=1./81, help="AMR min h (default: %default)")
    op.add_option("-l", "--lbg", dest="LBQ", type=float, default=0.9, help="Load balancing quality (default: %default)")
    op.add_option("-t", "--trees", dest="MAXTREES", type=int, default=sys.maxsize, help="Max number of trees (default: %default)")
    op.add_option("-p", dest="PEANO", default=None, help="Peano path (default: %default)")
    op.add_option("-s", dest="SOLVER", default="rpn2", help="Clawpack solver (default: %default)")
    op.add_option("-m", dest="BUILDMODE", default="release", help="Buildmode (default: %default)")
    op.add_option("-j", dest="J", default=6, type=int, help="Buildmode (default: %default)")
    opts, args = op.parse_args()

    if opts.CLAWPACK is None:
        clawpack_path = "/home/marion/workspace/Projects/ExaClaw/clawpack/src/clawpack-v5.7.1"
    else:
        clawpack_path = opts.CLAWPACK
    checkPath(clawpack_path)
    checkPath(os.path.join(clawpack_path, "geoclaw/src/2d/shallow/geoclaw_module.f90"))

    if opts.PEANO is None:
        peano_path = "/home/marion/workspace/Projects/ExaClaw/Peano"
    else:
        peano_path = opts.PEANO
    checkPath(peano_path)

    solverfiles = selectSolver(opts.SOLVER)

    assert(opts.BUILDMODE in ["release", "trace", "asserts"])

    # os.system("make clean")


    # The first thing to do in any ExaHyPE 2 application is to create an ExaHyPE
    # project. We have to tell it exactly what namespace we want to use and we have
    # to give it a name. There are a few more options that we omit here, i.e. we
    # use the defaults.

    project = exahype2.Project(["examples", "exahype2", "swe"], "exaclaw")

    # ## The simplest ExaClaw Finite Volumes solver
    #
    # Our first step is to use a Finite Volumes solver. We refer them as ExaClaw
    # solvers. ExaClaw is the research project, where we promised to marry ExaHyPE
    # and ClawPack's Riemann solvers. The actual softare is still ExaHyPE +
    # ClawPack. The ClawPack solver is similar to the Euler one. Instead of a
    # (generic) flux and eigenvalue, it needs the name of the ClawPack Riemann
    # solver plus the Fortran file with the implementation. The latter actually can
    # be a set of Fortran files for more complex solvers.

    my_unknowns = 3  # height and two velocities
    my_auxiliary_variables = 1  # bathymetry

    my_patch_size = opts.PATCHSIZE
    my_time_step_size = 0.000001 # keep this in, even though we do adaptive time stepping, for snapshot frequency
    my_max_h = opts.MAXH
    my_min_h = opts.MINH

    end_time = opts.ENDTIME


    my_solver = exahype2.solvers.fv.clawpack.GlobalFixedTimeStep(
      "SWE",
      my_patch_size,
      my_unknowns,
      my_auxiliary_variables,
      my_min_h,
      my_max_h,
      my_time_step_size,
      opts.SOLVER,
      solverfiles,
      discriminate_normal=True
    )

    my_solver.set_pointwise_constraint(
       """
         // CONSTRAINT
         if(Q[3] > 0){
           Q[0] = 0.0;
           Q[1] = 0.0;
           Q[2] = 0.0;
          }
       """
    )




    # my_solver = exahype2.solvers.fv.PointWiseClawPackAdaptiveTimeStepSize(
        # name="SWE",
        # patch_size=my_patch_size,
        # unknowns=my_unknowns,
        # auxiliary_variables=my_auxiliary_variables,
        # min_h=my_min_h,
        # max_h=my_max_h,
        # #  time_step_size=my_time_step_size,
        # clawpack_Riemann_solveggr=opts.SOLVER,
        # Riemann_solver_implementation_files=solverfiles,
        # discriminate_normal=True,
    # )
    project.add_solver(my_solver)


    # The two fundamental pieces of info we still have to feed into the solver are
    # proper boundary and initial conditions. I use some factory/symbolic
    # mechanisms here for the boundary conditions. For the initial condition, I
    # could just omit the parameter. I'd then get a file Acoustic.cpp with an empty
    # function that I have to implement. As this function will be close to trivial
    # once more, I directly inject the C++ code here. So I don't have to touch any
    # C++ code while I run the Juptyer notebook.

    pde = exahype2.sympy.PDE(unknowns=my_unknowns, auxiliary_variables=my_auxiliary_variables, dimensions=2)

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



    # ## Configure the global domain
    #
    # We next configure our global domain, i.e. we specify the dimensions of the
    # computational domain $\Omega $, we specify how long the simulation shall run
    # and how often we want it to dump its data. The dumps will later be used to
    # produce screenshots of the data (or to extract all kinds of properties).

    bmodes = {"release" :peano4.output.CompileMode.Release, "trace":peano4.output.CompileMode.Trace, "asserts":peano4.output.CompileMode.Asserts}
    build_mode = bmodes[opts.BUILDMODE]


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

    project.set_load_balancing("toolbox::loadbalancing::RecursiveSubdivision","(new ::exahype2::LoadBalancingConfiguration({},0,{}))".format(opts.LBQ, opts.MAXTREES) )
    peano4_project = project.generate_Peano4_project()


    # A "big" difference to Euler is that we now have some external Fortran files.
    # Usually, we take the default settings when we invoke the Fortran compiler.
    # Default is the stuff given to us by Peano's configure. There's however the
    # opportunity to add further flags manually. I use this here to inject debug
    # info.

    peano4_project.output.makefile.add_Fortran_flag("-g")

    # FLIBS = ["ifcore", "stdc++", "m"]
    FLIBS = ["gfortran", "stdc++", "m"]
    for fl in FLIBS: peano4_project.output.makefile.add_library(fl)

    FMODS = ["geoclaw/src/2d/shallow/utility_module.f90", "amrclaw/src/2d/amr_module.f90", "geoclaw/src/2d/shallow/geoclaw_module.f90"]
    for fm in FMODS: peano4_project.output.makefile.add_Fortran_module(os.path.join(clawpack_path, fm))

    peano4_project.output.makefile.add_header_search_path( os.path.join(clawpack_path, "amrclaw/src/2d/") )
    peano4_project.output.makefile.add_header_search_path( os.path.join(clawpack_path, "geoclaw/src/2d/shallow/") )
    peano4_project.output.makefile.add_Fortran_file( os.path.join(clawpack_path, "riemann/src/geoclaw_riemann_utils.f") )
    peano4_project.output.makefile.add_Fortran_file( os.path.join(clawpack_path, "amrclaw/src/2d/opendatafile.f") )
    peano4_project.output.makefile.add_cpp_file("TopologyParser.cpp")
    peano4_project.generate(throw_away_data_after_generation=False)


    peano4_project.build(make_clean_first=True, number_of_parallel_builds=opts.J)
