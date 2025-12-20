# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .Conversion import Conversion

import visualisation.output


class VTKConversion(Conversion):
    def build_visualiser(self, start: int, end: int):
        return visualisation.output.VTUUnstructuredGrid(
            file_name=self.args.filename,
            output_directory=self.args.dir,
            verbose=self.args.verbose,
            start=start,
            end=end,
        )
