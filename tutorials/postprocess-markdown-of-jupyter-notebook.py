#
# Extremely primitive Python script which takes the markdown generated from a 
# Jupyter notebook and adds all the stuff such that it is automatically 
# embedded into the generated webpage of Peano.
#
# See page


#
# jupyter nbconvert --to markdown swe-aderdg.ipynb
#
#
#

#
import sys


ouput_file_name = sys.argv[1] + ".dox"
output_file = open( ouput_file_name, "w" )

output_file.write( "/** \n ")
with open(sys.argv[1]) as file:
    for line in file:
        print(line.rstrip())
        
        
output_file.write( "*/ \n ")

        