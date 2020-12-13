#!/usr/bin/env python3
import os,sys
sys.path.insert(1, os.path.join(os.path.dirname(os.path.abspath(__file__)), '../'))

from LagrangeBasis import *

legendreBasis = GaussLegendreBasis(5)
lobattoBasis  = GaussLobattoBasis(6)

print(legendreBasis)
print(lobattoBasis)
