# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import argparse
import tarfile
import os
import exahype2.postprocessing

import matplotlib.pyplot as plt


if __name__ == "__main__":
  parser = argparse.ArgumentParser(description="""
ExaHyPE 2 scaling plotter

Default plotter for various standard ExaHyPE dumps.

""")
  parser.add_argument("file", nargs="+",    help="filename of the file to parse (should be a tar.gz file)")
  parser.add_argument("-v",     "--verbose",     dest="verbose",    help="increase output verbosity",   action="store_true", default=False )
  parser.add_argument("-log-y", "--log-ysaxis",  dest="log_yscale", help="increase output verbosity",   action="store_true", default=False )
  parser.add_argument("-t",     "--type",    dest="type",    help="Pick plot variant", choices=[
   "time-step-size","time-step-size-rt",
   "runtime-rt", "runtime-rt-sum-2",
  ], required=True  )
  parser.add_argument("-l",     "--label",   dest="label",   nargs="*", help="Label of dataset (optional) ",  default="output"  )
  parser.add_argument("-o",     "--output",  dest="output",  help="Name of output file",         default="output"  )
  args = parser.parse_args()
  
  
  plt.clf()
  if args.log_yscale:
    plt.yscale( 'log' )
  for dataset in zip( args.file, args.label) :
    performance_data = exahype2.postprocessing.PerformanceData( dataset[0], verbose=args.verbose )
    if args.type == "time-step-size":
      exahype2.postprocessing.plot_time_step_size_per_time_step( performance_data,dataset[1],False )
    elif args.type == "time-step-size-rt":
      exahype2.postprocessing.plot_time_step_size_per_time_step( performance_data,dataset[1],True )
    elif args.type == "runtime-rt":
      exahype2.postprocessing.plot_time_per_time_step( performance_data,dataset[1] )
    elif args.type == "runtime-rt-sum-2":
      exahype2.postprocessing.plot_time_per_time_step( performance_data,dataset[1], 2 )
    else:
      raise Exception( "Type " + args.type + " not supported" )

  plt.savefig( args.output + ".pdf" )
  plt.savefig( args.output + ".png" )
