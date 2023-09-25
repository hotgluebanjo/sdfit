// https://github.com/chensun11/dtfv/blob/master/src/Makefile
#include <assert.h>
#include <fstream>
#include <iomanip>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "../alglib/interpolation.h"
#include "../alglib/stdafx.h"

namespace ae = alglib;

#define exit_err(msg) { fprintf(stderr, msg); exit(1); }

ae::real_1d_array linspace(double start, double end, size_t steps) {
    ae::real_1d_array res;
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
ae::real_2d_array hstack(ae::real_2d_array x, ae::real_2d_array y) {
    assert(x.rows() == y.rows());

    ae::ae_int_t n_rows = x.rows();
    ae::ae_int_t x_cols = x.cols();
    ae::ae_int_t y_cols = y.cols();

    ae::real_2d_array res;
    res.setlength(n_rows, x_cols + y_cols);

    for (ae::ae_int_t i = 0; i < n_rows; i += 1) {
        for (ae::ae_int_t j = 0; j < x_cols; j += 1) {
            res[i][j] = x[i][j];
        }
        for (ae::ae_int_t j = x_cols; j < x_cols + y_cols; j += 1) {
            res[i][j] = y[i][j - x_cols];
        }
    }

    return res;
}

// TODO: Specify LUT format.
struct Config {
    // Paths to dataset files.
    std::string source_path;
    std::string target_path;

    // Output LUT.
    std::string output;

    // Dataset XSV delimiter.
    char delimiter;

    // LUT print precision.
    // Lower precision results in smaller files.
    int precision;

    // 3D LUT cube precision (N^3).
    size_t cube_size;

    // RBase gaussian size.
    double basis_size;

    // Number of hierarchical model layers.
    ae::ae_int_t layers;

    // Optional smoothing.
    double smoothing;
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
    printf("  -o   Output path and name   default: 'output.cube'\n");
    printf("  -d   Dataset delimiter      default: ' ' (space)\n");
    printf("  -p   LUT print precision    default: 8\n");
    printf("  -c   LUT cube size          default: 33\n");
    printf("  -s   RBF basis size         default: 1.0\n");
    printf("  -l   RBF layers             default: 5\n");
    printf("  -z   RBF smoothing          default: 0.0\n");
    exit(1);
}

ae::real_2d_array load_points(Config *opts) {
    ae::real_2d_array source, target;

    // Stuck with this.
    try {
        ae::read_csv(opts->source_path.c_str(), opts->delimiter, 0, source);
        ae::read_csv(opts->target_path.c_str(), opts->delimiter, 0, target);
    } catch (...) {
        exit_err("Could not read XSV. Check that the file exists and has real Nx3 contents.\n");
    }

    // --- Sanity checks. TODO: Reader? --- //

    if (source.rows() == 0) exit_err("No readable XSV content in source.\n");
    if (target.rows() == 0) exit_err("No readable XSV content in target.\n");

    if (source.rows() != target.rows())
        exit_err("Source and target do not have the same number of XSV rows.\n");

    if (target.cols() != 3)
        exit_err("Source contains non-triplet rows. This may be because the delimiter is wrong.\n");
    if (source.cols() != 3)
        exit_err("Target contains non-triplet rows. This may be because the delimiter is wrong.\n");

    return hstack(source, target);
}

// TODO: Return LUT.
ae::real_1d_array build_lut(ae::real_2d_array points, Config *opts) {
    assert(points.cols() == 6); // 2 * 3D

    ae::rbfmodel model;
    ae::rbfcreate(3, 3, model);

    // TODO: DDM solver?
    ae::rbfsetpoints(model, points);
    ae::rbfsetalgohierarchical(model, opts->basis_size, opts->layers, opts->smoothing);

    // TODO: Report errors? Print solve error. Also speed.
    ae::rbfreport rep;
    ae::rbfbuildmodel(model, rep);

    ae::real_1d_array grid = linspace(0.0, 1.0, opts->cube_size);

    ae::real_1d_array res;
    rbfgridcalc3v(model, grid, opts->cube_size, grid, opts->cube_size, grid, opts->cube_size, res);

    return res;
}

// Very primitive option parsing. Uses atoi/f, so don't mess up the input.
void parse_options(const char **argv, int argc, Config *opts) {
    for (int i = 3; i < argc; i += 1) {
        if (argv[i][0] == '-') {
            bool next_exists = i + 1 < argc;
            switch (argv[i][1]) {
            case 'h':
                print_help();
            case 'o':
                if (!next_exists) {
                    exit_err("Missing value for output name.\n");
                }
                opts->output = argv[i + 1];
                break;
            case 'd': {
                if (!next_exists) {
                    exit_err("Missing value for delimiter. It may need to be in quotes.\n");
                }
                char input = argv[i + 1][0]; // Assume first char.
                switch (input) {
                    case ' ':
                    case ',':
                    case ';':
                    case '\t': // ?
                        opts->delimiter = input;
                        break;
                    default:
                        exit_err("Unsupported delimiter. Use one of [' ' | ',' | ';' | '\t']\n");
                }
                break;
            }
            case 'p':
                if (!next_exists) {
                    exit_err("Missing value for precision.\n");
                }
                opts->precision = atoi(argv[i + 1]);
                break;
            case 'c':
                if (!next_exists) {
                    exit_err("Missing value for cube size.\n");
                }
                opts->cube_size = atoi(argv[i + 1]);
                break;
            case 's':
                if (!next_exists) {
                    exit_err("Missing value for basis size.\n");
                }
                opts->basis_size = atof(argv[i + 1]);
                break;
            case 'l':
                if (!next_exists) {
                    exit_err("Missing value for N-layers.\n");
                }
                opts->layers = atoi(argv[i + 1]);
                break;
            case 'z':
                if (!next_exists) {
                    exit_err("Missing value for smoothing.\n");
                }
                opts->smoothing = atof(argv[i + 1]);
            }
        }
    }
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
        .delimiter = ' ',
        .precision = 8,
        .cube_size = 33,
        .basis_size = 1.0,
        .layers = 5,
        .smoothing = 0.0,
    };

    parse_options(argv, argc, &opts);

    ae::real_2d_array concat_points = load_points(&opts);
    ae::real_1d_array res = build_lut(concat_points, &opts);

    std::ofstream lut_file;
    lut_file.open(opts.output);

    if (lut_file.fail()) {
        exit_err("Could not open LUT file.\n");
    }

    lut_file << "LUT_3D_SIZE " << opts.cube_size << "\n\n";

    for (ae::ae_int_t i = 0; i < opts.cube_size * opts.cube_size * opts.cube_size; i += 1) {
        lut_file
            << std::fixed
            << std::setprecision(opts.precision)
            << res[3 * i]
            << ' '
            << res[3 * i + 1]
            << ' '
            << res[3 * i + 2]
            << '\n';
    }

    lut_file.close();

    printf("Created LUT '%s'.\n", opts.output.c_str());

    return 0;
}
