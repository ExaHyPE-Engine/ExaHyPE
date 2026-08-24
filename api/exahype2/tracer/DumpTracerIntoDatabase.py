# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4.particles

import jinja2


class DumpTracerIntoDatabase(peano4.solversteps.ActionSet):
    """!

    Dump tracer data into a CSV database.

    This action set is to be added to your code if you want to dump tracer data
    into a database. By default, Peano supports csv files. The dumps are not
    synchronised with normal I/O: we keep track of the particles/tracers and dump
    a new snapshot whenever a tracer has moved far enough or enough time has
    passed. Both thresholds are set via the deltas in the constructor.

    This class uses the C++ class toolbox::particles::TrajectoryDatabase from the
    particles toolbox to realise the actual storage.
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
        The deltas below decide which snapshots are recorded: a new one is kept
        only once the position or time moved by more than the respective delta
        since the last recorded snapshot for that particle.

        position_delta_between_two_snapshots: Float
          Maps to the positionDelta argument of
          toolbox::particles::TrajectoryDatabase::TrajectoryDatabase().

        time_delta_between_two_snapshots: Float
          Maps to the timeDelta argument of
          toolbox::particles::TrajectoryDatabase::TrajectoryDatabase().

        use_relative_deltas: Boolean
          See description of C++ class in toolbox::particles::TrajectoryDatabase.

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
        self.d["SOLVER_INSTANCE"] = solver.get_name_of_global_instance()
        self.d["USE_RELATIVE_DELTAS"] = "true" if use_relative_deltas else "false"

        self.d["INI_RECORD"] = initial_record_time
        self.d["LAST_RECORD"] = last_record_time

    __Template_TouchCellLastTime = jinja2.Template(
        """
if (
  not marker.willBeRefined()
  and
  fineGridCell{{SOLVER_NAME}}CellLabel.getHasUpdated()
  and
  repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep()
) {
  constexpr double InitialRecordTime = {{INI_RECORD}};
  constexpr double LastRecordTime = {{LAST_RECORD}};
  for (int vertexNumber = 0; vertexNumber < TWO_POWER_D; vertexNumber++) {
    for (
      const auto* const particle :
      fineGridVertices{{PARTICLES_CONTAINER}}(vertexNumber)
    ) {
      if (
        particle->getParallelState() == globaldata::{{PARTICLE}}::ParallelState::Local
        and
        marker.isContained(particle->getX())
        and
        fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp() >= InitialRecordTime
        and
        fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp() <= LastRecordTime
      ) {
        _database.addParticleSnapshot(
          particle->getNumber(0),
          particle->getNumber(1),
          fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp(),
          particle->getX(),
          particle->getData().size(),
          particle->getData().data()
        );
      }
    }
  }
}
"""
    )

    def get_body_of_operation(self, operation_name):
        result = ""
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
            result = self.__Template_TouchCellLastTime.render(**self.d)
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_constructor_body(self):
        template = jinja2.Template(
            """
  _database.setOutputFileName(
    ::runner::RuntimeParameters::getInstance().getOutputPath() + "{{FILENAME}}"
  );
  _database.setOutputPrecision({{OUTPUT_PRECISION}});
  _database.setPositionDeltaBetweenTwoSnapshots(
    {{POSITION_DELTA}},
    {{USE_RELATIVE_DELTAS}}
  );
  _database.setTimeDeltaBetweenTwoSnapshots({{TIME_DELTA}});
"""
        )
        return template.render(**self.d)

    def get_includes(self):
        result = jinja2.Template(
            """
#include "toolbox/particles/TrajectoryDatabase.h"
#include "runner/RuntimeParameters.h"
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
#include "repositories/SolverRepository.h"
"""
        )
        return result.render(**self.d)

    def get_attributes(self):
        return """
    static toolbox::particles::TrajectoryDatabase _database;
"""

    def get_static_initialisations(self, full_qualified_class_name):
        return (
            """
toolbox::particles::TrajectoryDatabase """
            + full_qualified_class_name
            + """::_database;
"""
        )
