set -e
# g++ -std=c++11 -stdlib=libc++ -target arm64-apple-macos narivm.cpp -o narivm
# g++ -std=c++11 narivm.cpp -o narivm -O3
# 

g++ -std=c++11 -I/opt/homebrew/opt/boost/include \
    -L/opt/homebrew/opt/boost/lib \
    -lboost_system -lboost_filesystem \
    narivm.cpp -o narivm

kat -i cpptest.kat | ./narivm