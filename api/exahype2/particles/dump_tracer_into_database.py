# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from exahype2.actionsets.action_set import ActionSet


class DumpTracerIntoDatabase(ActionSet):
    """!

    Dumps tracer data into a CSV database.

    The dumps run independently of the plotter: The action set records a new
    snapshot of a tracer once that tracer has moved further than the position
    delta, or once more time than the time delta has passed.
    """

    def __init__(
        self,
        particle_set,
        solver,
        filename,
        position_delta_between_two_snapshots=1e-8,
        time_delta_between_two_snapshots=0,
        output_precision=8,
        use_relative_deltas=False,
        initial_record_time=-1,
        last_record_time=1e8,
    ):
        """!

        The database appends every recorded snapshot to its file straight away.

        position_delta_between_two_snapshots: Float
          Maps to the positionDelta argument of
          peano4::particles::TrajectoryDatabase::TrajectoryDatabase().

        time_delta_between_two_snapshots: Float
          Maps to the timeDelta argument of
          peano4::particles::TrajectoryDatabase::TrajectoryDatabase().

        use_relative_deltas: Boolean
          See description of C++ class in peano4::particles::TrajectoryDatabase.

        initial_record_time: Float
          Time from which on the tracer starts to dump data to files.

        last_record_time: Float
          Time at which the tracer stops dumping data to files.
        """
        super().__init__(descend_invocation_order=1)
        self.d = {}
        self.d["PARTICLE"] = particle_set.particle_model.name
        self.d["PARTICLES_CONTAINER"] = particle_set.name
        self.d["POSITION_DELTA"] = position_delta_between_two_snapshots
        self.d["TIME_DELTA"] = time_delta_between_two_snapshots
        self.d["OUTPUT_PRECISION"] = output_precision
        self.d["FILENAME"] = filename
        self.d["SOLVER_NAME"] = solver._name
        self.d["SOLVER_INSTANCE"] = solver.instance_name
        self.d["USE_RELATIVE_DELTAS"] = "true" if use_relative_deltas else "false"

        self.d["INI_RECORD"] = initial_record_time
        self.d["LAST_RECORD"] = last_record_time

    _TEMPLATE_TOUCH_CELL_LAST_TIME = jinja2.Template("""
  if (
    not marker.willBeRefined() and fineGridCell{{SOLVER_NAME}}CellLabel.getHasUpdated()
    and repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep()
  ) {
    constexpr double InitialRecordTime = {{INI_RECORD}};
    constexpr double LastRecordTime    = {{LAST_RECORD}};
    ::exahype2::particles::recordParticleSnapshots<globaldata::{{PARTICLE}}>(
      marker,
      fineGridVertices{{PARTICLES_CONTAINER}},
      DomainOffset,
      fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp(),
      InitialRecordTime,
      LastRecordTime,
      _database
    );
  }
""")

    def operation_body(self, operation_name):
        result = ""
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
            result = self._TEMPLATE_TOUCH_CELL_LAST_TIME.render(**self.d)
        return result

    def get_constructor_body(self):
        template = jinja2.Template("""
  _database.setOutputFileName(::runner::RuntimeParameters::getInstance().getOutputPath() + "{{FILENAME}}");
  _database.setOutputPrecision({{OUTPUT_PRECISION}});
  _database.setPositionDeltaBetweenTwoSnapshots({{POSITION_DELTA}}, {{USE_RELATIVE_DELTAS}});
  _database.setTimeDeltaBetweenTwoSnapshots({{TIME_DELTA}});
""")
        return template.render(**self.d)

    def includes(self):
        return [
            "Constants.h",
            "exahype2/particles/TrajectoryRecording.h",
            "globaldata/{}.h".format(self.d["PARTICLE"]),
            "peano4/particles/TrajectoryDatabase.h",
            "repositories/SolverRepository.h",
            "runner/RuntimeParameters.h",
            "vertexdata/{}.h".format(self.d["PARTICLES_CONTAINER"]),
        ]

    def attributes(self):
        return """
  static peano4::particles::TrajectoryDatabase _database;
"""

    def get_static_initialisations(self, full_qualified_class_name):
        return f"""
peano4::particles::TrajectoryDatabase {full_qualified_class_name}::_database;
"""
