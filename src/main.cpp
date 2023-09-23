// seyrek, ritka
#include <cassert>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interpolation.h"
#include "stdafx.h"

using namespace alglib;

// -p precision set by .tostring(precision)

// TODO: Consistency.
void linspace(real_1d_array *res, double start, double end, size_t steps) {
    double delta = (end - start) / double(steps - 1);
    double *buf = new double[steps];

    for (int i = 0; i < steps; i += 1) {
        buf[i] = double(i) * delta + start;
    }

    res->setcontent(steps, buf);
    delete[] buf;
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
    std::cout << res.tostring(2) << std::endl; // EXPECTED: [0.000,-1.000]

    // real_1d_array grid;
    // linspace(&grid, 0.0, 1.0, 10);
    // real_1d_array res2;
    // rbfgridcalc2v(model, grid, 10, grid, 10, res2);

    // std::cout << res2.tostring(3) << std::endl; // EXPECTED: [0.000,-1.000]

    real_1d_array thing;
    linspace(&thing, 0.0, 1.0, 5);
    std::cout << thing.tostring(2) << std::endl;

    real_2d_array bla_x =
        "[[1, 2, 3],"
         "[1, 2, 3],"
         "[1, 2, 3],"
         "[1, 2, 3]]";

    real_2d_array bla_y =
        "[[4, 5, 6],"
         "[4, 5, 6],"
         "[4, 5, 6],"
         "[4, 5, 6]]";

    real_2d_array resbla;
    hstack(bla_x, bla_y, &resbla);
    std::cout << resbla.tostring(3) << std::endl;

    return 0;
}
