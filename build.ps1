$build_lib = $false

$flags = "-O3"

if ($build_lib) {
    # zig c++ doesn't work with wildcards and with -c doesn't accept multiple inputs.
    foreach ($f in (dir ./alglib/*.cpp | select basename,extension)) {
        zig c++ $flags -c "./alglib/$($f.basename).cpp" -o "./target/alglib/$($f.basename).obj"
    }
}

zig c++ $flags -c ./src/main.cpp -o ./target/main.obj
zig c++ $flags ./target/main.obj $(dir ./target/alglib/*.obj) -o ./target/suntr.exe
