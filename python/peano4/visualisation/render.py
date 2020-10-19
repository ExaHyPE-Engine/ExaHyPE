from paraview.simple import *
from peano4.visualisation.Visualiser import render_single_file 
from peano4.visualisation.Visualiser import render_dataset

import peano4.visualisation

import sys
import os
import argparse


if __name__ == "__main__":
    
    parser = argparse.ArgumentParser(description='Peano 4 - pvserver render script')
    parser.add_argument("--dataset", type=int, default=-1, dest="dataset", help="Pick dataset from meta file. If this option is not present, the script assumes that you study a plain data file without further include statements. With the argument, you can pick a particular dataset (snapshot, time step)." )
    parser.add_argument("--identifier", dest="identifier", required=True, help="Identifier within data file that should be displayed (use EulerQ for standard Euler example, e.g.)" )
    parser.add_argument("--filter-fine-grid", dest="filter_fine_grid", action="store_true", default=False, help="Display only fine grid" )
    parser.add_argument("--output", dest="output", default=None, help="Name of VTK output file" )
    parser.add_argument(dest="filename", help="Input file name" )
    args = parser.parse_args()

    if not os.path.exists(args.filename):
      print("Error, specified input file '{}' does not exist, exiting...". format(args.filename))
      sys.exit(1)
    print("Rendering {}".format(args.filename))
    
    filter = []
    if args.filter_fine_grid:
      print( "add fine grid filter" )
      filter.append( peano4.visualisation.ExtractFineGridFilter() )

    data = None
    if args.dataset<0:      
      data = render_single_file(args.filename, args.identifier, True, filter)
    else:
      data = render_dataset(args.filename, args.identifier, args.dataset, True, filter)

    if args.output!=None and data!=None:
      import vtk
      if not args.output.endswith( ".vtu" ):
        print( "WARNING: output file name should end with .vtu" )
        
      print( "Write output file into " + args.output )
      writer = vtk.vtkXMLUnstructuredGridWriter()
      writer.SetFileName(args.output)
      writer.SetInputData(data)
      writer.Write()
      
      
    tp = TrivialProducer()
    tp.GetClientSideObject().SetOutput(data)
    Show(tp)
    Interact() #Needed if running from command line
    #WriteImage("Output.png")

    sys.exit(0)
