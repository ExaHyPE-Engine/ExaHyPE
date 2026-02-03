# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import argparse
import os


import matplotlib.pyplot as plt


def get_ranks_and_threads( filename ):
  ranks = 1
  threads = 1
  file = open( filename, "r" )
  for line in file:
    if "build:" in line and "no mpi" in line: 
      print( "code does not use MPI" )
      ranks = 1
    if "build:" in line and "threads" in line: 
      threads = int( line.split( "threads" )[0].split( "(" )[-1])
      print( "code uses " + str(threads) + " threads per rank" )
    if "manually reset number of threads used to" in line: 
      threads = int( line.split( "to" )[1])
      print( "code sets number of threads manually to " + str(threads) )
    if "build:" in line and "ranks)" in line: 
      ranks = int( line.split( "ranks)" )[0].split("(")[1])
      print( "code uses " + str(ranks) + " ranks" )
      
  return (ranks, threads)


def parse_file( filename, rank, verbose ):
  """
    Returns tuple of (x_data,y_data_local,y_data_remote)
    
    filename: String
      File to parse (what a surprise)
      
    rank: int
      Rank number of extract data for (can be -1 if you run serially)
    
    x_data: [Float] 
      A series of time stamps
      
    y_data_local: [[Float]] 
      A set of  series of corresponding cell count entries. Each entry 
      within the list is an array again as there might be many trees 
      per rank.


  """
  file = open( filename, "r" )
  x_data        = []
  y_data_local  = []
  y_data_remote = []
  for line in file:
    is_in_line = "toolbox::loadbalancing::dumpStatistics" in line
    if rank>=0:
      is_in_line = is_in_line and "rank:" + str(rank ) in line
      
    try:
      if is_in_line:
        x_data.append( float( line.strip().split(" ")[0] ) )
        y_data_local.append( [] )
        y_data_remote.append( [] )
        for entry in line.split( "(#")[1:]:
          local_data  = entry.split(":")[1].split("/")[0]
          remote_data = entry.split("/")[1].split(")")[0]
          y_data_local[-1].append( float(local_data) )
          y_data_remote[-1].append( float(remote_data) )
    except Exception as e:
      print( "parser error " + str(e) )
      print( "faulty line: " + line )
      print( "assume this is due to a race condition to write to the terminal; continue to parse ..." )
        
  return ( x_data, y_data_local, y_data_remote )
   
   
def flatten( x_data, y_data ):
  """
  
  x_data: [Float]
    Sequence of time stamps
  
  y_data: [[Float]]
    Multiple quantities per time stamp. There might be different numbers
    of quantities per time stamp.
  
  We return a tuple [Float],[Float].
  
  """
  x_data_flattened = []
  y_data_flattened = []
  
  assert len(x_data)==len(y_data)
  for i in range(0,len(x_data)):
    for data_point in y_data[i]:
      x_data_flattened.append( x_data[i] )
      y_data_flattened.append( data_point )
  
  return x_data_flattened, y_data_flattened


def compute_sums( x_data, y_data ):
  """
  x_data: [Float]
    Sequence of time stamps
  
  y_data: [[Float]]
    Multiple quantities per time stamp. There might be different numbers
    of quantities per time stamp.
  
  We return a sequence [Float] with the sum of all entries per time stamp.
  """
  result = []
  
  assert len(x_data)==len(y_data)
  for i in range(0,len(x_data)):
    result.append( 0.0 )
    for data_point in y_data[i]:
      result[-1] = result[-1] + data_point
      
  return result


def remove_duplicates(x_data,y_data):
  assert len(x_data)==len(y_data)
  zipped_input  = zip(x_data, y_data)
  zipped_output = []
  for i in zipped_input:
    if not i in zipped_output:
      zipped_output.append(i)
  return [x[0] for x in zipped_output], [x[1] for x in zipped_output]
  
  
     
