from .Scenario import Scenario

import os
import sys
sys.path.insert(0, os.path.abspath("../Equations"))
from Equations import SWE

class SWE_resting_lake(Scenario):
    _periodic_bc = True