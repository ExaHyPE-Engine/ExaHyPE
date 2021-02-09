import numpy as np
import matplotlib.pyplot as plt
from numpy.ma import masked_array
from pygraphviz import *

import re
import sys
import time
import argparse
from sympy.physics.units.definitions.dimension_definitions import current



if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='SmartMPI wait time plotter')
  parser.add_argument("file",            help="Filename of the file to parse (should be a text file)")
  parser.add_argument("-v", "--verbose", dest="verbose", action="store_true", help="Increase output verbosity", default=False)
  parser.add_argument("-s", "--select",  dest="select",  type=int, default=0, help="Define which rank to study. Ranks all have the same data, but their data might lag behind slightly." )
  parser.add_argument("-r", "--ranks",   dest="ranks",   type=int, required=True, help="Number of ranks." )
  parser.add_argument("-f", "--filter",  dest="filter",  default=False, action="store_true", help="Filter out light ranks." )
  args = parser.parse_args()

    
  file = open(args.file, 'r')

  waiting_times_pattern = re.compile("SmartMPI.*rank " + str(args.select) + "x([0-9]): (.*)")
  timestep_pattern      = re.compile(".([0-9]+).*step.*dt_\{max\}.*")

  current_step = 0
  current_waiting_times = np.zeros([args.ranks,args.ranks])
  
  for line in file:
    m=timestep_pattern.match(line)
    if m:
      max_wait_time = 0
      min_wait_time = 65546
      for i in range(args.ranks):
        for j in range(args.ranks):
          if i!=j:
            max_wait_time = max( max_wait_time, current_waiting_times[i][j] )
            min_wait_time = min( min_wait_time, current_waiting_times[i][j] )

      if args.verbose:
        print( "create new timestep visualisation for step " + str(current_step) + " (max=" + str(max_wait_time) + ",min=" + str(min_wait_time) + ")" )
      
      dot = AGraph(strict=False,directed=True,style="filled", label="timestep: "+str(current_step))

      for i in range(0,args.ranks):
        dot.add_node(str(i), label=str(i))

      for i in range(args.ranks):
        for j in range(args.ranks):
          time = current_waiting_times[i][j]
          if i==j and time>1e-8:
            print( "timing for " + str(i) + "x" + str(i) + " is not zero")

          if i!=j:
            my_color = "black"
            if time>=max_wait_time * 0.9:
              my_color = "red"
            if time<=min_wait_time + (max_wait_time-min_wait_time) * 0.1:
              my_color = "grey"
            if time>=min_wait_time + (max_wait_time-min_wait_time) * 0.1 or not args.filter:
              dot.add_edge(i, j, label="%0.1e"%time,fontcolor=my_color, color=my_color)
  
      output_file = "waiting_times_graph%03d.pdf" % current_step
      dot.draw(output_file, prog="dot")    
      current_step += 1

    m=waiting_times_pattern.match(line)
    if m:
      if args.verbose:
        print( "parse line " + line )
      src = int(m.group(1))
      times = m.group(2).split(" ")
      for i in range(0,args.ranks):
        dest = i
        time = float(times[i])
        if i==src and time>1e-8:
          print( "timing for " + str(i) + "x" + str(i) + " is not zero: " + line)
        current_waiting_times[src][dest]= time
  file.close()

