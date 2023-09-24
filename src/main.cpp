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

struct Config {
    const char *source_path;
    const char *target_path;
    const char *lut_path;
    int cube_size;
    double basis_size;
    int layers;
    double smoothing;
    char delimiter;
    int precision;
};

int main(int argc, char **argv) {
    Config opts = {
        .source_path = "test_data/p4k.txt",
        .target_path = "test_data/alexa.txt",
        .lut_path = "output.cube",
        .cube_size = 33,
        .basis_size = 1.0,
        .layers = 3,
        .smoothing = 0.0,
        .delimiter = ' ',
        .precision = 8,
    };

    real_2d_array source, target;

    read_csv(opts.source_path, opts.delimiter, 0, source);
    read_csv(opts.target_path, opts.delimiter, 0, target);

    real_2d_array concat;
    hstack(source, target, &concat);
    // std::cout << concat.tostring(3) << std::endl;

    rbfmodel model;
    rbfcreate(3, 3, model);

    rbfsetpoints(model, concat);

    rbfreport rep;
    rbfsetalgohierarchical(model, opts.basis_size, opts.layers, opts.smoothing);
    rbfbuildmodel(model, rep);

    real_1d_array grid;
    linspace(&grid, 0.0, 1.0, opts.cube_size);

    real_1d_array res;
    rbfgridcalc3v(model, grid, opts.cube_size, grid, opts.cube_size, grid, opts.cube_size, res);

    FILE *lut_file = fopen(opts.lut_path, "w");

    if (lut_file == NULL) {
        fprintf(stderr, "write: could not open lut file.\n");
        exit(1);
    }

    fprintf(lut_file, "LUT_3D_SIZE %d\n\n", opts.cube_size);

    for (int i = 0; i < opts.cube_size * opts.cube_size * opts.cube_size; i += 1) {
        fprintf(lut_file, "%f %f %f\n", res[3 * i], res[3 * i + 1], res[3 * i + 2]);
    }

    return 0;
}
