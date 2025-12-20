# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .ArgumentParser import ArgumentParser
from .MatplotlibConversion import MatplotlibConversion


def main():
    parser = ArgumentParser()
    args = parser.parse_args()

    conversion = MatplotlibConversion(args, fmt="pgf")
    conversion.run()


if __name__ == "__main__":
    main()
