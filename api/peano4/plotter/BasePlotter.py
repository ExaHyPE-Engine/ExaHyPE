# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import jinja2

from peano4.Logger import log
from peano4.solversteps.ActionSet import ActionSet


_FORMATS = {
    "hdf5": {
        "FILE_WRITER": "HDF5FileWriter",
        "TIME_SERIES_WRITER": "XDMFTimeSeriesWriter",
        "OUTPUT_FORMAT_INCLUDES": """
#include "tarch/plotter/HDF5FileWriter.h"
#include "tarch/plotter/XDMFTimeSeriesWriter.h"
""",
    },
    "vtu": {
        "FILE_WRITER": "VTUFileWriter",
        "TIME_SERIES_WRITER": "PVDTimeSeriesWriter",
        "OUTPUT_FORMAT_INCLUDES": """
#include "tarch/plotter/VTUFileWriter.h"
#include "tarch/plotter/PVDTimeSeriesWriter.h"
""",
    },
    "netcdf": {
        "FILE_WRITER": "NetCDFFileWriter",
        "TIME_SERIES_WRITER": "JSONTimeSeriesWriter",
        "OUTPUT_FORMAT_INCLUDES": """
#include "tarch/plotter/NetCDFFileWriter.h"
#include "tarch/plotter/JSONTimeSeriesWriter.h"
""",
    },
    "vtkhdf": {
        "FILE_WRITER": "VTKHDFFileWriter",
        "TIME_SERIES_WRITER": "VTKHDFTimeSeriesWriter",
        "OUTPUT_FORMAT_INCLUDES": """
#include "tarch/plotter/VTKHDFFileWriter.h"
#include "tarch/plotter/VTKHDFTimeSeriesWriter.h"
""",
    },
}

# The tuple order controls how argparse presents the public format choices.
SUPPORTED_FORMATS = tuple(_FORMATS.keys())
_DEFAULT_FORMAT = "vtkhdf"
_AVAILABLE_FORMATS = None


def set_default_format(file_format):
    """Sets the output format used by plotters without an explicit format."""
    if file_format not in _FORMATS:
        raise ValueError(
            'Plot file format "{}" is unknown. The known formats are {}.'.format(
                file_format, ", ".join(sorted(_FORMATS))
            )
        )
    global _DEFAULT_FORMAT
    _DEFAULT_FORMAT = file_format


def get_default_format():
    """Returns the output format used by plotters without an explicit format."""
    return _DEFAULT_FORMAT


def set_available_formats(formats):
    """Records the output formats enabled by the generated build."""
    global _AVAILABLE_FORMATS
    _AVAILABLE_FORMATS = set(formats)


def _resolve_available_format(file_format):
    """Returns file_format if the build offers it, and VTU otherwise."""
    if _AVAILABLE_FORMATS is None or file_format in _AVAILABLE_FORMATS:
        return file_format
    fallback_format = "vtu"
    log.warning(
        "This build offers no plot format '%s', so the plotter writes '%s'.",
        file_format,
        fallback_format,
    )
    return fallback_format


