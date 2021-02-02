import numpy as np
import matplotlib.pyplot as plt
from numpy.ma import masked_array
from pygraphviz import *

import re
import sys
import time
import argparse
from sympy.physics.units.definitions.dimension_definitions import current


waiting_times_pattern = re.compile("SmartMPI.*rank ([0-9]): (.*)")
timestep_pattern = re.compile(".([0-9]+).*step.*dt_\{max\}.*")

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='SmartMPI wait time plotter')
  parser.add_argument("file",            help="filename of the file to parse (should be a text file)")
  parser.add_argument("-v", "--verbose", dest="verbose", action="store_true", help="increase output verbosity", default=False)
  parser.add_argument("-r", "--rank",    dest="rank", type=int, required=True, help="Ranks to study. Each rank might have a slightly biased/delayed view of the total world.", default=0)
  args = parser.parse_args()

    
  file = open(args.file, 'r')


  current_step = 0
  wait_times = []
  
  for line in file:
    m=timestep_pattern.match(line)
    if m:
      if args.verbose:
        print( "create new timestep visualisation for step " + str(current_step) )

      #tmp = current_waiting_times.flatten()
      #tmp.sort()
      #lower_filter = (tmp[int(len(tmp)*edge_filter_threshold_lower)])
      #upper_filter = (tmp[int(len(tmp)*edge_filter_threshold_upper)])

      dot = AGraph(strict=False,directed=True,style="filled", label="timestep: "+str(current_step))

Ich weiss net, auf wen wir warten

      for i in range(0,len(wait_times)):
        dot.add_node(str(i), label=str(i))

      for i in range(args.ranks):
        for j in range(args.ranks):
          time = current_waiting_times[i][j]
          #if(time<lower_filter):
          #dot.add_edge(i, j, label=str(time),fontcolor="green", color="green")
          #if(time>upper_filter):
          #dot.add_edge(i, j, label=str(time),fontcolor="red", color="red")
          dot.add_edge(i, j, label=str(time),fontcolor="black", color="black")
  
      output_file = "waiting_times_graph%03d.pdf" % current_step
      dot.draw(output_file, prog="dot")    
      current_step += 1

    m=waiting_times_pattern.match(line)
    if m:
      src = int(m.group(1))
      times = m.group(2).split(",")
      for i in range(0,args.ranks):
        dest = i
        time = float(times[i])
        current_waiting_times[src][dest]= time
  file.close()

