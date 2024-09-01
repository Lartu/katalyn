set -e

# Compile the NariVM
g++ -std=c++11 -I/opt/homebrew/opt/boost/include \
    -L/opt/homebrew/opt/boost/lib \
    -lboost_system -lboost_filesystem \
    narivm.cpp -o narivm -O3
mv narivm ~/bin/narivm
