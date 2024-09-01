set -e

# Compile the NariVM
echo "Compiling NariVM"
g++ -std=c++17 narivm.cpp -c -o narivm.o -O1

echo "Building Tiny Process Library"
processdir="tiny-process-library"
g++ -std=c++17 "$processdir/process.cpp" -c -o process.o -O1
g++ -std=c++17 "$processdir/process_unix.cpp" -c -o process_unix.o -O1

echo "Linking NariVM"
g++ -std=c++17 narivm.o process.o process_unix.o -o narivm -O1 -pthread
mv narivm ~/bin/narivm
