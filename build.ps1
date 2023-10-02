$build_lib = $false

$flags = "-O3"

if ($build_lib) {
    # zig c++ -c doesn't work with multiple inputs.
    foreach ($f in (dir ./alglib/*.cpp | select basename,extension)) {
        zig c++ $flags -c "./alglib/$($f.basename).cpp" -o "./target/alglib/$($f.basename).obj"
    }
}

# Won't link concatenated for MinGW with -O3. TODO: concatenate dbg build.
zig c++ $flags -I . -c ./src/main.cpp -o ./target/main.obj
zig c++ $flags ./target/main.obj (dir ./target/alglib/*.obj) -o ./target/sdfit.exe
