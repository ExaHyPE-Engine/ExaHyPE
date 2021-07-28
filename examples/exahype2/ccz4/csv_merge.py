import numpy as np
import matplotlib.pyplot as plt
from scipy import io
import os
import time
import argparse

parser = argparse.ArgumentParser(description='csv_merger')
parser.add_argument("-iname",   "--input-name",  dest="input_name", required="True", type=str, help="input csv file prefix" )
parser.add_argument("-path",   "--input-path",  dest="path", default=".", type=str, help="input csv file directory" )
parser.add_argument("-of",   "--output-file",  dest="output_name", default=" ", type=str, help="output file name" )
args = parser.parse_args()

#########################################################################################################################
#main code

path=args.path
input_name=args.input_name
lis_file=[]
lis=os.listdir(path)
for item in lis:
	if item.startswith(input_name): lis_file.append(item)

#print(lis_file)
particle_collection={}
ID1=-999; ID2=-999
for filename in lis_file:
	f=open(path+"/"+filename)
	print("Read in datafile: "+path+"/"+filename)
	dat=f.readlines()[1:]
	for line in dat:
		tem=list(map(float,line.split(', ')))
		if (not (tem[0],tem[1]) in particle_collection) and (not all(data==0 for data in tem[6:])):
			particle_collection[(tem[0],tem[1])]=[[]]
			particle_collection[(tem[0],tem[1])][0]=tem[2:]
		elif (not tem[2] in np.array(particle_collection[(tem[0],tem[1])])[:,0] ) and (not all(data==0 for data in tem[6:])):
			particle_collection[(tem[0],tem[1])].append(tem[2:])
		
	f.close()
	
for items in particle_collection.values():
	items.sort(reverse=True)	
	
	
print("number of valid tracers: "+str(len(particle_collection.keys())) )	
	
if args.output_name==" ":
	output_name=path+"/merged_"+input_name+".csv"
else:
	output_name=path+"/"+args.output_name+".csv"
	
f=open(output_name,"w")
f.write("t, number(0), number(1), x(0), x(1), x(2), data \n")

for number, data in particle_collection.items():
	for snapshot in data:
		f.write( str(snapshot[0])+", "+str(int(number[0]))+", "+str(int(number[1])) )
		for variable in snapshot[1:]:
			f.write(", "+str(variable))
		f.write("\n")

f.close()








