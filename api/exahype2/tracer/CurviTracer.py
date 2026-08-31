# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2


class CurviTracer(ActionSet):
    """!

    Record the DG solution at fixed coordinates of a curvilinear mesh.

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
        # Do not overwrite the dictionary of the superclass.
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
  const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

  if (
    not marker.willBeRefined()
    and
    fineGridCell{{SOLVER_NAME}}CellLabel.getHasUpdated()
    and
    timeStamp >= {{INI_RECORD}}
    and
    timeStamp <= {{LAST_RECORD}}
    and
      (
        repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep()
      or
        repositories::{{SOLVER_INSTANCE}}.getSolverState()=={{SOLVER_NAME}}::SolverState::GridInitialisation
      )
  ) {
    for (int i=0; i<(int) _coordinates.size(); i++) {
      // A coordinate has to be recorded by exactly one cell, as the database
      // would otherwise hold duplicate snapshots for the same time stamp.
      if (
        not marker.willBeRefined()
        and
        marker.isContained(_coordinates[i])
      ) {
        constexpr int UnknownsToStore = {{UNKNOWNS_TO_STORE}};
        constexpr int UnknownsPerDoF = {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}};

        double data[UnknownsToStore];
        for (int unknown=0; unknown<UnknownsToStore; unknown++) {
          data[unknown] = ::exahype2::dg::evaluatePolynomial(
              marker,
              {{ORDER}},
              repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
              UnknownsPerDoF,
              fineGridCell{{SOLVER_NAME}}Q.data(),
              _coordinates[i],
              unknown
            );
        }

        _database.addParticleSnapshot(
          i, i,
          timeStamp,
          _coordinates[i],
          UnknownsToStore,
          data
        );
      }
    }
  }
"""
    )

    __Template_TouchFaceLastTime = jinja2.Template(
        """
  const double timeStamp = std::max(
    fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(0),
    fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(1)
  );

  if (
    not marker.willBeRefined()
    and
    fineGridFace{{SOLVER_NAME}}FaceLabel.getUpdated()[0]
    and
    timeStamp >= {{INI_RECORD}}
    and
    timeStamp <= {{LAST_RECORD}}
    and
      (
        repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep()
      or
        repositories::{{SOLVER_INSTANCE}}.getSolverState()=={{SOLVER_NAME}}::SolverState::GridInitialisation
      )
    and ((marker.getSelectedFaceNumber() % DIMENSIONS) == repositories::{{SOLVER_INSTANCE}}.getFaultNormal()) and
      std::abs(marker.getX()[repositories::{{SOLVER_INSTANCE}}.getFaultNormal()]-repositories::{{SOLVER_INSTANCE}}.getFaultPosition())
        < 0.5*marker.getH()[repositories::{{SOLVER_INSTANCE}}.getFaultNormal()]
  ) {
    auto isContained = [&marker](const tarch::la::Vector<DIMENSIONS, double>& coordinates) {
      const int normal = (marker.getSelectedFaceNumber() % DIMENSIONS);
      bool isInside = true;
      for (int d=0; d<DIMENSIONS; d++) {
        if (d==normal) {
          continue;
        }
        isInside &= coordinates[d] < marker.getX()[d] + 0.5*marker.getH()[d];
        isInside &= coordinates[d] > marker.getX()[d] - 0.5*marker.getH()[d];
      }
      return isInside;
    };

    for (int i=0; i<(int) _faultCoordinates.size(); i++) {
      // A fault coordinate has to be recorded by exactly one face, as the
      // database would otherwise hold duplicate snapshots for the same time
      // stamp.
      if (
        not marker.willBeRefined()
        and
        isContained(_faultCoordinates[i])
      ) {
        constexpr int UnknownsToStore = {{UNKNOWNS_TO_STORE}};
        constexpr int UnknownsPerDoF = {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}};

        constexpr int BasisElementsPerFace  = facedata::{{SOLVER_NAME}}QEstimates::Cardinality / 2;

        double data[UnknownsPerDoF];
        for (int unknown=0; unknown<UnknownsToStore; unknown++) {
          data[unknown] = 0.5 * (double) (
            ::exahype2::dg::evaluatePolynomial(
              marker,
              {{ORDER}},
              repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
              UnknownsPerDoF,
              fineGridFace{{SOLVER_NAME}}QEstimates.data() + BasisElementsPerFace,
              _faultCoordinates[i],
              unknown
            ) +
            ::exahype2::dg::evaluatePolynomial(
              marker,
              {{ORDER}},
              repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
              UnknownsPerDoF,
              fineGridFace{{SOLVER_NAME}}QEstimates.data(),
              _faultCoordinates[i],
              unknown
            )
          );
        }

        using Shortcuts = VariableShortcuts{{SOLVER_NAME}};
        for (int unknown=Shortcuts::v; unknown<Shortcuts::v+3; unknown++) {
          data[unknown] = (double) (::exahype2::dg::evaluatePolynomial(
              marker,
              {{ORDER}},
              repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
              UnknownsPerDoF,
              fineGridFace{{SOLVER_NAME}}QEstimates.data() + BasisElementsPerFace,
              _faultCoordinates[i],
              unknown
            ) - ::exahype2::dg::evaluatePolynomial(
              marker,
              {{ORDER}},
              repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
              UnknownsPerDoF,
              fineGridFace{{SOLVER_NAME}}QEstimates.data(),
              _faultCoordinates[i],
              unknown
            ));
        }

        for (int unknown=Shortcuts::u; unknown<Shortcuts::u+3; unknown++) {
          data[unknown] = (double) (::exahype2::dg::evaluatePolynomial(
              marker,
              {{ORDER}},
              repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
              UnknownsPerDoF,
              fineGridFace{{SOLVER_NAME}}QEstimates.data() + BasisElementsPerFace,
              _faultCoordinates[i],
              unknown
            ) - ::exahype2::dg::evaluatePolynomial(
              marker,
              {{ORDER}},
              repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
              UnknownsPerDoF,
              fineGridFace{{SOLVER_NAME}}QEstimates.data(),
              _faultCoordinates[i],
              unknown
          ));
        }

        _database.addParticleSnapshot(
          i+{{N}}, i+{{N}},
          timeStamp,
          _faultCoordinates[i],
          UnknownsToStore,
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

_treeId = treeNumber;

const std::scoped_lock lock(_coordinatesMutex);
if (_coordinates.empty()) {
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
if (_faultCoordinates.empty()) {
    """
        dimensions = -1
        for i in range(len(self._fault_coordinates)):
            if len(self._fault_coordinates[i]) == 3:
                result += (
                    " _faultCoordinates.push_back(repositories::{{SOLVER_INSTANCE}}.invertProjection(\n"
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
                    " _faultCoordinates.push_back(repositories::{{SOLVER_INSTANCE}}.invertProjection(\n"
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
"""
        return jinja2.Template(result).render(**self.d)

    def get_includes(self):
        return jinja2.Template(
            """
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
            + """::_faultCoordinates;
std::mutex     """
            + full_qualified_classname
            + """::_coordinatesMutex;
toolbox::particles::TrajectoryDatabase  """
            + full_qualified_classname
            + """::_database;
"""
        )

    def get_attributes(self):
        return """
    static std::mutex                                         _coordinatesMutex;
    static std::vector<tarch::la::Vector<DIMENSIONS,double>>  _coordinates;
    static std::vector<tarch::la::Vector<DIMENSIONS,double>>  _faultCoordinates;

    int                                        _treeId;

    static toolbox::particles::TrajectoryDatabase  _database;

  public:
    static std::vector<tarch::la::Vector<DIMENSIONS,double>> getCoordinates() {
      return _coordinates;
    }
"""
