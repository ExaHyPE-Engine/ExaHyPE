# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .ParticlePlotter import ParticlePlotter


class HDF5ParticlePlotter(ParticlePlotter):
    def __init__(
        self,
        output_path,
        filename,
        particle_set,
        description,
        time_stamp_evaluation,
    ):
        super().__init__(
            output_path,
            filename,
            particle_set,
            description,
            time_stamp_evaluation,
        )

        self.d["FILE_WRITER"] = "HDF5FileWriter"
        self.d["TIME_SERIES_WRITER"] = "XDMFTimeSeriesWriter"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return super().get_includes() + (
            """
#include "tarch/plotter/HDF5FileWriter.h"
#include "tarch/plotter/XDMFTimeSeriesWriter.h"
"""
        )
