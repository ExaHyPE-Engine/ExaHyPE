"""Input utilities

This file is part of the SWIFT 2 project. For conditions of distribution and 
use, please see the copyright notice at www.peano-framework.org

The package collects a couple of initialisation routines which are tailored 
towards the SPH code. Most of them are relatively simplistic wrappers around
Peano's particle toolbox.

"""
from .InsertParticlesByCoordinates import InsertParticlesByCoordinates
from .InsertParticlesFromHDF5File import InsertParticlesFromHDF5File
from .InsertParticlesAlongCartesianGrid import InsertParticlesAlongCartesianGrid
from .InsertRandomParticlesIntoCells import InsertRandomParticlesIntoCells
