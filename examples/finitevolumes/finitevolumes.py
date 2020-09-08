"""

 A very simple example which demonstrates how to configure a patch-based
 Finite Volume solver in Peano4. The code relies on snippets from ExaHyPE2.
 However, it relies only on ExaHyPE's C/FORTRAN compute kernels, i.e. the 
 sophisticated build environment of this H2020 project is not used. The 
 solver simulates the simple Euler equations.

"""


#
# We import Peano4 as project. If this step fails, ensure that your environment
# variable PYTHONPATH points to Peano4's python directory.
#
import os
import peano4
import peano4.datamodel
import peano4.solversteps
import peano4.output
import peano4.visualisation
import peano4.toolbox.blockstructured


#
# Lets first clean up all plot files, so we don't get a mismatch
#
output_files = [ f for f in os.listdir(".") if f.endswith(".peano-patch-file") ]
for f in output_files:
  os.remove(f)


#
# Create a project and configure it to end up in a subnamespace (and thus
# subdirectory). 
#
project = peano4.Project( ["examples", "finitevolumes"], "finitevolumes", "." )


#
# The solver will be patch-based, i.e. we will have one patch per cell.
#
patch_size = 13
unknowns   = 5
patch = peano4.datamodel.Patch( (patch_size,patch_size,patch_size), unknowns, "Q" )
project.datamodel.add_cell(patch)


#
# Along the faces, we have the patch overlaps. As we use only a Rusanov flux, 
# one cell of overlap between adjacent patches is sufficient.
#
patch_overlap = peano4.datamodel.Patch( (2,patch_size,patch_size), unknowns, "Q" )
project.datamodel.add_face(patch_overlap)


#
# For each step that we wanna do, we define one solver step. In the first step that
# we use, we add one grid level to the mesh per step. We also initialise the blocks
# with the initial values.
#
create_grid = peano4.solversteps.Step( "CreateGrid" )
create_grid.use_face(patch_overlap)
create_grid.use_cell(patch)
#create_grid.add_action_set( peano4.toolbox.PlotGridInPeanoBlockFormat("grid-dump", patch) )
create_grid.add_action_set( peano4.toolbox.blockstructured.ProjectPatchOntoFaces(patch,patch_overlap) )
project.solversteps.add_step(create_grid)


#
# Next, we want to dump the final solution once. Luckily, quite a lot of 
# support for blockstructured grid is available within Peano's toolbox. So
# we use features from there. In the example above, we added code to the 
# step manually (the grid setup). This time, we don't want to add any 
# further code manually.
#that I can 
print_solution = peano4.solversteps.Step( "PlotSolution" )
print_solution.use_cell(patch)
print_solution.use_face(patch_overlap)
print_solution.remove_all_actions()
plotter = peano4.toolbox.blockstructured.PlotPatchesInPeanoBlockFormat("solution",patch,"Q")
print_solution.add_action_set( plotter )
project.solversteps.add_step(print_solution)


