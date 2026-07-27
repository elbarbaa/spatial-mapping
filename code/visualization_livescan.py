import serial
import open3d as o3d
import numpy as np

x = 0

'''
variable = o3d.io.read_point_cloud("file",format = 'xyz')
o3d.visualization.draw_geometries([variable])
'''

#--------------------------------------------------------------

def string_to_int(string):
    counter = 0
    returned_value = 0
    negative = 1
    
    for i in string:
        if string[0] == '-': ##checks for negative sign
            negative = -1 ##sets negative variable accordingly then increments counter
            counter += 1
            continue

        returned_value += (ord(string[counter])-48)*10**(len(string)-1-counter)
        counter +=1
    returned_value *= negative
    return returned_value



file = open(r'ToF_XYZ_Data2.xyz','w') # creates a new file for storing the ToF data (XYZ file)


s = serial.Serial('COM5',115200,timeout = 10) ##serial communication

print("Opening: " + s.name)

# reset the buffers of the UART port to delete the remaining data in the buffers
s.reset_output_buffer()
s.reset_input_buffer()

# wait for user's signal to start the program
input("Press Enter to start communication...")
# send the character 's' to MCU via UART
# This will signal MCU to start the transmission
# recieve 10 measurements from UART of MCU

##############################################################################

x_value = 0
y_value = 0
z_value = 0

point = 0
temp = "" ##temporaary variable

read = s.read()

while read.decode() != 'Z': ##while the transission is not done
    read = s.read()
    if read.decode() != '@' and read.decode() != '!' and read.decode() != '$': ##if the decoded values are not x,y, or z
       print(read.decode(),end='')
       

    if read.decode() == '$': #if the symbol obtained from Keil is $ (used for X)
        x_value = x_value^1
        if temp != "": ##if the temporary variable is not empty...
            x = temp ##dump whatever is in the temp variable to x
            print(" -> x-value")
        temp = "" ##set temp to an empty variable

    ##the same for y
    elif read.decode() == '@':  
        y_value = y_value^1
        if temp != "":
   
            y = temp
            print(" -> y-value")
        temp = ""
            

    ##the same for z
    elif read.decode() == '!':
        z_value = z_value^1
        if temp != "":
            #z = string_to_int(temp)
            z = temp
            file.write("{}\t {}\t {}\t\n".format(x,y,z))
            print(" -> z-value")
        temp = ""

    elif y_value==1 or z_value==1 or x_value==1:
        temp += read.decode()
    
    
       
# encode() and decode() convert string to bytes (pyserial only works with bytes)

#close the port
print("Closing: " + s.name)
file.close()
s.close()

pcd = o3d.io.read_point_cloud(r'ToF_XYZ_Data2.xyz',format = 'xyz') ##read the pcd file
print(pcd)
o3d.visualization.draw_geometries([pcd])

num_points = int(string_to_int(x)/500)+1*256 #number of points = x values/500 +1 *256 measurements
num_levels = int(string_to_int(x)/500)+1 ##number of layers or levels is x values/500 which is the amount I walk forward


# Line set creation --------------------------

line_list = []


for a in range(0,num_levels*num_points,256):
        for i in range(256): ##loop through all the points
            if i==255: #if we are on the last point, append it or connect it to the first one
                line_list.append([a+i,a])
            else:
                line_list.append([a+i,a+i+1]) ##if we are on any other point, connect it to the next one

for a in range(0,255): ##same thing but with the levels
        for i in range(1,num_levels):
            line_list.append([a+(i-1)*256,a+i*256])

color_RGB = [[0,0,1]for i in range(len(line_list))] #choosing blue as the color [0,0,0] is for RGB. change whichever one needed to "1"

#plotting the shape
lines = o3d.geometry.LineSet( 
    points=o3d.utility.Vector3dVector(np.asarray(pcd.points)),
    lines=o3d.utility.Vector2iVector(line_list),
)
lines.colors = o3d.utility.Vector3dVector(color_RGB)
o3d.visualization.draw_geometries([lines])
