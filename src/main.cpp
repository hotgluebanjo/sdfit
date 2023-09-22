#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interpolation.h"
#include "stdafx.h"

using namespace alglib;

void linspace(real_1d_array *res, double start, double end, size_t steps) {
    double delta = (end - start) / double(steps - 1);
    double *buf;
    // Am I supposed to use `new` here?
    // `buf = new double[steps];`
    // buf = (double *)calloc(steps, sizeof(*buf));
    buf = new double[steps];

    for (int i = 0; i < steps; i += 1) {
        buf[i] = double(i) * delta + start;
    }

    res->setcontent(steps, buf);
    // free(buf);
    delete buf;
}


// ALGLIB expects a 2D array in the format:
// [xyz.x, xyz.y, xyz.z, f(xyz).x, f(xyz).y, f(xyz).z]
// or
// [source.x, source.y, source.z, target.x, target.y, target.z]
void concatenate_xy(real_1d_array x, real_1d_array y) {
}

int main(int argc, char **argv) {
    rbfmodel model;
    rbfcreate(2, 2, model);

    real_2d_array cat =
        "[[+1, +1, 0, -1],"
         "[+1, -1, -1, 0],"
         "[-1, -1, 0, +1],"
         "[-1, +1, +1, 0]]";

    rbfsetpoints(model, cat);

    rbfreport rep;
    rbfsetalgohierarchical(model, 1.0, 3, 0.0);
    rbfbuildmodel(model, rep);

    real_1d_array x = "[+1,+1]";
    real_1d_array res;
    rbfcalc(model, x, res);
    // rbfgridcalc2v(model,);

    std::cout << res.tostring(2) << std::endl; // EXPECTED: [0.000,-1.000]

    real_1d_array thing;
    linspace(&thing, 0.0, 1.0, 5);
    std::cout << thing.tostring(2) << std::endl;

    return 0;
}
