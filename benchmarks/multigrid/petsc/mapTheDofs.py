import numpy as np
import sys, os
from pdb import set_trace as bp

def parse_cell_line(line):
  # bp()
  start_of_cell_centre = line.index("x=[") + len("x=[")
  line   = line[start_of_cell_centre:]
  cell_x = float(line[:line.index(",")])
  line   = line[ line.index(",") + 1 : ]
  cell_y = float(line[ :line.index("]") ])

  # clip again
  line = line[ line.index("globalCellIndex:") + len("globalCellIndex:") : ]
  dof  = int(line[:line.index(" ")])

  return dof, [cell_x, cell_y]

def map_cell_centre_to_face_centre(cell_centre, face_number):
  map_dict = {0:[-1/3,0], 1:[0,-1/3], 2:[1/3,0], 3:[0,1/3]}
  out      = [0 for _ in range(len(cell_centre))]
  for i in range(len(cell_centre)):
    out[i] = cell_centre[i] + map_dict[face_number][i] / 2
  return out

def parse_face_line(line):
  # clip the line to start of cell centre
  # bp()
  start_of_cell_centre = line.index("cell-centre=[") + len("cell-centre=[")
  line   = line[start_of_cell_centre:]
  cell_x = float(line[:line.index(",")])
  # clip again
  line   = line[ line.index(",") + 1 : ]
  cell_y = float(line[ :line.index("]") ])
  #clip again
  line        = line[ line.index("select=") + len("select=") : ]
  face_no     = int(line[:line.index(",")])
  face_centre = map_cell_centre_to_face_centre([cell_x, cell_y], face_no)
  
  # we have multiple names for the first dof index
  # clip line to end of marker.x()
  line = line[line.index(")"):]
  # clip to where val is written
  line = line[line.index(":")+1:]

  dof  = int(line[:line.index(" ")])
  return dof, face_centre

def read(file_name):
  face_output = {}
  cell_output = {}

  with open(file_name, "r") as f:
    line  = f.readline()
    while line:
      if "tcft_experimenter" in line and "out" not in line:
        cell_dof, cell_centre = parse_cell_line(line)
        cell_output[cell_dof] = cell_centre
      if "tfft_experimenter" in line and "out" not in line:
        face_dof, face_centre = parse_face_line(line)
        face_output[face_dof] = face_centre
      line = f.readline()
  print("printing face output....\n")
  print(face_output)
  print("\nprinting cell output....\n")
  print(cell_output)

if __name__ == "__main__":
  assert len(sys.argv) > 1, "run this script with: python3 mapTheDofs.py file_to_be_parsed.txt"
  try:
    read(sys.argv[1])
  except Exception as e: 
    print(f"Got exception {e}.")
    print("Did you pass the right file?")
