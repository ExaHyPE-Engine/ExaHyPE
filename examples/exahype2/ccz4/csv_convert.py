import numpy as np
import matplotlib.pyplot as plt
from scipy import io
import os
import time
import argparse

pi=np.pi
def file_len(fname):
    with open(fname) as f:
        for i, l in enumerate(f):
            pass
    return i + 1

lis=os.listdir("./")
for item in lis:
	if item.endswith(".pvd") or item.endswith(".vtp"): os.remove(os.path.join("./",item))

parser = argparse.ArgumentParser(description='Tracer_conversion')
parser.add_argument("-if",   "--input-file",  dest="file_name", required="True", type=str, help="input csv file name" )
parser.add_argument("-dt",   "--delta-time",  dest="dt", default=-1, type=float, help="time interval between two snapshot, default is to print every timestep" )
parser.add_argument("-of",   "--output-file",  dest="output_name", default="TracerData", type=str, help="output file name, extension and number will be add automatically" )
args = parser.parse_args()


dt=args.dt;
file_name=args.file_name
output_name=args.output_name


#########################################################################################################################
#main code	
f=open(file_name)
print("Read in datafile: "+file_name)

#skip the description line
dat=f.readlines()[1:]

#first find out the number of timesteps
tstep=0
told=1e6
for line in dat:
	tem=list(map(float,line.split(', ')))
	tnew=tem[0]
	if (told-tnew)<0:
		break
	else:
		tstep+=1
		told=tnew

number_of_data=len(tem)-6

print("number of data entries: "+str(number_of_data))
print("total timesteps: "+str(tstep))

#second, find out how many tracer recorded here.
N_tracer=0
ID1=-100; ID2=-100
coors=[]
for line in dat:
	tem=list(map(float,line.split(', ')))
	if (not tem[1]==ID1) or (not tem[2]==ID2) and (not tem[6]==0):
		ID1=tem[1]; ID2=tem[2]
		N_tracer+=1
		coor=[tem[3],tem[4],tem[5]]
		coors.append(coor)
		
coors=np.array(coors)
print("valid tracers: "+str(len(coors))) 

#create the real data set
data_set=np.zeros((N_tracer,tstep,4+number_of_data))

ID1=list(map(float,dat[0].split(', ')))[1]; ID2=list(map(float,dat[0].split(', ')))[2]
t_count=0 #counting the tsteps
N_count=0 #counting the tracer
for line in dat:
	tem=list(map(float,line.split(', ')))
	if (tem[1]==ID1) and (tem[2]==ID2):
		data_set[N_count][t_count][0]=tem[0]; #timestep
		data_set[N_count][t_count][1]=tem[3]; data_set[N_count][t_count][2]=tem[4]; data_set[N_count][t_count][3]=tem[5] #coordinates
		for i in range(number_of_data):
			data_set[N_count][t_count][4+i]=tem[6+i];
		t_count+=1
	else:
		ID1=tem[1]; ID2=tem[2]
		N_count+=1
		t_count=0
		data_set[N_count][t_count][0]=tem[0]; #timestep
		data_set[N_count][t_count][1]=tem[3]; data_set[N_count][t_count][2]=tem[4]; data_set[N_count][t_count][3]=tem[5] #coordinates
		for i in range(number_of_data):
			data_set[N_count][t_count][4+i]=tem[6+i];
		t_count+=1

f.close()
#print(data_set[0,:,:])

print("----------------------------------------------------------------")

snapshot_count=0
snapshot_ids=[]
t_old=-1e10; t_new=0
for t_id in range(tstep-1,-1,-1):
	t_new=data_set[0][t_id][0]
	if (t_new-t_old)<dt:
		continue
	t_old=t_new
	print("writing vtp file "+output_name+"-"+str(snapshot_count)+".vtp"+", t= "+str(t_new))
	f=open(output_name+"-"+str(snapshot_count)+".vtp","w")
	f.write("<!-- Generated from CSV file of Tracer, Peano4 ExaHyPE 2-->\n\n")
	f.write("<VTKFile type=\"PolyData\">\n")
	f.write("<PolyData>\n")
	f.write("<Piece NumberOfPoints=\""+str(N_tracer)+"\">\n")
	f.write("<Points><DataArray type=\"Float64\" NumberOfComponents=\"3\" format=\"ascii\">\n")		
	for tra_index in range(N_tracer):
		f.write(str(data_set[tra_index][t_id][1])+" "+str(data_set[tra_index][t_id][2])+" "+str(data_set[tra_index][t_id][3])+"\n")
	f.write("</DataArray></Points>\n")
	
	f.write("<PointData>\n")
	f.write("<DataArray type=\"Float64\" Name=\"data\" format=\"ascii\" NumberOfComponents=\""+str(number_of_data)+"\" >")
	for tra_index in range(N_tracer):
		for data_index in range(4,4+number_of_data):
			f.write(str(data_set[tra_index][t_id][data_index])+" ")
		f.write("\n")
	f.write("</DataArray>\n")
	f.write("</PointData>\n")
	
	f.write("</Piece>\n</PolyData>\n</VTKFile>")
	f.close()
	snapshot_ids.append(t_id)
	snapshot_count+=1
	
		
f=open(output_name+".pvd","w")
f.write("<VTKFile type=\"Collection\">\n")
f.write("<Collection>\n")
count=0
for t_id in snapshot_ids:
	f.write("<DataSet timestep=\""+str(data_set[0][t_id][0])+"\" file=\""+output_name+"-"+str(count)+".vtp\" />\n")
	count+=1
f.write("</Collection>\n")
f.write("</VTKFile>")
f.close()
print("pvd file "+output_name+".pvd created. Total snapshot: "+str(snapshot_count))













