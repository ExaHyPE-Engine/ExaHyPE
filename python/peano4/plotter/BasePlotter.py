# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from peano4.solversteps.ActionSet import ActionSet

import jinja2


class BasePlotter(ActionSet):
    def __init__(
        self,
        output_path,
        filename,
        dataset_name,
        description,
        time_stamp_evaluation,
        guard,
    ):
        super().__init__(descend_invocation_order=1)

        self.d = {}
        self.d["NAME"] = dataset_name
        self.d["OUTPUT_PATH"] = output_path
        self.d["FILENAME"] = filename
        self.d["GUARD_PREDICATE"] = guard
        self.d["TIMESTAMP"] = time_stamp_evaluation
        self.d["DESCRIPTION"] = description

    def get_constructor_body(self):
        return """
  _treeNumber = treeNumber;
  _localTreeNumber = ::peano4::parallel::Node::getInstance().getLocalTreeId(treeNumber);
""".format(
            **self.d
        )

    def set_output_path(self, output_path):
        self.d["OUTPUT_PATH"] = output_path

    def get_destructor_body(self):
        return ""

    def get_body_of_getGridControlEvents(self):
        return "return std::vector<peano4::grid::GridControlEvent>();\n"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_body_of_prepareTraversal(self):
        return jinja2.Template(
            """
  // Only create it once as it manages all time steps throughout the whole simulation
  if (_timeSeriesWriter == nullptr) {
    _timeSeriesWriter = std::make_unique<tarch::plotter::{{TIME_SERIES_WRITER}}>();
  }

  // Recreate it to adjust the number of spacetrees in case of dynamic load balancing
  const auto localSpacetrees = ::peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees();
  const int numberOfLocalSubdomains = localSpacetrees.empty()
    ? 0
    : (::peano4::parallel::Node::getInstance().getLocalTreeId(*localSpacetrees.rbegin()) + 1);
  _fileWriter.reset();
  _fileWriter = std::make_unique<tarch::plotter::{{FILE_WRITER}}>(::exahype2::RuntimeParameters::getInstance().getPlotOutputPath(), numberOfLocalSubdomains);

  _time = {{TIMESTAMP}};
"""
        ).render(**self.d)

    def get_body_of_unprepareTraversal(self):
        return jinja2.Template(
            """
  assertion(_fileWriter != nullptr);
  assertion(_fileWriter->isOpen());
  assertion(_timeSeriesWriter != nullptr);

  _timeSeriesWriter->writeSnapshotFile("{{FILENAME}}", _time, *_fileWriter);
  _timeSeriesWriter->writeTimeSeriesMetaFile(::exahype2::RuntimeParameters::getInstance().getPlotOutputPath(), "{{FILENAME}}");
"""
        ).render(**self.d)

    def get_body_of_operation(self, operation_name):
        result = "\n"

        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            result = jinja2.Template(
                """
  _subdomainPlotter.reset();
  _subdomainPlotter = _fileWriter->createCellDataPlotter(_localTreeNumber, "Subdomain", 1);

  _refinementLevelPlotter.reset();
  _refinementLevelPlotter = _fileWriter->createCellDataPlotter(_localTreeNumber, "RefinementLevel", 1);
"""
            ).render(**self.d)

        return result

    def get_attributes(self):
        return """
  int _treeNumber;
  int _localTreeNumber;

  static double _time;
  static std::unique_ptr<tarch::plotter::FileWriter> _fileWriter;
  static std::unique_ptr<tarch::plotter::TimeSeriesWriter> _timeSeriesWriter;

  std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> _subdomainPlotter;
  std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> _refinementLevelPlotter;
"""

    def get_includes(self):
        return """
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/FileWriter.h"
#include "tarch/plotter/TimeSeriesWriter.h"
#include "peano4/utils/Loop.h"
#include "peano4/parallel/Node.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "exahype2/RuntimeParameters.h"
"""

    def get_static_initialisations(self, full_qualified_classname):
        return f"""
double {full_qualified_classname}::_time = 0.0;
std::unique_ptr<tarch::plotter::FileWriter> {full_qualified_classname}::_fileWriter = nullptr;
std::unique_ptr<tarch::plotter::TimeSeriesWriter> {full_qualified_classname}::_timeSeriesWriter = nullptr;
"""
