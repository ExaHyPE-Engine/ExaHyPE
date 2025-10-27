# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from ArgumentParser import ArgumentParser
from VTKConversion import VTKConversion

if __name__ == "__main__":
    parser = ArgumentParser()
    args = parser.parse_args()

    conversion = VTKConversion(args)
    conversion.run()
