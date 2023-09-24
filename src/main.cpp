// seyrek, ritka
// -p precision set by .tostring(precision)

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "../alglib/interpolation.h"
#include "../alglib/stdafx.h"

using namespace alglib;

#define exit_err(msg) { fprintf(stderr, msg); exit(1); }
#define exit_errf(str, ag) { fprintf(stderr, msg, ag); exit(1); }

real_1d_array linspace(double start, double end, size_t steps) {
    real_1d_array res;
    res.setlength(steps);

    double delta = (end - start) / double(steps - 1);

    for (int i = 0; i < steps; i += 1) {
        res[i] = double(i) * delta + start;
    }

    return res;
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
    // Paths to dataset files.
    std::string source_path;
    std::string target_path;

    std::string output;

    // LUT cube size
    int cube_size;
    double basis_size;
    int layers;
    double smoothing;
    char delimiter;
    int precision;
};

void print_help() {
    printf("rbf-interp v0.1.0\n");
    printf("Scattered data interpolation for tristimulus lookup tables.\n");
    printf("https://github.com/hotgluebanjo\n\n");
    printf("USAGE: rbf-interp <source> <target> [OPTIONS]\n\n");
    printf("EXAMPLE: rbf-interp alexa.csv print-film.csv -d ',' -o alexa_to_print_film.cube\n\n");
    printf("INPUTS:\n");
    printf("  <source>   Plaintext file containing source dataset\n");
    printf("  <target>   Plaintext file containing target dataset\n\n");
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

real_2d_array load_points(Config *opts) {
    real_2d_array source, target;

    // Stuck with this.
    try {
        read_csv(opts->source_path.c_str(), opts->delimiter, 0, source);
        read_csv(opts->target_path.c_str(), opts->delimiter, 0, target);
    } catch (...) {
        exit_err("Could not read XSV. Check that the file exists and has real Nx3 contents.\n");
    }

    // --- Sanity checks. TODO: Reader? --- //

    if (source.rows() == 0) exit_err("No readable XSV content in source.\n");
    if (target.rows() == 0) exit_err("No readable XSV content in target.\n");

    if (source.rows() != target.rows())
        exit_err("Source and target do not have the same number of XSV rows.\n");

    if (target.cols() != 3) exit_err("Source contains non-triplet rows.\n");
    if (source.cols() != 3) exit_err("Target contains non-triplet rows.\n");

    return hstack(source, target);
}

// TODO: Return LUT.
real_1d_array build_lut(real_2d_array points, Config *opts) {
    assert(points.cols() == 6); // 2 * 3D

    rbfmodel model;
    rbfcreate(3, 3, model);

    // TODO: DDM solver?
    rbfsetpoints(model, points);
    rbfsetalgohierarchical(model, opts->basis_size, opts->layers, opts->smoothing);

    // TODO: Report errors? Also speed.
    rbfreport rep;
    rbfbuildmodel(model, rep);

    real_1d_array grid = linspace(0.0, 1.0, opts->cube_size);

    real_1d_array res;
    rbfgridcalc3v(model, grid, opts->cube_size, grid, opts->cube_size, grid, opts->cube_size, res);

    return res;
}

int main(int argc, const char **argv) {
    if (argc == 1 || !strcmp(argv[1], "-h")) {
        print_help();
    }

    if (argc == 2) {
        exit_err("Missing target dataset.\n");
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

    real_2d_array concat_points = load_points(&opts);
    real_1d_array res = build_lut(concat_points, &opts);

    FILE *lut_file = fopen(opts.output.c_str(), "w");

    if (lut_file == NULL) {
        exit_err("Could not open LUT file.\n");
    }

    fprintf(lut_file, "LUT_3D_SIZE %d\n\n", opts.cube_size);

    for (ae_int_t i = 0; i < opts.cube_size * opts.cube_size * opts.cube_size; i += 1) {
        fprintf(lut_file, "%f %f %f\n", res[3 * i], res[3 * i + 1], res[3 * i + 2]);
    }

    fclose(lut_file);

    printf("Created LUT '%s'.\n", opts.output.c_str());

    return 0;
}
