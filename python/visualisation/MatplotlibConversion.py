# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .Conversion import Conversion

import visualisation.output


class MatplotlibConversion(Conversion):
    """Conversion that renders via Matplotlib."""

    def __init__(self, args, fmt=None):
        super().__init__(args)
        self.format = fmt

    def build_visualiser(self, start: int, end: int):
        # Reuse the parsed combine_unknowns indices stored by the base
        # class during Conversion.run. If none were provided this will be
        # an empty list, so convert it to None for the visualiser API.
        combine_unknowns_list = (
            self._indices_to_sum if getattr(self, "_indices_to_sum", None) else None
        )

        return visualisation.output.MatplotlibVisualiser(
            file_name=self.args.filename,
            dpi=self.args.dpi,
            output_directory=self.args.dir,
            verbose=self.args.verbose,
            start=start,
            end=end,
            format=self.format,
            unknown=self.args.unknown,
            # Only relevant for 3d plots
            slice_axis=self.args.slice_axis,
            slice_position=self.args.slice_position,
            slice_width=self.args.slice_width,
            flip_view=self.args.flip_view,
            combine_unknowns=combine_unknowns_list,
            enable_raytracing=self.args.enable_raytracing,
        )
