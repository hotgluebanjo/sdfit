#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interpolation.h"
#include "stdafx.h"

using namespace alglib;

static const char *HELP_INFO =
"SD Interp CLI v0.1.0\n\
https://github.com/hotgluebanjo\n\n\
USAGE: sd_interp <source> <target> [OPTIONS]\n\n\
OPTIONS:\n\n\
-h : Help\n\
-d : Delimiter\n\
-s : Basis size (default is TODO)";

struct Config {
    // Source data set.
    std::string source_file;

    // Target data set.
    std::string target_file;

    // Size of Gaussian basis.
    double size;

    // Dataset value delimiter:
    // - ' '
    // - ','
    // - '\t'
    char delimiter;
};

rbfmodel build_model() {
    //
    // Suppose that we have set of 2-dimensional points with associated VECTOR
    // function values, and we want to build a RBF model using our data.
    // 
    // Typical sequence of steps is given below:
    // 1. we create RBF model object
    // 2. we attach our dataset to the RBF model and tune algorithm settings
    // 3. we rebuild RBF model using new data
    // 4. we use RBF model (evaluate, serialize, etc.)
    //
    real_1d_array x;
    real_1d_array y;

    //
    // Step 1: RBF model creation.
    //
    // We have to specify dimensionality of the space (equal to 2) and
    // dimensionality of the function (2-dimensional vector function).
    //
    // New model is empty - it can be evaluated,
    // but we just get zero value at any point.
    //
    rbfmodel model;
    rbfcreate(3, 3, model);

    x = "[+1,+1]";
    rbfcalc(model, x, y);
    printf("%s\n", y.tostring(2).c_str()); // EXPECTED: [0.000,0.000]

    //
    // Step 2: we add dataset.
    //
    // XY arrays containt four points:
    // * (x0,y0) = (+1,+1), f(x0,y0)=(0,-1)
    // * (x1,y1) = (+1,-1), f(x1,y1)=(-1,0)
    // * (x2,y2) = (-1,-1), f(x2,y2)=(0,+1)
    // * (x3,y3) = (-1,+1), f(x3,y3)=(+1,0)
    //
    real_2d_array xy = "[[+1,+1,0,-1],[+1,-1,-1,0],[-1,-1,0,+1],[-1,+1,+1,0]]";
    rbfsetpoints(model, xy);

    // We added points, but model was not rebuild yet.
    // If we call rbfcalc(), we still will get 0.0 as result.
    rbfcalc(model, x, y);
    printf("%s\n", y.tostring(2).c_str()); // EXPECTED: [0.000,0.000]

    //
    // Step 3: rebuild model
    //
    // We use hierarchical RBF algorithm with following parameters:
    // * RBase - set to 1.0
    // * NLayers - three layers are used (although such simple problem
    //   does not need more than 1 layer)
    // * LambdaReg - is set to zero value, no smoothing is required
    //
    // After we've configured model, we should rebuild it -
    // it will change coefficients stored internally in the
    // rbfmodel structure.
    //
    rbfreport rep;
    rbfsetalgohierarchical(model, 1.0, 3, 0.0);
    rbfbuildmodel(model, rep);
    printf("%d\n", int(rep.terminationtype)); // EXPECTED: 1

    return model;

    //
    // Step 4: model was built
    //
    // After call of rbfbuildmodel(), rbfcalc() will return
    // value of the new model.
    //
    rbfcalc(model, x, y);
}

// void parse_args(char **args, int n_args, Config *opts) {
//     if (n_args == 1) {
//         printf("%s", HELP_INFO);
//         exit(0);
//     }

//     for (int i = 1; i < n_args; i += 1) {
//         bool last_arg;
//         if (i+1 == n_args) {
//             last_arg = true;
//         } else {
//             last_arg = false;
//         }

//         if (args[i][0] == '-') {
//             switch (args[i][1]) {
//             case 'd': {
//                 if (!last_arg) opts->delimiter = args[i+1][0];
//                 break;
//             }
//             case 'h':
//                 printf("%s", HELP_INFO);
//                 exit(0);
//             case 's':
//                 if (!last_arg) opts->size = atof(args[i+1]);
//                 break;
//             default:
//                 fprintf(stderr, "unknown option: %s", args[i]);
//                 exit(1);
//             }
//         }
//     }

//     if (!source_set) {
//         opts->source_file = std::string(args[i]);
//         source_set = true;
//     }
//     if (!target_set) {
//         opts->target_file = std::string(args[i]);
//         target_set = true;
//     }
// }

int main(int argc, char **argv) {
    Config opts = {
        .source_file = "yo",
        .target_file = "yo",
        .size = 3.0,
        .delimiter = ' ',
    };

    //
    // Suppose that we have set of 2-dimensional points with associated VECTOR
    // function values, and we want to build a RBF model using our data.
    // 
    // Typical sequence of steps is given below:
    // 1. we create RBF model object
    // 2. we attach our dataset to the RBF model and tune algorithm settings
    // 3. we rebuild RBF model using new data
    // 4. we use RBF model (evaluate, serialize, etc.)
    //
    real_1d_array x;
    real_1d_array y;

    //
    // Step 1: RBF model creation.
    //
    // We have to specify dimensionality of the space (equal to 2) and
    // dimensionality of the function (2-dimensional vector function).
    //
    // New model is empty - it can be evaluated,
    // but we just get zero value at any point.
    //
    rbfmodel model;
    rbfcreate(2, 2, model);

    x = "[+1,+1]";
    rbfgridcalc3v();
    //
    // Step 2: we add dataset.
    //
    // XY arrays containt four points:
    // * (x0,y0) = (+1,+1), f(x0,y0)=(0,-1)
    // * (x1,y1) = (+1,-1), f(x1,y1)=(-1,0)
    // * (x2,y2) = (-1,-1), f(x2,y2)=(0,+1)
    // * (x3,y3) = (-1,+1), f(x3,y3)=(+1,0)
    //
    // real_2d_array xy = "[[+1,+1,0,-1],[+1,-1,-1,0],[-1,-1,0,+1],[-1,+1,+1,0]]";
    real_2d_array xy;

    // Stuck with exceptions.
    try {
        read_csv("test_data/alglib_vector_space.txt", opts.delimiter, 0, xy);
    } catch (ap_error err) {
        std::cout << err.msg << '\n';
        return 1;
    }

    rbfsetpoints(model, xy);

    rbfreport rep;
    rbfsetalgohierarchical(model, 1.0, 3, 0.0);
    rbfbuildmodel(model, rep);
    printf("%d\n", int(rep.terminationtype)); // EXPECTED: 1

    rbfcalc(model, x, y);
    std::cout << y.tostring(2) << std::endl; // EXPECTED: [0.000,-1.000]
    return 0;
}
