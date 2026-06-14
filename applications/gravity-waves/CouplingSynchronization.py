def copyPatchAndApplyFunction(
    solverSource,
    solverDestination,
    indexArrayDestination,
    function,
    numberOfUnknownsToCopy,
):
    """!
    @coupling: This function can be used to copy values and transform them while doing so. Particularly nice for momenta and the like.

    solverSource:
    The solver to copy values from.

    solverDestination:
    The solver to copy values to.

    indexArrayDestination:
    Array of indices of length numberOfUnknownsToCopy providing a local index to copy source to. Source value depends on function.

    function:
    A function applied to source values before copying. Unknown to copy is given by "unknown" and source cell base index by "sourceIndexLinearised".

    numberOfUnknownsToCopy:
    Number of unknowns to copy in loop for each cell in patch.
    """
    assert solverSource.patch_size == solverDestination.patch_size
    patch_size = solverSource.patch_size
    numberOfVariablesSource = solverSource.unknowns + solverSource.auxiliary_variables
    numberOfVariablesDestination = (
        solverDestination.unknowns + solverDestination.auxiliary_variables
    )
    return f"""
        // this is copied and adapted from toolbox::blockstructured::copyUnknown()
        #if DIMENSIONS==2
        for (int x=0; x<{patch_size}; x++)
        for (int y=0; y<{patch_size}; y++)
        for (int unknown = 0; unknown < {numberOfUnknownsToCopy}; ++unknown) {{
          const int sourceIndexLinearised = (x + y * {patch_size} ) * {numberOfVariablesSource};
          const int destIndexLinearised   = (x + y * {patch_size} ) * {numberOfVariablesDestination} + {indexArrayDestination}[unknown];
          // apply any function to source value
          const auto sourceValue          = {function};
          fineGridCell{solverDestination.current_unknown_identifier()}.data()[destIndexLinearised] = sourceValue;
        }}
        #elif DIMENSIONS==3
        for (int x=0; x<{solverSource.patch_size}; x++)
        for (int y=0; y<{solverSource.patch_size}; y++)
        for (int z=0; z<{solverSource.patch_size}; z++)
        for (int unknown = 0; unknown < {numberOfUnknownsToCopy}; ++unknown) {{
          const int sourceIndexLinearised = (x + y * {patch_size} + z * {patch_size} * {patch_size}) * {numberOfVariablesSource};
          const int destIndexLinearised   = (x + y * {patch_size} + z * {patch_size} * {patch_size}) * {numberOfVariablesDestination} + {indexArrayDestination}[unknown];
          // apply any function to source value
          const auto sourceValue          = {function}
          fineGridCell{solverDestination.current_unknown_identifier()}.data()[destIndexLinearised] = sourceValue;
        }}
        #else
          #error DIMENSIONS not supported
        #endif
        """


def determineSolverCoupling(atmospheric_solver, diffusion_solver, chemical_solver):
    couple_chemical_solver = False
    couple_diffusion_solver = False
    if atmospheric_solver is not None:
        # any solver to couple?
        couple_chemical_solver = chemical_solver is not None
        couple_diffusion_solver = diffusion_solver is not None

    return couple_diffusion_solver, couple_chemical_solver


def setLastSolver(atmospheric_solver, diffusion_solver, chemical_solver):
    couple_diffusion_solver, couple_chemical_solver = determineSolverCoupling(
        atmospheric_solver=atmospheric_solver,
        diffusion_solver=diffusion_solver,
        chemical_solver=chemical_solver,
    )

    if couple_chemical_solver:
        # couple chemical solver
        atmospheric_solver.last_solver = chemical_solver
        if couple_diffusion_solver:
            # if a diffusion solver is to be coupled as well
            diffusion_solver.last_solver = chemical_solver
        chemical_solver.last_solver = chemical_solver

    elif couple_diffusion_solver:
        # no chemical solver, just diffusion
        atmospheric_solver.last_solver = diffusion_solver
        diffusion_solver.last_solver = diffusion_solver