class BasePlotter(ActionSet):
    """! One file writer serves all trees of a plotter during a traversal, and it.

    is recreated with the current number of local trees, since load balancing can
    change that number between snapshots.

    The three additional_cell_data arguments inject member declarations, traversal
    initialisation and cell plot calls, and the cell-plot snippet runs with
    cellIndex in scope.

    When file_format is None, the project default is resolved during code
    generation, and a backend that the generated build does not offer falls back
    to VTU.
    """

    def __init__(
        self,
        filename,
        dataset_name,
        description,
        time_stamp_evaluation,
        guard,
        file_format=None,
        additional_cell_data_declarations="",
        additional_cell_data_construction="",
        additional_cell_data_plots="",
    ):
        super().__init__(descend_invocation_order=1)
        self._additional_cell_data_declarations = additional_cell_data_declarations
        self._additional_cell_data_construction = additional_cell_data_construction
        self._additional_cell_data_plots = additional_cell_data_plots
        self._dictionary = {
            "NAME": dataset_name,
            "FILENAME": filename,
            "GUARD_PREDICATE": guard,
            "TIMESTAMP": time_stamp_evaluation,
            "DESCRIPTION": description,
            "OUTPUT_SUBFOLDER": "",
        }
        self.d = self._dictionary
        self._set_output_format(file_format)

    def _set_output_format(self, file_format):
        """Validates an explicit format and defers None to code generation."""
        if file_format is not None and file_format not in _FORMATS:
            raise ValueError(
                'Plot file format "{}" is unknown. The known formats are {}.'.format(
                    file_format, ", ".join(sorted(_FORMATS))
                )
            )
        self._file_format = file_format
        self._format_resolved = False

    def _ensure_format_resolved(self):
        """Binds the selected writer types and includes once."""
        if self._format_resolved:
            return
        resolved_format = self._file_format
        if resolved_format is None:
            resolved_format = get_default_format()
        resolved_format = _resolve_available_format(resolved_format)
        self._dictionary.update(_FORMATS[resolved_format])
        self._format_resolved = True

    def get_constructor_body(self):
        return """
  _treeNumber = treeNumber;
"""

    def get_destructor_body(self):
        return ""

    def get_body_of_getGridControlEvents(self):
        return "  return std::vector<peano4::grid::GridControlEvent>{};\n"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_body_of_prepareTraversal(self):
        self._ensure_format_resolved()
        return jinja2.Template(
            """
  if (_timeSeriesWriter == nullptr) {
    _timeSeriesWriter = std::make_unique<tarch::plotter::{{TIME_SERIES_WRITER}}>();
  }

  _fileWriter.reset();
  _fileWriter = std::make_unique<tarch::plotter::{{FILE_WRITER}}>(::runner::RuntimeParameters::getInstance().getOutputPath() + "{{OUTPUT_SUBFOLDER}}");

  _time = {{TIMESTAMP}};
"""
        ).render(**self._dictionary)

    def get_body_of_unprepareTraversal(self):
        return jinja2.Template(
            """
  assertion(_fileWriter);
  assertion(_fileWriter->isOpen());
  assertion(_timeSeriesWriter);

  _timeSeriesWriter->writeSnapshotFile("{{FILENAME}}", _time, *_fileWriter);
  _timeSeriesWriter->writeTimeSeriesMetaFile(
    ::runner::RuntimeParameters::getInstance().getOutputPath() + "{{OUTPUT_SUBFOLDER}}", "{{FILENAME}}"
  );
"""
        ).render(**self._dictionary)

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            result = (
                jinja2.Template(
                    """
  // The tree number orders the pieces within the file deterministically,
  // whatever order concurrent traversals reach this point in.
  _pieceId = _fileWriter->acquirePiece(_treeNumber);

  _subdomainPlotter.reset();
  _subdomainPlotter = _fileWriter->createCellDataPlotter(_pieceId, "Subdomain", 1);

  _subtreeTaskPlotter.reset();
  _subtreeTaskPlotter = _fileWriter->createCellDataPlotter(_pieceId, "Subtree", 1);

  _refinementLevelPlotter.reset();
  _refinementLevelPlotter = _fileWriter->createCellDataPlotter(_pieceId, "RefinementLevel", 1);
"""
                ).render(**self._dictionary)
                + self.additional_cell_data_construction()
            )
        return result

    def additional_cell_data_declarations(self):
        return self._additional_cell_data_declarations

    def additional_cell_data_construction(self):
        return self._additional_cell_data_construction

    def additional_cell_data_plots(self):
        return self._additional_cell_data_plots

    def get_attributes(self):
        return (
            """
  int _treeNumber{-1};
  int _pieceId{-1};

  static double                                            _time;
  static std::unique_ptr<tarch::plotter::FileWriter>       _fileWriter;
  static std::unique_ptr<tarch::plotter::TimeSeriesWriter> _timeSeriesWriter;

  std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> _subdomainPlotter{};
  std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> _subtreeTaskPlotter{};
  std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> _refinementLevelPlotter{};

  /**
   * Returns the tree id of the subtree task that emitted a cell. A task's tree
   * id is the rank plus a multiple of the rank count, so no two tasks of two
   * ranks carry the same id. A cell whose Subtree equals its Subdomain comes
   * from a sweep that did not decompose.
   */
  INLINE_METHOD int getSubtreeId() const { return _treeNumber; }
"""
            + self.additional_cell_data_declarations()
        )

    def get_includes(self):
        self._ensure_format_resolved()
        return (
            """
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/FileWriter.h"
#include "tarch/plotter/TimeSeriesWriter.h"

#include "peano4/Loop.h"
#include "runner/RuntimeParameters.h"
"""
            + self._dictionary["OUTPUT_FORMAT_INCLUDES"]
        )

    def get_static_initialisations(self, full_qualified_classname):
        return f"""
double                                            {full_qualified_classname}::_time             = 0.0;
std::unique_ptr<tarch::plotter::FileWriter>       {full_qualified_classname}::_fileWriter       = nullptr;
std::unique_ptr<tarch::plotter::TimeSeriesWriter> {full_qualified_classname}::_timeSeriesWriter = nullptr;
"""
