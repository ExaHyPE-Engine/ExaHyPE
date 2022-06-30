#!/usr/bin/env python3
from exahype2.solvers.dg.LagrangeBasis import *

import pprint

order        = 3
nodesPerAxis = order+1
for basis in [ GaussLegendreBasis(nodesPerAxis), GaussLobattoBasis(nodesPerAxis) ]:
  d = {}
  basis._init_dictionary_with_default_parameters(d)
  print(d["BASIS_DECLARATIONS"])
  print(d["BASIS_INITIALIZERS"])
