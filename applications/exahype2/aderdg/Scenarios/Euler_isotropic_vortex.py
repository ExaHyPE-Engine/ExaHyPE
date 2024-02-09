from .Scenario import Scenario

import os
import sys
sys.path.insert(0, os.path.abspath("../Equations"))
from Equations import Euler

class Euler_isotropic_vortex(Scenario):
    _periodic_bc = True