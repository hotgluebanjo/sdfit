# First
#    build.ps1 lib
# then
#    build.ps1 release x.y.z
# or
#    build.ps1
# for debug build.

if ($args[0] -eq "lib") {
    # zig c++ -c doesn't work with multiple inputs.
    foreach ($f in (dir ./alglib/*.cpp | select name)) {
        zig c++ -O3 -c "./alglib/$($f.name).cpp" -o "./target/alglib/$($f.name).obj"
    }
} elseif ($args[0] -eq "release") {
    if ($args.length -eq 1) {
        echo "Missing version"
        exit
    }

    $version = $args[1]

    # Won't link concatenated with -O3. MinGW issue.
    zig c++ -I . -O3 -c ./src/main.cpp -o ./target/main.obj
    zig c++ -O3 ./target/main.obj (dir ./target/alglib/*.obj) -o ./release/windows/sdfit.exe

    # Wildcards don't work of course.
    zig c++ -I . -O3 --target=x86_64-macos (dir ./src/*.cpp) (dir ./alglib/*.cpp) -o release/macos/sdfit
    zig c++ -I . -O3 --target=x86_64-linux (dir ./src/*.cpp) (dir ./alglib/*.cpp) -o release/linux/sdfit

    compress-archive release/windows/sdfit.exe "release/sdfit-$version-windows.zip"
    tar -czf "./release/sdfit-$version-macos.tar.gz" ./release/macos/sdfit
    tar -czf "./release/sdfit-$version-linux.tar.gz" ./release/linux/sdfit
} else {
    zig c++ -I . ./src/main.cpp (dir ./target/alglib/*.obj) -o ./target/sdfit.exe
}
