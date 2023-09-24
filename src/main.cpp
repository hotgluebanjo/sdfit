// seyrek, ritka
// -p precision set by .tostring(precision)

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "../alglib/interpolation.h"
#include "../alglib/stdafx.h"

using namespace alglib;

#define exit_err(msg) { fprintf(stderr, msg); exit(1); }
#define exit_errf(str, ag) { fprintf(stderr, msg, ag); exit(1); }

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
real_2d_array hstack(real_2d_array x, real_2d_array y) {
    assert(x.rows() == y.rows());

    ae_int_t n_rows = x.rows();
    ae_int_t x_cols = x.cols();
    ae_int_t y_cols = y.cols();

    real_2d_array res;
    res.setlength(n_rows, x_cols + y_cols);

    for (ae_int_t i = 0; i < n_rows; i += 1) {
        for (ae_int_t j = 0; j < x_cols; j += 1) {
            res[i][j] = x[i][j];
        }
        for (ae_int_t j = x_cols; j < x_cols + y_cols; j += 1) {
            res[i][j] = y[i][j - x_cols];
        }
    }

    return res;
}

struct Config {
    std::string source_path;
    std::string target_path;
    std::string output;
    int cube_size;
    double basis_size;
    int layers;
    double smoothing;
    char delimiter;
    int precision;
};

void print_help() {
    printf("SD Interp v0.1.0\n");
    printf("Scattered data interpolation for tristimulus lookup tables.\n\n");
    printf("USAGE: sdinterp <source.txt> <target.txt> [OPTIONS]\n");
    printf("Example: sdinterp alexa.csv print-film.csv -d ',' -o alexa_to_print_film.cube\n\n");
    printf("OPTIONS:\n");
    printf("  -h   Help\n");
    printf("  -o   Output path and name (default: 'output.cube')\n");
    printf("  -d   delimiter            (default: ' ')\n");
    printf("  -c   LUT cube size        (default: 33)\n");
    printf("  -s   RBF basis size       (default: 1.0)\n");
    printf("  -l   RBF layers           (default: 3)\n");
    printf("  -z   RBF smoothing        (default: 0.0)\n");
    exit(1);
}

int main(int argc, const char **argv) {
    if (argc == 1 || !strcmp(argv[1], "-h")) {
        print_help();
    }

    if (argc == 2) {
        exit_err("cli: missing target data set");
    }

    Config opts = {
        .source_path = argv[1],
        .target_path = argv[2],
        .output = "output.cube",
        .cube_size = 33,
        .basis_size = 1.0,
        .layers = 3,
        .smoothing = 0.0,
        .delimiter = ' ',
        .precision = 8,
    };

    real_2d_array source, target;

    // Stuck with exceptions.
    try {
        read_csv(opts.source_path.c_str(), opts.delimiter, 0, source);
        read_csv(opts.target_path.c_str(), opts.delimiter, 0, target);
    } catch (...) {
        exit_err("Could not read XSV. Check that the file exists and has real Nx3 contents.");
    }

    // A million sanity checks. TODO: Reader?
    if (source.rows() == 0) {
        exit_err("No readable XSV content in source.");
    }

    if (target.rows() == 0) {
        exit_err("No readable XSV content in target.");
    }

    if (source.rows() != target.rows()) {
        exit_err("Source and target do not have the same number of XSV rows.");
    }

    if (target.cols() != 3) {
        exit_err("Source contains non-triplet rows.");
    }

    if (source.cols() != 3) {
        exit_err("Target contains non-triplet rows.");
    }

    real_2d_array concat = hstack(source, target);
    // std::cout << concat.tostring(3) << std::endl;

    rbfmodel model;
    rbfcreate(3, 3, model);

    rbfsetpoints(model, concat);

    rbfreport rep;
    // TODO: DDM solver?
    rbfsetalgohierarchical(model, opts.basis_size, opts.layers, opts.smoothing);
    rbfbuildmodel(model, rep);

    real_1d_array grid;
    linspace(&grid, 0.0, 1.0, opts.cube_size);

    real_1d_array res;
    rbfgridcalc3v(model, grid, opts.cube_size, grid, opts.cube_size, grid, opts.cube_size, res);

    FILE *lut_file = fopen(opts.output.c_str(), "w");

    if (lut_file == NULL) {
        fprintf(stderr, "Could not open LUT file.\n");
        exit(1);
    }

    fprintf(lut_file, "LUT_3D_SIZE %d\n\n", opts.cube_size);

    for (ae_int_t i = 0; i < opts.cube_size * opts.cube_size * opts.cube_size; i += 1) {
        fprintf(lut_file, "%f %f %f\n", res[3 * i], res[3 * i + 1], res[3 * i + 2]);
    }

    fclose(lut_file);

    printf("Created LUT '%s'.\n", opts.output.c_str());

    return 0;
}
