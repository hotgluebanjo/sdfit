#include <assert.h>
#include <fstream>
#include <iomanip>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "alglib/dataanalysis.h"
#include "alglib/interpolation.h"
#include "alglib/stdafx.h"

namespace ae = alglib;

#define exit_err(msg) { fprintf(stderr, msg); exit(1); }
#define sqr_i(v) (v * v)
#define cube_i(v) (v * v * v)

union Int3 {
    struct {
        int x;
        int y;
        int z;
    };
    int xyz[3];
};

Int3 index_3d_from_1d(int i, int size) {
    return {i % size, (i / size) % size, i / sqr_i(size)};
}

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

enum Fit_Method {
    RBF,
    MLP,
};

enum Lut_Format {
    RESOLVE_CUBE,
    SONY_SPI3D,
};

struct Config {
    // Supported:
    // - RBF interpolation
    // - MLP neural network
    // TODO: Lattice regression?
    Fit_Method mode;

    // Paths to dataset files.
    std::string source_path;
    std::string target_path;

    // Output LUT.
    std::string output;

    // Dataset DSV delimiter.
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
    double rbf_size;

    // Number of hierarchical model layers.
    ae::ae_int_t rbf_layers;

    // Optional smoothing.
    double rbf_smoothing;

    // Hidden layers.
    ae::ae_int_t mlp_layers;

    // Random restarts.
    ae::ae_int_t mlp_restarts;
};

// TODO: DDM solver, speed.
ae::real_1d_array build_lut_rbf(ae::real_2d_array points, Config *opts, ae::rbfreport *rep) {
    assert(points.cols() == 6); // 2 * 3D

    ae::rbfmodel model;
    ae::rbfcreate(3, 3, model);

    ae::rbfsetpoints(model, points);
    ae::rbfsetalgohierarchical(model, opts->rbf_size, opts->rbf_layers, opts->rbf_smoothing);
    ae::rbfbuildmodel(model, *rep);

    ae::real_1d_array grid = linspace(0.0, 1.0, opts->cube_size);

    ae::real_1d_array res;
    rbfgridcalc3v(model, grid, opts->cube_size, grid, opts->cube_size, grid, opts->cube_size, res);

    return res;
}

ae::real_1d_array build_lut_mlp(ae::real_2d_array points, Config *opts, ae::mlpreport *rep) {
    assert(points.cols() == 6); // 2 * 3D

    ae::mlptrainer trn;
    ae::mlpcreatetrainer(3, 3, trn);
    ae::mlpsetdataset(trn, points, points.rows());

    ae::multilayerperceptron network;
    ae::mlpcreate1(3, opts->mlp_layers, 3, network); // hidden layers. 3xNx3 (3 in, 3 out)

    ae::mlptrainnetwork(trn, network, opts->mlp_restarts, *rep);

    ae::real_1d_array grid = linspace(0.0, 1.0, opts->cube_size);
    ae::real_1d_array res;
    res.setlength(3 * cube_i(opts->cube_size));

    for (ae::ae_int_t i = 0; i < cube_i(opts->cube_size); i += 1) {
        Int3 i_3d = index_3d_from_1d(i, opts->cube_size);

        // TODO
        ae::real_1d_array p;
        p.setlength(3);

        p[0] = grid[i_3d.x];
        p[1] = grid[i_3d.y];
        p[2] = grid[i_3d.z];

        ae::real_1d_array v;
        ae::mlpprocess(network, p, v);

        res[3 * i + 0] = v[0];
        res[3 * i + 1] = v[1];
        res[3 * i + 2] = v[2];
    }

    return res;
}

void print_help() {
    printf("sdfit v0.2.0\n");
    printf("Scattered data fitting for tristimulus lookup tables.\n");
    printf("https://github.com/hotgluebanjo\n\n");
    printf("USAGE: sdfit <source> <target> [OPTIONS]\n\n");
    printf("EXAMPLES:\n");
    printf("  sdfit alexa.csv print-film.csv -d ',' -o alexa_to_print_film.cube\n");
    printf("  sdfit venice.txt alexa.txt -m rbf -p 6 -f spi -o venice_to_alexa.spi3d\n\n");
    printf("INPUTS:\n");
    printf("  <source>   Plaintext file containing source dataset\n");
    printf("  <target>   Plaintext file containing target dataset\n\n");
    printf("OPTIONS:\n");
    printf("  -h   Help\n");
    printf("  -m   Method to use [mlp | rbf]              default: mlp\n");
    printf("  -o   Output path and name                   default: 'output.cube'\n");
    printf("  -d   Dataset delimiter [' ' | ',' | <tab>]  default: ' ' (space)\n");
    printf("  -p   LUT print precision                    default: 8\n");
    printf("  -c   LUT cube size                          default: 33\n");
    printf("  -f   LUT format [cube | spi]                default: cube\n");
    printf("  -s   RBF basis size                         default: 5.0\n");
    printf("  -l   RBF layers                             default: 5\n");
    printf("  -z   RBF smoothing                          default: 0.0\n");
    printf("  -L   MLP layers                             default: 5\n");
    printf("  -r   MLP restarts                           default: 5\n");
    exit(1);
}

