# This file is part of Peano's loadbalancing toolbox. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

class OutputFile(object):
    """!
    
    Represents  single output file dumped by Peano
    
    This class extracts all the load balancing-related information from
    an output file, i.e. is basically an abstraction of a file parser.
    It also holds some very basic runtime data.
    
    @param name File name
    """
    def __init__(self, 
                 name,
                 verbose = False
                 ):
                 
        self._file_name = name          
        self._has_parsed = False
        self._verbose = verbose
        pass

       
    def parse(self):
        """!
        
        Parse Peano output file
        
        Build up a tuple of (x_data,y_data_local,y_data_remote) internally
        but also extract some meta data
    
        """

        if self._has_parsed:
            return
        self._has_parsed = True

        if self._verbose:
            print( "parse file {}".format( self._file_name) )

        file = open( self._file_name, "r" )

        self._snapshot_lines = {}
        
        for line in file:
          if "build:" in line and "no mpi" in line: 
            if self._verbose:
                print( "code does not use MPI" )
            self._ranks = 1
          if "build:" in line and "threads" in line: 
            self._threads = int( line.split( "threads" )[0].split( "(" )[-1])
            if self._verbose:
                print( "code uses " + str(self._threads) + " threads per rank" )
          if "manually reset number of threads used to" in line: 
            self._threads = int( line.split( "to" )[1])
            if self._verbose:
                print( "code sets number of threads manually to " + str(self._threads) )
          if "build:" in line and "ranks)" in line: 
            self._ranks = int( line.split( "ranks)" )[0].split("(")[1])
            if self._verbose:
                print( "code uses " + str(self._ranks) + " ranks" )

          if "time stepping:" in line:
            self._runtime = float( line.split( "time stepping:" )[1].split( "s")[0] )
            if self._verbose:
                print( "code runs for " + str(self._runtime) + "s" )

                      
          if "toolbox::loadbalancing::dumpStatistics" in line:
              if "rank:" in line:
                  current_rank = int( line.split( "rank:" )[1].split(" ")[0] )
              else:
                  current_rank = 0
              if not current_rank in self._snapshot_lines:
                  self._snapshot_lines[ current_rank ] = []
                  if self._verbose:
                      print( "create new dataset for rank {}".format(current_rank) )
                  
              self._snapshot_lines[ current_rank ].append( line.split( "toolbox::loadbalancing::dumpStatistics()")[1] ) 

              assert current_rank in self._snapshot_lines
                  

            

    @property
    def ranks(self):
        self.parse()
        return self._ranks 
        

    @property
    def threads_per_rank(self):
        self.parse()
        return self._threads


    @property
    def runtime(self):
        """!
        
        Read out the total runtime 
        
        """
        self.parse()
        return self._runtime

        
    def trees_per_snapshot( self, 
                            rank
                           ):
        """!
        
        Return a sequence of integers that holds the number of trees per snapshot
        
        """                           
        self.parse()
        assert rank>=0
        assert rank<self._ranks                   
        
        result = []
        for entry in self._snapshot_lines[rank]:
          number_of_trees = int( entry.split( "tree(s)" )[0] )
          result.append( number_of_trees )
          
        return result


    def local_cells_per_snapshot( self, 
                                  rank
                                 ):
        """!
        
        Return local cells per snaphot
        
        This is a sequence of sequences. For each snapshot for this
        rank, we store one sequence with all the local cells in this
        snapshot. The standard plot_load_distribution.py script feeds
        this data into a scatter plot. For many cases, we might however
        be interested in the maximum value:
        
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        raw_data = ouput_file.local_cells_per_snapshot(rank)
        max_data = [max(x) for x in raw_data ]
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        
        """                           
        self.parse()
        assert rank>=0
        assert rank<self._ranks                   
    
        result = []
        for snapshot in self._snapshot_lines[rank]:
          result.append( [] )
          tree_entries_in_line = snapshot.split( "(#")
          for tree_entry in tree_entries_in_line[1:]:
              new_count = int( tree_entry.split(":")[1].split("/")[0] )
              result[-1].append(new_count)
          
        return result
        

    def virtual_cells_per_snapshot( self, 
                                  rank
                                 ):
        """!
        
        Return local cells per snaphot
        
        This is a sequence of sequences. For each snapshot for this
        rank, we store one sequence with all the local cells in this
        snapshot.
        
        """                           
        self.parse()
        assert rank>=0
        assert rank<self._ranks                   
    
        result = []
        for snapshot in self._snapshot_lines[rank]:
          result.append( [] )
          tree_entries_in_line = snapshot.split( "(#")
          for tree_entry in tree_entries_in_line[1:]:
              new_count = int( tree_entry.split("/")[1].split(")")[0] )
              result[-1].append(new_count)
          
        return result


    def total_local_cells_per_snapshot( self, 
                                        rank
                                       ):
        """!
        
        Return the total number of cells per snapshot for this rank
        
        The total number of cells is an interesting quantity, as it allows
        us to compare the load of different ranks.
        
        """
        local_cell_per_snapshot = self.local_cells_per_snapshot( rank )
        
        result = [] 
        for snapshot_data in local_cell_per_snapshot:
            new_entry = sum( snapshot_data )
            result.append(new_entry)
        return result


    def min_local_cells_per_snapshot( self, 
                                        rank
                                       ):
        """!
        
        Return the total number of cells per snapshot for this rank
        
        The total number of cells is an interesting quantity, as it allows
        us to compare the load of different ranks.
        
        """
        local_cell_per_snapshot = self.local_cells_per_snapshot( rank )
        
        result = [] 
        for snapshot_data in local_cell_per_snapshot:
            new_entry = min( snapshot_data )
            result.append(new_entry)
        return result

 
    def max_local_cells_per_snapshot( self, 
                                        rank
                                       ):
        """!
        
        Return the total number of cells per snapshot for this rank
        
        The total number of cells is an interesting quantity, as it allows
        us to compare the load of different ranks.
        
        """
        local_cell_per_snapshot = self.local_cells_per_snapshot( rank )
        
        result = [] 
        for snapshot_data in local_cell_per_snapshot:
            new_entry = max( snapshot_data )
            result.append(new_entry)
        return result

        
    def min_virtual_cells_per_snapshot( self, 
                                        rank
                                       ):
        """!
        
        Return the total number of cells per snapshot for this rank
        
        The total number of cells is an interesting quantity, as it allows
        us to compare the load of different ranks.
        
        """
        virtual_cell_per_snapshot = self.virtual_cells_per_snapshot( rank )
        
        result = [] 
        for snapshot_data in virtual_cell_per_snapshot:
            new_entry = min( snapshot_data )
            result.append(new_entry)
        return result

 
    def max_virtual_cells_per_snapshot( self, 
                                        rank
                                       ):
        """!
        
        Return the total number of cells per snapshot for this rank
        
        The total number of cells is an interesting quantity, as it allows
        us to compare the load of different ranks.
        
        """
        virtual_cell_per_snapshot = self.virtual_cells_per_snapshot( rank )
        
        result = [] 
        for snapshot_data in virtual_cell_per_snapshot:
            new_entry = max( snapshot_data )
            result.append(new_entry)
        return result
        
        
    def is_valid(self):
        return hasattr(self, "_runtime")        
        