#
# Haven't documented anything yet
#
# @todo Dsa ist schon gefused, weil wir ja die Reihenfolge umgedreht haben. Eigentlich muss das action set andersrum laufen
#
perform_time_step      = peano4.solversteps.Step( "TimeStep" )
perform_time_step.use_cell(patch)
perform_time_step.use_face(patch_overlap)
# @todo Darf ich den Face gleich wieder ueberschreiben? Nachbar bekommt ja dann GS-meassig den neuen mit u.U.
functor = """
  auto eigenvalues = [](double Q[5], const tarch::la::Vector<Dimensions,double>& x, int normal, double lambda[5]) -> void {
    constexpr double gamma = 1.4;
    const double irho = 1./Q[0];
    #if Dimensions==3
    const double p = (gamma-1) * (Q[4] - 0.5*irho*Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]);
    #else
    const double p = (gamma-1) * (Q[4] - 0.5*irho*Q[1]*Q[1]+Q[2]*Q[2]);
    #endif

    const double u_n = Q[normal + 1] * irho;
    assertion10( gamma * p * irho>=0.0, gamma, p, irho, x, normal, Q[0], Q[1], Q[2], Q[3], Q[4] );
    const double c   = std::sqrt(gamma * p * irho);

    lambda[0]  = u_n;
    lambda[1]  = u_n;
    lambda[2]  = u_n;
    lambda[3]  = u_n + c;
    lambda[4]  = u_n - c;
    
    assertion4( lambda[0]==lambda[0], u_n, c, x, normal );
    assertion4( lambda[1]==lambda[1], u_n, c, x, normal );
    assertion4( lambda[2]==lambda[2], u_n, c, x, normal );
    assertion4( lambda[3]==lambda[3], u_n, c, x, normal );
    assertion4( lambda[4]==lambda[4], u_n, c, x, normal );
  };
  
  auto flux = [](double Q[5], const tarch::la::Vector<Dimensions,double>& x, int normal, double F[5]) -> void {
    assertion5( Q[0]==Q[0], Q[0], Q[1], Q[2], Q[3], Q[4] );    
    assertion5( Q[1]==Q[1], Q[0], Q[1], Q[2], Q[3], Q[4] );    
    assertion5( Q[2]==Q[2], Q[0], Q[1], Q[2], Q[3], Q[4] );    
    assertion5( Q[3]==Q[3], Q[0], Q[1], Q[2], Q[3], Q[4] );    
    assertion5( Q[4]==Q[4], Q[0], Q[1], Q[2], Q[3], Q[4] );    
    
    assertion5( Q[0]>1e-12, Q[0], Q[1], Q[2], Q[3], Q[4] );
    constexpr double gamma = 1.4;
    const double irho = 1./Q[0];
    #if Dimensions==3
    const double p = (gamma-1) * (Q[4] - 0.5*irho*Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]);
    #else
    const double p = (gamma-1) * (Q[4] - 0.5*irho*Q[1]*Q[1]+Q[2]*Q[2]);
    #endif

    switch (normal) {
      case 0:
        {
          F[0] = Q[1];
          F[1] = irho*Q[1]*Q[1] + p;
          F[2] = irho*Q[2]*Q[1];
          F[3] = (Dimensions==3) ? irho*Q[3]*Q[1] : 0.0;
          F[4] = irho*(Q[4]+p)*Q[1];
        }
        break;
      case 1:
        {
          F[0] = Q[2];
          F[1] = irho*Q[1]*Q[2];
          F[2] = irho*Q[2]*Q[2] + p;
          F[3] = (Dimensions==3) ? irho*Q[3]*Q[2] : 0.0;
          F[4] = irho*(Q[4]+p)*Q[2];
        }
        break;
      case 2:
        {
          F[0] = Q[3];
          F[1] = irho*Q[1]*Q[3];
          F[2] = irho*Q[2]*Q[3];
          F[3] = (Dimensions==3) ? irho*Q[3]*Q[3] + p : 0.0;
          F[4] = irho*(Q[4]+p)*Q[3];
        }
        break;
    }

    assertion( F[0]==F[0] );
    assertion( F[1]==F[1] );
    assertion( F[2]==F[2] );
    assertion( F[3]==F[3] );
    assertion( F[4]==F[4] );
  };

  auto splitRiemann1d = [&flux, &eigenvalues](double QL[5], double QR[5], const tarch::la::Vector<Dimensions,double>& x, double dx, double dt, int normal, double F[5]) -> void {{
    double averageQ[5]; 
    for (int unknown=0; unknown<5; unknown++) {{
      averageQ[unknown] = 0.5 * (QL[unknown] + QR[unknown]);    
      assertion4(averageQ[unknown]==averageQ[unknown], x, dx, dt, normal);
    }}
    
    double averageF[5];
    double lambdas[5];
    flux(averageQ,x,normal,averageF);
    
    double lambdaMax = 0.0;
    eigenvalues(averageQ,x,normal,lambdas);
    for (int unknown=0; unknown<5; unknown++) {{
      assertion(lambdas[unknown]==lambdas[unknown]);
      lambdaMax = std::max(lambdaMax,lambdas[unknown]);
    }}
    
    for (int unknown=0; unknown<5; unknown++) {{
      assertion( QR[unknown]==QR[unknown] );
      assertion( QL[unknown]==QL[unknown] );
      F[unknown] = averageF[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
      assertion9( F[unknown]==F[unknown], averageF[unknown], lambdas[unknown], QR[unknown], QL[unknown], unknown, x, dx, dt, normal );
    }}
  }};  

  constexpr int PatchSize = 13;
  constexpr int HaloSize  = 1;    
  double dt = 0.0001;
  double dx = marker.h()(0) / PatchSize;
  assertion( dx>=tarch::la::NUMERICAL_ZERO_DIFFERENCE );
  dfor(cell,PatchSize) {{ // DOFS_PER_AXIS
    tarch::la::Vector<Dimensions,double> voxelCentre = marker.x()
                                           - static_cast<double>((PatchSize/2+HaloSize)) * tarch::la::Vector<Dimensions,double>(dx)
                                           + tarch::la::multiplyComponents(cell.convertScalar<double>(), tarch::la::Vector<Dimensions,double>(dx));
    
    tarch::la::Vector<Dimensions,int> currentVoxel = cell + tarch::la::Vector<Dimensions,int>(HaloSize); // OVERLAP / Halo layer size
    int currentVoxelSerialised = peano4::utils::dLinearised(currentVoxel,PatchSize + 2*HaloSize);
    
    double accumulatedNumericalFlux[] = { 0.0, 0.0, 0.0, 0.0, 0.0 };
    double numericalFlux[5];
    for (int d=0; d<Dimensions; d++) {
      tarch::la::Vector<Dimensions,int> neighbourVoxel = currentVoxel;
      tarch::la::Vector<Dimensions,double> x = voxelCentre;
      
      neighbourVoxel(d) -= 1;
      int neighbourVoxelSerialised = peano4::utils::dLinearised(neighbourVoxel,PatchSize + 2*HaloSize);
      x(d) -= 0.5 * dx;
      assertion(neighbourVoxel(d)>=0);

      splitRiemann1d( 
        reconstructedPatch + neighbourVoxelSerialised*5,
        reconstructedPatch + currentVoxelSerialised*5,
        x, dx, dt, d,
        numericalFlux
      );
      for (int unknown=0; unknown<5; unknown++) {
        //accumulatedNumericalFlux[unknown] -= numericalFlux[unknown];
        accumulatedNumericalFlux[unknown] += numericalFlux[unknown];
        assertion(  numericalFlux[unknown]==numericalFlux[unknown] );
        assertion(  accumulatedNumericalFlux[unknown]==accumulatedNumericalFlux[unknown] );
      }
      
      neighbourVoxel(d) += 2;
      neighbourVoxelSerialised = peano4::utils::dLinearised(neighbourVoxel,PatchSize + 2*HaloSize);
      x(d) += 1.0 * dx;
      
      splitRiemann1d( 
        reconstructedPatch + currentVoxelSerialised*5,
        reconstructedPatch + neighbourVoxelSerialised*5,
        x, dx, dt, d,
        numericalFlux
      );
      for (int unknown=0; unknown<5; unknown++) {
        //accumulatedNumericalFlux[unknown] += numericalFlux[unknown];
        accumulatedNumericalFlux[unknown] -= numericalFlux[unknown];
        assertion(  numericalFlux[unknown]==numericalFlux[unknown] );
        assertion(  accumulatedNumericalFlux[unknown]==accumulatedNumericalFlux[unknown] );
      }
    }

    int destinationVoxelSerialised = peano4::utils::dLinearised(cell,PatchSize);

    for (int unknown=0; unknown<5; unknown++) {{
      assertion( originalPatch[ destinationVoxelSerialised*5+unknown ]==originalPatch[ destinationVoxelSerialised*5+unknown ] );
      assertion( accumulatedNumericalFlux[unknown]==accumulatedNumericalFlux[unknown] );
      originalPatch[ destinationVoxelSerialised*5+unknown ] += dt / dx * accumulatedNumericalFlux[unknown];
    }}
  }}
"""


