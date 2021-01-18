#!/usr/bin/env python3
from exahype2.solvers.aderdg.LagrangeBasis import *

import pprint

order = 5
for basis in [ GaussLegendreBasis(order), GaussLobattoBasis(order) ]:
  d = {}
  basis._init_dictionary_with_default_parameters(d)
  pprint.pprint(d)
