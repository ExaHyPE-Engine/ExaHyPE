# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Scenario import Scenario

import os

class DynamicRuptureScenario(Scenario):
    """!

    Base class for the dynamic rupture scenarios.

    dynamicRuptureDictionary fills in specs/DynamicRuptureParameters.template.h. Every entry down
    to and including forced_rupture_time and d_c becomes the body of a function, so it has to be a
    string holding C++ statements and has to return a value. The remaining entries become the
    initialisers of constexpr doubles.

    Subclasses replace the whole dictionary rather than individual entries; the values below are
    the neutral defaults a new scenario can start from. The half-widths in requiredKeys have no
    neutral default and every scenario has to state them, see generate_required_files().
    """

    requiredKeys = ["f_wy", "f_wz"]

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
        # constant parameters
        "mu_d": 0,
        "mu_s": 0,
        "t0_rupture": 0,
        "f_cy": 0,
        "f_cz": 0,
    }

    def generate_required_files(self):
        # f_wy and f_wz are the half-widths of the boxcar in tauStrength(). A width of zero makes
        # the box empty, which adds 1e10 cohesion over the whole fault and locks it, so there is no
        # value that is safe to assume. Report the omission here rather than let the run produce a
        # fault that never ruptures.
        missing = [key for key in self.requiredKeys if key not in self.dynamicRuptureDictionary]
        if missing:
            raise KeyError(
                type(self).__name__ + " does not set " + ", ".join(missing)
                + " in dynamicRuptureDictionary; the cohesion boxcar half-widths have no default."
            )

        self.generate_file_from_template(
            os.path.dirname(os.path.realpath(__file__))+"/specs/DynamicRuptureParameters.template.h",
            "DynamicRuptureParameters.h", self.dynamicRuptureDictionary
        )
