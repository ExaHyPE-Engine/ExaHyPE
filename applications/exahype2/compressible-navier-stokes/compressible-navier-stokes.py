import os
import sys

sys.path.insert(0, os.path.abspath('../../../python'))

import peano4
import exahype2

dimensions                = 2
unknowns                  = dimensions + 2 # [rho, u, v, (w), e]
cell_size                 = 0.1
amr_levels                = 0 # Set to 0 to disable adaptive mesh refinement (AMR)
patch_size                = 3
auxiliary_variables       = 0
end_time                  = 3.0
plot_interval             = 0.05 # Set to 0 to disable I/O
mode                      = peano4.output.CompileMode.Release # Switch here between 'Release', 'Debug', 'Asserts', 'Trace', 'Stats'
use_gpu                   = False # Requires a GPU enabled build

if dimensions == 2:
  size                    = [1.0, 1.0]
  offset                  = [0.0, 0.0]
else:
  size                    = [1.0, 1.0, 1.0]
  offset                  = [0.0, 0.0, 0.0]

gamma                     = 1.4
fluid_viscosity           = 0.01
prandtl_number            = 0.7
lid_driven_cavity_velocity = 1.0 # Wall velocity for lid-driven cavity

my_project = exahype2.Project(
  ["applications", "exahype2", "CompressibleNavierStokes"],
  "CompressibleNavierStokesApplication", ".",
  executable="compressible-navier-stokes-" + str(dimensions) + "d"
)

my_solver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep(
  name                    = "NavierStokesSolver",
  patch_size              = patch_size,
  unknowns                = unknowns,
  auxiliary_variables     = 0,
  max_volume_h            = cell_size,
  min_volume_h            = cell_size * 3.0**(-amr_levels),
  time_step_relaxation    = 0.5,
  #pde_terms_without_state = use_gpu
)

if dimensions == 2:
  my_solver.plot_description = "rho, u, v, e"
else:
  my_solver.plot_description = "rho, u, v, w, e"

my_solver.set_implementation(
  ncp                   = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  flux                  = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  eigenvalues           = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  #source_term          = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  refinement_criterion = exahype2.solvers.fv.PDETerms.User_Defined_Implementation
)