def synchronizePatchGravityWaves(atmospheric_solver, diffusion_solver, chemical_solver):
    couple_diffusion_solver, couple_chemical_solver = determineSolverCoupling(
        atmospheric_solver=atmospheric_solver,
        diffusion_solver=diffusion_solver,
        chemical_solver=chemical_solver,
    )
    if (
        atmospheric_solver is not None
        or couple_chemical_solver
        or couple_diffusion_solver
    ):
        dimensions = atmospheric_solver._dimensions
    else:
        dimensions = chemical_solver._dimensions

    if couple_chemical_solver or couple_diffusion_solver:
        # add toolbox include to solvers for coupling by copying values
        atmospheric_solver.add_user_action_set_includes(
            '#include "toolbox/blockstructured/Copy.h"\n'
        )
        numberOfVariablesAtmospheric = (
            atmospheric_solver.auxiliary_variables + atmospheric_solver.unknowns
        )

        if couple_chemical_solver:
            chemical_solver.add_user_action_set_includes(
                '#include "toolbox/blockstructured/Copy.h"\n'
            )
            numberOfVariablesChemical = (
                chemical_solver.auxiliary_variables + chemical_solver.unknowns
            )

            if not couple_diffusion_solver:
                # take velocities and temperature from atmosphere
                atmosphereToChemistry(
                    atmospheric_solver=atmospheric_solver,
                    chemical_solver=chemical_solver,
                    numberOfVariablesAtmospheric=numberOfVariablesAtmospheric,
                    numberOfVariablesChemical=numberOfVariablesChemical,
                    dimensions=dimensions,
                )

            # copy chemistry values back to atmosphere
            chemistryToAtmosphere(
                atmospheric_solver=atmospheric_solver, chemical_solver=chemical_solver
            )

        if couple_diffusion_solver:
            numberOfVariablesDiffusion = (
                diffusion_solver.auxiliary_variables + diffusion_solver.unknowns
            )

            diffusion_solver.add_user_action_set_includes(
                '#include "toolbox/blockstructured/Copy.h"\n'
            )

            # copy atmospheric values to diffusion solver
            atmosphereToDiffusion(
                atmospheric_solver=atmospheric_solver,
                diffusion_solver=diffusion_solver,
                dimensions=dimensions,
            )

            # copy diffusion back to atmosphere
            diffusionToAtmosphere(
                atmospheric_solver=atmospheric_solver,
                diffusion_solver=diffusion_solver,
                numberOfVariablesAtmospheric=numberOfVariablesAtmospheric,
                numberOfVariablesDiffusion=numberOfVariablesDiffusion,
                dimensions=dimensions,
            )

            if couple_chemical_solver:
                # copy diffusion to chemistry
                diffusionToChemistry(
                    diffusion_solver=diffusion_solver,
                    chemical_solver=chemical_solver,
                    numberOfVariablesDiffusion=numberOfVariablesDiffusion,
                    numberOfVariablesChemical=numberOfVariablesChemical,
                    dimensions=dimensions,
                )


def coupleTimeStepping(atmospheric_solver, diffusion_solver, chemical_solver):
    couple_diffusion_solver, couple_chemical_solver = determineSolverCoupling(
        atmospheric_solver=atmospheric_solver,
        diffusion_solver=diffusion_solver,
        chemical_solver=chemical_solver,
    )

    if couple_diffusion_solver:
        # diffusion solver accompanies atmospheric solver
        diffusion_solver._compute_time_step_size = (
            """
      const double timeStepSize = repositories::"""
            + atmospheric_solver.get_name_of_global_instance()
            + """.getAdmissibleTimeStepSize();
    """
        )

    if couple_chemical_solver:
        # the time step size of the atmospheric solver will always be <= that of the chemical solver
        # so make synchronization easy and always use the atmospheric dt instead of the chemical one
        chemical_solver._compute_time_step_size = (
            """
      const double timeStepSize = repositories::"""
            + atmospheric_solver.get_name_of_global_instance()
            + """.getAdmissibleTimeStepSize();
    """
        )


