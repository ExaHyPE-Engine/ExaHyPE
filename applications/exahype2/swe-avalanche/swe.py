import os
import sys

#
# We use our code Python generator to generate additional glue code.
# The generator is located in the /Peano/python folder and needs to therefore added to PYTHONPATH.
# You can either set PYTHONPATH on the command line yourself or insert it here in the script.
#
sys.path.insert(0, os.path.abspath("../../../python"))

#
# We import the required modules for this project.
# We always need the 'peano4' module as this is our core project.
# Since we are creating an 'ExaHyPE' application, we additionally
# need to import the 'exahype2' module.
#
import peano4
import exahype2

#
# We first create a new ExaHyPE2 project.
# For this, we specify the (nested) namespaces, the name of our main file and our executable name.
#
my_project = exahype2.Project( ["applications", "exahype2", "avalanche"], "avalanche", ".", executable="avalanche" )

#
# Add the Finite Volumes solver using named arguments.
# This is the way you can add further PDE terms.
# This requires the 'BlockStructured' toolbox and 'ExaHyPE' to be built.
#
my_solver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep(
  name                  = "SWESolver",
  patch_size            = 3,
  unknowns              = 3,
  auxiliary_variables   = 11,
  min_volume_h          = 0.55,
  max_volume_h          = 1.1,
  time_step_relaxation  = 0.5
)

#
# To see which variables (unknowns + auxiliary variables) we can visualise,
# let's add a plot description for the variables to our solver.
#
my_solver.plot_description = "h, hu, hv, b"

