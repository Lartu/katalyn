set -e

# Compile the Katalyn Compiler (Python3)
python3.11 -m nuitka --standalone --onefile --deployment kat.py
rm -rf kat.build
rm -rf kat.dist
rm -rf kat.onefile-build
mv kat.bin ~/bin/kat

# Compile the NariVM
g++ -std=c++11 -I/opt/homebrew/opt/boost/include \
    -L/opt/homebrew/opt/boost/lib \
    -lboost_system -lboost_filesystem \
    narivm.cpp -o narivm
mv narivm ~/bin/narivm

echo 'print("Katalyn compilation successful!");' | kat -s