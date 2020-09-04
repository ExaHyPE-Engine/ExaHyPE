import matplotlib.pyplot as plt
import os.path
import sys
from fileinput import filename


D     = [2,3]
H     = [ 0.5,       0.2,       0.1,       0.05,      0.02,   0.01,  0.005,  0.002,   0.001 ]
Flops = [ 1,         100,       10000,     1000000 ]
Color = [ "#ff0000", "#00ff00", "#0000ff", "#a0a000", "#a000a0", "#00a0a0", "#998899", "#aa8888", "#88aa88", "#8888aa" ]
BenchmarkIterations = 20



def filter_file(filename):
  if os.path.exists( filename ):
    print( "found " + filename + " write " + filename + ".filter" )
    input_file   = open(filename, "r")
    output_file  = open(filename, "w")
    for line in file:
      if ("start parallel traversals" in line) or ("local unrefined cells" in line) or "terminated successfully" in line:
        output_file.write( line )


def get_shared_memory_file_name(cores,d,h,flops):    
  return "shared-memory-" + str(cores) + "-cores-" + str(d) + "d-" + str(h) + "-" + str(flops) + "-flops.out"


def get_distributed_memory_file_name(nodes,d,h,flops):    
  return "distributed-memory-" + str(nodes) + "-nodes-" + str(d) + "d-" + str(h) + "-" + str(flops) + "-flops.out"

    
def filter_shared_memory():
  for d in [2,3]:
    for h in H:
      for flops in Flops:
        for cores in range(1,64):
          filename = get_shared_memory_file_name(cores,d,h,flops)
          filter_file(filename)


def parse_file(filename):
  """
    return a tuple of new entry plus max cells. First entry in tuple is 0.0 if no data found
  """
  file       = open(filename, "r")
  start_time = 0
  new_entry  = 0.0
  max_cells  = 0
  for line in file:
    if "start parallel traversals" in line:
      start_time = int( line.split(" ")[0] )
    if "local unrefined cells" in line:
      max_cells = float( line.split("=")[-1] )
    if "terminated successfully" in line:
      time      = int( line.split(" ")[0] )
      new_entry = (time - start_time)
  return (new_entry,max_cells)


def normalise_raw_times(time_data,max_cells):
   """
    Divides each entry through max number of cells and iterations
   """
   return [i/BenchmarkIterations/max_cells for i in time_data]

def plot_shared_memory():
  for d in [2,3]:
    plt.clf()
    max_cores = 0
    max_serial_time = 0.0
    min_serial_time = 65536.0
    max_cores=0
    for h in H:
      for flops in Flops:
        max_cells = 0.0
        ydata = []
        xdata = []
        for cores in range(1,64):
          found_file = False
          filename = get_shared_memory_file_name(cores,d,h,flops)
          if os.path.exists( filename+".filter" ):
            found_file  = True
            filename   += ".filter"
          elif os.path.exists( filename ):
            found_file = True
          
          if found_file:
            print( "found " + filename )
            
            (new_entry,cells) = parse_file(filename)
            if new_entry>0.0:
                max_cells = max(cells,max_cells)
                max_cores = max(cores,max_cores)
                xdata.append(cores)
                ydata.append(new_entry)
                if cores==1:
                  max_serial_time = max(max_serial_time, new_entry/max_cells)
                  min_serial_time = min(min_serial_time, new_entry/max_cells)

        if len(xdata)>0:
          ydata_calibrated = normalise_raw_times(ydata,max_cells)
          #if H.index(h)==0 or Flops.index(flops)==0:
          plt.plot( 
              xdata, ydata_calibrated, 
              label="h=" + str(h) + ",#flops/cell=" + str(flops), 
              color=Color[H.index(h)],
              marker=Flops.index(flops)+4 
          )
          #else:
          #  plt.plot( 
          #    xdata, ydata_calibrated, 
          #    color=Color[H.index(h)],
          #    marker=Flops.index(flops)+4 
          #  )
            
    xdata = range(1,max_cores)
    plt.plot(
      xdata, [max_serial_time/i for i in xdata],
      "--", color="#000000"
    )
    plt.plot(
      xdata, [min_serial_time/i for i in xdata],
      "--", color="#000000"
    )
            
    plt.legend()
    plt.xlabel( "cores" )
    plt.ylabel( "time/cell [t]=ms" )
    plt.xscale( "log", basex=2 )
    plt.yscale( "log", basey=2 )
    xtics   = [ 1 ]
    xlabels = [ "serial" ]
    while xtics[-1] < max_cores:
      xtics.append( xtics[-1]*2 )
      xlabels.append( str(xtics[-1]) )
    plt.xticks( xtics, xlabels )
    plt.title( "shared memory scaling " + str(d) + "d" )
    plt.savefig( "shared-memory-" + str(d) + "d.pdf" )
    plt.savefig( "shared-memory-" + str(d) + "d.png" )


if __name__ == "__main__":
  if len(sys.argv)!=3:
    print( "Usage: python " + sys.argv[0] + " [option] [target]")
    print( "" )
    print( "With option from" )
    print( "  filter              filters out files, i.e. throws away all entries which are not required" )
    print( "  create-plots        generate plots" )
    print( "" )
    print( "With option from" )
    print( "  shared-memory       process shared memory data" )
    print( "  distributed-memory  process distributed memory data" )
  elif sys.argv[1]=="create-plots" and sys.argv[2]=="shared-memory":
    plot_shared_memory()
  elif sys.argv[1]=="filter" and sys.argv[2]=="shared-memory":
    filter_shared_memory()
  elif sys.argv[1]=="create-plots" and sys.argv[2]=="distributed-memory":
    plot_distributed_memory()
  elif sys.argv[1]=="filter" and sys.argv[2]=="distributed-memory":
    filter_distributed_memory()
  else:
    print( "Invalid option. Run without arguments for usage message" )
  
  
