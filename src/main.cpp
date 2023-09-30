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
#define sqr_i(v) (v * v)
#define cube_i(v) (v * v * v)

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

enum Lut_Format {
    RESOLVE_CUBE,
    SONY_SPI3D,
};

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

    // Supported:
    // - Resolve Cube
    // - Sony SPI3D
    Lut_Format format;

    // RBase gaussian size.
    double basis_size;

    // Number of hierarchical model layers.
    ae::ae_int_t layers;

    // Optional smoothing.
    double smoothing;
};

void print_help() {
    printf("sundaytrained v0.1.0\n");
    printf("Scattered data fitting for tristimulus lookup tables.\n");
    printf("https://github.com/hotgluebanjo\n\n");
    printf("USAGE: suntr <source> <target> [OPTIONS]\n\n");
    printf("EXAMPLE: suntr alexa.csv print-film.csv -d ',' -o alexa_to_print_film.cube\n\n");
    printf("INPUTS:\n");
    printf("  <source>   Plaintext file containing source dataset\n");
    printf("  <target>   Plaintext file containing target dataset\n\n");
    printf("OPTIONS:\n");
    printf("  -h   Help\n");
    printf("  -o   Output path and name                   default: 'output.cube'\n");
    printf("  -d   Dataset delimiter [' ' | ',' | <tab>]  default: ' ' (space)\n");
    printf("  -p   LUT print precision                    default: 8\n");
    printf("  -c   LUT cube size                          default: 33\n");
    printf("  -f   LUT format [cube | spi]                default: cube\n");
    printf("  -s   RBF basis size                         default: 5.0\n");
    printf("  -l   RBF layers                             default: 5\n");
    printf("  -z   RBF smoothing                          default: 0.0\n");
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
void parse_options(Config *opts, const char **argv, int argc) {
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
                        exit_err("Unsupported delimiter. Use one of [' ' | ',' | ';' | <tab>]\n");
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
            case 'f':
                if (!next_exists) {
                    exit_err("Missing value for LUT format.\n");
                }
                if (!strcmp(argv[i + 1], "cube")) {
                    opts->format = RESOLVE_CUBE;
                } else if (!strcmp(argv[i + 1], "spi")) {
                    opts->format = SONY_SPI3D;
                } else {
                    exit_err("Unsupported LUT format. Use one of [cube | spi]\n");
                }
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
                break;
            default:
                exit_err("Unkown option. Check -h for help.\n");
            }
        }
    }
}

void write_lut(ae::real_1d_array lut, Config *opts) {
    std::ofstream lut_file;

    switch (opts->format) {
    case RESOLVE_CUBE:
        if (opts->output == "") {
            opts->output = "output.cube";
        }

        lut_file.open(opts->output);

        if (lut_file.fail()) {
            exit_err("Could not open Cube LUT file.\n");
        }

        lut_file << "LUT_3D_SIZE " << opts->cube_size << "\n";

        for (ae::ae_int_t i = 0; i < cube_i(opts->cube_size); i += 1) {
            lut_file
                << std::fixed
                << std::setprecision(opts->precision)
                << lut[3 * i + 0] << ' '
                << lut[3 * i + 1] << ' '
                << lut[3 * i + 2] << '\n';
        }
        break;
    case SONY_SPI3D:
        if (opts->output == "") {
            opts->output = "output.spi3d";
        }

        lut_file.open(opts->output);

        if (lut_file.fail()) {
            exit_err("Could not open SPI3D LUT file.\n");
        }

        lut_file << "SPILUT 1.0\n3 3\n"
            << opts->cube_size << ' '
            << opts->cube_size << ' '
            << opts->cube_size << '\n';

        /*
        The indices are swizzled in other implementations, but their ordering of entries must
        be different. It doesn't matter in SPI3D as the indices are explicit. This works in OCIO.

        https://github.com/AcademySoftwareFoundation/OpenColorIO/blob/
        c429400170ccd34902d8a6b26e70c43e26d57751/src/OpenColorIO/fileformats/FileFormatSpi3D.cpp#L294
        */
        for (ae::ae_int_t i = 0; i < cube_i(opts->cube_size); i += 1) {
            ae::ae_int_t x = i % opts->cube_size;
            ae::ae_int_t y = (i / opts->cube_size) % opts->cube_size;
            ae::ae_int_t z = i / sqr_i(opts->cube_size);

            lut_file
                << std::fixed
                << std::setprecision(opts->precision)
                << x << ' '
                << y << ' '
                << z << ' '
                << lut[3 * i + 0] << ' '
                << lut[3 * i + 1] << ' '
                << lut[3 * i + 2] << '\n';
        }
    }

    lut_file.close();

    printf("Created LUT '%s'.\n", opts->output.c_str());
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
        .output = "",
        .delimiter = ' ',
        .precision = 8,
        .cube_size = 33,
        .format = RESOLVE_CUBE,
        .basis_size = 5.0,
        .layers = 5,
        .smoothing = 0.0,
    };

    parse_options(&opts, argv, argc);

    ae::real_2d_array concat_points = load_points(&opts);
    ae::real_1d_array res = build_lut(concat_points, &opts);

    write_lut(res, &opts);
}
