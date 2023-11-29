#read an xml file containing a petsc object
import numpy as np

def getMatDimensions(fName):
  output = 0
  with open(fName) as f:
    for _ in range(3): #skip the first two lines
      l = f.readline()
    while l:
      output += 1
      l = f.readline()
  return output

#advances an index
def readUntil(line, character, index=0):
  # print(line)
  output = line[index:].index(character) 
  return output + index

#get the pair (col, entry), chop the line
#and then return all three
def getFirstColNumberAndEntryFromLine(line):
  # print(line)
  colNumber = line[ readUntil(line,'(') + 1: readUntil(line,',') ]
  #convert to float
  colNumber = float(colNumber)
  # print(colNumber)
  #chop

  val = line[ readUntil(line, ',')+1 : readUntil(line, ')') ]
  val = float(val)

  #chop the line
  line = line[ readUntil(line, ')') + 1: ]
  # print(line)

  return int(colNumber), val, line

def convertLineToVector(line, dim):
  #chop off first part
  line = line[readUntil(line,'('):]

  output = np.zeros((dim))
  while ')' in line:
    col, val, line = getFirstColNumberAndEntryFromLine(line)
    output[col] = val

  return output
  

def populateMatrix(fName):
  dim = getMatDimensions(fName)
  output = np.zeros((dim,dim))
  with open(fName) as f:
    for _ in range(3): #skip the first two lines
      l = f.readline()
    
    row = 0
    while l:
      if l == "\n":
        break
      output[row,:] = convertLineToVector(l, dim)
      row += 1
      l = f.readline() # read the line
  return output

def populateRHS(fName, solDim=None):
  dim = solDim if solDim else getMatDimensions(fName)
  vec = np.zeros((dim))
  with open(fName) as f:
    for _ in range(3): #skip the first two lines
      l = f.readline()
  
    row = 0
    while l and row < dim:
      if l == '\n':
        break
      vec[row] = float(l)
      row += 1
      l=f.readline()
  return vec

def clip(array):
  return np.where( np.abs(array) < 1E-10, 0, array )

def getRankOfMatrix(mat):
  return np.linalg.matrix_rank(mat)

def get_null_vector(mat):
  _,_, vh = np.linalg.svd(mat)
  return clip(vh[-1])

if __name__ == "__main__":
  print("this file is here for development. please remove from git once satisfied with matrix")

  #hardcoding file names!

  dim = getMatDimensions("mat.xml")

  try:
    mat = populateMatrix("mat.xml")
  except:
    print("no mat.xml file available")
    pass
  try:
    rhs = populateRHS("rhs.xml")
  except:
    print("no rhs.xml available")
    pass
  try:
    sol = populateRHS("sol.xml")
  except:
    pass

  print(f'shape of this matrix is {mat.shape}')


  # clip the rounding errors
  mat=clip(mat)

  print(f"rank of this matrix is {getRankOfMatrix(mat)}")

  nv = get_null_vector(mat)
  # print("printing null vector....\n")
  # for i in range(nv.shape[0]): print(nv[i])
  
