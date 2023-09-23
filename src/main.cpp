// seyrek, ritka
// -p precision set by .tostring(precision)

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interpolation.h"
#include "stdafx.h"

using namespace alglib;

void linspace(real_1d_array *res, double start, double end, size_t steps) {
    double delta = (end - start) / double(steps - 1);
    res->setlength(steps);

    for (int i = 0; i < steps; i += 1) {
        (*res)[i] = double(i) * delta + start;
    }
}

// Concatenates two real 2D arrays column wise.
// ALGLIB expects vector input in the format:
// [xyz.x, xyz.y, xyz.z, f(xyz).x, f(xyz).y, f(xyz).z]
void hstack(real_2d_array x, real_2d_array y, real_2d_array *res) {
    assert(x.rows() == y.rows());

    ae_int_t n_rows = x.rows();
    ae_int_t x_cols = x.cols();
    ae_int_t y_cols = y.cols();

    res->setlength(n_rows, x_cols + y_cols);

    for (ae_int_t i = 0; i < n_rows; i += 1) {
        for (ae_int_t j = 0; j < x_cols; j += 1) {
            (*res)[i][j] = x[i][j];
        }
        for (ae_int_t j = x_cols; j < x_cols + y_cols; j += 1) {
            (*res)[i][j] = y[i][j - x_cols];
        }
    }
}

int main(int argc, char **argv) {
    real_2d_array source =
        "[[1, 2, 3],"
         "[1, 2, 3],"
         "[1, 2, 3],"
         "[1, 2, 3]]";

    real_2d_array target =
        "[[4, 5, 6],"
         "[4, 5, 6],"
         "[4, 5, 6],"
         "[4, 5, 6]]";

    real_2d_array concat;
    hstack(source, target, &concat);
    std::cout << concat.tostring(3) << std::endl;

    rbfmodel model;
    rbfcreate(3, 3, model);

    rbfsetpoints(model, concat);

    rbfreport rep;
    rbfsetalgohierarchical(model, 1.0, 3, 0.0);
    rbfbuildmodel(model, rep);

    real_1d_array point = "[1, 0.5, 2.0]";
    real_1d_array res;
    rbfcalc(model, point, res);
    std::cout << res.tostring(2) << std::endl;

    // real_1d_array grid;
    // linspace(&grid, 0.0, 1.0, 10);
    // real_1d_array res2;
    // rbfgridcalc2v(model, grid, 10, grid, 10, res2);

    // std::cout << res2.tostring(3) << std::endl; // EXPECTED: [0.000,-1.000]

    // real_1d_array thing;
    // linspace(&thing, 0.0, 1.0, 5);
    // std::cout << thing.tostring(2) << std::endl;

    return 0;
}
