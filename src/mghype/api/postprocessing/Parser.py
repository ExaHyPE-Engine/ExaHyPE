# This file is part of the Peano multigrid project. For conditions of 
# distribution and use, please see the copyright notice at www.peano-framework.org
import numpy as np
import peano4
import mghype
import re


def parse_residuals( file_name,
                     solver_name,
                     ):
    """!
        
    Parse residuals from input stream
    
    @param file_name: String
      We will open the file through ```open( file_name, "r").
    
    @param solver_name: String
      We have to know which solver to search for, as MGHyPE simulations might 
      host multiple solvers.
      
    @return [Float], [Float], [Float]
      Returns three sequences, all of the same size. The first sequence is the 
      development of the residuals in the max norm, the second in the 2 norm,
      and the third one hosts the residuals in the h norm.
      
    """
    res_max = []
    res_2   = []
    res_h   = []
        
    pattern = r"([-+]?\d*\.?\d+(?:e[-+]?\d+)?)"
    full_pattern = rf"\|r\|\_max={pattern},\s*\|r\|\_2={pattern},\s*\|r\|\_h={pattern}"
    with open(file_name, 'r') as file:
        for line in file.readlines():
            norms = re.search(full_pattern, line)
            if "endMeshSweep()" in line and solver_name in line and norms!=None:
                r_max, r_2, r_h = map(float, norms.groups())  # Convert to floats
                res_max.append(r_max)
                res_2.append(r_2)
                res_h.append(r_h)

    return res_max, res_2, res_h           
        
        
def parse_solution_updates( file_name,
                            solver_name,
                          ):
    """!
        
    Parse residuals from input stream
    
    @param file_name: String
      We will open the file through ```open( file_name, "r").
    
    @param solver_name: String
      We have to know which solver to search for, as MGHyPE simulations might 
      host multiple solvers.
      
    @return [Float], [Float], [Float]
      Returns three sequences, all of the same size. The first sequence is the 
      development of the solution updates in the max norm, the second in the 2 norm,
      and the third one hosts the deltas in the h norm.
      
    """
    result_max = []
    result_2   = []
    result_h   = []
        
    pattern = r"([-+]?\d*\.?\d+(?:e[-+]?\d+)?)"
    full_pattern = rf"\|du\|\_max={pattern},\s*\|du\|\_2={pattern},\s*\|du\|\_h={pattern}"
    with open(file_name, 'r') as file:
        for line in file.readlines():
            norms = re.search(full_pattern, line)
            if "endMeshSweep()" in line and solver_name in line and norms!=None:
                du_max, du_2, du_h = map(float, norms.groups())  # Convert to floats
                result_max.append(du_max)
                result_2.append(du_2)
                result_h.append(du_h)

    return result_max, result_2, result_h           
    