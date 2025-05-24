# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

print("Peano 4 (C) www.peano-framework.org")

try:
    import peano4.datamodel
    import peano4.solversteps
    import peano4.output
    import peano4.toolbox
    import peano4.visualisation

    from .Project import Project
except ImportError as error:
    raise Exception("Peano core project files cannot be loaded: {}".format(error))
