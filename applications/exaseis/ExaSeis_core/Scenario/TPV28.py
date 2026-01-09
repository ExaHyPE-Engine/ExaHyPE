# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Scenario import Scenario

import os

class TPV28(Scenario):
    """
    Part of a series of benchmarks by the Statewide California Earthquake Center (SCEC)

    Single non-planar fault with two hills.
    On the plane, the initial stresses are defined by the surface.
    There are homogeneous initial stress conditions outside of the plane.

    The description of the scenario can be found at: https://strike.scec.org/cvws/tpv28docs.html
    """

    domain_offset   = [4.5,  0., 4.5]
    domain_size     = [27., 27., 27.] #leads to a cell size of 0.3333 for 81 cells

    end_time = 13.0

    tracer_coordinates = [
          # off-fault at surface
          [17.0, 0.0,  5.0],
          [23.0, 0.0,  5.0],
          [17.0, 0.0, 20.0],
          [23.0, 0.0, 20.0],
          [17.0, 0.0, 35.0],
          [23.0, 0.0, 35.0],
          # on-fault at surface (3)
          [20.0, 0.0,  5.0],
          [20.0, 0.0, 20.0], #surface above fault hypocenter
          [20.0, 0.0, 35.0],
          #on-fault, general (21)
          [20.0,  6.0,  9.5], #above left patch center
          [20.0,  7.5,  5.0],
          [20.0,  7.5,  8.0],
          [20.0,  7.5,  9.5], #center of left patch
          [20.0,  7.5, 11.0],
          [20.0,  7.5, 14.0],
          [20.0,  9.0,  9.5], #below left patch center
          [20.0,  3.0, 20.0], #above fault hypocenter
          [20.0,  7.5, 20.0], #on fault hypocenter
          [20.0, 12.0, 20.0], #below fault hypocenter
          [20.0,  6.0, 30.5], #above right patch center
          [20.0,  7.5, 26.0],
          [20.0,  7.5, 29.0],
          [20.0,  7.5, 30.5], #center of right patch
          [20.0,  7.5, 32.0],
          [20.0,  7.5, 35.0],
          [20.0,  9.0, 30.5]  #below right patch center
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
            os.path.dirname(os.path.realpath(__file__))+"/specs/TPV28/tpv28.yaml.template",
            "tpv28.yaml", dictionary
        )

        self.copy_file_to_current_folder(
            os.path.dirname(os.path.realpath(__file__))+"/specs/TPV28/tpv28_fault.yaml",
            "tpv28_fault.yaml"
        )

        return