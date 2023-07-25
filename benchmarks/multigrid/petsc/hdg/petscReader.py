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


if __name__ == "__main__":

  #hardcoding file names!

  print(getMatDimensions("mat.xml"))
  
  dim = getMatDimensions("mat.xml")
  with open("mat.xml") as f:
    for _ in range(5): l = f.readline()

  mat = populateMatrix("mat.xml")