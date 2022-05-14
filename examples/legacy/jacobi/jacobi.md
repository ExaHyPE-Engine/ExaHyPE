# Jacobi solvers/solvers # 

A collection of simple Jacobi solvers which demonstrate, step by step, how 
one can write a smoother for multigrid within Peano. Each individual Python script 
in the root directory here is one complete solver example for the same 
pathologic setup. The simplest one is the Jacobi solver with a geometric, 
low-order rediscretisation. From thereon, each example adds a different level 
of complexity. Some of them build upon each other. Others are orthogonal.



## Solver variants ##

- matrix-free
    Baseline code that we use as a template for all of the subsequent examples.
    It solves the problem with a d-linear elements where the material parameter
    is sampled once per cell. Therefore, we can evaluate it on-the-fly. So this 
    is a pure matrix-free solver. It demonstrates how to use Peano 4 through its
    Python interface. It also highlights how important it is for these kind of 
    codes to fuse grid sweeps whenever possible.
    
    Files that have to be studied/defined for this setup:
    - SetupScenario - generic setup class holding the problem description that 
      I use for all of the subsequent examples.
    - JacobiUpdate - this is a generic update of the Jacobi smoother
    - ComputeGlobalResidual - again a generic action set. As we know the 
      analytic solution, we can compute  the global residuals, too.
    

- amr-parallel
    This second step discusses one functional and one non-functional extension: 
    I write a dynamic AMR variant of the Jacobi (the first step solely works on 
    a regular grid) and I then parallelise this code using MPI and TBB.  


Wie verwenden wir verschiedene mains?    
    
    
    
in-situ storage

compression

multitasking