my_solver._preprocess_reconstructed_patch = """
    const int patchSize = """ + str( my_solver.patch_size ) + """;
    int aux_var=""" + str( my_solver.auxiliary_variables ) + """;
    int real_var=""" + str(my_solver.unknowns) + """;

#if Dimensions == 2
    tarch::la::Vector<Dimensions, int> topLeftCell     = {0, patchSize + 1};
    tarch::la::Vector<Dimensions, int> topRightCell    = {patchSize + 1, patchSize + 1};
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

    const int topLeftCellSerialised     = peano4::utils::dLinearised(topLeftCell, patchSize + 2 * 1);
    const int topRightCellSerialised    = peano4::utils::dLinearised(topRightCell, patchSize + 2 * 1);
    const int bottomLeftCellSerialised  = peano4::utils::dLinearised(bottomLeftCell, patchSize + 2 * 1);
    const int bottomRightCellSerialised = peano4::utils::dLinearised(bottomRightCell, patchSize + 2 * 1);

    const int topLeftCellN1Serialised = peano4::utils::dLinearised(topLeftCellN1, patchSize + 2 * 1);
    const int topLeftCellN2Serialised = peano4::utils::dLinearised(topLeftCellN2, patchSize + 2 * 1);

    const int topRightCellN1Serialised = peano4::utils::dLinearised(topRightCellN1, patchSize + 2 * 1);
    const int topRightCellN2Serialised = peano4::utils::dLinearised(topRightCellN2, patchSize + 2 * 1);

    const int bottomLeftCellN1Serialised = peano4::utils::dLinearised(bottomLeftCellN1, patchSize + 2 * 1);
    const int bottomLeftCellN2Serialised = peano4::utils::dLinearised(bottomLeftCellN2, patchSize + 2 * 1);

    const int bottomRightCellN1Serialised = peano4::utils::dLinearised(bottomRightCellN1, patchSize + 2 * 1);
    const int bottomRightCellN2Serialised = peano4::utils::dLinearised(bottomRightCellN2, patchSize + 2 * 1);

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
#else
    // Edges
    for (int i = 1, i < patchSize + 1, i++) {
      tarch::la::Vector<Dimensions, int> backLeftCell   = {0, i, 0};
      tarch::la::Vector<Dimensions, int> backLeftCellN1 = {1, i, 0};
      tarch::la::Vector<Dimensions, int> backLeftCellN2 = {0, i, 1};

      const int backLeftCellSerialised   = peano4::utils::dLinearised(backLeftCell, patchSize + 2 * 1);
      const int backLeftCellN1Serialised = peano4::utils::dLinearised(backLeftCellN1, patchSize + 2 * 1);
      const int backLeftCellN2Serialised = peano4::utils::dLinearised(backLeftCellN2, patchSize + 2 * 1);

      tarch::la::Vector<Dimensions, int> backRightCell   = {patchSize + 1, i, 0};
      tarch::la::Vector<Dimensions, int> backRightCellN1 = {patchSize, i, 0};
      tarch::la::Vector<Dimensions, int> backRightCellN2 = {patchSize + 1, i, 1};

      const int backRightCellSerialised   = peano4::utils::dLinearised(backRightCell, patchSize + 2 * 1);
      const int backRightCellN1Serialised = peano4::utils::dLinearised(backRightCellN1, patchSize + 2 * 1);
      const int backRightCellN2Serialised = peano4::utils::dLinearised(backRightCellN2, patchSize + 2 * 1);

      tarch::la::Vector<Dimensions, int> backTopCell   = {i, patchSize + 1, 0};
      tarch::la::Vector<Dimensions, int> backTopCellN1 = {i, patchSize + 1, 1};
      tarch::la::Vector<Dimensions, int> backTopCellN2 = {i, patchSize, 0};

      const int backTopCellSerialised   = peano4::utils::dLinearised(backTopCell, patchSize + 2 * 1);
      const int backTopCellN1Serialised = peano4::utils::dLinearised(backTopCellN1, patchSize + 2 * 1);
      const int backTopCellN2Serialised = peano4::utils::dLinearised(backTopCellN2, patchSize + 2 * 1);

      tarch::la::Vector<Dimensions, int> backBottomCell   = {i, 0, 0};
      tarch::la::Vector<Dimensions, int> backBottomCellN1 = {i, 0, 1};
      tarch::la::Vector<Dimensions, int> backBottomCellN2 = {i, 1, 0};

      const int backBottomCellSerialised   = peano4::utils::dLinearised(backBottomCell, patchSize + 2 * 1);
      const int backBottomCellN1Serialised = peano4::utils::dLinearised(backBottomCellN1, patchSize + 2 * 1);
      const int backBottomCellN2Serialised = peano4::utils::dLinearised(backBottomCellN2, patchSize + 2 * 1);

      tarch::la::Vector<Dimensions, int> frontLeftCell   = {0, i, patchSize + 1};
      tarch::la::Vector<Dimensions, int> frontLeftCellN1 = {1, i, patchSize + 1};
      tarch::la::Vector<Dimensions, int> frontLeftCellN2 = {0, i, patchSize};

      const int frontLeftCellSerialised   = peano4::utils::dLinearised(frontLeftCell, patchSize + 2 * 1);
      const int frontLeftCellN1Serialised = peano4::utils::dLinearised(frontLeftCellN1, patchSize + 2 * 1);
      const int frontLeftCellN2Serialised = peano4::utils::dLinearised(frontLeftCellN2, patchSize + 2 * 1);

      tarch::la::Vector<Dimensions, int> frontRightCell   = {patchSize + 1, i, patchSize + 1};
      tarch::la::Vector<Dimensions, int> frontRightCellN1 = {patchSize, i, patchSize + 1};
      tarch::la::Vector<Dimensions, int> frontRightCellN2 = {patchSize + 1, i, patchSize};

      const int frontRightCellSerialised   = peano4::utils::dLinearised(frontRightCell, patchSize + 2 * 1);
      const int frontRightCellN1Serialised = peano4::utils::dLinearised(frontRightCellN1, patchSize + 2 * 1);
      const int frontRightCellN2Serialised = peano4::utils::dLinearised(frontRightCellN2, patchSize + 2 * 1);

      tarch::la::Vector<Dimensions, int> frontTopCell   = {i, patchSize + 1, patchSize + 1};
      tarch::la::Vector<Dimensions, int> frontTopCellN1 = {i, patchSize + 1, patchSize};
      tarch::la::Vector<Dimensions, int> frontTopCellN2 = {i, patchSize, patchSize + 1};

      const int frontTopCellSerialised   = peano4::utils::dLinearised(frontTopCell, patchSize + 2 * 1);
      const int frontTopCellN1Serialised = peano4::utils::dLinearised(frontTopCellN1, patchSize + 2 * 1);
      const int frontTopCellN2Serialised = peano4::utils::dLinearised(frontTopCellN2, patchSize + 2 * 1);

      tarch::la::Vector<Dimensions, int> frontBottomCell   = {i, 0, patchSize + 1};
      tarch::la::Vector<Dimensions, int> frontBottomCellN1 = {i, 0, patchSize};
      tarch::la::Vector<Dimensions, int> frontBottomCellN2 = {i, 1, patchSize + 1};

      const int frontBottomCellSerialised   = peano4::utils::dLinearised(frontBottomCell, patchSize + 2 * 1);
      const int frontBottomCellN1Serialised = peano4::utils::dLinearised(frontBottomCellN1, patchSize + 2 * 1);
      const int frontBottomCellN2Serialised = peano4::utils::dLinearised(frontBottomCellN2, patchSize + 2 * 1);

      tarch::la::Vector<Dimensions, int> bottomLeftCell   = {0, 0, i};
      tarch::la::Vector<Dimensions, int> bottomLeftCellN1 = {1, 0, i};
      tarch::la::Vector<Dimensions, int> bottomLeftCellN2 = {0, 1, i};

      const int bottomLeftCellSerialised   = peano4::utils::dLinearised(bottomLeftCell, patchSize + 2 * 1);
      const int bottomLeftCellN1Serialised = peano4::utils::dLinearised(bottomLeftCellN1, patchSize + 2 * 1);
      const int bottomLeftCellN2Serialised = peano4::utils::dLinearised(bottomLeftCellN2, patchSize + 2 * 1);

      tarch::la::Vector<Dimensions, int> bottomRightCell   = {patchSize + 1, 0, i};
      tarch::la::Vector<Dimensions, int> bottomRightCellN1 = {patchSize, 0, i};
      tarch::la::Vector<Dimensions, int> bottomRightCellN2 = {patchSize + 1, 1, i};

      const int bottomRightCellSerialised   = peano4::utils::dLinearised(bottomRightCell, patchSize + 2 * 1);
      const int bottomRightCellN1Serialised = peano4::utils::dLinearised(bottomRightCellN1, patchSize + 2 * 1);
      const int bottomRightCellN2Serialised = peano4::utils::dLinearised(bottomRightCellN2, patchSize + 2 * 1);

      tarch::la::Vector<Dimensions, int> topLeftCell   = {0, patchSize + 1, i};
      tarch::la::Vector<Dimensions, int> topLeftCellN1 = {1, patchSize + 1, i};
      tarch::la::Vector<Dimensions, int> topLeftCellN2 = {0, patchSize, i};

      const int topLeftCellSerialised   = peano4::utils::dLinearised(topLeftCell, patchSize + 2 * 1);
      const int topLeftCellN1Serialised = peano4::utils::dLinearised(topLeftCellN1, patchSize + 2 * 1);
      const int topLeftCellN2Serialised = peano4::utils::dLinearised(topLeftCellN2, patchSize + 2 * 1);

      tarch::la::Vector<Dimensions, int> topRightCell   = {patchSize + 1, patchSize + 1, i};
      tarch::la::Vector<Dimensions, int> topRightCellN1 = {patchSize, patchSize + 1, i};
      tarch::la::Vector<Dimensions, int> topRightCellN2 = {patchSize + 1, patchSize, i};

      const int topRightCellSerialised   = peano4::utils::dLinearised(topRightCell, patchSize + 2 * 1);
      const int topRightCellN1Serialised = peano4::utils::dLinearised(topRightCellN1, patchSize + 2 * 1);
      const int topRightCellN2Serialised = peano4::utils::dLinearised(topRightCellN2, patchSize + 2 * 1);

      for (int j = 0, j < aux_var + real_var, j++) {
        oldQWithHalo[(backLeftCellSerialised) * (real_var + aux_var) + i]
          = 0.5
            * (oldQWithHalo[(backLeftCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(backLeftCellN2Serialised) * (real_var + aux_var) + i]);

        oldQWithHalo[(backRightCellSerialised) * (real_var + aux_var) + i]
          = 0.5
            * (oldQWithHalo[(backRightCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(backRightCellN2Serialised) * (real_var + aux_var) + i]);

        oldQWithHalo[(backTopCellSerialised) * (real_var + aux_var) + i]
          = 0.5
            * (oldQWithHalo[(backTopCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(backTopCellN2Serialised) * (real_var + aux_var) + i]);

        oldQWithHalo[(backBottomCellSerialised) * (real_var + aux_var) + i]
          = 0.5
            * (oldQWithHalo[(backBottomCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(backBottomCellN2Serialised) * (real_var + aux_var) + i]);

        oldQWithHalo[(frontLeftCellSerialised) * (real_var + aux_var) + i]
          = 0.5
            * (oldQWithHalo[(frontLeftCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(frontLeftCellN2Serialised) * (real_var + aux_var) + i]);

        oldQWithHalo[(frontRightCellSerialised) * (real_var + aux_var) + i]
          = 0.5
            * (oldQWithHalo[(frontRightCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(frontRightCellN2Serialised) * (real_var + aux_var) + i]);
      
      oldQWithHalo[(frontTopCellSerialised) * (real_var + aux_var) + i]
          = 0.5
            * (oldQWithHalo[(frontTopCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(frontTopCellN2Serialised) * (real_var + aux_var) + i]);
      
      oldQWithHalo[(frontBottomCellSerialised) * (real_var + aux_var) + i]
          = 0.5
            * (oldQWithHalo[(frontBottomCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(frontBottomCellN2Serialised) * (real_var + aux_var) + i]);
      
      oldQWithHalo[(bottomLeftCellSerialised) * (real_var + aux_var) + i]
          = 0.5
            * (oldQWithHalo[(bottomLeftCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(bottomLeftCellN2Serialised) * (real_var + aux_var) + i]);
      
      oldQWithHalo[(bottomRightCellSerialised) * (real_var + aux_var) + i]
          = 0.5
            * (oldQWithHalo[(bottomRightCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(bottomRightCellN2Serialised) * (real_var + aux_var) + i]);
      
      oldQWithHalo[(topLeftCellSerialised) * (real_var + aux_var) + i]
          = 0.5
            * (oldQWithHalo[(topLeftCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(topLeftCellN2Serialised) * (real_var + aux_var) + i]);
      
      oldQWithHalo[(topRightCellSerialised) * (real_var + aux_var) + i]
          = 0.5
            * (oldQWithHalo[(topRightCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(topRightCellN2Serialised) * (real_var + aux_var) + i]);
      
      }
    }

    // Corners
    tarch::la::Vector<Dimensions, int> topLeftBackCell     = {0, patchSize + 1, 0};
    tarch::la::Vector<Dimensions, int> topRightBackCell    = {patchSize + 1, patchSize + 1, 0};
    tarch::la::Vector<Dimensions, int> bottomLeftBackCell  = {0, 0, 0};
    tarch::la::Vector<Dimensions, int> bottomRightBackCell = {patchSize + 1, 0, 0};

    tarch::la::Vector<Dimensions, int> topLeftFrontCell     = {0, patchSize + 1, patchSize + 1};
    tarch::la::Vector<Dimensions, int> topRightFrontCell    = {patchSize + 1, patchSize + 1, patchSize + 1};
    tarch::la::Vector<Dimensions, int> bottomLeftFrontCell  = {0, 0, patchSize + 1};
    tarch::la::Vector<Dimensions, int> bottomRightFrontCell = {patchSize + 1, 0, patchSize + 1};

    tarch::la::Vector<Dimensions, int> topLeftBackCellN1 = topLeftBackCell;
    topLeftBackCellN1(0)++;
    tarch::la::Vector<Dimensions, int> topLeftBackCellN2 = topLeftBackCell;
    topLeftBackCellN2(1)--;
    tarch::la::Vector<Dimensions, int> topLeftBackCellN3 = topLeftBackCell;
    topLeftBackCellN3(2)++;

    tarch::la::Vector<Dimensions, int> topRightBackCellN1 = topRightBackCell;
    topRightBackCellN1(0)--;
    tarch::la::Vector<Dimensions, int> topRightBackCellN2 = topRightBackCell;
    topRightBackCellN2(1)--;
    tarch::la::Vector<Dimensions, int> topRightBackCellN3 = topRightBackCell;
    topRightBackCellN3(2)++;

    tarch::la::Vector<Dimensions, int> bottomLeftBackCellN1 = bottomLeftBackCell;
    bottomLeftBackCellN1(0)++;
    tarch::la::Vector<Dimensions, int> bottomLeftBackCellN2 = bottomLeftBackCell;
    bottomLeftBackCellN2(1)++;
    tarch::la::Vector<Dimensions, int> bottomLeftBackCellN3 = bottomLeftBackCell;
    bottomLeftBackCellN3(2)++;

    tarch::la::Vector<Dimensions, int> bottomRightBackCellN1 = bottomRightBackCell;
    bottomRightBackCellN1(0)--;
    tarch::la::Vector<Dimensions, int> bottomRightBackCellN2 = bottomRightBackCell;
    bottomRightBackCellN2(1)++;
    tarch::la::Vector<Dimensions, int> bottomRightBackCellN3 = bottomRightBackCell;
    bottomRightBackCellN3(2)++;

    tarch::la::Vector<Dimensions, int> topLeftFrontCellN1 = topLeftFrontCell;
    topLeftFrontCellN1(0)++;
    tarch::la::Vector<Dimensions, int> topLeftFrontCellN2 = topLeftFrontCell;
    topLeftFrontCellN2(1)--;
    tarch::la::Vector<Dimensions, int> topLeftFrontCellN3 = topLeftFrontCell;
    topLeftFrontCellN3(2)--;

    tarch::la::Vector<Dimensions, int> topRightFrontCellN1 = topRightFrontCell;
    topRightFrontCellN1(0)--;
    tarch::la::Vector<Dimensions, int> topRightFrontCellN2 = topRightFrontCell;
    topRightFrontCellN2(1)--;
    tarch::la::Vector<Dimensions, int> topRightFrontCellN3 = topRightFrontCell;
    topRightFrontCellN3(2)--;

    tarch::la::Vector<Dimensions, int> bottomLeftFrontCellN1 = bottomLeftFrontCell;
    bottomLeftFrontCellN1(0)++;
    tarch::la::Vector<Dimensions, int> bottomLeftFrontCellN2 = bottomLeftFrontCell;
    bottomLeftFrontCellN2(1)++;
    tarch::la::Vector<Dimensions, int> bottomLeftFrontCellN3 = bottomLeftFrontCell;
    bottomLeftFrontCellN3(2)--;

    tarch::la::Vector<Dimensions, int> bottomRightFrontCellN1 = bottomRightFrontCell;
    bottomRightFrontCellN1(0)--;
    tarch::la::Vector<Dimensions, int> bottomRightFrontCellN2 = bottomRightFrontCell;
    bottomRightFrontCellN2(1)++;
    tarch::la::Vector<Dimensions, int> bottomRightFrontCellN3 = bottomRightFrontCell;
    bottomRightFrontCellN3(2)--;

    const int topLeftBackCellSerialised     = peano4::utils::dLinearised(topLeftBackCell, patchSize + 2 * 1);
    const int topRightBackCellSerialised    = peano4::utils::dLinearised(topRightBackCell, patchSize + 2 * 1);
    const int bottomLeftBackCellSerialised  = peano4::utils::dLinearised(bottomLeftBackCell, patchSize + 2 * 1);
    const int bottomRightBackCellSerialised = peano4::utils::dLinearised(bottomRightBackCell, patchSize + 2 * 1);

    const int topLeftFrontCellSerialised     = peano4::utils::dLinearised(topLeftFrontCell, patchSize + 2 * 1);
    const int topRightFrontCellSerialised    = peano4::utils::dLinearised(topRightFrontCell, patchSize + 2 * 1);
    const int bottomLeftFrontCellSerialised  = peano4::utils::dLinearised(bottomLeftFrontCell, patchSize + 2 * 1);
    const int bottomRightFrontCellSerialised = peano4::utils::dLinearised(bottomRightFrontCell, patchSize + 2 * 1);

    const int topLeftBackCellN1Serialised = peano4::utils::dLinearised(topLeftBackCellN1, patchSize + 2 * 1);
    const int topLeftBackCellN2Serialised = peano4::utils::dLinearised(topLeftBackCellN2, patchSize + 2 * 1);
    const int topLeftBackCellN3Serialised = peano4::utils::dLinearised(topLeftBackCellN3, patchSize + 2 * 1);

    const int topRightBackCellN1Serialised = peano4::utils::dLinearised(topRightBackCellN1, patchSize + 2 * 1);
    const int topRightBackCellN2Serialised = peano4::utils::dLinearised(topRightBackCellN2, patchSize + 2 * 1);
    const int topRightBackCellN3Serialised = peano4::utils::dLinearised(topRightBackCellN3, patchSize + 2 * 1);

    const int bottomLeftBackCellN1Serialised = peano4::utils::dLinearised(bottomLeftBackCellN1, patchSize + 2 * 1);
    const int bottomLeftBackCellN2Serialised = peano4::utils::dLinearised(bottomLeftBackCellN2, patchSize + 2 * 1);
    const int bottomLeftBackCellN3Serialised = peano4::utils::dLinearised(bottomLeftBackCellN3, patchSize + 2 * 1);

    const int bottomRightBackCellN1Serialised = peano4::utils::dLinearised(bottomRightBackCellN1, patchSize + 2 * 1);
    const int bottomRightBackCellN2Serialised = peano4::utils::dLinearised(bottomRightBackCellN2, patchSize + 2 * 1);
    const int bottomRightBackCellN3Serialised = peano4::utils::dLinearised(bottomRightBackCellN3, patchSize + 2 * 1);

    const int topLeftFrontCellN1Serialised = peano4::utils::dLinearised(topLeftFrontCellN1, patchSize + 2 * 1);
    const int topLeftFrontCellN2Serialised = peano4::utils::dLinearised(topLeftFrontCellN2, patchSize + 2 * 1);
    const int topLeftFrontCellN3Serialised = peano4::utils::dLinearised(topLeftFrontCellN3, patchSize + 2 * 1);

    const int topRightFrontCellN1Serialised = peano4::utils::dLinearised(topRightFrontCellN1, patchSize + 2 * 1);
    const int topRightFrontCellN2Serialised = peano4::utils::dLinearised(topRightFrontCellN2, patchSize + 2 * 1);
    const int topRightFrontCellN3Serialised = peano4::utils::dLinearised(topRightFrontCellN3, patchSize + 2 * 1);

    const int bottomLeftFrontCellN1Serialised = peano4::utils::dLinearised(bottomLeftFrontCellN1, patchSize + 2 * 1);
    const int bottomLeftFrontCellN2Serialised = peano4::utils::dLinearised(bottomLeftFrontCellN2, patchSize + 2 * 1);
    const int bottomLeftFrontCellN3Serialised = peano4::utils::dLinearised(bottomLeftFrontCellN3, patchSize + 2 * 1);

    const int bottomRightFrontCellN1Serialised = peano4::utils::dLinearised(bottomRightFrontCellN1, patchSize + 2 * 1);
    const int bottomRightFrontCellN2Serialised = peano4::utils::dLinearised(bottomRightFrontCellN2, patchSize + 2 * 1);
    const int bottomRightFrontCellN3Serialised = peano4::utils::dLinearised(bottomRightFrontCellN3, patchSize + 2 * 1);

    for (int i = 0; i < real_var + aux_var; i++) {

      oldQWithHalo[(topLeftBackCellSerialised) * (real_var + aux_var) + i]
        = 1.0 / 3.0
          * (oldQWithHalo[(topLeftBackCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(topLeftBackCellN2Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(topLeftBackCellN3Serialised) * (real_var + aux_var) + i]);

      oldQWithHalo[(topRightBackCellSerialised) * (real_var + aux_var) + i]
        = 0.5
          * (oldQWithHalo[(topRightBackCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(topRightBackCellN2Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(topRightBackCellN3Serialised) * (real_var + aux_var) + i]);

      oldQWithHalo[(bottomLeftBackCellSerialised) * (real_var + aux_var) + i]
        = 0.5
          * (oldQWithHalo[(bottomLeftBackCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(bottomLeftBackCellN2Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(bottomLeftBackCellN3Serialised) * (real_var + aux_var) + i]);

      oldQWithHalo[(bottomRightBackCellSerialised) * (real_var + aux_var) + i]
        = 0.5
          * (oldQWithHalo[(bottomRightBackCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(bottomRightBackCellN2Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(bottomRightBackCellN3Serialised) * (real_var + aux_var) + i]);

      oldQWithHalo[(topLeftFrontCellSerialised) * (real_var + aux_var) + i]
        = 1.0 / 3.0
          * (oldQWithHalo[(topLeftFrontCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(topLeftFrontCellN2Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(topLeftFrontCellN3Serialised) * (real_var + aux_var) + i]);

      oldQWithHalo[(topRightFrontCellSerialised) * (real_var + aux_var) + i]
        = 0.5
          * (oldQWithHalo[(topRightFrontCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(topRightFrontCellN2Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(topRightFrontCellN3Serialised) * (real_var + aux_var) + i]);

      oldQWithHalo[(bottomLeftFrontCellSerialised) * (real_var + aux_var) + i]
        = 0.5
          * (oldQWithHalo[(bottomLeftFrontCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(bottomLeftFrontCellN2Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(bottomLeftFrontCellN3Serialised) * (real_var + aux_var) + i]);

      oldQWithHalo[(bottomRightFrontCellSerialised) * (real_var + aux_var) + i]
        = 0.5
          * (oldQWithHalo[(bottomRightFrontCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(bottomRightFrontCellN2Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(bottomRightFrontCellN3Serialised) * (real_var + aux_var) + i]);
    }
#endif
    """



