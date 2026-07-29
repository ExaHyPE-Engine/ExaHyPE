# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2


class CurviTracer(ActionSet):
    """!

    Record the DG solution at fixed coordinates of a curvilinear mesh.

    The coordinates are handed in as physical positions and are mapped into the
    mesh through the solver's invertProjection(). Whenever a cell holds one of
    these positions, we evaluate the DG polynomial there and append a snapshot to
    a toolbox::particles::TrajectoryDatabase.

    Fault coordinates are recorded from the face estimates. There the two
    estimates of a face are averaged, except for the components addressed by the
    solver's u and v shortcuts, which are recorded as the jump across the face.

    ## Update plug-in points

    We only record from cells and faces which are marked as updated, and from the
    grid initialisation. For Runge-Kutta type solvers, we assume that the updated
    flag is set by the computation of the final linear combination of estimates.
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

        Set up the recording.

        ## Arguments

        coordinates: [[Float]]
          Physical positions at which the solution is recorded. Each entry holds
          either two or three components, and all entries have to agree.

        solver: ADERDG
          The solver whose polynomial is evaluated. It has to offer
          invertProjection() to map a physical position into the mesh.

        filename: String
          Name of the database file, relative to the output path.

        fault_coordinates: [[Float]]
          Positions which are recorded from the face estimates. Their snapshots
          are appended to the database behind those of coordinates.

        position_delta_between_two_snapshots: Float
          Maps to the positionDelta argument of
          toolbox::particles::TrajectoryDatabase::TrajectoryDatabase().

        time_delta_between_two_snapshots: Float
          Maps to the timeDelta argument of
          toolbox::particles::TrajectoryDatabase::TrajectoryDatabase().

        output_precision: Int
          Number of digits the database writes per value.

        use_relative_deltas: Boolean
          See description of the C++ class toolbox::particles::TrajectoryDatabase.

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
        # Do not overwrite the dictionary of the superclass, as we need it.
        self.d["SOLVER_NAME"] = solver._name
        self.d["SOLVER_INSTANCE"] = solver.get_name_of_global_instance()
        self.d["ORDER"] = solver._basis.order
        self.d["NUMBER_OF_UNKNOWNS"] = solver.unknowns
        self.d["NUMBER_OF_AUXILIARY_VARIABLES"] = solver.auxiliary_variables

        self.d["UNKNOWNS_TO_STORE"] = (
            unknowns_to_store if unknowns_to_store > 0 else solver.unknowns
        )

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

        self.d["INI_RECORD"] = initial_record_time
        self.d["LAST_RECORD"] = last_record_time

        self.d["N"] = len(coordinates)

    __Template_TouchCellLastTime = jinja2.Template(
        """
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

    for(int i=0; i<_coordinates.size(); i++){
      // A coordinate has to be recorded by exactly one cell, as the database
      // would otherwise hold duplicate snapshots for the same time stamp.
      if (
        not marker.willBeRefined()
        and
        marker.isContained(_coordinates[i])
      ) {

        constexpr int unknownsToStore = {{UNKNOWNS_TO_STORE}};
        constexpr int unknownsPerDoF = {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}};

        double data[unknownsToStore];
        for (int unknown=0; unknown<unknownsToStore; unknown++) {
          data[unknown] = ::exahype2::dg::evaluatePolynomial(
              marker,
              {{ORDER}},
              repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
              unknownsPerDoF,
              fineGridCell{{SOLVER_NAME}}Q.data(),
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
"""
    )

    __Template_TouchFaceLastTime = jinja2.Template(
        """
  if (
    not marker.willBeRefined()
    and
    fineGridFace{{SOLVER_NAME}}FaceLabel.getUpdated()[0]
    and
      (
        repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep()
      or
        repositories::{{SOLVER_INSTANCE}}.getSolverState()=={{SOLVER_NAME}}::SolverState::GridInitialisation
      )
    and ((marker.getSelectedFaceNumber() % DIMENSIONS) == 2-repositories::{{SOLVER_INSTANCE}}.context->fault_normal) and
      std::abs(marker.getX()[2-repositories::{{SOLVER_INSTANCE}}.context->fault_normal]-repositories::{{SOLVER_INSTANCE}}.context->fault_position)
        < 0.5*marker.getH()[2-repositories::{{SOLVER_INSTANCE}}.context->fault_normal]
  ) {

    double IniRecord={{INI_RECORD}};
    double LastRecord={{LAST_RECORD}};

    const double timeStamp = std::max(
      fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(0),
      fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(1)
    );

    auto isContained = [&marker](tarch::la::Vector<DIMENSIONS, double> coordinates){
      const int normal = (marker.getSelectedFaceNumber() % DIMENSIONS);
      bool is_contained = true;
      for(int d=0; d<DIMENSIONS; d++){
        if(d==normal){continue;}
        is_contained &= coordinates[d] < marker.getX()[d] + 0.5*marker.getH()[d];
        is_contained &= coordinates[d] > marker.getX()[d] - 0.5*marker.getH()[d];
      }
      return is_contained;
    };

    for(int i=0; i<_fault_coordinates.size(); i++){
      // A fault coordinate has to be recorded by exactly one face, as the
      // database would otherwise hold duplicate snapshots for the same time
      // stamp.
      if (
        not marker.willBeRefined()
        and
        isContained(_fault_coordinates[i])
      ) {

        constexpr int unknownsToStore = {{UNKNOWNS_TO_STORE}};
        constexpr int unknownsPerDoF = {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}};

        constexpr int BasisElementsPerFace  = facedata::{{SOLVER_NAME}}QEstimates::Cardinality / 2;

        double data[unknownsPerDoF];
        for (int unknown=0; unknown<unknownsToStore; unknown++) {
          data[unknown] = 0.5 * (
            ::exahype2::dg::evaluatePolynomial(
              marker,
              {{ORDER}},
              repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
              unknownsPerDoF,
              fineGridFace{{SOLVER_NAME}}QEstimates.data() + BasisElementsPerFace,
              _fault_coordinates[i],
              unknown
            ) +
            ::exahype2::dg::evaluatePolynomial(
              marker,
              {{ORDER}},
              repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
              unknownsPerDoF,
              fineGridFace{{SOLVER_NAME}}QEstimates.data(),
              _fault_coordinates[i],
              unknown
            )
          );
        }

        using Shortcuts = VariableShortcuts{{SOLVER_NAME}};
        for(int unknown=Shortcuts::v; unknown<Shortcuts::v+3; unknown++) {
          data[unknown] = ::exahype2::dg::evaluatePolynomial(
              marker,
              {{ORDER}},
              repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
              unknownsPerDoF,
              fineGridFace{{SOLVER_NAME}}QEstimates.data() + BasisElementsPerFace,
              _fault_coordinates[i],
              unknown
            ) - ::exahype2::dg::evaluatePolynomial(
              marker,
              {{ORDER}},
              repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
              unknownsPerDoF,
              fineGridFace{{SOLVER_NAME}}QEstimates.data(),
              _fault_coordinates[i],
              unknown
            );
        }

        for(int unknown=Shortcuts::v; unknown<Shortcuts::v+3; unknown++) {
          data[unknown] = ::exahype2::dg::evaluatePolynomial(
              marker,
              {{ORDER}},
              repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
              unknownsPerDoF,
              fineGridFace{{SOLVER_NAME}}QEstimates.data() + BasisElementsPerFace,
              _fault_coordinates[i],
              unknown
            ) - ::exahype2::dg::evaluatePolynomial(
              marker,
              {{ORDER}},
              repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
              unknownsPerDoF,
              fineGridFace{{SOLVER_NAME}}QEstimates.data(),
              _fault_coordinates[i],
              unknown
            );
        }
        for(int unknown=Shortcuts::u; unknown<Shortcuts::u+3; unknown++) {
          data[unknown] = ::exahype2::dg::evaluatePolynomial(
              marker,
              {{ORDER}},
              repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
              unknownsPerDoF,
              fineGridFace{{SOLVER_NAME}}QEstimates.data() + BasisElementsPerFace,
              _fault_coordinates[i],
              unknown
            ) - ::exahype2::dg::evaluatePolynomial(
              marker,
              {{ORDER}},
              repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
              unknownsPerDoF,
              fineGridFace{{SOLVER_NAME}}QEstimates.data(),
              _fault_coordinates[i],
              unknown
          );
        }

        _database.addParticleSnapshot(
          i+{{N}}, i+{{N}},
          timeStamp,
          _fault_coordinates[i],
          unknownsToStore,
          data
        );

      }
    }
  }
"""
    )

    def get_body_of_operation(self, operation_name):
        result = ""
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
            result = self.__Template_TouchCellLastTime.render(**self.d)
        if (
            operation_name == ActionSet.OPERATION_TOUCH_FACE_LAST_TIME
            and len(self._fault_coordinates) > 0
        ):
            result = self.__Template_TouchFaceLastTime.render(**self.d)
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_constructor_body(self):
        result = """
_database.setOutputFileName( ::runner::RuntimeParameters::getInstance().getOutputPath() + "{{FILENAME}}" );
_database.setOutputPrecision( {{OUTPUT_PRECISION}} );
_database.setPositionDeltaBetweenTwoSnapshots( {{POSITION_DELTA}}, {{USE_RELATIVE_DELTAS}} );
_database.setTimeDeltaBetweenTwoSnapshots( {{TIME_DELTA}} );

_spacetreeId = treeNumber;

tarch::multicore::Lock lock (_coordinatesSemaphore);
if(_coordinates.empty()){
    """
        dimensions = -1
        for i in range(len(self._coordinates)):
            if len(self._coordinates[i]) == 3:
                result += (
                    " _coordinates.push_back(repositories::{{SOLVER_INSTANCE}}.invertProjection(\n"
                    + "    {\n      "
                    + str(self._coordinates[i][0])
                    + ", "
                    + str(self._coordinates[i][1])
                    + "\n"
                )
                result += " #if DIMENSIONS==3\n"
                result += "      ," + str(self._coordinates[i][2]) + "\n"
                result += " #endif\n"
                result += "    }\n"
                result += "  ));\n"
                assert (
                    dimensions == -1 or dimensions == 3
                ), "Inconsistent data: all coordinates must have three entries: {}".format(
                    self._coordinates[i]
                )
                dimensions = 3
            if len(self._coordinates[i]) == 2:
                result += (
                    " _coordinates.push_back(repositories::{{SOLVER_INSTANCE}}.invertProjection(\n"
                    + "    {\n      "
                    + str(self._coordinates[i][0])
                    + ", "
                    + str(self._coordinates[i][1])
                    + "\n"
                )
                result += "    }\n"
                result += "  ));\n"
                assert (
                    dimensions == -1 or dimensions == 2
                ), "Inconsistent data: all coordinates must have two entries: {}".format(
                    self._coordinates[i]
                )
                dimensions = 2
        result += """
}
if(_fault_coordinates.empty()){
    """
        dimensions = -1
        for i in range(len(self._fault_coordinates)):
            if len(self._fault_coordinates[i]) == 3:
                result += (
                    " _fault_coordinates.push_back(repositories::{{SOLVER_INSTANCE}}.invertProjection(\n"
                    + "    {\n      "
                    + str(self._fault_coordinates[i][0])
                    + ", "
                    + str(self._fault_coordinates[i][1])
                    + "\n"
                )
                result += " #if DIMENSIONS==3\n"
                result += "      ," + str(self._fault_coordinates[i][2]) + "\n"
                result += " #endif\n"
                result += "    }\n"
                result += "  ));\n"
                assert (
                    dimensions == -1 or dimensions == 3
                ), "Inconsistent data: all fault coordinates must have three entries: {}".format(
                    self._fault_coordinates[i]
                )
                dimensions = 3
            if len(self._fault_coordinates[i]) == 2:
                result += (
                    " _fault_coordinates.push_back(repositories::{{SOLVER_INSTANCE}}.invertProjection(\n"
                    + "    {\n      "
                    + str(self._fault_coordinates[i][0])
                    + ", "
                    + str(self._fault_coordinates[i][1])
                    + "\n"
                )
                result += "    }\n"
                result += "  ));\n"
                assert (
                    dimensions == -1 or dimensions == 2
                ), "Inconsistent data: all fault coordinates must have two entries: {}".format(
                    self._fault_coordinates[i]
                )
                dimensions = 2
        result += """
}
#pragma clang optimize on\n
"""
        return jinja2.Template(result).render(**self.d)

    def get_includes(self):
        return jinja2.Template(
            """
#include "tarch/multicore/Lock.h"
#include "peano4/Loop.h"
#include "repositories/SolverRepository.h"
#include "runner/RuntimeParameters.h"
#include "exahype2/dg/Tracer.h"
#include "toolbox/particles/TrajectoryDatabase.h"
#include "VariableShortcuts{{SOLVER_NAME}}.h"
        """
        ).render(**self.d)

    def get_static_initialisations(self, full_qualified_classname):
        return (
            """
std::vector<tarch::la::Vector<DIMENSIONS,double>>  """
            + full_qualified_classname
            + """::_coordinates;
std::vector<tarch::la::Vector<DIMENSIONS,double>>  """
            + full_qualified_classname
            + """::_fault_coordinates;
tarch::multicore::BooleanSemaphore     """
            + full_qualified_classname
            + """::_coordinatesSemaphore;
toolbox::particles::TrajectoryDatabase  """
            + full_qualified_classname
            + """::_database;
"""
        )

    def get_attributes(self):
        return """
    static tarch::multicore::BooleanSemaphore                 _coordinatesSemaphore;
    static std::vector<tarch::la::Vector<DIMENSIONS,double>>  _coordinates;
    static std::vector<tarch::la::Vector<DIMENSIONS,double>>  _fault_coordinates;

    int                                        _spacetreeId;
    double _timeStepSize;

    static toolbox::particles::TrajectoryDatabase  _database;

  public:
    static std::vector<tarch::la::Vector<DIMENSIONS,double>> getCoordinates(){
      return _coordinates;
    }
"""
