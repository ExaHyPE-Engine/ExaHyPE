import matplotlib.pyplot as plt
import os.path
import sys
import enum


Dimensions_Marker = [ "2d", "3d" ]
MPI_Marker = [ "no mpi", "with mpi" ]
Multithreading_Marker = [ "no threading", "omp", "tbb", "C++ threading" ]


class BuildReport(enum.Enum):
  Failed = 0,
  MPILevelNoSupported = 1,
  UnitTestsFailed = 2,
  Success = 3


def read_build_report( filename, dimensions, mpi, multithreading ):
  """
    Return a tuple. The first entry means whether the build is there, the second means that the unit tests have passed
  """
  try:
    print( "parse " + filename + " (" + dimensions + ", " + mpi + ", " + multithreading + ") ..." )
    file = open(filename, "r")
    found_build_summary  = False
    mpi_level_complaint  = False
    for line in file:
      if "MPI implementation does not support MPI_THREAD_MULTIPLE" in line:
        mpi_level_complaint = True
      if "build: " in line:
        if dimensions in line and mpi in line and multithreading in line:
          found_build_summary = True
      if "running test case collection ..." in line and "ok" in line and found_build_summary:
        if mpi_level_complaint:
          print( "mpi issues\n" )
          return BuildReport.MPILevelNoSupported
        else:
          print( "success\n" )
          return BuildReport.Success
      elif "running test case collection ..." in line and found_build_summary:
        print("unit tests failed\n")
        return BuildReport.UnitTestsFailed
      elif "running global test case collection" in line:
        mpi_level_complaint  = False

  except:
    pass
  print( "failed\n" )
  return BuildReport.Failed




def write_summary():  
  out = open( "build_summary.html", "w" )
  out.write( "<table>")
  out.write( "<tr><td />")
  for dimensions in Dimensions_Marker:
    out.write( "<td bgcolor=\"white\" align=\"center\" colspan=\"" + str(len(MPI_Marker)) + "\">" )   
    out.write( dimensions )   
    out.write( "</td>" )   
  out.write( "</tr>\n")
  out.write( "<tr><td />")
  for dimensions in Dimensions_Marker:
    for mpi in MPI_Marker:
      out.write( "<td bgcolor=\"white\" >" )
      out.write( mpi )
      out.write( "</td>" )
  out.write( "</tr>\n")
  for multithreading in Multithreading_Marker:
    out.write( "<tr>")
    out.write( "<td bgcolor=\"white\" >")
    out.write( multithreading )
    out.write( "</td>")
    for dimensions in Dimensions_Marker:
      for mpi in MPI_Marker:
        result = read_build_report(sys.argv[1],dimensions,mpi,multithreading)
        text   = ""
        color  = ""
        if result==BuildReport.Failed:
          text = "failed"
          color = "red"
        if result==BuildReport.MPILevelNoSupported:
          text = "mpi threading level not supported"
          color = "fuchsia"
        if result==BuildReport.UnitTestsFailed:
          text = "unit tests failed"
          color = "yellow"
        if result==BuildReport.Success:
          text = "success"
          color = "lime"
        out.write( "<td bgcolor=\"" + color + "\">" + text + "</td>" )
    out.write( "</tr>\n")
  out.write( "</table>")



if __name__ == "__main__":
  if len(sys.argv)!=2:
    print( "Usage: python3 " + sys.argv[0] + " input-file")
  else:
    write_summary()

  
