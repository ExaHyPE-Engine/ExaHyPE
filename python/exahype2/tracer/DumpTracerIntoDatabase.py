# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4.toolbox.particles

import jinja2


class DumpTracerIntoDatabase(peano4.solversteps.ActionSet):
    """!

    Dump the tracer data into a csv database

    This action set is to be added to you code if you want to dump tracer data
    into a database. By default, Peano supports csv files. Please consult the
    constructor for information on the configuration of these dumps: The dumps
    are not synchronised with normal IO, but we keep track of the particles/tracers and
    allow codes to dump updated particle data whenver some attributes or the
    tracer position changes significantly. The term significantly can be controlled
    via relative deltas through the constructor.

    This class uses the C++ class toolbox::particles::TrajectoryDatabase from the
    particles toolbox to realise the actual storage.

    """

    def __init__(
        self,
        particle_set,
        solver,
        filename,
        number_of_entries_between_two_db_flushes=65536,
        data_delta_between_two_snapsots=1e-8,
        position_delta_between_two_snapsots=1e-8,
        time_delta_between_two_snapsots=0,
        output_precision=8,
        clear_database_after_flush=True,
        use_relative_deltas=False,
        initial_record_time=-1,
        last_record_time=1e8,
    ):
        """!

        Constructor

        data_delta_between_two_snapsots: Float
         See toolbox::particles::TrajectoryDatabase::TrajectoryDatabase(). The 
         argument maps 1:1 to the constructor's argument dataDelta.
                  
         This flag has no impact whatsoever how often the data is dumped into a 
         file. The frequency of datafile writes is solely controlled via 
         number_of_entries_between_two_db_flushes.

        position_delta_between_two_snapsots: Float
         See toolbox::particles::TrajectoryDatabase::TrajectoryDatabase(). The 
         argument maps 1:1 to the constructor's argument positionDelta.
         
         This flag has no impact whatsoever how often the data is dumped into a 
         file. The frequency of datafile writes is solely controlled via 
         number_of_entries_between_two_db_flushes.

        time_delta_between_two_snapsots: Float
         See toolbox::particles::TrajectoryDatabase::TrajectoryDatabase(). The 
         argument maps 1:1 to the constructor's argument timeDelta.
         
         This flag has no impact whatsoever how often the data is dumped into a 
         file. The frequency of datafile writes is solely controlled via 
         number_of_entries_between_two_db_flushes.

        number_of_entries_between_two_db_flushes: Int
          See toolbox::particles::TrajectoryDatabase::TrajectoryDatabase(). The 
          argument maps 1:1 to the constructor's argument growthBetweenTwoDatabaseFlushes.
        
        use_relative_deltas: Boolean
          See description of C++ class in toolbox::particles::TrajectoryDatabase

        initial_record_time: Float
          you can specify from when does the tracer start to dump data to files.

        last_record_time: Float
          you can specify at when does the tracer stop dumping data to files.
          
        clear_database_after_flush: Boolean
        """
        super(DumpTracerIntoDatabase, self).__init__(descend_invocation_order=1, parallel=False)
        self.d = {}
        self.d["PARTICLE"] = particle_set.particle_model.name
        self.d["PARTICLES_CONTAINER"] = particle_set.name
        self.d["DATA_DELTA"] = data_delta_between_two_snapsots
        self.d["POSITION_DELTA"] = position_delta_between_two_snapsots
        self.d["TIME_DELTA"] = time_delta_between_two_snapsots
        self.d["OUTPUT_PRECISION"] = output_precision
        self.d["FILENAME"] = filename
        self.d["SOLVER_NAME"] = solver._name
        self.d["SOLVER_INSTANCE"] = solver.get_name_of_global_instance()
        if use_relative_deltas:
            self.d["USE_RELATIVE_DELTAS"] = "true"
        else:
            self.d["USE_RELATIVE_DELTAS"] = "false"
        if clear_database_after_flush:
            self.d["CLEAR_DATABASE_AFTER_FLUSH"] = "true"
        else:
            self.d["CLEAR_DATABASE_AFTER_FLUSH"] = "false"

        self.d["INI_RECORD"] = initial_record_time
        self.d["LAST_RECORD"] = last_record_time

        self.number_of_entries_between_two_db_flushes = (
            number_of_entries_between_two_db_flushes
        )

    __Template_TouchCellLastTime = jinja2.Template(
        """
if ( 
  not marker.willBeRefined() 
  and 
  fineGridCell{{SOLVER_NAME}}CellLabel.getHasUpdated() 
  and
  repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep()
) {
  double IniRecord={{INI_RECORD}};
  double LastRecord={{LAST_RECORD}};
  for (int i=0; i<TwoPowerD; i++) {
    for (auto* p: fineGridVertices{{PARTICLES_CONTAINER}}(i) ) {
      if (
        p->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local
        and
        marker.isContained( p->getX() )
        and
        fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp()>=IniRecord
        and
        fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp()<=LastRecord
      ) {
        _database.addParticleSnapshot( 
          p->getNumber(0), 
          p->getNumber(1),
          fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp(),
          p->getX(),
          p->getData().size(),
          p->getData().data()
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
  _database.setOutputFileName( "{{FILENAME}}" );
  _database.setOutputPrecision( {{OUTPUT_PRECISION}} );
  _database.setDataDeltaBetweenTwoSnapshots( {{DATA_DELTA}}, {{USE_RELATIVE_DELTAS}} );
  _database.setPositionDeltaBetweenTwoSnapshots( {{POSITION_DELTA}}, {{USE_RELATIVE_DELTAS}} );
  _database.setTimeDeltaBetweenTwoSnapshots( {{TIME_DELTA}} );
  _database.clearDatabaseAfterFlush( {{CLEAR_DATABASE_AFTER_FLUSH}} );
"""
        )
        return template.render(**self.d)

    def get_includes(self):
        result = jinja2.Template(
            """
#include "toolbox/particles/TrajectoryDatabase.h"
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
#include "peano4/parallel/Node.h"
#include "repositories/SolverRepository.h"
"""
        )
        return result.render(**self.d)

    def get_attributes(self):
        return """
    static toolbox::particles::TrajectoryDatabase  _database;
"""

    def get_static_initialisations(self, full_qualified_classname):
        return (
            """
toolbox::particles::TrajectoryDatabase  """
            + full_qualified_classname
            + """::_database( """
            + str(self.number_of_entries_between_two_db_flushes)
            + """);
"""
        )
