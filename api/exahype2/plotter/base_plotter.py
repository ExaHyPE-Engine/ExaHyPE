# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from exahype2.actionsets.action_set import ActionSet
from exahype2.log import log

_FORMATS = {
    "hdf5": {
        "FILE_WRITER": "HDF5FileWriter",
        "TIME_SERIES_WRITER": "XDMFTimeSeriesWriter",
        "INCLUDES": [
            "tarch/plotter/HDF5FileWriter.h",
            "tarch/plotter/XDMFTimeSeriesWriter.h",
        ],
    },
    "vtu": {
        "FILE_WRITER": "VTUFileWriter",
        "TIME_SERIES_WRITER": "PVDTimeSeriesWriter",
        "INCLUDES": [
            "tarch/plotter/PVDTimeSeriesWriter.h",
            "tarch/plotter/VTUFileWriter.h",
        ],
    },
    "netcdf": {
        "FILE_WRITER": "NetCDFFileWriter",
        "TIME_SERIES_WRITER": "JSONTimeSeriesWriter",
        "INCLUDES": [
            "tarch/plotter/JSONTimeSeriesWriter.h",
            "tarch/plotter/NetCDFFileWriter.h",
        ],
    },
    "vtkhdf": {
        "FILE_WRITER": "VTKHDFFileWriter",
        "TIME_SERIES_WRITER": "VTKHDFTimeSeriesWriter",
        "INCLUDES": [
            "tarch/plotter/VTKHDFFileWriter.h",
            "tarch/plotter/VTKHDFTimeSeriesWriter.h",
        ],
    },
}

SUPPORTED_FORMATS = tuple(_FORMATS.keys())
_DEFAULT_FORMAT = "vtkhdf"


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


def data_field_list(fields):
    """Returns the C++ initialiser list of ``exahype2::plotter::DataField`` for name and
    component pairs."""
    entries = ", ".join(
        '{"' + name + '", ' + str(count) + "}" for name, count in fields
    )
    return "{" + entries + "}"


class BasePlotter(ActionSet):
    """! One file writer serves all trees of a plotter during a traversal.

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
        self._available_formats = None
        self._set_output_format(file_format)

    def render(self, template):
        """Renders a Jinja2 template against the plotter's dictionary."""
        return jinja2.Template(template, trim_blocks=True, lstrip_blocks=True).render(
            **self._dictionary
        )

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

    def set_available_formats(self, formats):
        """Records the output formats the generated build offers."""
        self._available_formats = set(formats)

    def _resolve_available_format(self, file_format):
        """Returns file_format if the build offers it, and VTU otherwise."""
        if self._available_formats is None or file_format in self._available_formats:
            return file_format
        fallback_format = "vtu"
        log.warning(
            "This build offers no plot format '%s', so the plotter writes '%s'.",
            file_format,
            fallback_format,
        )
        return fallback_format

    def _ensure_format_resolved(self):
        """Binds the selected writer types and includes once."""
        if self._format_resolved:
            return
        resolved_format = self._file_format
        if resolved_format is None:
            resolved_format = get_default_format()
        resolved_format = self._resolve_available_format(resolved_format)
        self._dictionary.update(_FORMATS[resolved_format])
        self._format_resolved = True

    def plotted_fields(self, fallback_name, fallback_components):
        """Returns the plotted fields as name and component pairs; an empty description
        writes all unknowns under the dataset name."""
        description = self._dictionary["DESCRIPTION"]
        if description:
            return list(description.items())
        return [(fallback_name, fallback_components)]

    def get_constructor_body(self):
        return """
  _treeId = treeId;
"""

    def prepare_traversal_body(self):
        self._ensure_format_resolved()
        return self.render("""
  _snapshot.openFile<tarch::plotter::{{FILE_WRITER}}, tarch::plotter::{{TIME_SERIES_WRITER}}>(
    ::runner::RuntimeParameters::getInstance().getOutputPath() + "{{OUTPUT_SUBFOLDER}}",
    {{TIMESTAMP}}
  );
""")

    def unprepare_traversal_body(self):
        return self.render("""
  _snapshot.writeFile(
    ::runner::RuntimeParameters::getInstance().getOutputPath() + "{{OUTPUT_SUBFOLDER}}",
    "{{FILENAME}}"
  );
""")

    def operation_body(self, operation_name):
        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            return self.additional_cell_data_construction()
        return "\n"

    def additional_cell_data_declarations(self):
        return self._additional_cell_data_declarations

    def additional_cell_data_construction(self):
        return self._additional_cell_data_construction

    def additional_cell_data_plots(self):
        return self._additional_cell_data_plots

    def attributes(self):
        return """
  int _treeId{-1};

  static ::exahype2::plotter::Snapshot _snapshot;
""" + self.additional_cell_data_declarations()

    def includes(self):
        self._ensure_format_resolved()
        return [
            "exahype2/plotter/Snapshot.h",
            "runner/RuntimeParameters.h",
        ] + self._dictionary["INCLUDES"]

    def get_static_initialisations(self, full_qualified_classname):
        return f"""
::exahype2::plotter::Snapshot {full_qualified_classname}::_snapshot;
"""