my_solver._preprocess_reconstructed_patch = """
        const int patchSize = """ + str( my_solver._patch.dim[0] ) + """;
        int aux_var=""" + str( my_solver._auxiliary_variables ) + """;
        int real_var=3;
        double volumeH = ::exahype2::fv::getVolumeLength(marker.h(),patchSize);

        // extrapolation
        tarch::la::Vector<Dimensions, int> topLeftCell  = {0, patchSize + 1};
        tarch::la::Vector<Dimensions, int> topRightCell = {
          patchSize + 1, patchSize + 1};
        tarch::la::Vector<Dimensions, int> bottomLeftCell  = {0, 0};
        tarch::la::Vector<Dimensions, int> bottomRightCell = {patchSize + 1, 0};
        
        tarch::la::Vector<Dimensions, int> topLeftCellN1 = topLeftCell;
        topLeftCellN1(0)++;
        tarch::la::Vector<Dimensions, int> topLeftCellN2 = topLeftCell;
        topLeftCellN2(1)--;

        tarch::la::Vector<Dimensions, int> topRightCellN1 = topRightCell;
        topRightCellN1(0)--;
        tarch::la::Vector<Dimensions, int> topRightCellN2 = topRightCell;
        topRightCellN2(1)--;

        tarch::la::Vector<Dimensions, int> bottomLeftCellN1 = bottomLeftCell;
        bottomLeftCellN1(0)++;
        tarch::la::Vector<Dimensions, int> bottomLeftCellN2 = bottomLeftCell;
        bottomLeftCellN2(1)++;

        tarch::la::Vector<Dimensions, int> bottomRightCellN1 = bottomRightCell;
        bottomRightCellN1(0)--;
        tarch::la::Vector<Dimensions, int> bottomRightCellN2 = bottomRightCell;
        bottomRightCellN2(1)++;

        const int topLeftCellSerialised = peano4::utils::dLinearised(topLeftCell, patchSize + 2);
        const int topRightCellSerialised = peano4::utils::dLinearised(
          topRightCell, patchSize + 2
        );

        const int bottomLeftCellSerialised = peano4::utils::dLinearised(
          bottomLeftCell, patchSize + 2
        );
        const int bottomRightCellSerialised = peano4::utils::dLinearised(
          bottomRightCell, patchSize + 2
        );

        const int topLeftCellN1Serialised = peano4::utils::dLinearised(
          topLeftCellN1, patchSize + 2
        );
        const int topLeftCellN2Serialised = peano4::utils::dLinearised(
          topLeftCellN2, patchSize + 2
        );

        const int topRightCellN1Serialised = peano4::utils::dLinearised(
          topRightCellN1, patchSize + 2
        );
        const int topRightCellN2Serialised = peano4::utils::dLinearised(
          topRightCellN2, patchSize + 2
        );

        const int bottomLeftCellN1Serialised = peano4::utils::dLinearised(
          bottomLeftCellN1, patchSize + 2
        );
        const int bottomLeftCellN2Serialised = peano4::utils::dLinearised(
          bottomLeftCellN2, patchSize + 2
        );

        const int bottomRightCellN1Serialised = peano4::utils::dLinearised(
          bottomRightCellN1, patchSize + 2
        );
        const int bottomRightCellN2Serialised = peano4::utils::dLinearised(
          bottomRightCellN2, patchSize + 2
        );

        for (int i = 0; i < real_var + aux_var; i++) {

          oldQWithHalo[(topLeftCellSerialised) * (real_var + aux_var) + i]
            = 0.5
              * (oldQWithHalo[(topLeftCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(topLeftCellN2Serialised) * (real_var + aux_var) + i]);
          oldQWithHalo[(topRightCellSerialised) * (real_var + aux_var) + i]
            = 0.5
              * (oldQWithHalo[(topRightCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(topRightCellN2Serialised) * (real_var + aux_var) + i]);

          oldQWithHalo[(bottomLeftCellSerialised) * (real_var + aux_var) + i]
            = 0.5
              * (oldQWithHalo[(bottomLeftCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(bottomLeftCellN2Serialised) * (real_var + aux_var) + i]);

          oldQWithHalo[(bottomRightCellSerialised) * (real_var + aux_var) + i]
            = 0.5
              * (oldQWithHalo[(bottomRightCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(bottomRightCellN2Serialised) * (real_var + aux_var) + i]);
        }

        // derivative calculation
        // central difference for single derivatives
        // central difference for double derivatives
        enum derivatives{u_x=1,u_y,u_xx,u_yy,u_xy,v_x,v_y,v_xx,v_yy,v_xy};
        enum unknowns{h=0,hu,hv};
        const int totalVariables = real_var+aux_var;

        for(int y = 1 ; y < patchSize + 1; y++){
          for(int x = 1; x < patchSize + 1;x++){
            const tarch::la::Vector<Dimensions, int> currentCell    = {x, y};
            const tarch::la::Vector<Dimensions, int> xLeftNeighbourCell = {x - 1, y};
            const tarch::la::Vector<Dimensions, int> xRightNeighbourCell = {x + 1, y};
            const tarch::la::Vector<Dimensions, int> yLeftNeighbourCell = {x, y - 1};
            const tarch::la::Vector<Dimensions, int> yRightNeighbourCell = {x, y + 1};
            const tarch::la::Vector<Dimensions, int> xLeftYLeftNeighbourCell = {x - 1, y - 1};
            const tarch::la::Vector<Dimensions, int> xLeftYRightNeighbourCell = {x - 1, y + 1};
            const tarch::la::Vector<Dimensions, int> xRightYLeftNeighbourCell = {x + 1, y - 1};
            const tarch::la::Vector<Dimensions, int> xRightYRightNeighbourCell = {x + 1, y + 1};



            const int currentCellSerialized = peano4::utils::dLinearised(
            currentCell, patchSize + 2
            );
            const int xLeftNeighbourCellSerialized = peano4::utils::dLinearised(
              xLeftNeighbourCell, patchSize + 2
            );
            const int xRightNeighbourCellSerialized = peano4::utils::dLinearised(
              xRightNeighbourCell, patchSize + 2
            );
            const int yLeftNeighbourCellSerialized = peano4::utils::dLinearised(
              yLeftNeighbourCell, patchSize + 2
            );
            const int yRightNeighbourCellSerialized = peano4::utils::dLinearised(
              yRightNeighbourCell, patchSize + 2
            );

            const int xLeftYLeftNeighbourCellSerialized = peano4::utils::dLinearised(
              xLeftYLeftNeighbourCell, patchSize + 2
            );

            const int xLeftYRightNeighbourCellSerialized = peano4::utils::dLinearised(
              xLeftYRightNeighbourCell, patchSize + 2
            );

            const int xRightYLeftNeighbourCellSerialized = peano4::utils::dLinearised(
              xRightYLeftNeighbourCell, patchSize + 2
            );

            const int xRightYRightNeighbourCellSerialized = peano4::utils::dLinearised(
              xRightYRightNeighbourCell, patchSize + 2
            );

            const double uCurrentCell = oldQWithHalo[currentCellSerialized*totalVariables+hu]/oldQWithHalo[currentCellSerialized*totalVariables+h];
            const double vCurrentCell = oldQWithHalo[currentCellSerialized*totalVariables+hv]/oldQWithHalo[currentCellSerialized*totalVariables+h]; 

            const double uXLeftNeighbourCell = oldQWithHalo[xLeftNeighbourCellSerialized*totalVariables+hu]/oldQWithHalo[xLeftNeighbourCellSerialized*totalVariables+h];
            const double vXLeftNeighbourCell = oldQWithHalo[xLeftNeighbourCellSerialized*totalVariables+hv]/oldQWithHalo[xLeftNeighbourCellSerialized*totalVariables+h];

            const double uXRightNeighbourCell = oldQWithHalo[xRightNeighbourCellSerialized*totalVariables+hu]/oldQWithHalo[xRightNeighbourCellSerialized*totalVariables+h];
            const double vXRightNeighbourCell = oldQWithHalo[xRightNeighbourCellSerialized*totalVariables+hv]/oldQWithHalo[xRightNeighbourCellSerialized*totalVariables+h];
 
            const double uYLeftNeighbourCell = oldQWithHalo[yLeftNeighbourCellSerialized*totalVariables+hu]/oldQWithHalo[yLeftNeighbourCellSerialized*totalVariables+h];
            const double vYLeftNeighbourCell = oldQWithHalo[yLeftNeighbourCellSerialized*totalVariables+hv]/oldQWithHalo[yLeftNeighbourCellSerialized*totalVariables+h];

            const double uYRightNeighbourCell = oldQWithHalo[yRightNeighbourCellSerialized*totalVariables+hu]/oldQWithHalo[yRightNeighbourCellSerialized*totalVariables+h];
            const double vYRightNeighbourCell = oldQWithHalo[yRightNeighbourCellSerialized*totalVariables+hv]/oldQWithHalo[yRightNeighbourCellSerialized*totalVariables+h];
 
            const double uXLeftYLeftNeighbourCell = oldQWithHalo[xLeftYLeftNeighbourCellSerialized*totalVariables+hu]/oldQWithHalo[xLeftYLeftNeighbourCellSerialized*totalVariables+h];
            const double vXLeftYLeftNeighbourCell = oldQWithHalo[xLeftYLeftNeighbourCellSerialized*totalVariables+hv]/oldQWithHalo[xLeftYLeftNeighbourCellSerialized*totalVariables+h];
 
            const double uXLeftYRightNeighbourCell = oldQWithHalo[xLeftYRightNeighbourCellSerialized*totalVariables+hu]/oldQWithHalo[xLeftYRightNeighbourCellSerialized*totalVariables+h];
            const double vXLeftYRightNeighbourCell = oldQWithHalo[xLeftYRightNeighbourCellSerialized*totalVariables+hv]/oldQWithHalo[xLeftYRightNeighbourCellSerialized*totalVariables+h];
 
            const double uXRightYLeftNeighbourCell = oldQWithHalo[xRightYLeftNeighbourCellSerialized*totalVariables+hu]/oldQWithHalo[xRightYLeftNeighbourCellSerialized*totalVariables+h];
            const double vXRightYLeftNeighbourCell = oldQWithHalo[xRightYLeftNeighbourCellSerialized*totalVariables+hv]/oldQWithHalo[xRightYLeftNeighbourCellSerialized*totalVariables+h];
 
            const double uXRightYRightNeighbourCell = oldQWithHalo[xRightYRightNeighbourCellSerialized*totalVariables+hu]/oldQWithHalo[xRightYRightNeighbourCellSerialized*totalVariables+h];
            const double vXRightYRightNeighbourCell = oldQWithHalo[xRightYRightNeighbourCellSerialized*totalVariables+hv]/oldQWithHalo[xRightYRightNeighbourCellSerialized*totalVariables+h];
 
            //u_x
            oldQWithHalo[currentCellSerialized*totalVariables+real_var+u_x] = (uXRightNeighbourCell - uXLeftNeighbourCell);

            //u_y
            oldQWithHalo[currentCellSerialized*totalVariables+real_var+u_y] = (uYRightNeighbourCell - uYLeftNeighbourCell);

            //u_xx
            oldQWithHalo[currentCellSerialized*totalVariables+real_var+u_xx] = (uXRightNeighbourCell -2*uCurrentCell + uXLeftNeighbourCell);

            //u_yy
            oldQWithHalo[currentCellSerialized*totalVariables+real_var+u_yy] = (uYRightNeighbourCell -2*uCurrentCell + uYLeftNeighbourCell);

            //u_xy
            oldQWithHalo[currentCellSerialized*totalVariables+real_var+u_xy] = (uXRightYRightNeighbourCell - uXLeftYRightNeighbourCell - uXRightYLeftNeighbourCell + uXLeftYLeftNeighbourCell);

            //v_x
            oldQWithHalo[currentCellSerialized*totalVariables+real_var+v_x] = (vXRightNeighbourCell - vXLeftNeighbourCell);

            //v_y
            oldQWithHalo[currentCellSerialized*totalVariables+real_var+v_y] = (vYRightNeighbourCell - vYLeftNeighbourCell);

            //v_xx
            oldQWithHalo[currentCellSerialized*totalVariables+real_var+v_xx] = (vXRightNeighbourCell -2*vCurrentCell + vXLeftNeighbourCell);

            //v_yy
            oldQWithHalo[currentCellSerialized*totalVariables+real_var+v_yy] = (vYRightNeighbourCell -2*vCurrentCell + vYLeftNeighbourCell);

            //v_xy
            oldQWithHalo[currentCellSerialized*totalVariables+real_var+v_xy] = (vXRightYRightNeighbourCell - vXLeftYRightNeighbourCell - vXRightYLeftNeighbourCell + vXLeftYLeftNeighbourCell);  
          }  
        }


    """