def atmosphereToChemistry(
    atmospheric_solver,
    chemical_solver,
    numberOfVariablesAtmospheric,
    numberOfVariablesChemical,
    dimensions,
):
    copy_function = copyPatchAndApplyFunction(
        solverSource=atmospheric_solver,
        solverDestination=chemical_solver,
        indexArrayDestination="chemicalIndices",
        function=f"fineGridCell{atmospheric_solver.current_unknown_identifier()}.data()[sourceIndexLinearised + atmosphericIndices[unknown]] / fineGridCell{atmospheric_solver.current_unknown_identifier()}.data()[sourceIndexLinearised + densityIndex]",
        numberOfUnknownsToCopy=dimensions,
    )

    atmospheric_solver.postprocess_updated_patch += f"""// copy velocities to chemical solver
  {{
    const int atmosphericIndices[DIMENSIONS] = {{
      {atmospheric_solver.variable_names.index("rhou")},
      {atmospheric_solver.variable_names.index("rhov")}{"," + str(atmospheric_solver.variable_names.index("rhow")) if dimensions == 3 else ""}
    }};

    const int chemicalIndices[DIMENSIONS] = {{
      {chemical_solver.variable_names.index("u")},
      {chemical_solver.variable_names.index("v")}{"," + str(chemical_solver.variable_names.index("w")) if dimensions == 3 else ""}
    }};

    const int densityIndex = {atmospheric_solver.variable_names.index("rho")};

    {copy_function}
  }}"""

    atmospheric_solver.postprocess_updated_patch += f"""// copy temperature
    ::toolbox::blockstructured::copyUnknown(
      {atmospheric_solver.patch_size}, // no of unknowns per dimension
      fineGridCell{atmospheric_solver.current_unknown_identifier()}.data(), // source
      {atmospheric_solver.variable_names.index("T")}, // temperature in source
      {numberOfVariablesAtmospheric}, // unknowns in source (incl material parameters)
      0, // no overlap/halo here
      fineGridCell{chemical_solver.current_unknown_identifier()}.data(), // dest
      {chemical_solver.variable_names.index("T")} , // temperature in dest
      {numberOfVariablesChemical}, // unknowns in dest (incl material parameters)
      0 // no overlap/halo here
    );
  """


def atmosphereToDiffusion(atmospheric_solver, diffusion_solver, dimensions):
    # copy velocities to diffusion solver
    copy_function = copyPatchAndApplyFunction(
        solverSource=atmospheric_solver,
        solverDestination=diffusion_solver,
        indexArrayDestination="diffusionIndices",
        function=f"fineGridCell{atmospheric_solver.current_unknown_identifier()}.data()[sourceIndexLinearised + atmosphericIndices[unknown]] / fineGridCell{atmospheric_solver.current_unknown_identifier()}.data()[sourceIndexLinearised + densityIndex]",
        numberOfUnknownsToCopy=dimensions,
    )

    atmospheric_solver.postprocess_updated_patch += f"""// copy velocities to diffusion solver
    {{
      const int atmosphericIndices[DIMENSIONS] = {{
        {atmospheric_solver.variable_names.index("rhou")},
        {atmospheric_solver.variable_names.index("rhov")}{"," + str(atmospheric_solver.variable_names.index("rhow")) if dimensions == 3 else ""}
      }};

      const int diffusionIndices[DIMENSIONS] = {{
        {diffusion_solver.variable_names.index("u")},
        {diffusion_solver.variable_names.index("v")}{"," + str(diffusion_solver.variable_names.index("w")) if dimensions == 3 else ""}
      }};

      const int densityIndex = {atmospheric_solver.variable_names.index("rho")};

      {copy_function}
    }}
  """

    # copy other unknowns to diffusion solver
    copy_function = copyPatchAndApplyFunction(
        solverSource=atmospheric_solver,
        solverDestination=diffusion_solver,
        indexArrayDestination="diffusionIndices",
        function=f"fineGridCell{atmospheric_solver.current_unknown_identifier()}.data()[sourceIndexLinearised + atmosphericIndices[unknown]]",
        numberOfUnknownsToCopy=dimensions,
    )

    atmospheric_solver.postprocess_updated_patch += f"""// copy other unknowns to diffusion solver
  {{
    const int atmosphericIndices[] = {{
      {atmospheric_solver.variable_names.index("rho")},
      {atmospheric_solver.variable_names.index("E")},
      {atmospheric_solver.variable_names.index("O")},
      {atmospheric_solver.variable_names.index("O2")},
      {atmospheric_solver.variable_names.index("N2")},
      {atmospheric_solver.variable_names.index("T")}
    }};

    const int diffusionIndices[] = {{
      {diffusion_solver.variable_names.index("rho")},
      {diffusion_solver.variable_names.index("E")},
      {diffusion_solver.variable_names.index("O")},
      {diffusion_solver.variable_names.index("O2")},
      {diffusion_solver.variable_names.index("N2")},
      {diffusion_solver.variable_names.index("T")}
    }};

    {copy_function}
  }}
  """


