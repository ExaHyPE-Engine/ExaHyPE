# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .PatchAMRPlotter import PatchAMRPlotter


class NetCDFPatchAMRPlotter(PatchAMRPlotter):
    def __init__(
        self,
        output_path,
        filename,
        patch,
        dataset_name,
        description,
        time_stamp_evaluation,
        guard="true",
    ):
        super().__init__(
            output_path,
            filename,
            patch,
            dataset_name,
            description,
            time_stamp_evaluation,
            amr_scale_factor=1.0,
            amr_base_thickness=0.0,
            guard=guard,
        )

        self.d["FILE_WRITER"] = "NetCDFFileWriter"
        self.d["TIME_SERIES_WRITER"] = "JSONTimeSeriesWriter"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return super().get_includes() + (
            """
#include "tarch/plotter/NetCDFFileWriter.h"
#include "tarch/plotter/JSONTimeSeriesWriter.h"
"""
        )
