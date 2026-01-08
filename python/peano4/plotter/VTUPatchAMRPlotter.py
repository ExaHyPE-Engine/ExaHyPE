# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .PatchAMRPlotter import PatchAMRPlotter


class VTUPatchAMRPlotter(PatchAMRPlotter):
    def __init__(
        self,
        output_path,
        filename,
        patch,
        dataset_name,
        description,
        time_stamp_evaluation,
        amr_scale_factor=0.5,
        amr_base_thickness=0.2,
        guard="true",
    ):
        super().__init__(
            output_path,
            filename,
            patch,
            dataset_name,
            description,
            time_stamp_evaluation,
            amr_scale_factor,
            amr_base_thickness,
            guard,
        )

        self.d["FILE_WRITER"] = "VTUFileWriter"
        self.d["TIME_SERIES_WRITER"] = "PVDTimeSeriesWriter"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return super().get_includes() + (
            """
#include "tarch/plotter/VTUFileWriter.h"
#include "tarch/plotter/PVDTimeSeriesWriter.h"
"""
        )