my_solver.set_implementation(
  ncp                   = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  flux                  = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  refinement_criterion  = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  source_term           = exahype2.solvers.fv.PDETerms.User_Defined_Implementation
)

#
# We can add additional variables to our solver instance, such as constexpr values
# we want to use in our solver implementations multiple times.
#
## Independent Constants
my_solver.add_solver_constants("static constexpr double Gravity = {}; // [m/s^2]".format(9.81))
my_solver.add_solver_constants("static constexpr double Pi = {};".format(3.14159))
my_solver.add_solver_constants("static constexpr double Zeta = Pi/6.0;")
my_solver.add_solver_constants("static constexpr double Zeta1 = Pi/3.0;")
my_solver.add_solver_constants("static constexpr double Zeta2 = Pi/12.0;")
my_solver.add_solver_constants("static constexpr double Zeta3 = Pi/15.0;")
my_solver.add_solver_constants("static constexpr double Beta = {};".format(1.0))
my_solver.add_solver_constants("static constexpr double FrictionLengthScale = {};".format(1.0))

## Dependent Constants
my_solver.add_solver_constants("static constexpr double Mu1 = tan(Zeta1);")
my_solver.add_solver_constants("static constexpr double Mu2 = tan(Zeta2);")
my_solver.add_solver_constants("static constexpr double Mu3 = tan(Zeta3);")
my_solver.add_solver_constants("static constexpr double Nu = (2.0/9.0)*(FrictionLengthScale/Beta)*(Gravity*sin(Zeta)/sqrt(Gravity*cos(Zeta)))*(((Mu2-Mu1)/(tan(Zeta)-Mu1))-1); //Granular Viscosity Coefficient")
my_solver.add_solver_constants("static constexpr double Hstart = FrictionLengthScale*(((Mu2-Mu1)/(tan(Zeta)-Mu3))-1) ;")
my_solver.add_solver_constants("static constexpr double Hstop = FrictionLengthScale*(((Mu2-Mu1)/(tan(Zeta)-Mu1))-1) ;")
#

