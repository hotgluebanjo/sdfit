"""
Uses the gridcalc3v function instead of nested Python loops.
"""
from argparse import ArgumentParser
import numpy as np
import xalglib

def rbf_interp(source, target, cube_size):
    """
    Takes a 2D array of RGB triplet points as both args.
    Returns an array of interpolated grid values.
    """
    data = np.hstack((source, target))

    model = xalglib.rbfcreate(3, 3)
    xalglib.rbfsetpoints(model, data.tolist())

    xalglib.rbfsetalgohierarchical(model, 1.0, 3, 0.0)
    xalglib.rbfbuildmodel(model)

    grid = np.linspace(0, 1, cube_size).tolist()

    values = xalglib.rbfgridcalc3v(model, grid, cube_size, grid, cube_size, grid, cube_size)
    res = np.reshape(values, (cube_size**3, 3))
    print(res)

source = np.loadtxt("test_data/p4k.txt", delimiter=" ")
target = np.loadtxt("test_data/alexa.txt", delimiter=" ")

rbf_interp(source, target, 3)
