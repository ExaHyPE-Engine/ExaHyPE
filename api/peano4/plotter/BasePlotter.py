# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from peano4.solversteps.ActionSet import ActionSet

import jinja2


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
    # One growing <name>.vtkhdf file per plotter; ParaView reads it natively as a
    # time series. Requires HDF5 (falls back to VTU otherwise).
    "vtkhdf": {
        "FILE_WRITER": "VTKHDFFileWriter",
        "TIME_SERIES_WRITER": "VTKHDFTimeSeriesWriter",
        "OUTPUT_FORMAT_INCLUDES": """
#include "tarch/plotter/VTKHDFFileWriter.h"
#include "tarch/plotter/VTKHDFTimeSeriesWriter.h"
""",
    },
}


# Requestable output formats, in a stable order. The command-line choices and
# their validation (see peano4.Project) read from this list.
SUPPORTED_FORMATS = tuple(_FORMATS.keys())


# Project-wide default output format. A plotter built with file_format=None binds
# to this at code-generation time, so --plot-format or project.set_plot_format()
# switches every plotter at once.
_DEFAULT_FORMAT = "vtkhdf"


def set_default_format(file_format):
    """Set the project-wide default output format (one of _FORMATS)."""
    if file_format not in _FORMATS:
        raise ValueError(
            "Unsupported plot file format: {} (choose one of {})".format(
                file_format, ", ".join(sorted(_FORMATS))
            )
        )
    global _DEFAULT_FORMAT
    _DEFAULT_FORMAT = file_format


def get_default_format():
    """Return the project-wide default output format."""
    return _DEFAULT_FORMAT


_AVAILABLE_FORMATS = None


def set_available_formats(formats):
    """Record which output formats this build supports (an iterable of names)."""
    global _AVAILABLE_FORMATS
    _AVAILABLE_FORMATS = set(formats)


def _resolve_available_format(file_format):
    """Map a requested format onto one this build actually provides.

    VTU is always available, so an HDF5/NetCDF request on a build configured
    without those libraries falls back to VTU instead of failing to link.
    """
    if _AVAILABLE_FORMATS is None or file_format in _AVAILABLE_FORMATS:
        return file_format
    fallback = "vtu"
    try:
        from peano4.Logger import log

        log.warning(
            "Plot format '%s' is not available in this build; falling back to '%s'",
            file_format,
            fallback,
        )
    except Exception:
        pass
    return fallback


class BasePlotter(ActionSet):
    def __init__(
        self,
        filename,
        dataset_name,
        description,
        time_stamp_evaluation,
        guard,
        file_format=None,
    ):
        super().__init__(descend_invocation_order=1)

        self.d = {}
        self.d["NAME"] = dataset_name
        self.d["FILENAME"] = filename
        self.d["GUARD_PREDICATE"] = guard
        self.d["TIMESTAMP"] = time_stamp_evaluation
        self.d["DESCRIPTION"] = description
        # Entity plotters set this to "patch/", "face/" or "particle/".
        self.d["OUTPUT_SUBFOLDER"] = ""
        self._set_output_format(file_format)

    def _set_output_format(self, file_format):
        """
        Record the requested output format.

        A concrete format (hdf5/vtu/netcdf) is validated eagerly;
        file_format=None defers to the project default and is resolved only at
        code-generation time (see _ensure_format_resolved), so
        project.set_plot_format() still takes effect when called after the
        plotter has been constructed.
        """
        if file_format is not None and file_format not in _FORMATS:
            raise ValueError(
                "Unsupported plot file format: {} (choose one of {})".format(
                    file_format, ", ".join(sorted(_FORMATS))
                )
            )
        self._file_format = file_format
        self._format_resolved = False

    def _ensure_format_resolved(self):
        """Bind the per-format writer/header strings into self.d exactly once."""
        if self._format_resolved:
            return
        file_format = self._file_format
        if file_format is None:
            file_format = get_default_format()
        file_format = _resolve_available_format(file_format)
        self.d.update(_FORMATS[file_format])
        self._format_resolved = True

    def get_constructor_body(self):
        return """
  _treeNumber = treeNumber;
  _localTreeNumber = ::peano4::parallel::TreeIdMapping::getLocalTreeId(treeNumber);
""".format(
            **self.d
        )

    def get_destructor_body(self):
        return ""

    def get_body_of_getGridControlEvents(self):
        return "return std::vector<peano4::grid::GridControlEvent>();\n"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_body_of_prepareTraversal(self):
        self._ensure_format_resolved()
        return jinja2.Template(
            """
  // Create once: it manages all time steps of the whole simulation
  if (_timeSeriesWriter == nullptr) {
    _timeSeriesWriter = std::make_unique<tarch::plotter::{{TIME_SERIES_WRITER}}>();
  }

  // Recreate to track the spacetree count under dynamic load balancing
  const auto localSpacetrees = ::peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees();
  const int numberOfLocalSubdomains = localSpacetrees.empty()
    ? 0
    : (::peano4::parallel::TreeIdMapping::getLocalTreeId(*localSpacetrees.rbegin()) + 1);
  _fileWriter.reset();
  _fileWriter = std::make_unique<tarch::plotter::{{FILE_WRITER}}>(::runner::RuntimeParameters::getInstance().getOutputPath() + "{{OUTPUT_SUBFOLDER}}", numberOfLocalSubdomains);

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
  _timeSeriesWriter->writeTimeSeriesMetaFile(::runner::RuntimeParameters::getInstance().getOutputPath() + "{{OUTPUT_SUBFOLDER}}", "{{FILENAME}}");
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
        self._ensure_format_resolved()
        return (
            """
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/FileWriter.h"
#include "tarch/plotter/TimeSeriesWriter.h"
#include "peano4/Loop.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "peano4/parallel/TreeIdMapping.h"
#include "runner/RuntimeParameters.h"
"""
            + self.d["OUTPUT_FORMAT_INCLUDES"]
        )

    def get_static_initialisations(self, full_qualified_classname):
        return f"""
double {full_qualified_classname}::_time = 0.0;
std::unique_ptr<tarch::plotter::FileWriter> {full_qualified_classname}::_fileWriter = nullptr;
std::unique_ptr<tarch::plotter::TimeSeriesWriter> {full_qualified_classname}::_timeSeriesWriter = nullptr;
"""
