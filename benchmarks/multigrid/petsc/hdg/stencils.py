'''!
in this file, we store stencils. saves space in hdg.py

for FC and CF, we need one for each face. we store them in 
a list. see the enumeration order described in 
../../../../documentation/Doxygen/Multigrid/Enumeration.dox for 
the way to order them.

'''

#HARDCODE CELLDOFS AND FACEDOFS
cellDofs=4
faceDofs=2

#store 2 * Dimensions stencils in this list
#each stencil is itself a list of lists
#there should be one list for each row, and those list
#should have the desired number of columns
stencilsCF=[
  ##0th stencil (left side)
  [
    [1 for _ in range(faceDofs)]
       for _ in range(cellDofs)
  ],
  #1st stencil (bottom)
  [
    [1 for _ in range(faceDofs)]
       for _ in range(cellDofs)
  ],
  #2nd (right side)
  [
    [1 for _ in range(faceDofs)]
       for _ in range(cellDofs)
  ],
  #3rd (top)
  [
    [1 for _ in range(faceDofs)]
       for _ in range(cellDofs)
  ],
]

stencilsFC=[
  [
    [1 for _ in range(cellDofs)]
      for _ in range(faceDofs)
  ],
  [
    [1 for _ in range(cellDofs)]
      for _ in range(faceDofs)
  ],
  [
    [1 for _ in range(cellDofs)]
      for _ in range(faceDofs)
  ],
  [
    [1 for _ in range(cellDofs)]
      for _ in range(faceDofs)
  ],
]

stencilCC=[
  [1 for _ in range(cellDofs)]
     for _ in range(cellDofs)
]

stencilFF=[
  [1 for _ in range(faceDofs)]
     for _ in range(faceDofs)
]
