# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import argparse
import os


from OutputFile import OutputFile

import matplotlib.pyplot as plt


Colours = [ "#0000ff", "#00ff00", "#ff0000", "#00ffff", "#ff00ff", "#ffff00", "#aaaaaa" ]
Symbols = [ "o",       "s",       "<",       ">",       "^",       "v",       "x"       ]
   
   
def flatten( input_data ):
  """
  
  y_data: [[Float]]
    Multiple quantities per time stamp. There might be different numbers
    of quantities per time stamp.
  
  We return a tuple [Float],[Float].
  
  """
  x_data_flattened = []
  y_data_flattened = []
  
  for i in range(0,len(input_data)):
    for data_point in input_data[i]:
      x_data_flattened.append( i )
      y_data_flattened.append( data_point )
  
  return x_data_flattened, y_data_flattened


def remove_duplicates(x_data,y_data):
  assert len(x_data)==len(y_data)
  zipped_input  = zip(x_data, y_data)
  zipped_output = []
  for i in zipped_input:
    if not i in zipped_output:
      zipped_output.append(i)
  return [x[0] for x in zipped_output], [x[1] for x in zipped_output]
  
  
     
def plot_cells_per_rank( data: OutputFile, 
                         plot_remote_cells, 
                         show_optimum,
                         ):
  Ranks   = range(0,data.ranks)
    
  symbol_counter = 0
  colour_counter = 0
  line_counter   = 0
  my_alpha       = 0.1+(1.0-0.3)/len(Ranks)
  
  final_number_of_cells = 0
  
  for rank in Ranks:
    local_cells = data.local_cells_per_snapshot( rank )

    x_data_flattened, y_data_flattened = flatten( local_cells )
    x_data_flattened, y_data_flattened = remove_duplicates( x_data_flattened, y_data_flattened )
    plt.scatter(x_data_flattened, 
                  y_data_flattened, 
                  marker=Symbols[symbol_counter], 
                  color=Colours[colour_counter],  
                  alpha=my_alpha, 
                  label="#cells per tree on rank " + str(rank) )  
          
    if plot_remote_cells:
      remote_cells = data.remote_cells_per_snapshot( rank )
      x_data_flattened, y_data_flattened = flatten( remote_cells )
      x_data_flattened, y_data_flattened = remove_duplicates( x_data_flattened, y_data_flattened )
      plt.scatter(x_data_flattened, 
                  y_data_flattened, marker=Symbols[symbol_counter], color=Colours[colour_counter], alpha=my_alpha, facecolors='none', label="Remote #cells per tree")

    if show_optimum: 
        optimum_cells = data.total_local_cells_per_snapshot( rank )
        max_trees     = max( data.trees_per_snapshot( rank ) )
        y_data = [ x / max_trees for x in optimum_cells ]
        x_data = range(0,len(y_data))
        plt.plot(x_data, y_data, "-", color=Colours[colour_counter] )

    symbol_counter += 1
    colour_counter += 1
    if symbol_counter>=len(Symbols):
      symbol_counter = 0
    if colour_counter>=len(Colours):
      colour_counter = 0
      line_counter  += 1

     
 
def plot_trees_per_rank( data: OutputFile ):
  Ranks   = range(0,data.ranks)
  
  symbol_counter = 0
  colour_counter = 0
  my_alpha       = 0.1
  for rank in Ranks:
    y_data = data.trees_per_snapshot( rank )
    x_data = range(0,len(y_data))
    plt.plot(x_data, y_data, marker=Symbols[symbol_counter], color=Colours[colour_counter], label="rank " + str(rank) )     

    symbol_counter += 1
    colour_counter += 1
    if symbol_counter>=len(Symbols):
      symbol_counter = 0
      colour_counter += 1

    if colour_counter>=len(Colours):
      colour_counter = 0



if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='Peano 4 load balancing plotter')
  parser.add_argument("file",   help="filename of the file to parse (should be a text file)")
  parser.add_argument("-v",       "--verbose",                                      action="store_true",                                      help="increase output verbosity", default=False)
  parser.add_argument("-rc",      "--remote-cells",      dest="plot_remote_cells",  action="store_true",                    default=False,    help="plot remote cells, too")
  parser.add_argument("-opt",     "--show-optimum",      dest="show_optimum",       action="store_false",                   default=True,     help="display optimal number of cells per rank")
  parser.add_argument("-yscale",  "--yscale",            dest="yscale",             choices=["linear", "log-2", "log-10"],  default="linear", help="select scaling of y-axis" )
  parser.add_argument("-ncol",    "--columns-in-legend", dest="columns_in_legend",  type=int,                               default=1,        help="number of columns within legend" )
  args = parser.parse_args()

  data = OutputFile( args.file, args.verbose )

  plt.clf()
  plot_trees_per_rank( data )
  plt.xlabel( "time" )
  plt.ylabel( "trees per rank" )
  
  if args.yscale=="log-2":
    plt.yscale( "log", base=2 )
  if args.yscale=="log-10":
    plt.yscale( "log", base=10 )

  plt.legend(ncol=args.columns_in_legend)
  plt.savefig( args.file + "-trees-per-rank.pdf" )
  plt.savefig( args.file + "-trees-per-rank.png" )
  
  plt.clf()
  plot_cells_per_rank( data, args.plot_remote_cells, args.show_optimum )
  plt.xlabel( "time" )
  plt.ylabel( "cells per tree" )
  
  if args.yscale=="log-2":
    plt.yscale( "log", base=2 )
  if args.yscale=="log-10":
    plt.yscale( "log", base=10 )

  plt.ylim( bottom=9 )
  plt.legend(ncol=args.columns_in_legend)
  output_filename = args.file + "-cells-per-rank"
  plt.savefig( output_filename + ".pdf" )
  plt.savefig( output_filename + ".png" )
  print( "wrote output to " + output_filename + "[-cells-per-rank|-tree-per-rank].[png|pdf]")

