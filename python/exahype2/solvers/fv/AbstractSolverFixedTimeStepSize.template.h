    double getMinTimeStamp() const final;
    double getMaxTimeStamp() const final;
    double getMinTimeStepSize() const final;
    double getMaxTimeStepSize() const final;


    /**
     * @param Q Vector of unknowns
     * @param t Time
     */
    virtual ::exahype2::RefinementCommand refinementCriterion(
      double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  volumeCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t
    ) {% if REFINEMENT_CRITERION_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final {% endif %};



    /**
     * Feel free to change the solution in the particular finite volume.
     * You can for example change the initial conditions by overwriting
     * the solution for t=0. You may change Q. All other parameters are
     * in.
     */
    virtual void adjustSolution(
      double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  volumeCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t
    ) {% if INITIAL_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final {% endif %};


    /**
     * Apply boundary conditions. You can overwrite both the inside and
     * outside values though most BCs only modify the outside ones. Please
     * note that the boundary conditions you set here are after that subject
     * to the Riemann solver, i.e. flux and eigenvalues.
     */
    virtual void boundaryConditions(
      double * __restrict__ Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      double * __restrict__ Qoutside, // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal
    ) {% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final{% endif %};




    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    void startGridConstructionStep() override;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    void finishGridConstructionStep() override;



    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    void startGridInitialisationStep() override;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    void finishGridInitialisationStep() override;




    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    void startTimeStep(
      double globalMinTimeStamp,
      double globalMaxTimeStamp,
      double globalMinTimeStepSize,
      double globalMaxTimeStepSize
    ) override;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    void finishTimeStep() override;


    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    void startPlottingStep(
      double globalMinTimeStamp,
      double globalMaxTimeStamp,
      double globalMinTimeStepSize,
      double globalMaxTimeStepSize
    ) override;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    void finishPlottingStep() override;


    double getMaxMeshSize() const;
    double getMinMeshSize() const;

    SolverState  getSolverState() const;
  protected:
    static tarch::logging::Log  _log;

    const int  _NumberOfFiniteVolumesPerAxisPerPatch;

    double     _timeStamp;

    SolverState  _solverState;

    double     _maxH;
    double     _minH;