def diffusionToAtmosphere(
    atmospheric_solver,
    diffusion_solver,
    numberOfVariablesAtmospheric,
    numberOfVariablesDiffusion,
    dimensions,
):
    # copy updated velocities to atmospheric solver
    copy_function = copyPatchAndApplyFunction(
        solverSource=diffusion_solver,
        solverDestination=atmospheric_solver,
        indexArrayDestination="atmosphericIndices",
        function=f"fineGridCell{diffusion_solver.current_unknown_identifier()}.data()[sourceIndexLinearised + diffusionIndices[unknown]] * fineGridCell{atmospheric_solver.current_unknown_identifier()}.data()[destIndexLinearised - atmosphericIndices[unknown] + densityIndex]",
        numberOfUnknownsToCopy=dimensions,
    )

    diffusion_solver.postprocess_updated_patch += f"""// copy velocities to atmospheric solver
    {{
      const int atmosphericIndices[DIMENSIONS] = {{
        {atmospheric_solver.variable_names.index("rhou")},
        {atmospheric_solver.variable_names.index("rhov")}{"," + str(atmospheric_solver.variable_names.index("rhow")) if dimensions == 3 else ""}
      }};

      const int diffusionIndices[DIMENSIONS] = {{
        {diffusion_solver.variable_names.index("u")},
        {diffusion_solver.variable_names.index("v")}{"," + str(diffusion_solver.variable_names.index("w")) if dimensions == 3 else ""}
      }};

      const int densityIndex = {atmospheric_solver.variable_names.index("rho")};

      {copy_function}
    }}
  """

    # copy updated temperature to atmospheric solver
    diffusion_solver.postprocess_updated_patch += f"""// copy temperature
    ::toolbox::blockstructured::copyUnknown(
      {atmospheric_solver.patch_size}, // no of unknowns per dimension
      fineGridCell{diffusion_solver.current_unknown_identifier()}.data(), // source
      {diffusion_solver.variable_names.index("T")}, // temperature in source
      {numberOfVariablesDiffusion}, // unknowns in source (incl material parameters)
      0, // no overlap/halo here
      fineGridCell{atmospheric_solver.current_unknown_identifier()}.data(), // dest
      {atmospheric_solver.variable_names.index("T")} , // temperature in dest
      {numberOfVariablesAtmospheric}, // unknowns in dest (incl material parameters)
      0 // no overlap/halo here
    );
  """

    # calculate new energy from pressure with new temperature and new velocities
    diffusion_solver.postprocess_updated_patch += f""" // copy new energy for atmosphere
    ::toolbox::blockstructured::copyUnknown(
      {atmospheric_solver.patch_size}, // no of unknowns per dimension
      fineGridCell{diffusion_solver.current_unknown_identifier()}.data(), // source
      {diffusion_solver.variable_names.index("E")}, // temperature in source
      {numberOfVariablesDiffusion}, // unknowns in source (incl material parameters)
      0, // no overlap/halo here
      fineGridCell{atmospheric_solver.current_unknown_identifier()}.data(), // dest
      {atmospheric_solver.variable_names.index("E")} , // temperature in dest
      {numberOfVariablesAtmospheric}, // unknowns in dest (incl material parameters)
      0 // no overlap/halo here
    );
  """


