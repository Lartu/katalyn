set -e

# Compile the NariVM
echo "Compiling NariVM"
g++ -std=c++17 narivm.cpp -c -o narivm -O3 --verbose
#g++ -std=c++11 -I/opt/homebrew/opt/boost/include -L/opt/homebrew/opt/boost/lib -lboost_system -lboost_filesystem narivm.o -o narivm -O3
echo "Linking NariVM"
g++ -std=c++17 narivm.o -o narivm -O3 --verbose
mv narivm ~/bin/narivm
