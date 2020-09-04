# Delta demonstrator #


## Configure build ##

We assume Delta is built. Before you do anything, ensure your LDFLAGS are set
properly:

export LDFLAGS=-L/home/tobias/git/Delta/src/delta

or where ever you have installed the Delta libraries.
Then, rerun configure with 

./configure --with-multithreading=cpp --with-delta=/home/tobias/git/Delta/src --with-vtk=/usr/include/vtk-8.2

and recompile. As our code tends to link statically, this is it. No need to 
set any further compile flags.

If you need further troubleshooting, use 

make V=1 

instead of make to see what exactly the build parameters are.


## Run and visualise outcome ##

./Delta_debug
~/git/Peano/src/visualisation/convert inspect marker.peano-patch-file
~/git/Peano/src/visualisation/convert convert-file marker.peano-patch-file cell-data output vtu


## Brief code overview ##

Most of the Delta stuff is in InitData.cpp. 

- I create an instance of Delta (type ::delta::Mesh*) in my object constructor. In this case, it is 
  the InitData constructor, and the instance is called _primitive.   
- At any time, I can dump the Delta geometry into a vtk file by invoking ::delta::io::writeVTK on 
  primitive.
- Instead of creating a built-in geometry, Delta also provides some routines to read in a file. This 
  is not used here.
- When I create a cell, I iterate over all the points/nodes within the cell. In the present example, 
  I use a Cartesian point layout, but it could be Gauss Lobatto nodes, e.g., as well.
- Per node, I invoke ::delta::contactdetection::sphereToTriangle. Technically, each node for which I 
  wanna get the vector is a degenerated sphere. Consequently, I pass the radius 0 to sphereToTriangle.
  Epsilon is the search distance (in which area around a point should Delta search for contacts). I 
  set it to the distance between two nodes here, but it could be any bigger value as well.
- sphereToTriangle doesn't look for the closest triangle around a sphere/point. It looks for all 
  triangles within the specified area. So next, I sort all the found triangles via 
  ::delta::contactdetection::filter. As the name suggest, I could also use this one to filter out 
  triangles that are "too close". Don't need that this time.
- As I'm only interested in the closest triangle, I ignore all entries in the result of filter besides
  the very first one. 
  
  
  



