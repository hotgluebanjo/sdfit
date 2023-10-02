# Wildcards don't work of course.
zig c++ -I . -O3 --target=x86_64-linux (dir ./src/*.cpp) (dir ./alglib/*.cpp) -o release/linux/sdfit
zig c++ -I . -O3 --target=x86_64-macos (dir ./src/*.cpp) (dir ./alglib/*.cpp) -o release/macos/sdfit