def plot_cells_per_rank( filename, 
                         verbose, 
                         plot_remote_cells, 
                         sum_per_rank, 
                         show_optimum,
                         plot_local_cells = True 
                         ):
  (ranks,threads) = get_ranks_and_threads( filename )

  Colours = [ "#0000ff", "#00ff00", "#ff0000", "#00ffff", "#ff00ff", "#ffff00", "#aaaaaa" ]
  Symbols = [ "o",       "s",       "<",       ">",       "^",       "v"]

  Ranks   = range(0,ranks)
  if ranks<=1: 
    Ranks = [-1]
  
  symbol_counter = 0
  colour_counter = 0
  line_counter   = 0
  my_alpha       = 0.1+(1.0-0.3)/len(Ranks)
  
  final_number_of_cells = 0
  
  for rank in Ranks:
    y_data_sum = []  

    ( x_data, y_data_local, y_data_remote ) = parse_file( filename, rank, verbose )

    if verbose:
      print( "found {} timestamps for rank {}".format( len(x_data), rank ))
      
    if plot_local_cells:
      x_data_flattened, y_data_flattened = flatten( x_data, y_data_local )
      x_data_flattened, y_data_flattened = remove_duplicates( x_data_flattened, y_data_flattened )
      plt.scatter(x_data_flattened, 
                  y_data_flattened, 
                  marker=Symbols[symbol_counter], 
                  color=Colours[colour_counter],  
                  alpha=my_alpha, 
                  label="#cells per tree on rank " + str(rank) )  
          
    if plot_remote_cells:
      x_data_flattened, y_data_flattened = flatten( x_data, y_data_remote )
      x_data_flattened, y_data_flattened = remove_duplicates( x_data_flattened, y_data_flattened )
      plt.scatter(x_data_flattened, 
                  y_data_flattened, marker=Symbols[symbol_counter], color=Colours[colour_counter], alpha=my_alpha, facecolors='none', label="Remote #cells per tree")

    sums = compute_sums(x_data, y_data_local )
    if sum_per_rank:
      if rank==0: 
        plt.plot(x_data, sums, line_symbol+Symbols[symbol_counter], color=Colours[colour_counter], markevery=symbol_counter*3+2, label="total #cells of rank 0" )
      else:
        plt.plot(x_data, sums, line_symbol+Symbols[symbol_counter], color=Colours[colour_counter], markevery=symbol_counter*3+2 )
        
    final_number_of_cells += sums[-1]

    symbol_counter += 1
    colour_counter += 1
    if symbol_counter>=len(Symbols):
      symbol_counter = 0
    if colour_counter>=len(Colours):
      colour_counter = 0
      line_counter  += 1

  if sum_per_rank and show_optimum: 
    plt.plot([0,x_data[-1]], [final_number_of_cells/len(Ranks),final_number_of_cells/len(Ranks)], ":", color="#000000", label="opt. #cells per rank" )
  if show_optimum: 
    plt.plot([0,x_data[-1]], [final_number_of_cells/len(Ranks)/threads,final_number_of_cells/len(Ranks)/threads], "--", color="#000000", label="opt. #cells per tree" )
      
      
 
def plot_trees_per_rank( filename, verbose ):
  (ranks,threads) = get_ranks_and_threads( filename )
  #
  # Should be one colour per rank
  #
  Colours = [ "#0000ff", "#00ff00", "#ff0000", "#00ffff", "#ff00ff", "#ffff00", "#aaaaaa" ]
  Symbols = [ "o",       "s",       "<",       ">",       "^",       "v",       "x"       ]

  Ranks   = range(0,ranks)
  if ranks<=1: 
    Ranks = [-1]
  
  symbol_counter = 0
  colour_counter = 0
  my_alpha       = 0.1
  for rank in Ranks:
    if verbose:
      print( "parse data of rank " + str(rank) )
    ( x_data, y_data_local, y_data_remote ) = parse_file( filename, rank, verbose )

    y_data = []
    for snapshot in y_data_local:
      y_data.append( len(snapshot) )
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
  parser.add_argument("-sum",     "--sum-per-rank",      dest="sum_per_rank",       action="store_true",                    default=False,    help="sum up cells per rank")
  parser.add_argument("-opt",     "--show-optimum",      dest="show_optimum",       action="store_false",                   default=True,     help="display optimal number of cells per rank")
  parser.add_argument("-yscale",  "--yscale",            dest="yscale",             choices=["linear", "log-2", "log-10"],  default="linear", help="select scaling of y-axis" )
  parser.add_argument("-ncol",    "--columns-in-legend", dest="columns_in_legend",  type=int,                               default=1,        help="number of columns within legend" )
  args = parser.parse_args()

  plt.clf()
  plot_trees_per_rank(args.file, args.verbose )
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
  plot_cells_per_rank(args.file, args.verbose, args.plot_remote_cells, args.sum_per_rank, args.show_optimum )
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
  