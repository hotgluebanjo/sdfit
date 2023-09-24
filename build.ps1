if (-not (test-path target)) {
    md target
}

zig c++ -o ./target/rbf-interp.exe -I ./alglib/ `
    ./src/main.cpp `
    ./alglib/alglibinternal.cpp `
    ./alglib/alglibmisc.cpp `
    ./alglib/ap.cpp `
    ./alglib/dataanalysis.cpp `
    ./alglib/diffequations.cpp `
    ./alglib/fasttransforms.cpp `
    ./alglib/integration.cpp `
    ./alglib/interpolation.cpp `
    ./alglib/kernels_avx2.cpp `
    ./alglib/kernels_fma.cpp `
    ./alglib/kernels_sse2.cpp `
    ./alglib/linalg.cpp `
    ./alglib/optimization.cpp `
    ./alglib/solvers.cpp `
    ./alglib/specialfunctions.cpp `
    ./alglib/statistics.cpp