def diffusionToChemistry(
    diffusion_solver,
    chemical_solver,
    numberOfVariablesDiffusion,
    numberOfVariablesChemical,
    dimensions,
):
    # copy updated velocities to chemical solver
    copy_function = copyPatchAndApplyFunction(
        solverSource=diffusion_solver,
        solverDestination=chemical_solver,
        indexArrayDestination="chemicalIndices",
        function=f"fineGridCell{diffusion_solver.current_unknown_identifier()}.data()[sourceIndexLinearised + diffusionIndices[unknown]]",
        numberOfUnknownsToCopy=dimensions,
    )

    diffusion_solver.postprocess_updated_patch += f"""// copy velocities to chemical solver
    {{
      const int chemicalIndices[DIMENSIONS] = {{
        {chemical_solver.variable_names.index("u")},
        {chemical_solver.variable_names.index("v")}{"," + str(chemical_solver.variable_names.index("w")) if dimensions == 3 else ""}
      }};

      const int diffusionIndices[DIMENSIONS] = {{
        {diffusion_solver.variable_names.index("u")},
        {diffusion_solver.variable_names.index("v")}{"," + str(diffusion_solver.variable_names.index("w")) if dimensions == 3 else ""}
      }};

      {copy_function}
    }}
  """

    # copy updated temperature to chemical solver
    diffusion_solver.postprocess_updated_patch += f"""// copy temperature
    ::toolbox::blockstructured::copyUnknown(
      {chemical_solver.patch_size}, // no of unknowns per dimension
      fineGridCell{diffusion_solver.current_unknown_identifier()}.data(), // source
      {diffusion_solver.variable_names.index("T")}, // temperature in source
      {numberOfVariablesDiffusion}, // unknowns in source (incl material parameters)
      0, // no overlap/halo here
      fineGridCell{chemical_solver.current_unknown_identifier()}.data(), // dest
      {chemical_solver.variable_names.index("T")} , // temperature in dest
      {numberOfVariablesChemical}, // unknowns in dest (incl material parameters)
      0 // no overlap/halo here
    );
  """


def chemistryToAtmosphere(atmospheric_solver, chemical_solver):
    # copy chemical densities of major species over to atmospheric model
    copy_function = copyPatchAndApplyFunction(
        solverSource=chemical_solver,
        solverDestination=atmospheric_solver,
        indexArrayDestination="atmosphericIndices",
        function=f"fineGridCell{chemical_solver.current_unknown_identifier()}.data()[sourceIndexLinearised + chemicalIndices[unknown]]",
        numberOfUnknownsToCopy=3,
    )

    chemical_solver.postprocess_updated_patch += f"""
    // copy major species densities
    {{
      const int chemicalIndices[3] = {{
        {chemical_solver.variable_names.index("O")},
        {chemical_solver.variable_names.index("O2")},
        {chemical_solver.variable_names.index("N2")}
      }};

      const int atmosphericIndices[3] = {{
        {atmospheric_solver.variable_names.index("O")},
        {atmospheric_solver.variable_names.index("O2")},
        {atmospheric_solver.variable_names.index("N2")}
      }};

      {copy_function}
    }}
  """
