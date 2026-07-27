import serial
import string
import numpy as np
import open3d as o3d

x = "0"

def string_to_int(t):
    count = 0
    ta = 0
    neg = 1
    for i in t:
        if t[0] == '-':
            neg = -1
            count += 1
            continue
        ta += (ord(t[count])-48)*10**(len(t)-1-count)
        count +=1
    ta *= neg
    return ta

pcd = o3d.io.read_point_cloud(r'ToF_XYZ_Data.xyz',format = 'xyz')
print(pcd)
o3d.visualization.draw_geometries([pcd])


number_of_planes = int(string_to_int(x)/500)+1 ##decides which plane the point is on
number_of_points = int(string_to_int(x)/500)+1*256 ##put 256 points on that plane

# Line set creation --------------------------

lines = []


for u in range(0,number_of_planes*number_of_points,256):
        for i in range(256):
            if i==255:
                lines.append([u+i,u])
            else:
                lines.append([u+i,u+i+1])

for u in range(0,255):
        for i in range(1,number_of_planes):
            lines.append([u+(i-1)*256,u+i*256])

colors = [[0,0,1]for i in range(len(lines))]

line_set = o3d.geometry.LineSet(
    points=o3d.utility.Vector3dVector(np.asarray(pcd.points)),
    lines=o3d.utility.Vector2iVector(lines),
)
line_set.colors = o3d.utility.Vector3dVector(colors)
o3d.visualization.draw_geometries([line_set])