"""!
Boundary Conditions

This file is part of the SWIFT 2 project. For conditions of distribution and
use, please see the copyright notice at www.peano-framework.org

The package collects a couple of initialisation routines which are tailored
towards the SPH code. Most of them are relatively simplistic wrappers around
Peano's particle toolbox.

"""
from .Fixed import Fixed
from .Inflow import Inflow
