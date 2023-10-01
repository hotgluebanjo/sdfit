# Note: This is for an outdated version of ALGLIB: v3.17.0
import numpy as np
import xalglib

source = np.loadtxt("test_data/p4k.txt", delimiter=" ")
target = np.loadtxt("test_data/alexa.txt", delimiter=" ")

cube_size = 33

assert len(source) == len(target)

data = np.hstack((source, target))

trn = xalglib.mlpcreatetrainer(3, 3)

# 5 hidden layers. 3x5x3 (3 in, 3 out)
network = xalglib.mlpcreate1(3, 5, 3)

# must give n_pts
xalglib.mlpsetdataset(trn, data.tolist(), len(data))

# 5 random restarts
rep = xalglib.mlptrainnetwork(trn, network, 5)

x = [0.23200172185897827, 0.17972472310066223, 0.15904557704925537]
# Empty array because it can't mutate python lists or something.
y = xalglib.mlpprocess(network, x, [])
print(y)

grid = np.linspace(0, 1, cube_size)
f = open("_test.cube", "w")

f.write(f"LUT_3D_SIZE {cube_size}\n")

for b in grid:
    for g in grid:
        for r in grid:
            p = [r, g, b]
            v = xalglib.mlpprocess(network, p, [])
            f.write(f"{v[0]} {v[1]} {v[2]}\n")
f.close()
