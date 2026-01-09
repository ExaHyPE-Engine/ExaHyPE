# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Scenario import Scenario

import os

class TPV26(Scenario):
    """
    Part of a series of benchmarks by the Statewide California Earthquake Center (SCEC)

    Single planar vertical strike-slip fault in a linear elastic medium.

    The description of the scenario can be found at: https://strike.scec.org/cvws/tpv26_27docs.html
    """

    domain_offset   = [-1.6, 0.0,  -1.6] #for 81 cells, this leads to a "buffer" of three cells at the start and end around the fault
    domain_size     = [43.2, 43.2, 43.2] #leads to a cell size of 0.5333... for 81 cells

    end_time = 20.0

    tracer_coordinates = [
          # off-fault at surface (18 total)
          [ 0.0, 0.0,  0.0],
          [10.0, 0.0,  0.0],
          [30.0, 0.0,  0.0],
          [40.0, 0.0,  0.0],
          [17.0, 0.0, 15.0],
          [23.0, 0.0, 15.0],
          [ 0.0, 0.0, 20.0],
          [10.0, 0.0, 20.0],
          [30.0, 0.0, 20.0],
          [40.0, 0.0, 20.0],
          [17.0, 0.0, 25.0],
          [23.0, 0.0, 25.0],
          [17.0, 0.0, 35.0],
          [23.0, 0.0, 35.0],
          [ 0.0, 0.0, 40.0],
          [10.0, 0.0, 40.0],
          [30.0, 0.0, 40.0],
          [40.0, 0.0, 40.0],
          # on-fault at surface (2 total)
          [20.0, 0.0, 15.0],
          [20.0, 0.0, 35.0],
          #on-fault at hypocenter depth (6 total)
          [20.0, 10.0,  5.0],
          [20.0, 10.0, 15.0],
          [20.0, 10.0, 20.0],
          [20.0, 10.0, 25.0],
          [20.0, 10.0, 30.0],
          [20.0, 10.0, 35.0],
          #on-fault off hypocenter depth (4 total)
          [20.0,  5.0, 15.0],
          [20.0,  5.0, 35.0],
          [20.0, 15.0, 15.0],
          [20.0, 15.0, 35.0],
    ]

    def initial_conditions(self):
        return """
      Q[Shortcuts::rho] = 2.67;
      Q[Shortcuts::cp ] = 6.0;
      Q[Shortcuts::cs ] = 3.464;
"""
    
    def generate_required_files(self, order):

        dictionary = { "MODE": order+1 }
        self.generate_file_from_template(
            os.path.dirname(os.path.realpath(__file__))+"/specs/TPV26/tpv26.yaml.template",
            "tpv26.yaml", dictionary
        )

        self.copy_file_to_current_folder(
            os.path.dirname(os.path.realpath(__file__))+"/specs/TPV26/tpv26_fault.yaml", 
            "tpv26_fault.yaml"
        )

        return