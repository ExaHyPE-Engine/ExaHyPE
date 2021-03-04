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
   "time-step-size-it", "time-step-size-rt", "time-step-size-st",
   "runtime-per-step", "runtime-per-step-sum-2",
   "runtime-vs-simulated-time",
  ], required=True  )
  parser.add_argument("-l",     "--label",   dest="label",   nargs="*", help="Label of dataset (optional; if used, you have to add --label/-l after the actual files and you have to embed the labels into hyphens)",  default="output"  )
  parser.add_argument("-o",     "--output",  dest="output",  help="Name of output file",         default="output"  )
  args = parser.parse_args()
  
  
  plt.clf()
  if args.log_yscale:
    plt.yscale( 'log' )
  for dataset in zip( args.file, args.label) :
    performance_data = exahype2.postprocessing.PerformanceData( dataset[0], verbose=args.verbose )
    if args.type == "time-step-size-it":
      exahype2.postprocessing.plot_time_step_size_per_step( performance_data,dataset[1], exahype2.postprocessing.XAxis.Iterations )
    elif args.type == "time-step-size-rt":
      exahype2.postprocessing.plot_time_step_size_per_step( performance_data,dataset[1], exahype2.postprocessing.XAxis.RealTime )
    elif args.type == "time-step-size-st":
      exahype2.postprocessing.plot_time_step_size_per_step( performance_data,dataset[1], exahype2.postprocessing.XAxis.SimulatedTime )
    elif args.type == "runtime-per-step":
      exahype2.postprocessing.plot_runtime_per_time_step( performance_data,dataset[1] )
    elif args.type == "runtime-per-step-sum-2":
      exahype2.postprocessing.plot_runtime_per_time_step( performance_data,dataset[1], 2 )
    elif args.type == "runtime-vs-simulated-time":
      exahype2.postprocessing.plot_runtime_against_simulated_time( performance_data,dataset[1] )
    else:
      raise Exception( "Type " + args.type + " not supported" )

  if len(args.label)>0:
    plt.legend()

  plt.savefig( args.output + ".pdf" )
  plt.savefig( args.output + ".png" )
