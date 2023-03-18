
# Readme of a Peano 4 project

This experiment did rely on Peano 4 or a toolbox created on top of Peano.   
If you publish results obtained with this source code, we would appreciate  
if you could point to the project homepage                                  
                                                                            
http://www.peano-framework.org/                                             
                                                                            
and cite the appropriate papers from below.


## Peano 4 ##                                  
                                                                            
Peano's core ideas, algorithms and (usage) vision are best discussed in the 
TOMS paper below. The TOMS paper describes the third generation of Peano. 
While it is thus not a perfect description, the core concepts by means of 
data structures and user model is the same as in Peano 4. Please cite this
paper when you use the software:


        @article{Weinzierl:2019:Peano,                    
          author = {Weinzierl, Tobias},                   
          title = {The Peano Software-Parallel, Automaton-Based, Dynamically Adaptive Grid Traversals},                    
          year = {2019},                    
          issue_date = {June 2019},                    
          publisher = {Association for Computing Machinery},                    
          address = {New York, NY, USA},                    
          volume = {45},                    
          number = {2},                    
          issn = {0098-3500},                    
          url = {https://doi.org/10.1145/3319797},                    
          doi = {10.1145/3319797},                    
          abstract = {We discuss the design decisions, design alternatives, and rationale behind the third generation of Peano, a framework for dynamically adaptive Cartesian meshes derived from spacetrees. Peano ties the mesh traversal to the mesh storage and supports only one element-wise traversal order resulting from space-filling curves. The user is not free to choose a traversal order herself. The traversal can exploit regular grid subregions and shared memory as well as distributed memory systems with almost no modifications to a serial application code. We formalize the software design by means of two interacting automata-one automaton for the multiscale grid traversal and one for the application-specific algorithmic steps. This yields a callback-based programming paradigm. We further sketch the supported application types and the two data storage schemes realized before we detail high-performance computing aspects and lessons learned. Special emphasis is put on observations regarding the used programming idioms and algorithmic concepts. This transforms our report from a "one way to implement things" code description into a generic discussion and summary of some alternatives, rationale, and design decisions to be made for any tree-based adaptive mesh refinement software.},
          journal = {ACM Trans. Math. Softw.},                              month = apr,                    
          articleno = {14},                    
          numpages = {41},                    
          keywords = {Software, spacetree, adaptive mesh refinement, parallel multiscale grid traversal
        }


There is an earlier publication about Peano in SISC which discusses the core 
algorithms with the stacks in great detail. This paper can be seen as release 
paper for previous Peano code generations (generation 2).


        @article{10.1137/100799071,
          author = {Weinzierl, Tobias and Mehl, Miriam},
          title = {Peano-A Traversal and Storage Scheme for Octree-Like Adaptive Cartesian Multiscale Grids}, 
          year = {2011}, 
          issue_date = {September 2011}, 
          publisher = {Society for Industrial and Applied Mathematics}, 
          address = {USA}, 
          volume = {33}, 
          number = {5}, 
          issn = {1064-8275}, 
          url = {https://doi.org/10.1137/100799071}, 
          doi = {10.1137/100799071}, 
          abstract = {Almost all approaches to solving partial differential equations (PDEs) are based upon a spatial discretization of the computational domain - a grid. This paper presents an algorithm to generate, store, and traverse a hierarchy of $d$-dimensional Cartesian grids represented by a $(k=3)$-spacetree, a generalization of the well-known octree concept, and it also shows the correctness of the approach. These grids may change their adaptive structure throughout the traversal. The algorithm uses $2d+4$ stacks as data structures for both cells and vertices, and the storage requirements for the pure grid reduce to one bit per vertex for both the complete grid connectivity structure and the multilevel grid relations. Since the traversal algorithm uses only stacks, the algorithm's cache hit rate is continually higher than 99.9 percent, and the runtime per vertex remains almost constant; i.e., it does not depend on the overall number of vertices or the adaptivity pattern. We use the algorithmic approach as the fundamental concept for a mesh management for $d$-dimensional PDEs and for a matrix-free PDE solver represented by a compact discrete $3^d$-point operator. In the latter case, one can implement a Jacobi smoother, a Krylov solver, or a geometric multigrid scheme within the presented traversal scheme which inherits the low memory requirements and the good memory access characteristics directly.}, 
          journal = {SIAM J. Sci. Comput.}, 
          month = oct, 
          pages = {2732--2760}, 
          numpages = {29}, 
          keywords = {octree, cache efficiency, multiscale, space-filling curve, adaptive Cartesian grid, spacetree, partial differential equation} 
        } 

                                                                            
 
    
## Information provided by used packages    
    


### Peano4 PETSc backend

This yet has to be written.




### MPI parallelisation

Peano's distributed memory parallelisation is based upon plain space-filling 
curves which are used first to split the cells among the ranks, and, after   
that, to split the cells into chunks and deploy them onto cores (if shared   
memory is used). There is no particularly novel contribution/paper on this   
aspect of the code. The paper that is (algorithmically) most interesting is  
an old EuroPar publication:                                                 


       @InProceedings{Bungartz:2006:Parallelisation,
         author={Bungartz, Hans-Joachim and Mehl, Miriam and Weinzierl, Tobias}, 
         editor={Nagel, Wolfgang E. and Walter, Wolfgang V. and Lehner, Wolfgang}, 
         title={A Parallel Adaptive Cartesian PDE Solver Using Space--Filling Curves},  
         booktitle={Euro-Par 2006 Parallel Processing}, 
         year={2006}, 
         publisher={Springer Berlin Heidelberg}, 
         address={Berlin, Heidelberg}, 
         pages={1064--1074}, 
         abstract={In this paper, we present a parallel multigrid PDE solver working on adaptive hierarchical cartesian grids. The presentation is restricted to the linear elliptic operator of second order, but extensions are possible and have already been realised as prototypes. Within the solver the handling of the vertices and the degrees of freedom associated to them is implemented solely using stacks and iterates of a Peano space--filling curve. Thus, due to the structuredness of the grid, two administrative bits per vertex are sufficient to store both geometry and grid refinement information. The implementation and parallel extension, using a space--filling curve to obtain a load balanced domain decomposition, will be formalised. In view of the fact that we are using a multigrid solver of linear complexity {\$}{\backslash}mathcal{\{}O{\}}(n){\$}, it has to be ensured that communication cost and, hence, the parallel algorithm's overall complexity do not exceed this linear behaviour.}, 
         isbn={978-3-540-37784-9} 
       } 




### OpenMP parallelisation

Peano 4 run uses a wrapper around OpenMP to obtain a high task efficiency. 
The wrapper can be read as user-level threading implemented on top of OpenMP's 
tasking mechanism. It is described in
                                                                            
       @article{Schulz:2021:Tasking,   
         title = {Task inefficiency patterns for a wave equation solver},   
         journal = {IWOMP},   "
         year = {2021},   
         author = {Holger Schulz and Gonzalo Brito Gadeschi and Oleksandr Rudyy and Tobias Weinzierl},
      } 






Peano relies on a mixture of classic domain decomposition and task-based  
parallelism. The domain decomposition provides the baseline performance,  
and the tasking adds the big flexibility and scalability gain on top. The 
key publication discussing the overall idea and algorithmic ingredients is
the SISC paper
                                                                            
       @article{Charrier:2020:Enclave,
         author = {Charrier, Dominic Etienne and Hazelwood, Benjamin and Weinzierl, Tobias},
         title = {Enclave Tasking for DG Methods on Dynamically Adaptive Meshes},
         journal = {SIAM Journal on Scientific Computing},
         volume = {42},
         number = {3},
         pages = {C69-C96},
         year = {2020},
         doi = {10.1137/19M1276194},
         URL = {https://doi.org/10.1137/19M1276194},
         eprint = {https://doi.org/10.1137/19M1276194}
       }


 
    
## Information provided by data model
    

## Makefile settings

CXX             = mpiicpc
CXXFLAGS        = -fsycl -fma -fomit-frame-pointer -Wno-unknown-attributes -Ofast -march=native -mtune=native --std=c++20 -fiopenmp -fopenmp-targets=spir64 -I/home/tobias/git/otter/include -I/opt/otf2/include -g -I/opt/intel/oneapi/itac/2021.8.0/include
FC              = gfortran
FCFLAGS         = 
LDFLAGS         = -Ofast -g -fiopenmp -fopenmp-targets=spir64 -L/home/tobias/git/otter/build/lib -L/opt/otf2/lib -fsycl -L/opt/intel/oneapi/itac/2021.8.0/lib -fopenmp -fopenmp-targets=spir64 
GPUOBJS         = 
LIBS            = -ltbb -lVT -ldwarf -lelf -lvtunwind -lm -lpthread -lz -lotter-serial -lotf2
DIM             = 2
CONFIGUREPATH   = ../../../../
EXECUTABLENAME  = peano4petsc
FORTRAN_MODULES = []
MODE            = RELEASE


## Compiler version

Intel(R) oneAPI DPC++/C++ Compiler 2023.0.0 (2023.0.0.20221201)
Target: x86_64-unknown-linux-gnu
Thread model: posix
InstalledDir: /opt/intel/oneapi/compiler/2023.0.0/linux/bin-llvm
Configuration file: /opt/intel/oneapi/compiler/2023.0.0/linux/bin-llvm/../bin/icpx.cfg



## Summary

File has been generated by    

python3  poisson.py -m release

www.peano-framework.org
(C) 2021-  Tobias Weinzierl    
    
