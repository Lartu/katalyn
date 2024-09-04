set -e

optimization="-O1"

# Compile the NariVM
echo "Compiling NariVM"
g++ -c -o user_code.o user_code.S
g++ -std=c++17 narivm.cpp -c -o narivm.o $optimization

echo "Building Tiny Process Library"
processdir="tiny-process-library"
g++ -std=c++17 "$processdir/process.cpp" -c -o process.o $optimization
g++ -std=c++17 "$processdir/process_unix.cpp" -c -o process_unix.o $optimization

echo "Linking NariVM"
g++ -std=c++17 narivm.o process.o process_unix.o user_code.o -o narivm_program -pthread

echo "Cleaning up"
rm *.o
