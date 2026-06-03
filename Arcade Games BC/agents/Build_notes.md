Build Notes :



If the build folder ever disappears or CMake needs regenerating:

cmake -S . -B build
cmake --build build
./build/retro_arcade


If you changed code and want to rebuild first:

cmake --build build
./build/retro_arcade


