# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Scenario import Scenario

import os


class DynamicRuptureScenario(Scenario):
    """!

    A subclass replaces the whole dynamicRuptureDictionary and sets every key of requiredKeys.
    """

    requiredKeys = ["f_wy", "f_wz"]

    # The initial condition writes these fault parameters into the auxiliary variables of every node.
    auxiliary_variables = {}

    dynamicRuptureDictionary = {
        "initialStresses": """
      sxx = 0;
      syy = 0;
      szz = 0;
      sxy = 0;
      sxz = 0;
      syz = 0;
    """,
        "n_yz": "return 0;",
        "cohesion": "return 0;",
        "forced_rupture_time": "return 0;",
        "d_c": "return 0;",
        "mu_d": 0,
        "mu_s": 0,
        "t0_rupture": 0,
        "f_cy": 0,
        "f_cz": 0,
    }

    def generate_required_files(self):
        # A zero half-width of the cohesion box locks the whole fault.
        missing = [
            key for key in self.requiredKeys if key not in self.dynamicRuptureDictionary
        ]
        if missing:
            raise KeyError(
                type(self).__name__
                + " does not set "
                + ", ".join(missing)
                + " in dynamicRuptureDictionary; set both cohesion boxcar half-widths."
            )

        for suffix in (".h", ".cpph"):
            self.generate_file_from_template(
                os.path.dirname(os.path.realpath(__file__))
                + "/specs/DynamicRuptureParameters"
                + suffix
                + ".jinja2",
                "DynamicRuptureParameters" + suffix,
                self.dynamicRuptureDictionary,
            )
