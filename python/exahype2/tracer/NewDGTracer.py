# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2


class NewDGTracer(ActionSet):
    """!

    Particle tracing over the DG solver.

    Please consult the class FiniteVolumesTracing for details on the semantics
    of this type. It is an exact analogon for DG.

    ## Update plug-in points

    We only update cells which are marked with getHasUpdated(). For enclave solvers,
    we assume that this flag is only set after a cell has updated. For Runge-Kutta
    type solvers, I assume that this flag is set by the computation of the final
    linear combination of estimates.
    """

    def __init__(
        self,
        coordinates,
        solver,
        filename,
        number_of_entries_between_two_db_flushes=65536,
        data_delta_between_two_snapshots=1e-8,
        position_delta_between_two_snapshots=1e-8,
        time_delta_between_two_snapshots=0,
        output_precision=8,
        clear_database_after_flush=True,
        use_relative_deltas=False,
        initial_record_time=-1,
        last_record_time=1e8,
        unknowns_to_store=0
    ):
        """

        @see FiniteVolumesTracing.__init()__.

        """
        super(NewDGTracer, self).__init__(descend_invocation_order=1, parallel=False)

        self._coordinates = coordinates

        self.d = {}
        # Don't overwrite hte dictionary of hte superclass. We'll need it
        self.d["SOLVER_NAME"] = solver._name
        self.d["SOLVER_INSTANCE"] = solver.get_name_of_global_instance()
        self.d["ORDER"] = solver._basis.order
        self.d["NUMBER_OF_UNKNOWNS"] = solver._unknowns
        self.d["NUMBER_OF_AUXILIARY_VARIABLES"] = solver._auxiliary_variables

        self.d["UNKNOWNS_TO_STORE"] = (unknowns_to_store if unknowns_to_store>0 else solver._unknowns)

        self.d["DATA_DELTA"] = data_delta_between_two_snapshots
        self.d["POSITION_DELTA"] = position_delta_between_two_snapshots
        self.d["TIME_DELTA"] = time_delta_between_two_snapshots
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

        self.d["N"] = len(coordinates)

        self.number_of_entries_between_two_db_flushes = (
            number_of_entries_between_two_db_flushes
        )



    __Template_TouchCellLastTime = jinja2.Template( """
  if ( 
    not marker.willBeRefined() 
    and 
    fineGridCell{{SOLVER_NAME}}CellLabel.getHasUpdated() 
    and
      (
        repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep()
      or
        repositories::{{SOLVER_INSTANCE}}.getSolverState()=={{SOLVER_NAME}}::SolverState::GridInitialisation
      )
  ) {

    double IniRecord={{INI_RECORD}};
    double LastRecord={{LAST_RECORD}};

    for(int i=0;i<{{N}};i++){
      // It is important to have this asymmetric comparisons with <= as we
      // need to ensure that particles right in the centre are either associated
      // with the vertex left or right.
      if (
        not marker.willBeRefined()
        and
        marker.isContained(_coordinates[i])
      ) {

        constexpr int unknownsToStore = {{UNKNOWNS_TO_STORE}};
        constexpr int unknownsPerDoF = {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}};
        
        double data[unknownsPerDoF];
        for (int unknown=0; unknown<unknownsPerDoF; unknown++) {
          data[unknown] = ::exahype2::dg::evaluatePolynomial(
              marker,
              {{ORDER}},
              repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
              unknownsPerDoF,
              fineGridCell{{SOLVER_NAME}}Q.value,
              _coordinates[i],
              unknown
            );
        }

        _database.addParticleSnapshot( 
          i, i,
          fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp(),
          _coordinates[i],
          unknownsToStore,
          data
        );

      }
    }
  }
""")

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
        """!

        Initialise the coordinates

        This routine initialises the coordinates array, but it does so if and only
        if this hasn't been done yet. Each tree traversal creates its own action
        set. However, it would be ridiculously stupid to create the coordinates
        multiple times. Rather, I

        """
        result = jinja2.Template(
            """
_database.setOutputFileName( "{{FILENAME}}" );
_database.setOutputPrecision( {{OUTPUT_PRECISION}} );
_database.setDataDeltaBetweenTwoSnapshots( {{DATA_DELTA}}, {{USE_RELATIVE_DELTAS}} );
_database.setPositionDeltaBetweenTwoSnapshots( {{POSITION_DELTA}}, {{USE_RELATIVE_DELTAS}} );
_database.setTimeDeltaBetweenTwoSnapshots( {{TIME_DELTA}} );
_database.clearDatabaseAfterFlush( {{CLEAR_DATABASE_AFTER_FLUSH}} );

_spacetreeId              = treeNumber;
  
#pragma clang optimize off
tarch::multicore::Lock lock (_coordinatesSemaphore);
if (_coordinates==nullptr) {
  _coordinates = new tarch::la::Vector<Dimensions,double>["""
            + str(self.d["N"])
            + """];

    """
        ).render(**self.d)
        dimensions = -1
        for i in range(self.d["N"]):
            if len(self._coordinates[i]) == 3:
                result += (
                    " _coordinates["
                    + str(i)
                    + "](0)="
                    + str(self._coordinates[i][0])
                    + ";\n"
                )
                result += (
                    " _coordinates["
                    + str(i)
                    + "](1)="
                    + str(self._coordinates[i][1])
                    + ";\n"
                )
                result += " #if Dimensions==3\n"
                result += (
                    " _coordinates["
                    + str(i)
                    + "](2)="
                    + str(self._coordinates[i][2])
                    + ";\n"
                )
                result += " #endif\n"
                result += "\n"
                assert (
                    dimensions == -1 or dimensions == 3
                ), "inconsistent data: all coordinates have to have three entries: {}".format(
                    self._coordinates[i]
                )
                dimensions = 3
            if len(self._coordinates[i]) == 2:
                result += (
                    " _coordinates["
                    + str(i)
                    + "](0)="
                    + str(self._coordinates[i][0])
                    + ";\n"
                )
                result += (
                    " _coordinates["
                    + str(i)
                    + "](1)="
                    + str(self._coordinates[i][1])
                    + ";\n"
                )
                result += " assertion(Dimensions==2);\n"
                result += "\n"
                assert (
                    dimensions == -1 or dimensions == 2
                ), "inconsistent data: all coordinates have to have three entries: {}".format(
                    self._coordinates[i]
                )
                dimensions = 2
        result += """
}
#pragma clang optimize on
"""
        return result

    def get_includes(self):
        return """
#include "tarch/multicore/Lock.h"
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h"
#include "exahype2/dg/Tracer.h"
#include "toolbox/particles/TrajectoryDatabase.h"
#include "peano4/parallel/Node.h"
"""

    def get_static_initialisations(self, full_qualified_classname):
        return (
            """
tarch::la::Vector<Dimensions,double>*  """
            + full_qualified_classname
            + """::_coordinates = nullptr;
tarch::multicore::BooleanSemaphore     """
            + full_qualified_classname
            + """::_coordinatesSemaphore;
toolbox::particles::TrajectoryDatabase  """
            + full_qualified_classname
            + """::_database( """
            + str(self.number_of_entries_between_two_db_flushes)
            + """);
"""
        )

    def get_attributes(self):
        return """
    static tarch::multicore::BooleanSemaphore     _coordinatesSemaphore;
    static tarch::la::Vector<Dimensions,double>*  _coordinates;
    
    int                                        _spacetreeId;
    double _timeStepSize;    

    static toolbox::particles::TrajectoryDatabase  _database;
"""