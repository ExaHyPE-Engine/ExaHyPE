from paraview.simple import *
from peano4.visualisation.Visualiser import render_single_file 
import sys


if __name__ == "__main__":
    import sys, os
    if len(sys.argv) != 2:
        print("Error, no input file given, exiting...")
        sys.exit(1)
    filename = sys.argv[1]
    if not os.path.exists(filename):
        print("Error, specified input file '{}' does not exist, exiting...". format(filename))
        sys.exit(1)
    print("Rendering {}".format(filename))
    render_single_file(filename, "EulerQ")

    sys.exit(0)