ae::real_2d_array load_points(Config *opts) {
    ae::real_2d_array source, target;

    // Stuck with this.
    try {
        ae::read_csv(opts->source_path.c_str(), opts->delimiter, 0, source);
        ae::read_csv(opts->target_path.c_str(), opts->delimiter, 0, target);
    } catch (...) {
        exit_err("Could not read DSV. Check that the file exists and has real Nx3 contents.\n");
    }

    if (source.rows() == 0) exit_err("No readable DSV content in source.\n");
    if (target.rows() == 0) exit_err("No readable DSV content in target.\n");

    if (source.rows() != target.rows())
        exit_err("Source and target do not have the same number of DSV rows.\n");

    if (target.cols() != 3)
        exit_err("Source contains non-triplet rows. This may be because the delimiter is wrong.\n");
    if (source.cols() != 3)
        exit_err("Target contains non-triplet rows. This may be because the delimiter is wrong.\n");

    return hstack(source, target);
}

// Very primitive option parsing. Uses atoi/f, so don't mess up the input.
void parse_options(Config *opts, const char **argv, int argc) {
    for (int i = 3; i < argc; i += 1) {
        if (argv[i][0] == '-') {
            bool next_exists = i + 1 < argc;
            switch (argv[i][1]) {
            case 'h':
                print_help();
            case 'm':
                if (!next_exists) {
                    exit_err("Missing value for method.\n");
                }
                if (!strcmp(argv[i + 1], "rbf")) {
                    opts->mode = RBF;
                } else if (!strcmp(argv[i + 1], "mlp")) {
                    opts->mode = MLP;
                } else {
                    exit_err("Unsupported method. Use one of [mlp | rbf]\n");
                }
                break;
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
                    exit_err("Missing value for RBF basis size.\n");
                }
                opts->rbf_size = atof(argv[i + 1]);
                break;
            case 'l':
                if (!next_exists) {
                    exit_err("Missing value for RBF N-layers.\n");
                }
                opts->rbf_layers = atoi(argv[i + 1]);
                break;
            case 'z':
                if (!next_exists) {
                    exit_err("Missing value for RBF smoothing.\n");
                }
                opts->rbf_smoothing = atof(argv[i + 1]);
                break;
            case 'L':
                if (!next_exists) {
                    exit_err("Missing value for MLP layers.\n");
                }
                opts->mlp_layers = atoi(argv[i + 1]);
                break;
            case 'r':
                if (!next_exists) {
                    exit_err("Missing value for MLP restarts.\n");
                }
                opts->mlp_restarts = atoi(argv[i + 1]);
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
            Int3 i_3d = index_3d_from_1d(i, opts->cube_size);

            lut_file
                << std::fixed
                << std::setprecision(opts->precision)
                << i_3d.x << ' '
                << i_3d.y << ' '
                << i_3d.z << ' '
                << lut[3 * i + 0] << ' '
                << lut[3 * i + 1] << ' '
                << lut[3 * i + 2] << '\n';
        }
    }

    lut_file.close();
}

int main(int argc, const char **argv) {
    if (argc == 1 || !strcmp(argv[1], "-h")) {
        print_help();
    }

    if (argc == 2) {
        exit_err("Missing target dataset.\n");
    }

    Config opts = {
        .mode = MLP,
        .source_path = argv[1],
        .target_path = argv[2],
        .output = "",
        .delimiter = ' ',
        .precision = 8,
        .cube_size = 33,
        .format = RESOLVE_CUBE,
        .rbf_size = 5.0,
        .rbf_layers = 5,
        .rbf_smoothing = 0.0,
        .mlp_layers = 5,
        .mlp_restarts = 5,
    };

    parse_options(&opts, argv, argc);

    ae::real_2d_array concat_points = load_points(&opts);
    ae::real_1d_array lut;

    switch (opts.mode) {
    case RBF: {
        ae::rbfreport report;
        lut = build_lut_rbf(concat_points, &opts, &report);
        printf("Built LUT. RMS error: %f, Max error: %f\n", report.rmserror, report.maxerror);
        break;
    }
    case MLP: {
        ae::mlpreport report;
        lut = build_lut_mlp(concat_points, &opts, &report);
        printf("Built LUT. RMS error: %f\n", report.rmserror);
    }
    }

    write_lut(lut, &opts);
    printf("Created LUT '%s'.\n", opts.output.c_str());
}
