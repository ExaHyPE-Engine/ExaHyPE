#!/usr/bin/env python3
from exahype2.solvers.aderdg.LagrangeBasis import *

legendreBasis = GaussLegendreBasis(5)
lobattoBasis  = GaussLobattoBasis(6)

print(legendreBasis)
print(lobattoBasis)
