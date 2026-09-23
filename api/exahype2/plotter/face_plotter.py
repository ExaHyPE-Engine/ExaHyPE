# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.actionsets.action_set import ActionSet

from .base_plotter import BasePlotter
from .base_plotter import data_field_list
from .patch_plotter import mapping_along_axis


class FacePlotter(BasePlotter):
    """! A face carries the values of the cell on its left and of the cell on its
    right, and each field of the description writes both traces.
    """

    def __init__(
        self,
        filename,
        patch,
        dataset_name,
        description,
        time_stamp_evaluation,
        mapping,
        guard,
        file_format=None,
    ):
        super().__init__(
            filename,
            dataset_name,
            description,
            time_stamp_evaluation,
            guard,
            file_format=file_format,
        )

        self.d["OUTPUT_SUBFOLDER"] = "face/"

        degrees_of_freedom_per_axis = patch.dim[1]
        self.d["DOFS_PER_AXIS"] = str(degrees_of_freedom_per_axis)
        self.d["UNKNOWNS"] = str(patch.no_of_unknowns)
        self.d["PLOT_FIELDS"] = data_field_list(
            self.plotted_fields(dataset_name, patch.no_of_unknowns)
        )

        if mapping is None or len(mapping) == 0:
            raise ValueError("The face plotter requires mapping coordinates.")
        coordinates = mapping_along_axis(mapping, degrees_of_freedom_per_axis)
        self.d["MAPPING"] = (
            "{" + ", ".join(str(coordinate) for coordinate in coordinates) + "}"
        )

    def operation_body(self, operation_name):
        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            return self.render("""
  _plotter.open(_snapshot, _treeId, {{PLOT_FIELDS}});
""") + super().operation_body(operation_name)
        if operation_name == ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
            return self.render("""
  if (not marker.hasBeenRefined() and not marker.hasCellBeenRefined() and {{GUARD_PREDICATE}}) {
    constexpr std::array<double, {{DOFS_PER_AXIS}}> Mapping{{MAPPING}};
    _plotter.plotFace<{{DOFS_PER_AXIS}}, {{UNKNOWNS}}>(marker, Mapping, fineGridFace{{NAME}}.data());
  }
""")
        return "\n"

    def includes(self):
        return super().includes() + ["exahype2/plotter/FacePlotter.h"]

    def attributes(self):
        return super().attributes() + """
  ::exahype2::plotter::FacePlotter _plotter{};
"""
