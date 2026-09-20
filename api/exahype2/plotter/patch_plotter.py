# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import textwrap

from exahype2.actionsets.action_set import ActionSet

from .base_plotter import BasePlotter
from .base_plotter import data_field_list


def mapping_along_axis(mapping, points_per_axis):
    """Returns the coordinates the plot points of a patch take along one axis.

    The mapping runs over the tensor product of these coordinates with the first axis
    running fastest, so one list of coordinates describes every point.
    """
    if len(mapping) < points_per_axis:
        raise ValueError("The mapping needs at least one point per node along an axis.")
    axes = len(mapping[0])
    if axes < 2:
        raise ValueError("Each mapping point needs at least two coordinates.")
    if len(mapping) != points_per_axis**axes:
        raise ValueError(
            "The mapping needs one point per node of the tensor product of {} coordinates "
            "along {} axes.".format(points_per_axis, axes)
        )

    coordinates = [point[0] for point in mapping[:points_per_axis]]
    for point_index, point in enumerate(mapping):
        if len(point) != axes:
            raise ValueError(
                "Every mapping point needs the same number of coordinates."
            )
        for axis, coordinate in enumerate(point):
            index_along_axis = (point_index // points_per_axis**axis) % points_per_axis
            if coordinate != coordinates[index_along_axis]:
                raise ValueError(
                    "The mapping must be the tensor product of its coordinates along one axis."
                )
    return coordinates


class PatchPlotter(BasePlotter):
    """! The nodal representation gives every patch its own mapped vertices and
    connects neighbouring vertices within that patch, so a patch boundary keeps
    two traces. The cell representation writes one cell-data value per finite
    volume and ignores mapping.
    """

    def __init__(
        self,
        filename,
        patch,
        dataset_name,
        description,
        time_stamp_evaluation,
        guard,
        representation="nodal",
        mapping=None,
        file_format=None,
        additional_cell_data_declarations="",
        additional_cell_data_construction="",
        additional_cell_data_plots="",
        alternative_data_guard="",
        alternative_data_access="",
        alternative_data_dofs_per_axis=0,
    ):
        """Whenever ``alternative_data_guard`` evaluates to true, every plot point
        reads the nearest degree of freedom of the array that
        ``alternative_data_access`` yields, on a grid of
        ``alternative_data_dofs_per_axis`` entries per axis."""
        super().__init__(
            filename,
            dataset_name,
            description,
            time_stamp_evaluation,
            guard,
            file_format=file_format,
            additional_cell_data_declarations=additional_cell_data_declarations,
            additional_cell_data_construction=additional_cell_data_construction,
            additional_cell_data_plots=additional_cell_data_plots,
        )
        self._dictionary["OUTPUT_SUBFOLDER"] = "patch/"

        if representation not in ("nodal", "cell"):
            raise ValueError(
                'The representation must be "nodal" or "cell" and is "{}".'.format(
                    representation
                )
            )
        if any(dimension != patch.dim[0] for dimension in patch.dim):
            raise ValueError("The patch must span the same extent in every dimension.")

        if alternative_data_guard and representation != "nodal":
            raise ValueError(
                "An alternative data source requires the nodal representation."
            )
        if alternative_data_guard and (
            not alternative_data_access or alternative_data_dofs_per_axis <= 0
        ):
            raise ValueError(
                "An alternative data source requires its data access and a "
                "positive number of degrees of freedom per axis."
            )

        self._representation = representation
        degrees_of_freedom_per_axis = patch.dim[0]
        self._dictionary["DOFS_PER_AXIS"] = str(degrees_of_freedom_per_axis)
        self._dictionary["UNKNOWNS"] = str(patch.no_of_unknowns)
        self._dictionary["PLOT_FIELDS"] = data_field_list(
            self.plotted_fields(dataset_name, patch.no_of_unknowns)
        )

        self._dictionary["ALTERNATIVE_DATA_GUARD"] = alternative_data_guard or "false"
        self._dictionary["ALTERNATIVE_DATA_ACCESS"] = (
            alternative_data_access or "nullptr"
        )
        self._dictionary["ALTERNATIVE_DATA_DOFS_PER_AXIS"] = str(
            alternative_data_dofs_per_axis
            if alternative_data_guard
            else degrees_of_freedom_per_axis
        )

        if representation == "nodal":
            if mapping is None or len(mapping) == 0:
                raise ValueError(
                    "The nodal representation requires mapping coordinates."
                )
            coordinates = mapping_along_axis(mapping, degrees_of_freedom_per_axis)
            self._dictionary["MAPPING"] = (
                "{" + ", ".join(str(coordinate) for coordinate in coordinates) + "}"
            )

    def operation_body(self, operation_name):
        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            # The caller's cell fields precede the curve position in the file, so the plotter
            # creates them from within open().
            self._dictionary["ADDITIONAL_CELL_DATA_CONSTRUCTION"] = textwrap.dedent(
                self.additional_cell_data_construction()
            ).strip()
            return self.render("""
{% if ADDITIONAL_CELL_DATA_CONSTRUCTION %}
  _plotter.open(_snapshot, _treeId, {{PLOT_FIELDS}}, [&]() {
    {{ADDITIONAL_CELL_DATA_CONSTRUCTION | indent(4)}}
  });
{% else %}
  _plotter.open(_snapshot, _treeId, {{PLOT_FIELDS}}, []() {});
{% endif %}
""")
        if operation_name != ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            return "\n"
        self._dictionary["ADDITIONAL_CELL_DATA_PLOTS"] = textwrap.dedent(
            self.additional_cell_data_plots()
        ).strip()
        if self._representation == "nodal":
            return self.render("""
  if ({{GUARD_PREDICATE}}) {
    constexpr std::array<double, {{DOFS_PER_AXIS}}> Mapping{{MAPPING}};
    _plotter.plotCell<{{DOFS_PER_AXIS}}, {{UNKNOWNS}}, {{ALTERNATIVE_DATA_DOFS_PER_AXIS}}>(
      marker,
      DomainOffset,
      DomainSize,
      Mapping,
      fineGridCell{{NAME}}.data(),
      {{ALTERNATIVE_DATA_GUARD}},
      {{ALTERNATIVE_DATA_ACCESS}},
      [&]([[maybe_unused]] const int cellIndex) {
{% if ADDITIONAL_CELL_DATA_PLOTS %}
        {{ADDITIONAL_CELL_DATA_PLOTS | indent(8)}}
{% endif %}
      }
    );
  }
""")
        return self.render("""
  if ({{GUARD_PREDICATE}}) {
    _plotter.plotCell<{{DOFS_PER_AXIS}}, {{UNKNOWNS}}>(
      marker,
      DomainOffset,
      DomainSize,
      fineGridCell{{NAME}}.data(),
      [&]([[maybe_unused]] const int cellIndex) {
{% if ADDITIONAL_CELL_DATA_PLOTS %}
        {{ADDITIONAL_CELL_DATA_PLOTS | indent(8)}}
{% endif %}
      }
    );
  }
""")

    def includes(self):
        return super().includes() + [
            "Constants.h",
            "exahype2/plotter/PatchPlotter.h",
        ]

    def attributes(self):
        if self._representation == "nodal":
            return super().attributes() + """
  ::exahype2::plotter::NodalPatchPlotter _plotter{};
"""
        return super().attributes() + """
  ::exahype2::plotter::CellPatchPlotter _plotter{};
"""