my_solver.add_solver_constants("static constexpr double Gamma = {};".format(gamma))
my_solver.add_solver_constants("static constexpr double Viscosity = {};".format(fluid_viscosity))
my_solver.add_solver_constants("static constexpr double PrandtlNumber = {};".format(prandtl_number))
my_solver.add_solver_constants("static constexpr double LidDrivenCavityVelocity = {};".format(lid_driven_cavity_velocity))
my_solver.add_solver_constants("static constexpr double DomainSize[] = {%s};" % str(size).strip('[]'))
my_solver.add_solver_constants("static constexpr double DomainOffset[] = {%s};" % str(offset).strip('[]'))

my_project.add_solver(my_solver)

my_project.set_global_simulation_parameters(
  dimensions            = dimensions,
  size                  = size,
  offset                = offset,
  min_end_time          = end_time,
  max_end_time          = end_time,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = plot_interval,
  periodic_BC           = [False, False, False]
)

my_project.set_output_path("solution")
if not os.path.exists("solution"):
  os.makedirs("solution")

my_project.set_load_balancing( "toolbox::loadbalancing::RecursiveBipartition", "new ::exahype2::LoadBalancingConfiguration()" )
my_project.set_Peano4_installation("../../../", mode )
my_project = my_project.generate_Peano4_project(verbose=True)
my_project.set_fenv_handler("FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW")
my_project.output.makefile.add_h_file( "NavierStokesSolver.h", generated=False )
my_project.output.makefile.add_cpp_file( "NavierStokesSolver.cpp", generated=False )
my_project.build(make_clean_first=True, throw_away_data_after_build=True)

print("Executable is 'compressible-navier-stokes-<dimensions>d'")
print("Clean object files via 'make clean'")
print("Recompile the generated code via 'make -j'")
print("Remove all generated code via 'make distclean'")
print("Regenerate all code by running 'python3 compressible-navier-stokes.py' again")
print("Convert any output via python3 ../../../python/peano4/visualisation/render.py solution/solution-NavierStokesSolver.peano-patch-file")
print("Then you can open the generated .pvd file in a tool of your choice (e.g., ParaView)")
