# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from exahype2.actionsets.action_set import ActionSet


def _components_of(coordinates, description):
    """Returns the number of components every entry of coordinates carries."""
    lengths = {len(coordinate) for coordinate in coordinates}
    if not lengths:
        return 0
    if len(lengths) > 1 or not lengths.issubset({2, 3}):
        raise ValueError(
            "All {} need the same number of components, either two or three.".format(
                description
            )
        )
    return next(iter(lengths))


class CurviTracer(ActionSet):
    """!

    Records the DG solution at fixed coordinates of a curvilinear mesh.

    The coordinates are handed in as physical positions and are mapped into the
    mesh through the solver's invertProjection(). Fault coordinates are recorded
    from the face estimates, where the two estimates of a face are averaged
    except for the components addressed by the solver's u and v shortcuts.
    """

    def __init__(
        self,
        coordinates,
        solver,
        filename,
        fault_coordinates=[],
        position_delta_between_two_snapshots=1e-8,
        time_delta_between_two_snapshots=0,
        output_precision=8,
        use_relative_deltas=False,
        initial_record_time=-1,
        last_record_time=1e8,
        unknowns_to_store=0,
    ):
        """!

        ## Arguments

        coordinates: [[Float]]
          Physical positions at which the solution is recorded. Each entry holds
          either two or three components, and all entries have to agree. Two
          components require a two-dimensional build.

        solver: exahype2.dg.ader.Solver
          The solver whose polynomial is evaluated. It has to offer
          invertProjection() to map a physical position into the mesh.

        filename: String
          Name of the database file, relative to the output path.

        fault_coordinates: [[Float]]
          Positions which are recorded from the face estimates. Their snapshots
          are appended to the database behind those of coordinates.

        position_delta_between_two_snapshots: Float
          Maps to the positionDelta argument of
          peano4::particles::TrajectoryDatabase::TrajectoryDatabase().

        time_delta_between_two_snapshots: Float
          Maps to the timeDelta argument of
          peano4::particles::TrajectoryDatabase::TrajectoryDatabase().

        output_precision: Int
          Number of digits the database writes per value.

        use_relative_deltas: Boolean
          See description of the C++ class peano4::particles::TrajectoryDatabase.

        initial_record_time: Float
          Lower bound of the recording time window handed to the generated code.

        last_record_time: Float
          Upper bound of the recording time window handed to the generated code.

        unknowns_to_store: Int
          Number of unknowns written per snapshot. Zero selects all unknowns of
          the solver.
        """
        super(CurviTracer, self).__init__(descend_invocation_order=1)

        self._fault_coordinates = fault_coordinates
        self._coordinates = coordinates

        self.d = {}
        self.d["SOLVER_NAME"] = solver._name
        self.d["SOLVER_INSTANCE"] = solver.instance_name
        self.d["ORDER"] = solver._basis.order
        self.d["SOLUTION_STORAGE_PRECISION"] = solver.precision.storage
        self.d["UNKNOWNS_PER_DOF"] = solver.unknowns + solver.auxiliary_variables

        self.d["UNKNOWNS_TO_STORE"] = (
            unknowns_to_store if unknowns_to_store > 0 else solver.unknowns
        )

        self.d["POSITION_DELTA"] = position_delta_between_two_snapshots
        self.d["TIME_DELTA"] = time_delta_between_two_snapshots
        self.d["OUTPUT_PRECISION"] = output_precision
        self.d["FILENAME"] = filename
        if use_relative_deltas:
            self.d["USE_RELATIVE_DELTAS"] = "true"
        else:
            self.d["USE_RELATIVE_DELTAS"] = "false"

        self.d["INI_RECORD"] = initial_record_time
        self.d["LAST_RECORD"] = last_record_time

        self.d["N"] = len(coordinates)

        self.d["COORDINATES"] = coordinates
        self.d["FAULT_COORDINATES"] = fault_coordinates
        self.d["COORDINATE_COMPONENTS"] = _components_of(coordinates, "coordinates")
        self.d["FAULT_COORDINATE_COMPONENTS"] = _components_of(
            fault_coordinates, "fault coordinates"
        )

    _TEMPLATE_TOUCH_CELL_LAST_TIME = jinja2.Template("""
  const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

  constexpr double InitialRecordTime = {{INI_RECORD}};
  constexpr double LastRecordTime    = {{LAST_RECORD}};

  if (
    not marker.willBeRefined() and fineGridCell{{SOLVER_NAME}}CellLabel.getHasUpdated()
    and timeStamp >= InitialRecordTime and timeStamp <= LastRecordTime
    and (
      repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep()
      or repositories::{{SOLVER_INSTANCE}}.isGridInitialisationSweep()
    )
  ) {
    ::exahype2::particles::recordCurvilinearCellSolution<{{ORDER}}, {{UNKNOWNS_PER_DOF}}, {{UNKNOWNS_TO_STORE}}>(
      marker,
      kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::QuadraturePoints1D,
      fineGridCell{{SOLVER_NAME}}Q.data(),
      _coordinates,
      DomainOffset,
      timeStamp,
      _database
    );
  }
""")

    _TEMPLATE_TOUCH_FACE_LAST_TIME = jinja2.Template("""
  const double timeStamp = std::max(
    fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(0),
    fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(1)
  );

  constexpr double InitialRecordTime = {{INI_RECORD}};
  constexpr double LastRecordTime    = {{LAST_RECORD}};

  if (
    not marker.willBeRefined() and fineGridFace{{SOLVER_NAME}}FaceLabel.getUpdated()[0]
    and timeStamp >= InitialRecordTime and timeStamp <= LastRecordTime
    and (
      repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep()
      or repositories::{{SOLVER_INSTANCE}}.isGridInitialisationSweep()
    )
    and ::exahype2::particles::isFaultFace(
      marker,
      repositories::{{SOLVER_INSTANCE}}.getFaultNormal(),
      repositories::{{SOLVER_INSTANCE}}.getFaultPosition()
    )
  ) {
    using Shortcuts                    = VariableShortcuts{{SOLVER_NAME}};
    constexpr int BasisElementsPerFace = facedata::{{SOLVER_NAME}}QEstimates::Cardinality / 2;

    ::exahype2::particles::recordCurvilinearFaultSolution<
      {{ORDER}},
      {{UNKNOWNS_PER_DOF}},
      {{UNKNOWNS_TO_STORE}},
      Shortcuts::u,
      Shortcuts::v
    >(
      marker,
      kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::QuadraturePoints1D,
      fineGridFace{{SOLVER_NAME}}QEstimates.data(),
      BasisElementsPerFace,
      _faultCoordinates,
      {{N}},
      timeStamp,
      _database
    );
  }
""")

    def operation_body(self, operation_name):
        result = ""
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
            result = self._TEMPLATE_TOUCH_CELL_LAST_TIME.render(**self.d)
        if (
            operation_name == ActionSet.OPERATION_TOUCH_FACE_LAST_TIME
            and len(self._fault_coordinates) > 0
        ):
            result = self._TEMPLATE_TOUCH_FACE_LAST_TIME.render(**self.d)
        return result

    _TEMPLATE_CONSTRUCTOR = jinja2.Template("""
  _database.setOutputFileName(::runner::RuntimeParameters::getInstance().getOutputPath() + "{{FILENAME}}");
  _database.setOutputPrecision({{OUTPUT_PRECISION}});
  _database.setPositionDeltaBetweenTwoSnapshots({{POSITION_DELTA}}, {{USE_RELATIVE_DELTAS}});
  _database.setTimeDeltaBetweenTwoSnapshots({{TIME_DELTA}});

  _treeId = treeId;

  // One coordinate list serves every tree of the rank, so the first tree to arrive inverts the
  // coordinates.
  const std::scoped_lock lock(_coordinatesMutex);
{%- if COORDINATES %}
  if (_coordinates.empty()) {
    static constexpr double RawCoordinates[][{{COORDINATE_COMPONENTS}}] = {
{%- for coordinate in COORDINATES %}
      { {{coordinate | join(", ")}} }{{"," if not loop.last}}
{%- endfor %}
    };
    ::exahype2::particles::invertCoordinates(repositories::{{SOLVER_INSTANCE}}, RawCoordinates, _coordinates);
  }
{%- endif %}
{%- if FAULT_COORDINATES %}
  if (_faultCoordinates.empty()) {
    static constexpr double RawFaultCoordinates[][{{FAULT_COORDINATE_COMPONENTS}}] = {
{%- for coordinate in FAULT_COORDINATES %}
      { {{coordinate | join(", ")}} }{{"," if not loop.last}}
{%- endfor %}
    };
    ::exahype2::particles::invertCoordinates(repositories::{{SOLVER_INSTANCE}}, RawFaultCoordinates, _faultCoordinates);
  }
{%- endif %}
""")

    def get_constructor_body(self):
        return self._TEMPLATE_CONSTRUCTOR.render(**self.d)

    def includes(self):
        return [
            "Constants.h",
            "VariableShortcuts{}.h".format(self.d["SOLVER_NAME"]),
            "exahype2/particles/CurviTracer.h",
            "kernels/{}/Quadrature.h".format(self.d["SOLVER_NAME"]),
            "peano4/particles/TrajectoryDatabase.h",
            "repositories/SolverRepository.h",
            "runner/RuntimeParameters.h",
        ]

    def get_static_initialisations(self, full_qualified_classname):
        return f"""
std::vector<tarch::la::Vector<DIMENSIONS, double>> {full_qualified_classname}::_coordinates;
std::vector<tarch::la::Vector<DIMENSIONS, double>> {full_qualified_classname}::_faultCoordinates;
std::mutex                                         {full_qualified_classname}::_coordinatesMutex;
peano4::particles::TrajectoryDatabase              {full_qualified_classname}::_database;
"""

    def attributes(self):
        return """
  static std::mutex                                         _coordinatesMutex;
  static std::vector<tarch::la::Vector<DIMENSIONS, double>> _coordinates;
  static std::vector<tarch::la::Vector<DIMENSIONS, double>> _faultCoordinates;

  int _treeId{-1};

  static peano4::particles::TrajectoryDatabase _database;
"""
