import os
import sys
import argparse

from enum import Enum

import matplotlib.pyplot as plt
import numpy as np


Markers = [ "o", "s", "<", ">", "^", "v", "D" ]
Colours = [ "#ff0000",
            "#00ff00",
            "#0000ff",
            "#abab00",
            "#ab00ab",
            "#00abab",
            "#787878",
            "#ffab50",
            "#ff50ab",
            "#abff50",
            "#50ffab",
            "#50abff",
            "#ab50ff",
            ]


Devices         = ["host", "device"]
CallSemantics   = ["heap-functors", "stateless"]
DataFlow        = ["patch-wise", "batched", "task-graph", "insitu"]
Parallelisation = ["sequential", "parallel-for", "subtasks", "managed", "usm", "copy"]


def parse(device, call_semantics, data_flow, parallelisation):
  """"
  Parse file and search for entries for particular argument combination
  We return two lists: The first one gives the total throughput per measurement
  while the second one gives the throughput for one kernel.
  """
  throughput_total   = []
  throughput_kernel  = []
  current_number_of_launching_threads = "xxx"
  current_number_of_compute_threads = "xxx"
  current_number_of_patches = "xxx"

  file = open( args.file, "r" )
  for line in file:
    if "Number of threads launching compute kernels" in line:
      current_number_of_launching_threads = line.split(":")[-1].strip()
    if "Number of compute threads" in line:
      current_number_of_compute_threads = line.split(":")[-1].strip()
    if "Number of patches per thread/compute kernel launch" in line:
      current_number_of_patches = line.split(":")[-1].strip()
    if device in line and call_semantics in line and data_flow in line and parallelisation in line:
      current_throughput_kernel = 1.0/float(line.split("|")[4])
      current_throughput_total  = 1.0/float(line.split("|")[2])
      throughput_kernel.append(current_throughput_kernel)
      throughput_total.append(current_throughput_total)
  return throughput_total, throughput_kernel


parser = argparse.ArgumentParser(description='ExaHyPE 2 - Finite Volumes Rusanov Kernels Benchmarking - Plotting Script')
parser.add_argument(dest="file", help="File to parse")
args = parser.parse_args()

for device in Devices:
  print( "Create plot for {}".format(device) )
  print( "============================" )
  plt.clf()
  marker_counter = 0
  for call_semantics in CallSemantics:
    for data_flow in DataFlow:
      for parallelisation in Parallelisation:
        throughput_total, throughput_kernel = parse(device, call_semantics, data_flow, parallelisation)
        assert len(throughput_total)==len(throughput_kernel), "data cardinalities {} vs {}".format(len(throughput_total), len(throughput_kernel))
        if len(throughput_total) > 0:
          print("data found for {}, {}, {}, {}".format(device, call_semantics, data_flow, parallelisation))
          plt.scatter( max(throughput_total),
                       max(throughput_kernel),
                       c      = Colours[marker_counter % len(Colours)],
                       marker = Markers[marker_counter % len(Markers)],
                       label  = device + ", " + call_semantics + ", " + data_flow + ", " + parallelisation,
                       )
          marker_counter += 1

  plt.title(device + " (measured FV throughput - higher is better)")
  plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
  plt.xlabel("Total throughput (dofs/s)")
  plt.ylabel("Throughput for one kernel (dofs/s)")

  output = args.file + "-" + device
  plt.savefig(output + ".png", bbox_inches='tight')
  #plt.savefig(output + ".pdf", bbox_inches='tight')

  plt.xscale('log')
  plt.yscale('log')
  plt.savefig(output + "-log.png", bbox_inches='tight')
  #plt.savefig(output + "-log.pdf", bbox_inches='tight')
