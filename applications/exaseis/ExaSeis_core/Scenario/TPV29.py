# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Scenario import Scenario

import os

class TPV29(Scenario):
    """
    Part of a series of benchmarks by the Statewide California Earthquake Center (SCEC)

    Single rough planar vertical strike-slip fault in a linear elastic medium.

    The description of the scenario can be found at: https://strike.scec.org/cvws/tpv29_30docs.html
    """

    domain_offset   = [-3.4,  0.0, -3.4] #Ensures that the fault, which is at 20., is between two cells at 13/27th of the domain
    domain_size     = [48.6, 48.6, 48.6] #leads to cs of 0.6 for 81 cells

    end_time = 20.0

    tracer_coordinates = [
          # off-fault at surface
          [ 0.0, 0.0,  0.0], #0
          [40.0, 0.0,  0.0], #1
          [17.0, 0.0,  5.0], #2
          [23.0, 0.0,  5.0], #3
          [ 0.0, 0.0, 20.0], #4
          [17.0, 0.0, 20.0], #5 
          [23.0, 0.0, 20.0], #6
          [40.0, 0.0, 20.0], #7
          [17.0, 0.0, 35.0], #8
          [23.0, 0.0, 35.0], #9
          [ 0.0, 0.0, 40.0], #10
          [40.0, 0.0, 40.0], #11
          # on-fault at surface (3)
          [20.0, 0.0, 15.0], #surface above fault hypocenter
          [20.0, 0.0, 25.0],
          [20.0, 0.0, 35.0],
          #on-fault, general (21)
          [20.0, 15.6,  2.0],
          [20.0,  5.0,  5.0],
          [20.0, 12.0,  5.0],
          [20.0,  1.4,  9.0],
          [20.0, 10.1, 11.1],
          [20.0, 10.0, 15.0], #on fault hypocenter
          [20.0, 16.0, 15.0],
          [20.0,  6.1, 15.8],
          [20.0, 12.0, 20.0],
          [20.0,  6.2, 14.3],
          [20.0,  6.0, 14.6],
          [20.0, 12.0, 25.0],
          [20.0,  5.7, 25.1],
          [20.0,  4.7, 25.9],
          [20.0, 15.3, 29.0],
          [20.0, 15.3, 29.3],
          [20.0,  5.0, 30.0],
          [20.0, 11.0, 30.0],
          [20.0, 13.0, 35.0],
          [20.0, 10.5, 36.7],
          [20.0,  4.5, 37.0]
    ]

    fault_coordinates = [
          # on-fault at surface (3)
          [20.0, 0.0, 15.0], #surface above fault hypocenter
          [20.0, 0.0, 25.0],
          [20.0, 0.0, 35.0],
          #on-fault, general (21)
          [20.0, 15.6,  2.0],
          [20.0,  5.0,  5.0],
          [20.0, 12.0,  5.0],
          [20.0,  1.4,  9.0],
          [20.0, 10.1, 11.1],
          [20.0, 10.0, 15.0], #on fault hypocenter
          [20.0, 16.0, 15.0],
          [20.0,  6.1, 15.8],
          [20.0, 12.0, 20.0],
          [20.0,  6.2, 14.3],
          [20.0,  6.0, 14.6],
          [20.0, 12.0, 25.0],
          [20.0,  5.7, 25.1],
          [20.0,  4.7, 25.9],
          [20.0, 15.3, 29.0],
          [20.0, 15.3, 29.3],
          [20.0,  5.0, 30.0],
          [20.0, 11.0, 30.0],
          [20.0, 13.0, 35.0],
          [20.0, 10.5, 36.7],
          [20.0,  4.5, 37.0]
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
            os.path.dirname(os.path.realpath(__file__))+"/specs/TPV29/tpv29.yaml.template",
            "tpv29.yaml", dictionary
        )

        self.copy_file_to_current_folder(
            os.path.dirname(os.path.realpath(__file__))+"/specs/TPV29/tpv29_fault.yaml",
            "tpv29_fault.yaml"
        )

        self.copy_file_to_current_folder(
            os.path.dirname(os.path.realpath(__file__))+"/specs/TPV29/tpv29_buffer_n.nc",
            "tpv29_buffer_n.nc"
        )

        return
