import jinja2


def create_constructor_implementation_for_error_measurement(deltaBetweenDatabaseFlushes, outputFileName, dataDeltaBetweenSnapshots, timeDeltaBetweenSnapshots, clearDatabaseAfterFlush):
    Template = jinja2.Template(
        """
  if(tarch::mpi::Rank::getInstance().isGlobalMaster()){
    _errorDatabase.setDeltaBetweenTwoDatabaseFlushes( {{FLUSH_DELTA}} );
    _errorDatabase.setOutputFileName( "{{FILENAME}}" );
    _errorDatabase.setDataName( "l0 error,l2 error,l_inf error" );
    // _errorDatabase.setOutputPrecision( {{OUTPUT_PRECISION}} );
    _errorDatabase.setDataDeltaBetweenTwoSnapshots( {{DATA_DELTA}}, {{USE_RELATIVE_DELTAS}} );
    _errorDatabase.setTimeDeltaBetweenTwoSnapshots( {{TIME_DELTA}} );
    _errorDatabase.clearDatabaseAfterFlush( {{CLEAR_DATABASE_AFTER_FLUSH}} );
  }
""",
        undefined=jinja2.DebugUndefined,
    )

    d = {}
    d["FLUSH_DELTA"]                = deltaBetweenDatabaseFlushes
    d["FILENAME"]                   = outputFileName
    d["DATA_DELTA"]                 = dataDeltaBetweenSnapshots
    d["USE_RELATIVE_DELTAS"]        = "false"
    d["TIME_DELTA"]                 = timeDeltaBetweenSnapshots
    d["CLEAR_DATABASE_AFTER_FLUSH"] = ( "true" if clearDatabaseAfterFlush else "false")
    return Template.render(**d)

def create_finish_time_step_implementation_for_error_measurement(guard):
    return """

  if ( """ + guard + """ ){

    #ifdef Parallel
    double errorsThisTimeStamp[3] = {_errors[0], _errors[1], _errors[2]};

    tarch::mpi::Rank::getInstance().allReduce(
        &errorsThisTimeStamp[0],
        &_errors[0],
        1,
        MPI_DOUBLE,
        MPI_SUM,
        [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
    );

    tarch::mpi::Rank::getInstance().allReduce(
        &errorsThisTimeStamp[1],
        &_errors[1],
        1,
        MPI_DOUBLE,
        MPI_SUM,
        [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
    );

    tarch::mpi::Rank::getInstance().allReduce(
        &errorsThisTimeStamp[2],
        &_errors[2],
        1,
        MPI_DOUBLE,
        MPI_MAX,
        [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
    );
    #endif


    if (
      tarch::mpi::Rank::getInstance().isGlobalMaster()
    ) {
      _errors[1] = std::sqrt(_errors[1]);
      _errorDatabase.addGlobalSnapshot(_minTimeStampThisTimeStep, 3, _errors);
    }

    std::fill_n(_errors, 3, 0.0);

  }
"""

def create_postprocessing_kernel_for_fv_error_measurement():
    return """
      double errors[3] = {0.0, 0.0, 0.0};
      double sol[{{NUMBER_OF_UNKNOWNS}}];
#if Dimensions==2
    const double cell_v = volumeH[0]*volumeH[1];
#else
    const double cell_v = volumeH[0]*volumeH[1]*volumeH[2];
#endif

      repositories::{{SOLVER_INSTANCE}}.analyticalSolution(
        volumeX,
        timeStamp,
        timeStepSize,
        sol
      );

      for(int v=0; v<{{NUMBER_OF_UNKNOWNS}}; v++){ //for each variable
        double error = std::abs(value[v]-sol[v]);
        errors[0] += cell_v*error; //L1 norm
        errors[1] += cell_v*error*error; //L2 norm
        errors[2] = std::max(errors[2], error); //L_inf norm
      }

      repositories::{{SOLVER_INSTANCE}}.setError(errors);

"""

def create_postprocessing_kernel_for_ader_error_measurement():
    return """
  if (
       repositories::{{SOLVER_INSTANCE}}.getSolverState()=={{SOLVER_NAME}}::SolverState::GridInitialisation
    or repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep()
    and not marker.willBeRefined() and not marker.hasBeenRefined()
  ) {

    double errors[3];

    ::exahype2::aderdg::computeCellErrorIntegral(
      [&](
        const tarch::la::Vector<Dimensions,double>&   x,
        const double                                  t,
        const double                                  dt,
        double*                                       sol
      )->void {

        repositories::{{SOLVER_INSTANCE}}.analyticalSolution(x,t,dt,sol);

      },
      marker.x(),
      marker.h(),
      fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp(),
      repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize(),
      repositories::{{SOLVER_INSTANCE}}.Order,
      repositories::{{SOLVER_INSTANCE}}.QuadratureWeights,
      repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
      repositories::{{SOLVER_INSTANCE}}.NumberOfUnknowns,
      repositories::{{SOLVER_INSTANCE}}.NumberOfAuxiliaryVariables,
      fineGridCell{{UNKNOWN_IDENTIFIER}}.value,
      errors
    );

    repositories::{{SOLVER_INSTANCE}}.setError(errors);

  }
"""


def create_solver_user_abstract_declarations(user_defined):
    return """
    /**
     * Exact solution of the equation at a given point and time
     *
     * @param x             position
     * @param t             Time
     * @param dt            Timestep size
     * @param sol           Solution of the equation
     */
    virtual void analyticalSolution(
      [[maybe_unused]] const tarch::la::Vector<Dimensions,double>&   x,
      [[maybe_unused]] const double                                  t,
      [[maybe_unused]] const double                                  dt,
      [[maybe_unused]] double*                                       sol // sol[{{NUMBER_OF_UNKNOWNS}}]
    )""" + (" = 0" if user_defined else " final") + ";"


def create_solver_user_abstract_definitions(error_measurement_implementation):
    return """
void {{"{{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}"}}::analyticalSolution(
  [[maybe_unused]] const tarch::la::Vector<Dimensions,double>&   x,
  [[maybe_unused]] const double                                  t,
  [[maybe_unused]] const double                                  dt,
  [[maybe_unused]] double*                                       sol // sol[{{NUMBER_OF_UNKNOWNS}}]
) {
""" + error_measurement_implementation + """
}
"""


def create_solver_user_declarations():
    return """
    virtual void analyticalSolution(
      [[maybe_unused]] const tarch::la::Vector<Dimensions,double>&   x,
      [[maybe_unused]] const double                                  t,
      [[maybe_unused]] const double                                  dt,
      [[maybe_unused]] double*                                       sol // sol[{{NUMBER_OF_UNKNOWNS}}]
    ) override;
"""


def create_solver_user_definitions():
    return """
void {{"{{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}"}}::analyticalSolution(
  [[maybe_unused]] const tarch::la::Vector<Dimensions,double>&   x,
  [[maybe_unused]] const double                                  t,
  [[maybe_unused]] const double                                  dt,
  [[maybe_unused]] double*                                       sol // sol[{{NUMBER_OF_UNKNOWNS}}]
) {
  logTraceInWith2Arguments( "analyticalSolution(...)", x, t );

  // @todo Implement your stuff here

  logTraceOut( "analyticalSolution(...)" );
}
"""