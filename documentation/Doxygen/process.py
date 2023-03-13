import os, fileinput
'''
this script is for modifying doxygen code from latex source. 
pass in a "dirty" .dox file and we can clean it up here
'''

FILENAME = "test.txt"

replace_dict = {r"\begin{code}" : r"<code> <b>",
                r"\end{code}":r"</b> </code>"}

assert "installation.dox" in os.listdir()

def removeText_ttt(line):
  # while loop should catch each instance
  while r"\texttt{" in line:
    # return line
    startIndex = line.index(r"\texttt{")
    endIndex = startIndex + len(r"\texttt{")
    closingBraceIndex = startIndex + line[startIndex:].index(r"}")
    preservedString = line[endIndex:closingBraceIndex]

    #remove \texttt and closing brace
    # line = line[:closingBraceIndex] + line[closingBraceIndex + 1:]
    line = line[:startIndex] + line[closingBraceIndex + 1:]

    #insert preserved string, enclosed within <code> </code>
    line = line[:startIndex] + f"<code> <b>{preservedString}</b> </code>" + line[startIndex:]

  return line

def modifyLine(line):
  #handle \textttt separately
  line = removeText_ttt(line)

  for key in replace_dict.keys():
    val = replace_dict[key]
    if key in line:
      startIndex = line.index(key)
      finishIndex = startIndex + len(key)
      #delete forbidden characters
      line = line[:startIndex] + line[finishIndex:]
      #add the new ones in 
      line = line[:startIndex] + f"{val} " + line[startIndex+1:]
      if line[-1] != "\n":
        line = line + "\n"
  return line

def modifyFile(fName):
  for line in fileinput.input(fName, inplace=True):
    #modify line if needs be
    line = modifyLine(line)
    #write to file
    print(line, end='')
    

  #next, run through again, but append "<br>" to everything marked as code
  addLineBreak = False
  for line in fileinput.input(fName, inplace=True):
    if r"<code> <b>" in line and not addLineBreak:
      #we aren't in a code block, and we've encountered the beginning of one!
      addLineBreak = True
    if r"</b> </code>" in line:
      addLineBreak = False
    
    if addLineBreak:
      print(line[:-1] + "<br>\n", end='')
    else:
      print(line, end='')

def reset(fName):
  with open(fName, 'w') as f:
    line = "this is the first line\nthis is the second line\n\nprevious was a blank line\n"
    f.write(line)


if __name__ == "__main__":
  import sys
  if len(sys.argv) > 2:
    FILENAME = sys.argv[-1]
    print("filename is now ", FILENAME)
    # exit(0)

  if 'reset' in sys.argv:
    if FILENAME != "test.txt": exit()
    reset(FILENAME)

  if 'modify' in sys.argv:
    modifyFile(FILENAME)

  print(replace_dict)

  with open("test.txt") as f:
  # with open("installation.dox") as f:
    line = f.readline()
    while line:
      # if replace_dict[1] in line: print(line)
      if r"\texttt{" in line: print(line)
      line = f.readline()
