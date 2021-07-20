import numpy as np
import matplotlib.pyplot as plt
from scipy import io
import os
import time
import glob

pi=np.pi
def file_len(fname):
    with open(fname) as f:
        for i, l in enumerate(f):
            pass
    return i + 1

#########################################################################################################################
#main code

file_name="zz.csv"
	
f=open(file_name)
print("datafile: "+file_name)

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
print("timesteps: "+str(tstep))

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

for t_id in range(tstep):
	f=open("TracerData-"+str(tstep-1-t_id)+".vtp","w")
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
	
f=open("TracerData.pvd","w")
f.write("<VTKFile type=\"Collection\">\n")
f.write("<Collection>\n")
for t_id in range(tstep):
	f.write("<DataSet timestep=\""+str(data_set[0][tstep-1-t_id][0])+"\" file=\"TracerData-"+str(t_id)+".vtp\" />\n")
f.write("</Collection>\n")
f.write("</VTKFile>")
f.close()














"""
	
##############################################################################################			
#ready for mode decomposition
#remember to add sin(theta) for GL scheme
l_mode=2; m_mode=2;
for t in range(tstep):
	for n in range(N_tracer):
		x=data_set[n][t][1]; y=data_set[n][t][2]; z=data_set[n][t][3];
		p4re=data_set[n][t][4]; p4im=data_set[n][t][5]
		#we calculate the triangle function directly to reduce numerical error
		sintheta=(x**2+y**2)**0.5/(x**2+y**2+z**2)**0.5; costheta=z/(x**2+y**2+z**2)**0.5;
		if x**2+y**2==0: x+=1e-6;
		sinphi=y/(x**2+y**2)**0.5; cosphi=x/(x**2+y**2)**0.5
		cos2phi=2*cosphi**2-1; sin2phi=2*sinphi*cosphi
		#mode 22
		if l_mode==2 and m_mode==2:
			data_set[n][t][4]=(p4re*cos2phi-p4im*sin2phi)*(1+costheta)**2*(5/64/np.pi)**0.5
			data_set[n][t][5]=(p4re*sin2phi-p4im*cos2phi)*(1+costheta)**2*(5/64/np.pi)**0.5
			#data_set[n][t][4]=p4re*4*sintheta
		if scheme=="Gauss_Legendre":
			data_set[n][t][4]*=sintheta;
			data_set[n][t][5]*=sintheta;

##############################################################################################
#start real surface integral here
ModeRe=np.zeros(tstep)
ModeIm=np.zeros(tstep)
if scheme=="Gauss_Legendre":
	for t in range(tstep):
		for n in range(N_tracer):
			w=(1.0/40)*(2*np.pi)*data_set[n][t][6]*(np.pi/2.0)
			x=data_set[n][t][1]; y=data_set[n][t][2]; z=data_set[n][t][3];
			ModeRe[t]+=w*data_set[n][t][4]#notice we do not need to do this when we add things above
			#if t==0 :
				#print(data_set[n][t][4])
			ModeIm[t]+=w*data_set[n][t][5]			
elif scheme=="t-design":
	for t in range(tstep):
		for n in range(N_tracer):
			w=(1.0/N_tracer)*(4*np.pi)
			ModeRe[t]+=w*data_set[n][t][4] #notice we do not need to do this when we add things above
			#if t==0 :
				#print(data_set[n][t][4])
			ModeIm[t]+=w*data_set[n][t][5]

#plt.plot(data_set[0,:,0],ModeRe[:]/4/np.pi)
#plt.show()
print(ModeRe/4/np.pi)
print(ModeIm/4/np.pi)

"""

