#
# Add the solver to our project
#
my_project.add_solver( my_solver )

#
# Configure some global parameters
#
my_project.set_global_simulation_parameters(
  dimensions            = 2,
  size                  = [3.0, 3.0],
  offset                = [0.0, 0.0],
  min_end_time          = 0.05,
  max_end_time          = 0.05,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.005, # snapshot interval
  periodic_BC           = [False, False, False]
)

#
# This defines where the output files should go.
# If you omit this, output files are automatically put into the application's folder
#
my_project.set_output_path("solution")
if not os.path.exists("solution"):
  os.makedirs("solution")

#
# If you only target a sequential execution, you can omit the load balancing.
# However, for a parallel build and execution you need to set load balancing.
# This requires the 'LoadBalancing' toolbox to be built.
#
my_project.set_load_balancing( "toolbox::loadbalancing::RecursiveBipartition", "new ::exahype2::LoadBalancingConfiguration()" )

#
# We need to set the location of our core libraries ('Peano4').
# This helps us to resolve any dependencies.
# Additionally, we specify the build mode which you can also change to a different mode.
# Switch between 'Release', 'Debug', 'Asserts', 'Trace', 'Stats'.
#
my_project.set_Peano4_installation( "../../../", mode=peano4.output.CompileMode.Release )

#
# We generate and grab the underlying core project of 'Peano4'.
# This gives us access to some functions we want to use to finalise and build this project.
#
my_project = my_project.generate_Peano4_project(verbose=True)

#
# We add our custom C++ solver file here.
# This allows us to call 'make distclean' without deleting this file as it is not generated.
# This step is optional and only required if 'make distclean' is used.
# Furthermore, this ensures that the C++ file is not ignored by the generated .gitignore (which ignores all generated files).
# As you want to have your own custom C++ file tracked by Git.
#
my_project.output.makefile.add_cpp_file( "SWESolver.cpp" )

#
# Finally, we want to build our project.
# First, all of the necessary glue code is generated in the application folder,
# then 'make' is invoked automatically which compiles the generated code and links against our core libraries
# and toolboxes which have been built before.
# You can also always invoke 'make' yourself to compile, or cleanup with 'make clean'.
#
my_project.build(make_clean_first=True, throw_away_data_after_build=True)

print("Executable is 'swe'")
print("Clean object files via 'make clean'")
print("Recompile the generated code via 'make -j'")
print("Remove all generated code via 'make distclean'")
print("Regenerate all code by running 'python3 swe.py' again")
print("Convert any output via python3 ../../../python/peano4/visualisation/render.py solution/solution-SWESolver.peano-patch-file")
print("Then you can open the generated .pvd file in a tool of your choice (e.g., ParaView)")