perform_time_step.add_action_set( peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor(patch,patch_overlap,functor,"","") )
perform_time_step.add_action_set( peano4.toolbox.blockstructured.ProjectPatchOntoFaces(patch,patch_overlap) )
# @todo raus
perform_time_step.add_action_set( plotter )
project.solversteps.add_step(perform_time_step)


#
# Peano's API does not know which settings to use on the present system. To 
# make it copy/clone the settings identified by ./configure, we ask it to 
# parse the generated configuration scripts. The makefile would also offer a
# routine to set the dimension. We take the default here.
#
# @todo Adopt to your particular system settings
#
project.output.makefile.parse_configure_script_outcome( "../../.." )
project.constants.export( "PatchSize", patch_size )
project.constants.export( "NumberOfUnknownsPerCell", unknowns )


#
# Standard triad of operations. You can skip the first two steps if you want as 
# the script then will automatically invoke the previous steps. The other way 
# round, it is always admissible to only generate stuff, e.g., but to build and
# run the project through a command line
#
project.generate(peano4.output.Overwrite.Default)
project.build()
success = project.run( ["myarguments"] )


#
# Dump grid into VTK
#
#convert = peano4.visualisation.Convert( "grid-dump" )
#convert.set_visualisation_tools_path( "/home/tobias/git/Peano/src/visualisation" )
#convert.extract_fine_grid()
#convert.convert_to_vtk()


if success:
  convert = peano4.visualisation.Convert( "solution" )
  convert.set_visualisation_tools_path( "/home/tobias/git/Peano/src/visualisation" )
  convert.extract_fine_grid()
  convert.convert_to_vtk()
