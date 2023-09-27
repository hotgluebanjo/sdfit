$build_lib = $false

if ($build_lib) {
    zig c++ -O3 -c ./alglib/alglibinternal.cpp -o .\target\alglib\alglibinternal.obj
    zig c++ -O3 -c ./alglib/alglibmisc.cpp -o .\target\alglib\alglibmisc.obj
    zig c++ -O3 -c ./alglib/ap.cpp -o .\target\alglib\ap.obj
    zig c++ -O3 -c ./alglib/dataanalysis.cpp -o .\target\alglib\dataanalysis.obj
    zig c++ -O3 -c ./alglib/diffequations.cpp -o .\target\alglib\diffequations.obj
    zig c++ -O3 -c ./alglib/fasttransforms.cpp -o .\target\alglib\fasttransforms.obj
    zig c++ -O3 -c ./alglib/integration.cpp -o .\target\alglib\integration.obj
    zig c++ -O3 -c ./alglib/interpolation.cpp -o .\target\alglib\interpolation.obj
    zig c++ -O3 -c ./alglib/kernels_avx2.cpp -o .\target\alglib\kernels_avx2.obj
    zig c++ -O3 -c ./alglib/kernels_fma.cpp -o .\target\alglib\kernels_fma.obj
    zig c++ -O3 -c ./alglib/kernels_sse2.cpp -o .\target\alglib\kernels_sse2.obj
    zig c++ -O3 -c ./alglib/linalg.cpp -o .\target\alglib\linalg.obj
    zig c++ -O3 -c ./alglib/optimization.cpp -o .\target\alglib\optimization.obj
    zig c++ -O3 -c ./alglib/solvers.cpp -o .\target\alglib\solvers.obj
    zig c++ -O3 -c ./alglib/specialfunctions.cpp -o .\target\alglib\specialfunctions.obj
    zig c++ -O3 -c ./alglib/statistics.cpp -o .\target\alglib\statistics.obj
}

zig c++ -O3 -c .\src\main.cpp -o .\target\main.obj

zig c++ -O3 -o .\target\rbf-interp.exe `
    .\target\main.obj `
    .\target\alglib\alglibinternal.obj `
    .\target\alglib\alglibmisc.obj `
    .\target\alglib\ap.obj `
    .\target\alglib\dataanalysis.obj `
    .\target\alglib\diffequations.obj `
    .\target\alglib\fasttransforms.obj `
    .\target\alglib\integration.obj `
    .\target\alglib\interpolation.obj `
    .\target\alglib\kernels_avx2.obj `
    .\target\alglib\kernels_fma.obj `
    .\target\alglib\kernels_sse2.obj `
    .\target\alglib\linalg.obj `
    .\target\alglib\optimization.obj `
    .\target\alglib\solvers.obj `
    .\target\alglib\specialfunctions.obj `
    .\target\alglib\statistics.obj
