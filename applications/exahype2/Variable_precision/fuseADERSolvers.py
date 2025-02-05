
def postprocessing_kernel(condition, solver_name):
   return ("""
    if (
      repositories::{{SOLVER_INSTANCE}}.isFirstGridSweepOfTimeStep()
      and not marker.willBeRefined() and not marker.hasBeenRefined()
      """ + condition + """
    ) {

      constexpr int basisSize = {{ORDER}} + 1;

      #if Dimensions == 2
      constexpr int SpaceFaceSize      = basisSize;
      #else
      constexpr int SpaceFaceSize      = basisSize*basisSize;
      #endif

      constexpr int FluxElementsPerFace   = SpaceFaceSize * {{NUMBER_OF_UNKNOWNS}};
      constexpr int BasisElementsPerFace  = SpaceFaceSize * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}});

      for (int d = 0; d < Dimensions; d++) {

        // Projected solution estimations
        for(int i=0; i<BasisElementsPerFace; i++){
          fineGridFaces""" + solver_name + """Estimates(d).value[BasisElementsPerFace + i ] = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value[BasisElementsPerFace+i];
          fineGridFaces""" + solver_name + """Estimates(d+Dimensions).value[ i ] = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d+Dimensions).value[i];
        }

        // Projected solution estimations
        for(int i=0; i<FluxElementsPerFace; i++){
          fineGridFaces""" + solver_name + """FluxEstimates(d).value[FluxElementsPerFace + i ] = fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value[FluxElementsPerFace+i];
          fineGridFaces""" + solver_name + """FluxEstimates(d+Dimensions).value[ i ] = fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d+Dimensions).value[i];
        }

        /*
        // Projected solution estimations
        std::copy_n(
          fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value + BasisElementsPerFace,
          BasisElementsPerFace,
          fineGridFaces""" + solver_name + """Estimates(d).value + BasisElementsPerFace
        );

        std::copy_n(
          fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d+Dimensions).value
          BasisElementsPerFace,
          fineGridFaces""" + solver_name + """Estimates(d+Dimensions).value
        );

        // Projected flux estimations
        std::copy_n(
          fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value + FluxElementsPerFace,
          FluxElementsPerFace,
          fineGridFaces""" + solver_name + """FluxEstimates(d).value + FluxElementsPerFace
        );

        std::copy_n(
          fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d+Dimensions).value
          FluxElementsPerFace,
          fineGridFaces""" + solver_name + """FluxEstimates(d+Dimensions).value
        );
        */

      }

    }
""")

def fuseADERSolvers(solver1, solver2, condition):

    # fit timestep sizes to one another
    compute_time_step_size = """
    double timeStepSize = std::min( repositories::""" + solver1.get_name_of_global_instance() + """.getAdmissibleTimeStepSize(),
                                repositories::""" + solver2.get_name_of_global_instance() + """.getAdmissibleTimeStepSize());
    """

    solver1._compute_time_step_size  = compute_time_step_size
    solver2._compute_time_step_size = compute_time_step_size

    solver_1_cond = " and " + condition
    solver_2_cond = " and not ( " + condition + ")"

    #Add solver guards
    solver1._action_set_prediction.guard += solver_1_cond
    solver1._action_set_correction.guard += solver_1_cond

    solver2._action_set_prediction.guard += solver_2_cond
    solver2._action_set_correction.guard += solver_2_cond

    # add postprocessing action set to copy data from one face to the other
    solver1._action_set_postprocess_solution._compute_kernel += postprocessing_kernel(solver_1_cond, solver2._unknown_identifier())
    solver2._action_set_postprocess_solution._compute_kernel += postprocessing_kernel(solver_2_cond, solver1._unknown_identifier())
