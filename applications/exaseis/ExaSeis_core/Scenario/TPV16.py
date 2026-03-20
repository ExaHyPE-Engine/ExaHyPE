# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Scenario import Scenario

import os

class TPV16(Scenario):
    """
    Part of a series of benchmarks by the Statewide California Earthquake Center (SCEC)

    Spontaneous rupture on a vertical strike-slip fault in a homogeneous halfspace.
    There are randomly-generated heterogeneous initial stress conditions.

    The description of the scenario can be found at: https://strike.scec.org/cvws/tpv16_17docs.html
    """

    domain_offset   = [-5.92,  0., -5.92] #centers the fault which is at 20.
    domain_size     = [51.84, 51.84, 51.84]  #leads to one cell outside of fault on each side (with 27 cells)

    end_time = 15.0

    tracer_coordinates = [
      [26.0, 0.0, 11.0],
      [26.0, 0.0, 20.0],
      [26.0, 0.0, 29.0],
      [20.4, 0.0, 11.0],
      [20.4, 0.0, 20.0],
      [20.4, 0.0, 29.0],
      [19.6, 0.0, 11.0],
      [19.6, 0.0, 20.0],
      [19.6, 0.0, 29.0],
      [14.0, 0.0, 11.0],
      [14.0, 0.0, 20.0],
      [14.0, 0.0, 29.0]
    ]

    fault_coordinates = [
      [20.0, 0.0, 11.0],
      [20.0, 0.0, 20.0],
      [20.0, 0.0, 29.0],
      [20.0, 9.0, 11.0],
      [20.0, 9.0, 20.0],
      [20.0, 9.0, 29.0],
      [20.0, 0.0, 11.0],
      [20.0, 0.0, 20.0],
      [20.0, 0.0, 29.0],
      [20.0, 9.0, 11.0],
      [20.0, 9.0, 20.0],
      [20.0, 9.0, 29.0]
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
            os.path.dirname(os.path.realpath(__file__))+"/specs/TPV16/tpv16.yaml.template",
            "tpv16.yaml", dictionary
        )

        self.copy_file_to_current_folder(
            os.path.dirname(os.path.realpath(__file__))+"/specs/TPV16/tpv16_fault.yaml", 
            "tpv16_fault.yaml"
        )

        self.copy_file_to_current_folder(
            os.path.dirname(os.path.realpath(__file__))+"/specs/TPV16/tpv16_fault.nc", 
            "tpv16_fault.nc"
        )

        return