# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Scenario import Scenario

import os

class TPV5(Scenario):
    """
    Part of a series of benchmarks by the Statewide California Earthquake Center (SCEC)

    Spontaneous rupture on a vertical strike-slip fault in a homogeneous halfspace.
    There are slightly heterogeneous initial stress conditions. 

    The description of the scenario can be found at: https://strike.scec.org/cvws/tpv5docs.html
    """

    domain_offset   = [6.5,  0., 6.5] #centers the fault which is at 20.
    domain_size     = [27., 27., 27.] #leads to a cell size of 0.333 for 81 cells

    end_time = 10.0

    tracer_coordinates = [
          # off-fault at surface
          [17.0, 0.0,  8.0],
          [17.0, 0.0, 32.0],
          [23.0, 0.0,  8.0],
          [23.0, 0.0, 32.0],
          # off-fault at hypocenter depth
          [17.0, 7.5,  8.0],
          [17.0, 7.5, 32.0],
          [23.0, 7.5,  8.0],
          [23.0, 7.5, 32.0],
          # on-fault at surface
          [20.0, 0.0,  8.0],
          [20.0, 0.0, 12.5],
          [20.0, 0.0, 15.5],
          [20.0, 0.0, 20.0], #surface above fault hypocenter
          [20.0, 0.0, 24.5],
          [20.0, 0.0, 27.5],
          [20.0, 0.0, 32.0],
          [20.0, 3.0, 20.0], #above fault hypocenter
          [20.0, 12.0, 20.0], #below fault hypocenter
          #on-fault at hypocenter depth
          [20.0, 7.5,  8.0],
          [20.0, 7.5, 12.5], #center of left patch
          [20.0, 7.5, 15.5],
          [20.0, 7.5, 20.0], #on fault hypocenter
          [20.0, 7.5, 24.5],
          [20.0, 7.5, 27.5], #center of right patch
          [20.0, 7.5, 32.0]
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
            os.path.dirname(os.path.realpath(__file__))+"/specs/TPV5/tpv5.yaml.template",
            "tpv5.yaml", dictionary
        )

        self.copy_file_to_current_folder(
            os.path.dirname(os.path.realpath(__file__))+"/specs/TPV5/tpv5_fault.yaml", 
            "tpv5_fault.yaml"
        )

        return